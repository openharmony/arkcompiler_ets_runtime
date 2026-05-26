#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2022-2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Batch test script for ArkSteed
Runs all test cases in the test/ directory.
Each test case is a subdirectory containing:
  - <dirname>.ts (or any .ts file)
  - expected_output.txt
  - extra_options.txt (optional)

Usage:
  ./run_arksteed_tests.py [debug|release]
  Default: debug

Options:
  --exclude REGEX    Exclude test case directories by regex (comma-separated)
                    Example: --exclude "mega_ic_test,timeout_test"
  --rerun-failed-from-latest-log
                    Only run test cases that failed in the newest log under
                    out/arksteed_test_logs/, skipping cases that passed.
  --external-repo URL
                    Git URL of an external test-case repository.
                    If test/external/ does not exist, it is cloned automatically
                    (lazy fetch). Use --external-dir to change the sub-directory name.
  --external-dir NAME
                    Local sub-directory under test/ for external cases
                    (default: external).
  --skip-external   Skip cloning external test cases even when --external-repo is set.

Defaults:
  --print-graph and --check-live-range are enabled by default.
  Use --no-check-live-range to disable live range checking.
"""

from __future__ import annotations

import os
import json
import platform
import sys
import subprocess
import shlex
import argparse
import shutil
import tempfile
from pathlib import Path
import difflib
import threading
import re
import zipfile
from enum import Enum
from dataclasses import dataclass
from typing import Optional, List, Tuple, Dict
from concurrent.futures import ThreadPoolExecutor, as_completed
from xml.sax.saxutils import escape

__all__ = ["main", "run_test_case", "find_test_cases", "clean_test_cases"]

OUTPUT_LOCK = threading.Lock()
SUBPROCESS_OUTPUT_LOCK = threading.Lock()

SCRIPT_DIR = Path(__file__).resolve().parent
ARKSTEED_ROOT = SCRIPT_DIR.parent.parent.parent.parent
TEST_DIR = SCRIPT_DIR / "test"
LOG_DIR = ARKSTEED_ROOT / "out" / "arksteed_test_logs"

if not ARKSTEED_ROOT.exists():
    print(f"Error: ArkSteed root directory not found: {ARKSTEED_ROOT}", file=sys.stderr)
    print(
        "Please ensure the script is in the correct directory structure",
        file=sys.stderr,
    )
    sys.exit(1)


def _detect_platform() -> str:
    """Detect the build platform string based on host OS and architecture."""
    system = platform.system().lower()
    machine = platform.machine().lower()
    if system == "darwin" and machine == "arm64":
        return "mac_arm64"
    return "x64"


IS_MACOS = platform.system().lower() == "darwin"
HOST_PLATFORM = _detect_platform()
LIB_PATH_ENV_VAR = "DYLD_LIBRARY_PATH" if IS_MACOS else "LD_LIBRARY_PATH"


@dataclass(frozen=True)
class BuildConfig:
    mode: str
    platform: str = HOST_PLATFORM

    @property
    def _out_prefix(self) -> str:
        return f"out/{self.platform}.{self.mode}"

    @property
    def es2abc(self) -> Path:
        return ARKSTEED_ROOT / f"{self._out_prefix}/arkcompiler/ets_frontend/es2abc"

    @property
    def ark_disasm(self) -> Path:
        return (
            ARKSTEED_ROOT / f"{self._out_prefix}/arkcompiler/runtime_core/ark_disasm"
        )

    @property
    def ark_js_vm(self) -> Path:
        return ARKSTEED_ROOT / f"{self._out_prefix}/arkcompiler/ets_runtime/ark_js_vm"

    @property
    def lib_paths(self) -> List[Path]:
        paths = [
            ARKSTEED_ROOT / f"{self._out_prefix}/arkcompiler/ets_runtime",
            ARKSTEED_ROOT / f"{self._out_prefix}/thirdparty/icu",
            ARKSTEED_ROOT / f"{self._out_prefix}/thirdparty/zlib",
            ARKSTEED_ROOT / f"{self._out_prefix}/thirdparty/bounds_checking_function",
        ]
        if IS_MACOS:
            paths.extend([
                ARKSTEED_ROOT / f"{self._out_prefix}/thirdparty/libuv",
                ARKSTEED_ROOT / "prebuilts/clang/ohos/darwin-arm64/llvm/lib",
            ])
        else:
            paths.extend([
                ARKSTEED_ROOT / f"{self._out_prefix}/resourceschedule/frame_aware_sched",
                ARKSTEED_ROOT / f"{self._out_prefix}/hiviewdfx/hilog",
                ARKSTEED_ROOT / "prebuilts/clang/ohos/linux-x86_64/llvm/lib",
                ARKSTEED_ROOT / f"{self._out_prefix}/hmosbundlemanager/zlib_override/",
            ])
        return paths

    @property
    def stub_file(self) -> Optional[Path]:
        if not IS_MACOS:
            return None
        return ARKSTEED_ROOT / f"{self._out_prefix}/gen/arkcompiler/ets_runtime/stub.an"

    @property
    def icu_data_path(self) -> Optional[Path]:
        if not IS_MACOS:
            return None
        return ARKSTEED_ROOT / "third_party/icu/ohos_icu4j/data"


BASE_ARGS = [
    "--asm-interpreter=true",
    "--compiler-enable-jit=true",
    "--compiler-enable-litecg=true",
    "--enable-force-gc=false",
    "--enable-cmc-gc=false",
]

BUILD_MODES = ("debug", "release")
DEFAULT_EXECUTION_TIMEOUT = 3000

FILE_CLEAN_PATTERNS = ("disasm.txt", ".abc", ".actual_output.txt")
SOURCE_INPUT_SUFFIXES = (".ts", ".js")
BARE_HELPER_IMPORT_RE = re.compile(
    r"""(?P<prefix>\bimport\s+(?:[^'"]+\s+from\s+)?|require\()\s*['"](?P<spec>(?:@babel/runtime/helpers/[^'"]+|@js-joda/core|core-js/[^'"]+))['"]"""
)
SIDE_EFFECT_IMPORT_RE = re.compile(
    r"""^\s*import\s+['"](?P<spec>core-js/[^'"]+)['"];\s*$""",
    re.MULTILINE,
)
CJS_MARKER_RE = re.compile(
    r"""(?:\bmodule\.exports\b|\brequire\s*\(|typeof\s+exports\s*==|typeof\s+module\s*!==|typeof\s+module\s*==|typeof\s+exports\s*!==|commonjsGlobal|global\s*\|\|\s*self)"""
)
ESM_MARKER_RE = re.compile(r"""(^|\n)\s*(?:import\s+|export\s+)""")
JS_LINE_COMMENT_RE = re.compile(r"//.*?$", re.MULTILINE)
JS_BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)

CRASH_SIGNALS = {
    6: "SIGABRT",
    11: "SIGSEGV",
}

CRASH_RETURN_CODES = set(range(128, 128 + 32))

LIVE_RANGE_PATTERNS = {
    "lr": re.compile(r"(v\d+)/n\d+:\s+\S+.*?→\s+live range: \[(\d+)-(\d+)\]"),
    "use": re.compile(r"(v\d+)/n\d+:.*?\(([^)]+)\)"),
    "phi": re.compile(r"\bphi\b", re.IGNORECASE),
    "phi_line": re.compile(r"v\d+/n\d+:\s+Phi[^\(]*\(([^)]+)\)"),
}

LOG_LINE_PATTERN = re.compile(r"^\[[a-z]+\]")
LOG_CASE_NAME_PATTERN = re.compile(r"^Test case:\s*(.+)$")
LOG_RESULT_PATTERN = re.compile(r"^Result:\s*(PASS|FAIL)$")


PREPROCESSOR_BEGIN = "================ BEGIN: Result of BytecodePreprocessorNew ================"
PREPROCESSOR_END = "================ END: Result of BytecodePreprocessorNew ================"
COMPILER_LOG_PREFIX = "[compiler] "

LIVENESS_BEGIN = "================ BEGIN: Result of BytecodeAnalysisNew ================"
LIVENESS_END = "================ END: Result of BytecodeAnalysisNew ================"


def is_crash(returncode: Optional[int]) -> bool:
    if returncode is None:
        return False
    if returncode < 0:
        return abs(returncode) in CRASH_SIGNALS
    return returncode in CRASH_RETURN_CODES and (returncode - 128) in CRASH_SIGNALS


def get_crash_signal_name(returncode: Optional[int]) -> Optional[str]:
    if returncode is None:
        return None
    if returncode < 0:
        sig = abs(returncode)
    elif returncode >= 128:
        sig = returncode - 128
    else:
        return None
    return CRASH_SIGNALS.get(sig)


def format_gdb_command(result: TestResult) -> str:
    """Format debugger command for reproducing a crash."""
    if not result.cmd_str or not result.ld_library_path:
        return ""
    vm_path = result.cmd_str.split()[0] if result.cmd_str else ""
    args = result.cmd_str[len(vm_path) :].strip() if vm_path else result.cmd_str
    debugger = "lldb --" if IS_MACOS else "gdb --args"
    return f"Reproduce with command:\n{LIB_PATH_ENV_VAR}={result.ld_library_path} {debugger} {vm_path} {args}"


def _print_progress(current: int, total: int, test_name: str, ok: bool) -> None:
    status = "PASS" if ok else "FAIL"
    print(
        f"[PROGRESS] {current}/{total} completed ({status}): {test_name}",
        flush=True,
    )


def _latest_log_file(log_dir: Path = LOG_DIR) -> Optional[Path]:
    """Return the newest ArkSteed log file from the log directory."""
    if not log_dir.exists():
        return None

    candidates = [
        path
        for path in log_dir.glob("arksteed_tests_*.log")
        if path.is_file()
    ]
    if not candidates:
        return None
    return max(candidates, key=lambda path: path.stat().st_mtime)


def _failed_cases_from_log(log_file: Path) -> List[str]:
    """Extract failed case names from a previous ArkSteed log file."""
    failed_cases: List[str] = []
    seen = set()
    current_case: Optional[str] = None

    try:
        lines = log_file.read_text(encoding="utf-8", errors="ignore").splitlines()
    except Exception as exc:
        print(f"Error: failed to read log file {log_file}: {exc}", file=sys.stderr)
        return []

    for line in lines:
        case_match = LOG_CASE_NAME_PATTERN.match(line)
        if case_match:
            current_case = case_match.group(1).strip()
            continue

        result_match = LOG_RESULT_PATTERN.match(line)
        if result_match and current_case:
            if result_match.group(1) == "FAIL" and current_case not in seen:
                failed_cases.append(current_case)
                seen.add(current_case)
            current_case = None

    return failed_cases


def _filter_test_cases_by_case_names(
    test_cases: List[TestCase],
    case_names: List[str],
) -> List[TestCase]:
    """Keep only the test cases whose case_name appears in case_names."""
    if not case_names:
        return []

    allowed = set(case_names)
    filtered = [tc for tc in test_cases if tc.case_name in allowed]
    print(
        f"Selected {len(filtered)} test cases from {len(case_names)} failed case(s) in the latest log"
    )
    return filtered


@dataclass
class CommandResult:
    returncode: int
    stdout: str
    stderr: str


class CaseKind(Enum):
    DIR = "dir"
    SINGLE = "single"


def is_app_preheat_case(case_dir: Path) -> bool:
    app_preheat_root = TEST_DIR / "app_preheat"
    try:
        case_dir.relative_to(app_preheat_root)
        return True
    except ValueError:
        return False


@dataclass
class TestCase:
    case_dir: Path
    ts_file: Optional[Path]
    case_kind: CaseKind = CaseKind.SINGLE

    @property
    def ts_stem(self) -> str:
        if self.case_kind == CaseKind.DIR:
            return self.case_dir.name
        assert self.ts_file is not None
        return self.ts_file.stem

    @property
    def case_name(self) -> str:
        if self.case_kind == CaseKind.DIR:
            return self.case_dir.name
        return f"{self.case_dir.name}/{self.ts_stem}"

    @property
    def expected_file(self) -> Path:
        expected = self.case_dir / f"{self.ts_stem}_expected_output.txt"
        if expected.exists():
            return expected
        return self.case_dir / "expected_output.txt"

    @property
    def expected_preproc_file(self) -> Path:
        return self.case_dir / "expected_output.preproc.txt"

    @property
    def expected_liveness_file(self) -> Path:
        return self.case_dir / "expected_output.liveness.txt"

    @property
    def actual_file(self) -> Path:
        return self.case_dir / f"{self.ts_stem}.actual_output.txt"

    @property
    def abc_path(self) -> Path:
        return self.case_dir / f"{self.ts_stem}.abc"

    @property
    def disasm_path(self) -> Path:
        return self.case_dir / f"{self.ts_stem}.disasm.txt"

    @property
    def extra_options_file(self) -> Path:
        return self.case_dir / "extra_options.txt"


def _collect_source_inputs(test: TestCase) -> List[Path]:
    """Collect files that affect compilation for a test case."""
    if test.case_kind == CaseKind.DIR:
        inputs = list(test.case_dir.rglob("*.ts"))
        inputs.extend(test.case_dir.rglob("*.js"))
        file_info = test.case_dir / "fileInfo.txt"
        if file_info.exists():
            inputs.append(file_info)
        return inputs
    assert test.ts_file is not None
    return [test.ts_file]


def _is_up_to_date(target: Path, inputs: List[Path]) -> bool:
    """Return True when target exists and is newer than every input."""
    if not target.exists():
        return False

    try:
        target_mtime = target.stat().st_mtime
    except FileNotFoundError:
        return False

    for input_path in inputs:
        try:
            if input_path.stat().st_mtime > target_mtime:
                return False
        except FileNotFoundError:
            return False
    return True


@dataclass
class TestResult:
    case_name: str
    ok: bool
    message: str
    cmd_str: Optional[str] = None
    ld_library_path: Optional[str] = None
    output: Optional[str] = None
    returncode: Optional[int] = None


def run_command_real_time(
    cmd: List[str] | str,
    env: Optional[Dict[str, str]] = None,
    cwd: Optional[Path] = None,
    timeout: int = 30,
    verbose: bool = False,
) -> CommandResult:
    """Run command and collect stdout/stderr; stream output only in verbose mode."""
    if verbose:
        cmd_str = " ".join(cmd) if isinstance(cmd, list) else cmd
        print(f"Executing command: {cmd_str}")
        if cwd:
            print(f"Working directory: {cwd}")

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env,
        cwd=cwd,
        bufsize=1,
        universal_newlines=True,
    )

    stdout_lines: List[str] = []
    stderr_lines: List[str] = []

    def read_stdout() -> None:
        for line in iter(process.stdout.readline, ""):
            with SUBPROCESS_OUTPUT_LOCK:
                stdout_lines.append(line)
                if verbose:
                    sys.stdout.write(line)
                    sys.stdout.flush()
        process.stdout.close()

    def read_stderr() -> None:
        for line in iter(process.stderr.readline, ""):
            with SUBPROCESS_OUTPUT_LOCK:
                stderr_lines.append(line)
                if verbose:
                    sys.stderr.write(line)
                    sys.stderr.flush()
        process.stderr.close()

    stdout_thread = threading.Thread(target=read_stdout)
    stderr_thread = threading.Thread(target=read_stderr)
    stdout_thread.start()
    stderr_thread.start()

    try:
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        raise

    stdout_thread.join()
    stderr_thread.join()

    return CommandResult(
        process.returncode, "".join(stdout_lines), "".join(stderr_lines)
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Batch test ArkSteed")
    parser.add_argument(
        "mode",
        choices=BUILD_MODES,
        default="debug",
        nargs="?",
        help="Build mode: debug or release",
    )
    parser.add_argument("--skip-build", action="store_true", help="Skip build step")
    parser.add_argument(
        "--keep-going",
        type=int,
        default=0,
        help="Continue on error (N=allow N errors, 0=ignore all errors and continue)",
    )
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument(
        "-f",
        "--skip-stub",
        action="store_true",
        help="Skip stub file generation (add --gn-args=skip_gen_stub=true)",
    )
    parser.add_argument("--filter", type=str, help="Filter test case names by regex")
    parser.add_argument(
        "--exclude",
        type=str,
        help="Exclude test case directories by regex (comma-separated)",
    )
    parser.add_argument(
        "--rerun-failed-from-latest-log",
        action="store_true",
        help="Only rerun test cases that failed in the newest log file under out/arksteed_test_logs",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean generated files (disasm.txt, .abc, .actual_output.txt) in test directories",
    )
    parser.add_argument(
        "--log-level",
        type=str,
        help="Log level to pass to ark_js_vm (e.g., error, warning, info, debug)",
    )
    parser.add_argument(
        "--log-components",
        type=str,
        help="Log components to pass to ark_js_vm (comma-separated, e.g., runtime,compiler)",
    )
    parser.add_argument(
        "--print-graph",
        action="store_true",
        default=True,
        help="Enable ArkSteed graph printing during test execution",
    )
    parser.add_argument(
        "--check-live-range",
        dest="check_live_range",
        action="store_true",
        default=True,
        help="Check live range correctness after running each test (enabled by default; requires --print-graph)",
    )
    parser.add_argument(
        "--no-check-live-range",
        dest="check_live_range",
        action="store_false",
        help="Disable live range correctness check after running each test",
    )
    parser.add_argument(
        "--summary-output-path",
        type=str,
        default=None,
        help="Path to write test summary output. If the file exists and is non-empty, "
        "the script will exit with an error. If not specified, summary is written to stdout.",
    )
    parser.add_argument(
        "-j",
        "--num-workers",
        type=int,
        default=12,
        help="Number of worker threads for parallel test execution (default: 12)",
    )
    parser.add_argument(
        "--no-codex-analysis",
        dest="codex_analysis",
        action="store_false",
        default=True,
        help="Disable automatic Codex analysis of the generated reports",
    )
    parser.add_argument(
        "--codex-analysis-timeout",
        type=int,
        default=3000,
        help="Timeout in seconds for automatic Codex report analysis (default: 3000)",
    )
    parser.add_argument(
        "--hotness-threshold",
        type=int,
        default=1,
        help="Hotness threshold for JIT compilation (passed to --compiler-jit-hotness-threshold)",
    )
    parser.add_argument(
        "--external-repo",
        type=str,
        default=None,
        help="Git URL of an external test-case repository. "
             "If test/external/ does not exist, it is cloned automatically.",
    )
    parser.add_argument(
        "--external-dir",
        type=str,
        default="external",
        help="Local sub-directory under test/ for external cases (default: external)",
    )
    parser.add_argument(
        "--skip-external",
        action="store_true",
        help="Skip cloning external test cases even when --external-repo is set",
    )
    return parser.parse_args()


def build_ark(
    mode: str,
    verbose: bool = False,
    skip_stub: bool = False,
    keep_going: Optional[int] = None,
) -> bool:
    """Execute python ark.py {platform}.{mode} under ARKSTEED_ROOT"""
    target = f"{HOST_PLATFORM}.{mode}"
    print(f"Building ArkSteed ({target})...")
    cmd = ["python3", "ark.py", target]
    if skip_stub:
        cmd.append("--gn-args=skip_gen_stub=true")
    if keep_going is not None:
        cmd.append(f"--keep-going={keep_going}")

    try:
        result = run_command_real_time(
            cmd, cwd=ARKSTEED_ROOT, timeout=None, verbose=verbose
        )
        if result.returncode != 0:
            if verbose:
                print(
                    f"Build failed, return code: {result.returncode}", file=sys.stderr
                )
            return False
        print("Build successful")
        return True
    except subprocess.TimeoutExpired:
        print("Build timeout", file=sys.stderr)
        return False
    except Exception as e:
        print(f"Build exception: {e}", file=sys.stderr)
        return False


def find_test_cases(
    only_subdirs: Optional[List[str]] = None,
    all_files: bool = True,
    test_dir: Optional[Path] = None,
    exclude_subdirs: Optional[List[str]] = None,
) -> List[TestCase]:
    """Return list of test case directories."""
    scan_dir = test_dir if test_dir is not None else TEST_DIR
    if not scan_dir.exists():
        print(f"Test directory does not exist: {scan_dir}", file=sys.stderr)
        return []

    cases: List[TestCase] = []

    def get_source_files(directory: Path) -> List[Path]:
        ts_files = sorted(directory.glob("*.ts"))
        js_files = sorted(directory.glob("*.js"))
        return ts_files + js_files

    def add_cases_from_dir(directory: Path) -> bool:
        file_info = directory / "fileInfo.txt"
        if file_info.exists():
            cases.append(TestCase(directory, None, CaseKind.DIR))
            return True

        source_files = get_source_files(directory)
        if source_files:
            if all_files:
                for src_file in source_files:
                    cases.append(TestCase(directory, src_file, CaseKind.SINGLE))
            else:
                cases.append(TestCase(directory, source_files[0], CaseKind.SINGLE))
        return False

    def walk_recursive(directory: Path) -> None:
        """Recursively walk directories to find test cases."""
        if directory.name == "excluded":
            return
        if add_cases_from_dir(directory):
            return
        for sub_entry in sorted(directory.iterdir(), key=lambda x: x.name):
            if sub_entry.is_dir():
                walk_recursive(sub_entry)

    if only_subdirs is None:
        parent_dirs = [e for e in scan_dir.iterdir() if e.is_dir()]
    else:
        parent_dirs = [
            scan_dir / name for name in only_subdirs if (scan_dir / name).exists()
        ]

    if exclude_subdirs:
        parent_dirs = [
            d for d in parent_dirs if d.name not in exclude_subdirs
        ]

    for parent in sorted(parent_dirs, key=lambda x: x.name):
        walk_recursive(parent)

    return cases


def clean_test_cases() -> bool:
    """Clean generated files (disasm.txt, .abc, .actual_output.txt) recursively.
    
    NOTE: expected_output.txt, expected_output.preproc.txt, expected_output.liveness.txt
    are NOT cleaned as they are reference files for test validation.
    """
    if not TEST_DIR.exists():
        print(f"Test directory does not exist: {TEST_DIR}", file=sys.stderr)
        return False

    total_cleaned = 0
    for root, _, files in os.walk(TEST_DIR):
        for file in files:
            if any(file.endswith(pattern) for pattern in FILE_CLEAN_PATTERNS):
                file_path = Path(root) / file
                try:
                    file_path.unlink()
                    rel_path = file_path.relative_to(TEST_DIR)
                    print(f"  Cleaned: {rel_path}")
                    total_cleaned += 1
                except Exception as e:
                    print(f"  Cleanup failed: {rel_path}: {e}", file=sys.stderr)
                    return False

    print(f"Cleanup complete, deleted {total_cleaned} files")
    return True


def fetch_external_tests(repo_url: str, external_dir_name: str = "external") -> bool:
    """Lazy-clone external test cases from a Git repository.

    - If the external directory already exists, do nothing (lazy fetch).
    - If git is not available, print a warning and return False.
    - On clone failure, print a warning and return False.
    """
    external_path = TEST_DIR / external_dir_name
    if external_path.exists():
        return True

    if shutil.which("git") is None:
        print("Warning: git not found; cannot clone external test cases", file=sys.stderr)
        return False

    print(f"External test directory not found, cloning from {repo_url} ...")
    TEST_DIR.mkdir(parents=True, exist_ok=True)
    cmd = ["git", "clone", "--depth", "1", repo_url, str(external_path)]
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=120, check=False
        )
        if result.returncode != 0:
            err = result.stderr.strip() if result.stderr else "(unknown error)"
            print(f"Warning: failed to clone external tests: {err}", file=sys.stderr)
            return False
        print(f"External tests cloned to {external_path}")
        return True
    except subprocess.TimeoutExpired:
        print("Warning: git clone timed out after 120s", file=sys.stderr)
        return False
    except Exception as e:
        print(f"Warning: git clone failed: {e}", file=sys.stderr)
        return False


