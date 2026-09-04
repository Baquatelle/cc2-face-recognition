# Testing — cc2-face-recognition

This document defines the test scenarios for the `facerec` application. Tests cover the full feature
surface: detection, recognition, video, webcam, tracking, liveness, display filters, self-diagnostics,
and edge cases.

Scenarios were defined before any result was recorded, so that outcomes could not be written to fit.
Actual results are in [RESULTS.md](RESULTS.md).

**Total:** 51 test cases across 9 categories
**Tester:** Modar Issa
**Environment:** Windows 11 / MSYS2 MinGW64 / OpenCV 4.13.0

| Category | Tests | Mode | Evidence |
|---|---|---|---|
| D — Detection (image) | 7 | GUI + headless | Screenshots |
| R — Recognition | 7 | GUI + headless | Screenshots |
| V — Video | 4 | GUI | Screenshots |
| W — Webcam | 5 | GUI | Screenshots |
| T — Face tracking | 5 | GUI | Screenshots |
| L — Liveness detection | 8 | GUI + headless | Screenshots + logs |
| F — Frame filter | 4 | GUI | Screenshots |
| S — Self-test | 5 | Headless | Terminal output |
| E — Edge cases | 6 | GUI | Screenshots |
| **Total** | **51** | | |

> Test IDs match `cc2fr_Phases_0103_Contribution_Documentation.docx`, which is the authoritative
> test plan. Anything referring to these IDs should agree with that document.
>
> **One exception.** Category F was added on 29 August 2026, after Commit 4 integrated `FrameFilter`
> into the app. Those four IDs are not in the phases document, which was written before the feature
> existed. They need adding there before it is delivered.

---

## How to run

All commands run in an MSYS2 MinGW64 shell.

From the **repo root**:

```bash
python3 scripts/build.py --check          # built-in diagnostics, covers S1–S5
python3 scripts/run_headless_tests.py     # every headless test, PASS/FAIL with measured numbers
python3 scripts/build.py --run            # launch the GUI for the interactive tests
```

From `facerec/bin/` for individual headless tests:

```bash
./facerec.exe --detect samples/messi5.jpg
./facerec.exe --identify samples/messi-worldcup.jpg
./facerec.exe --liveness-replay C:/Users/<you>/cc2-face-recognition/test/videos/Abel_live.mp4
```

**Two different path rules, which is easy to trip over.** `--detect` and `--identify` resolve their
argument under `facerec/bin/data/`, so pass `samples/messi5.jpg`. Passing `data/samples/messi5.jpg`
fails, because the prefix is added anyway and you get `data\data/samples/...`. `--liveness-replay`
hands the path straight to OpenCV with no prefix, so it needs a full path.

`scripts/run_headless_tests.py` covers S1–S5, D4, D5, R1, R2, L7 and L7b. The rest are interactive
and have to be run by hand.

---

## Category D — Detection (image mode)

**Goal:** confirm YuNet correctly detects faces under different conditions.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| D1 | Single face — clean | Open app → open image… → load `samples/messi5.jpg` | 1 face detected, box drawn, "Faces: 1" |
| D2 | Group photo | Load `samples/group.pgm` | Multiple faces detected, count matches visible faces |
| D3 | No face | Load any landscape or object-only image | "Faces: 0", no boxes drawn |
| D4 | Headless detect — single | `./facerec.exe --detect samples/messi5.jpg` | Prints face count, box coordinates, confidence ≥ 0.6 |
| D5 | Headless detect — group | `./facerec.exe --detect samples/group.pgm` | Prints correct count and a box for each face |
| D6 | Conf threshold low | Load the group photo → drag conf threshold down to ~0.4 | More faces detected as the bar drops |
| D7 | Conf threshold high | Drag conf threshold up above the default, to ~0.7 | Fewer or no detections on marginal faces |

---

## Category R — Recognition (gallery matching)

