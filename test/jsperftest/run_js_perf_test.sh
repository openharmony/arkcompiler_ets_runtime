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
#!/bin/bash

declare -i ret_ok=0
declare -i ret_error=1

OPENHARMONY_OUT_PATH="/root" # refer to openharmony out folder
CUR_PATH=$(dirname $(readlink -f "$0"))
TMP_PATH=$CUR_PATH/tmp
# JS_TEST_PATH配置说明：
# 默认为空，不配置，则会下载用例工程https://gitee.com/dov1s/arkjs-perf-test.git ，取builtins_test1110分支用例进行测试。
JS_TEST_PATH=""     
ETS_RUNTIME_PATH=""
ICU_PATH=""
ZLIB_PATH=""
LIB_PATH=""


function check_command_exist()
{
    command=$1
    if type "$command" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

function check_pip_component()
{
    pip3 list|grep "$1"
    return $?
}

function check_fangzhou_binary_path()
{
    ETS_RUNTIME_PATH="$OPENHARMONY_OUT_PATH/out/rk3568/clang_x64/arkcompiler/ets_runtime/"
    ICU_PATH="$OPENHARMONY_OUT_PATH/out/rk3568/clang_x64/thirdparty/icu/"
    ZLIB_PATH="$OPENHARMONY_OUT_PATH/out/rk3568/clang_x64/thirdparty/zlib/"
    LIB_PATH="$OPENHARMONY_OUT_PATH/prebuilts/clang/ohos/linux-x86_64/llvm/lib/"
    [ -d $ETS_RUNTIME_PATH ] || { echo "wrong  ets_runtime folder path: $ETS_RUNTIME_PATH, please check it ";return $ret_error; }
    [ -d $ICU_PATH ] || { echo "wrong icu folder path: $ICU_PATH, please check";return $ret_error; }
    [ -d $ZLIB_PATH ] || { echo "wrong zlib folder path: $ZLIB_PATH, please check it";return $ret_error; }
    [ -d $LIB_PATH ] || { echo "wrong lib folder path: $LIB_PATH, please check it";return $ret_error; }          
}


function export_fangzhou_env
{
    export LD_LIBRARY_PATH=$ETS_RUNTIME_PATH
    export LD_LIBRARY_PATH=$ICU_PATH:$LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=$ZLIB_PATH:$LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=$LIB_PATH:$LD_LIBRARY_PATH
}


function download_js_test_files()
{
    code_path="$TMP_PATH"/code/arkjs-perf-test
    if [ -d $code_path ];then
        # rm -rf $code_path
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
    para_num=$#
    cur_path=$(dirname $(readlink -f "$0"))
    archive_folder_name="js_perf_test_archive"
    
    if [ $para_num -eq 1 ];then
        js_perf_test_archive_path=$1
    elif [ $para_num -gt 1 ];then
        echo "At most one parameterr, please check it\n"
        return 1
    else
        js_perf_test_archive_path=$cur_path/$archive_folder_name
    fi

    if [ ! -d "$js_perf_test_archive_path" ];then
        mkdir -p "js_perf_test_archive_path"
    fi

    # execute cmd format: bash_test.sh
    check_command_exist git || { echo "git is not available"; return $ret_error; }
    check_command_exist unzip || { echo "unzip is not available"; return $ret_error; }
    check_command_exist python3 || { echo "python3 is not available"; return $ret_error; }
    check_fangzhou_binary_path
    check_pip_component "openpyxl"  || { pip3 install openpyxl; }

    export_fangzhou_env
    
    [ -f "$cur_path/run_js_test.py" ] || { echo "no run_js_test.py, please check it";return $ret_error;}
   
    download_js_test_files || { return $ret_error; } 

    # time_str=$(date "+%Y-%m-%d_%H-%M-%S")
    # output_path=$TMP_PATH/test_result_$time_str
    # mkdir -p "$output_path"
    echo "LD_LIBRARY_PATH:$LD_LIBRARY_PATH"
    python3  "$cur_path"/run_js_test.py -bp $OPENHARMONY_OUT_PATH -p "$JS_TEST_PATH" -o "$js_perf_test_archive_path"
    # rm -rf "$codes_path"
}

main "$@"
exit $?
