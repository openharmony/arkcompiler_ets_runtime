#!/usr/bin/env python3
#coding: utf-8
"""
Copyright (c) 2024 Huawei Device Co., Ltd.
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
    input: input file
    output: output file
    prefix: prefix
"""

import os
import sys
import argparse

replace_config1 = [
    {"id": "REPLACE_FUNC_FOO1", "start": 0, "end": 31608},
    {"id": "REPLACE_FUNC_FOO2", "start": 31609, "end": 65535},
]
replace_config2 = [
    {"id": "REPLACE_FUNC_FOO1", "start": 0, "end": 65488},
    {"id": "REPLACE_FUNC_FOO2", "start": 65489, "end": 65535},
]
replace_config3 = [
    {"id": "REPLACE_FUNC_FOO1", "start": 0, "end": 65488},
    {"id": "REPLACE_FUNC_FOO2", "start": 65489, "end": 65536},
]
replace_config4 = [
    {"id": "REPLACE_FUNC_FOO1", "start": 35004, "end": 40153},
    {"id": "REPLACE_FUNC_FOO2", "start": 1, "end": 34999, "isFunc": True, "msg": "base"},
]
replace_config5 = [
    {"id": "REPLACE_FUNC_FOO1", "start": 35004, "end": 40153},
    {"id": "REPLACE_FUNC_FOO2", "start": 1, "end": 34999, "isFunc": True, "msg": "patch"},
]

AOT_MULTI_CONSTANTPOOL_TEST_PATH = "ets_runtime/test/aottest/aot_multi_constantpool_test/"

file_list = [
    {
        "file_name": "ets_runtime/test/quickfix/multi_funcconstpool/base_modify.js",
        "replace_config": replace_config1,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_funcconstpool/base.js",
        "replace_config": replace_config1,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_funccallconstpool/base_modify.js",
        "replace_config": replace_config1,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_funccallconstpool/base.js",
        "replace_config": replace_config1,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_constructorconstpool/base_modify.js",
        "replace_config": replace_config1,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_constructorconstpool/base.js",
        "replace_config": replace_config1,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_closureconstpool/base_modify.js",
        "replace_config": replace_config3,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_closureconstpool/base.js",
        "replace_config": replace_config3,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_classconstpool/base_modify.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multi_classconstpool/base.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multiconstpool_multifunc/base.js",
        "replace_config": replace_config4,
    },
    {
        "file_name": "ets_runtime/test/quickfix/multiconstpool_multifunc/base_modify.js",
        "replace_config": replace_config5,
    },
    {
        "file_name": "ets_runtime/test/moduletest/multiconstpoolobj/multiconstpoolobj.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": "ets_runtime/test/moduletest/multiconstpoolfunc/multiconstpoolfunc.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": "ets_runtime/test/moduletest/multiconstpoolconstructor/multiconstpoolconstructor.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": "ets_runtime/test/moduletest/multiconstpoolclass/multiconstpoolclass.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": "ets_runtime/test/moduletest/multiconstpoolarray/multiconstpoolarray.js",
        "replace_config": replace_config2,
    },
    {
        "file_name": AOT_MULTI_CONSTANTPOOL_TEST_PATH
            + "multi_constantpool_func/multi_constantpool_func.ts",
        "replace_config": replace_config1,
    },
    {
        "file_name": AOT_MULTI_CONSTANTPOOL_TEST_PATH
            + "multi_constantpool_constructor/multi_constantpool_constructor.ts",
        "replace_config": replace_config2,
    },
    {
        "file_name": AOT_MULTI_CONSTANTPOOL_TEST_PATH
            + "multi_constantpool_class/multi_constantpool_class.ts",
        "replace_config": replace_config2,
    },
    {
        "file_name": AOT_MULTI_CONSTANTPOOL_TEST_PATH
            + "multi_constantpool_funccall/multi_constantpool_funccall.ts",
        "replace_config": replace_config1,
    },
    {
        "file_name": AOT_MULTI_CONSTANTPOOL_TEST_PATH
            + "multi_constantpool_closure/multi_constantpool_closure.ts",
        "replace_config": replace_config3,
    },
]