**Goal:** confirm SFace embeddings identify known faces and return "unknown" for everyone else.

Gallery ships with **Messi** and **Ronaldo**. Once `gallery/Modar/` is added it holds 7 embeddings
across 3 people, which `--identify` reports on startup.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| R1 | Known person — Messi | `./facerec.exe --identify samples/messi-worldcup.jpg` | Name label matches the gallery entry, score above match threshold |
| R2 | Known person — Ronaldo | `./facerec.exe --identify samples/ronaldo-worldcup.jpg` | Name label matches the gallery entry, score above match threshold |
| R3 | Unknown face | Load an image of someone not in the gallery | Label shows "unknown" |
| R4 | Match threshold — lower | Drag match threshold down to ~0.2 on a borderline face | More faces get matched, possibly wrong ones |
| R5 | Match threshold — higher | Drag match threshold up to ~0.5 | Fewer matches; a previously matched face may become "unknown" |
| R6 | Enrol yourself live | Webcam → enable tracking → press `A` → enter `#id` → type a name | Face is labelled with that name |
| R7 | Gallery load | Click **load gallery…** → point at a custom folder | Gallery reloads, new people recognised |

---

## Category V — Video mode

**Goal:** confirm the pipeline runs on a video file.

> GUI video uses Windows DirectShow and needs the K-Lite Codec Pack for MP4 playback. Headless
> `--liveness-replay` uses OpenCV directly and needs no codecs.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| V1 | Open video file | open video… → load an MP4 with faces | Boxes appear on detected faces, count updates per frame |
| V2 | Recognition on video | Load a video of someone in the gallery | Name label follows the face across frames |
| V3 | Multiple faces in video | Load a video with more than one face | Multiple boxes, each with a correct or "unknown" label |
| V4 | FPS / performance | Watch the status bar during playback | Frame rate stable, app does not freeze |

---

## Category W — Webcam mode

**Goal:** confirm live webcam detection works with all features enabled.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| W1 | Basic webcam detection | Enable the webcam toggle | Face detected in the live feed, box and landmarks visible |
| W2 | Multiple webcams | With two cameras attached, press `[` / `]` | Switches camera, device name updates in the status bar |
| W3 | Refresh webcams | Click **refresh webcams** | Devices are re-detected and listed |
| W4 | Face enters / leaves frame | Cover the camera, then uncover | Detection disappears and reappears cleanly |
| W5 | FPS check | Observe the status bar in webcam mode | Around 8–15 fps on CPU is normal |

---

## Category T — Face tracking

**Goal:** confirm the IoU/centroid tracker assigns stable IDs and handles dropouts.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| T1 | Stable ID on webcam | Enable tracking → sit still | Face gets a `#id`, and it does not flicker |
| T2 | ID persists through movement | Move head left, right, up, down | The same `#id` stays on the face |
| T3 | ID survives short dropout | Cover the camera for 1–2 s, then uncover | The same `#id` is reassigned, not a new one |
| T4 | Two faces — separate IDs | Two faces in frame with tracking on | Each gets its own stable `#id` |
| T5 | Tracking off vs on | Toggle tracking off during webcam | No `#id` label, and the liveness toggle greys out |

---

## Category L — Liveness detection

**Goal:** confirm blink and mouth-movement detection distinguishes live faces from photos.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| L1 | LIVE flag — blink | Enable tracking + liveness → blink normally | `LIVE` appears after a blink |
| L2 | LIVE flag — mouth movement | Keep eyes open, talk or yawn | `LIVE` appears from mouth movement |
| L3 | PHOTO? flag — still face | Enable tracking + liveness → sit completely still ~7 s | `PHOTO?` appears after the timeout |
| L4 | PHOTO? flag — phone screen | Hold a photo or phone showing a face to the webcam | `PHOTO?` appears, no blinks detected |
| L5 | Glasses interference | Wear glasses, try to trigger `LIVE` | Note whether blinks register; glasses may interfere |
| L6 | Liveness requires tracking | Disable tracking, try to enable liveness | Liveness toggle is inactive |
| L7 | Liveness replay — live video | `./facerec.exe --liveness-replay <full path>/test/videos/Abel_live.mp4` | Per-frame signals printed; `LIVE` leads the summary |
| L7b | Liveness replay — photo video | `./facerec.exe --liveness-replay <full path>/test/videos/Abel_photo.mp4` | `PHOTO?` leads the summary; few or no blinks |

