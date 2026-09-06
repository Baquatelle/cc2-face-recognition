#!/usr/bin/env python3
"""
run_headless_tests.py - headless test runner for cc2-face-recognition
Tester: Modar Issa

Runs the headless commands (self-test, detect, identify, liveness-replay) and
prints PASS or FAIL for each, with the numbers each test produced so they can be
copied straight into the results document.

Run it from the repo root in an MSYS2 MinGW64 shell:

    cd /c/Users/<you>/cc2-face-recognition
    python3 scripts/run_headless_tests.py

Two things about paths, both learned the hard way:

  * --detect and --identify resolve their argument under facerec/bin/data/, so
    they take "samples/messi5.jpg", not "data/samples/messi5.jpg" and not an
    absolute path.
  * --liveness-replay hands the path to OpenCV directly with no data/ prefix, so
    it needs an absolute path (or one relative to facerec/bin).
"""

import os
import re
import subprocess
import sys

# ---------------------------------------------------------------- paths

REPO     = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
BIN_DIR  = os.path.join(REPO, 'facerec', 'bin')
EXE      = os.path.join(BIN_DIR, 'facerec.exe')
DATA_DIR = os.path.join(BIN_DIR, 'data')
BUILD    = os.path.join(REPO, 'scripts', 'build.py')

# arguments for --detect and --identify, relative to facerec/bin/data/
MESSI_SINGLE = 'samples/messi5.jpg'
GROUP_PHOTO  = 'samples/group.pgm'
MESSI_WC     = 'samples/messi-worldcup.jpg'
RONALDO_WC   = 'samples/ronaldo-worldcup.jpg'

# videos need a full path
ABEL_LIVE  = os.path.join(REPO, 'test', 'videos', 'Abel_live.mp4')
ABEL_PHOTO = os.path.join(REPO, 'test', 'videos', 'Abel_photo.mp4')

# ---------------------------------------------------------------- output

GREEN = '\033[92m'
RED   = '\033[91m'
RESET = '\033[0m'

results = []


def section(title):
    print('')
    print('-' * 62)
    print('  ' + title)
    print('-' * 62)


def report(test_id, description, passed, evidence, output=''):
    if passed:
        tag = GREEN + 'PASS' + RESET
    else:
        tag = RED + 'FAIL' + RESET
    print('  [' + tag + '] ' + test_id + '  ' + description)
    if evidence:
        print('         ' + evidence)
    if not passed and output:
        lines = output.strip().splitlines()
        for line in lines[-6:]:
            print('         | ' + line)
    if passed:
        results.append((test_id, 'PASS'))
    else:
        results.append((test_id, 'FAIL'))


# ---------------------------------------------------------------- checks

def faces_in(output):
    """Read the 'faces: N' line. Returns None if it is not there."""
    m = re.search(r'faces:\s*(\d+)', output)
    if m is None:
        return None
    return int(m.group(1))


def names_in(output):
    """Every name= value from the 'face N: name=... best=... score=...' lines."""
    return re.findall(r'name=(\S+)', output)


def summary_in(output):
    """Read the liveness summary line into a dict, or None if it is missing."""
    m = re.search(r'blinks=(\d+).*?LIVE=(\d+)\s+PHOTO\?=(\d+)', output, re.S)
    if m is None:
        return None
    return {
        'blinks': int(m.group(1)),
        'live':   int(m.group(2)),
        'photo':  int(m.group(3)),
    }


def execute(cmd, cwd, timeout=300):
    proc = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    return proc.returncode, proc.stdout + proc.stderr


# ---------------------------------------------------------------- tests

def test_selftest():
    code, out = execute([sys.executable, BUILD, '--check'], REPO)
    # build.py --check reports a failed diagnostic through its exit code
    passed = (code == 0)
    last = ''
    lines = out.strip().splitlines()
    if lines:
        last = lines[-1]
    report('S1-S5', 'self-test: OpenCV, models, tracker, liveness',
           passed, 'exit code ' + str(code) + ', last line: ' + last, out)


