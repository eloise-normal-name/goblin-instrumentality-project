---
name: "Stage Changelist"
description: "Review changed files since the last highlights commit and stage relevant files; produce highlights HTML and commit message."
tools: ['vscode', 'execute', 'read', 'agent', 'edit', 'search', 'web', 'memory', 'todo']
argument-hint: "Do a dry-run first: list diffs and proposed stages, run build, then ask to confirm before staging. Update the canonical highlights file via git (modify and commit the same file); do not create separate dated filenames."
---

# Review Changed Files & Stage

Review the changed files in this repository since the last highlights commit (look it up with git) and:

1. Stage relevant changed files
1. Summarize what changed and why (compare HEAD vs the last highlights commit)
1. Halt if there are signficant errors requiring more than a trivial fix.
1. Ensure correct formatting and styling on everything changed and every modification you do or propose
1. Check build status
1. Suggest a concise commit message following conventional commits and add it to vs code's commit message box using git.setCommitMessage
1. Make a highlights page in html at `refactor-highlights.html` (update this canonical file via git; do not create dated copies)
    * summarize the changes and their motivation
        * explain high level goals so far and how these changes fit into that
    * snippets showing what happened, rendered nicely into cute html cards
        * expandable summaries
        * relevant colored code if available
        * links if relevant
    * update log inside highlights page with build stats
        * preserve all previous build-log entries already present in the existing `refactor-highlights.html`
        * size of minsizerel binaries created
        * total source lines in /src using cloc (not per-file counts)
    * proposed next steps
    * open it up to read.