---

## Category F — Frame filter (display modes)

**Goal:** confirm the four display modes render correctly, and that detection and recognition keep
running on the unfiltered frame underneath rather than on what is shown.

Cycle order is Normal → Greyscale → Edges → Face Crop → Normal, driven by the `F` key. The active
mode is printed bottom-left as `Filter [F]: <mode>`.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| F1 | Normal — pass-through | Webcam on, tracking on → press `F` until the label reads `Normal` | Frame drawn unchanged; box, landmarks and name label present |
| F2 | Greyscale (7.3a) | Press `F` to reach `Greyscale` | Colour removed; box, landmarks and name label still drawn |
| F3 | Edges (7.3b / 7.3c) | Press `F` to reach `Edges` | White Canny outlines on black; box, landmarks and name label still drawn on top |
| F4 | Face crop (7.3d) | Press `F` to reach `Face Crop` | View zooms to the largest detected face, keeping the frame's proportions so the face is not stretched; overlays suppressed; `Faces:` keeps updating |

Overlays are hidden in F4 by design: the frame is zoomed, so boxes drawn in original frame
coordinates would land in the wrong place. A `Faces:` count that keeps updating is what shows
detection is still running.

---

## Category S — Self-test and diagnostics

**Goal:** confirm the automated suite passes and the headless modes work.

Run `python3 scripts/build.py --check` from the repo root, then read its output.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| S1 | Full self-test | `python3 scripts/build.py --check` | Exits with code 0, checks printed as passing |
| S2 | OpenCV version | Observe the self-test output | Confirms OpenCV ≥ 4.5.4 |
| S3 | Model files present | Observe the self-test output | YuNet and SFace ONNX models found in `data/models` |
| S4 | Tracker unit tests | Observe the self-test output | Synthetic tracking scenarios all pass |
| S5 | Liveness unit tests | Observe the self-test output | Synthetic blink and mouth state-machine tests all pass |

---

## Category E — Edge cases

**Goal:** test robustness under unusual or stressful conditions.

| ID | Test | Steps | Expected result |
|----|------|-------|-----------------|
| E1 | Dark lighting | Enable webcam in a dim room | Detection may miss the face or show low confidence; note where it fails |
| E2 | Partial face | Cover half the face with a hand | Detection may drop or show reduced confidence |
| E3 | Extreme angle | Tilt the head roughly 45° sideways | Detection may still work; note the angle at which it drops |
| E4 | Very small face | Sit far from the webcam | Face may not be detected below a certain size; note the distance |
| E5 | Drag and drop image | Drag an image file onto the app window | Image loads and detection runs, same as the open button |
| E6 | Haar vs YuNet | Compare YuNet's behaviour against the Haar cascade described in the README | Haar detects fewer faces and more false positives; note the differences |

---

## Notes on environment

- Build from the MSYS2 MinGW64 shell. PowerShell and the PyCharm terminal have no `make`.
- GUI video (V1–V4) needs the [K-Lite Codec Pack](https://codecguide.com) for MP4 on Windows.
- The `.string()` fix at `ofApp.cpp:667` can be reverted by a `git pull` that touches that file.
  Re-apply it if the build breaks after a sync.
- See [SETUP_WINDOWS.md](SETUP_WINDOWS.md) for the full Windows setup, and
  [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for everything that went wrong and how it was fixed.
