#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Copyright (c) 2023 Huawei Device Co., Ltd.
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
import datetime
import json
import logging
import os
import platform
import re
import shutil
import signal
import stat
import subprocess
import sys

from regress_test_config import RegressTestConfig


def init_log_file(args):
    logging.basicConfig(filename=args.out_log, format=RegressTestConfig.DEFAULT_LOG_FORMAT, level=logging.INFO)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--test-dir', metavar='DIR',
                        help='Directory to test ')
    parser.add_argument('--test-file', metavar='FILE',
                        help='File to test')
    parser.add_argument('--timeout', default=RegressTestConfig.DEFAULT_TIMEOUT, type=int,
                        help='Set a custom test timeout in milliseconds !!!\n')
    parser.add_argument('--ark-tool',
                        help="ark's binary tool")
    parser.add_argument('--ark-frontend-binary',
                        help="ark frontend conversion binary tool")
    parser.add_argument('--LD_LIBRARY_PATH',
                        dest='ld_library_path', default=None, help='LD_LIBRARY_PATH')
    parser.add_argument('--icu-path',
                        dest='icu_path', help='icu-data-path')
    parser.add_argument('--out-dir',
                        default=None, help='target out dir')
    return parser.parse_args()


def check_args(args):
    current_dir = os.getcwd()
    if args.ark_frontend_binary is None:
        print('ark_frontend_binary is required, please add this parameter')
        return False
    else:
        current_frontend_binary = os.path.join(current_dir, args.ark_frontend_binary)
        test_tool_frontend_binary = os.path.join(RegressTestConfig.TEST_TOOL_FILE_DIR, args.ark_frontend_binary)
        if not os.path.exists(current_frontend_binary) and not os.path.exists(test_tool_frontend_binary):
            print('entered ark_frontend_binary does not exist. please confirm')
            return False
        else:
            args.ark_frontend_binary = current_frontend_binary if os.path.exists(
                current_frontend_binary) else test_tool_frontend_binary

    if args.ark_tool is None:
        print('ark_tool is required, please add this parameter')
        return False
    else:
        current_ark_tool = os.path.join(current_dir, args.ark_tool)
        test_tool_ark_tool = os.path.join(RegressTestConfig.TEST_TOOL_FILE_DIR, args.ark_tool)
        if not os.path.exists(current_ark_tool) and not os.path.exists(test_tool_ark_tool):
            print('entered ark_tool does not exist. please confirm')
            return False
        else:
            args.ark_tool = current_ark_tool if os.path.exists(current_ark_tool) else test_tool_ark_tool
    if args.ld_library_path is None:
        args.ld_library_path = RegressTestConfig.DEFAULT_LIBS_DIR
    if args.icu_path is None:
        args.icu_path = RegressTestConfig.ICU_PATH
    if args.out_dir is None:
        args.out_dir = RegressTestConfig.PROJECT_BASE_OUT_DIR
    else:
        args.out_dir = os.path.join(RegressTestConfig.CURRENT_PATH, args.out_dir)
    if not args.out_dir.endswith("/"):
        args.out_dir = f"{args.out_dir}/"
    args.regress_out_dir = os.path.join(args.out_dir, "regresstest")
    args.out_result = os.path.join(args.regress_out_dir, 'result.txt')
    args.out_log = os.path.join(args.regress_out_dir, 'test.log')
    args.test_case_out_dir = os.path.join(args.regress_out_dir, RegressTestConfig.REGRESS_GIT_REPO)
    return True


def get_skip_test_cases():
    skip_tests_list = []
    with os.fdopen(os.open(RegressTestConfig.SKIP_LIST_FILE, os.O_RDONLY, stat.S_IRUSR), "r") as file_object:
        json_data = json.load(file_object)
        for key in json_data:
            skip_tests_list.extend(key["files"])
    return skip_tests_list


def remove_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)


def git_clone(git_url, code_dir):
    cmds = ['git', 'clone', git_url, code_dir]
    result = True
    with subprocess.Popen(cmds) as proc:
        ret = proc.wait()
        if ret:
            print(f"\n error: Cloning '{git_url}' failed.")
            result = False
    return result


def output(msg):
    print(str(msg))
    logging.info(str(msg))