def read_extra_options(extra_file: Path) -> List[str]:
    """Read extra_options.txt, return parameter list."""
    if not extra_file.exists():
        return []
    with open(extra_file, "r") as f:
        lines = [
            line.strip() for line in f if line.strip() and not line.startswith("#")
        ]
    args: List[str] = []
    for line in lines:
        args.extend(shlex.split(line))
    return args


_PACKAGE_JSON_CACHE: Dict[Path, Optional[dict]] = {}


def _load_package_json(directory: Path) -> Optional[dict]:
    """Load the nearest package.json in a directory tree."""
    if directory in _PACKAGE_JSON_CACHE:
        return _PACKAGE_JSON_CACHE[directory]
    package_file = directory / "package.json"
    if not package_file.exists():
        _PACKAGE_JSON_CACHE[directory] = None
        return None
    try:
        data = json.loads(package_file.read_text(encoding="utf-8"))
    except Exception:
        data = None
    _PACKAGE_JSON_CACHE[directory] = data
    return data


def _strip_js_comments(text: str) -> str:
    """Strip JS comments before heuristically inspecting syntax markers."""
    text = JS_BLOCK_COMMENT_RE.sub("", text)
    return JS_LINE_COMMENT_RE.sub("", text)


def _find_package_type(source_path: Path) -> Optional[str]:
    """Find the nearest package.json type value for a source file."""
    for parent in [source_path.parent, *source_path.parents]:
        package_json = _load_package_json(parent)
        if package_json and isinstance(package_json, dict):
            package_type = package_json.get("type")
            if isinstance(package_type, str):
                return package_type.lower()
    return None


