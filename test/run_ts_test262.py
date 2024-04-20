#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#  Copyright (c) 2023 Huawei Device Co., Ltd.
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.


import os
import argparse
import subprocess
import re

asserts = os.path.join(os.getcwd(), "test262", 'assert')
total = 0
errno = 0


def is_testcase_exist(parse, arg):
    if not os.path.isabs(arg):
        arg = './' + arg
    if not os.path.exists(arg):
        parse.error("The directory or file '%s' does not exist" % arg)
    return os.path.abspath(arg)


def is_directory(parse, arg):
    if not os.path.isdir(arg):
        parse.error("The directory '%s' does not exist" % arg)

    return os.path.abspath(arg)


# create a parser object
parser = argparse.ArgumentParser(description="TypeScript Test262 Test Tool")
parser.add_argument("release", nargs='*', metavar="release", type=lambda arg: is_testcase_exist(parser, arg),
                    help="All test case in the release will be execute")
parser.add_argument(
    '--js-runtime', dest='js_runtime_path', default=None, type=lambda arg: is_directory(parser, arg),
    help='the path of js vm runtime')
parser.add_argument(
    '--LD_LIBRARY_PATH', dest='ld_library_path', default=None, help='LD_LIBRARY_PATH')

parser.add_argument(
    '--es2abc', dest='es2abc', default=None, help='ES2ABC_PATH')

parser.add_argument(
    '--ark_aot', dest='ark_aot', default=None, help='ARK_AOT')

args = parser.parse_args()


def get_path_file(dir_path, allfiles=None):
    if allfiles is None:
        allfiles = []
    file_or_dir = os.listdir(dir_path)
    for fileOrDir in file_or_dir:
        file_or_dir_path = os.path.join(dir_path, fileOrDir)
        if os.path.isfile(file_or_dir_path):
            allfiles.append(file_or_dir_path)
        else:
            get_path_file(file_or_dir_path, allfiles)
    return allfiles


def get_node_cmd(filepath):
    cmd = ['ts-node', filepath]
    return cmd


def get_error_message(strs, filename):
    if len(re.findall(filename + r':(\d+)', strs)) > 0:
        line_number = re.findall(filename + r':(\d+)', strs)
    else:
        line_number = 0
    err_message = strs
    return err_message, line_number


def get_es2abc_cmd(filepath):
    abc_file_path = ("%s.abc" % (os.path.splitext(filepath)[0]))
    abc_file_path_temp = abc_file_path
    cmd = [args.es2abc + 'es2abc']
    cmd.extend(['--module', '--merge-abc', '--extension=ts', '--output', abc_file_path, filepath])
    return cmd, abc_file_path_temp


# get es2abc file commands
def get_ark_js_cmd_aot(abc_file_path_temp):
    os.environ.setdefault("LD_LIBRARY_PATH", args.ld_library_path)
    path = abc_file_path_temp.split('.abc')[0]
    run_abc_cmd = [os.path.join(args.js_runtime_path, 'ark_js_vm'), '--asm-interpreter=true',
                   '--aot-file=' + path, '--entry-point=' + abc_file_path_temp.split('/')[-1].split('.abc')[0],
                   abc_file_path_temp]
    return run_abc_cmd


# get es2abc file commands
def get_ark_js_cmd(abc_file_path_temp):
    os.environ.setdefault("LD_LIBRARY_PATH", args.ld_library_path)
    run_abc_cmd = [os.path.join(args.js_runtime_path, 'ark_js_vm'), '--asm-interpreter=true',
                   '--entry-point=' + abc_file_path_temp.split('/')[-1].split('.abc')[0],
                   abc_file_path_temp]
    return run_abc_cmd


def create_abc(path):
    # compiler to abc
    cmd, abc_file_path_temp = get_es2abc_cmd(path)
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    process.communicate(timeout=5000)


def test_es2abc(path):
    # compiler to abc
    cmd, abc_file_path_temp = get_es2abc_cmd(path)
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate(timeout=5000)
    return_code = process.returncode
    if return_code != 0:
        get_error_message(
            out.decode("utf-8", errors="ignore") + err.decode("utf-8", errors="ignore"), path)
        return 0

    paths = abc_file_path_temp.split('.abc')[0]
    os.environ.setdefault("LD_LIBRARY_PATH", args.ld_library_path)
    cmds = [args.ark_aot, '--aot-file=', paths, abc_file_path_temp]
    # execute ark_aot
    if args.ark_aot:
        process = subprocess.Popen(cmds, stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        process.communicate(timeout=5000)

        # execute ark_js_vm
        process = subprocess.Popen(get_ark_js_cmd_aot(abc_file_path_temp), stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        out, err = process.communicate(timeout=5000)
    else:
        # execute ark_js_vm
        process = subprocess.Popen(get_ark_js_cmd(abc_file_path_temp), stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        out, err = process.communicate(timeout=5000)

    if ("SUCCESS" not in out.decode("utf-8", errors="ignore")) | (
            "AsyncTestFailure" in out.decode("utf-8", errors="ignore")):
        err_msg, lines = get_error_message(out.decode("utf-8", errors="ignore") + err.decode("utf-8", errors="ignore"),
                                           filePath)
        print("NEW ERROR :")
        print(filePath, err_msg, sep='\t')
        return 1
    else:
        return 0


contents = ['{']
for ass in os.listdir(asserts):
    if os.path.isfile(os.path.join(asserts, ass)):
        with open(os.path.join(asserts, ass), 'r', encoding="utf-8") as file:
            contents.append('\n')
            contents.extend(file.readlines())
            contents.append('\n')
print("TEST CASE", "FAIL REASON", "FAIL LINE", sep="\t")

for file_path in args.release:
    all_files = get_path_file(file_path)
    for filePath in all_files:
        create_abc(filePath)
    for filePath in all_files:
        count = 0
        is_case = False
        c = []
        c.extend(contents)
        content = []
        with open(filePath, 'r', encoding="ISO-8859-1") as file:
            while True:
                count += 1
                line = file.readline()
                start_pattern = re.compile(r'^\/\*\-*')
                if start_pattern.match(line):
                    is_case = True
                    break
                if count > 100:
                    break

            if is_case:
                with open(filePath, 'r', encoding="utf-8") as file1:
                    content.extend(file1.readlines())
                    content.append('\n')
                    c.extend(content)
                with open(filePath, 'w', encoding="utf-8") as files:
                    c.append('}')
                    c.extend(['\n', 'print("SUCCESS")'])
                    files.writelines(c)

        # abc
        if is_case:
            total += 1
            errno += test_es2abc(filePath)
            # cover files
            with open(filePath, 'w', encoding="utf-8") as files:
                content[-1] = content[-1].replace('\n', '')
                files.writelines(content)

# delete abc,ai,an files
for file_paths in get_path_file("test262/test"):
    if file_paths.endswith(".abc") | file_paths.endswith(".ai") | file_paths.endswith(".an"):
        if os.path.exists(file_paths):
            os.remove(file_paths)

print("TOTAL CASE COUNT:%d" % total)
print("SUCCESS CASE COUNT:%d" % (total - errno))
print("FAILED CASE COUNT:%d" % errno)
