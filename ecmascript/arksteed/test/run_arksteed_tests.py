#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2026 Huawei Device Co., Ltd.
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

"""Test case runner for the ArkSteed JIT compiler.

Collects test cases by traversing the directory tree (each leaf directory
holds exactly one JS/TS source plus one expected_output.txt), compiles the
sources with es2abc, runs them under ark_js_vm with the compiler log
unconditionally enabled, and verifies:

  * stdout (with log lines filtered out) matches expected_output.txt;
  * the `//!` IR constraints (HAS, HAS_NOT, COUNT, COUNT_GE, COUNT_LE, etc.)
    hold on the CFG dumps parsed from the compiler log.

All generated artifacts (abc, disassembly, stdout/stderr captures) are
written to a fresh /tmp/arksteed-<mode>-<timestamp>/ directory whose tree
mirrors this test directory; nothing is written inside the source tree.

Exit status is 0 iff every selected case passes.
"""

import argparse
import collections
import difflib
from fnmatch import fnmatchcase
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import time
from abc import ABC, abstractmethod
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path
from typing import ClassVar, Dict, List, NamedTuple, Optional, Tuple

SCRIPT_DIR = Path(__file__).resolve().parent
TEST_DIR = SCRIPT_DIR
ARK_ROOT = SCRIPT_DIR.parents[4]
OPCODE_LIST_HEADER = SCRIPT_DIR.parent / "arksteed_opcode_list.h"

IS_MACOS = platform.system().lower() == "darwin"
LIB_PATH_ENV_VAR = "DYLD_LIBRARY_PATH" if IS_MACOS else "LD_LIBRARY_PATH"

BASE_ARGS = [
    "--compiler-enable-jit=true",
    "--compiler-enable-litecg=true",
    "--enable-force-gc=false",
    "--open-ark-tools=true",
]

ARKSTEED_GN_ARG = "ets_runtime_enable_ark_steed=true"
COMPILE_TIMEOUT = 30  # es2abc is always a host binary
DISASM_TIMEOUT = 30
DISASM_TIMEOUT_ARM64 = 30
EXECUTION_TIMEOUT = 300
QEMU_TIMEOUT_FACTOR = 10

SOURCE_SUFFIXES = (".ts", ".js")
EXPECTED_OUTPUT_NAME = "expected_output.txt"

CRASH_SIGNALS = {6: "SIGABRT", 11: "SIGSEGV"}
CRASH_RETURN_CODES = set(range(128, 128 + 32))

# A method may be JIT-compiled more than once per run (recompilation after
# deopt, OSR, ...); every compilation emits its own section starting with
# `======== ArkSteedCompilerTask: Starts compiling: <name> ========`
# (see ArkSteedCompilerTask::DebugLogOnCompilationStart). The name may be
# empty for anonymous functions. Counts for a method are aggregated over all
# of its compilations.
COMPILE_START_RE = re.compile(
    r"^\[compiler\] ======== ArkSteedCompilerTask: Starts compiling: ?(\S*) ========",
    re.MULTILINE,
)
GRAPH_DUMP_START = "[compiler] ===== Starts Graph Dump ====="
GRAPH_DUMP_END = "[compiler] ===== Finishes Graph Dump ====="


def iter_graph_dumps(stdout: str):
    """Yield (method_name, dump_text) for every CFG dump in the log.

    The dump delimiters (not the `Starts compiling` markers) delimit one
    graph: the attribution line can be missing for anonymous functions, and
    the per-graph vN numbering restarts with every dump - merging two dumps
    makes equal labels overwrite each other.
    """
    position = 0
    while True:
        start = stdout.find(GRAPH_DUMP_START, position)
        if start == -1:
            return
        end = stdout.find(GRAPH_DUMP_END, start)
        end = len(stdout) if end == -1 else end + len(GRAPH_DUMP_END)
        method = "(unknown)"
        for match in COMPILE_START_RE.finditer(stdout, 0, start):
            method = match.group(1) or "(anonymous)"
        yield method, stdout[start:end]
        position = end


# Vertex lines look like `<label>:  <Opcode> [...] (inputs) ; extras` where the
# label is `v<id>`, `n<label>` or `v?` (see Vertex::Dump). Vertices inside
# nested blocks are prefixed with box-drawing tree characters (`││  v43:`),
# so the label may be preceded by any run of non-word decoration characters.
VERTEX_LINE_RE = re.compile(r"^[\W_]*(v\?|v\d+|n\S+):\s+([A-Za-z_]\w*)")
ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;]*m")

ANSI_GREEN = "\033[32m"
ANSI_RED = "\033[31m"
ANSI_RESET = "\033[0m"

# `//!` annotations. IR constraints are only valid inside a METHOD scope;
# see the Constraint class hierarchy below for their grammar and semantics.
# Constraints on these opcodes must name the target stub, e.g.
# `//! HAS_NOT CallCommonStub GetPropertyByName` (wildcards allowed); other
# stubs of the same opcode are irrelevant to such a constraint.
STUB_CALL_OPCODES = ("CallCommonStub", "CallRuntime")

# Options owned by the runner (logging / dump infrastructure and environment
# paths). Setting them via `//! PARAMS` is a configuration error: overriding
# any of these could silently disable the compiler log, the CFG dumps and
# every check built on top of them.
RUNNER_MANAGED_OPTIONS = (
    "--log-level",
    "--log-components",
    "--compiler-arksteed-print-graph",
    "--compiler-arksteed-print-method-name",
    "--compiler-arksteed-print-code",
    "--compiler-arksteed-enable-code-comment",
    "--compiler-arksteed-print-with-colors",
    "--stub-file",
    "--icu-data-path",
)


# ---------------------------------------------------------------------------
# Build configuration and toolchain locations
# ---------------------------------------------------------------------------


HOST_MACHINE_TO_PLATFORM = {
    "x86_64": "x64",
    "amd64": "x64",
    "aarch64": "arm64",
    "arm64": "arm64",
}

HOST_PLATFORM = HOST_MACHINE_TO_PLATFORM.get(platform.machine().lower())