def _guess_script_kind(source_path: Path) -> str:
    """Guess whether a source should be compiled as module or commonjs."""
    try:
        text = source_path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return "module" if source_path.suffix == ".ts" else "commonjs"

    if "@babel/runtime/helpers" in source_path.as_posix():
        return "module"

    stripped_text = _strip_js_comments(text)
    has_esm = bool(ESM_MARKER_RE.search(stripped_text))
    has_cjs = bool(CJS_MARKER_RE.search(stripped_text))
    package_type = _find_package_type(source_path)

    if has_cjs and not has_esm:
        return "commonjs"
    if has_cjs and has_esm:
        return "commonjs"
    if has_esm:
        return "module"
    if package_type == "module":
        return "module"
    return "commonjs" if source_path.suffix == ".js" else "module"


def _package_root_for(source_path: Path) -> Optional[Path]:
    """Return the top-level test package directory for a source file."""
    try:
        rel = source_path.relative_to(TEST_DIR)
    except ValueError:
        return None
    if len(rel.parts) >= 2:
        return TEST_DIR / rel.parts[0] / rel.parts[1]
    if len(rel.parts) >= 1:
        return TEST_DIR / rel.parts[0]
    return None


def _create_temp_copy(
    text: str, suffix: str, prefix: str, dir_path: Optional[Path] = None
) -> Path:
    """Write text to a temporary file and return its path."""
    temp_file = tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        suffix=suffix,
        delete=False,
        prefix=prefix,
        dir=str(dir_path) if dir_path is not None else None,
    )
    try:
        temp_file.write(text)
        temp_file.flush()
    finally:
        temp_file.close()
    return Path(temp_file.name)


def _rewrite_single_source(
    source_path: Path, verbose: bool = False
) -> Tuple[Path, Optional[Path], List[Path]]:
    """Create a temporary rewritten copy for sources with bare imports."""
    try:
        original_text = source_path.read_text(encoding="utf-8")
    except Exception:
        return source_path, None, []

    rewritten_text = original_text
    changed = False
    package_root = _package_root_for(source_path)
    source_kind = _guess_script_kind(source_path)

    def rewrite_to_relative(spec: str, target_rel: str) -> None:
        nonlocal rewritten_text, changed
        if package_root is None:
            return
        target_path = package_root / target_rel
        rel_path = os.path.relpath(target_path, source_path.parent)
        rel_path = rel_path.replace("\\", "/")
        if spec in rewritten_text:
            rewritten_text = rewritten_text.replace(spec, rel_path)
            changed = True
            if verbose:
                print(f"Rewrote import {spec} -> {rel_path} for {source_path.name}")

    if "@js-joda/core" in rewritten_text and package_root is not None:
        core_target = "js-joda/core/dist/js-joda.cjs.js" if source_kind == "commonjs" else "js-joda/core/dist/js-joda.esm.js"
        rewrite_to_relative("@js-joda/core", core_target)

    if package_root is not None:
        helper_spec = "@babel/runtime/helpers/defineProperty"
        if helper_spec in rewritten_text:
            rewrite_to_relative(
                helper_spec,
                "@babel/runtime/helpers/defineProperty.js",
            )

    if "import assert from 'assert';" in rewritten_text or 'import assert from "assert";' in rewritten_text:
        assert_shim = (
            "const assert = {\n"
            "  strictEqual(actual, expected, message) {\n"
            "    if (actual !== expected) {\n"
            "      throw new Error(message || `Expected ${actual} to strictly equal ${expected}`);\n"
            "    }\n"
            "  },\n"
            "};\n"
        )
        rewritten_text = rewritten_text.replace("import assert from 'assert';", assert_shim)
        rewritten_text = rewritten_text.replace('import assert from "assert";', assert_shim)
        changed = True

    core_js_lines = SIDE_EFFECT_IMPORT_RE.findall(rewritten_text)
    if core_js_lines:
        rewritten_lines = []
        for line in rewritten_text.splitlines():
            if SIDE_EFFECT_IMPORT_RE.match(line):
                changed = True
                if verbose:
                    print(f"Stripped polyfill import in {source_path.name}: {line.strip()}")
                continue
            rewritten_lines.append(line)
        rewritten_text = "\n".join(rewritten_lines) + ("\n" if original_text.endswith("\n") else "")

    if not changed:
        return source_path, None, []

    temp_path = _create_temp_copy(rewritten_text, source_path.suffix, f"arksteed_{source_path.stem}_")
    return temp_path, temp_path, []


def compile_ts(
    ts_path: Path,
    output_abc: Path,
    tools: BuildConfig,
    verbose: bool = False,
    source_file: Optional[Path] = None,
) -> bool:
    """Use es2abc to compile .ts file to .abc."""
    if not tools.es2abc.exists():
        print(f"Error: es2abc does not exist: {tools.es2abc}", file=sys.stderr)
        return False
    cmd = [
        str(tools.es2abc),
        str(ts_path),
        "--merge-abc",
        "--output",
        str(output_abc),
    ]
    script_kind = _guess_script_kind(source_file or ts_path)
    if script_kind == "commonjs":
        cmd.append("--commonjs")
    else:
        cmd.append("--module")
    if source_file is not None:
        cmd.extend(["--source-file", str(source_file)])
    try:
        result = run_command_real_time(cmd, verbose=verbose, timeout=30)
        if result.returncode != 0:
            if verbose:
                print(
                    f"TypeScript compilation failed, return code: {result.returncode}",
                    file=sys.stderr,
                )
            return False
        return True
    except subprocess.TimeoutExpired:
        if verbose:
            print("Compilation timeout", file=sys.stderr)
        return False
    except Exception as e:
        if verbose:
            print(f"Compilation exception: {e}", file=sys.stderr)
        return False


def parse_file_info(file_info_path: Path) -> List[Dict[str, str]]:
    """Parse fileInfo.txt and return module rows."""
    rows: List[Dict[str, str]] = []
    with open(file_info_path, "r") as f:
        for raw_line in f:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            parts = [p.strip() for p in line.split(";")]
            if len(parts) < 3:
                continue
            rows.append(
                {
                    "file_name": parts[0],
                    "record_name": parts[1],
                    "script_kind": parts[2].lower(),
                }
            )
    return rows


def select_entry_from_file_info(
    rows: List[Dict[str, str]], fallback_name: str
) -> str:
    """Pick the VM entry point from fileInfo.txt."""
    for row in rows:
        record_name = row.get("record_name")
        if record_name == fallback_name:
            return record_name
    for row in rows:
        record_name = row.get("record_name")
        if record_name:
            return record_name
    return fallback_name


def compile_with_file_info(
    case_dir: Path,
    file_info_path: Path,
    output_abc: Path,
    tools: BuildConfig,
    verbose: bool = False,
) -> bool:
    """Compile a directory-form test case using fileInfo.txt."""
    if not tools.es2abc.exists():
        print(f"Error: es2abc does not exist: {tools.es2abc}", file=sys.stderr)
        return False

    cmd = [
        str(tools.es2abc),
        "--module",
        "--merge-abc",
        f"@{file_info_path}",
        "--output",
        str(output_abc),
    ]
    try:
        result = run_command_real_time(
            cmd, cwd=case_dir.parent, verbose=verbose, timeout=60
        )
        if result.returncode != 0:
            if verbose:
                print(
                    f"fileInfo compilation failed, return code: {result.returncode}",
                    file=sys.stderr,
                )
            return False
        return True
    except subprocess.TimeoutExpired:
        if verbose:
            print("fileInfo compilation timeout", file=sys.stderr)
        return False
    except Exception as e:
        if verbose:
            print(f"fileInfo compilation exception: {e}", file=sys.stderr)
        return False