def out_put_std(ret_code, cmds, msg):
    error_messages = {
        0: msg,
        -6: f'{cmds}:{msg}\nAborted (core dumped)',
        -4: f'{cmds}:{msg}\nAborted (core dumped)',
        -11: f'{cmds}:{msg}\nSegmentation fault (core dumped)'
    }
    error_message = error_messages.get(ret_code, f'{cmds}: Unknown Error: {str(ret_code)}')
    if error_message != '':
        output(str(error_message))


def exec_command(cmd_args, timeout=RegressTestConfig.DEFAULT_TIMEOUT):
    code_format = 'utf-8'
    if platform.system() == "Windows":
        code_format = 'gbk'
    code = 0
    msg = ""
    try:
        with subprocess.Popen(cmd_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True,
                              start_new_session=True) as process:
            cmd_string = " ".join(cmd_args)
            output_res, errs = process.communicate(timeout=timeout)
            ret_code = process.poll()
            if errs.decode(code_format, 'ignore'):
                msg += errs.decode(code_format, 'ignore')
            if ret_code and ret_code != 1:
                code = ret_code
                msg += f"Command {cmd_string}:\n error: {errs.decode(code_format, 'ignore')}"
            else:
                msg += output_res.decode(code_format, 'ignore')
    except subprocess.TimeoutExpired:
        process.kill()
        process.terminate()
        os.kill(process.pid, signal.SIGTERM)
        code = 1
        msg += f"Timeout: '{cmd_string}' timed out after {timeout} seconds"
    except Exception as error:
        code = 1
        msg += f"{cmd_string}: unknown error: {error}"
    out_put_std(code, cmd_args, msg)


class RegressTestPrepare:
    def __init__(self, args, skip_test_cases):
        self.args = args
        self.out_dir = args.regress_out_dir
        self.skil_test = skip_test_cases

    def run(self):
        self.get_test_case()
        self.prepare_clean_data()
        test_list = self.get_regress_test_files()
        self.gen_test_tool()
        self.gen_abc_files(test_list)

    def get_test_case(self):
        if not os.path.isdir(os.path.join(RegressTestConfig.REGRESS_TEST_CASE_DIR, '.git')):
            git_clone(RegressTestConfig.REGRESS_GIT_URL, RegressTestConfig.REGRESS_TEST_CASE_DIR)
            return self.git_checkout(RegressTestConfig.REGRESS_GIT_HASH, RegressTestConfig.REGRESS_TEST_CASE_DIR)
        return True

    @staticmethod
    def change_extension(path):
        base_path, ext = os.path.splitext(path)
        if ext:
            new_path = base_path + ".abc"
        else:
            new_path = path + ".abc"
        return new_path

    def gen_test_tool(self):
        self.gen_abc_files([RegressTestConfig.REGRESS_TEST_TOOL_DIR])

    def gen_abc_files(self, test_list):
        for file_path in test_list:
            input_file_path = file_path
            start_index = input_file_path.find(RegressTestConfig.REGRESS_GIT_REPO)
            if start_index != -1:
                test_case_path = input_file_path[start_index + len(RegressTestConfig.REGRESS_GIT_REPO) + 1:]
                file_extensions = ['.out', '.txt', '.abc']
                pattern = r'({})$'.format('|'.join(re.escape(ext) for ext in file_extensions))
                if re.search(pattern, test_case_path) or test_case_path in self.skil_test:
                    continue
                out_flle = input_file_path.replace('.js', '.out')
                expect_file_exits = os.path.exists(out_flle)
                src_dir = RegressTestConfig.REGRESS_TEST_CASE_DIR
                out_dir = self.args.test_case_out_dir
                self.mk_dst_dir(input_file_path, src_dir, out_dir)
                output_test_case_path = self.change_extension(test_case_path)
                output_file = os.path.join(self.args.test_case_out_dir, output_test_case_path)
                command = [self.args.ark_frontend_binary]
                command.extend([input_file_path, f'--output={output_file}'])
                exec_command(command)
                if expect_file_exits:
                    out_file_path = os.path.join(self.args.test_case_out_dir, test_case_path.replace('.js', '.out'))
                    shutil.copy(out_flle, out_file_path)

    def mk_dst_dir(self, file, src_dir, dist_dir):
        idx = file.rfind(src_dir)
        fpath, _ = os.path.split(file[idx:])
        fpath = fpath.replace(src_dir, dist_dir)
        self.mk_dir(fpath)

    @staticmethod
    def mk_dir(path):
        if not os.path.exists(path):
            os.makedirs(path)

    def get_regress_test_files(self):
        result = []
        if self.args.test_file is not None and len(self.args.test_file) > 0:
            test_file_list = os.path.join(RegressTestConfig.REGRESS_TEST_CASE_DIR, self.args.test_file)
            result.append(test_file_list)
            return result
        elif self.args.test_dir is not None and len(self.args.test_dir) > 0:
            test_file_list = os.path.join(RegressTestConfig.REGRESS_TEST_CASE_DIR, self.args.test_dir)
        else:
            test_file_list = RegressTestConfig.REGRESS_TEST_CASE_DIR
        for dir_path, path, filenames in os.walk(test_file_list):
            if dir_path.find(".git") != -1:
                continue
            for filename in filenames:
                result.append(os.path.join(dir_path, filename))
        return result

    def prepare_clean_data(self):
        self.git_clean(RegressTestConfig.REGRESS_TEST_CASE_DIR)
        self.git_pull(RegressTestConfig.REGRESS_TEST_CASE_DIR)
        self.git_checkout(RegressTestConfig.REGRESS_GIT_HASH, RegressTestConfig.REGRESS_TEST_CASE_DIR)

    @staticmethod
    def git_checkout(checkout_options, check_out_dir=os.getcwd()):
        cmds = ['git', 'checkout', checkout_options]
        result = True
        with subprocess.Popen(cmds, cwd=check_out_dir) as proc:
            ret = proc.wait()
            if ret:
                print(f"\n error: git checkout '{checkout_options}' failed.")
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


