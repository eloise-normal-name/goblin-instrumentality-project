param(
	[int]$ThresholdDays = 2,
	[string[]]$ProtectedBranches = @("main", "gh-pages"),
	[switch]$IncludeRemoteDeletes,
	[switch]$Apply,
	[switch]$SkipBranchCleanup,
	[switch]$SkipWorktreeCleanup
)

$ErrorActionPreference = "Stop"

function Assert-GitRepository {
	$null = & git rev-parse --is-inside-work-tree 2>$null
	if ($LASTEXITCODE -ne 0) {
		throw "Current directory is not inside a git repository."
	}
}

function Get-DefaultBranchName {
	$origin_head = (& git symbolic-ref --quiet --short refs/remotes/origin/HEAD 2>$null | Select-Object -First 1)
	if (-not [string]::IsNullOrWhiteSpace($origin_head)) {
		$segments = $origin_head.Trim().Split("/")
		return $segments[$segments.Length - 1]
	}

	$candidate = (& git remote show origin 2>$null | Select-String -Pattern "HEAD branch" | Select-Object -First 1)
	if ($candidate) {
		$default_branch = ($candidate.ToString().Split(":") | Select-Object -Last 1).Trim()
		if (-not [string]::IsNullOrWhiteSpace($default_branch)) {
			return $default_branch
		}
	}

	return "main"
}

function Get-MergedBranches {
	param(
		[string]$DefaultBranch
	)

	$merged = @{}
	$lines = & git branch --format "%(refname:short)" --merged $DefaultBranch
	foreach ($line in $lines) {
		if ([string]::IsNullOrWhiteSpace($line)) {
			continue
		}
		$merged[$line.Trim()] = $true
	}

	return $merged
}

function Get-LocalBranchCandidates {
	param(
		[int]$AgeThresholdDays,
		[string]$DefaultBranch,
		[string]$CurrentBranch,
		[string[]]$Protected,
		[hashtable]$MergedMap
	)

	$now_utc = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
	$protected_set = @{}
	foreach ($branch_name in $Protected + @($DefaultBranch, $CurrentBranch)) {
		if ([string]::IsNullOrWhiteSpace($branch_name)) {
			continue
		}
		$protected_set[$branch_name] = $true
	}

	$candidates = @()
	$format = "%(refname:short)|%(committerdate:unix)|%(upstream:short)|%(upstream:track)"
	$lines = & git for-each-ref --format $format refs/heads
	foreach ($line in $lines) {
		if ([string]::IsNullOrWhiteSpace($line)) {
			continue
		}

		$parts = $line.Split("|")
		$branch = $parts[0].Trim()
		if ($protected_set.ContainsKey($branch)) {
			continue
		}

		$commit_unix = [long]($parts[1].Trim())
		$upstream = if ($parts.Length -ge 3) { $parts[2].Trim() } else { "" }
		$track = if ($parts.Length -ge 4) { $parts[3].Trim() } else { "" }
		$age_days = [Math]::Floor(($now_utc - $commit_unix) / 86400)

		$is_old = $age_days -ge $AgeThresholdDays
		$is_merged = $MergedMap.ContainsKey($branch)
		$is_upstream_gone = $track -match "gone"
		$has_upstream = -not [string]::IsNullOrWhiteSpace($upstream)

		if (-not $is_old) {
			continue
		}

		if (-not ($is_merged -or $is_upstream_gone -or -not $has_upstream)) {
			continue
		}

		$reasons = @()
		if ($is_merged) {
			$reasons += "merged:$DefaultBranch"
		}
		if ($is_upstream_gone) {
			$reasons += "upstream-gone"
		}
		if (-not $has_upstream) {
			$reasons += "no-upstream"
		}
		$reasons += "age:${age_days}d"

		$remote_name = ""
		if ($upstream.StartsWith("origin/", [System.StringComparison]::OrdinalIgnoreCase)) {
			$remote_name = $upstream.Substring("origin/".Length)
		}

		$candidates += [PSCustomObject]@{
			Branch = $branch
			Upstream = $upstream
			AgeDays = $age_days
			Merged = $is_merged
			UpstreamGone = $is_upstream_gone
			RemoteBranch = $remote_name
			Reasons = ($reasons -join ",")
		}
	}

	return $candidates | Sort-Object @{ Expression = 'AgeDays'; Descending = $true }, @{ Expression = 'Branch'; Descending = $false }
}

