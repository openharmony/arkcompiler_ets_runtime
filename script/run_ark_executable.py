#!/usr/bin/env python3
#coding: utf-8

"""
Copyright (c) 2021-2022 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: run script
    expect_output will get run result,
    expect_sub_output will catch pivotal sub output,
    expect_file will get print string
"""

import argparse
import os
import stat
import subprocess
import sys
import time


def get_env_path_from_rsp(script_file: str) -> list:
    """get env path from response file recursively."""
    rsp_file = "{0}{1}".format(script_file, ".rsp")
    if not os.path.exists(rsp_file):
        print(
            "File \"{}\" does not exist!\n" \
            "This indicates that its related shared_library is not compiled by this project, but there is an " \
            "executable or shared_library depend on its related shared_library!".format(rsp_file))
        sys.exit(1)

    rsp_info_list = []
    with open(rsp_file, "r") as fi:
        rsp_info_str = fi.read()
        rsp_info_list = rsp_info_str.split(" ")

    env_path_list = []
    for element in rsp_info_list:
        if element.endswith(".so") or element.endswith(".dll"):
            env_path_list.extend(get_env_path_from_rsp(element))
            env_path_list.append(os.path.dirname(element))
    return env_path_list


def get_command_and_env_path(args: object) -> [str, str]:
    """get command and environment path from args for running excutable."""
    env_path_list = list(set(get_env_path_from_rsp(args.script_file)))
    env_path_list.append(args.clang_lib_path)
    env_path = ":".join(env_path_list)
    if args.qemu_binary_path:
        if not os.path.exists(args.qemu_binary_path):
            print("Have you set up environment for running executables with qemu?\n" \
                "If not, get set-up steps from https://gitee.com/ark_standalone_build/docs ," \
                " append your build command of ark.py with option \"--clean-continue\"," \
                " and execute the appended command after setting up the environment.\n" \
                "If yes, the environment settings for qemu on your host machine may be different from what the link" \
                " above shows, it is suggested to match your local environment settings with what the link shows.")
            sys.exit(1)
        cmd = \
            "{}".format(args.qemu_binary_path) + \
            " -L {}".format(args.qemu_ld_prefix) + \
            " -E LD_LIBRARY_PATH={}".format(env_path) + \
            " {}".format(args.script_file)
    else:
        cmd = "{}".format(args.script_file)
    cmd += " {}".format(args.script_options) if args.script_options else ""
    cmd += " {}".format(args.script_args) if args.script_args else ""
    return [cmd, env_path]