# Where prebuilts_download lands gn/ninja per host (see
# build/prebuilts_download/prebuilts_download_config.json); mirrors ark.py.
HOST_BUILD_TOOLS_DIR = {
    "x64": "linux-x86",
    "arm64": "linux-aarch64",
}


def detect_host_platform() -> str:
    if HOST_PLATFORM is None:
        print(f"Error: unsupported host machine '{platform.machine()}' "
              f"(supported: {', '.join(sorted(HOST_MACHINE_TO_PLATFORM))}). "
              "Pass -p/--platform explicitly.", file=sys.stderr)
        sys.exit(2)
    return HOST_PLATFORM


@dataclass(frozen=True)
class BuildConfig:
    mode: str
    platform: str
    host: Optional[str] = HOST_PLATFORM

    @property
    def _out_prefix(self) -> str:
        return f"out/{self.platform}.{self.mode}"

    def _out_path(self, *parts: str) -> Path:
        return ARK_ROOT.joinpath(self._out_prefix, *parts)

    @property
    def is_cross_build(self) -> bool:
        """True when the target runs on a different machine than the host."""
        return self.platform != self.host

    @property
    def host_toolchain_prefix(self) -> str:
        """GN host-toolchain output label of the es2abc build directory."""
        return "clang_arm64" if self.host == "arm64" else "clang_x64"

    @property
    def ninja(self) -> Path:
        if IS_MACOS:
            return ARK_ROOT / "prebuilts/build-tools/darwin-arm64/bin/ninja"
        dirname = HOST_BUILD_TOOLS_DIR.get(self.host or "x64")
        return ARK_ROOT / "prebuilts/build-tools" / dirname / "bin/ninja"

    @property
    def ark_js_vm(self) -> Path:
        return self._out_path("arkcompiler", "ets_runtime", "ark_js_vm")

    @property
    def ark_disasm(self) -> Path:
        return self._out_path("arkcompiler", "runtime_core", "ark_disasm")

    @property
    def es2abc(self) -> Path:
        if not self.is_cross_build:
            # The host-toolchain es2abc lands in the plain output directory.
            return self._out_path("arkcompiler", "ets_frontend", "es2abc")
        return self._out_path(self.host_toolchain_prefix,
                              "arkcompiler", "ets_frontend", "es2abc")

    @property
    def stub_file(self) -> Optional[Path]:
        # On Linux the path compiled into STUB_AN_FILE resolves inside the
        # build tree, so the flag is only needed on macOS.
        if not IS_MACOS:
            return None
        return self._out_path("gen", "arkcompiler", "ets_runtime", "stub.an")

    @property
    def icu_data_path(self) -> Optional[Path]:
        # localeCompare/Intl need the ICU data; without it the VM throws
        # "RangeError: invalid collation" on locale-sensitive builtins.
        path = ARK_ROOT / "third_party/icu/ohos_icu4j/data"
        return path if path.is_dir() else None

    @property
    def qemu_ld_prefix(self) -> Optional[Path]:
        if not self.is_cross_build or self.platform != "arm64":
            return None
        return self._out_path("common", "common", "libc")

    @property
    def command_prefix(self) -> List[str]:
        # Native execution when the target matches the host machine.
        if self.qemu_ld_prefix is None:
            return []
        qemu = shutil.which("qemu-aarch64-static") or shutil.which("qemu-aarch64")
        return [qemu, "-L", str(self.qemu_ld_prefix)]

    @property
    def lib_paths(self) -> List[Path]:
        paths = [
            self._out_path("arkcompiler", "ets_runtime"),
            self._out_path("thirdparty", "icu"),
            self._out_path("thirdparty", "zlib"),
            self._out_path("thirdparty", "bounds_checking_function"),
        ]
        if self.platform == "arm64":
            paths.extend(
                [
                    self._out_path("arkcompiler", "runtime_core"),
                    self._out_path("arkcompiler", "toolchain"),
                    self._out_path("thirdparty", "libuv"),
                ]
            )
        elif IS_MACOS:
            paths.extend(
                [
                    self._out_path("thirdparty", "libuv"),
                    ARK_ROOT / "prebuilts/clang/ohos/darwin-arm64/llvm/lib",
                ]
            )
        else:
            paths.extend(
                [
                    self._out_path("resourceschedule", "frame_aware_sched"),
                    self._out_path("hiviewdfx", "hilog"),
                    ARK_ROOT / "prebuilts/clang/ohos/linux-x86_64/llvm/lib",
                    self._out_path("hmosbundlemanager", "zlib_override"),
                ]
            )
        return paths


def build_ark(cfg: BuildConfig, skip_stub: bool) -> bool:
    """Run `python3 ark.py <platform>.<mode>` with the ArkSteed GN args."""

    target = f"{cfg.platform}.{cfg.mode}"
    print(f"Building ArkSteed ({target})...")

    cmd = ["python3", "ark.py", target]
    gn_args = [ARKSTEED_GN_ARG]
    if cfg.platform == "arm64" and cfg.is_cross_build:
        gn_args.append("run_with_qemu=true")
    if skip_stub:
        gn_args.append("skip_gen_stub=true")
    cmd.append(f"--gn-args={' '.join(gn_args)}")

    try:
        result = subprocess.run(cmd, cwd=ARK_ROOT)
        if result.returncode != 0:
            print(f"Build failed with return code {result.returncode}", file=sys.stderr)
            return False
        if cfg.is_cross_build:
            # ark.py only builds the target-toolchain es2abc; the host one
            # (used to compile the test sources) needs an explicit target.
            result = subprocess.run(
                [str(cfg.ninja), "-C", f"out/{target}",
                 f"{cfg.host_toolchain_prefix}/arkcompiler/ets_frontend/es2abc"],
                cwd=ARK_ROOT,
            )
            if result.returncode != 0:
                print("Host es2abc build failed", file=sys.stderr)
                return False
        print("Build successful")
        return True
    except Exception as e:  # noqa: BLE001 - report any build-system failure
        print(f"Build exception: {e}", file=sys.stderr)
        return False