def dump_ts_abc(
    abc_path: Path, output_path: Path, tools: BuildConfig, verbose: bool = False
) -> bool:
    """Use ark_disasm to print bytecode information."""
    if not tools.ark_disasm.exists():
        print(f"Error: ark_disasm does not exist: {tools.ark_disasm}", file=sys.stderr)
        return False
    cmd = [str(tools.ark_disasm), str(abc_path), str(output_path)]
    try:
        result = run_command_real_time(cmd, verbose=verbose, timeout=30)
        if result.returncode != 0:
            if verbose:
                print(
                    f"Disassembly failed, return code: {result.returncode}",
                    file=sys.stderr,
                )
            return False
        return True
    except subprocess.TimeoutExpired:
        if verbose:
            print("Disassembly timeout", file=sys.stderr)
        return False
    except Exception as e:
        if verbose:
            print(f"Disassembly exception: {e}", file=sys.stderr)
        return False


def _check_live_range_section(output: str) -> Tuple[bool, int, int, List[str]]:
    """
    Check live range correctness in one compiler graph section.
    Returns (ok, error_count, ok_count, error_messages)

    Checks:
    1. All phi input variables have live ranges
    2. For each use instruction A, the used variable B's end >= A's start
    """
    live_ranges: Dict[str, Tuple[int, int]] = {}
    uses: Dict[str, List[str]] = {}
    phi_inputs: set = set()

    for match in LIVE_RANGE_PATTERNS["lr"].finditer(output):
        var = match.group(1)
        start = int(match.group(2))
        end = int(match.group(3))
        live_ranges[var] = (start, end)

    for match in LIVE_RANGE_PATTERNS["use"].finditer(output):
        var = match.group(1)
        full_line = match.group(0)
        if LIVE_RANGE_PATTERNS["phi"].search(full_line):
            continue
        args_str = match.group(2)
        args = re.findall(r"(v\d+)/n\d+", args_str)
        if var not in uses:
            uses[var] = []
        uses[var].extend(args)

    for match in LIVE_RANGE_PATTERNS["phi_line"].finditer(output):
        args_str = match.group(1)
        args = re.findall(r"(v\d+)/n\d+", args_str)
        phi_inputs.update(args)

    errors: List[str] = []
    ok_count = 0

    for var in sorted(phi_inputs):
        if var not in live_ranges:
            errors.append(f"{var} is used as phi input but has no live range")

    for use_var, args in uses.items():
        if use_var not in live_ranges:
            continue
        use_start, use_end = live_ranges[use_var]
        for arg in args:
            if arg not in live_ranges:
                continue
            arg_start, arg_end = live_ranges[arg]
            if arg_end < use_start:
                errors.append(
                    f"{arg} [{arg_start}-{arg_end}] end < {use_var} [{use_start}-{use_end}] start={use_start}"
                )
            else:
                ok_count += 1

    return (len(errors) == 0, len(errors), ok_count, errors)


def check_live_range(output: str) -> Tuple[bool, int, int, List[str]]:
    """
    Check live range correctness in the output.

    One VM execution can compile several functions, and every ArkSteed graph
    restarts its virtual vertex numbering from v0. Checking the whole output as
    one graph lets a later function's vN live range overwrite an earlier
    function's vN live range, which reports false errors across unrelated
    graphs. Split by compiler section so vN uses are compared only within the
    graph that defines them.
    """
    sections = re.split(r"(?=^\[compiler\] Starts compiling )", output, flags=re.MULTILINE)
    total_errors = 0
    total_ok_count = 0
    all_errors: List[str] = []

    for section in sections:
        if "[compiler] ArkSteed IR Graph" not in section:
            continue
        _, error_count, ok_count, error_messages = _check_live_range_section(section)
        total_errors += error_count
        total_ok_count += ok_count
        all_errors.extend(error_messages)

    return (total_errors == 0, total_errors, total_ok_count, all_errors)


def _filter_log_lines(lines: List[str]) -> List[str]:
    """Remove log lines from output."""
    return [line for line in lines if not LOG_LINE_PATTERN.match(line)]


def _normalize_output(text: str) -> List[str]:
    """Normalize text output for comparison (strip trailing whitespace)."""
    return [line.rstrip() for line in text.splitlines()]


def compare_output(actual: str, expected_file: Path) -> Tuple[bool, Optional[str]]:
    """Compare actual output with expected file."""
    if not expected_file.exists():
        return True, None  # No expected file, skip output comparison
    with open(expected_file, "r") as f:
        expected = f.read()

    if not expected.strip():
        return True, None  # Empty expected file, skip output comparison

    expected_lines = _normalize_output(expected)
    actual_lines = _normalize_output(actual)
    actual_filtered = _filter_log_lines(actual_lines)

    if "\n".join(expected_lines) == "\n".join(actual_filtered):
        return True, None

    diff = difflib.unified_diff(
        expected_lines,
        actual_filtered,
        fromfile="expected",
        tofile="actual (filtered)",
        lineterm="",
    )
    return False, "\n".join(diff)


def extract_preprocessor_output(output: str) -> Optional[str]:
    """Extract BytecodePreprocessorNew output from log."""
    prefixed_begin = COMPILER_LOG_PREFIX + PREPROCESSOR_BEGIN
    prefixed_end = COMPILER_LOG_PREFIX + PREPROCESSOR_END

    has_begin = PREPROCESSOR_BEGIN in output or prefixed_begin in output
    has_end = PREPROCESSOR_END in output or prefixed_end in output
    if not has_begin or not has_end:
        return None

    start_idx = output.find(prefixed_begin) if prefixed_begin in output else output.find(PREPROCESSOR_BEGIN)
    end_idx = output.find(prefixed_end) if prefixed_end in output else output.find(PREPROCESSOR_END)
    if start_idx == -1 or end_idx == -1 or start_idx >= end_idx:
        return None

    preprocessor_output = output[start_idx:end_idx]
    lines = preprocessor_output.splitlines()[1:]  # Skip the BEGIN line
    filtered_lines = []

    for line in lines:
        if line.strip():
            if line.startswith(COMPILER_LOG_PREFIX):
                filtered_lines.append(line[len(COMPILER_LOG_PREFIX):].rstrip())
            else:
                filtered_lines.append(line.rstrip())

    return "\n".join(filtered_lines)


def compare_preprocessor_output(actual: str, expected_file: Path) -> Tuple[bool, Optional[str]]:
    """Compare preprocessor output with expected file."""
    if not expected_file.exists():
        return False, "Expected preprocessor output file does not exist"
    with open(expected_file, "r") as f:
        expected = f.read()

    expected_lines = _normalize_output(expected)
    actual_lines = _normalize_output(actual)

    if "\n".join(expected_lines) == "\n".join(actual_lines):
        return True, None

    diff = difflib.unified_diff(
        expected_lines,
        actual_lines,
        fromfile="expected_preproc",
        tofile="actual_preproc",
        lineterm="",
    )
    return False, "\n".join(diff)


def extract_liveness_output(output: str) -> Optional[str]:
    """Extract BytecodeAnalysisNew (liveness) output from log."""
    prefixed_begin = COMPILER_LOG_PREFIX + LIVENESS_BEGIN
    prefixed_end = COMPILER_LOG_PREFIX + LIVENESS_END

    has_begin = LIVENESS_BEGIN in output or prefixed_begin in output
    has_end = LIVENESS_END in output or prefixed_end in output
    if not has_begin or not has_end:
        return None

    start_idx = output.find(prefixed_begin) if prefixed_begin in output else output.find(LIVENESS_BEGIN)
    end_idx = output.find(prefixed_end) if prefixed_end in output else output.find(LIVENESS_END)
    if start_idx == -1 or end_idx == -1 or start_idx >= end_idx:
        return None

    liveness_output = output[start_idx:end_idx]
    lines = liveness_output.splitlines()[1:]  # Skip the BEGIN line
    filtered_lines = []

    for line in lines:
        if line.strip():
            if line.startswith(COMPILER_LOG_PREFIX):
                filtered_lines.append(line[len(COMPILER_LOG_PREFIX):].rstrip())
            else:
                filtered_lines.append(line.rstrip())

    return "\n".join(filtered_lines)


def compare_liveness_output(actual: str, expected_file: Path) -> Tuple[bool, Optional[str]]:
    """Compare liveness output with expected file."""
    if not expected_file.exists():
        return False, "Expected liveness output file does not exist"
    with open(expected_file, "r") as f:
        expected = f.read()

    expected_lines = _normalize_output(expected)
    actual_lines = _normalize_output(actual)

    if "\n".join(expected_lines) == "\n".join(actual_lines):
        return True, None

    diff = difflib.unified_diff(
        expected_lines,
        actual_lines,
        fromfile="expected_liveness",
        tofile="actual_liveness",
        lineterm="",
    )
    return False, "\n".join(diff)


def run_ark_vm(
    abc_path: Path,
    entry_point: str,
    extra_args: List[str],
    tools: BuildConfig,
    verbose: bool = False,
    log_level: Optional[str] = None,
    log_components: Optional[str] = None,
    print_graph: bool = False,
    hotness_threshold: int = 1,
    timeout: int = DEFAULT_EXECUTION_TIMEOUT,
) -> Tuple[Optional[CommandResult], str, str, bool]:
    """Use ark_js_vm to execute .abc file, return (result, cmd_str, ld_library_path, timed_out)."""
    if not tools.ark_js_vm.exists():
        print(f"Error: ark_js_vm does not exist: {tools.ark_js_vm}", file=sys.stderr)
        return None, "", "", False

    cmd = [str(tools.ark_js_vm)] + BASE_ARGS + extra_args
    if tools.stub_file is not None:
        cmd.append(f"--stub-file={tools.stub_file}")
    if tools.icu_data_path is not None:
        cmd.append(f"--icu-data-path={tools.icu_data_path}")
    if log_level is not None:
        cmd.append(f"--log-level={log_level}")
    if log_components is not None:
        cmd.append(f"--log-components={log_components}")
    cmd.append(f"--compiler-jit-hotness-threshold={hotness_threshold}")
    if print_graph:
        cmd.append("--compiler-arksteed-print-graph=true")
    cmd.append("--compiler-arksteed-print-method-name=false")
    cmd.append("--open-ark-tools=true")
    cmd.extend([f"--entry-point={entry_point}", str(abc_path)])
    cmd_str = " ".join(cmd)

    env = os.environ.copy()
    ld_library_path = ":".join(str(p) for p in tools.lib_paths)
    env[LIB_PATH_ENV_VAR] = ld_library_path
    if verbose:
        print(f"{LIB_PATH_ENV_VAR}: {ld_library_path}")

    try:
        result = run_command_real_time(cmd, env=env, timeout=timeout, verbose=verbose)
        return result, cmd_str, ld_library_path, False
    except subprocess.TimeoutExpired:
        if verbose:
            print("Execution timeout", file=sys.stderr)
        return None, cmd_str, ld_library_path, True
    except Exception as e:
        if verbose:
            print(f"Execution exception: {e}", file=sys.stderr)
        return None, cmd_str, ld_library_path, False


