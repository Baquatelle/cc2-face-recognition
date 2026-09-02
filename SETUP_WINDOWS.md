# Windows Setup Guide

Building and running `facerec` on Windows with MSYS2 MinGW64.

The main README covers the project itself. This file only covers the Windows-specific parts, since
most of what goes wrong on Windows does not go wrong on macOS or Linux.

Written and verified by Modar Issa. Windows, MSYS2 MinGW64, OpenCV 4.13.0, August 2026.

---

## 1. What you need

| Tool | Why | Notes |
|---|---|---|
| Python (Windows) | runs `scripts/bootstrap.py` | any recent 3.x |
| MSYS2 with the MinGW64 shell | runs the actual build | the build will not work anywhere else |
| Git client | cloning, branches | Fork was used here, any client works |

`scripts/build.py` is the build command. It is a Python wrapper around `make`, so running it is not
"writing Python", it is just how the project is built.

---

## 2. Bootstrap

Run this from PowerShell or the PyCharm terminal, not from MSYS2:

```
py scripts/bootstrap.py
```

It downloads openFrameworks, the ONNX models and the sample media, then runs the project generator.

When it finishes you should see:

```
[bootstrap] done. Next: python3 scripts/build.py
```

If `python` is not recognised, use `py` instead. The `py` launcher is installed with Python on
Windows and resolves the right interpreter, while `python` often points at nothing or at the
Microsoft Store stub.

---

## 3. Install the MSYS2 packages

Open the **MSYS2 MinGW64** shell (not "MSYS2 MSYS", not "MSYS2 UCRT64") and run:

```bash
pacman -S python
pacman -S make
pacman -S mingw-w64-x86_64-pkg-config
pacman -S mingw-w64-x86_64-cairo
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-nlohmann-json
pacman -S mingw-w64-x86_64-pugixml
pacman -S mingw-w64-x86_64-mpg123
pacman -S mingw-w64-x86_64-libsndfile
pacman -S mingw-w64-x86_64-portaudio
pacman -S mingw-w64-x86_64-opencv
pacman -S mingw-w64-x86_64-gtk3
pacman -S mingw-w64-x86_64-gstreamer
pacman -S mingw-w64-x86_64-gst-plugins-base
pacman -S mingw-w64-x86_64-libpng
pacman -S mingw-w64-x86_64-libjpeg-turbo
pacman -S mingw-w64-x86_64-glm
pacman -S mingw-w64-x86_64-utf8cpp
```

`mingw-w64-x86_64-tiff` does **not** exist in MSYS2. Leave it out. Asking for it makes pacman fail
and it is not needed.

---

## 4. Build

Still in the MinGW64 shell:

```bash
cd /c/Users/<you>/cc2-face-recognition
python3 scripts/build.py
```

Success looks like:

```
[build] built C:/Users/<you>/cc2-face-recognition/facerec/bin/facerec.exe
```

A `cp: cannot stat '.../lib/msys2/*.dll'` line at the very end is harmless and appears on every
successful build.

### Make sure you are actually in the right shell

Two checks, and you need both:

```bash
echo $MSYSTEM        # must print MINGW64
pacman --version     # must work
```

`MINGW64` alone is not enough. **Git Bash also prints `MINGW64`**, and many Git clients open Git
Bash when you click their console button. Git Bash has no `pacman` and no `make`, so the build fails
there with a message that sounds like you are in the wrong shell. `pacman --version` is the fastest
way to tell the two apart.

If `make` is genuinely missing (it can disappear after an MSYS2 update), install it:

```bash
pacman -S make
```

---

## 5. One Windows-only source change

`facerec/src/ofApp.cpp` line 705, inside `dragEvent()`:

```cpp
openPath(dragInfo.files.front().string());   // .string() is required on Windows
```

Without `.string()` the build fails with:

```
error: cannot convert 'std::filesystem::__cxx11::path' to 'const std::string&'
```

`std::filesystem::path` keeps its native string in `path::string_type`. That is `std::string` on
macOS and Linux, so the path converts implicitly and the call compiles. On Windows it is
`std::wstring`, so there is no implicit conversion and the same line fails. It is the platform, not
the compiler or the standard version, which is why this only ever shows up on Windows builds.

