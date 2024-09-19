#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Copyright (c) 2023-2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: run regress test case
"""
import argparse
import dataclasses
import datetime
import json
import logging
import multiprocessing
import os
import platform
import re
import shutil
import stat
import subprocess
import sys
from abc import ABC
from typing import Optional, List, Type, Dict, Set, Tuple, Callable
from os.path import dirname, join
from pathlib import Path
import xml.etree.cElementTree as XTree
from enum import Enum, auto

from regress_test_config import RegressTestConfig

ENV_PATTERN = re.compile(r"//\s+Environment Variables:(.*)")


def init_log_file(args):
    logging.basicConfig(filename=args.out_log, format=RegressTestConfig.DEFAULT_LOG_FORMAT, level=logging.INFO)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--test-dir', metavar='DIR',
                        help='Directory to test ')
    parser.add_argument('--test-file', metavar='FILE',
                        help='File to test')
    parser.add_argument('--test-list', metavar='FILE', dest="test_list", default=None,
                        help='File with list of tests to run')
    parser.add_argument('--ignore-list', metavar='FILE', dest="ignore_list", default=None,
                        help='File with known failed tests list')
    parser.add_argument('--timeout', default=RegressTestConfig.DEFAULT_TIMEOUT, type=int,
                        help='Set a custom test timeout in seconds !!!\n')
    parser.add_argument('--processes', default=RegressTestConfig.DEFAULT_PROCESSES, type=int,
                        help='set number of processes to use. Default value: 1\n')
    parser.add_argument('--merge-abc-binary',
                        help="merge-abc's binary tool")
    parser.add_argument('--ark-tool',
                        help="ark's binary tool")
    parser.add_argument('--ark-aot-tool',
                        help="ark_aot's binary tool")
    parser.add_argument('--ark-aot', default=False, action='store_true',
                        help="runs in ark-aot mode")
    parser.add_argument('--run-pgo', default=False, action='store_true',
                        help="runs in pgo mode")
    parser.add_argument('--enable-litecg', default=False, action='store_true',
                        help="runs in litecg mode")
    parser.add_argument('--ark-frontend-binary',
                        help="ark frontend conversion binary tool")
    parser.add_argument('--stub-path',
                        help="stub file for run in AOT modes")
    parser.add_argument('--LD_LIBRARY_PATH', '--libs-dir',
                        dest='ld_library_path', default=None, help='LD_LIBRARY_PATH')
    parser.add_argument('--icu-path',
                        dest='icu_path', help='icu-data-path')
    parser.add_argument('--out-dir',
                        default=None, help='target out dir')
    parser.add_argument('--force-clone', action="store_true",
                        default=False, help='Force to clone tests folder')
    parser.add_argument('--ark-arch',
                        default=RegressTestConfig.DEFAULT_ARK_ARCH,
                        required=False,
                        nargs='?', choices=RegressTestConfig.ARK_ARCH_LIST, type=str)
    parser.add_argument('--ark-arch-root',
                        default=RegressTestConfig.DEFAULT_ARK_ARCH,
                        required=False,
                        help="the root path for qemu-aarch64 or qemu-arm")
    parser.add_argument('--disable-force-gc', action='store_true',
                        help="Run regress tests with close force-gc")
    return parser.parse_args()


def check_ark_frontend_binary(args) -> bool:
    if args.ark_frontend_binary is None:
        output('ark_frontend_binary is required, please add this parameter')
        return False
    return True


def check_frontend_library(args) -> bool:
    current_dir = str(os.getcwd())
    current_frontend_binary = os.path.join(current_dir, str(args.ark_frontend_binary))
    test_tool_frontend_binary = os.path.join(RegressTestConfig.TEST_TOOL_FILE_DIR, args.ark_frontend_binary)
    if not os.path.exists(current_frontend_binary) and not os.path.exists(test_tool_frontend_binary):
        output('entered ark_frontend_binary does not exist. please confirm')
        return False
    args.ark_frontend_binary = current_frontend_binary if os.path.exists(
        current_frontend_binary) else test_tool_frontend_binary
    args.ark_frontend_binary = os.path.abspath(args.ark_frontend_binary)
    return True


def check_ark_tool(args) -> bool:
    current_dir = str(os.getcwd())
    if args.ark_tool is None:
        output('ark_tool is required, please add this parameter')
        return False

    current_ark_tool = os.path.join(current_dir, str(args.ark_tool))
    test_tool_ark_tool = os.path.join(RegressTestConfig.TEST_TOOL_FILE_DIR, args.ark_tool)
    if not os.path.exists(current_ark_tool) and not os.path.exists(test_tool_ark_tool):
        output('entered ark_tool does not exist. please confirm')
        return False

    args.ark_tool = current_ark_tool if os.path.exists(current_ark_tool) else test_tool_ark_tool
    args.ark_tool = os.path.abspath(args.ark_tool)
    return True


def check_ark_aot(args) -> bool:
    if args.ark_aot:
        current_dir = str(os.getcwd())
        current_ark_aot_tool = os.path.join(current_dir, str(args.ark_aot_tool))
        test_tool_ark_aot_tool = os.path.join(RegressTestConfig.TEST_TOOL_FILE_DIR, args.ark_aot_tool)
        if not os.path.exists(current_ark_aot_tool) and not os.path.exists(test_tool_ark_aot_tool):
            output(f'entered ark_aot_tool "{args.ark_aot_tool}" does not exist. Please check')
            return False
        args.ark_aot_tool = current_ark_aot_tool if os.path.exists(current_ark_aot_tool) else test_tool_ark_aot_tool
        args.ark_aot_tool = os.path.abspath(args.ark_aot_tool)
        return True
    if args.run_pgo and not args.ark_aot:
        output('pgo mode cannot be used without aot')
        return False
    return True


def check_stub_path(args) -> bool:
    if args.stub_path:
        current_dir = str(os.getcwd())
        stub_path = os.path.join(current_dir, str(args.stub_path))
        if not os.path.exists(stub_path):
            output(f'entered stub-file "{args.stub_path}" does not exist. Please check')
            return False
        args.stub_path = os.path.abspath(args.stub_path)
    return True


def is_ignore_file_present(ignore_path: str) -> bool:
    if os.path.exists(ignore_path):
        return True
    output(f"Cannot find ignore list '{ignore_path}'")
    return False


def check_ignore_list(args) -> bool:
    if args.ignore_list:
        if os.path.isabs(args.ignore_list):
            return is_ignore_file_present(args.ignore_list)
        args.ignore_list = str(os.path.join(RegressTestConfig.TEST_TOOL_FILE_DIR, args.ignore_list))
        return is_ignore_file_present(args.ignore_list)
    return True


def check_args(args):
    result = check_ark_frontend_binary(args)
    result = result and check_frontend_library(args)
    result = result and check_ark_tool(args)
    result = result and check_ark_aot(args)
    result = result and check_stub_path(args)
    result = result and check_ignore_list(args)

    if not result:
        return False

    if args.ld_library_path is not None:
        libs = args.ld_library_path.split(":")
        current_dir = str(os.getcwd())
        libs = [os.path.abspath(os.path.join(current_dir, str(lib))) for lib in libs]
        args.ld_library_path = ":".join(libs)
    else:
        args.ld_library_path = RegressTestConfig.DEFAULT_LIBS_DIR
    if args.icu_path is None:
        args.icu_path = RegressTestConfig.ICU_PATH
    if args.out_dir is None:
        args.out_dir = RegressTestConfig.PROJECT_BASE_OUT_DIR
    else:
        args.out_dir = os.path.abspath(os.path.join(RegressTestConfig.CURRENT_PATH, args.out_dir))
    if not args.out_dir.endswith("/"):
        args.out_dir = f"{args.out_dir}/"
    args.regress_out_dir = os.path.join(args.out_dir, "regresstest")
    args.out_result = os.path.join(args.regress_out_dir, 'result.txt')
    args.junit_report = os.path.join(args.regress_out_dir, 'report.xml')
    args.out_log = os.path.join(args.regress_out_dir, 'test.log')
    args.test_case_out_dir = os.path.join(args.regress_out_dir, RegressTestConfig.REGRESS_GIT_REPO)
    return True


def remove_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)


def output(msg):
    print(str(msg))
    logging.info(str(msg))


def output_debug(msg):
    logging.debug(str(msg))


def get_extra_error_message(ret_code: int) -> str:
    error_messages = {
        0: '',
        -6: 'Aborted (core dumped)',
        -4: 'Aborted (core dumped)',
        -11: 'Segmentation fault (core dumped)',
        255: '(uncaught error)'
    }
    error_message = error_messages.get(ret_code, f'Unknown Error: {str(ret_code)}')
    return error_message


@dataclasses.dataclass
class StepResult:
    step_name: str  # a copy of the step name
    is_passed: bool = False  # True if passed, any other state is False
    command: List[str] = dataclasses.field(default_factory=list)  # command to run
    return_code: int = -1
    stdout: Optional[str] = None  # present only if there is some output
    stderr: Optional[str] = None  # can be present only if is_passed == False
    fileinfo: Optional[str] = None  # content of fileinfo file if present

    def report(self) -> str:
        stdout = self.stdout if self.stdout else ''
        stderr = self.stderr if self.stderr else ''
        cmd = " ".join([str(cmd) for cmd in self.command])
        result: List[str] = [
            f"{self.step_name}:",
            f"\tCommand: {cmd}",
            f"\treturn code={self.return_code}",
            f"\toutput='{stdout}'",
            f"\terrors='{stderr}'"]
        if self.fileinfo:
            result.append(f"\tFileInfo:\n{self.fileinfo}")
        return "\n".join(result)


@dataclasses.dataclass
class TestReport:
    src_path: str  # full path to the source test
    test_id: str = ""  # path starting from regresstest
    out_path: str = ""  # full path to intermediate files up to folder
    passed: bool = False  # False if the test has not started or failed
    is_skipped: bool = False  # True if the test has found in the skipped (excluded) list
    is_ignored: bool = False  # True if the test has found in the ignored list
    steps: List[StepResult] = dataclasses.field(default_factory=list)  # list of results

    def report(self) -> str:
        result: List[str] = [f"{self.test_id}:"]
        if self.steps is None:
            return ""
        for step in self.steps:
            result.append(f"\t{step.report()}")
        return "\n".join(result)


class RegressTestStep(ABC):
    step_obj: Optional['RegressTestStep'] = None

    def __init__(self, args, name):
        output(f"--- Start step {name} ---")
        self.args = args
        self.__start: Optional[datetime.datetime] = None
        self.__end: Optional[datetime.datetime] = None
        self.__duration: Optional[datetime.timedelta] = None
        self.name: str = name

    @staticmethod
    def run(args, test_reports: Optional[List[TestReport]] = None) -> List[TestReport]:
        pass

    def get_duration(self) -> datetime.timedelta:
        if self.__duration is None:
            output(f"Step {self.name} not started or not completed")
            sys.exit(1)
        return self.__duration

    def _start(self):
        self.__start = datetime.datetime.now()

    def _end(self):
        self.__end = datetime.datetime.now()
        self.__duration = self.__end - self.__start


class RegressTestRepoPrepare(RegressTestStep):
    def __init__(self, args):
        RegressTestStep.__init__(self, args, "Repo preparation")
        self.test_list: List[str] = self.read_test_list(args.test_list)

    @staticmethod
    def read_test_list(test_list_name: Optional[str]) -> List[str]:
        if test_list_name is None:
            return []
        filename = join(dirname(__file__), test_list_name)
        if not Path(filename).exists():
            output(f"File {filename} set as --test-list value cannot be found")
            exit(1)
        with open(filename, 'r') as stream:
            return stream.read().split("\n")

    @staticmethod
    def run(args, test_reports: Optional[List[TestReport]] = None) -> List[TestReport]:
        repo = RegressTestRepoPrepare(args)
        RegressTestRepoPrepare.step_obj = repo
        repo._start()

        repo.run_regress_test_prepare()
        repo.prepare_clean_data()
        repo.get_test_case()
        test_list = repo.get_regress_test_files()
        skip_list = repo.get_skip_test_cases()
        if test_reports is None:
            test_reports = []
        for test in test_list:
            shorten = Utils.get_inside_path(test)
            test_id = f"regresstest/ark-regress/{shorten}"
            if shorten not in skip_list:
                report = TestReport(src_path=test, test_id=test_id)
                test_reports.append(report)

        repo._end()
        return test_reports

    @staticmethod
    def git_checkout(checkout_options, check_out_dir=os.getcwd()):
        cmds = ['git', 'checkout', checkout_options]
        result = True
        with subprocess.Popen(cmds, cwd=check_out_dir) as proc:
            ret = proc.wait()
            if ret:
                output(f"\n error: git checkout '{checkout_options}' failed.")
                result = False
        return result

    @staticmethod
    def git_pull(check_out_dir=os.getcwd()):
        cmds = ['git', 'pull', '--rebase']
        with subprocess.Popen(cmds, cwd=check_out_dir) as proc:
            proc.wait()

    @staticmethod
    def git_clean(clean_dir=os.getcwd()):
        cmds = ['git', 'checkout', '--', '.']
        with subprocess.Popen(cmds, cwd=clean_dir) as proc:
            proc.wait()

    @staticmethod
    def git_clone(git_url, code_dir):
        cmds = ['git', 'clone', git_url, code_dir]
        retries = RegressTestConfig.DEFAULT_RETRIES
        while retries > 0:
            with subprocess.Popen(cmds) as proc:
                ret = proc.wait()
                if ret:
                    output(f"\n Error: Cloning '{git_url}' failed. Retry remaining '{retries}' times")
                    retries -= 1
                else:
                    return True
        sys.exit(1)

    @staticmethod
    def get_skip_test_cases() -> List[str]:
        return Utils.read_skip_list(RegressTestConfig.SKIP_LIST_FILE)

    def get_test_case(self):
        if not os.path.isdir(os.path.join(RegressTestConfig.REGRESS_TEST_CASE_DIR, '.git')):
            self.git_clone(RegressTestConfig.REGRESS_GIT_URL, RegressTestConfig.REGRESS_TEST_CASE_DIR)
            return self.git_checkout(RegressTestConfig.REGRESS_GIT_HASH, RegressTestConfig.REGRESS_TEST_CASE_DIR)
        return True

    def prepare_clean_data(self):
        self.git_clean(RegressTestConfig.REGRESS_TEST_CASE_DIR)
        self.git_pull(RegressTestConfig.REGRESS_TEST_CASE_DIR)
        self.git_checkout(RegressTestConfig.REGRESS_GIT_HASH, RegressTestConfig.REGRESS_TEST_CASE_DIR)

    def run_regress_test_prepare(self):
        if self.args.force_clone:
            remove_dir(self.args.regress_out_dir)
            remove_dir(RegressTestConfig.REGRESS_TEST_CASE_DIR)
        os.makedirs(self.args.regress_out_dir, exist_ok=True)
        os.makedirs(RegressTestConfig.REGRESS_TEST_CASE_DIR, exist_ok=True)
        init_log_file(self.args)

    def get_regress_test_files(self) -> List[str]:
        result: List[str] = []
        if self.args.test_file is not None and len(self.args.test_file) > 0:
            test_file_list = os.path.join(RegressTestConfig.REGRESS_TEST_CASE_DIR, self.args.test_file)
            result.append(str(test_file_list))
            return result
        elif self.args.test_dir is not None and len(self.args.test_dir) > 0:
            test_file_list = os.path.join(RegressTestConfig.REGRESS_TEST_CASE_DIR, self.args.test_dir)
        else:
            test_file_list = RegressTestConfig.REGRESS_TEST_CASE_DIR
        for dir_path, path, filenames in os.walk(test_file_list):
            if dir_path.find(".git") != -1:
                continue
            for filename in filenames:
                if filename.endswith(".js") or filename.endswith(".mjs"):
                    result.append(str(os.path.join(dir_path, filename)))
        return result


class RegressTestCompile(RegressTestStep):
    def __init__(self, args, test_reports: List[TestReport]):
        RegressTestStep.__init__(self, args, "Regress test compilation")
        self.out_dir = args.out_dir
        self.test_reports = test_reports
        for test in self.test_reports:
            test.out_path = os.path.dirname(os.path.join(self.out_dir, test.test_id))

    @staticmethod
    def run(args, test_reports: Optional[List[TestReport]] = None) -> List[TestReport]:
        if test_reports is None:
            output("No tests loaded")
            exit(-1)
        test_prepare = RegressTestCompile(args, test_reports)
        RegressTestCompile.step_obj = test_prepare
        test_prepare._start()
        test_reports = test_prepare.gen_abc_files()
        test_prepare._end()
        return test_reports

    @staticmethod
    def create_files_info(test_report: TestReport) -> Tuple[str, str]:
        src_files_info = [
            RegressTestConfig.REGRESS_TEST_TOOL_DIR,
            test_report.src_path
        ]
        file_info_content: List[str] = []
        file_info_path = str(os.path.join(
            test_report.out_path,
            f"{Utils.get_file_only_name(test_report.src_path)}-filesInfo.txt"))
        os.makedirs(test_report.out_path, exist_ok=True)
        with os.fdopen(
                os.open(file_info_path, flags=os.O_RDWR | os.O_CREAT, mode=stat.S_IRUSR | stat.S_IWUSR),
                mode="w+", encoding="utf-8"
        ) as fp:
            for src_file_info in src_files_info:
                line = f"{src_file_info};{Utils.get_file_only_name(src_file_info)};esm;xxx;yyy\n"
                file_info_content.append(line)
                fp.write(line)
        return file_info_path, "\n".join(file_info_content)

    def gen_abc_files(self) -> List[TestReport]:
        with multiprocessing.Pool(processes=self.args.processes) as pool:
            results = pool.imap_unordered(self.gen_abc_file, self.test_reports)
            results = list(results)
            pool.close()
            pool.join()

        return results

    def gen_abc_file(self, test_report: TestReport) -> Optional[TestReport]:
        if test_report.src_path == RegressTestConfig.REGRESS_TEST_TOOL_DIR:
            return None
        file_info_path, file_info_content = self.create_files_info(test_report)
        out_file = change_extension(test_report.src_path, '.out')
        expect_file_exists = os.path.exists(out_file)
        output_file = change_extension(
            os.path.join(test_report.out_path, Utils.get_file_name(test_report.test_id)),
            ".abc")
        command = [
            self.args.ark_frontend_binary,
            f"@{file_info_path}",
            "--merge-abc",
            "--module",
            f'--output={output_file}'
        ]
        step_result = StepResult(self.name, command=command, fileinfo=file_info_content)
        Utils.exec_command(command, test_report.test_id, step_result, self.args.timeout,
                           lambda rt, _, _2: get_extra_error_message(rt))
        test_report.steps.append(step_result)
        test_report.passed = step_result.is_passed
        if expect_file_exists:
            out_file_path = os.path.join(test_report.out_path, change_extension(test_report.test_id, '.out'))
            shutil.copy(str(out_file), str(out_file_path))
        return test_report


class RegressTestPgo(RegressTestStep):
    def __init__(self, args):
        RegressTestStep.__init__(self, args, "Regress Test PGO ")

    @staticmethod
    def run(args, test_reports: Optional[List[TestReport]] = None) -> List[TestReport]:
        pgo = RegressTestPgo(args)
        RegressTestPgo.step_obj = pgo
        pgo._start()
        test_reports = pgo.generate_aps(test_reports)
        pgo._end()
        return test_reports

    def get_test_ap_cmd(self, test_report: TestReport) -> List[str]:
        abc_file = change_extension(
            os.path.join(test_report.out_path, Utils.get_file_name(test_report.test_id)),
            ".abc")
        ap_file = change_extension(abc_file, ".ap")
        entry_point = Utils.get_file_only_name(RegressTestConfig.TEST_TOOL_FILE_JS_NAME)
        os.environ["LD_LIBRARY_PATH"] = self.args.ld_library_path
        gen_ap_cmd = []
        if self.args.ark_arch == RegressTestConfig.ARK_ARCH_LIST[1]:
            qemu_tool = "qemu-aarch64"
            gen_ap_cmd = [
                qemu_tool,
                "-L",
                self.args.ark_arch_root
            ]
        gen_ap_cmd.append(self.args.ark_tool)
        gen_ap_cmd.append("--log-level=info")
        gen_ap_cmd.append(f"--icu-data-path={self.args.icu_path}")
        gen_ap_cmd.append("--compiler-target-triple=aarch64-unknown-linux-gn")
        gen_ap_cmd.append("--enable-pgo-profiler=true")
        gen_ap_cmd.append("--compiler-opt-inlining=true")
        gen_ap_cmd.append(f"--compiler-pgo-profiler-path={ap_file}")
        gen_ap_cmd.append("--asm-interpreter=true")
        gen_ap_cmd.append(f"--entry-point={entry_point}")
        gen_ap_cmd.append(f"{abc_file}")
        return gen_ap_cmd

    def generate_ap(self, test_report: Optional[TestReport]) -> Optional[TestReport]:
        if test_report is None or not test_report.passed:
            return test_report
        command = self.get_test_ap_cmd(test_report)
        step = StepResult(self.name, command=command)
        Utils.exec_command(command, test_report.test_id, step, self.args.timeout,
                           lambda rt, _, _2: get_extra_error_message(rt))
        test_report.steps.append(step)
        test_report.passed = step.is_passed
        return test_report

    def generate_aps(self, test_reports: List[TestReport]) -> List[TestReport]:
        with multiprocessing.Pool(processes=self.args.processes) as pool:
            results = pool.imap_unordered(self.generate_ap, test_reports)
            results = list(results)
            pool.close()
            pool.join()

        return results


class Utils:
    ark_regress = "ark-regress"

    @staticmethod
    def get_file_only_name(full_file_name: str) -> str:
        _, file_name = os.path.split(full_file_name)
        only_name, _ = os.path.splitext(file_name)
        return only_name

    @staticmethod
    def get_file_name(full_file_name: str) -> str:
        _, file_name = os.path.split(full_file_name)
        return file_name

    @staticmethod
    def mk_dst_dir(file, src_dir, dist_dir):
        idx = file.rfind(src_dir)
        fpath, _ = os.path.split(file[idx:])
        fpath = fpath.replace(src_dir, dist_dir)
        os.makedirs(fpath, exist_ok=True)

    @staticmethod
    def get_inside_path(file_path: str, marker: Optional[str] = None) -> str:
        if marker is None:
            marker = Utils.ark_regress
        index = file_path.find(marker)
        if index > -1:
            return file_path[index + len(marker) + 1:]
        return file_path

    @staticmethod
    def exec_command(cmd_args, test_id: str, step_result: StepResult, timeout=RegressTestConfig.DEFAULT_TIMEOUT,
                     get_extra_error_msg: Optional[Callable[[int, str, str], str]] = None) -> None:
        code_format = 'utf-8'
        if platform.system() == "Windows":
            code_format = 'gbk'
        cmd_string = "\n\t".join([str(arg).strip() for arg in cmd_args if arg is not None])
        msg_cmd = "\n".join([
            f"Run command:\n{cmd_string}",
            f"Env: {os.environ.get('LD_LIBRARY_PATH')}"
        ])
        msg_result = f"TEST ({step_result.step_name.strip()}): {test_id}"
        try:
            with subprocess.Popen(cmd_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True,
                                  start_new_session=True) as process:
                output_res, errs = process.communicate(timeout=timeout)
                ret_code = process.poll()
                step_result.return_code = ret_code
                stderr = str(errs.decode(code_format, 'ignore').strip())
                stdout = str(output_res.decode(code_format, 'ignore').strip())
                extra_message = get_extra_error_msg(ret_code, stdout, stderr) if get_extra_error_msg is not None else ""
                step_result.stderr = f"{extra_message + '. ' if extra_message else '' }{stderr if stderr else ''}"
                step_result.stdout = stdout
                if ret_code == 0:
                    msg_result = f"{msg_result} PASSED"
                    step_result.is_passed = True
                else:
                    msg_result = f"{msg_result} FAILED"
        except subprocess.TimeoutExpired:
            process.kill()
            process.terminate()
            step_result.return_code = -1
            step_result.stderr = f"Timeout: timed out after {timeout} seconds"
            msg_result = f"{msg_result} FAILED"
        except Exception as exc:
            step_result.return_code = -1
            step_result.stderr = f"Unknown error: {exc}"
            msg_result = f"{msg_result} FAILED"
        if step_result.is_passed:
            output(msg_result)
            output_debug(msg_cmd)
        else:
            output(f"{msg_result}\n{step_result.stderr}\n{msg_cmd}")

    @staticmethod
    def read_skip_list(skip_list_path: str) -> List[str]:
        skip_tests_list = []
        with os.fdopen(os.open(skip_list_path, os.O_RDONLY, stat.S_IRUSR), "r") as file_object:
            json_data = json.load(file_object)
            for key in json_data:
                skip_tests_list.extend(key["files"])
        return skip_tests_list

    @staticmethod
    def read_file_as_str(file_name: str) -> str:
        with os.fdopen(os.open(file_name, os.O_RDONLY, stat.S_IRUSR), "r") as file_object:
            content = [line.strip() for line in file_object.readlines()]
        return "\n".join(content)


class RegressTestAot(RegressTestStep):
    def __init__(self, args):
        RegressTestStep.__init__(self, args, "Regress Test AOT mode")

    @staticmethod
    def run(args, test_reports: Optional[List[TestReport]] = None) -> List[TestReport]:
        aot = RegressTestAot(args)
        RegressTestAot.step_obj = aot
        aot._start()
        test_reports = aot.compile_aots(test_reports)
        aot._end()
        return test_reports

    def get_test_aot_cmd(self, test_report: TestReport) -> List[str]:
        abc_file = change_extension(
            os.path.join(test_report.out_path, Utils.get_file_name(test_report.test_id)),
            ".abc")
        ap_file = change_extension(abc_file, ".ap")
        aot_file = change_extension(abc_file, "")
        os.environ["LD_LIBRARY_PATH"] = self.args.ld_library_path
        if self.args.ark_arch == RegressTestConfig.ARK_ARCH_LIST[1]:
            aot_cmd = [
                "qemu-aarch64",
                "-L",
                self.args.ark_arch_root,
                self.args.ark_aot_tool,
                "--compiler-target-triple=aarch64-unknown-linux-gnu",
                f"--aot-file={aot_file}"
            ]
        else:
            aot_cmd = [
            self.args.ark_aot_tool,
            f"--aot-file={aot_file}",
        ]

        pgo = [
            "--compiler-opt-loop-peeling=true",
            "--compiler-fast-compile=false",
            "--compiler-opt-track-field=true",
            "--compiler-opt-inlining=true",
            "--compiler-max-inline-bytecodes=45",
            "--compiler-opt-level=2",
            f"--compiler-pgo-profiler-path={ap_file}",
        ]
        litecg = [
            "--compiler-enable-litecg=true",
        ]
        aot_cmd_tail = [
            f"{abc_file}",
        ]

        if self.args.run_pgo:
            aot_cmd.extend(pgo)
        if self.args.enable_litecg:
            aot_cmd.extend(litecg)
        aot_cmd.extend(aot_cmd_tail)
        return aot_cmd

    def compile_aot(self, test_report: Optional[TestReport]) -> Optional[TestReport]:
        if test_report is None or not test_report.passed:
            return test_report
        command = self.get_test_aot_cmd(test_report)
        step = StepResult(self.name, command=command)
        Utils.exec_command(command, test_report.test_id, step, self.args.timeout,
                           lambda rt, _, _2: get_extra_error_message(rt))
        test_report.steps.append(step)
        test_report.passed = step.is_passed
        return test_report

    def compile_aots(self, test_reports: List[TestReport]) -> List[TestReport]:
        with multiprocessing.Pool(processes=self.args.processes) as pool:
            results = pool.imap_unordered(self.compile_aot, test_reports)
            results = list(results)
            pool.close()
            pool.join()

        return results


class RegressOption(Enum):
    NO_FORCE_GC = auto()
    ELEMENTS_KIND = auto()


def get_regress_groups() -> Dict[RegressOption, List[str]]:
    groups = {}
    with os.fdopen(os.open(RegressTestConfig.REGRESS_TEST_OPTIONS, os.O_RDONLY, stat.S_IRUSR), "r") as file:
        for group in json.load(file):
            groups[RegressOption[group["name"]]] = group["files"]
    return groups


def get_test_options(test: str, test_groups: Dict[RegressOption, List[str]], regress_option: RegressOption) -> List[str]:
    opt_values: Dict[RegressOption, str] = {
        RegressOption.NO_FORCE_GC: "--enable-force-gc=",
        RegressOption.ELEMENTS_KIND: "--enable-elements-kind="
    }

    def match(opt: RegressOption) -> bool:
        return test in test_groups.get(opt, [])

    def to_flag(b: bool) -> str:
        return str(b).lower()

    try:
        return [opt_values.get(regress_option) + to_flag(not match(regress_option))]
    except KeyError:
        return []


class RegressTestRun(RegressTestStep):
    def __init__(self, args):
        RegressTestStep.__init__(self, args, "Regress Test Run ")
        self.test_groups: Dict[RegressOption, List[str]] = get_regress_groups()

    @staticmethod
    def run(args, test_reports: Optional[List[TestReport]] = None) -> List[TestReport]:
        runner = RegressTestRun(args)
        RegressTestRun.step_obj = runner
        runner._start()
        test_reports = runner.run_test_case_dir(test_reports)
        runner._end()
        return test_reports

    @staticmethod
    def extra_check_with_expect(ret_code: int, test_report: TestReport, expect_file, stdout: str, stderr: str) -> str:
        expect_output_str = read_expect_file(expect_file, test_report.src_path)
        test_case_file = Utils.read_file_as_str(test_report.src_path)
        if stdout == expect_output_str.strip() or stderr == expect_output_str.strip():
            if ret_code == 0 or (ret_code == 255 and "/fail/" in test_case_file):
                return ""
            else:
                return get_extra_error_message(ret_code)
        msg = f'expect: [{expect_output_str}]\nbut got: [{stderr}]'
        return msg

    @staticmethod
    def extra_check_with_assert(ret_code: int, stderr: Optional[str]) -> str:
        if ret_code == 0 and stderr and "[ecmascript] Stack overflow" not in stderr:
            return str(stderr)
        return get_extra_error_message(ret_code)

    def run_test_case_dir(self, test_reports: List[TestReport]) -> List[TestReport]:
        with multiprocessing.Pool(processes=self.args.processes, initializer=init_worker,
                                  initargs=(self.args,)) as pool:
            results = pool.imap_unordered(self.run_test_case, test_reports)
            results = list(results)
            pool.close()
            pool.join()

        return results

    def run_test_case(self, test_report: TestReport) -> Optional[TestReport]:
        self.args = worker_wrapper_args
        if self.args is None or test_report is None or not test_report.passed:
            return test_report
        if test_report.src_path.endswith(RegressTestConfig.TEST_TOOL_FILE_JS_NAME):
            return None

        abc_file = change_extension(
            os.path.join(test_report.out_path, Utils.get_file_name(test_report.test_id)),
            ".abc")
        aot_file = change_extension(abc_file, "")
        expect_file = change_extension(abc_file, ".out")
        entry_point = Utils.get_file_only_name(RegressTestConfig.TEST_TOOL_FILE_JS_NAME)

        os.environ["LD_LIBRARY_PATH"] = self.args.ld_library_path

        # test environ LC_ALL/TZ
        test_name = test_report.test_id.replace('regresstest/ark-regress/', '')
        set_test_environ(test_report.src_path)
        command = []
        if self.args.ark_arch == RegressTestConfig.ARK_ARCH_LIST[1]:
            qemu_tool = "qemu-aarch64"
            qemu_arg1 = "-L"
            qemu_arg2 = self.args.ark_arch_root
            command = [qemu_tool, qemu_arg1, qemu_arg2]
        command.append(self.args.ark_tool)
        command.append(f"--icu-data-path={self.args.icu_path}")
        command.append(f"--entry-point={entry_point}")
        if self.args.ark_aot:
            command.append(f"--stub-file={self.args.stub_path}")
            command.append(f"--aot-file={aot_file}")
        if self.args.disable_force_gc:
            command.append("--enable-force-gc=false")
        else:
            command.extend(get_test_options(test_name, self.test_groups, RegressOption.NO_FORCE_GC))
        command.extend(get_test_options(test_name, self.test_groups, RegressOption.ELEMENTS_KIND))
        command.append(abc_file)

        return self.run_test_case_file(command, test_report, expect_file)

    def run_test_case_file(self, command, test_report: TestReport, expect_file) -> TestReport:
        expect_file_exits = os.path.exists(expect_file)
        step = StepResult(self.name, command=command)
        if expect_file_exits:
            self.run_test_case_with_expect(command, step, test_report, expect_file)
        else:
            self.run_test_case_with_assert(command, step, test_report)
        test_report.steps.append(step)
        test_report.passed = step.is_passed
        return test_report

    def run_test_case_with_expect(self, command, step: StepResult, test_report: TestReport, expect_file) -> None:
        Utils.exec_command(command, test_report.test_id, step, self.args.timeout,
                           lambda rt, out, err: self.extra_check_with_expect(rt, test_report, expect_file, out, err))

    def run_test_case_with_assert(self, command, step: StepResult, test_report: TestReport) -> None:
        Utils.exec_command(command, test_report.test_id, step, self.args.timeout,
                           lambda rt, _, err: self.extra_check_with_assert(rt, err))


class Stats:
    def __init__(self, args, test_reports: List[TestReport]):
        self.args = args
        self.pass_count = 0
        self.fail_count = 0
        self.test_reports = test_reports
        self.errors: Dict[str, List[TestReport]] = {}

    def read_ignore_list(self) -> Optional[Set[str]]:
        if self.args.ignore_list is None:
            return None
        with os.fdopen(os.open(self.args.ignore_list, os.O_RDWR, stat.S_IRUSR), "r+") as file_object:
            lines = file_object.readlines()
            lines = [line.strip() for line in lines if not line.strip().startswith('#')]
        return set(lines)

    def get_new_failures(self) -> Optional[List[TestReport]]:
        ignore_list = self.read_ignore_list()
        if ignore_list is None:
            return None
        new_failures: List[TestReport] = []
        for test_report in self.test_reports:
            if test_report and not test_report.passed and test_report.steps:
                if test_report.test_id not in ignore_list:
                    new_failures.append(test_report)
        return new_failures

    def statistics(self):
        root = XTree.Element("testsuite")
        root.set("name", "Regression")

        result_file = open_write_file(self.args.out_result, False)
        for test_report in self.test_reports:
            if test_report is None:
                continue
            testcase = XTree.SubElement(root, "testcase")
            testcase.set("name", f"{test_report.test_id}")
            if test_report.passed:
                write_result_file(f"PASS: {test_report.test_id}", result_file)
                self.pass_count += 1
            else:
                self.fail_count += 1
                write_result_file(f"FAIL: {test_report.test_id}", result_file)
                failed_step = test_report.steps[-1]
                if failed_step.step_name not in self.errors:
                    self.errors[failed_step.step_name] = []
                self.errors[failed_step.step_name].append(test_report)
                XTree.SubElement(testcase, "failure").text = f"<![CDATA[{test_report.report()}]]>"

        root.set("tests", f"{self.pass_count + self.fail_count}")
        root.set("failures", f"{self.fail_count}")

        tree = XTree.ElementTree(root)
        tree.write(self.args.junit_report, xml_declaration=True, encoding="UTF-8")
        result_file.close()

    def print_result(self, args, steps):
        result_file = open_write_file(args.out_result, True)
        summary_duration = datetime.timedelta()
        for step in steps:
            output(f"Step {step.step_obj.name} - duration {step.step_obj.get_duration()}")
            summary_duration += step.step_obj.get_duration()
        msg = f'\npass count: {self.pass_count}'
        write_result_file(msg, result_file)
        output(msg)
        msg = f'fail count: {self.fail_count}'
        write_result_file(msg, result_file)
        output(msg)
        msg = f'total count: {self.fail_count + self.pass_count}'
        write_result_file(msg, result_file)
        output(msg)
        msg = f'total used time is: {str(summary_duration)}'
        write_result_file(msg, result_file)
        output(msg)
        result_file.close()

    def print_failed_tests(self):
        output("=== Failed tests ===")
        for key, values in self.errors.items():
            output(f"{key}: {len(values)} tests")


def change_extension(path, new_ext: str):
    base_path, ext = os.path.splitext(path)
    if ext:
        new_path = base_path + new_ext
    else:
        new_path = path + new_ext
    return new_path


def get_files_by_ext(start_dir, suffix):
    result = []
    for dir_path, dir_names, filenames in os.walk(start_dir):
        for filename in filenames:
            if filename.endswith(suffix):
                result.append(os.path.join(dir_path, filename))
    return result


def read_expect_file(expect_file, test_case_file):
    with os.fdopen(os.open(expect_file, os.O_RDWR, stat.S_IRUSR), "r+") as file_object:
        lines = file_object.readlines()
        lines = [line for line in lines if not line.strip().startswith('#')]
        expect_output = ''.join(lines)
        if test_case_file.startswith("/"):
            test_case_file = test_case_file.lstrip("/")
        expect_file = test_case_file.replace('regresstest/', '')
        test_file_path = os.path.join(RegressTestConfig.REGRESS_BASE_TEST_DIR, expect_file)
        expect_output_str = expect_output.replace('*%(basename)s', test_file_path)
    return expect_output_str


def open_write_file(file_path, append):
    if append:
        args = os.O_RDWR | os.O_CREAT | os.O_APPEND
    else:
        args = os.O_RDWR | os.O_CREAT
    file_descriptor = os.open(file_path, args, stat.S_IRUSR | stat.S_IWUSR)
    file_object = os.fdopen(file_descriptor, "w+")
    return file_object


def open_result_excel(file_path):
    file_descriptor = os.open(file_path, os.O_RDWR | os.O_CREAT | os.O_APPEND, stat.S_IRUSR | stat.S_IWUSR)
    file_object = os.fdopen(file_descriptor, "w+")
    return file_object


def get_file_source(file):
    with open(file, encoding='ISO-8859-1') as f:
        return f.read()


def set_test_environ(case):
    # intl environ LC_ALL
    if 'LC_ALL' in os.environ:
        del os.environ['LC_ALL']
    if 'TZ' in os.environ:
        del os.environ['TZ']
    if not os.path.exists(case):
        return
    source = get_file_source(case)
    env_match = ENV_PATTERN.search(source)
    if env_match:
        for env_pair in env_match.group(1).strip().split():
            var, value = env_pair.split('=')
            if var.find('TZ') >= 0:
                os.environ['TZ'] = value
            if var.find('LC_ALL') >= 0:
                os.environ['LC_ALL'] = value
            break


# pylint: disable=invalid-name,global-statement
worker_wrapper_args = None


def init_worker(args):
    global worker_wrapper_args
    worker_wrapper_args = args


def write_result_file(msg: str, result_file):
    result_file.write(f'{msg}\n')


def main(args):
    if not check_args(args):
        return 1
    output("\nStart regresstest........")
    steps: List[Type[RegressTestStep]] = [
        RegressTestRepoPrepare,
        RegressTestCompile,
    ]
    if args.ark_aot:
        if args.run_pgo:
            steps.append(RegressTestPgo)
        steps.append(RegressTestAot)
    steps.append(RegressTestRun)

    test_reports: List[TestReport] = []
    for step in steps:
        test_reports = step.run(args, test_reports)

    stats = Stats(args, test_reports)
    stats.statistics()
    stats.print_result(args, steps)
    stats.print_failed_tests()
    new_failures = stats.get_new_failures()
    if new_failures is None:
        return 0
    if len(new_failures) > 0:
        msg = [f"Found {len(new_failures)} new failures:"]
        for failure in new_failures:
            msg.append(f"\t{failure.test_id}")
        output("\n".join(msg))
    else:
        output("No new failures have been found")
    return len(new_failures)


if __name__ == "__main__":
    sys.exit(main(parse_args()))
