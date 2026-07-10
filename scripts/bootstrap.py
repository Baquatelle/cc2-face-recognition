#!/usr/bin/env python3
"""Bootstrap the facerec build environment.

Idempotent: safe to re-run, skips whatever is already in place. Gets a clean
checkout to the point where scripts/build.py can produce a runnable binary.

Steps:
  1. Download openFrameworks 0.12.1 into <repo>/of/  (gitignored)
  2. Clone the ofxCv addon into of/addons/
  3. Verify the OpenCV bundled with ofxOpenCv is >= 4.5.4 (YuNet requirement)
  4. Download the YuNet + SFace ONNX models into facerec/bin/data/models/
  5. Run the openFrameworks projectGenerator for the current platform
  6. macOS: add the camera-permission key to the generated Info.plist

Platforms: macOS (Xcode) and Windows (msys2/mingw64, run from an MSYS2 shell
or plain Python — only downloads and the projectGenerator run here).
"""

import hashlib
import plistlib
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OF_DIR = ROOT / "of"
APP_DIR = ROOT / "facerec"
MODELS_DIR = APP_DIR / "bin" / "data" / "models"
CACHE_DIR = ROOT / ".cache"

OF_VERSION = "0.12.1"
OF_RELEASE_BASE = f"https://github.com/openframeworks/openFrameworks/releases/download/{OF_VERSION}"
OF_ARCHIVES = {
    "darwin": f"of_v{OF_VERSION}_osx_release.tar.gz",
    "win32": f"of_v{OF_VERSION}_msys2_mingw64_release.zip",
}

OFXCV_GIT = "https://github.com/kylemcdonald/ofxCv.git"

# opencv_zoo stores models in git-lfs; the media host serves the real bytes
MODEL_BASE = "https://media.githubusercontent.com/media/opencv/opencv_zoo/main/models"
MODELS = {
    "face_detection_yunet_2023mar.onnx": {
        "url": f"{MODEL_BASE}/face_detection_yunet/face_detection_yunet_2023mar.onnx",
        "sha256": "8f2383e4dd3cfbb4553ea8718107fc0423210dc964f9f4280604804ed2552fa4",
    },
    "face_recognition_sface_2021dec.onnx": {
        "url": f"{MODEL_BASE}/face_recognition_sface/face_recognition_sface_2021dec.onnx",
        "sha256": "0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79",
    },
}

MIN_OPENCV = (4, 5, 4)


def log(msg):
    print(f"[bootstrap] {msg}", flush=True)


def die(msg):
    print(f"[bootstrap] ERROR: {msg}", file=sys.stderr, flush=True)
    sys.exit(1)


def platform_key():
    if sys.platform == "darwin":
        return "darwin"
    if sys.platform in ("win32", "cygwin", "msys"):
        return "win32"
    die(f"unsupported platform: {sys.platform} (macOS and Windows only)")


def download(url, dest: Path, label=None):
    """Download url to dest atomically, with a crude progress indicator."""
    label = label or dest.name
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    log(f"downloading {label} ...")

    def hook(blocks, block_size, total):
        done = blocks * block_size
        if total > 0 and blocks % 400 == 0:
            print(f"    {done / 1e6:.0f} / {total / 1e6:.0f} MB", flush=True)

    urllib.request.urlretrieve(url, tmp, reporthook=hook)
    tmp.replace(dest)
    return dest


