"""Run each GC test in its own process; compare Stella stdout exactly."""

import difflib
import json
import os
from pathlib import Path
import subprocess
import sys


def run(name, command, expected=None, timeout=10):
    env = os.environ.copy()
    env.pop("STELLA_GC_STATS", None)
    env.pop("STELLA_GC_STATE", None)
    try:
        result = subprocess.run(
            command, capture_output=True, text=True, timeout=timeout, env=env
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"FAIL {name}: {error}")
        return False
    if result.returncode != 0 or (expected is not None and result.stdout != expected):
        print(f"FAIL {name}: exit code {result.returncode}")
        if expected is not None:
            print("".join(difflib.unified_diff(
                expected.splitlines(keepends=True),
                result.stdout.splitlines(keepends=True),
                fromfile="expected", tofile="actual",
            )), end="")
        elif result.stdout:
            print(result.stdout, end="")
        if result.stderr:
            print(result.stderr, end="")
        return False
    print(f"PASS {name}")
    return True


def main():
    mode, *paths = sys.argv[1:]
    results = []
    for path in paths:
        if mode == "c":
            results.append(run(Path(path).name, [path]))
        elif mode == "stella":
            try:
                cases = json.loads(Path(path).read_text())
                if not isinstance(cases, list) or not cases:
                    raise ValueError("expected a nonempty array of test cases")
                for case in cases:
                    args = case["args"]
                    expected = case["stdout"]
                    if not isinstance(args, list) or not all(isinstance(a, str) for a in args):
                        raise ValueError("args must be an array of strings")
                    if not isinstance(expected, str):
                        raise ValueError("stdout must be a string")
                    results.append(run(
                        f"{Path(path).stem}/{case['name']}",
                        [str(Path("build/tests/stella") / Path(path).stem), *args],
                        expected, case.get("timeout", 10),
                    ))
            except (OSError, ValueError, KeyError, TypeError) as error:
                print(f"FAIL {path}: invalid test specification: {error}")
                results.append(False)
        else:
            raise ValueError(f"unknown test mode: {mode}")
    if not results:
        print("FAIL: no tests found")
        return 1
    print(f"{sum(results)}/{len(results)} tests passed")
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