def compile_test_case(
    test: TestCase, tools: BuildConfig, verbose: bool
) -> Tuple[bool, str]:
    """Compile a test case. Returns (success, message)."""
    compile_inputs = _collect_source_inputs(test)
    compile_inputs.append(tools.es2abc)

    temp_source: Optional[Path] = None
    extra_temp_paths: List[Path] = []
    if test.case_kind != CaseKind.DIR:
        assert test.ts_file is not None
        compile_source, temp_source, extra_temp_paths = _rewrite_single_source(
            test.ts_file, verbose
        )

    abc_up_to_date = _is_up_to_date(test.abc_path, compile_inputs)
    disasm_up_to_date = _is_up_to_date(
        test.disasm_path, [test.abc_path, tools.ark_disasm]
    )

    if abc_up_to_date and temp_source is None:
        if not disasm_up_to_date:
            if not dump_ts_abc(test.abc_path, test.disasm_path, tools, verbose):
                return False, "Disassembly failed"
            if verbose:
                print(f"Reused cached ABC for {test.case_name}")
            return True, "Compilation reused (cache refreshed disasm)"
        if verbose:
            print(f"Reused cached ABC for {test.case_name}")
        return True, "Compilation reused (up to date)"

    if test.abc_path.exists():
        test.abc_path.unlink()
    if test.disasm_path.exists():
        test.disasm_path.unlink()

    if test.case_kind == CaseKind.DIR:
        if not compile_with_file_info(
            test.case_dir,
            test.case_dir / "fileInfo.txt",
            test.abc_path,
            tools,
            verbose,
        ):
            return False, "Compilation failed (fileInfo)"
    else:
        try:
            if not compile_ts(
                compile_source,
                test.abc_path,
                tools,
                verbose,
                source_file=test.ts_file,
            ):
                return False, "Compilation failed"
            for temp_dep_source in extra_temp_paths:
                dep_abc = temp_dep_source.with_suffix(".abc")
                if not compile_ts(
                    temp_dep_source,
                    dep_abc,
                    tools,
                    verbose,
                    source_file=temp_dep_source,
                ):
                    return False, "Compilation failed (dependency)"
        finally:
            if temp_source is not None:
                try:
                    Path(temp_source).unlink(missing_ok=True)
                except Exception:
                    pass
    if not dump_ts_abc(test.abc_path, test.disasm_path, tools, verbose):
        return False, "Disassembly failed"
    return True, "Compilation passed"


def execute_test_case(
    test: TestCase,
    tools: BuildConfig,
    verbose: bool,
    log_level: Optional[str],
    log_components: Optional[str],
    print_graph: bool,
    hotness_threshold: int = 1,
    entry_point: Optional[str] = None,
) -> Tuple[Optional[CommandResult], str, str, bool]:
    """Execute a compiled test case. Returns (result, cmd_str, ld_library_path, timed_out)."""
    if test.case_kind == CaseKind.DIR:
        file_info_path = test.case_dir / "fileInfo.txt"
        entry_point = select_entry_from_file_info(
            parse_file_info(file_info_path), test.case_dir.name
        )
    else:
        entry_point = entry_point or test.ts_stem
    timeout = DEFAULT_EXECUTION_TIMEOUT

    return run_ark_vm(
        test.abc_path,
        entry_point,
        read_extra_options(test.extra_options_file),
        tools,
        verbose,
        log_level,
        log_components,
        print_graph,
        hotness_threshold,
        timeout,
    )


def _format_live_range_errors(
    lr_errors: int, lr_ok_count: int, lr_error_msgs: List[str]
) -> str:
    """Format live range error messages for display."""
    msg = f"Live range check: {lr_errors} errors, {lr_ok_count} OK"
    for err in lr_error_msgs[:5]:
        msg += f"\n  ERROR: {err}"
    if len(lr_error_msgs) > 5:
        msg += f"\n  ... and {len(lr_error_msgs) - 5} more errors"
    return msg


def check_live_range_only(
    test: TestCase,
    actual_output: str,
    cmd_str: str,
    ld_library_path: str,
    check_live_range_flag: bool,
    returncode: int,
) -> TestResult:
    """Check result for app_preheat test case (live range only, no output comparison)."""
    if check_live_range_flag and actual_output:
        lr_ok, lr_errors, lr_ok_count, lr_error_msgs = check_live_range(actual_output)
        if not lr_ok:
            return TestResult(
                test.case_name,
                False,
                f"Live range check failed: {lr_errors} errors, {lr_ok_count} OK",
                cmd_str,
                ld_library_path,
                actual_output,
                returncode,
            )
    return TestResult(
        test.case_name,
        True,
        "Passed",
        cmd_str,
        ld_library_path,
        actual_output,
        returncode,
    )


def check_standard_test_result(
    test: TestCase,
    actual_output: str,
    cmd_str: str,
    ld_library_path: str,
    check_live_range_flag: bool,
    log_level: Optional[str],
    log_components: Optional[str],
    returncode: int = 0,
) -> TestResult:
    """Check test result for standard test cases (output comparison + optional live range)."""
    ok, diff = compare_output(actual_output, test.expected_file)

    if not ok:
        return TestResult(
            test.case_name,
            False,
            diff or "Comparison failed",
            cmd_str,
            ld_library_path,
            actual_output,
            returncode,
        )

    if check_live_range_flag and actual_output:
        lr_ok, lr_errors, lr_ok_count, lr_error_msgs = check_live_range(actual_output)
        if not lr_ok:
            return TestResult(
                test.case_name,
                False,
                _format_live_range_errors(lr_errors, lr_ok_count, lr_error_msgs),
                cmd_str,
                ld_library_path,
                actual_output,
                returncode,
            )

    if (log_level == "debug" and
        log_components and "compiler" in log_components and
        test.expected_preproc_file.exists()):
        preprocessor_output = extract_preprocessor_output(actual_output)
        if preprocessor_output:
            preproc_ok, preproc_diff = compare_preprocessor_output(preprocessor_output, test.expected_preproc_file)
            if not preproc_ok:
                return TestResult(
                    test.case_name,
                    False,
                    f"BytecodePreprocessorNew output mismatch:\n{preproc_diff}",
                    cmd_str,
                    ld_library_path,
                    actual_output,
                    returncode,
                )

    if (log_level == "debug" and
        log_components and "compiler" in log_components and
        test.expected_liveness_file.exists()):
        liveness_output = extract_liveness_output(actual_output)
        if liveness_output:
            liveness_ok, liveness_diff = compare_liveness_output(liveness_output, test.expected_liveness_file)
            if not liveness_ok:
                return TestResult(
                    test.case_name,
                    False,
                    f"BytecodeAnalysisNew (liveness) output mismatch:\n{liveness_diff}",
                    cmd_str,
                    ld_library_path,
                    actual_output,
                    returncode,
                )

    return TestResult(
        test.case_name,
        True,
        "Passed",
        cmd_str,
        ld_library_path,
        actual_output,
        returncode,
    )


def check_compiler_result(
    test: TestCase,
    actual_output: str,
    cmd_str: str,
    ld_library_path: str,
    check_live_range_flag: bool,
    log_level: Optional[str],
    log_components: Optional[str],
    returncode: int = 0,
) -> TestResult:
    """Check compiler stage results: RA live range, preproc, and liveness (no output comparison)."""
    has_preproc_or_liveness = (
        log_level == "debug" and
        log_components and
        "compiler" in log_components and
        (test.expected_preproc_file.exists() or test.expected_liveness_file.exists())
    )

    if not check_live_range_flag and not has_preproc_or_liveness:
        return TestResult(
            test.case_name,
            True,
            "Execution OK",
            cmd_str,
            ld_library_path,
            actual_output,
            returncode,
        )

    if check_live_range_flag and actual_output:
        lr_ok, lr_errors, lr_ok_count, lr_error_msgs = check_live_range(actual_output)
        if not lr_ok:
            return TestResult(
                test.case_name,
                False,
                _format_live_range_errors(lr_errors, lr_ok_count, lr_error_msgs),
                cmd_str,
                ld_library_path,
                actual_output,
                returncode,
            )

    if log_level == "debug" and log_components and "compiler" in log_components:
        if test.expected_preproc_file.exists():
            preprocessor_output = extract_preprocessor_output(actual_output)
            if preprocessor_output:
                preproc_ok, preproc_diff = compare_preprocessor_output(preprocessor_output, test.expected_preproc_file)
                if not preproc_ok:
                    return TestResult(
                        test.case_name,
                        False,
                        f"BytecodePreprocessorNew output mismatch:\n{preproc_diff}",
                        cmd_str,
                        ld_library_path,
                        actual_output,
                        returncode,
                    )

        if test.expected_liveness_file.exists():
            liveness_output = extract_liveness_output(actual_output)
            if liveness_output:
                liveness_ok, liveness_diff = compare_liveness_output(liveness_output, test.expected_liveness_file)
                if not liveness_ok:
                    return TestResult(
                        test.case_name,
                        False,
                        f"BytecodeAnalysisNew (liveness) output mismatch:\n{liveness_diff}",
                        cmd_str,
                        ld_library_path,
                        actual_output,
                        returncode,
                    )

    return TestResult(
        test.case_name,
        True,
        "Passed",
        cmd_str,
        ld_library_path,
        actual_output,
        returncode,
    )


def check_test_result(
    test: TestCase,
    actual_output: str,
    cmd_str: str,
    ld_library_path: str,
    check_live_range_flag: bool,
    log_level: Optional[str],
    log_components: Optional[str],
    returncode: int = 0,
) -> TestResult:
    """Check test result against expected output and live range. Returns TestResult."""
    if is_app_preheat_case(test.case_dir):
        return check_live_range_only(
            test,
            actual_output,
            cmd_str,
            ld_library_path,
            check_live_range_flag,
            returncode,
        )
    return check_standard_test_result(
        test, actual_output, cmd_str, ld_library_path, check_live_range_flag, log_level, log_components, returncode
    )


def _save_actual_output(test: TestCase, actual_output: str) -> None:
    """Save actual output to file for later inspection."""
    with open(test.actual_file, "w") as f:
        f.write(actual_output)


def _handle_execution_failure(
    test: TestCase,
    result: Optional[CommandResult],
    cmd_str: str,
    ld_library_path: str,
    timed_out: bool = False,
) -> TestResult:
    """Create TestResult for execution failure cases."""
    if result is None:
        if timed_out:
            message = "Execution failed (timeout)"
        else:
            message = "Execution failed (no result)"
        return TestResult(
            test.case_name,
            False,
            message,
            cmd_str,
            ld_library_path,
            None,
            None,
        )

    output = result.stderr + result.stdout
    return TestResult(
        test.case_name,
        False,
        f"Non-zero exit code: {result.returncode}\nstderr: {result.stderr}",
        cmd_str,
        ld_library_path,
        output,
        result.returncode,
    )


def _single_case_package_root(test: TestCase) -> Optional[Path]:
    """Return the top-level package root for a single-file test."""
    if test.ts_file is None:
        return None
    return _package_root_for(test.ts_file)


def _entry_point_for_single_source(source_path: Path, package_root: Path) -> str:
    """Derive the VM entry point for a source file inside a package root."""
    rel = source_path.relative_to(package_root)
    return rel.with_suffix("").as_posix()