def sha256sum(path: Path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def ensure_openframeworks(plat):
    if (OF_DIR / "libs" / "openFrameworks").is_dir():
        log(f"openFrameworks already present at {OF_DIR}")
        return

    archive_name = OF_ARCHIVES[plat]
    archive = CACHE_DIR / archive_name
    if not archive.exists():
        download(f"{OF_RELEASE_BASE}/{archive_name}", archive)
    else:
        log(f"using cached {archive_name}")

    log(f"extracting {archive_name} ...")
    extract_dir = CACHE_DIR / "of_extract"
    if extract_dir.exists():
        shutil.rmtree(extract_dir)
    extract_dir.mkdir(parents=True)

    if archive_name.endswith(".tar.gz"):
        # shell tar: much faster than the tarfile module and keeps exec bits
        subprocess.run(["tar", "xzf", str(archive), "-C", str(extract_dir)], check=True)
    else:
        with zipfile.ZipFile(archive) as z:
            z.extractall(extract_dir)

    roots = [p for p in extract_dir.iterdir() if p.is_dir()]
    if len(roots) != 1:
        die(f"expected one top-level dir in {archive_name}, got: {[p.name for p in roots]}")
    roots[0].rename(OF_DIR)
    shutil.rmtree(extract_dir, ignore_errors=True)
    log(f"openFrameworks {OF_VERSION} installed at {OF_DIR}")


def ensure_ofxcv():
    addon = OF_DIR / "addons" / "ofxCv"
    if addon.is_dir():
        log("ofxCv already present")
        return
    log("cloning ofxCv ...")
    subprocess.run(["git", "clone", "--depth", "1", OFXCV_GIT, str(addon)], check=True)


def check_bundled_opencv():
    """The scope requires OpenCV >= 4.5.4 (YuNet). oF bundles OpenCV with the
    ofxOpenCv addon; parse its version header. If this ever fails, the fallback
    is a system OpenCV (Homebrew on macOS) — see SCOPE.md section 9."""
    header = (OF_DIR / "addons" / "ofxOpenCv" / "libs" / "opencv"
              / "include" / "opencv2" / "core" / "version.hpp")
    if not header.exists():
        die(f"cannot find bundled OpenCV version header at {header}")
    version = {}
    for line in header.read_text().splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[0] == "#define" and parts[1] in (
                "CV_VERSION_MAJOR", "CV_VERSION_MINOR", "CV_VERSION_REVISION"):
            version[parts[1]] = int(parts[2])
    v = (version.get("CV_VERSION_MAJOR", 0),
         version.get("CV_VERSION_MINOR", 0),
         version.get("CV_VERSION_REVISION", 0))
    if v < MIN_OPENCV:
        die(f"bundled OpenCV is {v[0]}.{v[1]}.{v[2]}, need >= "
            f"{'.'.join(map(str, MIN_OPENCV))} for YuNet. Switch to a system "
            f"OpenCV build (see SCOPE.md risks).")
    log(f"bundled OpenCV {v[0]}.{v[1]}.{v[2]} — ok (>= {'.'.join(map(str, MIN_OPENCV))})")


def ensure_models():
    MODELS_DIR.mkdir(parents=True, exist_ok=True)
    for name, spec in MODELS.items():
        dest = MODELS_DIR / name
        if dest.exists() and sha256sum(dest) == spec["sha256"]:
            log(f"model {name} already present")
            continue
        download(spec["url"], dest)
        digest = sha256sum(dest)
        if digest != spec["sha256"]:
            dest.unlink()
            die(f"checksum mismatch for {name}: got {digest}")
        log(f"model {name} verified")


def find_project_generator(plat):
    if plat == "darwin":
        candidates = list(OF_DIR.glob(
            "projectGenerator*/projectGenerator.app/Contents/Resources/app/app/projectGenerator"))
    else:
        candidates = list(OF_DIR.glob(
            "projectGenerator*/resources/app/app/projectGenerator.exe"))
        candidates += list(OF_DIR.glob("projectGenerator*/projectGenerator.exe"))
    for c in candidates:
        if c.is_file():
            return c
    die(f"projectGenerator CLI not found under {OF_DIR}")


def run_project_generator(plat):
    pg = find_project_generator(plat)
    of_platform = "osx" if plat == "darwin" else "msys2"
    log(f"running projectGenerator ({of_platform}) on {APP_DIR} ...")
    subprocess.run(
        [str(pg), f"-o{OF_DIR}", f"-p{of_platform}", str(APP_DIR)],
        check=True,
    )


def patch_info_plist():
    """Webcam mode (M2) needs a camera-usage string on macOS; the plist is
    regenerated by projectGenerator, so re-apply it on every bootstrap."""
    plist_path = APP_DIR / "openFrameworks-Info.plist"
    if not plist_path.exists():
        log("no openFrameworks-Info.plist generated, skipping camera-permission patch")
        return
    with open(plist_path, "rb") as f:
        plist = plistlib.load(f)
    key = "NSCameraUsageDescription"
    message = "facerec uses the camera for live face detection."
    # The projectGenerator template writes a generic value for this key, so we
    # always overwrite it with our own message; idempotent because we only
    # rewrite the file when the value actually differs.
    if plist.get(key) == message:
        return
    plist[key] = message
    with open(plist_path, "wb") as f:
        plistlib.dump(plist, f)
    log(f"set {key} in openFrameworks-Info.plist")


def main():
    plat = platform_key()
    CACHE_DIR.mkdir(exist_ok=True)
    ensure_openframeworks(plat)
    ensure_ofxcv()
    check_bundled_opencv()
    ensure_models()
    run_project_generator(plat)
    if plat == "darwin":
        patch_info_plist()
    log("done. Next: python3 scripts/build.py")


if __name__ == "__main__":
    main()
