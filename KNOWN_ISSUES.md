# Known Issues

Problems hit while setting up, building and testing the project, with the cause and the fix for
each. Most are environment issues rather than defects in the app. They cost enough time to be worth
writing down.

Compiled by Modar Issa from 51 test scenarios run in August 2026.
Environment: Windows, MSYS2 MinGW64, OpenCV 4.13.0.

---

## Build and setup

### 1. Build fails at `ofApp.cpp:705` on Windows

**Symptom**

```
src/ofApp.cpp:705: error: cannot convert 'std::filesystem::__cxx11::path' to 'const std::string&'
    openPath(dragInfo.files.front());
```

**Cause**

`dragInfo.files.front()` is a `std::filesystem::path` and `openPath()` takes a
`const std::string&`. The platform decides whether that compiles, and the compiler has nothing to do
with it. `path::string_type` is `std::string` on macOS and Linux, so the path converts and binds to
the reference. On Windows it is `std::wstring`, so no conversion exists and the compiler rejects the
call. The same standard and the same GCC give a different answer on each platform.

**Fix**

```cpp
openPath(dragInfo.files.front().string());
```

`.string()` is valid on every platform, so this does not affect the macOS or Linux build.

**Watch out**

The fix lives in a tracked source file, so any `git pull`, `checkout` or branch switch touching
`ofApp.cpp` can revert it without telling you. It came back once during Phase 3 testing for that
reason. If a build that worked before fails here, check the line before you look anywhere else.

**Status:** fixed and committed.

---

### 2. `[build] ERROR: make not found` inside a shell that looks correct

**Symptom**

The build reports that `make` is missing and tells you to use an MSYS2 MinGW64 shell, even though
the prompt says `MINGW64` and the same build worked an hour earlier.

**Cause**

Two different things produce this one message.

`scripts/build.py` calls `shutil.which("make")`, so the message means "make is not on PATH" and
nothing more. You get it in the wrong shell, and you also get it when `make` is missing from a shell
that is right. An MSYS2 update can remove it.

**Git Bash also shows `MINGW64` in its prompt**, and several Git clients open Git Bash when you
click their console button. Git Bash carries no `make` and no `pacman`.

**Fix**

Check both rather than assuming:

```bash
echo $MSYSTEM        # environment: must be MINGW64, but see below
pacman --version     # tells Git Bash and real MSYS2 apart
which make           # the package itself
```

The prompt is the quickest tell: Git Bash shows the current git branch, MSYS2 does not. A prompt
reading `MINGW64 ~/repo (main)` is Git Bash. Failing that, this succeeds only in a real MSYS2 shell,
since Git for Windows ships its own unrelated `/mingw64/bin`:

```bash
ls /mingw64/bin/libopencv_core-413.dll
```

Then, if the shell is right and `make` is missing:

```bash
pacman -S make
```

**Status:** resolved, `make` reinstalled.

---

### 3. `mingw-w64-x86_64-tiff` does not exist

**Symptom**

`pacman` fails on that package while installing the dependency list.

**Cause**

There is no such package in the MSYS2 repositories.

**Fix**

Leave it out. Nothing in the build needs it.

**Status:** not an issue, a wrong package name to avoid.

---

### 4. Pasted commands fail in MSYS2

**Symptom**

```
-bash: $'\E[200~cd': command not found
```

**Cause**

Windows terminals wrap pasted text in bracketed-paste escape sequences (`\E[200~`) and MSYS2 bash
treats them as part of the command.

**Fix**

Type commands by hand, particularly multi-line blocks. After a `cd`, confirm the prompt shows the
new path before running the next command.

**Status:** workaround only. The terminal behaves this way and the project cannot change it.

---

## Running the app

### 4b. `facerec.exe` reports a missing DLL inside a shell that says MINGW64

**Symptom**

```
facerec.exe: error while loading shared libraries: liburiparser-1.dll: cannot open shared object file
```

and `ldd facerec.exe` lists a dozen or more libraries as "not found", including OpenCV, freetype,
glew and glfw.

**Cause**

The Git Bash shell again, almost certainly, rather than a broken install. Git for Windows has its
own `/mingw64/bin` holding none of the MSYS2 libraries, so nothing resolves from that shell and
`ldd` reports everything missing. The Windows loader names the first library it fails on and stops,
which makes one package look like the culprit.

**Fix**

Check what is on disk before installing anything:

```bash
ls /mingw64/bin/liburiparser-1.dll /mingw64/bin/libopencv_core-413.dll
```

If the files are there, you were in the wrong shell. Open a real MSYS2 MinGW64 shell and run it
again. Reinstall a package only when its file is missing from disk.

`ldd` is an MSYS-side tool and resolves MinGW dependencies badly. `ntldd`
(`pacman -S mingw-w64-x86_64-ntldd`) gives a list you can trust.

**Status:** environment issue, no change needed to the project.

---

### 5. `facerec.exe` fails when double-clicked

**Symptom**

Double-clicking the executable in Windows Explorer gives a `libfreeimage-3.dll` error and the app
never opens.

**Cause**

The executable links against MSYS2 DLLs that Windows does not have on its PATH. The MSYS2 shell is
the only place they resolve.

