#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Copyright (c) 2024 Huawei Device Co., Ltd.
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

import os
import subprocess
import argparse
import shutil


def get_script_directory():
    return os.path.dirname(os.path.abspath(__file__))


def copy_files_to_dist(target_dir):
    file_list = [
        "out/sdk/clang_x64/thirdparty/icu/libhmicui18n.so",
        "out/sdk/clang_x64/thirdparty/icu/libhmicuuc.so",
        "out/sdk/clang_x64/arkcompiler/ets_runtime/libprofDumpJson.so",
        "out/sdk/mingw_x86_64/thirdparty/icu/libhmicui18n.dll",
        "out/sdk/mingw_x86_64/thirdparty/icu/libhmicuuc.dll",
        "out/sdk/mingw_x86_64/arkcompiler/ets_runtime/libprofDumpJson.dll",
        "out/sdk/mingw_x86_64/hiviewdfx/hilog/libhilog.dll",
        "out/sdk/mingw_x86_64/thirdparty/bounds_checking_function/libsec_shared.dll",
        "out/sdk/mingw_x86_64/thirdparty/zlib/libshared_libz.dll"
    ]
    current_path = get_script_directory()
    dist_dir = os.path.join(current_path, "dist")
    for relative_file_path in file_list:
        absolute_file_path = os.path.join(target_dir, relative_file_path)
        if os.path.exists(absolute_file_path):
            print(f'Copied {absolute_file_path} to {dist_dir}')
            shutil.copy(absolute_file_path, dist_dir)
        else:
            print(f"File not found: {absolute_file_path}")


def run_command(command):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for line in process.stdout:
        print(line.decode('utf-8').strip())
    process.wait()
    if process.returncode != 0:
        raise Exception(f"Command failed with return code {process.returncode}")


def main(target_dir):
    original_dir = get_script_directory()
    try:
        command = ["npm", "install"]
        print(f"Running command: {command}")
        run_command(command)
        os.chdir(target_dir)
        print(f"Changed directory to: {os.getcwd()}")
        command = ["/bin/bash", "build.sh", "--product-name", "ohos-sdk", "--build-target",
                   "ap_viewer_all_host_tools_packages"]
        print(f"Running command: {command}")
        run_command(command)
        os.chdir(original_dir)
        print(f"Returned to original directory: {os.getcwd()}")
        npm_command = ["npm", "run", "build"]
        print(f"Running command: {npm_command}")
        run_command(npm_command)
        copy_files_to_dist(target_dir)
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        os.chdir(original_dir)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build script with optional target directory.")
    parser.add_argument("--target-dir", metavar="dir", type=str,
                        help="The target directory to cd into before running the build script.")
    args = parser.parse_args()
    target_directory = args.target_dir if args.target_dir else "../../../../"
    main(target_directory)