def _build_package_file_info(
    package_root: Path,
    source_overrides: Optional[Dict[Path, Path]] = None,
) -> Tuple[Path, List[Path]]:
    """Build a temporary fileInfo.txt for every source file in a package tree."""
    source_overrides = source_overrides or {}
    temp_file = tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        suffix=".txt",
        delete=False,
        prefix=f"arksteed_{package_root.name}_pkg_fileInfo_",
    )
    temp_path = Path(temp_file.name)
    try:
        for source_path in sorted(
            list(package_root.rglob("*.js")) + list(package_root.rglob("*.ts"))
        ):
            if not source_path.is_file():
                continue
            if "embeddedDocs" in source_path.parts:
                continue
            file_source = source_overrides.get(source_path, source_path)
            try:
                rel_source = file_source.relative_to(package_root.parent).as_posix()
            except ValueError:
                rel_source = file_source.as_posix()
            record_name = source_path.relative_to(package_root).with_suffix("").as_posix()
            script_kind = _guess_script_kind(file_source)
            file_spec = rel_source if rel_source.startswith("/") else f"./{rel_source}"
            temp_file.write(
                f"{file_spec};{record_name};{script_kind};xxx;yyy\n"
            )
        temp_file.flush()
    finally:
        temp_file.close()
    return temp_path, [temp_path]


def _retry_single_case_with_package_file_info(
    test: TestCase,
    tools: BuildConfig,
    verbose: bool,
    log_level: Optional[str],
    log_components: Optional[str],
    print_graph: bool,
    check_live_range_flag: bool,
    hotness_threshold: int = 1,
) -> Optional[TestResult]:
    """Retry a single-file case by compiling the whole package tree when needed."""
    if test.ts_file is None:
        return None
    package_root = _single_case_package_root(test)
    if package_root is None:
        return None

    rewritten_source, temp_source, extra_temp_paths = _rewrite_single_source(
        test.ts_file, verbose
    )
    file_info_path = package_root / "fileInfo.txt"
    temp_file_info: Optional[Path] = None
    compile_file_info_path = file_info_path
    if not file_info_path.exists():
        compile_file_info_path, temp_paths = _build_package_file_info(
            package_root, {test.ts_file: rewritten_source}
        )
        temp_file_info = temp_paths[0]
    else:
#Use the rewritten source if the package already provides fileInfo.txt.
        rewritten_source = temp_source or rewritten_source
        if temp_source is not None:
            compile_file_info_path, temp_paths = _build_package_file_info(
                package_root, {test.ts_file: rewritten_source}
            )
            temp_file_info = temp_paths[0]

    try:
        if not compile_with_file_info(
            package_root,
            compile_file_info_path,
            test.abc_path,
            tools,
            verbose,
        ):
            return None

        entry_point = _entry_point_for_single_source(test.ts_file, package_root)
        result, cmd_str, ld_library_path, timed_out = run_ark_vm(
            test.abc_path,
            entry_point,
            read_extra_options(test.extra_options_file),
            tools,
            verbose,
            log_level,
            log_components,
            print_graph,
            hotness_threshold,
            True,
        )
        if result is None or result.returncode != 0:
            return _handle_execution_failure(test, result, cmd_str, ld_library_path, timed_out)

        actual_output = result.stdout
        _save_actual_output(test, actual_output)
        return check_test_result(
            test,
            actual_output,
            cmd_str,
            ld_library_path,
            check_live_range_flag,
            log_level,
            log_components,
            result.returncode,
        )
    finally:
        if temp_file_info is not None:
            try:
                Path(temp_file_info).unlink(missing_ok=True)
            except Exception:
                pass
        if temp_source is not None:
            try:
                Path(temp_source).unlink(missing_ok=True)
            except Exception:
                pass
        for temp_path in extra_temp_paths:
            try:
                Path(temp_path).unlink(missing_ok=True)
            except Exception:
                pass


def _needs_package_file_info_retry(output: Optional[str]) -> bool:
    """Decide whether a failed single-file run should retry with package fileInfo."""
    if not output:
        return False
    retry_markers = (
        "requested module",
        "does not provide an export name",
        "Cannot find module",
        "Load file with filename",
        "open file ",
        "Can't fopen location",
    )
    return any(marker in output for marker in retry_markers)


def run_test_case(
    test: TestCase,
    mode: str,
    verbose: bool = False,
    log_level: Optional[str] = None,
    log_components: Optional[str] = None,
    print_graph: bool = False,
    check_live_range_flag: bool = False,
    stage: str = "compile",
    hotness_threshold: int = 1,
) -> TestResult:
    """Run a single test case. Returns TestResult.

    Stage behavior:
      compile - compile + disassemble only
      jit     - compile + execute with ArkSteed JIT + full comparison
    """
    tools = BuildConfig(mode)

    ok, msg = compile_test_case(test, tools, verbose)
    if not ok:
        return TestResult(test.case_name, False, msg, None, None, None, None)

    if stage == "compile":
        return TestResult(test.case_name, True, "Compilation passed", None, None, None, 0)

    result, cmd_str, ld_library_path, timed_out = execute_test_case(
        test, tools, verbose, log_level, log_components, print_graph, hotness_threshold
    )

    if result is None or result.returncode != 0:
        return _handle_execution_failure(test, result, cmd_str, ld_library_path, timed_out)

    actual_output = result.stdout
    _save_actual_output(test, actual_output)

    return check_test_result(
        test,
        actual_output,
        cmd_str,
        ld_library_path,
        check_live_range_flag,
        log_level,
        log_components,
        result.returncode,
    )


def _setup_logging() -> Tuple[Path, str]:
    """Create log directory and file. Returns (log_file, timestamp)."""
    LOG_DIR.mkdir(exist_ok=True, parents=True)
    from datetime import datetime

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    log_file = LOG_DIR / f"arksteed_tests_{timestamp}.log"
    return log_file, timestamp


def _log_file_for_mode(timestamp: str, run_mode: str) -> Path:
    return LOG_DIR / f"arksteed_tests_{run_mode}_{timestamp}.log"


def _xlsx_file_for_mode(timestamp: str, run_mode: str) -> Path:
    return LOG_DIR / f"arksteed_tests_{run_mode}_{timestamp}.xlsx"


def _analysis_file_for_timestamp(timestamp: str) -> Path:
    return LOG_DIR / f"arksteed_tests_analysis_{timestamp}.md"


def _app_preheat_analysis_file_for_timestamp(timestamp: str) -> Path:
    return LOG_DIR / f"arksteed_tests_app_preheat_analysis_{timestamp}.md"


def _codex_has_local_config() -> bool:
    """Return true when Codex has local auth/config or API-key based configuration."""
    if os.environ.get("OPENAI_API_KEY") or os.environ.get("CODEX_API_KEY"):
        return True

    codex_home = Path(os.environ.get("CODEX_HOME", Path.home() / ".codex"))
    auth_file = codex_home / "auth.json"
    if auth_file.exists() and auth_file.stat().st_size > 2:
        return True

    return False


def _run_codex_report_analysis(
    timestamp: str,
    mode: str,
    report_files: List[Tuple[str, Path, Path]],
    timeout: int,
) -> Optional[Path]:
    """Ask Codex CLI to analyze generated test reports and save the conclusion."""
    codex = shutil.which("codex")
    if codex is None:
        print("Warning: codex CLI not found; skipping report analysis.", file=sys.stderr)
        return None
    if not _codex_has_local_config():
        print("Warning: codex is not configured locally; skipping report analysis.", file=sys.stderr)
        return None

    analysis_path = _analysis_file_for_timestamp(timestamp)
    report_lines = "\n".join(
        f"- {run_stage}: log={log_file}, excel={xlsx_file}"
        for run_stage, log_file, xlsx_file in report_files
    )
    prompt = f"""Analyze the full ArkSteed test log files from this run and output a concise conclusion in Chinese.

Workspace: {ARKSTEED_ROOT}
Build mode: {mode}
Reports:
{report_lines}

Read the full log files directly, not a pre-summarized report. Focus on:
1. compile mode pass/fail summary;
2. jit mode pass/fail summary;
3. top failure categories and representative test cases;
4. whether failures are crashes, output mismatches, live-range/preprocessor/liveness mismatches, or other errors;
5. recommended next debugging direction.

Keep the answer practical and concise. Do not modify files.
"""
    cmd = [
        codex,
        "exec",
        "--skip-git-repo-check",
        "--color",
        "never",
        "-s",
        "read-only",
        "-C",
        str(ARKSTEED_ROOT),
        "-o",
        str(analysis_path),
        prompt,
    ]

    print("\nAnalyzing reports with Codex...")
    sys.stdout.flush()
    try:
        result = run_command_real_time(cmd, timeout=timeout)
    except subprocess.TimeoutExpired:
        print("Warning: Codex report analysis timeout.", file=sys.stderr)
        return None
    except Exception as e:
        print(f"Warning: Codex report analysis failed: {e}", file=sys.stderr)
        return None

    if result.returncode != 0:
        print(
            f"Warning: Codex report analysis failed, return code: {result.returncode}",
            file=sys.stderr,
        )
        return None

    if analysis_path.exists():
        print(f"Codex analysis saved to: {analysis_path}")
        return analysis_path

    print("Warning: Codex report analysis finished without output file.", file=sys.stderr)
    return None


def _is_app_preheat_result(result: TestResult) -> bool:
    return bool(result.cmd_str and "/test/app_preheat/" in result.cmd_str)


def _summarize_app_preheat_results(results: List[TestResult]) -> Tuple[int, int, List[str]]:
    app_preheat_results = [r for r in results if _is_app_preheat_result(r)]
    failed = [r for r in app_preheat_results if not r.ok]
    lines: List[str] = []
    for result in app_preheat_results:
        status = "PASS" if result.ok else "FAIL"
        reason = result.message or "(no message)"
        if is_crash(result.returncode):
            sig_name = get_crash_signal_name(result.returncode)
            if sig_name:
                reason = f"{reason} [{sig_name}]"
        lines.append(f"- {result.case_name}: {status}; {reason}")
    return len(app_preheat_results), len(failed), lines


def _run_codex_app_preheat_analysis(
    timestamp: str,
    mode: str,
    report_files: List[Tuple[str, Path, Path]],
    results: List[TestResult],
    timeout: int,
) -> Optional[Path]:
    """Ask Codex CLI to analyze only app_preheat failures and save the conclusion."""
    codex = shutil.which("codex")
    if codex is None:
        print("Warning: codex CLI not found; skipping app_preheat analysis.", file=sys.stderr)
        return None
    if not _codex_has_local_config():
        print("Warning: codex is not configured locally; skipping app_preheat analysis.", file=sys.stderr)
        return None

    total_cases, failed_cases, case_lines = _summarize_app_preheat_results(results)
    if total_cases == 0:
        return None
    if failed_cases == 0:
        print("No app_preheat failures detected; skipping app_preheat Codex analysis.")
        return None

    analysis_path = _app_preheat_analysis_file_for_timestamp(timestamp)
    report_lines = "\n".join(
        f"- {run_stage}: log={log_file}, excel={xlsx_file}"
        for run_stage, log_file, xlsx_file in report_files
    )
    case_summary = "\n".join(case_lines)
    prompt = f"""Analyze only the app_preheat-related ArkSteed test cases from this run and output a concise conclusion in Chinese.

Workspace: {ARKSTEED_ROOT}
Build mode: {mode}
Reports:
{report_lines}

app_preheat case summary:
{case_summary}

Tasks:
1. Count how many app_preheat cases were run and how many failed.
2. List each failed case and its direct reason.
3. Group failures by root cause when possible.
4. State whether the dominant issue looks like timeout/no-result, crash, output mismatch, or live-range/preprocessor/liveness mismatch.
5. Give the most likely next debugging direction.

Read the full log files directly if needed. Do not modify files.
"""
    cmd = [
        codex,
        "exec",
        "--skip-git-repo-check",
        "--color",
        "never",
        "-s",
        "read-only",
        "-C",
        str(ARKSTEED_ROOT),
        "-o",
        str(analysis_path),
        prompt,
    ]

    print("\nAnalyzing app_preheat failures with Codex...")
    sys.stdout.flush()
    try:
        result = run_command_real_time(cmd, timeout=timeout)
    except subprocess.TimeoutExpired:
        print("Warning: app_preheat Codex analysis timeout.", file=sys.stderr)
        return None
    except Exception as e:
        print(f"Warning: app_preheat Codex analysis failed: {e}", file=sys.stderr)
        return None

    if result.returncode != 0:
        print(
            f"Warning: app_preheat Codex analysis failed, return code: {result.returncode}",
            file=sys.stderr,
        )
        return None

    if analysis_path.exists():
        print(f"App preheat Codex analysis saved to: {analysis_path}")
        return analysis_path

    print("Warning: app_preheat Codex analysis finished without output file.", file=sys.stderr)
    return None


