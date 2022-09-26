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

DEFAULT_TIMEOUT = 300

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('name', metavar='file|path', type=str, help='test case name: file or path')
    parser.add_argument('--all', action='store_true', help='run all test cases on path')
    parser.add_argument('--product', metavar='name',
        help='product name, default is hispark_taurus on x64, rk3568 on arm64')
    parser.add_argument('--type', metavar='opt', help='run test type opt: js, ts, [jt]s, default is [jt]s')
    parser.add_argument('--tool', metavar='opt',
        help='run tool supported opt: aot, int(c interpreter tool), asmint(asm interpreter tool)')
    parser.add_argument('--step', metavar='opt',
        help='run step supported opt: abc, aot, aotd, run, rund, asmint, asmintd, int, intd')
    parser.add_argument('--debug', action='store_true', help='run on debug mode')
    parser.add_argument('--arm64', action='store_true', help='run on arm64 platform')
    parser.add_argument('--aot-args', metavar='args', help='pass to aot compiler args')
    parser.add_argument('--jsvm-args', metavar='args', help='pass to jsvm args')
    parser.add_argument('--timeout', metavar='n', default=DEFAULT_TIMEOUT, type=int,
        help=f'specify seconds of test timeout, default is {DEFAULT_TIMEOUT}')
    parser.add_argument('--env', action='store_true', help='print LD_LIBRARY_PATH')
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

