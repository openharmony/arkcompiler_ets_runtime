#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Copyright (c) 2021 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: Use ark to execute ts/js files
"""

import os
import sys
import re
import glob
import argparse
import subprocess
import signal
import time
import json

DEFAULT_TIMEOUT = 300
DEFAULT_PGO_THRESHOLD = 10
TARGET_PLATFORM = ['x64', 'arm64']
PRODUCT_LIST = ['hispark_taurus', 'rk3568', 'baltimore']
TARGET_PRODUCT_MAP = {'x64': 0, 'arm64': 1}

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('name', metavar='file|path', type=str, help='test case name: file or path')
    parser.add_argument('-a', '--all', action='store_true', help='run all test cases on path')
    parser.add_argument('-p', '--product', metavar='name',
        help='product name, default is hispark_taurus on x64, rk3568 on arm64')
    parser.add_argument('-t', '--tool', metavar='opt',
        help='run tool supported opt: aot, int(c interpreter tool), asmint(asm interpreter tool)')
    parser.add_argument('-s', '--step', metavar='opt',
        help='run step supported opt: abc, pack, aot, aotd, run, rund, asmint, asmintd, int, intd')
    parser.add_argument('-d', '--debug', action='store_true', help='run on debug mode')
    parser.add_argument('--arm64', action='store_true', help='run on arm64 platform')
    parser.add_argument('--aot-args', metavar='args', help='pass to aot compiler args')
    parser.add_argument('--jsvm-args', metavar='args', help='pass to jsvm args')
    parser.add_argument('-i', '--info', action='store_true', help='add log level of info to args')
    parser.add_argument('-c', '--clean', action='store_true', help='clean output files')
    parser.add_argument('--npm', action='store_true', help='npm install')
    parser.add_argument('--bt', dest='builtin', action='store_true', help='aot compile with lib_ark_builtins.d.ts')
    parser.add_argument('--pgo', action='store_true',
        help=f'aot compile with pgo, default threshold is {DEFAULT_PGO_THRESHOLD}')
    parser.add_argument('--pgo-th', metavar='n', default=DEFAULT_PGO_THRESHOLD, type=int,
        help=f'pgo hotness threshold, default is {DEFAULT_PGO_THRESHOLD}')
    parser.add_argument('--timeout', metavar='n', default=DEFAULT_TIMEOUT, type=int,
        help=f'specify seconds of test timeout, default is {DEFAULT_TIMEOUT}')
    parser.add_argument('-e', '--env', action='store_true', help='print LD_LIBRARY_PATH')
    arguments = parser.parse_args()
    return arguments

def run_command(cmd, timeout=DEFAULT_TIMEOUT):
    proc = subprocess.Popen(cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE, shell=True)
    code_format = 'UTF-8'
    try:
        (msg, errs) = proc.communicate(timeout=timeout)
        ret_code = proc.poll()
        if errs:
            ret_code = 2
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.terminate()
        os.kill(proc.pid, signal.SIGTERM)
        return (1, '', f'exec command timeout {timeout}s')
    return (ret_code, msg.decode(code_format), errs.decode(code_format))

def match_list_name(list, name):
    for str in list:
        found = str.find(name)
        if (found == 0):
            return str
    return ''

class ArkTest():
    def __init__(self, args):
        self.args = args
        self.self_dir = os.path.abspath(sys.argv[0])
        self.hap_abc = 'ets/modules.abc'
        self.place_dir = 'arkcompiler/ets_runtime/test'
        if self.self_dir.find(self.place_dir) < 0:
            print(f'pls place this script at: {self.place_dir}')
            sys.exit(1)

        self.ohdir = os.path.abspath(f'{self.self_dir}/../../../..')
        self.product = PRODUCT_LIST[TARGET_PRODUCT_MAP['x64']]
        self.builtin = ''
        if args.builtin:
            self.builtin = f'{self.ohdir}/arkcompiler/ets_runtime/ecmascript/ts_types/lib_ark_builtins.d'
        self.arm64 = False
        if args.step == 'hap':
            self.arm64 = True
        if args.arm64:
            self.product = PRODUCT_LIST[TARGET_PRODUCT_MAP['arm64']]
            self.arm64 = True
        if args.product:
            self.product = match_list_name(PRODUCT_LIST, args.product)
        self.step = 'all'
        if args.step:
            self.step = args.step
        if args.clean:
            self.step = 'clean'
        self.expect = 'expect_output.txt'
        self.types = {'all': ['.ts', '.js'],
                      'abc': ['.ts', '.js'],
                      'pack': ['.an'],
                      'aot': ['.abc'],
                      'aotd': ['.abc'],
                      'run': ['.abc'],
                      'rund': ['.abc'],
                      'asmint': ['.abc'],
                      'asmintd': ['.abc'],
                      'int': ['.abc'],
                      'intd': ['.abc'],
                      'clean': ['.abc']}

        product_dir = f'{self.ohdir}/out/{self.product}'
        libs_dir_x64_release = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib:'
                                f'{product_dir}/clang_x64/arkcompiler/ets_runtime:'
                                f'{product_dir}/clang_x64/thirdparty/icu')
        libs_dir_x64_debug = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib:'
                              f'{product_dir}/clang_x64/exe.unstripped/clang_x64/arkcompiler/ets_runtime:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/arkcompiler/ets_runtime:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/test/test:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/thirdparty/icu')
        libs_dir_arm64_release = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:'
                                  f'{product_dir}/arkcompiler/ets_runtime/:'
                                  f'{product_dir}/utils/utils_base/:'
                                  f'{product_dir}/thirdparty/icu:'
                                  f'{product_dir}/common/dsoftbus/:'
                                  f'{product_dir}/commonlibrary/c_utils:'
                                  f'{product_dir}/systemabilitymgr/samgr:'
                                  f'{product_dir}/hiviewdfx/hisysevent_native:'
                                  f'{product_dir}/common/common:'
                                  f'{product_dir}/securec/thirdparty_bounds_checking_function:'
                                  f'{product_dir}/hiviewdfx/faultloggerd:'
                                  f'{product_dir}/thirdparty/bounds_checking_function:'
                                  f'{product_dir}/hiviewdfx/hilog_native:'
                                  f'{product_dir}/startup/init:'
                                  f'{product_dir}/thirdparty/cjson:'
                                  f'{product_dir}/lib.unstripped/common/dsoftbus:'
                                  f'{product_dir}/security/selinux:'
                                  f'{product_dir}/hiviewdfx/hitrace_native/:'
                                  f'{product_dir}/communication/ipc/:'
                                  f'{product_dir}/distributedschedule/samgr_standard:'
                                  f'{product_dir}/security/access_token:'
                                  f'{product_dir}/communication/dsoftbus:'
                                  f'{product_dir}/startup/startup_l2/:'
                                  f'{product_dir}/security/huks/:'
                                  f'{product_dir}/clang_x64/thirdparty/icu/:'
                                  f'{product_dir}/clang_x64/arkcompiler/ets_runtime')
        libs_dir_arm64_debug = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:'
                                f'{product_dir}/lib.unstripped/arkcompiler/ets_runtime/:'
                                f'{product_dir}/utils/utils_base/:'
                                f'{product_dir}/thirdparty/icu:'
                                f'{product_dir}/common/dsoftbus/:'
                                f'{product_dir}/commonlibrary/c_utils:'
                                f'{product_dir}/systemabilitymgr/samgr:'
                                f'{product_dir}/hiviewdfx/hisysevent_native:'
                                f'{product_dir}/common/common:'
                                f'{product_dir}/securec/thirdparty_bounds_checking_function:'
                                f'{product_dir}/hiviewdfx/faultloggerd:'
                                f'{product_dir}/thirdparty/bounds_checking_function:'
                                f'{product_dir}/hiviewdfx/hilog_native:'
                                f'{product_dir}/startup/init:'
                                f'{product_dir}/thirdparty/cjson:'
                                f'{product_dir}/security/selinux:'
                                f'{product_dir}/hiviewdfx/hitrace_native/:'
                                f'{product_dir}/communication/ipc/:'
                                f'{product_dir}/distributedschedule/samgr_standard:'
                                f'{product_dir}/security/access_token:'
                                f'{product_dir}/communication/dsoftbus:'
                                f'{product_dir}/startup/startup_l2/:'
                                f'{product_dir}/security/huks/:'
                                f'{product_dir}/clang_x64/thirdparty/icu/:'
                                f'{product_dir}/clang_x64/arkcompiler/ets_runtime')
        libs_dir = [[libs_dir_x64_release, libs_dir_x64_debug], [libs_dir_arm64_release, libs_dir_arm64_debug]]
        bins_dir = [['clang_x64/arkcompiler', 'clang_x64/exe.unstripped/clang_x64/arkcompiler'],
                    ['arkcompiler', 'exe.unstripped/arkcompiler']]
        icu_arg = f'--icu-data-path={self.ohdir}/third_party/icu/ohos_icu4j/data'
        self.libs_dir = libs_dir[self.arm64][args.debug]
        self.compiler = f'{product_dir}/{bins_dir[0][args.debug]}/ets_runtime/ark_aot_compiler'
        self.jsvm = f'{product_dir}/{bins_dir[self.arm64][args.debug]}/ets_runtime/ark_js_vm'
        self.ts2abc = f'{product_dir}/clang_x64/arkcompiler/ets_frontend/build/src/index.js'
        self.builtin
        self.aot_args = ''
        self.jsvm_args = icu_arg
        if self.builtin:
            self.aot_args = f'{self.aot_args} --builtins-dts={self.builtin}.abc'
        self.pgo = False
        if args.pgo:
            self.pgo = True
            self.aot_args = (f'{self.aot_args} --enable-pgo-profiler=true --pgo-hotness-threshold={args.pgo_th}'
                             f' --pgo-profiler-path=pgo_file_name.ap')
        if args.aot_args:
            self.aot_args = f'{self.aot_args} {args.aot_args}'
        if args.jsvm_args:
            self.jsvm_args = f'{self.jsvm_args} {args.jsvm_args}'
        if args.info:
            self.aot_args = f'{self.aot_args} --log-level=info'
            self.jsvm_args = f'{self.jsvm_args} --log-level=info'
        self.runner = ''
        self.runnerd = 'gdb --args'
        if self.arm64:
            if self.step[:3] != 'aot':
                self.runner = 'qemu-aarch64'
                self.runnerd = 'qemu-aarch64 -cpu max,sve=off -g 123456'
            self.aot_args = f'{self.aot_args} --target-triple=aarch64-unknown-linux-gnu'
        self.test_count = 0
        self.fail_cases = []
        os.environ['LD_LIBRARY_PATH'] = self.libs_dir
        if args.env:
            print(f'export LD_LIBRARY_PATH={self.libs_dir}')
            sys.exit(0)
        if args.npm:
            index_dir = os.path.dirname(self.ts2abc)
            os.system(f'cd {index_dir}/.. && npm install')
            sys.exit(0)

    def run_cmd(self, cmd):
        print(cmd)
        ret = run_command(cmd, self.args.timeout)
        if ret[0]:
            print(ret[2])
        return ret

    @staticmethod
    def get_module_name(hap_dir):
        with open(f'{hap_dir}/module.json') as f:
            data = json.load(f)
        if len(data):
            return data['module']['name']
        else:
            return 'entry'

    def run_test(self, file):
        self.test_count += 1
        basename = os.path.basename(f'{file}')
        type = os.path.splitext(basename)[-1]
        name = os.path.splitext(basename)[0]
        dir = os.path.dirname(file)
        out_case_dir = f'{dir}'
        hap_dir = 'null'
        hap_name = 'null'
        module_name = 'null'
        if self.step == 'hap' or self.step == 'pack':
            hap_dir = os.path.abspath(f'{out_case_dir}/..')
            hap_name = os.path.basename(hap_dir)
            module_name = self.get_module_name(hap_dir)
        abc_file = f'{os.path.splitext(file)[0]}.abc'
        if self.pgo:
            pgo_file = f'{hap_dir}/ap/{module_name}'
            self.aot_args = self.aot_args.replace('pgo_file_name', pgo_file)
        cmd_map = {
            'abc': f'node --expose-gc {self.ts2abc} {file} --merge-abc',
            'pack': [f'mkdir -p {out_case_dir}/../an/arm64-v8a',
                     f'mv {out_case_dir}/{name}.an {hap_dir}/an/arm64-v8a/{module_name}.an',
                     f'mv {out_case_dir}/{name}.ai {hap_dir}/an/arm64-v8a/{module_name}.ai',
                     f'cd {out_case_dir}/.. && rm -f ../{hap_name}.hap && zip -r -q ../{hap_name}.hap *',
                     f'mv {hap_dir}/an/arm64-v8a/{module_name}.an {out_case_dir}/{name}.an',
                     f'mv {hap_dir}/an/arm64-v8a/{module_name}.ai {out_case_dir}/{name}.ai',
                     f'rm -rf {hap_dir}/an'],
            'aot': f'{self.compiler} {self.aot_args} --aot-file={out_case_dir}/{name} {abc_file}',
            'aotd': f'{self.runnerd} {self.compiler} {self.aot_args} --aot-file={out_case_dir}/{name} {abc_file}',
            'run': f'{self.runner} {self.jsvm} {self.jsvm_args} --aot-file={out_case_dir}/{name} --entry-point={name} {abc_file}',
            'rund': f'{self.runnerd} {self.jsvm} {self.jsvm_args} --aot-file={out_case_dir}/{name} --entry-point={name} {abc_file}',
            'asmint': f'{self.runner} {self.jsvm} {self.jsvm_args} --entry-point={name} {abc_file}',
            'asmintd': f'{self.runnerd} {self.jsvm} {self.jsvm_args} --entry-point={name} {abc_file}',
            'int': f'{self.runner} {self.jsvm} {self.jsvm_args} --asm-interpreter=0 --entry-point={name} {abc_file}',
            'intd': f'{self.runnerd} {self.jsvm} {self.jsvm_args} --asm-interpreter=0 --entry-point={name} {abc_file}',
            'clean': f'rm -f {out_case_dir}/{name}.abc {out_case_dir}/{name}.an {out_case_dir}/{name}.ai',
            'cleanhap': f'rm -rf {hap_dir}/an {out_case_dir}/{name}.an {out_case_dir}/{name}.ai'}
        if self.builtin:
            cmd = f'node --expose-gc {self.ts2abc} {self.builtin}.ts -m --merge-abc -q -b'
            print(cmd)
            os.system(cmd)
        if self.step == 'hap':
            self.step = 'aot'
            perf_start = time.perf_counter()
            cmd = cmd_map[self.step]
            print(cmd)
            os.system(cmd)
            perf_end = time.perf_counter()
            abc_size = os.path.getsize(file) / 1024
            an_size = os.path.getsize(f'{out_case_dir}/{name}.an') / 1024
            print(f'test: {file}  abc_size: {abc_size: .1f}KB  an_size: {an_size:.1f}KB  '
                  f'expand: {an_size / abc_size: .1f}  time: {perf_end - perf_start: .1f}s')
            self.step = 'pack'
        if self.step == 'pack':
            for cmd in cmd_map[self.step]:
                print(cmd)
                os.system(cmd)
            print(f'packed hap: {hap_name}.hap')
            return
        if self.step == 'clean':
            if os.path.isfile(f'{hap_dir}/{self.hap_abc}'):
                self.step = 'cleanhap'
        if self.step != 'all':
            # gdb should use the os.system
            cmd = cmd_map[self.step]
            print(cmd)
            if self.arm64 and self.step[-1:] == 'd' and self.step[:3] != 'aot':
                print(f'gdb-client start:   gdb-multiarch {self.jsvm}')
                print(f'gdb-server connect: target remote:123456')
            os.system(cmd)
            return
        ret = self.run_cmd(cmd_map['abc'])
        if ret[0]:
            self.judge_test(file, ret)
            return
        if (not self.args.tool) or (self.args.tool == 'aot'):
            ret = self.run_cmd(cmd_map['aot'])
            if ret[0] and ret[2].find('aot compile success') < 0:
                self.judge_test(file, ret)
                return
            ret = self.run_cmd(cmd_map['run'])
        else:
            ret = self.run_cmd(cmd_map[self.args.tool])
        self.judge_test(file, ret)

    def judge_test(self, file, out):
        if out[0]:
            self.fail_cases.append(file)
            print_fail(f'FAIL: {file}')
            return
        expect_file = f'{os.path.dirname(file)}/{self.expect}'
        if os.path.exists(expect_file):
            with open(expect_file, mode='r') as infile:
                expect = ''.join(infile.readlines()[13:])
            if out[1] != expect:
                self.fail_cases.append(file)
                print(f'expect: [{expect}]\nbut got: [{out[1]}]')
                print_fail(f'FAIL: {file}')
            else:
                print_pass(f'PASS: {file}')
        else:
            print_pass(f'PASS: {file}')
            print(out[1])

    def report_test(self):
        fail_count = len(self.fail_cases)
        print(f'Ran tests: {self.test_count}')
        print(f'Ran failed: {fail_count}')
        if fail_count == 0:
            print_pass('================================== All tests Run PASSED!')
            return
        print_fail('==================================')
        for case in self.fail_cases:
            print(case)
        print_fail('==================================')

    def find_file(self, dir, postfix_list):
        result = []
        for root, lists, files in os.walk(dir):
            for file in files:
                for postfix in postfix_list:
                    path = os.path.join(root, file)
                    found = path.find(postfix)
                    if found == len(path) - len(postfix):
                        result.append(path)
        if os.path.isfile(dir):
            for postfix in postfix_list:
                found = dir.find(postfix)
                if found == len(dir) - len(postfix):
                    result.append(dir)
                    break
        return result

    def test_hap(self):
        if self.step != 'all':
            return 1
        files = self.find_file(self.args.name, [self.hap_abc, '.hap'])
        if len(files):
            self.step = 'hap'
            file = files[0]
            type = os.path.splitext(file)[-1]
            if type == '.hap':
                hap_dir = f'{os.path.splitext(file)[0]}.aot'
                os.system(f'mkdir -p {hap_dir} && unzip -o -q {file} -d {hap_dir}')
                file = f'{hap_dir}/{self.hap_abc}'
            self.run_test(file)
            return 0
        return 1

    def test(self):
        # run single test by name
        files = []
        if self.step not in self.types:
            print(f'not supported step: {self.step}')
            return 1
        if not self.args.all:
            files = self.find_file(self.args.name, self.types[self.step])
            if len(files):
                self.run_test(files[0])
            elif self.test_hap():
                print(f'input path no test case: {self.args.name}')
                return 1
            return 0

        # run all test in path
        if not os.path.isdir(self.args.name):
            print(f'input path not exists or is file: {self.args.name}')
            return 1
        files = self.find_file(self.args.name, self.types[self.step])
        for test in files:
            self.run_test(test)

        if len(files) == 0:
            self.test_hap()

        if self.step == 'clean':
            print('clean output files finished')
            return 0

        if self.test_count == 0:
            print(f'input path no test case: {self.args.name}')
            return 1

        # output report
        self.report_test()
        return 0

def print_pass(str):
    print(f'\033[32;2m{str}\033[0m')
    sys.stdout.flush()

def print_fail(str):
    print(f'\033[31;2m{str}\033[0m')
    sys.stdout.flush()

def main():
    args = parse_args()
    arktest = ArkTest(args)
    return arktest.test()

if __name__ == '__main__':
    sys.exit(main())