def generate_var(var_begin, var_end):
    str_var_list = []
    for i in range(var_begin, var_end + 1):
        str_var_list.append('var a{0} = "{1}";'.format(i, i))
        if (i + 1) % 6 == 0:
            str_var_list.append("\n")
    return ''.join(str_var_list)


def generate_funcs(var_begin, var_end, msg):
    str_func_list = []
    for i in range(var_begin, var_end + 1):
        str_func_list.append('function foo{0}()'.format(i))
        str_func_list.append('{\n\t')
        str_func_list.append('var a{0} = "{1}";\n\tprint("{2} foo{3}")\n'.format(i, i, msg, i))
        str_func_list.append('}\n')
    return ''.join(str_func_list)


def read_file_content(input_file):
    input_fd = os.open(input_file, os.O_RDONLY, 0o755)
    with os.fdopen(input_fd, 'r') as fp:
        return fp.read()


def write_file_content(output_file, data):
    output_fd = os.open(output_file, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o755)
    if os.path.exists(output_file):
        with os.fdopen(output_fd, 'w') as fp:
            fp.write(data)


def replace_in_data(data, replace_config):
    for cfg in replace_config:
        if cfg["id"] in data:
            if "isFunc" in cfg:
                str_func = generate_funcs(cfg["start"], cfg["end"], cfg["msg"])
                data = data.replace(cfg["id"], str_func)
            else:
                str_var = generate_var(cfg["start"], cfg["end"])
                data = data.replace(cfg["id"], str_var)
    return data


def replace_js(input_file, output_file):
    for file in file_list:
        file_name = file.get("file_name")
        if not input_file.endswith(file_name):
            continue
        data = read_file_content(input_file)
        replace_config = file.get("replace_config")
        data = replace_in_data(data, replace_config)
        write_file_content(output_file, data)
        return output_file
    return None


def is_file_in_list(file_path):
    input_dir_path = (
        os.path.dirname(file_path)
        if os.path.isfile(file_path)
        else file_path
    )

    for item in file_list:
        file_name = item.get("file_name")
        list_dir_path = os.path.dirname(file_name)
        if list_dir_path in input_dir_path:
            return True
    return False


def process_with_prefix(input_file, output_file, prefix):
    input_fd = os.open(input_file, os.O_RDONLY, 0o755)
    output_fd = os.open(output_file, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o755)
    with os.fdopen(input_fd, 'r') as inputfp, os.fdopen(output_fd, 'w') as outputfp:
        for line in inputfp:
            outputfp.write(prefix + line)

        
def handle_files(input_file, output_file, prefix):
    input_fd = os.open(input_file, os.O_RDONLY, 0o755)
    output_fd = os.open(output_file, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o755)
    output_file_dir = os.path.dirname(output_file)

    with os.fdopen(input_fd, "r") as fp, os.fdopen(output_fd, "w") as outputfp:
        lines = fp.readlines()
        for line in lines:
            ll = line.split(";")
            original_js_filename = os.path.basename(ll[0])
            js_fn = replace_js(prefix + ll[0], os.path.join(output_file_dir, original_js_filename))
            if js_fn and os.path.exists(js_fn):
                ll[0] = js_fn
            else:
                ll[0] = prefix + ll[0]
            outputfp.write(";".join(ll))


def replace_merge_file(input_file, output_file, prefix):
    if input_file.endswith(("base.txt", "patch.txt")) and is_file_in_list(prefix):
        handle_files(input_file, output_file, prefix)
    else:
        process_with_prefix(input_file, output_file, prefix)

     
def replace_var(input_file, output_file, prefix):
    if prefix:
        replace_merge_file(input_file, output_file, prefix)
    else:
        replace_js(input_file, output_file)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=str, required=True)
    parser.add_argument("--output", type=str, required=True)
    parser.add_argument("--prefix", type=str)

    args = parser.parse_args()
    replace_var(os.path.abspath(args.input), args.output, args.prefix)


if __name__ == "__main__":
    sys.exit(main())