`.string()` is correct everywhere, so the fix does not break the macOS or Linux build.

Note that this lives in a tracked file, so a `git pull` or `git checkout` that touches `ofApp.cpp`
can silently take it away again. If a build that worked yesterday fails at line 705 today, check
that line first.

---

## 6. Running the app

```bash
cd /c/Users/<you>/cc2-face-recognition/facerec/bin
./facerec.exe
```

**Run it from the MinGW64 shell only.** Double-clicking `facerec.exe` in Windows Explorer fails with
a `libfreeimage-3.dll` error. The MSYS2 DLLs the executable needs are not on the Windows PATH, and
only the MSYS2 shell sets that up.

On launch it loads the YuNet detection model and the SFace recognition model from `data/models/`,
loads the sample gallery, and lists the webcams it found.

---

## 7. Video files need a codec

Opening an MP4 through "open video..." can fail with:

```
[ error ] ofDirectShowPlayer: Cannot load video of this file type. Make sure you have codecs installed on your system.
```

The GUI video player uses Windows DirectShow, which has no MP4/H.264 support out of the box.
Installing the free K-Lite Codec Pack (codecguide.com) fixes it.

This affects the GUI only. The headless `--liveness-replay` mode goes through OpenCV and works
without any codec pack.

---

## 8. Headless commands

The self-test runs from the **repo root**:

```bash
# full self-test: OpenCV version, model files, tracker and liveness unit tests
python3 scripts/build.py --check
```

Everything below runs from `facerec/bin/`. Both in the MinGW64 shell:

```bash
# detect faces in an image
./facerec.exe --detect samples/messi5.jpg
./facerec.exe --detect samples/group.pgm

# identify faces against the gallery
./facerec.exe --identify samples/messi-worldcup.jpg
./facerec.exe --identify samples/ronaldo-worldcup.jpg

# liveness diagnostic over a video file
./facerec.exe --liveness-replay C:/Users/<you>/cc2-face-recognition/test/videos/Abel_live.mp4
```

Two different path rules here, which is easy to trip over:

- `--detect` and `--identify` resolve their argument **under `facerec/bin/data/`**, so pass
  `samples/messi5.jpg`. Passing `data/samples/messi5.jpg` fails, because the prefix is added anyway
  and you end up with `data\data/samples/...`.
- `--liveness-replay` passes the path straight to OpenCV with no prefix, so give it a full path such
  as `C:/Users/<you>/cc2-face-recognition/test/videos/Abel_live.mp4`.

In the app's file dialog you need the real location on disk, `facerec\bin\data\samples\`.

`scripts/run_headless_tests.py` runs all of the above and prints PASS or FAIL for each with the
numbers it measured.

---

## 9. Quick troubleshooting table

| What you see | What it means |
|---|---|
| `python3: command not found` in MSYS2 | `pacman -S python` |
| `python.exe not found` in PyCharm | use `py`, not `python` |
| `make: command not found` | wrong shell, open MSYS2 MinGW64 |
| `[build] ERROR: make not found` inside MinGW64 | either Git Bash pretending to be MinGW64, or `make` really is missing. Check `pacman --version`, then `pacman -S make` |
| `cairo pkg-config error` | `pacman -S mingw-w64-x86_64-pkg-config mingw-w64-x86_64-cairo` |
| `nlohmann_json not found` | `pacman -S mingw-w64-x86_64-nlohmann-json` |
| `pugixml not found` | `pacman -S mingw-w64-x86_64-pugixml` |
| `glm not found` | `pacman -S mingw-w64-x86_64-glm` |
| `utf8cpp not found` | `pacman -S mingw-w64-x86_64-utf8cpp` |
| error at `ofApp.cpp:705` | the `.string()` fix is gone, see section 5 |
| `libfreeimage-3.dll` on double-click | run from MSYS2, see section 6 |
| black screen on a video file | DirectShow codec, see section 7 |
| `cp: cannot stat '.../*.dll'` at the end of a build | harmless, the build succeeded |

See `KNOWN_ISSUES.md` for the longer write-up of each of these.