class ArkTest():
    def __init__(self, args):
        self.args = args
        self.ohdir = '../..'
        self.work_dir = 'arkcompiler/ets_runtime'
        if os.getcwd().find(self.work_dir) < 0:
            print(f'pls run this script at: {self.work_dir}')
            sys.exit(1)

        self.product = 'hispark_taurus'
        if args.arm64:
            self.product = 'rk3568'
        if args.product:
            self.product = args.product

        self.expect = 'expect_output.txt'
        self.type = '[jt]s'
        if self.args.type:
            self.type = self.args.type

        product_dir = f'{self.ohdir}/out/{self.product}'
        libs_dir_x64_release = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib:'
                                f'{product_dir}/clang_x64/ark/ark:'
                                f'{product_dir}/clang_x64/ark/ark_js_runtime:'
                                f'{product_dir}/clang_x64/thirdparty/icu')
        libs_dir_x64_debug = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib:'
                              f'{product_dir}/clang_x64/exe.unstripped/clang_x64/ark/ark:'
                              f'{product_dir}/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/ark/ark:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/ark/ark_js_runtime:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/test/test:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/thirdparty/icu')
        libs_dir_arm64_release = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:'
                                  f'{product_dir}/ark/ark/:'
                                  f'{product_dir}/ark/ark_js_runtime/:'
                                  f'{product_dir}/utils/utils_base/:'
                                  f'{product_dir}/thirdparty/icu:'
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
                                  f'{product_dir}/clang_x64/ark/ark/:'
                                  f'{product_dir}/clang_x64/thirdparty/icu/:'
                                  f'{product_dir}/clang_x64/ark/ark_js_runtime')
        libs_dir_arm64_debug = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:'
                                f'{product_dir}/lib.unstripped/ark/ark/:'
                                f'{product_dir}/lib.unstripped/ark/ark_js_runtime/:'
                                f'{product_dir}/utils/utils_base/:'
                                f'{product_dir}/thirdparty/icu:'
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
                                f'{product_dir}/clang_x64/ark/ark/:'
                                f'{product_dir}/clang_x64/thirdparty/icu/:'
                                f'{product_dir}/clang_x64/ark/ark_js_runtime')
        libs_dir = [[libs_dir_x64_release, libs_dir_x64_debug], [libs_dir_arm64_release, libs_dir_arm64_debug]]
        bins_dir = [['clang_x64/ark', 'clang_x64/exe.unstripped/clang_x64/ark'], ['ark', 'exe.unstripped/ark']]
        icu_arg = f'--icu-data-path={self.ohdir}/third_party/icu/ohos_icu4j/data'
        self.libs_dir = libs_dir[args.arm64][args.debug]
        self.compiler = f'{product_dir}/{bins_dir[0][args.debug]}/ark_js_runtime/ark_aot_compiler'
        self.jsvm = f'{product_dir}/{bins_dir[args.arm64][args.debug]}/ark_js_runtime/ark_js_vm'
        self.ts2abc = f'{product_dir}/clang_x64/ark/ark/build/src/index.js'
        self.aot_args = ''
        self.jsvm_args = icu_arg
        if args.aot_args:
            self.aot_args = f'{self.aot_args} {args.aot_args}'
        if args.jsvm_args:
            self.jsvm_args = f'{self.jsvm_args} {args.jsvm_args}'
        self.runner = ''
        self.runnerd = 'gdb --args'
        if args.arm64:
            self.runner = 'qemu-aarch64'
            self.runnerd = 'qemu-aarch64 -cpu max,sve=off -g 123456'
            self.aot_args = f'{self.aot_args} --target-triple=aarch64-unknown-linux-gnu'
        self.out_dir = f'out/{self.product}/test'
        self.test_count = 0
        self.fail_cases = []
        os.environ['LD_LIBRARY_PATH'] = self.libs_dir
        if self.args.env:
            print(f'export LD_LIBRARY_PATH={self.libs_dir}')
            sys.exit(0)

    def run_cmd(self, cmd):
        print(cmd)
        ret = run_command(cmd, self.args.timeout)
        if ret[0]:
            print(ret[2])
        return ret

    def run_test(self, file):
        self.test_count += 1
        name = os.path.basename(file)[:-3]
        out_case_dir = f'{self.out_dir}/{name}'
        abc_file = f'{file[:-3]}.abc'
        module_abc_file = abc_file
        js_dir = os.path.dirname(file)
        module_arg = ''
        with open(file, mode='r') as infile:
            for line in infile.readlines():
                import_line = re.findall(r"^(?:ex|im)port.*\.js", line)
                if len(import_line):
                    module_arg = '-m'
                    import_file = re.findall(r"['\"].*\.js", import_line[0])
                    if len(import_file):
                        abc = import_file[0][1:].replace(".js", ".abc")
                        abc = os.path.abspath(f'{js_dir}/{abc}')
                        if module_abc_file.find(abc) < 0:
                            module_abc_file = ''.join([module_abc_file, f':{abc}'])
        os.system(f'mkdir -p {out_case_dir}')
        cmd_map = {
            'abc': f'node --expose-gc {self.ts2abc} {file} {module_arg}',
            'aot': f'{self.compiler} {self.aot_args} --aot-file={out_case_dir}/{name} {module_abc_file}',
            'aotd': f'{self.runnerd} {self.compiler} {self.aot_args} --aot-file={out_case_dir}/{name} {module_abc_file}',
            'run': f'{self.runner} {self.jsvm} {self.jsvm_args} --aot-file={out_case_dir}/{name} {abc_file}',
            'rund': f'{self.runnerd} {self.jsvm} {self.jsvm_args} --aot-file={out_case_dir}/{name} {abc_file}',
            'asmint': f'{self.runner} {self.jsvm} {self.jsvm_args} {abc_file}',
            'asmintd': f'{self.runnerd} {self.jsvm} {self.jsvm_args} {abc_file}',
            'int': f'{self.runner} {self.jsvm} {self.jsvm_args} --asm-interpreter=0 {abc_file}',
            'intd': f'{self.runnerd} {self.jsvm} {self.jsvm_args} --asm-interpreter=0 {abc_file}'}
        if self.args.step:
            # gdb should use the os.system
            cmd = cmd_map[self.args.step]
            print(cmd)
            if self.args.step[-1:] == 'd':
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

    def get_test(self, dir):
        name = os.path.basename(dir)
        file = glob.glob(f'{dir}/*.{self.type}')
        if len(file):
            return file[0]
        else:
            return ''

    def test(self):
        # run single test by name
        if not self.args.all:
            if os.path.isdir(self.args.name):
                test = self.get_test(self.args.name)
                if test:
                    self.run_test(test)
                else:
                    print(f'input path no test case: {self.args.name}')
                    return 1
            elif os.path.exists(self.args.name):
                self.run_test(self.args.name)
            else:
                print(f'input test not exists: {self.args.name}')
                return 1
            return 0

        # run all test in path
        if not os.path.isdir(self.args.name):
            print(f'input path not exists or is file: {self.args.name}')
            return 1
        for test in glob.glob(f'{self.args.name}/*'):
            if (test[-3:] == '.js') or (test[-3:] == '.ts'):
                self.run_test(test)
            elif os.path.isdir(test):
                name = os.path.basename(test)
                file = glob.glob(f'{test}/*.{self.type}')
                if self.get_test(test):
                    self.run_test(file[0])
        if self.test_count == 0:
            print(f'input path no test case: {self.args.name}')
            return 1
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