function Get-WorktreeCandidates {
	param(
		[array]$BranchCandidates
	)

	$stale_branch_set = @{}
	foreach ($candidate in $BranchCandidates) {
		$stale_branch_set[$candidate.Branch] = $true
	}

	$current_path = (& git rev-parse --show-toplevel).Trim()
	$candidates = @()

	$raw_lines = & git worktree list --porcelain
	$entries = @()
	$current = @{}
	foreach ($line in $raw_lines) {
		if ([string]::IsNullOrWhiteSpace($line)) {
			if ($current.Count -gt 0) {
				$entries += [PSCustomObject]$current
				$current = @{}
			}
			continue
		}

		$kv = $line.Split(" ", 2)
		$key = $kv[0]
		$value = if ($kv.Length -gt 1) { $kv[1] } else { "" }
		$current[$key] = $value
	}
	if ($current.Count -gt 0) {
		$entries += [PSCustomObject]$current
	}

	foreach ($entry in $entries) {
		$path = [string]$entry.worktree
		if ([string]::IsNullOrWhiteSpace($path)) {
			continue
		}
		if ($path -eq $current_path) {
			continue
		}

		$path_exists = Test-Path $path
		$branch_ref = [string]$entry.branch
		$branch_name = ""
		if ($branch_ref -match "^refs/heads/") {
			$branch_name = $branch_ref.Substring("refs/heads/".Length)
		}

		$locked = -not [string]::IsNullOrWhiteSpace([string]$entry.locked)
		$prunable = -not [string]::IsNullOrWhiteSpace([string]$entry.prunable)
		$dirty = $false
		if ($path_exists) {
			$status = (& git -C $path status --porcelain 2>$null)
			$dirty = -not [string]::IsNullOrWhiteSpace(($status | Out-String).Trim())
		}

		$reasons = @()
		if (-not $path_exists) {
			$reasons += "missing-path"
		}
		if ($prunable) {
			$reasons += "prunable"
		}
		if (-not [string]::IsNullOrWhiteSpace($branch_name) -and $stale_branch_set.ContainsKey($branch_name)) {
			$reasons += "stale-branch:$branch_name"
		}

		if ($reasons.Count -eq 0) {
			continue
		}

		$candidates += [PSCustomObject]@{
			Path = $path
			Branch = $branch_name
			Locked = $locked
			Dirty = $dirty
			Exists = $path_exists
			Reasons = ($reasons -join ",")
		}
	}

	return $candidates | Sort-Object Path
}

function Remove-LocalBranch {
	param(
		[pscustomobject]$Candidate
	)

	if ($Candidate.Merged) {
		& git branch -d $Candidate.Branch
	} else {
		& git branch -D $Candidate.Branch
	}
}

function Remove-RemoteBranch {
	param(
		[pscustomobject]$Candidate,
		[hashtable]$ProtectedSet
	)

	if (-not $Candidate.Merged) {
		return
	}
	if ([string]::IsNullOrWhiteSpace($Candidate.RemoteBranch)) {
		return
	}
	if ($ProtectedSet.ContainsKey($Candidate.RemoteBranch)) {
		return
	}

	$null = & git ls-remote --exit-code --heads origin $Candidate.RemoteBranch 2>$null
	if ($LASTEXITCODE -ne 0) {
		return
	}

	& git push origin --delete $Candidate.RemoteBranch
}

if ($ThresholdDays -lt 0) {
	throw "ThresholdDays must be >= 0."
}

Assert-GitRepository

& git fetch --all --prune
if ($LASTEXITCODE -ne 0) {
	throw "git fetch --all --prune failed."
}

$default_branch = Get-DefaultBranchName
$current_branch = (& git branch --show-current).Trim()
$merged_map = Get-MergedBranches -DefaultBranch $default_branch

$branch_candidates = @()
if (-not $SkipBranchCleanup) {
	$branch_candidates = Get-LocalBranchCandidates -AgeThresholdDays $ThresholdDays -DefaultBranch $default_branch -CurrentBranch $current_branch -Protected $ProtectedBranches -MergedMap $merged_map
}

$worktree_candidates = @()
if (-not $SkipWorktreeCleanup) {
	$worktree_candidates = Get-WorktreeCandidates -BranchCandidates $branch_candidates
}

Write-Output "Cleanup mode: $(if ($Apply) { 'apply' } else { 'dry-run' })"
Write-Output "ThresholdDays: $ThresholdDays"
Write-Output "Default branch: $default_branch"
Write-Output "Current branch: $current_branch"
Write-Output "Protected branches: $($ProtectedBranches -join ', ')"
Write-Output ""

Write-Output "Branch candidates: $($branch_candidates.Count)"
if ($branch_candidates.Count -gt 0) {
	$branch_candidates | Format-Table Branch, AgeDays, Upstream, Merged, UpstreamGone, Reasons -AutoSize | Out-String | Write-Output
}

Write-Output "Worktree candidates: $($worktree_candidates.Count)"
if ($worktree_candidates.Count -gt 0) {
	$worktree_candidates | Format-Table Path, Branch, Exists, Locked, Dirty, Reasons -AutoSize | Out-String | Write-Output
}

if (-not $Apply) {
	Write-Output "Dry-run complete. Re-run with -Apply to execute deletions."
	exit 0
}

$protected_set = @{}
foreach ($name in $ProtectedBranches + @($default_branch, $current_branch)) {
	if (-not [string]::IsNullOrWhiteSpace($name)) {
		$protected_set[$name] = $true
	}
}

foreach ($candidate in $worktree_candidates) {
	if ($candidate.Locked) {
		Write-Output "Skipped locked worktree: $($candidate.Path)"
		continue
	}
	if ($candidate.Dirty) {
		Write-Output "Skipped dirty worktree: $($candidate.Path)"
		continue
	}

	if (-not $candidate.Exists) {
		continue
	}

	& git worktree remove $candidate.Path
	if ($LASTEXITCODE -ne 0) {
		throw "Failed removing worktree: $($candidate.Path)"
	}
}

& git worktree prune
if ($LASTEXITCODE -ne 0) {
	throw "git worktree prune failed."
}

foreach ($candidate in $branch_candidates) {
	Remove-LocalBranch -Candidate $candidate
	if ($LASTEXITCODE -ne 0) {
		throw "Failed deleting local branch: $($candidate.Branch)"
	}

	if ($IncludeRemoteDeletes) {
		Remove-RemoteBranch -Candidate $candidate -ProtectedSet $protected_set
		if ($LASTEXITCODE -ne 0) {
			throw "Failed deleting remote branch: $($candidate.RemoteBranch)"
		}
	}
}

Write-Output "Cleanup complete."