**Fix**

Launch from the MinGW64 shell:

```bash
cd /c/Users/<you>/cc2-face-recognition/facerec/bin
./facerec.exe
```

**Status:** expected behaviour on Windows, documented rather than fixed.

---

### 6. Video files will not play in the GUI

**Symptom**

Loading an MP4 through "open video..." shows a black screen and:

```
[ error ] ofDirectShowPlayer: Cannot load video of this file type. Make sure you have codecs installed on your system.
[ error ] facerec: Could not load video: ...
```

**Cause**

The GUI video player uses Windows DirectShow, which carries no built-in MP4/H.264 support. The codec
is missing from the system rather than from the app.

**Fix**

Install the free K-Lite Codec Pack from codecguide.com.

Headless video is unaffected, since `--liveness-replay` reads video through OpenCV. That path worked
before the codec pack went on.

**Status:** resolved. All four GUI video tests passed afterwards.

---

### 7. `samples/` cannot be found in the file dialog

**Symptom**

The headless commands accept `samples/messi5.jpg`, but browsing to `samples` in the app's file
dialog finds nothing under `facerec/bin/`.

**Cause**

`--detect` and `--identify` resolve their argument **under `facerec/bin/data/`** rather than against
the working directory. openFrameworks puts its data path in front of whatever you pass, so
`samples/messi5.jpg` becomes `facerec/bin/data/samples/messi5.jpg`. The file dialog browses the real
filesystem, so it needs that full location.

Passing the path you can see on disk makes it worse, since openFrameworks adds the prefix anyway:

```
$ ./facerec.exe --identify data/samples/messi-worldcup.jpg
[ error ] loadImage: File not found: "data\\data/samples/messi-worldcup.jpg"
```

`--liveness-replay` is the exception. It hands the path straight to OpenCV with no prefix, so give
it an absolute path, or one relative to `facerec/bin`.

**Fix**

In the GUI, navigate to `facerec\bin\data\samples\`. On the command line, pass `samples/...` for
images and a full path for videos.

**Status:** not a defect. The two commands resolve paths differently and that is easy to trip over.

---

## Detection, recognition and liveness limitations

These are properties of the models and the approach rather than bugs. Every one of them showed up
more than once during testing.

### 8. Performance is around 8 fps on a live webcam

Roughly 122 ms per frame with CPU-only inference. Normal for YuNet plus SFace without GPU
acceleration. Video files play far faster, 140 to 158 fps measured, since camera capture no longer
sets the pace.

### 9. A still face gets flagged `PHOTO?`

Sit still for 7 to 10 seconds and the liveness detector decides it might be looking at a
photograph. The detector is doing what it was designed to do, and it still catches people out on a
first run. Blink two or three times and the label flips back to `LIVE`.

### 10. Glasses make blink detection harder

The blink signal is photometric, so glasses interfere with it. The detector still catches blinks and
wants harder ones to do it. Tested with glasses on and working, with less margin than without them.

### 11. Liveness needs tracking enabled

Liveness runs per tracked face. Turn tracking off and the label shows `unknown` with no `#id`, and
the app produces no LIVE or PHOTO? verdict.

### 12. Virtual cameras show a black feed

Switch to a virtual camera device and you get a black image. An IdeaCamera entry showed up on the
test machine and behaved this way. The app keeps running and switching back works, and there is
nothing in the frame to detect.

### 13. Tracking IDs are not persistent identities

A face that leaves the frame for more than about two seconds, or walks far enough away, comes back
with a new `#id`. Under two seconds it keeps the same one. The tracker works on IoU and centroids,
so `#id` means "the same face across recent frames" and says nothing about which person that is.
Recognition against the gallery is what identifies people.

### 14. A low conf threshold finds faces that are not there

Dragging **conf threshold** well below its `0.60` default makes the detector report background
texture as faces. A repeating tile pattern on a wall produced about ten boxes, scoring between 0.10
and 0.31. The default sits where it does for that reason. If you lower it to pull in a face the
detector is missing, check what else turns up alongside it.

---

## Repository workflow notes

Two git behaviours rather than app issues. Both cost time on this project and would cost it again.

### 15. A pushed branch looks like it did not push

GitHub opens a repository on its default branch. Commit to any other branch and your work stays off
the landing page, which looks the same as a failed push. Use the branch dropdown, or go to
`/tree/<branch>`. `git ls-remote origin` tells you what is on the remote before you start debugging
a push that worked.

### 16. A rewritten history changes every SHA

Rewrite a branch's history and force-push it, and every commit on it gets a new hash. A clone made
before that point shares no commits with the remote. You see a large "ahead and behind" count on a
branch you never touched, and 404s on links to any branch renamed or deleted in the same operation.
Nothing is lost. Point the local branch at the new history rather than merging into it, since
merging or pulling replays the whole old history on top of the new one.

The safe repair, with uncommitted changes in the working tree:

```bash
git fetch --prune origin
git branch -f <local-branch> origin/<branch>   # moves the pointer, leaves the working tree alone
```

`git checkout` followed by `git reset --hard` does the same job and touches the working tree, which
puts uncommitted work at risk.