def run_regress_test_prepare(args):
    remove_dir(args.regress_out_dir)
    remove_dir(RegressTestConfig.REGRESS_TEST_CASE_DIR)
    os.makedirs(args.regress_out_dir)
    init_log_file(args)
    skip_tests_case = get_skip_test_cases()
    test_prepare = RegressTestPrepare(args, skip_tests_case)
    test_prepare.run()


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


def run_test_case_with_expect(command, test_case_file, expect_file, result_file, timeout):
    ret_code = 0
    expect_output_str = read_expect_file(expect_file, test_case_file)
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        try:
            out, err = process.communicate(timeout=timeout)
            out_str = out.decode('UTF-8').strip()
            err_str = err.decode('UTF-8').strip()
            ret_code = process.poll()
            # ret_code equals 0 means execute ok, ret_code equals 255 means uncaught error
            if (ret_code == 0 and (out_str == expect_output_str.strip() or err_str == expect_output_str.strip())) or \
                ret_code == 255:
                write_result_file(f'PASS {test_case_file} \n', result_file)
                out_put_std(ret_code, command, f'PASS: {test_case_file}')
                return True
            else:
                msg = f'FAIL: {test_case_file} \nexpect: [{expect_output_str}]\nbut got: [{err_str}]'
                out_put_std(ret_code, command, msg)
                write_result_file(f'FAIL {test_case_file} \n', result_file)
                return False
        except subprocess.TimeoutExpired as exception:
            process.kill()
            process.terminate()
            os.kill(process.pid, signal.SIGTERM)
            out_put_std(ret_code, command, f'FAIL: {test_case_file},err:{exception}')
            write_result_file(f'FAIL {test_case_file} \n', result_file)
            return False


def run_test_case_with_assert(command, test_case_file, result_file, timeout):
    ret_code = 0
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        try:
            out, err = process.communicate(timeout=timeout)
            ret_code = process.poll()
            err_str = err.decode('UTF-8')
            # ret_code equals 0 means execute ok, ret_code equals 255 means uncaught error
            if ret_code == 255:
                out_put_std(ret_code, command, f'PASS: {test_case_file}')
                write_result_file(f'PASS {test_case_file} \n', result_file)
                return True
            elif ret_code != 0 or (err_str != '' and "[ecmascript] Stack overflow" not in err_str):
                out_put_std(ret_code, command, f'FAIL: {test_case_file} \nerr: {str(err_str)}')
                write_result_file(f'FAIL {test_case_file} \n', result_file)
                return False
            else:
                out_put_std(ret_code, command, f'PASS: {test_case_file}')
                write_result_file(f'PASS {test_case_file} \n', result_file)
                return True
        except subprocess.TimeoutExpired as exception:
            process.kill()
            process.terminate()
            os.kill(process.pid, signal.SIGTERM)
            out_put_std(ret_code, command, f'FAIL: {test_case_file},err:{exception}')
            write_result_file(f'FAIL {test_case_file} \n', result_file)
            return False


