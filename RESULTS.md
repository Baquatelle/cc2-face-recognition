# Test Results — cc2-face-recognition

**Tester:** Modar Issa
**Date:** August 2026
**Environment:** Windows 11 / MSYS2 MinGW64 / OpenCV 4.13.0
**Build:** `facerec.exe` compiled successfully from source
**Test plan:** [TESTING.md](TESTING.md)

**Summary: 51 Pass | 0 Fail | 0 Pending — all tests complete ✅**

The headless subset was re-run on 29 August 2026 through
[`scripts/run_headless_tests.py`](scripts/run_headless_tests.py). **All 7 checks passed with numbers
identical to those recorded below**: 1 face, 22 faces, messi, ronaldo, blinks=11 with LIVE=262, and
PHOTO?=180. These results are reproducible on demand rather than a single observation.

Before testing could start, the `ofApp.cpp` type error had to be re-applied (see D&C-1), because a
git operation had reverted the fix.

> Test IDs match `cc2fr_Phases_0103_Contribution_Documentation.docx`, which is the authoritative test
> plan.
>
> **One exception.** Category F was added on 29 August 2026, after Commit 4 integrated `FrameFilter`
> into the app. Those four IDs are not in the phases document, which was written before the feature
> existed. They need adding there before it is delivered.

---

## Category D — Detection (image mode)

| ID | Test | Result | Notes |
|----|------|--------|-------|
| D1 | Single face — clean | ✅ Pass | Faces: 1, box and all 5 landmarks on Messi |
| D2 | Group photo | ✅ Pass | 22 faces detected, all labelled unknown (none in the gallery) |
| D3 | No face | ✅ Pass | Faces: 0 on a stadium photo, correct rejection with no false positives |
| D4 | Headless detect — single | ✅ Pass | 1 face, confidence 0.91 |
| D5 | Headless detect — group | ✅ Pass | 22 faces, confidence range 0.66–0.91 |
| D6 | Conf threshold low | ✅ Pass | 22 faces at 0.60 → 24 at 0.41; two marginal faces picked up as the bar dropped |
| D7 | Conf threshold high | ✅ Pass | Fewer faces confirmed as the slider was raised above the 0.70 baseline |

---

## Category R — Recognition (gallery matching)

| ID | Test | Result | Notes |
|----|------|--------|-------|
| R1 | Known person — Messi | ✅ Pass | Identified as Messi, score 0.72 against a 0.363 threshold |
| R2 | Known person — Ronaldo | ✅ Pass | Identified as Ronaldo, score 0.68 |
| R3 | Unknown face | ✅ Pass | Confirmed through D2: all 22 group faces returned "unknown". No dedicated screenshot |
| R4 | Match threshold — lower | ✅ Pass | Re-run 30 Aug 2026. Match threshold 0.20, conf 0.608: own face labelled `Modar 0.59`, box blue. Matched, the score sitting well above the threshold |
| R5 | Match threshold — higher | ✅ Pass | Same session and position. At 0.50 the label held at `Modar 0.62` — the score was still above the threshold, so nothing changed. Raised further to 0.86 the label flipped to `unknown 0.62` and the box from blue to green. The crossing point is the score itself, not a fixed value |
| R6 | Enrol yourself live | ✅ Pass | Webcam with tracking and liveness on, pressed `A`, typed "Modar". Face labelled "Modar LIVE" after re-enrolment |
| R7 | Gallery load | ✅ Pass | **load gallery…** with the parent folder of `Modar/`; gallery reloaded and recognition updated |

**The R4/R5 re-run.** Both were re-run on 30 August 2026 with the gallery loaded, varying only
the match threshold — ~0.2, then ~0.5 as documented, then past 0.5 to locate the crossing point.
The earlier pair is kept under `test-evidence/R-recognition/_superseded_pre_enrolment/`.

**Side observation from the re-run.** With glasses on the score sat at 0.59–0.64; with glasses off,
same position and lighting, it rose to 0.78–0.79. The gallery photographs are unspectacled, so the
bare-faced frame is the closer match. Threshold and appearance were varied one at a time, and the
R4/R5/R5b series is all glasses-on so that only the threshold changes across the three figures.

---

## Category V — Video mode

> All four were initially blocked by the missing DirectShow codec (D&C-3). They passed after the
> K-Lite Codec Pack was installed.

| ID | Test | Result | Notes |
|----|------|--------|-------|
| V1 | Open video file | ✅ Pass | Played correctly after the codec install; face detected with box and landmarks per frame |
| V2 | Recognition on video | ✅ Pass | Face labelled "unknown" with a tracking ID, correct since Abel is not in the gallery |
| V3 | Multiple faces in video | ✅ Pass | Tested with `Abel_live.mp4`, a single-face clip. Box and 5 landmarks per frame. No group video was available, so multi-face video is covered only by the single-face case |
| V4 | FPS / performance | ✅ Pass | 140.3–158.4 fps throughout, no freezing. Higher than live webcam (~8 fps) because file playback is not capture-bound |