def check_arksteed_gn_args(cfg: BuildConfig) -> bool:
    """Ensure the selected build directory was generated with ArkSteed enabled."""

    args_gn = ARK_ROOT / cfg._out_prefix / "args.gn"
    if not args_gn.exists():
        print(f"Error: args.gn does not exist: {args_gn}", file=sys.stderr)
        return False
    content = args_gn.read_text(encoding="utf-8")
    if re.search(r"^\s*ets_runtime_enable_ark_steed\s*=\s*true\s*$", content, re.MULTILINE):
        return True
    print(
        f"Error: ArkSteed GN option is not enabled in {args_gn}. "
        f"Please rebuild, or run:\n"
        f"  python3 ark.py {cfg.platform}.{cfg.mode} --gn-args={ARKSTEED_GN_ARG}",
        file=sys.stderr,
    )
    return False


# ---------------------------------------------------------------------------
# Test case discovery and `//!` annotation parsing
# ---------------------------------------------------------------------------


# Call vertices carry their target stub as a bare token among the extras
# (e.g. `COStub_GetPropertyByName`, `RTStub_NumberToString`); the COStub_ /
# RTStub_ prefixes are distinctive enough to anchor the match regardless of
# the extras separator style (semicolon- or space-separated).
STUB_ANNOTATION_RE = re.compile(r"(?:^|[\s;])((?:COStub_|RTStub_)[A-Za-z_0-9]+)")


class MethodCounts(NamedTuple):
    opcodes: collections.Counter               # opcode -> count
    stubs: collections.Counter                 # (opcode, stub name) -> count


class ConstraintError(Exception):
    """Raised when a `//!` constraint annotation cannot be parsed."""


@dataclass(frozen=True)
class Constraint(ABC):
    """Base class of the `//!` IR constraints.

    Each subclass owns the constraint's whole lifecycle - annotation grammar
    (from_tokens), matching against the CFG dumps (actual) and the verdict
    (violated, label) - so adding a constraint kind is one new class instead
    of edits scattered over the runner.
    """

    opcode: str
    # Stub-name pattern (wildcards allowed) for CallCommonStub / CallRuntime
    # constraints; None means "any node of the opcode".
    stub: Optional[str]
    KEYWORD: ClassVar[str]

    @staticmethod
    def _stub_matches(name: str, pattern: str) -> bool:
        """A stub pattern matches the dumped full name (`COStub_*` /
        `RTStub_*`) or its prefix-stripped form, so `GetPropertyByName`,
        `Get*ByName` and `COStub_GetPropertyByName` all work."""
        stripped = re.sub(r"^(?:COStub_|RTStub_)", "", name)
        return fnmatchcase(name, pattern) or fnmatchcase(stripped, pattern)

    @classmethod
    def takes_count(cls) -> bool:
        return False

    @classmethod
    def _grammar(cls) -> str:
        tail = "<opcode> [<stub-name>]"
        return f"{tail} <n>" if cls.takes_count() else tail

    @classmethod
    def from_tokens(cls, tokens: List[str], valid_opcodes: Optional[set]) -> "Constraint":
        """Parse `<opcode> [<stub-name>] [<n>]` tail tokens into a constraint,
        raising ConstraintError with a specific message on any mismatch."""
        keyword, counting = tokens[0], cls.takes_count()
        rest = tokens[1:]
        number: Optional[int] = None
        if counting:
            if not rest or not rest[-1].isdigit():
                raise ConstraintError(f"'{keyword}' expects {cls._grammar()}")
            number = int(rest[-1])
            rest = rest[:-1]
        if not rest:
            raise ConstraintError(f"'{keyword}' expects {cls._grammar()}")
        opcode, extra = rest[0], rest[1:]
        if valid_opcodes is not None and opcode not in valid_opcodes:
            raise ConstraintError(
                f"unknown IR opcode '{opcode}' (not in {OPCODE_LIST_HEADER.name})")
        stub: Optional[str] = None
        if opcode in STUB_CALL_OPCODES:
            if len(extra) != 1 or extra[0].isdigit():
                raise ConstraintError(
                    f"a constraint on {opcode} requires a stub name pattern "
                    "(e.g. 'GetPropertyByName', wildcards allowed)")
            stub = extra[0]
        elif extra:
            raise ConstraintError(
                f"'{opcode}' does not take a stub name; unexpected token '{extra[0]}'")
        fields: Dict[str, object] = {"opcode": opcode, "stub": stub}
        if counting:
            fields["count"] = number
        return cls(**fields)  # type: ignore[arg-type]

    def actual(self, counts: MethodCounts) -> int:
        """Number of matching nodes across the CFG dumps of one method."""
        if self.stub is None:
            return counts.opcodes.get(self.opcode, 0)
        return sum(number for (opcode, name), number in counts.stubs.items()
                   if opcode == self.opcode and self._stub_matches(name, self.stub))

    def label(self) -> str:
        """Human-readable form used in verdicts and verbose dumps."""
        tail = f" {self.SYMBOL} {self.count}" if self.takes_count() else ""
        return f"{self.KEYWORD} {self.opcode}" \
               + (f" {self.stub}" if self.stub else "") + tail

    def __str__(self) -> str:
        return self.label()

    @abstractmethod
    def violated(self, counts: MethodCounts) -> Optional[str]:
        """Error text when the dumps violate this constraint, else None."""


@dataclass(frozen=True)
class HasConstraint(Constraint):
    KEYWORD = "HAS"

    def violated(self, counts: MethodCounts) -> Optional[str]:
        actual = self.actual(counts)
        if actual >= 1:
            return None
        return f"\"{self.label()}\" violated, found {actual}"


@dataclass(frozen=True)
class HasNotConstraint(Constraint):
    KEYWORD = "HAS_NOT"

    def violated(self, counts: MethodCounts) -> Optional[str]:
        actual = self.actual(counts)
        if actual == 0:
            return None
        return f"\"{self.label()}\" violated, found {actual}"


@dataclass(frozen=True)
class CountingConstraint(Constraint):
    count: int
    SYMBOL: ClassVar[str]

    @classmethod
    def takes_count(cls) -> bool:
        return True

    @abstractmethod
    def _satisfied(self, actual: int) -> bool:
        ...

    def violated(self, counts: MethodCounts) -> Optional[str]:
        actual = self.actual(counts)
        if self._satisfied(actual):
            return None
        return f"\"{self.label()}\" violated, found {actual}"