def test_detect(test_id, description, image, min_faces):
    code, out = execute([EXE, '--detect', image], BIN_DIR)
    count = faces_in(out)
    passed = (code == 0 and count is not None and count >= min_faces)
    if count is None:
        evidence = 'no "faces:" line in the output'
    else:
        evidence = 'faces: ' + str(count) + '  (expected at least ' + str(min_faces) + ')'
    report(test_id, description, passed, evidence, out)


def test_identify(test_id, description, image, expected_name):
    code, out = execute([EXE, '--identify', image], BIN_DIR)
    names = names_in(out)
    passed = (code == 0 and expected_name in names)
    if len(names) == 0:
        evidence = 'no face line with a name= in the output'
    else:
        evidence = 'name(s): ' + ', '.join(names) + '  (expected ' + expected_name + ')'
    report(test_id, description, passed, evidence, out)


def test_liveness(test_id, description, video, expect_live):
    code, out = execute([EXE, '--liveness-replay', video], BIN_DIR)
    s = summary_in(out)
    if s is None:
        report(test_id, description, False, 'no summary line in the output', out)
        return
    if expect_live:
        passed = (code == 0 and s['live'] > s['photo'])
        wanted = 'expected LIVE to lead'
    else:
        passed = (code == 0 and s['photo'] > s['live'])
        wanted = 'expected PHOTO? to lead'
    evidence = ('blinks=' + str(s['blinks']) + '  LIVE=' + str(s['live']) +
                '  PHOTO?=' + str(s['photo']) + '  (' + wanted + ')')
    report(test_id, description, passed, evidence, out)


# ---------------------------------------------------------------- pre-flight

def check_file(path, label):
    if os.path.isfile(path):
        return True
    print('  missing: ' + label + '  ->  ' + path)
    return False


print('')
print('cc2-face-recognition - headless test runner')
print('Tester: Modar Issa')
print('repo: ' + REPO)
print('')
print('Pre-flight checks...')

ok = True
ok = check_file(EXE, 'facerec.exe') and ok
ok = check_file(BUILD, 'scripts/build.py') and ok
ok = check_file(os.path.join(DATA_DIR, MESSI_SINGLE.replace('/', os.sep)), MESSI_SINGLE) and ok
ok = check_file(os.path.join(DATA_DIR, GROUP_PHOTO.replace('/', os.sep)), GROUP_PHOTO) and ok
ok = check_file(os.path.join(DATA_DIR, MESSI_WC.replace('/', os.sep)), MESSI_WC) and ok
ok = check_file(os.path.join(DATA_DIR, RONALDO_WC.replace('/', os.sep)), RONALDO_WC) and ok
ok = check_file(ABEL_LIVE, 'test/videos/Abel_live.mp4') and ok
ok = check_file(ABEL_PHOTO, 'test/videos/Abel_photo.mp4') and ok

if not ok:
    print('')
    print('  Something is missing. If it is a sample or a model, run the bootstrap:')
    print('    py scripts/bootstrap.py')
    print('  If facerec.exe is missing, build it from an MSYS2 MinGW64 shell:')
    print('    python3 scripts/build.py')
    sys.exit(1)

print('  all files found')

# ---------------------------------------------------------------- run

section('S - self-test')
test_selftest()

section('D - detection')
test_detect('D4', 'detect a single face (messi5.jpg)', MESSI_SINGLE, 1)
test_detect('D5', 'detect faces in the group photo (group.pgm)', GROUP_PHOTO, 20)

section('R - recognition')
test_identify('R1', 'identify Messi', MESSI_WC, 'messi')
test_identify('R2', 'identify Ronaldo', RONALDO_WC, 'ronaldo')

section('L - liveness replay')
test_liveness('L7',  'live footage should read LIVE',   ABEL_LIVE,  True)
test_liveness('L7b', 'photo footage should read PHOTO?', ABEL_PHOTO, False)

# ---------------------------------------------------------------- summary

section('summary')

passed = 0
failed = 0
for test_id, status in results:
    if status == 'PASS':
        passed = passed + 1
    else:
        failed = failed + 1

print('  total:  ' + str(len(results)))
print('  passed: ' + str(passed))
print('  failed: ' + str(failed))
print('')

if failed == 0:
    print('  All headless tests passed.')
    sys.exit(0)
else:
    print('  ' + str(failed) + ' test(s) failed. See the output above.')
    sys.exit(1)
