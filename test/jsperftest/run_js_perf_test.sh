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
declare SCRIPT_NAME
SCRIPT_NAME="$(basename "$0")"
declare JS_PERF_RESULT_PATH=""
declare OPENHARMONY_ROOT_PATH=""
declare D8_BINARY_PATH="/usr/bin/v8/d8"
declare VER_PLATFORM="full_x86_64"
declare -a BENCH_FILTER=()
declare NO_NEED_DOWNLOAD_BENCHS=false
declare BENCH_MULTIPLIER=""
declare -i ITERATIONS=1

function init()
{
    while [[ -n "$1" ]]
    do
        case $1 in
            --help|-h)
                usage
                exit 0
                ;;
            --d8-path)
                shift
                D8_BINARY_PATH="$1"
                ;;
            --platform)
                shift
                VER_PLATFORM="$1"
                ;;
            --bench-filter=*)
                local BENCH_FILTER_STR="${1#*=}"
                BENCH_FILTER=(${BENCH_FILTER_STR//:/ })
                ;;
            --no-download-benchs)
                NO_NEED_DOWNLOAD_BENCHS=true
                ;;
            --multiplier)
                shift
                BENCH_MULTIPLIER=$1
                ;;
            --iterations)
                shift
                ITERATIONS=$1
                ;;
            *)
                JS_PERF_RESULT_PATH="$(realpath "$1")"
                OPENHARMONY_ROOT_PATH="$2"
                break
                ;;
        esac
        shift
    done
    if [[ -z "${OPENHARMONY_ROOT_PATH}" || -z "${JS_PERF_RESULT_PATH}" ]]
    then
        usage
        echo "Invalid input arguments"
        exit 1
    fi
    if [[ ! -d "${OPENHARMONY_ROOT_PATH}" ]]
    then
        echo "Path to openharmony root dir does not exist: ${OPENHARMONY_ROOT_PATH}"
        exit 1
    else
        OPENHARMONY_ROOT_PATH="$(realpath "${OPENHARMONY_ROOT_PATH}")"
    fi
    if [[ ! -x "${D8_BINARY_PATH}" ]]
    then
        echo "Executable file does not exist: ${D8_BINARY_PATH}"
        exit 1
    fi
    if [[ ! -d "${JS_PERF_RESULT_PATH}" ]]
    then
        mkdir -p "${JS_PERF_RESULT_PATH}"
    fi
    WORKDIR_PATH="${JS_PERF_RESULT_PATH}/workdir"
    if [[ -d "${WORKDIR_PATH}" ]]
    then
        rm -rf "${WORKDIR_PATH}"
    fi
    mkdir -p "${WORKDIR_PATH}"
    echo "
OpenHarmony root path: ${OPENHARMONY_ROOT_PATH}
D8 path: ${D8_BINARY_PATH}
JS perf results: ${JS_PERF_RESULT_PATH}
Platform: ${VER_PLATFORM}
"
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

function prepare_js_test_files()
{
    local -r bench_repo_path="${JS_PERF_RESULT_PATH}/code/arkjs-perf-test"
    if ! ${NO_NEED_DOWNLOAD_BENCHS}
    then
        rm -rf "${bench_repo_path}"
    fi
    if [[ ! -d "${bench_repo_path}" ]]
    then
        mkdir -p "${bench_repo_path}"
        git clone -b builtins_test1110 https://gitee.com/dov1s/arkjs-perf-test.git "${bench_repo_path}"
    fi
    local -r test_dir="js-perf-test"
    JS_TEST_PATH="${JS_PERF_RESULT_PATH}/${test_dir}"
    if [[ -d "${JS_TEST_PATH}" ]]
    then
        rm -rf "${JS_TEST_PATH}"
    fi
    mkdir -p "${JS_TEST_PATH}"
    if [[ ${#BENCH_FILTER[@]} -eq 0 ]]
    then
        cp -r "${bench_repo_path}/${test_dir}"/* "${JS_TEST_PATH}"
    else
        for bench in "${BENCH_FILTER[@]}"
        do
            if [[ -d "${bench_repo_path}/${test_dir}/${bench}" ]]
            then
                mkdir -p "${JS_TEST_PATH}/${bench}"
                cp -r "${bench_repo_path}/${test_dir}/${bench}"/* "${JS_TEST_PATH}/${bench}"
            elif [[ -f "${bench_repo_path}/${test_dir}/${bench}" ]]
            then
                mkdir -p "$(dirname "${JS_TEST_PATH}/${bench}")"
                cp "${bench_repo_path}/${test_dir}/${bench}" "${JS_TEST_PATH}/${bench}"
            else
                echo "No benchmarks for filter '${bench}'"
            fi
        done
    fi
}

function usage()
{
    echo "${SCRIPT_NAME} [options] <JS_PERF_RESULT_PATH> <OPENHARMONY_ROOT>
Options:
    --d8-path <path>      - path to d8 binary file.
                            Default: /usr/bin/v8/d8
    --platform <platform> - used platform. Possible values in config.json.
                            Default: full_x86_64
    --multiplier N        - iteration multiplier for js benchmarks
    --iterations N        - number of benchmark launches and get average
    --bench-filter=BenchDir1:BenchDir2:BenchDir3/bench.js:...
                          - filter for benchmarks: directory or file
    --no-download-benchs  - no download benchmarks from repository if repo already exists
    --help, -h            - print help info about script

Positional arguments:
    JS_PERF_RESULT_PATH - directory path to benchmark results
    OPENHARMONY_ROOT - path to root directory for ark_js_vm and es2panda
"
}

main() 
{
    init "$@"
    cur_path=$(dirname "$(readlink -f "$0")")

    check_command_exist git || { echo "git is not available"; return $ret_error; }
    check_command_exist unzip || { echo "unzip is not available"; return $ret_error; }
    check_command_exist jq || { echo "jq is not available"; return $ret_error; }
    check_command_exist python3 || { echo "python3 is not available"; return $ret_error; }
    check_pip_component "openpyxl"  || { pip3 install openpyxl; }
    
    [ -f "$cur_path/run_js_test.py" ] || { echo "no run_js_test.py, please check it";return $ret_error;}
   
    prepare_js_test_files || { return $ret_error; }

    cd "${WORKDIR_PATH}"
    if [[ -n ${BENCH_MULTIPLIER} ]]
    then
        for js_test in $(find ${JS_TEST_PATH} -name "*js")
        do
            python3 ${cur_path}/prerun_proc.py __MULTIPLIER__=${BENCH_MULTIPLIER} "${js_test}"
        done
    fi
    mkdir -p "${WORKDIR_PATH}/tmp"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
    python3  "${cur_path}"/run_js_test.py \
                        -bp "${OPENHARMONY_ROOT_PATH}" \
                        -p "${JS_TEST_PATH}" \
                        -o "${JS_PERF_RESULT_PATH}" \
                        -v "${D8_BINARY_PATH}" \
                        -e "${VER_PLATFORM}" \
                        -n "${ITERATIONS}"
}

main "$@"
exit $?