SPECIAL_SUBDIR_FILTERS = {"aottest", "jittest"}


def _parse_filter_string(
    filter_str: Optional[str],
) -> Tuple[Optional[List[str]], Optional[str], Optional[Path], Optional[str]]:
    """
    Parse filter argument into (subdirs, regex_pattern, path_prefix, dir_name).
    Handles formats like "subdir/regex", "aottest", "jittest",
    directory filters like "fundamental/if_simple", and nested directory
    name filters like "fundamental".
    """
    if not filter_str:
        return None, None, None, None

    if "/" in filter_str:
        path_candidate = TEST_DIR / filter_str
        if path_candidate.is_dir():
            return None, None, path_candidate.relative_to(TEST_DIR), None

        parts = filter_str.split("/", 1)
        subdirs = [parts[0]] if parts[0] else None
        regex = parts[1] if len(parts) > 1 else None
        return subdirs, regex, None, None

    if (TEST_DIR / filter_str).is_dir():
        return [filter_str], None, None, None

    if filter_str in SPECIAL_SUBDIR_FILTERS:
        return [filter_str], None, None, None

    nested_dir_matches = [
        p for p in TEST_DIR.rglob(filter_str) if p.is_dir() and p != TEST_DIR / filter_str
    ]
    if nested_dir_matches:
        return None, None, None, filter_str

    return None, filter_str, None, None


def _apply_filter_path_prefix(
    test_cases: List[TestCase],
    path_prefix: Optional[Path],
) -> List[TestCase]:
    """Apply relative directory prefix filter such as fundamental/if_simple."""
    if path_prefix is None:
        return test_cases

    filtered: List[TestCase] = []
    for tc in test_cases:
        try:
            rel = tc.case_dir.relative_to(TEST_DIR)
        except ValueError:
            continue
        if rel == path_prefix or path_prefix in rel.parents:
            filtered.append(tc)

    print(
        f"After filtering by path '{path_prefix.as_posix()}', {len(filtered)} test cases remain"
    )
    return filtered


def _apply_filter_dir_name(
    test_cases: List[TestCase],
    dir_name: Optional[str],
) -> List[TestCase]:
    """Apply nested directory-name filter such as fundamental."""
    if not dir_name:
        return test_cases

    filtered: List[TestCase] = []
    for tc in test_cases:
        try:
            rel = tc.case_dir.relative_to(TEST_DIR)
        except ValueError:
            continue
        if dir_name in rel.parts:
            filtered.append(tc)

    print(
        f"After filtering by directory name '{dir_name}', {len(filtered)} test cases remain"
    )
    return filtered


def _apply_filter_regex(
    test_cases: List[TestCase],
    filter_regex: Optional[str],
) -> List[TestCase]:
    """Apply regex filter to test cases, matching against case_dir.name."""
    if not filter_regex:
        return test_cases

    try:
        pattern = re.compile(filter_regex)
        filtered = [tc for tc in test_cases if pattern.fullmatch(tc.case_dir.name)]
        print(
            f"After filtering by regex '{filter_regex}', {len(filtered)} test cases remain"
        )
        return filtered
    except re.error as e:
        print(f"Error: Invalid regex '{filter_regex}': {e}", file=sys.stderr)
        sys.exit(1)


def _apply_exclude_patterns(
    test_cases: List[TestCase],
    exclude_str: Optional[str],
) -> List[TestCase]:
    """Apply exclude patterns (comma-separated regexes) to test cases."""
    if not exclude_str:
        return test_cases

    exclude_patterns = [p.strip() for p in exclude_str.split(",") if p.strip()]
    if not exclude_patterns:
        return test_cases

    try:
        exclude_regexes = [re.compile(p) for p in exclude_patterns]
    except re.error as e:
        print(
            f"Error: Invalid regex in --exclude '{exclude_str}': {e}", file=sys.stderr
        )
        sys.exit(1)

    original_count = len(test_cases)
    filtered = [
        tc
        for tc in test_cases
        if not any(
            r.search(str(tc.case_dir.relative_to(TEST_DIR))) for r in exclude_regexes
        )
    ]
    excluded_count = original_count - len(filtered)
    print(f"Excluded {excluded_count} test cases matching '{exclude_str}'")
    return filtered


def _filter_and_exclude_tests(
    filter_path_prefix: Optional[Path],
    filter_dir_name: Optional[str],
    filter_regex: Optional[str],
    exclude_str: Optional[str],
    test_cases: List[TestCase],
) -> List[TestCase]:
    """Filter and exclude test cases based on command-line arguments."""
    filtered = test_cases

    if filter_path_prefix is not None:
        filtered = _apply_filter_path_prefix(filtered, filter_path_prefix)

    if filter_dir_name is not None:
        filtered = _apply_filter_dir_name(filtered, filter_dir_name)

    if filter_regex:
        filtered = _apply_filter_regex(filtered, filter_regex)

    if exclude_str:
        filtered = _apply_exclude_patterns(filtered, exclude_str)

    return filtered


def _run_single_test(
    test: TestCase,
    mode: str,
    verbose: bool,
    log_level: Optional[str],
    log_components: Optional[str],
    print_graph: bool,
    check_live_range_flag: bool,
    run_stage: str,
    hotness_threshold: int = 1,
) -> TestResult:
    """Run a single test case with atomic output. Used by both sequential and parallel execution."""
    if verbose:
        with OUTPUT_LOCK:
            print(f"\nRunning {run_stage} test case: {test.case_name}", flush=True)

    result = run_test_case(
        test,
        mode,
        verbose,
        log_level,
        log_components,
        print_graph,
        check_live_range_flag,
        run_stage,
        hotness_threshold,
    )

    if verbose:
        with OUTPUT_LOCK:
            if result.ok:
                print(f"[PASS] {test.case_name}", flush=True)
            else:
                print(f"[FAIL] {test.case_name}: {result.message}", flush=True)

    return result


def _run_all_tests(
    test_cases: List[TestCase],
    mode: str,
    verbose: bool,
    keep_going: int,
    log_level: Optional[str],
    log_components: Optional[str],
    print_graph: bool,
    check_live_range_flag: bool,
    run_stage: str = "compile",
    num_workers: int = 1,
    hotness_threshold: int = 1,
) -> Tuple[int, int, List[TestResult]]:
    """Run all test cases and collect results. Returns (passed, failed, results)."""
    results: List[TestResult] = []
    results_lock = threading.Lock()
    progress_lock = threading.Lock()
    failed_count = 0
    should_stop = False
    completed_count = 0

    def run_with_tracking(test: TestCase) -> TestResult:
        nonlocal failed_count, should_stop, completed_count
        result = _run_single_test(
            test,
            mode,
            verbose,
            log_level,
            log_components,
            print_graph,
            check_live_range_flag,
            run_stage,
            hotness_threshold,
        )
        with results_lock:
            if not result.ok:
                failed_count += 1
                if keep_going > 0 and failed_count >= keep_going:
                    should_stop = True
        with progress_lock:
            completed_count += 1
            _print_progress(completed_count, len(test_cases), result.case_name, result.ok)
        return result

    if num_workers <= 1:
        for test in test_cases:
            if should_stop:
                remaining_result = TestResult(
                    test.case_name, True, "Skipped (stopped by --keep-going)", None, None, None, None
                )
                results.append(remaining_result)
                with progress_lock:
                    completed_count += 1
                    _print_progress(
                        completed_count, len(test_cases), remaining_result.case_name, remaining_result.ok
                    )
                continue
            result = run_with_tracking(test)
            results.append(result)
    else:
        with ThreadPoolExecutor(max_workers=num_workers) as executor:
            future_to_test = {
                executor.submit(run_with_tracking, test): test
                for test in test_cases
            }
            for future in as_completed(future_to_test):
                test = future_to_test[future]
                if should_stop:
                    remaining_result = TestResult(
                        test.case_name, True, "Skipped (stopped by --keep-going)", None, None, None, None
                    )
                    results.append(remaining_result)
                    with progress_lock:
                        completed_count += 1
                        _print_progress(
                            completed_count,
                            len(test_cases),
                            remaining_result.case_name,
                            remaining_result.ok,
                        )
                    continue
                try:
                    result = future.result()
                    results.append(result)
                except Exception as exc:
                    result = TestResult(
                        test.case_name, False, f"Exception: {exc}", None, None, None, None
                    )
                    results.append(result)

    passed = len([r for r in results if r.ok])
    failed = len([r for r in results if not r.ok])

    return passed, failed, results


def _report_results(
    log_file: Path,
    timestamp: str,
    mode: str,
    test_cases: List[TestCase],
    results: List[TestResult],
    passed: int,
    failed: int,
    summary_output_path: Optional[str] = None,
    report_name: str = "test",
    summary_mode: str = "w",
    show_failures: bool = False,
) -> None:
    """Write log file and print summary."""
    summary_file: Optional[object] = None
    if summary_output_path is not None:
        summary_file = open(summary_output_path, summary_mode)
        summary_output = summary_file
    else:
        summary_output = sys.stdout

    with open(log_file, "w") as log:
        log.write(f"ArkSteed test log - {timestamp}\n")
        log.write(f"Report: {report_name}\n")
        log.write(f"Platform: {HOST_PLATFORM}\n")
        log.write(f"Mode: {mode}\n")
        log.write(f"Total test cases: {len(test_cases)}\n")
        log.write(f"Passed: {passed}, Failed: {failed}\n")
        log.write("=" * 80 + "\n\n")

        for result in results:
            log.write(f"Test case: {result.case_name}\n")
            log.write(f"Result: {'PASS' if result.ok else 'FAIL'}\n")
            if result.cmd_str:
                log.write(f"Command: {result.cmd_str}\n")
            if result.ld_library_path:
                log.write(f"{LIB_PATH_ENV_VAR}: {result.ld_library_path}\n")
            if result.message and not result.ok:
                log.write(f"Error message: {result.message}\n")
            if is_crash(result.returncode):
                sig_name = get_crash_signal_name(result.returncode)
                log.write(f"Signal: {sig_name}\n")
                log.write(f"{format_gdb_command(result)}\n")
            if result.output:
                log.write("Output:\n")
                log.write("-" * 40 + "\n")
                log.write(result.output)
                log.write("\n" + "-" * 40 + "\n")
            else:
                log.write("Output: (none)\n")
            log.write("\n" + "=" * 80 + "\n\n")

    print(f"\n=== {report_name} Report ===", file=summary_output)

    if failed > 0 and show_failures:
        print("\n=== Failed Test Cases ===", file=summary_output)
        for result in results:
            if not result.ok:
                print(f"\n[FAIL] {result.case_name}", file=summary_output)
                if result.message:
                    print(
                        f"  Error: {result.message[:200]}{'...' if len(result.message) > 200 else ''}",
                        file=summary_output,
                    )
                if is_crash(result.returncode):
                    sig_name = get_crash_signal_name(result.returncode)
                    print(f"  {sig_name} detected", file=summary_output)
                    print(f"  {format_gdb_command(result)}", file=summary_output)
        print(file=summary_output)

    print("=== Test Summary ===", file=summary_output)
    print(f"Passed: {passed}", file=summary_output)
    print(f"Failed: {failed}", file=summary_output)
    if summary_file is not None:
        print(f"\nDetailed log saved to: {log_file}", file=summary_output)

    if summary_file is not None:
        summary_file.close()


