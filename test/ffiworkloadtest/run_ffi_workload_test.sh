#!/bin/bash
# Copyright (c) 2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# usage: bash run_ffi_workload_test.sh $run_mode $openharmony_path  $daily_report_folder_path $hdc_machine_ip 
# eg:  bash run_ffi_workload_test.sh 1 /home/codes out 172.24.32.1:8710 
#
set -e
declare -i ret_ok=0
declare -i ret_error=1

function init() {
    LOOP_NUM=5                            # execute workload times
    WORKLOAD_BINARY_NAME='FFIWorkLoadTest'       # workload test binary name
    SO_NAME="libark_jsruntime_test.so"
    CUR_PATH=$(dirname "$(readlink -f "$0")")
    WORK_PATH=$CUR_PATH/tmp/
    LOCAL_XML_PATH=$WORK_PATH/xmls # local xml result files path
}

function check_command_exist() {
    command=$1
    if type "$command" >/dev/null 2>&1; then return $ret_ok; else return $ret_error; fi
}

function check_pip_component() {
    pip3 list | grep "$1"
    return $?
}

function compile_workload_test() {
    if [ ! -f "$WORKLOAD_BINARY" ] || [ ! -f "$WORKLOAD_SO_PATH" ]; then
        echo "enter comple_worklaod_test()"
        "$OPENHARMONY_ROOT_PATH"/build.sh --product-name rk3568 --build-target ark_js_unittest --fast-rebuild > "$WORK_PATH"/test.log
        ret=$(grep "build  successful" "$CUR_PATH"/test.log)
        if [[ "$ret" == *"build successful"* ]];then
            echo "ut build success"
        else
            echo "ut build failed"
        fi
    fi

    if [ ! -f "$WORKLOAD_BINARY" ] || [ ! -f "$WORKLOAD_SO_PATH" ]; then
        echo "ut build fail. please chceck it!"
        return "$ret_error"
    fi

    return "$ret_ok"
}


function push_and_run_test_objs_on_development_board() {
    [ -f "$WORKLOAD_SO_PATH" ] || {
        echo "no libark_jsruntime_test.so, please check it"
        return "$ret_error"
    }
    [ -f "$WORKLOAD_BINARY" ] || {
        echo "no libark_jsruntime_test binary file, please check it"
        return "$ret_error"
    }

    hdc -s "$HDC_MACHINE_IP" shell [ -d "$HDC_TEST_PATH" ] &&  rm -rf "$HDC_TEST_PATH"
    hdc -s "$HDC_MACHINE_IP" shell mkdir -p "$HDC_TEST_PATH"
    hdc -s "$HDC_MACHINE_IP" shell chmod u+x "$HDC_TEST_PATH"

    hdc -s "$HDC_MACHINE_IP" file send "$WORKLOAD_SO_PATH" "$HDC_TEST_PATH"
    hdc -s "$HDC_MACHINE_IP" file send "$WORKLOAD_BINARY" "$HDC_TEST_PATH"

    hdc -s "$HDC_MACHINE_IP" shell chmod u+x "$HDC_TEST_PATH"/"$WORKLOAD_BINARY_NAME"
    hdc -s "$HDC_MACHINE_IP" shell chmod u+x "$HDC_TEST_PATH"/"$SO_NAME"

    # excute FFIWorkLoadTest
    hdc -s "$HDC_MACHINE_IP" shell "cd $HDC_TEST_PATH;./$WORKLOAD_BINARY_NAME;./$WORKLOAD_BINARY_NAME;\
        ./$WORKLOAD_BINARY_NAME;./$WORKLOAD_BINARY_NAME;./$WORKLOAD_BINARY_NAME"

    return "$ret_ok"
}