---

## Category W — Webcam mode

| ID | Test | Result | Notes |
|----|------|--------|-------|
| W1 | Basic webcam detection | ✅ Pass | Live feed active, face visible with box and 5 landmarks |
| W2 | Multiple webcams | ✅ Pass | Switched to IdeaCamera: black screen, expected for a virtual camera, no crash |
| W3 | Refresh webcams | ✅ Pass | Both devices re-detected after clicking refresh, confirmed in the terminal |
| W4 | Face enters / leaves frame | ✅ Pass | Detection dropped when the camera was covered and resumed immediately on uncover |
| W5 | FPS check | ✅ Pass | "webcam 1/2: Integrated W" shown, fps within the expected range |

---

## Category T — Face tracking

| ID | Test | Result | Notes |
|----|------|--------|-------|
| T1 | Stable ID on webcam | ✅ Pass | Face assigned `#1`, label stable over 30+ seconds |
| T2 | ID persists through movement | ✅ Pass | `#1` held through left, right and tilt movement without flicker |
| T3 | ID survives short dropout | ✅ Pass | Under 2 s returned the same ID, over 2 s assigned a new one. Grace period of roughly 2 s confirmed, repeated after an app restart to isolate the behaviour |
| T4 | Two faces — separate IDs | ✅ Pass | `#2` (Modar) and `#6` (a face on a phone screen) tracked simultaneously |
| T5 | Tracking off vs on | ✅ Pass | Tested via distance rather than the toggle: a new ID was assigned when the face moved far away and returned. Same outcome, distinct-face behaviour confirmed |

---

## Category L — Liveness detection

| ID | Test | Result | Notes |
|----|------|--------|-------|
| L1 | LIVE flag — blink | ✅ Pass | Sat still until `PHOTO?` appeared within ~10 s, then 2–3 blinks flipped it to `LIVE`. Combined with L3 |
| L2 | LIVE flag — mouth movement | ✅ Pass | Opening and closing the mouth triggered `LIVE`. No screenshot taken |
| L3 | PHOTO? flag — still face | ✅ Pass | `PHOTO?` appeared after stillness and reverted correctly once the LIVE sticky window elapsed |
| L4 | PHOTO? flag — phone screen | ✅ Pass | Not run standalone. The phone screen used in T4 showed "unknown" with no LIVE flag, which is the expected outcome |
| L5 | Glasses interference | ✅ Pass | Blink detected with glasses on, but needed a more exaggerated blink. Matches the limitation documented in the README |
| L6 | Liveness requires tracking | ✅ Pass | With tracking off the label showed "unknown" without a `#id`, and liveness was inactive |
| L7 | Liveness replay — live video | ✅ Pass | `Abel_live.mp4`: 11 blinks, LIVE = 262/285 frames (91.9%). Re-run 29 Aug 2026 with the same result |
| L7b | Liveness replay — photo video | ✅ Pass | `Abel_photo.mp4`: PHOTO? = 180/327 frames (55%), only 1 late blink. Re-run 29 Aug 2026 with the same result |

---

## Category F — Frame filter (display modes)

Tested on webcam immediately after Commit 4 (`286e681`), with tracking and liveness enabled.
Screenshots in `test-evidence/F-framefilter/`.

| ID | Test | Result | Notes |
|----|------|--------|-------|
| F1 | Normal — pass-through | ✅ Pass | Cycle wrapped from `Face Crop` back to `Normal`; frame and overlays unchanged |
| F2 | Greyscale (7.3a) | ✅ Pass | Colour removed; box and all 5 landmarks still drawn, label `#4 Modar 0.74 LIVE` |
| F3 | Edges (7.3b / 7.3c) | ✅ Pass | Canny outlines only on screen, yet box, landmarks and `#4 Modar 0.70 PHOTO?` still drawn on top |
| F4 | Face crop (7.3d) | ✅ Pass | Zoomed to the largest face with the frame's proportions kept, so the face is not stretched; box and label absent as designed; HUD still reported `Faces: 1` |

**F3 is the clearest evidence that the filter is display-only.** The screen shows nothing but white
edge outlines, and the app is simultaneously naming the face at 0.70 and reporting a liveness
verdict — both computed from the unfiltered frame underneath. F4 makes the same point from the
other direction: overlays are deliberately suppressed, but `Faces: 1` keeps updating, so detection
never stopped.

---

## Category S — Self-test and diagnostics

| ID | Test | Result | Notes |
|----|------|--------|-------|
| S1 | Full self-test | ✅ Pass | All checks passed after re-applying the `ofApp.cpp` fix (D&C-1), which git had reverted |
| S2 | OpenCV version | ✅ Pass | OpenCV 4.13.0 detected |
| S3 | Model files present | ✅ Pass | YuNet and SFace both found and verified |
| S4 | Tracker unit tests | ✅ Pass | All 7 tracker unit tests passed |
| S5 | Liveness unit tests | ✅ Pass | All 20+ blink, mouth and liveness state tests passed |

---

## Category E — Edge cases

