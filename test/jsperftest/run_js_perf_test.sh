#!/bin/bash
# Copyright (c) 2023 Huawei Device Co., Ltd.
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

set -e

declare -i ret_error=1

function init()
{
    CUR_PATH=$(dirname "$(readlink -f "$0")")
    TMP_PATH=$CUR_PATH/tmp
}

function check_command_exist()
{
    command=$1
    type "$command" >/dev/null 2>&1
    echo  $?
}

function check_pip_component()
{
    pip3 list|grep "$1"
    return $?
}

function download_js_test_files()
{
    code_path="$TMP_PATH"/code/arkjs-perf-test
    if [ -d "$code_path" ];then
        JS_TEST_PATH=$code_path/js-perf-test
        return
    fi

    mkdir -p "$code_path"
    echo "$code_path"
    git clone -b builtins_test1110 https://gitee.com/dov1s/arkjs-perf-test.git "$code_path"
    JS_TEST_PATH=$code_path/js-perf-test
}

main() 
{
    init
    js_perf_test_archive_path=$1
    OPENHARMONY_OUT_PATH=$2
    D8_BINARY_PATH=$3
    VER_PLATFORM="full_x86_64"
    if [ $# == 4 ]; then
        VER_PLATFORM=$4
    fi
    cur_path=$(dirname "$(readlink -f "$0")")
    
    if [ ! -d "$js_perf_test_archive_path" ];then
        mkdir -p "js_perf_test_archive_path"
    fi

    check_command_exist git || { echo "git is not available"; return $ret_error; }
    check_command_exist unzip || { echo "unzip is not available"; return $ret_error; }
    check_command_exist jq || { echo "jq is not available"; return $ret_error; }
    check_command_exist python3 || { echo "python3 is not available"; return $ret_error; }
    check_pip_component "openpyxl"  || { pip3 install openpyxl; }
    
    [ -f "$cur_path/run_js_test.py" ] || { echo "no run_js_test.py, please check it";return $ret_error;}
   
    download_js_test_files || { return $ret_error; } 

    echo "LD_LIBRARY_PATH:$LD_LIBRARY_PATH"
    python3  "$cur_path"/run_js_test.py -bp "$OPENHARMONY_OUT_PATH" -p "$JS_TEST_PATH" -o "$js_perf_test_archive_path"\
        -v "$D8_BINARY_PATH" -e "$VER_PLATFORM"
}

main "$@"
exit $?