@dataclass(frozen=True)
class CountConstraint(CountingConstraint):
    KEYWORD = "COUNT"
    SYMBOL = "=="

    def _satisfied(self, actual: int) -> bool:
        return actual == self.count


@dataclass(frozen=True)
class CountGeConstraint(CountingConstraint):
    KEYWORD = "COUNT_GE"
    SYMBOL = ">="

    def _satisfied(self, actual: int) -> bool:
        return actual >= self.count


@dataclass(frozen=True)
class CountLeConstraint(CountingConstraint):
    KEYWORD = "COUNT_LE"
    SYMBOL = "<="

    def _satisfied(self, actual: int) -> bool:
        return actual <= self.count


CONSTRAINT_BY_KEYWORD = {cls.KEYWORD: cls for cls in
                         (HasConstraint, HasNotConstraint, CountConstraint,
                          CountGeConstraint, CountLeConstraint)}


@dataclass
class TestCase:
    rel: str  # path relative to TEST_DIR, e.g. "example/test_1"
    source: Path
    expected: Path
    params: List[str] = field(default_factory=list)
    constraints: Dict[str, List[Constraint]] = field(default_factory=dict)
    config_errors: List[str] = field(default_factory=list)

    @property
    def entry(self) -> str:
        return self.source.stem


@dataclass
class CaseResult:
    rel: str
    reasons: List[str] = field(default_factory=list)
    duration: float = 0.0
    repro_cmd: Optional[str] = None
    # Per-case runner messages (verbose cmd line, disasm warnings); printed
    # together with the verdict so parallel runs stay grouped per case.
    log: List[str] = field(default_factory=list)

    @property
    def passed(self) -> bool:
        return not self.reasons


def _pattern_matches(rel: str, patterns: List[str]) -> bool:
    """Whether any pattern selects `rel`.

    A pattern matches when it matches the case path itself or any of its
    ancestor directory paths, with shell-style wildcards. For wildcard-free
    patterns this is exactly the previous directory-prefix semantics
    (`deopt` selects `deopt/...` but not a sibling `deopt_lazy_x`).
    """
    ancestors = rel.split("/")
    paths = ["/".join(ancestors[:i]) for i in range(len(ancestors), 0, -1)]
    for pattern in patterns:
        pattern = pattern.rstrip("/")
        if pattern and any(fnmatchcase(path, pattern) for path in paths):
            return True
    return False


def discover_cases(
    includes: List[str], excludes: List[str], verbose: bool = False
) -> List[TestCase]:
    """Walk the test tree and return one TestCase per conforming leaf directory."""

    cases: List[TestCase] = []
    if not TEST_DIR.exists():
        print(f"Test directory does not exist: {TEST_DIR}", file=sys.stderr)
        return cases

    for dirpath, dirnames, filenames in os.walk(TEST_DIR):
        dirnames[:] = sorted(d for d in dirnames if d not in ("__pycache__", "excluded"))
        sources = sorted(f for f in filenames if f.endswith(SOURCE_SUFFIXES))
        if not sources and EXPECTED_OUTPUT_NAME not in filenames:
            continue
        case_dir = Path(dirpath)
        rel = case_dir.relative_to(TEST_DIR).as_posix()
        if rel == ".":
            continue
        if len(sources) != 1:
            if verbose:
                print(f"warning: skipping {rel}: "
                      f"expected exactly one source file, found {len(sources)}",
                      file=sys.stderr)
            continue
        if EXPECTED_OUTPUT_NAME not in filenames:
            if verbose:
                print(f"warning: skipping {rel}: missing {EXPECTED_OUTPUT_NAME}", file=sys.stderr)
            continue
        if includes and not _pattern_matches(rel, includes):
            continue
        if excludes and _pattern_matches(rel, excludes):
            continue
        cases.append(TestCase(rel, case_dir / sources[0], case_dir / EXPECTED_OUTPUT_NAME))
    return cases


def load_opcode_names() -> Optional[set]:
    """Collect IR opcode names from arksteed_opcode_list.h (the authoritative list)."""
    try:
        text = OPCODE_LIST_HEADER.read_text(encoding="utf-8")
    except OSError as e:
        print(f"warning: cannot read {OPCODE_LIST_HEADER} ({e}); "
              "IR opcodes in annotations will not be validated", file=sys.stderr)
        return None
    return set(re.findall(r"\bV\((\w+)\)", text))


def parse_annotations(case: TestCase, valid_opcodes: Optional[set]) -> None:
    """Fill case.params / case.constraints; record problems in case.config_errors."""

    params: Optional[List[str]] = None
    current_method: Optional[str] = None
    try:
        lines = case.source.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as e:
        case.config_errors.append(f"cannot read source file: {e}")
        return

    for lineno, raw in enumerate(lines, 1):
        stripped = raw.strip()
        if not stripped.startswith("//!"):
            continue
        body = stripped[3:]
        comment_at = body.find("#")
        if comment_at != -1:
            body = body[:comment_at]
        tokens = body.split()
        if not tokens:
            continue

        def err(message: str) -> None:
            case.config_errors.append(f"line {lineno}: {message}")

        keyword = tokens[0]
        if keyword == "PARAMS":
            if params is not None:
                err("PARAMS may appear at most once")
            else:
                params = tokens[1:]
                # Log and dump infrastructure is runner-managed: overriding
                # any of it via PARAMS could silently disable the compiler
                # log, the CFG dumps and every check built on top of them.
                for name in _iter_option_names(params):
                    if name in RUNNER_MANAGED_OPTIONS:
                        err(f"'{name}' is runner-managed and "
                            "cannot be set via PARAMS")
        elif keyword == "METHOD":
            if len(tokens) != 2:
                err("METHOD expects exactly one function name")
            else:
                current_method = tokens[1]
                case.constraints.setdefault(current_method, [])
        elif keyword in CONSTRAINT_BY_KEYWORD:
            if current_method is None:
                err(f"'{keyword}' constraint appears before any METHOD annotation")
                continue
            try:
                case.constraints[current_method].append(
                    CONSTRAINT_BY_KEYWORD[keyword].from_tokens(tokens, valid_opcodes))
            except ConstraintError as error:
                err(str(error))
        else:
            err(f"unknown annotation keyword '{keyword}'")
    case.params = params if params is not None else []