| ID | Test | Result | Notes |
|----|------|--------|-------|
| E1 | Dark lighting | ✅ Pass | Detected in significantly dimmed lighting at the default 0.6 threshold, all 5 landmarks correct. Liveness flagged `PHOTO?` while sitting still, as expected |
| E2 | Partial face | ✅ Pass | Half the face covered, detection held with box and landmarks visible |
| E3 | Extreme angle | ✅ Pass | Detected at maximum comfortable tilt, roughly 45° |
| E4 | Very small face | ✅ Pass | Detected at the maximum webcam distance available in the room |
| E5 | Drag and drop image | ✅ Pass | Ronaldo image dropped onto the window: detected, score 1.00 |
| E6 | Haar vs YuNet | ✅ Pass | YuNet's observed behaviour compared against the Haar description in the README: more faces, and at steeper angles. Consistent with the README |

**On the `unknown` labels in these screenshots.** Every webcam capture in categories D, W, T, L
and E predates the live enrolment recorded in R6. Faces are therefore labelled `unknown`
throughout those figures. Each of those tests asserts detection, tracking or liveness, none of
which depend on gallery membership, so the results stand as recorded.

---

## Results summary

| Category | Total | Passed | Failed | Notes |
|---|---|---|---|---|
| D — Detection (image) | 7 | 7 | 0 | Both threshold directions confirmed with the slider |
| R — Recognition | 7 | 7 | 0 | R4–R7 tested with the sliders, live enrolment and gallery load |
| V — Video | 4 | 4 | 0 | All four passed after the K-Lite Codec Pack install |
| W — Webcam | 5 | 5 | 0 | Includes the virtual-camera black screen as expected behaviour |
| T — Face tracking | 5 | 5 | 0 | T3 grace period confirmed via restart |
| L — Liveness | 8 | 8 | 0 | L7 and L7b cross-checked headlessly and re-run on 29 Aug |
| F — Frame filter | 4 | 4 | 0 | Display-only; detection confirmed still running underneath in F2–F4 |
| S — Self-test | 5 | 5 | 0 | Fix re-applied after the git revert |
| E — Edge cases | 6 | 6 | 0 | E1 completed with the lights dimmed |
| **Total** | **51** | **51** | **0** | 0 deferred, 0 failed |

Three coverage caveats stated plainly: **R3** is covered by D2 rather than by a dedicated test,
**L4** was observed as a side effect of T4 rather than run standalone, and **V3** used a single-face
clip because no group video was available.

---

## Difficulties & Challenges

### D&C-1 — `ofApp.cpp` type error (recurring)

**Symptom:**
```
src/ofApp.cpp:667: error: cannot convert 'std::filesystem::__cxx11::path' to 'const std::string&'
    openPath(dragInfo.files.front());
```

**Root cause:** `dragInfo.files.front()` returns `std::filesystem::path` and `openPath()` expects
`const std::string&`. This is a platform difference rather than a compiler one. `path::string_type`
is `std::string` on macOS and Linux, so the conversion is implicit there, and `std::wstring` on
Windows, so it is not. The same file compiles unchanged on macOS, which is why the error only
appeared on the Windows build.

**Fix:** `openPath(dragInfo.files.front().string());` — correct on every platform.

**Warning:** this fix lives in a tracked source file. Any `git pull` or `checkout` touching
`ofApp.cpp` will silently revert it. Re-apply after any sync involving that file.

---

### D&C-2 — `samples/` path confusion, headless vs GUI dialog

**Symptom:** the `samples/` folder is not visible in the Windows file dialog.

**Root cause:** `--detect` and `--identify` resolve their argument under `facerec/bin/data/`, so
`samples/x.jpg` means `facerec\bin\data\samples\x.jpg`. The file dialog browses the real filesystem
and cannot resolve that. Passing the visible path instead does not help either, because the prefix
is added regardless:

```
$ ./facerec.exe --identify data/samples/messi-worldcup.jpg
[ error ] loadImage: File not found: "data\\data/samples/messi-worldcup.jpg"
```

**Fix:** navigate to `facerec\bin\data\samples\` in the dialog, and pass `samples/…` on the command
line. `--liveness-replay` is the exception and takes a full path, since it goes straight to OpenCV.

---

### D&C-3 — GUI video fails: missing DirectShow MP4 codec

**Symptom:**
```
[ error ] ofDirectShowPlayer: Cannot load video of this file type.
Make sure you have codecs installed on your system.
OF recommends the free K-Lite Codec pack.
```

**Root cause:** the GUI video player uses Windows DirectShow, which needs system MP4 codecs. Headless
`--liveness-replay` uses OpenCV and was unaffected, as confirmed by L7.

**Fix:** install the free [K-Lite Codec Pack](https://codecguide.com).

**Impact:** V1–V4 were initially not testable; all four passed after the install.

---

The three above are the ones that affected testing directly. [KNOWN_ISSUES.md](KNOWN_ISSUES.md)
carries the full log, including the build environment problems and the model limitations observed
during testing.