def parse_args() -> object:
    """parse arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--script-file', help='execute script file')
    parser.add_argument('--script-options', help='execute script options')
    parser.add_argument('--script-args', help='args of script')
    parser.add_argument('--expect-output', help='expect output')
    parser.add_argument('--expect-sub-output', help='expect sub output')
    parser.add_argument('--expect-file', help='expect file')
    parser.add_argument('--env-path', help='LD_LIBRARY_PATH env')
    parser.add_argument('--timeout-limit', help='timeout limit')
    parser.add_argument('--clang-lib-path', help='part for LD_LIBRARY_PATH, it is not in .rsp file')
    parser.add_argument('--qemu-binary-path', help='path to qemu binary, run executable with qemu if assigned')
    parser.add_argument('--qemu-ld-prefix', help='elf interpreter prefix')
    args = parser.parse_args()
    return args


def process_open(args: object) -> [str, object]:
    """get command and open subprocess."""
    if args.env_path:
        # use the given env-path
        cmd = args.script_file
        cmd += " {}".format(args.script_options) if args.script_options else ""
        cmd += " {}".format(args.script_args) if args.script_args else ""
        # process for running executable directly
        subp = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            env={'LD_LIBRARY_PATH': str(args.env_path)})
    else:
        # get env-path from response file recursively
        [cmd, env_path] = get_command_and_env_path(args)
        if args.qemu_binary_path:
            # process for running executable with qemu
            subp = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        else:
            # process for running executable directly
            subp = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                env={'LD_LIBRARY_PATH': str(env_path)})
    return [cmd, subp]


def generate_stub_code_comment(out_str:str):
    flags = os.O_WRONLY | os.O_CREAT
    mode = stat.S_IWUSR | stat.S_IRUSR
    dir_path = './gen/arkcompiler/ets_runtime/'
    if not os.path.exists(dir_path):
        return
    file_path = dir_path + 'stub_code_comment.txt'
    fd = os.open(file_path, flags, mode)
    with os.fdopen(fd, "w") as code_comment_file:
        code_comment_file.write(out_str)


def judge_output(args: object):
    """run executable and judge is success or not."""
    start_time = time.time()
    [cmd, subp] = process_open(args)
    timeout_limit = int(args.timeout_limit) if args.timeout_limit else 1200  # units: s

    try:
        out, err = subp.communicate(timeout=timeout_limit)
    except subprocess.TimeoutExpired:
        raise RuntimeError('Run [', cmd, '] timeout, timeout_limit = ', timeout_limit, 's')

    out_str = out.decode('UTF-8', errors="ignore")
    err_str = err.decode('UTF-8', errors="ignore")
    generate_stub_code_comment(out_str)
    returncode = str(subp.returncode)
    if args.expect_output:
        if returncode != args.expect_output:
            print(">>>>> ret <<<<<")
            print(returncode)
            print(">>>>> out <<<<<")
            print(out_str)
            print(">>>>> err <<<<<")
            print(err_str)
            print(">>>>> Expect return: [" + args.expect_output \
                + "]\n>>>>> But got: [" + returncode + "]")
            raise RuntimeError("Run [" + cmd + "] failed!")
    elif args.expect_sub_output:
        if out_str.find(args.expect_sub_output) == -1 or returncode != "0":
            print(">>>>> ret <<<<<")
            print(returncode)
            print(">>>>> err <<<<<")
            print(err_str)
            print(">>>>> Expect contain: [" + args.expect_sub_output \
                + "]\n>>>>> But got: [" + out_str + "]")
            raise RuntimeError("Run [" + cmd + "] failed!")
    elif args.expect_file:
        with open(args.expect_file, mode='r') as file:
            # skip license header
            expect_output = ''.join(file.readlines()[13:])
            file.close()
            result_cmp = compare_line_by_line(expect_output, out_str)
            if result_cmp or returncode != "0":
                print(">>>>> ret <<<<<")
                print(returncode)
                print(">>>>> err <<<<<")
                print(err_str)
                print(">>>>> Expect {} lines: [{}]\n>>>>> But got {} lines: [{}]".format(
                    expect_output.count('\n'), expect_output, out_str.count('\n'), out_str
                ))
                raise RuntimeError("Run [" + cmd + "] failed!")
    else:
        raise RuntimeError("Run [" + cmd + "] with no expect !")

    print("Run [" + cmd + "] success!")
    print("used: %.5f seconds" % (time.time() - start_time))

def compare_line_by_line(expect_output:str, got_output:str):
    expect_output_list = expect_output.split("\n")
    got_output_list = got_output.split("\n")
    for index, (expect_line, got_line) in enumerate(zip(expect_output_list, got_output_list)):
        if expect_line == got_line:
            continue
        error_msg = ""

        if "__INT_MORE_PREV__" in expect_line:
            prev_got_value = reverse_find_first_not_trace_line(got_output_list, index-1)
            if got_line.isdigit() and prev_got_value.isdigit() and int(prev_got_value) < int(got_line):
                continue
            error_msg = "Got integer result is not more than previous integer result"

        if "__INT__" in expect_line:
            if got_line.isdigit():
                continue
            error_msg = "Got not integer"

        print(">>>>> diff <<<<<")
        if error_msg:
            print(error_msg)
        print("Difference in line {}:\nExcepted: [{}]\nBut got:  [{}]".format(index+1, expect_line, got_line))
        return True
    return False

def reverse_find_first_not_trace_line(output_list: list, init_index: int) -> str:
    for i in range(init_index, -1, -1):
        if "[trace]" not in output_list[i]:
            return output_list[i]
    return ""

if __name__ == '__main__':
    input_args = parse_args()
    judge_output(input_args)