# ---------------------------------------------------------------------------
# Compilation, disassembly and execution
# ---------------------------------------------------------------------------


def compile_case(case: TestCase, abc_path: Path, cfg: BuildConfig) -> Tuple[bool, str]:
    """Compile the source to abc with es2abc (always as an ES module)."""

    cmd = [
        str(cfg.es2abc),
        str(case.source),
        "--merge-abc",
        "--output",
        str(abc_path),
        "--module",
    ]
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=COMPILE_TIMEOUT,
        )
    except subprocess.TimeoutExpired:
        return False, "es2abc timed out"
    except OSError as e:
        return False, f"failed to run es2abc: {e}"
    if result.returncode != 0:
        tail = "\n".join((result.stderr or result.stdout or "").splitlines()[-15:])
        return False, f"es2abc exited with {result.returncode}:\n{tail}"
    return True, ""


def disassemble_case(case: TestCase, abc_path: Path, disasm_path: Path,
                     cfg: BuildConfig, log: List[str]) -> None:
    """Dump abc disassembly; diagnostic only - failures are reported as warnings."""

    if not cfg.ark_disasm.exists():
        log.append(f"warning: ark_disasm does not exist: {cfg.ark_disasm}")
        return
    cmd = cfg.command_prefix + [str(cfg.ark_disasm), str(abc_path), str(disasm_path)]
    timeout = DISASM_TIMEOUT_ARM64 if cfg.is_cross_build else DISASM_TIMEOUT
    try:
        result = subprocess.run(cmd, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        log.append(f"warning: ark_disasm timed out for {case.rel}")
        return
    except OSError as e:
        log.append(f"warning: ark_disasm failed to run for {case.rel}: {e}")
        return
    if result.returncode != 0:
        log.append(f"warning: ark_disasm failed for {case.rel} "
                   f"(return code {result.returncode})")


def build_repro_command(cmd: List[str], cfg: BuildConfig) -> str:
    """One copy-pastable shell line re-running the command under GDB."""

    parts = ["env", f"{LIB_PATH_ENV_VAR}={':'.join(str(p) for p in cfg.lib_paths)}"]
    if cfg.qemu_ld_prefix is not None:
        parts.append(f"QEMU_LD_PREFIX={cfg.qemu_ld_prefix}")
    parts.append("gdb")
    parts.append("--args")
    parts.extend(shlex.quote(str(token)) for token in cmd)
    return " ".join(parts)


def _option_name(token: str) -> Optional[str]:
    """Option name of a long-option token (`--opt=value` -> `--opt`), else None."""

    if token.startswith("--"):
        return token.split("=", 1)[0]
    return None


def _iter_option_names(params: List[str]):
    """Yield option names from `//! PARAMS` tokens, covering both `--opt=value`
    and the space-separated `--opt value` form."""
    index = 0
    while index < len(params):
        name = _option_name(params[index])
        if name is None:
            index += 1
            continue
        yield name
        index += 1
        if "=" not in params[index - 1] and index < len(params) \
                and not params[index].startswith("--"):
            index += 1  # skip the value of a space-separated option


def _collect_param_overrides(params: List[str]) -> set:
    """Option names set by `//! PARAMS`."""
    return set(_iter_option_names(params))


def execute_abc(
    case: TestCase, abc_path: Path, cfg: BuildConfig, args: argparse.Namespace,
    log: List[str],
) -> Tuple[List[str], Optional[int], str, str, bool]:
    """Run ark_js_vm on the abc; returns (cmd, returncode, stdout, stderr, timed_out)."""

    defaults = list(BASE_ARGS)
    if cfg.stub_file is not None:
        defaults.append(f"--stub-file={cfg.stub_file}")
    if cfg.icu_data_path is not None:
        defaults.append(f"--icu-data-path={cfg.icu_data_path}")
    defaults.append(f"--log-level={'debug' if args.log_debug else 'info'}")
    defaults.append(f"--log-components={args.log_components_value}")
    defaults.append("--compiler-arksteed-print-graph=true")
    defaults.append("--compiler-arksteed-print-method-name=false")
    if args.print_asm_code:
        defaults.append("--compiler-arksteed-print-code=true")
        defaults.append("--compiler-arksteed-enable-code-comment=true")
    if args.fully_colored:
        # ANSI escapes in the log are stripped before CFG parsing and log-line
        # filtering, so the verdicts are unaffected.
        defaults.append("--compiler-arksteed-print-with-colors=true")
    # A case parameter replaces (instead of merely overriding) the default it
    # names, so the command line never carries contradicting duplicates.
    overridden = _collect_param_overrides(case.params)
    defaults = [d for d in defaults if _option_name(d) not in overridden]

    cmd = cfg.command_prefix + [str(cfg.ark_js_vm)] + defaults + case.params
    cmd.extend([f"--entry-point={case.entry}", str(abc_path)])

    env = os.environ.copy()
    env[LIB_PATH_ENV_VAR] = ":".join(str(p) for p in cfg.lib_paths)
    if cfg.qemu_ld_prefix is not None:
        env["QEMU_LD_PREFIX"] = str(cfg.qemu_ld_prefix)
    if args.verbose:
        log.append(f"cmd: {' '.join(cmd)}")

    timeout = EXECUTION_TIMEOUT
    if cfg.is_cross_build:
        timeout *= QEMU_TIMEOUT_FACTOR
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=timeout, env=env,
        )
        return cmd, result.returncode, result.stdout or "", result.stderr or "", False
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode("utf-8", errors="replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
        return cmd, None, out, "", True
    except OSError as e:
        log.append(f"error: failed to run ark_js_vm: {e}")
        return cmd, None, "", "", False


# ---------------------------------------------------------------------------
# Verdicts: output comparison and IR constraint checking
# ---------------------------------------------------------------------------


def is_crash(returncode: Optional[int]) -> bool:
    if returncode is None:
        return False
    if returncode < 0:
        return abs(returncode) in CRASH_SIGNALS
    return returncode in CRASH_RETURN_CODES and (returncode - 128) in CRASH_SIGNALS


def crash_signal_name(returncode: Optional[int]) -> Optional[str]:
    if returncode is None:
        return None
    signal_number = abs(returncode) if returncode < 0 else returncode - 128
    return CRASH_SIGNALS.get(signal_number)


def compare_output(
    stdout: str, expected_file: Path, log_fragment_res: List[re.Pattern]
) -> Optional[str]:
    """Return None on match (or skipped comparison), otherwise a unified diff."""
    try:
        expected = expected_file.read_text(encoding="utf-8", errors="replace")
    except OSError as e:
        return f"cannot read {expected_file}: {e}"
    if not expected.strip():
        # Empty expectation means "nothing to check" (mirrors corpus convention).
        return None

    filtered = stdout
    for fragment_re in log_fragment_res:
        filtered = fragment_re.sub("", filtered)
    actual_lines = [line.rstrip() for line in filtered.splitlines()]
    expected_lines = [line.rstrip() for line in expected.splitlines()]
    if actual_lines == expected_lines:
        return None
    return "\n".join(
        difflib.unified_diff(
            expected_lines, actual_lines,
            fromfile="expected", tofile="actual", lineterm="",
        )
    )


def collect_opcode_counts(
    stdout: str, valid_opcodes: Optional[set]
) -> Dict[str, MethodCounts]:
    """Aggregate per-method IR opcode counts from the CFG dumps in the log."""
    counts: Dict[str, MethodCounts] = {}
    for method, dump in iter_graph_dumps(stdout):
        entry = counts.setdefault(
            method, MethodCounts(collections.Counter(), collections.Counter()))
        for line in dump.splitlines():
            line = ANSI_ESCAPE_RE.sub("", line)
            if line.startswith("[compiler] "):
                line = line[len("[compiler] "):]
            vertex = VERTEX_LINE_RE.match(line.strip())
            if vertex is None:
                continue
            opcode = vertex.group(2)
            if valid_opcodes is not None and opcode not in valid_opcodes:
                continue
            entry.opcodes[opcode] += 1
            stub = STUB_ANNOTATION_RE.search(line)
            if stub is not None:
                entry.stubs[(opcode, stub.group(1))] += 1
    return counts


def check_constraints(
    constraints: Dict[str, List[Constraint]],
    counts: Dict[str, MethodCounts],
) -> List[str]:
    """Verify all IR constraints against the aggregated opcode counts."""
    errors: List[str] = []
    for method, method_constraints in constraints.items():
        method_counts = counts.get(method)
        if method_counts is None:
            errors.append(
                f"method '{method}' was never JIT-compiled (no CFG dump in compiler log)"
            )
            continue
        for constraint in method_constraints:
            message = constraint.violated(method_counts)
            if message is not None:
                errors.append(f"{method}: {message}")
    return errors


# Live range checking (ported from the retired batch runner; the patterns are
# adapted to the current vertex dump format: `v<id>:` labels and the
# `→ live range: [start-end]` suffix).
LIVE_RANGE_RE = re.compile(r"(v\d+):\s+\S+.*?→\s*live range: \[(\d+)-(\d+)\]")
USE_LINE_RE = re.compile(r"(v\d+):.*?\(([^)]+)\)")
PHI_LINE_RE = re.compile(r"(v\d+):\s*Phi[^\(]*\(([^)]+)\)")
PHI_WORD_RE = re.compile(r"\bphi\b", re.IGNORECASE)
INPUT_REF_RE = re.compile(r"\bv\d+\b")


def _check_live_range_section(section: str) -> List[str]:
    """Check live range correctness within one compiler graph dump.

    1. Every value used as a Phi input must have a live range;
    2. For every use, the used value's live range must extend to (at least)
       the start of the using vertex - a value dying before its use is a bug.
    """
    # The dump may carry ANSI colors (-C); strip them before any matching,
    # otherwise the escape sequences break up the patterns mid-token.
    section = ANSI_ESCAPE_RE.sub("", section)
    live_ranges: Dict[str, Tuple[int, int]] = {}
    uses: Dict[str, List[str]] = {}
    phi_inputs: set = set()

    for match in LIVE_RANGE_RE.finditer(section):
        live_ranges[match.group(1)] = (int(match.group(2)), int(match.group(3)))

    for match in USE_LINE_RE.finditer(section):
        if PHI_WORD_RE.search(match.group(0)):
            continue  # Phi inputs are handled separately below.
        uses.setdefault(match.group(1), []).extend(INPUT_REF_RE.findall(match.group(2)))

    for match in PHI_LINE_RE.finditer(section):
        # Only an allocated Phi imposes requirements on its inputs; the
        # compiler deliberately leaves some Phis unallocated ("Skips
        # unallocated Phi" in the log), and so may their inputs be.
        if match.group(1) in live_ranges:
            phi_inputs.update(INPUT_REF_RE.findall(match.group(2)))

    errors: List[str] = []
    if not live_ranges and "live range:" in section:
        errors.append("live range info present but unparsable (log format drift?)")
        return errors

    for var in sorted(phi_inputs):
        if var not in live_ranges:
            errors.append(f"{var} is used as a Phi input but has no live range")

    for use_var, args in uses.items():
        if use_var not in live_ranges:
            continue
        use_start, _ = live_ranges[use_var]
        for arg in args:
            if arg not in live_ranges:
                continue
            arg_start, arg_end = live_ranges[arg]
            if arg_end < use_start:
                errors.append(
                    f"{arg} [{arg_start}-{arg_end}] dies before its use "
                    f"{use_var} [starting at {use_start}]"
                )
    return errors


def check_live_ranges(stdout: str) -> List[str]:
    """Check live ranges of every CFG dump; vN numbering restarts per graph,
    so each dump is checked independently."""
    errors: List[str] = []
    for method, dump in iter_graph_dumps(stdout):
        errors.extend(f"{method}: {e}" for e in _check_live_range_section(dump))
    return errors


# ---------------------------------------------------------------------------
# Per-case pipeline and main
# ---------------------------------------------------------------------------


def run_case(
    case: TestCase,
    cfg: BuildConfig,
    args: argparse.Namespace,
    artifacts_root: Path,
    log_fragment_res: List[re.Pattern],
    valid_opcodes: Optional[set],
) -> CaseResult:
    result = CaseResult(case.rel)
    start = time.time()
    art_dir = artifacts_root / case.rel
    art_dir.mkdir(parents=True, exist_ok=True)

    if case.config_errors:
        result.reasons.extend(case.config_errors)
        result.duration = time.time() - start
        return result

    abc_path = art_dir / f"{case.entry}.abc"
    ok, message = compile_case(case, abc_path, cfg)
    if not ok:
        result.reasons.append(f"compilation failed: {message}")
        result.duration = time.time() - start
        return result

    disassemble_case(case, abc_path, art_dir / f"{case.entry}.disasm.txt", cfg, result.log)

    cmd, returncode, stdout, stderr, timed_out = execute_abc(
        case, abc_path, cfg, args, result.log)
    result.repro_cmd = build_repro_command(cmd, cfg)
    (art_dir / f"{case.entry}.stdout.txt").write_text(stdout, encoding="utf-8")
    (art_dir / f"{case.entry}.stderr.txt").write_text(stderr, encoding="utf-8")

    if timed_out:
        result.reasons.append("execution timed out")
    elif returncode is None:
        result.reasons.append("ark_js_vm could not be executed")
    else:
        if is_crash(returncode):
            result.reasons.append(
                f"ark_js_vm crashed with {crash_signal_name(returncode)} "
                f"(return code {returncode})"
            )
        elif returncode != 0:
            result.reasons.append(f"ark_js_vm exited with return code {returncode}")

    diff = compare_output(stdout, case.expected, log_fragment_res)
    if diff is not None:
        result.reasons.append(f"stdout mismatch:\n{diff}")

    constraint_errors = check_constraints(
        case.constraints, collect_opcode_counts(stdout, valid_opcodes)
    )
    if args.verbose and case.constraints and not constraint_errors:
        detail: List[str] = []
        for method, constraints in case.constraints.items():
            detail.append(f"METHOD {method}:")
            detail.extend(f"  {constraint}" for constraint in constraints)
        result.log.append("All IR constraints are satisfied:\n" + "\n".join(detail))
    result.reasons.extend(constraint_errors)
    result.reasons.extend(check_live_ranges(stdout))
    result.duration = time.time() - start
    return result


def make_artifacts_root(platform: str, mode: str) -> Path:
    stamp = time.strftime("%Y%m%d-%H%M%S")
    root = Path("/tmp") / f"arksteed-{platform}-{mode}-{stamp}"
    suffix = 1
    while root.exists():
        root = Path("/tmp") / f"arksteed-{platform}-{mode}-{stamp}-{suffix}"
        suffix += 1
    root.mkdir(parents=True)
    return root


MAX_LISTED_FAILED_CASES = 20


def format_failed_case_lines(failed: List[CaseResult]) -> List[str]:
    """Terminal listing of failed cases, capped with an overflow hint."""

    lines = [f"  {r.rel}" for r in failed[:MAX_LISTED_FAILED_CASES]]
    hidden = len(failed) - MAX_LISTED_FAILED_CASES
    if hidden > 0:
        lines.append(f"  ... and {hidden} more (all listed in summary.md)")
    return lines


def write_summary_md(
    path: Path,
    results: List[CaseResult],
    total_cases: int,
    cfg: BuildConfig,
    elapsed: float,
    wall: float,
    stopped_early: bool,
) -> None:
    """Write a markdown summary listing every failed case with its reasons."""
    failed = [r for r in results if not r.passed]
    lines = [
        "# ArkSteed Test Run Summary",
        "",
        f"- Date: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        f"- Platform: `{cfg.platform}`  Mode: `{cfg.mode}`",
        f"- Total: {len(results)}  Passed: {len(results) - len(failed)}  "
        f"Failed: {len(failed)}",
        f"- Elapsed: {elapsed:.1f}s  Wall: {wall:.1f}s",
    ]
    if stopped_early:
        lines.append(f"- Stopped on first failure; {total_cases - len(results)} case(s) not run")
    lines.append("")
    if not failed:
        lines.append("No failed case.")
    else:
        lines.append(f"## Failed Cases ({len(failed)})")
        lines.append("")
        for result in failed:
            lines.append(f"### `{result.rel}` ({result.duration:.1f}s)")
            lines.append("")
            for reason in result.reasons:
                lines.append("```")
                lines.append(reason)
                lines.append("```")
                lines.append("")
            if result.repro_cmd is not None:
                lines.append("Reproduce under GDB (single copy-paste):")
                lines.append("")
                lines.append("```bash")
                lines.append(result.repro_cmd)
                lines.append("```")
                lines.append("")
    try:
        path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    except OSError as e:
        print(f"warning: failed to write {path}: {e}", file=sys.stderr)


def write_run_meta(path: Path, args: argparse.Namespace, cfg: BuildConfig) -> None:
    try:
        git_head = subprocess.run(
            ["git", "-C", str(SCRIPT_DIR), "rev-parse", "HEAD"],
            capture_output=True, text=True,
        ).stdout.strip()
    except OSError:
        git_head = None
    meta = {
        "argv": sys.argv,
        "platform": cfg.platform,
        "mode": cfg.mode,
        "git_head": git_head or None,
        "ark_js_vm": str(cfg.ark_js_vm),
        "es2abc": str(cfg.es2abc),
        "ark_disasm": str(cfg.ark_disasm),
    }
    path.write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Test case runner for the ArkSteed JIT compiler."
    )
    parser.add_argument(
        "-p", "--platform", choices=("x64", "arm64"), default=None,
        help="Target platform; arm64 runs under QEMU (default: host platform)."
    )
    parser.add_argument(
        "-m", "--mode", choices=("debug", "release"), default="debug",
        help="Build mode to use (default: debug)."
    )
    parser.add_argument("-f", "--skip-stub", action="store_true",
                        help="Skip building stub.an (safe only if stub IR is unchanged).")
    parser.add_argument("-F", "--skip-build", action="store_true",
                        help="Skip building all the targets.")
    parser.add_argument("-I", "--include", "--filter", dest="include", action="append",
                        default=[], metavar="DIR",
                        help="Keep only the test cases under the given directory prefix; "
                             "repeatable.")
    parser.add_argument("-X", "--exclude", action="append", default=[], metavar="DIR",
                        help="Exclude the test cases under the given directory prefix; "
                             "repeatable.")
    parser.add_argument("-d", "--log-debug", action="store_true",
                        help="Set the runtime log level to debug.")
    parser.add_argument("-l", "--log-components", default="", metavar="COMPONENTS",
                        help="Log components besides the always-enabled compiler log, "
                             "e.g. -l gc.")
    parser.add_argument("--print-asm-code", action="store_true",
                        help="Dump generated ASM code (with comments) to the compiler log.")
    parser.add_argument("-c", "--colored", action="store_true",
                        help="Enable colored text on the terminal.")
    parser.add_argument("-C", "--fully-colored", action="store_true",
                        help="Enable colored text both on the terminal and in the "
                             "compiler log.")
    parser.add_argument("-s", "--stops-on-error", action="store_true",
                        help="Stop the runner as soon as any test case fails.")
    parser.add_argument("-j", "--num-workers", type=int, default=1, metavar="N",
                        help="Number of parallel workers (default: 1, serial).")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Show verbose information.")
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)
    if args.num_workers < 1:
        print("Error: --num-workers expects a positive integer", file=sys.stderr)
        return 2
    colored = args.colored or args.fully_colored
    extras = [c.strip() for c in args.log_components.split(",") if c.strip()]
    # The compiler log is unconditionally enabled and cannot be removed via
    # -l; the -l items are added on top (duplicates dropped).
    components = ["compiler"] + [c for c in dict.fromkeys(extras) if c != "compiler"]
    # LogComponent lines are prefixed with "[<component>]"; filter exactly the
    # enabled ones so that program output is never mistaken for a log line.
    # The VM parses --log-components as a COLON-separated list - a comma in
    # the value becomes one unknown component name and silences ALL logs.
    args.log_components_value = ":".join(components)
    # Each enabled component logs lines prefixed with "[<component>] "; those
    # fragments are cut out of the raw stdout before comparison (wherever they
    # occur - see compare_output).
    log_fragment_res = [re.compile(r"\[%s\] [^\n]*\n?" % re.escape(c)) for c in components]

    # Resolve lazily so an explicit -p works even on an unsupported machine.
    if args.platform is None:
        args.platform = detect_host_platform()
    cfg = BuildConfig(args.mode, args.platform)
    if cfg.is_cross_build and cfg.platform == "arm64" \
            and not shutil.which("qemu-aarch64-static") \
            and not shutil.which("qemu-aarch64"):
        print("Error: qemu-aarch64 (or qemu-aarch64-static) is required for cross-platform "
              "ARM64 testing", file=sys.stderr)
        return 2

    if not args.skip_build:
        if not build_ark(cfg, args.skip_stub):
            return 2
    if not check_arksteed_gn_args(cfg):
        return 2
    for binary in (cfg.ark_js_vm, cfg.es2abc):
        if not binary.exists():
            print(f"Error: {binary} does not exist", file=sys.stderr)
            return 2

    valid_opcodes = load_opcode_names()
    cases = discover_cases(args.include, args.exclude, args.verbose)
    if not cases:
        print("No test case selected", file=sys.stderr)
        return 2
    for case in cases:
        parse_annotations(case, valid_opcodes)
    print(f"Collected {len(cases)} test case(s) for {cfg.platform}.{cfg.mode}\n")

    artifacts_root = make_artifacts_root(cfg.platform, args.mode)
    write_run_meta(artifacts_root / "run.meta.json", args, cfg)

    results: List[CaseResult] = []
    stopped_early = False

    def report(result: CaseResult) -> None:
        badge = "[PASS]" if result.passed else "[FAIL]"
        if colored:
            badge = (ANSI_GREEN if result.passed else ANSI_RED) + badge + ANSI_RESET
        print(f"[{len(results):>4}/{len(cases)}] {badge} {result.rel}")
        for message in result.log:
            print("         " + message.replace("\n", "\n         "))
        for reason in result.reasons:
            print("         " + reason.replace("\n", "\n         "))

    def run_one(case: TestCase) -> CaseResult:
        return run_case(case, cfg, args, artifacts_root, log_fragment_res, valid_opcodes)

    wall_start = time.time()
    if args.num_workers > 1:
        with ThreadPoolExecutor(max_workers=args.num_workers) as pool:
            futures = {pool.submit(run_one, case): case for case in cases}
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
                report(result)
                if args.stops_on_error and not result.passed:
                    stopped_early = True
                    for pending in futures:
                        pending.cancel()
                    break
    else:
        for case in cases:
            result = run_one(case)
            results.append(result)
            report(result)
            if args.stops_on_error and not result.passed:
                stopped_early = True
                break

    failed = [r for r in results if not r.passed]
    elapsed = sum(r.duration for r in results)
    wall = time.time() - wall_start
    write_summary_md(
        artifacts_root / "summary.md", results, len(cases), cfg,
        elapsed, wall, stopped_early,
    )
    if stopped_early:
        print(f"\nStopped on first failure; {len(cases) - len(results)} case(s) not run")
    failed_label = f"Failed: {len(failed)}"
    if colored and failed:
        failed_label = ANSI_RED + failed_label + ANSI_RESET
    print(f"\nTotal: {len(results)}  Passed: {len(results) - len(failed)}  "
          f"{failed_label}  Elapsed: {elapsed:.1f}s  Wall: {wall:.1f}s")
    if failed:
        print("\nFailed cases:")
        for line in format_failed_case_lines(failed):
            print(line)
    print(f"\nArtifacts: {artifacts_root}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
