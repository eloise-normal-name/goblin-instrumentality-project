---
status: active
owner: copilot
related: headless-regression-testing
---

# Golden Frame Comparison: H.264 Output Regression Testing

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

`/.agent/PLANS.md` is checked into this repository and this document is maintained in accordance with that file.

## Purpose / Big Picture

Establish a repeatable regression test that runs the app in headless mode, extracts individual frames from the resulting `output.h264`, and compares them to committed golden images. Any render change that visually alters output will be caught when the extracted frames no longer match the goldens. A baseline mode lets a developer deliberately update the goldens when a change is intentional.

## Progress

- [ ] Audit build output structure and confirm headless run produces `output.h264` in a stable location.
- [ ] Decide ffmpeg procurement strategy (system PATH check vs. bundled winget install step in script).
- [ ] Create `tests/golden/` directory and document its purpose in `README.md`.
- [ ] Write `scripts/test-frame-compare.ps1` with `--baseline` and `--compare` modes.
- [ ] Run baseline to produce initial golden images from current output.
- [ ] Commit goldens and script; validate a clean compare run returns exit 0.
- [ ] Validate a deliberate render change causes compare to return non-zero and report differing frames.
- [ ] Record outcomes and close plan.

## Surprises & Discoveries

<!-- Fill in as work proceeds -->

## Decision Log

- Decision: Use `ffmpeg` as an external CLI tool (not a linked library) for H.264 → BMP frame extraction.
  Rationale: The "no external libraries" rule applies to C++ source dependencies. Scripts may invoke standalone CLI tools. `ffmpeg` is the de-facto standard for this job, avoids writing a Media Foundation decoder from scratch, and produces predictable per-frame BMP output. Windows Media Foundation is an alternative if ffmpeg cannot be assumed on all machines.
  Date/Author: 2026-02-19 / copilot

- Decision: Store golden images as lossless BMP files in `tests/golden/` committed to the repository.
  Rationale: BMP is a Windows-native format, byte-stable, and requires no external decoder. PNG would need a third-party encoder or a Windows Imaging Component wrapper. BMP is sufficient and simpler.
  Date/Author: 2026-02-19 / copilot

- Decision: Frame comparison is pixel-exact (byte-level BMP comparison after stripping the BITMAPFILEHEADER and BITMAPINFOHEADER timestamps/reserved fields that ffmpeg may vary).
  Rationale: The renderer is deterministic on the same GPU in headless mode. Non-exact comparisons introduce tolerance tuning complexity. If GPU variability proves to be an issue, a per-channel tolerance flag can be added later.
  Date/Author: 2026-02-19 / copilot

- Decision: Compare only the first 30 frames (the full headless run).
  Rationale: Headless mode exits after exactly 30 submitted frames. Extracting all frames from the output covers the full deterministic output without over-specifying a frame count.
  Date/Author: 2026-02-19 / copilot

## Outcomes & Retrospective

Pending implementation.

## Context and Orientation

`src/main.cpp` parses `--headless` and passes it to `App`. `src/app.ixx` runs the frame loop and exits after `frames_submitted >= 30` in headless mode. `src/encoder/bitstream_file_writer.cpp` writes the encoded bitstream to `output.h264` (hardcoded path relative to the working directory, which is the repo root when launched from VS Code or CMake Tools).

The comparison pipeline is entirely outside the C++ source: it lives in a PowerShell script that orchestrates the headless run, ffmpeg extraction, and image comparison. No new C++ source files are needed unless a native BMP pixel-diff tool is preferred later.

## Plan of Work

**Step 1 – Headless run and output location audit**

Read `src/app.ixx` and `src/encoder/bitstream_file_writer.cpp` to confirm the exact `output.h264` write path and that the file is fully flushed before process exit. Verify that a headless run from `bin/Debug/` writes to the repo root (or `bin/Debug/`) so the script targets the right file.

**Step 2 – Golden directory scaffold**

Create `tests/golden/` with a `.gitkeep` placeholder. Add a one-sentence `tests/README.md` explaining that BMP files here are committed baselines and are updated by running `scripts/test-frame-compare.ps1 --baseline`.

**Step 3 – Write `scripts/test-frame-compare.ps1`**

The script accepts one of two modes:

- `--baseline`: Runs the app headless, extracts all frames with ffmpeg, writes them to `tests/golden/frame_%03d.bmp`, replacing any existing files. Exits 0.
- `--compare` (default): Runs the app headless, extracts frames to a temp directory, compares each against the corresponding golden by byte-comparing the pixel data section of the BMP (skipping the 54-byte header to avoid timestamp or reserved-field noise). Reports each mismatching frame. Exits 0 on full match, 1 on any mismatch.

Internal structure of the script:

```
1. Locate exe: bin/Debug/goblin-stream.exe (fail fast if missing).
2. Check ffmpeg is on PATH; if not, print guidance and exit 2.
3. Run: & "$exe" --headless   (via agent-wrap.ps1 with TimeoutSec 60)
4. Locate output.h264 (check both repo root and bin/Debug/).
5. Run: ffmpeg -y -i output.h264 -pix_fmt bgr24 "$frameDir/frame_%03d.bmp"
6. Count extracted frames; fail if 0.
7. --baseline: copy frames to tests/golden/; report count; exit 0.
8. --compare:  foreach frame, Compare-BmpPixels golden candidate; accumulate failures.
9. Print summary; exit code = number of failed frames clamped to 1.
```

`Compare-BmpPixels` is a helper function in the same script that reads both files as byte arrays, skips the 54-byte BMP file+info header, and does a direct array comparison.

**Step 4 – Baseline run**

Run the script in `--baseline` mode to populate `tests/golden/` from the current renderer output. Inspect a few frames visually (open in Windows Photo Viewer) to confirm they show the expected mesh output.

**Step 5 – Validation**

Run the script in `--compare` mode against the freshly written goldens. Confirm exit 0 and "30/30 frames matched" output. Then deliberately alter a constant in a shader or geometry and re-run compare to verify exit 1 and a useful per-frame diff report. Revert the deliberate change.

**Step 6 – Update README and close plan**

Add a "Regression Testing" section to `README.md` documenting the two script modes. Update Progress checklist and Outcomes section in this document.

## Concrete Steps

Working directory: `C:\Users\Admin\goblin-stream`

1. Read `src/app.ixx` lines around `BitstreamFileWriter` construction to confirm output path.
2. Read `src/encoder/bitstream_file_writer.cpp` destructor to confirm flush-on-close.
3. Create `tests/golden/.gitkeep` and `tests/README.md`.
4. Create `scripts/test-frame-compare.ps1` (full implementation).
5. Run baseline:
   ```
   powershell -ExecutionPolicy Bypass -File scripts/test-frame-compare.ps1 --baseline
   ```
6. Run compare:
   ```
   powershell -ExecutionPolicy Bypass -File scripts/test-frame-compare.ps1 --compare
   ```
   Expected: exit 0, "30/30 frames matched".
7. Commit `tests/golden/*.bmp`, `tests/README.md`, `scripts/test-frame-compare.ps1`, and this execplan.
8. Update `README.md` with regression testing section.
9. Mark all Progress items complete and fill Outcomes section.
