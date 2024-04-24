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
declare D8_DEVICE_DIR=""
declare CONFIG_PATH
CONFIG_PATH="$(dirname "${BASH_SOURCE[0]}")"/config.json
declare VER_PLATFORM="full_x86_64"
declare -a BENCH_FILTER=()
declare -a EXCLUDE_BENCHMARKS=()
declare NO_NEED_DOWNLOAD_BENCHS=false
declare BENCH_MULTIPLIER=""
declare HDC_PATH=""
declare TASKSET_MASK=""
declare -i ITERATIONS=1

function echo_fatal()
{
    echo "Error: $1"
    exit 1
}

function init()
{
    while [[ -n "$1" ]]
    do
        case $1 in
            --help|-h)
                usage
                exit 0
                ;;
            --d8-host-path)
                shift
                D8_BINARY_PATH="$1"
                ;;
            --d8-device-dir)
                shift
                D8_DEVICE_DIR="$1"
                ;;
            --platform)
                shift
                VER_PLATFORM="$1"
                ;;
            --config)
                shift
                CONFIG_PATH="$1"
                ;;
            --bench-filter=*)
                local BENCH_FILTER_STR="${1#*=}"
                BENCH_FILTER=(${BENCH_FILTER_STR//:/ })
                ;;
            --exclude=*)
                local EXCLUDE_BENCHS_STR="${1#*=}"
                EXCLUDE_BENCHMARKS=(${EXCLUDE_BENCHS_STR//:/ })
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
            --hdc-path)
                shift
                HDC_PATH="$1"
                ;;
            --taskset)
                shift
                TASKSET_MASK="$1"
                ;;
            *)
                JS_PERF_RESULT_PATH="$1"
                OPENHARMONY_ROOT_PATH="$2"
                break
                ;;
        esac
        shift
    done
    if [[ "${VER_PLATFORM}" == *arm64* ]]
    then
        if [[ -z "${D8_DEVICE_DIR}" ]]
        then
            echo_fatal "--d8-device-dir is required for device"
        fi
        if [[ ! -d "${D8_DEVICE_DIR}" ]]
        then
            echo_fatal "${D8_DEVICE_DIR}: directory with v8 libs for device does not exist"
        fi
        if [[ ! -f "${D8_DEVICE_DIR}/d8" ]]
        then
            echo_fatal "${D8_DEVICE_DIR}/d8 is required, but does not exist"
        fi
        if [[ ! -f "${D8_DEVICE_DIR}/snapshot_blob.bin" ]]
        then
            echo_fatal "${D8_DEVICE_DIR}/snapshot_blob.bin is required, but does not exist"
        fi
        if [[ ! -f "${D8_DEVICE_DIR}/icudtl.dat" ]]
        then
            echo_fatal "${D8_DEVICE_DIR}/icudtl.dat is required, but does not exist"
        fi
        if [[ -z "${HDC_PATH}" ]]
        then
            if ! command -v hdc
            then
                echo_fatal "hdc path was not specified"
            fi
            HDC_PATH=$(command -v hdc)
        elif [[ ! -e "${HDC_PATH}" ]]
        then
            echo_fatal "Executable file does not exist: ${HDC_PATH}"
        else
            HDC_PATH=$(realpath "${HDC_PATH}")
        fi
    elif [[ ! -x "${D8_BINARY_PATH}" ]]
    then
        echo_fatal "Executable file does not exist: ${D8_BINARY_PATH}"
    fi
    if [[ ! -f "${CONFIG_PATH}" ]]
    then
        echo_fatal "Config file does not exist: ${CONFIG_PATH}"
    fi
    CONFIG_PATH=$(realpath "${CONFIG_PATH}")
    if [[ -z "${JS_PERF_RESULT_PATH}" ]]
    then
        usage
        echo_fatal "JS_PERF_RESULT_PATH argument is not set"
    fi
    if [[ -z "${OPENHARMONY_ROOT_PATH}" ]]
    then
        usage
        echo_fatal "OPENHARMONY_ROOT argument is not set"
    fi
    if [[ ! -d "${OPENHARMONY_ROOT_PATH}" ]]
    then
        echo_fatal "Path to openharmony root dir does not exist: ${OPENHARMONY_ROOT_PATH}"
    else
        OPENHARMONY_ROOT_PATH="$(realpath "${OPENHARMONY_ROOT_PATH}")"
    fi
    if [[ ! -d "${JS_PERF_RESULT_PATH}" ]]
    then
        mkdir -p "${JS_PERF_RESULT_PATH}"
    fi
    JS_PERF_RESULT_PATH="$(realpath "${JS_PERF_RESULT_PATH}")"
    WORKDIR_PATH="${JS_PERF_RESULT_PATH}/workdir"
    if [[ -d "${WORKDIR_PATH}" ]]
    then
        rm -rf "${WORKDIR_PATH}"
    fi
    mkdir -p "${WORKDIR_PATH}"
    echo "