def _excel_col_name(index: int) -> str:
    name = ""
    while index:
        index, remainder = divmod(index - 1, 26)
        name = chr(ord("A") + remainder) + name
    return name


def _xlsx_cell(row: int, col: int, value: object, style_id: int = 0) -> str:
    ref = f"{_excel_col_name(col)}{row}"
    text = "" if value is None else str(value)
    style_attr = f' s="{style_id}"' if style_id else ""
    return f'<c r="{ref}"{style_attr} t="inlineStr"><is><t>{escape(text)}</t></is></c>'


def _xlsx_sheet(rows: List[List[object]]) -> str:
    xml_rows: List[str] = []
    for row_index, row in enumerate(rows, start=1):
        style_id = 0
        if row_index > 1 and len(row) > 1:
            if row[1] == "PASS":
                style_id = 1
            elif row[1] == "FAIL":
                style_id = 2
        cells = "".join(
            _xlsx_cell(row_index, col_index, value, style_id)
            for col_index, value in enumerate(row, start=1)
        )
        xml_rows.append(f'<row r="{row_index}">{cells}</row>')
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
        f'<sheetData>{"".join(xml_rows)}</sheetData>'
        '</worksheet>'
    )


def _xlsx_rows(results: List[TestResult]) -> List[List[object]]:
    rows: List[List[object]] = [["Case", "Result", "Message", "Command"]]
    for result in sorted(results, key=lambda r: r.case_name):
        rows.append([
            result.case_name,
            "PASS" if result.ok else "FAIL",
            result.message,
            result.cmd_str or "",
        ])
    return rows


def _xlsx_styles() -> str:
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
        '<fonts count="1"><font><sz val="11"/><name val="Calibri"/></font></fonts>'
        '<fills count="4">'
        '<fill><patternFill patternType="none"/></fill>'
        '<fill><patternFill patternType="gray125"/></fill>'
        '<fill><patternFill patternType="solid"><fgColor rgb="FFC6EFCE"/><bgColor indexed="64"/></patternFill></fill>'
        '<fill><patternFill patternType="solid"><fgColor rgb="FFFFC7CE"/><bgColor indexed="64"/></patternFill></fill>'
        '</fills>'
        '<borders count="1"><border><left/><right/><top/><bottom/><diagonal/></border></borders>'
        '<cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>'
        '<cellXfs count="3">'
        '<xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>'
        '<xf numFmtId="0" fontId="0" fillId="2" borderId="0" xfId="0" applyFill="1"/>'
        '<xf numFmtId="0" fontId="0" fillId="3" borderId="0" xfId="0" applyFill="1"/>'
        '</cellXfs>'
        '<cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles>'
        '</styleSheet>'
    )


def _write_xlsx_report(xlsx_path: Path, sheets: List[Tuple[str, List[TestResult]]]) -> None:
    workbook_sheets = []
    rels = []
    content_overrides = [
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>',
        '<Default Extension="xml" ContentType="application/xml"/>',
        '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>',
        '<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>',
        '<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>',
    ]

    with zipfile.ZipFile(xlsx_path, "w", zipfile.ZIP_DEFLATED) as xlsx:
        for index, (sheet_name, results) in enumerate(sheets, start=1):
            safe_name = sheet_name[:31]
            workbook_sheets.append(
                f'<sheet name="{escape(safe_name)}" sheetId="{index}" r:id="rId{index}"/>'
            )
            rels.append(
                f'<Relationship Id="rId{index}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet{index}.xml"/>'
            )
            content_overrides.append(
                f'<Override PartName="/xl/worksheets/sheet{index}.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
            )
            xlsx.writestr(f"xl/worksheets/sheet{index}.xml", _xlsx_sheet(_xlsx_rows(results)))

        rels.append(
            f'<Relationship Id="rId{len(sheets) + 1}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>'
        )
        xlsx.writestr(
            "[Content_Types].xml",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
            f'{"".join(content_overrides)}'
            '</Types>',
        )
        xlsx.writestr(
            "_rels/.rels",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
            '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>'
            '<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>'
            '</Relationships>',
        )
        xlsx.writestr(
            "xl/workbook.xml",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
            'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
            f'<sheets>{"".join(workbook_sheets)}</sheets>'
            '</workbook>',
        )
        xlsx.writestr(
            "xl/_rels/workbook.xml.rels",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            f'{"".join(rels)}'
            '</Relationships>',
        )
        xlsx.writestr(
            "xl/styles.xml",
            _xlsx_styles(),
        )
        xlsx.writestr(
            "docProps/app.xml",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" '
            'xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">'
            '<Application>ArkSteed</Application></Properties>',
        )
        xlsx.writestr(
            "docProps/core.xml",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" '
            'xmlns:dc="http://purl.org/dc/elements/1.1/" '
            'xmlns:dcterms="http://purl.org/dc/terms/" '
            'xmlns:dcmitype="http://purl.org/dc/dcmitype/" '
            'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">'
            '<dc:creator>ArkSteed</dc:creator><dc:title>ArkSteed Test Report</dc:title>'
            '</cp:coreProperties>',
        )


def _find_test_cases_with_stage_fallback(
    only_subdirs: Optional[List[str]],
    all_files: bool = True,
    test_dir: Optional[Path] = None,
) -> List[TestCase]:
    """Find test cases from the unified test directory."""
    scan_dir = test_dir if test_dir is not None else TEST_DIR
    return find_test_cases(only_subdirs, all_files=all_files, test_dir=scan_dir)


def main() -> None:
    args = parse_args()

    mode = args.mode
    verbose = args.verbose
    keep_going = args.keep_going

    if args.clean:
        print("Cleaning test cases...")
        if not clean_test_cases():
            sys.exit(1)
        sys.exit(0)

    if not args.skip_build:
        if not build_ark(
            mode, verbose, skip_stub=args.skip_stub, keep_going=args.keep_going
        ):
            sys.exit(1)

    _, timestamp = _setup_logging()

    only_subdirs, filter_regex, filter_path_prefix, filter_dir_name = _parse_filter_string(args.filter)

    run_stages = ["jit"]

    print(f"Run mode: {', '.join(run_stages)}")

    if args.external_repo and not args.skip_external:
        fetch_external_tests(args.external_repo, args.external_dir)

    test_cases = _find_test_cases_with_stage_fallback(
        only_subdirs, all_files=True
    )

    test_cases = _filter_and_exclude_tests(
        filter_path_prefix, filter_dir_name, filter_regex, args.exclude, test_cases
    )

    if args.rerun_failed_from_latest_log:
        latest_log = _latest_log_file()
        if latest_log is None:
            print(
                f"Error: no log files found in {LOG_DIR}; cannot rerun failed cases",
                file=sys.stderr,
            )
            sys.exit(1)

        failed_case_names = _failed_cases_from_log(latest_log)
        print(f"Latest log: {latest_log}")
        if not failed_case_names:
            print("No failed cases found in the latest log; nothing to rerun.")
            sys.exit(0)

        test_cases = _filter_test_cases_by_case_names(test_cases, failed_case_names)
        if not test_cases:
            print(
                "No current test cases match the failed cases from the latest log.",
                file=sys.stderr,
            )
            sys.exit(1)

    if not test_cases:
        print("No test cases found", file=sys.stderr)
        sys.exit(1)
    print(f"Found {len(test_cases)} test cases")

    if args.summary_output_path is not None:
        summary_path = Path(args.summary_output_path)
        if summary_path.exists() and summary_path.stat().st_size > 0:
            print(f"Error: summary output file already exists and is non-empty: {summary_path}", file=sys.stderr)
            sys.exit(1)

    check_live_range_flag = args.check_live_range
    print_graph = args.print_graph
    if check_live_range_flag and not print_graph:
        print_graph = True
        print("Note: --check-live-range enabled, auto-enabling --print-graph")

    all_failed = 0
    report_files: List[Tuple[str, Path, Path]] = []
    all_results: List[TestResult] = []
    for index, run_stage in enumerate(run_stages):
        print(f"\n=== Running {run_stage} mode ===")
        passed, failed, results = _run_all_tests(
            test_cases,
            mode,
            verbose,
            keep_going,
            args.log_level,
            args.log_components,
            print_graph,
            check_live_range_flag,
            run_stage,
            args.num_workers,
            args.hotness_threshold,
        )
        all_failed += failed
        stage_log_file = _log_file_for_mode(timestamp, run_stage)
        _report_results(
            stage_log_file,
            timestamp,
            mode,
            test_cases,
            results,
            passed,
            failed,
            args.summary_output_path,
            f"{run_stage} mode",
            "w" if index == 0 else "a",
            verbose,
        )

        stage_xlsx_file = _xlsx_file_for_mode(timestamp, run_stage)
        _write_xlsx_report(stage_xlsx_file, [(run_stage, results)])
        report_files.append((run_stage, stage_log_file, stage_xlsx_file))
        all_results.extend(results)

    print("\n=== Report Links ===")
    for run_stage, log_file, xlsx_file in report_files:
        print(f"{run_stage} log: {log_file}")
        print(f"{run_stage} excel: {xlsx_file}")

    analysis_path: Optional[Path] = None
    app_preheat_analysis_path: Optional[Path] = None
    if args.codex_analysis:
        analysis_path = _run_codex_report_analysis(
            timestamp, mode, report_files, args.codex_analysis_timeout
        )
        app_preheat_analysis_path = _run_codex_app_preheat_analysis(
            timestamp, mode, report_files, all_results, args.codex_analysis_timeout
        )
    if analysis_path is not None:
        print(f"analysis: {analysis_path}")
    if app_preheat_analysis_path is not None:
        print(f"app_preheat_analysis: {app_preheat_analysis_path}")

    if all_failed > 0:
        print(f"Note: {all_failed} total failure(s).")
    sys.exit(1 if all_failed > 0 else 0)


if __name__ == "__main__":
    main()