def run_test_case_file(command, test_case_file, expect_file, result_file, timeout):
    expect_file_exits = os.path.exists(expect_file)
    if expect_file_exits:
        return run_test_case_with_expect(command, test_case_file, expect_file, result_file, timeout)
    else:
        return run_test_case_with_assert(command, test_case_file, result_file, timeout)


def run_test_case_dir(args, test_abc_files, force_gc_files, timeout=RegressTestConfig.DEFAULT_TIMEOUT):
    pass_count = 0
    fail_count = 0
    result_file = open_write_file(args.out_result, False)
    for index, file in enumerate(test_abc_files):
        test_file = file
        if test_file.endswith(RegressTestConfig.TEST_TOOL_FILE_NAME):
            continue
        test_case_file = test_file.replace('abc', 'js').replace(f'{args.out_dir}', '')
        assert_file = os.path.join(args.test_case_out_dir, RegressTestConfig.TEST_TOOL_FILE_NAME)
        test_case = f'{assert_file}:{file}'
        ld_library_path = args.ld_library_path
        os.environ["LD_LIBRARY_PATH"] = ld_library_path
        unforced_gc = False
        force_gc_file = test_case_file.replace('regresstest/ark-regress/', '')
        if force_gc_file in force_gc_files:
            unforced_gc = True
        asm_arg1 = "--enable-force-gc=true"
        if unforced_gc:
            asm_arg1 = "--enable-force-gc=false"
        icu_path = f"--icu-data-path={args.icu_path}"
        command = [args.ark_tool, asm_arg1, icu_path, test_case]
        expect_file = test_file.replace('.abc', '.out')
        test_res = run_test_case_file(command, test_case_file, expect_file, result_file, timeout)
        if test_res:
            pass_count += 1
        else:
            fail_count += 1
    result_file.close()
    return pass_count, fail_count


def run_regress_test_case(args):
    test_cast_list = get_regress_test_files(args.test_case_out_dir, 'abc')
    force_gc_files = get_regress_force_gc_files()
    return run_test_case_dir(args, test_cast_list, force_gc_files)


def get_regress_force_gc_files():
    skip_force_gc_tests_list = []
    with os.fdopen(os.open(RegressTestConfig.FORCE_GC_FILE_LIST, os.O_RDONLY, stat.S_IRUSR), "r") as file_object:
        json_data = json.load(file_object)
        for key in json_data:
            skip_force_gc_tests_list.extend(key["files"])
    return skip_force_gc_tests_list


def get_regress_test_files(start_dir, suffix):
    result = []
    for dir_path, dir_names, filenames in os.walk(start_dir):
        for filename in filenames:
            if filename.endswith(suffix):
                result.append(os.path.join(dir_path, filename))
    return result


def write_result_file(msg, result_file):
    result_file.write(f'{str(msg)}\n')


def main(args):
    if not check_args(args):
        return
    print("\nStart conducting regress test cases........\n")
    start_time = datetime.datetime.now()
    run_regress_test_prepare(args)
    pass_count, fail_count = run_regress_test_case(args)
    end_time = datetime.datetime.now()
    print_result(args, pass_count, fail_count, end_time, start_time)


def print_result(args, pass_count, fail_count, end_time, start_time):
    result_file = open_write_file(args.out_result, True)
    msg = f'\npass count: {pass_count}'
    write_result_file(msg, result_file)
    print(msg)
    msg = f'fail count: {fail_count}'
    write_result_file(msg, result_file)
    print(msg)
    msg = f'total count: {fail_count + pass_count}'
    write_result_file(msg, result_file)
    print(msg)
    msg = f'used time is: {str(end_time - start_time)}'
    write_result_file(msg, result_file)
    print(msg)
    result_file.close()


if __name__ == "__main__":
    sys.exit(main(parse_args()))