function get_xml_files() {
    echo "enter get_xml_files"
    for ((i=0;i<"$LOOP_NUM";i++)); do
        if [ "$i" == 0 ]; then
            xml_name="FFIWorkLoadTest"
        else
            xml_name="FFIWorkLoadTest_$i"
        fi
        xml_file_path=$HDC_TEST_PATH/$xml_name".xml"
        echo "xml_file_path: $xml_file_path"
        echo "LOCAL_XML_PATH: $LOCAL_XML_PATH"
        hdc  -s "$HDC_MACHINE_IP" file recv "$xml_file_path" "$LOCAL_XML_PATH"
    done
    return "$ret_ok"
}


function get_report() {
    work_path=$1
    report_path=$2
    python3 ./get_ffi_workload_report.py -w "$work_path"  -o "$report_path"
}


main() {
    init
    para_num=$#
    if [ "$para_num" -lt 4 ]; then
        echo "bad parameters. please check them."
        return "$ret_error"
    fi
    # 1--open network;2-yellow zone(network restricted zone)
    run_mode=$1
    if [[ $run_mode -eq "1" || $run_mode -eq "2" ]];then
        echo "normal parameter run_mode. run_mode: $run_mode"
    else
        return "$ret_error"
    fi

    OPENHARMONY_ROOT_PATH=$2
    if [[ "$3" = /* ]]; then
        FFI_WORKLOAD_TEST_ARCHIVE_PATH=$3
    else
        FFI_WORKLOAD_TEST_ARCHIVE_PATH=$(readlink -f "$3")
    fi
    WORKLOAD_BINARY=$OPENHARMONY_ROOT_PATH/out/rk3568/tests/unittest/arkcompiler/ets_runtime/$WORKLOAD_BINARY_NAME
    WORKLOAD_SO_PATH=$OPENHARMONY_ROOT_PATH/out/rk3568/test/test/$SO_NAME
    sub_path="data/local/tmp/workload_test"
    HDC_TEST_PATH=/"$sub_path"

    if [ "$para_num" -eq 4 ]; then
        HDC_MACHINE_IP=$4
    fi

    if [ -d "$LOCAL_XML_PATH" ];then
        rm -rf "$LOCAL_XML_PATH"
    fi
    mkdir -p "$LOCAL_XML_PATH"
    [ ! -d "$FFI_WORKLOAD_TEST_ARCHIVE_PATH" ] || { mkdir -p "$FFI_WORKLOAD_TEST_ARCHIVE_PATH"; }
    check_command_exist "unzip" || {
        echo "unzip is unavailable, please check it!"
        return "$ret_error"
    }
    check_command_exist "hdc" || { return "$ret_error";}
    check_command_exist python3 || {
        echo "python3 is not available"
        return "$ret_error"
    }
    check_pip_component "openpyxl" || { pip3 install openpyxl; }

    [ -f "$CUR_PATH/get_ffi_workload_report.py" ] || {
        echo "no get_ffi_workload_report.py, please check it"
        return "$ret_error"
    }

 
    # when run_mode is 2, no need compile unittest
    if [ "$run_mode" -eq 1 ];then
        compile_workload_test || {
            echo "fail to compile workload test "
            return "$ret_error"
        }
    fi
    
    push_and_run_test_objs_on_development_board || {
        echo "fail to push and run test objects to development board"
        return "$ret_error"
    }

    get_xml_files || {
        echo "fail to run workload tests in development board"
        return "$ret_error"
    }

    time_str=$(date "+%Y%m%d")
    output_path=$FFI_WORKLOAD_TEST_ARCHIVE_PATH/ffi_workload_daily_report_$time_str".xlsx"

    get_report "$WORK_PATH" "$FFI_WORKLOAD_TEST_ARCHIVE_PATH" || {
        echo "fail to get report"
        return "$ret_error"
    }
    if [ -d "$WORK_PATH" ];then
        rm -rf "$WORK_PATH"
    fi
    if [ -f "$output_path" ];then
        echo "run workload test successfully and get daily report. report path: $output_path"
    else
        echo "run workload test failed."
    fi
}   

main "$@"
exit $?