OpenHarmony root path: ${OPENHARMONY_ROOT_PATH}
JS perf results: ${JS_PERF_RESULT_PATH}
Config: ${CONFIG_PATH}
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
    if [[ ${#EXCLUDE_BENCHMARKS[@]} -ne 0 ]]
    then
        for bench in "${EXCLUDE_BENCHMARKS[@]}"
        do
            if [[ -e "${JS_TEST_PATH}/${bench}" ]]
            then
                rm -rf "${JS_TEST_PATH}/${bench}"
            else
                echo "No excluding benchmark '${bench}'"
            fi
        done
    fi
}


function get_cores {
    local -i cores_count
    local -i mask=$((16#$1))
    cores_count=$(${HDC_PATH} shell nproc --all)
    for i in $(seq 0 $((cores_count - 1)) )
    do
        if [[ $((mask & 1)) -ne 0 ]]
        then
            cores+=("${i}")
        fi
        mask=$((mask >> 1))
    done
}

function fix_cores_freq()
{
    if [[ -z "${TASKSET_MASK}" ]]
    then
        echo "Taskset mask was not set. Frequency for cores will not be fixed"
        return
    fi
    local -a cores=()
    get_cores "${TASKSET_MASK}"
    echo "Using cores: ${cores[*]}"
    declare -i avg_freq
    declare freqs_str=""
    declare -a freqs_arr=()
    declare -i freq_index=0
    for core in "${cores[@]}"
    do
        freqs_str=$(hdc shell "cat /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_available_frequencies")
        # split string to array
        freqs_arr=(${freqs_str})
        freq_index=$(( ${#freqs_arr[@]} / 2 ))
        avg_freq=${freqs_arr[${freq_index}]}
        ${HDC_PATH} shell "echo performance > /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_governor"
        ${HDC_PATH} shell "echo ${avg_freq} > /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_min_freq"
        ${HDC_PATH} shell "echo ${avg_freq} > /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_max_freq"
    done
    echo "Cores info after frequencies fix:"
    for core in "${cores[@]}"
    do
        echo "Core ${core} info:"
        echo "Governor: $(${HDC_PATH} shell "cat /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_governor")"
        echo "Min freq: $(${HDC_PATH} shell "cat /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_min_freq")"
        echo "Max freq: $(${HDC_PATH} shell "cat /sys/devices/system/cpu/cpu${core}/cpufreq/scaling_max_freq")"
    done
}

function prepare_device()
{
    # Check device
    local -a devices
    devices=$(${HDC_PATH} list targets)
    if [[ ${#devices[@]} -eq 1 ]]
    then
        if [[ "${devices[0]}" == "[Empty]"* ]]
        then
            echo_fatal "No devices"
        fi
        echo "Device id: ${devices[0]}"
    else
        echo_fatal "Required immidieatly one device"
    fi
    # Create workdir
    local -r WORKDIR_ON_DEVICE="/data/local/tmp/jsperftest"
    D8_BINARY_PATH="${WORKDIR_ON_DEVICE}/v8/d8"
    set -x
    ${HDC_PATH} shell "rm -rf ${WORKDIR_ON_DEVICE}"
    ${HDC_PATH} shell "mkdir -p ${WORKDIR_ON_DEVICE}/v8 ${WORKDIR_ON_DEVICE}/lib ${WORKDIR_ON_DEVICE}/thirdparty"
    ${HDC_PATH} file send "${D8_DEVICE_DIR}"/d8 ${D8_BINARY_PATH}
    ${HDC_PATH} shell chmod u+x "${D8_BINARY_PATH}"
    ${HDC_PATH} file send "${D8_DEVICE_DIR}"/snapshot_blob.bin ${WORKDIR_ON_DEVICE}/v8
    ${HDC_PATH} file send "${D8_DEVICE_DIR}"/icudtl.dat ${WORKDIR_ON_DEVICE}/v8
    set +x
    fix_cores_freq
}

function usage()
{
    echo "${SCRIPT_NAME} [options] <JS_PERF_RESULT_PATH> <OPENHARMONY_ROOT>
Options:
    --d8-host-path <path>  - path to d8 binary file.
                             Default: /usr/bin/v8/d8
    --d8-device-dir <path> - path to dir with d8 for ohos device.
                             See https://gitee.com/qishui7/ohcompiler-daily/repository/archive/master.zip
    --hdc-path <hdc_path>  - path to hdc executable file. Use from PATH env variable by default
    --platform <platform>  - used platform. Possible values in config.json.
                             Default: full_x86_64
    --config <config_path> - path to specific json config
    --taskset mask         - use CPU affinity with mask for benchmark runnings.
                             Frequency of these cores will be fixed
    --multiplier N         - iteration multiplier for js benchmarks
    --iterations N         - number of benchmark launches and get average
    --bench-filter=BenchDir1:BenchDir2:BenchDir3/bench.js:...
                           - filter for benchmarks: directory or file
    --exclude=BenchDir1:BenchDir2:BenchDir3/bench.js:...
                           - exclude benchmarks from running: directory or file
    --no-download-benchs   - no download benchmarks from repository if repo already exists
    --help, -h             - print help info about script

Positional arguments:
    JS_PERF_RESULT_PATH - directory path to benchmark results
    OPENHARMONY_ROOT    - path to root directory for ark_js_vm and es2panda
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

    if [[ "${VER_PLATFORM}" == *arm64* ]]
    then
        prepare_device
    fi
   
    prepare_js_test_files || { return $ret_error; }

    cd "${WORKDIR_PATH}"
    if [[ -n "${BENCH_MULTIPLIER}" ]]
    then
        for js_test in $(find ${JS_TEST_PATH} -name "*js")
        do
            python3 "${cur_path}"/prerun_proc.py __MULTIPLIER__="${BENCH_MULTIPLIER}" "${js_test}"
        done
    fi
    mkdir -p "${WORKDIR_PATH}/tmp"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
    local -i benchs_time_start benchs_time benchs_minutes
    benchs_time_start=$(date +%s)
    python3  "${cur_path}"/run_js_test.py \
                        -c "${CONFIG_PATH}" \
                        -bp "${OPENHARMONY_ROOT_PATH}" \
                        -p "${JS_TEST_PATH}" \
                        -o "${JS_PERF_RESULT_PATH}" \
                        -v "${D8_BINARY_PATH}" \
                        -e "${VER_PLATFORM}" \
                        --hdc "${HDC_PATH}" \
                        -t "${TASKSET_MASK}" \
                        -n "${ITERATIONS}"
    benchs_time=$(($(date +%s) - benchs_time_start))
    benchs_minutes=$((benchs_time / 60))
    echo "Benchmark script time: ${benchs_minutes} min $((benchs_time - benchs_minutes * 60)) sec"

    if [[ "${VER_PLATFORM}" == *arm64* && -n "${TASKSET_MASK}" ]]
    then
        echo "Reboot device after fix frequencies for restoring of default values"
        ${HDC_PATH} shell reboot
    fi
}

main "$@"
exit $?
