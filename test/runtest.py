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
        help='product name, default is hispark_taurus on x64, rk3568 on arm64, baltimore on arm64')
    parser.add_argument('-t', '--tool', metavar='opt',
        help='run tool supported opt: aot, int(c interpreter tool), asmint(asm interpreter tool), node(v8), qjs, hermes')
    parser.add_argument('-f', '--frontend', metavar='opt',
        help='run frontend supported opt: ts2abc (slow), es2abc (quick not released)')
    parser.add_argument('-s', '--step', metavar='opt',
        help='run step supported opt: abc, pack, aot, aotd, run, rund, asmint, asmintd, int, intd')
    parser.add_argument('-d', '--debug', action='store_true', help='run on debug mode')
    parser.add_argument('--arm64', action='store_true', help='run on arm64 platform')
    parser.add_argument('--device', action='store_true', help='run on device')
    parser.add_argument('--copy-path', metavar='path', help='copy bins to device')
    parser.add_argument('-m', '--module', action='store_true', help='frontend compile with module')
    parser.add_argument('--frontend-args', metavar='args', help='pass to frontend args')
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
    parser.add_argument('--sign', metavar='name',
        help='sign level, default is system_core, other is normal, system_basic')
    parser.add_argument('--timeout', metavar='n', default=DEFAULT_TIMEOUT, type=int,
        help=f'specify seconds of test timeout, default is {DEFAULT_TIMEOUT}')
    parser.add_argument('-e', '--env', action='store_true', help='print LD_LIBRARY_PATH')
    arguments = parser.parse_args()
    return arguments

def run_and_print(cmd):
    print(cmd)
    os.system(cmd)

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

def get_module_name(hap_dir):
    with open(f'{hap_dir}/module.json') as f:
        data = json.load(f)
    if len(data):
        return data['module']['name']
    else:
        return 'entry'

def get_bundle_name(hap_dir):
    with open(f'{hap_dir}/module.json') as f:
        data = json.load(f)
    if len(data):
        return data['app']['bundleName']
    else:
        return 'entry'

class ArkTest():
    def __init__(self, args):
        self.args = args
        self.self_dir = os.path.abspath(sys.argv[0])
        self.hap_abc = 'ets/modules.abc'
        self.place_dir = 'arkcompiler/ets_runtime/test'
        if not args.device and self.self_dir.find(self.place_dir) < 0:
            print(f'pls place this script at: {self.place_dir}')
            sys.exit(1)

        self.ohdir = os.path.abspath(f'{self.self_dir}/../../../..')
        self.product = PRODUCT_LIST[TARGET_PRODUCT_MAP['x64']]
        self.builtin = ''
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
        search_type_list = ['.ts', '.js', '.abc']
        self.types = {'all': ['.ts', '.js'],
                      'abc': ['.ts', '.js'],
                      'pack': ['.an'],
                      'aot': search_type_list,
                      'aotd': search_type_list,
                      'run': search_type_list,
                      'rund': search_type_list,
                      'asmint': search_type_list,
                      'asmintd': search_type_list,
                      'int': search_type_list,
                      'intd': search_type_list,
                      'clean': search_type_list}

        product_dir = f'{self.ohdir}/out/{self.product}'
        libs_dir_x64_release = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib:'
                                f'{product_dir}/clang_x64/arkcompiler/ets_runtime:'
                                f'{product_dir}/clang_x64/thirdparty/icu:'
                                f'{product_dir}/clang_x64/thirdparty/zlib')
        libs_dir_x64_debug = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib:'
                              f'{product_dir}/clang_x64/exe.unstripped/clang_x64/arkcompiler/ets_runtime:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/arkcompiler/ets_runtime:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/test/test:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/thirdparty/icu:'
                              f'{product_dir}/clang_x64/lib.unstripped/clang_x64/thirdparty/zlib')
        libs_dir_arm64_release = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:'
                                  f'{product_dir}/arkcompiler/ets_runtime/:'
                                  f'{product_dir}/utils/utils_base/:'
                                  f'{product_dir}/thirdparty/icu:'
                                  f'{product_dir}/thirdparty/zlib:'
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
                                  f'{product_dir}/clang_x64/thirdparty/icu:'
                                  f'{product_dir}/clang_x64/thirdparty/zlib:'
                                  f'{product_dir}/clang_x64/arkcompiler/ets_runtime')
        libs_dir_arm64_debug = (f'{self.ohdir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:'
                                f'{product_dir}/lib.unstripped/arkcompiler/ets_runtime/:'
                                f'{product_dir}/utils/utils_base/:'
                                f'{product_dir}/thirdparty/icu:'
                                f'{product_dir}/thirdparty/zlib:'
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
                                f'{product_dir}/clang_x64/thirdparty/zlib/:'
                                f'{product_dir}/clang_x64/arkcompiler/ets_runtime')
        libs_dir = [[libs_dir_x64_release, libs_dir_x64_debug], [libs_dir_arm64_release, libs_dir_arm64_debug]]
        bins_dir = [['clang_x64/arkcompiler', 'clang_x64/exe.unstripped/clang_x64/arkcompiler'],
                    ['arkcompiler', 'exe.unstripped/arkcompiler']]
        icu_arg = f'--icu-data-path={self.ohdir}/third_party/icu/ohos_icu4j/data'
        self.libs_dir = libs_dir[self.arm64][args.debug]
        self.compiler = f'{product_dir}/{bins_dir[0][args.debug]}/ets_runtime/ark_aot_compiler'
        self.jsvm = f'{product_dir}/{bins_dir[self.arm64][args.debug]}/ets_runtime/ark_js_vm'
        self.ts2abc = f'node --expose-gc {product_dir}/clang_x64/arkcompiler/ets_frontend/build/src/index.js'
        self.es2abc = f'{product_dir}/clang_x64/arkcompiler/ets_frontend/es2abc'
        self.frontend = self.ts2abc
        if not args.frontend:
            args.frontend = 'ts2abc'
        if args.frontend not in ['ts2abc', 'es2abc']:
            print(f'not supported frontend: {args.frontend}')
            sys.exit(1)
        if args.frontend == 'es2abc':
            self.frontend = self.es2abc
        abcmode = {'ts2abc': ['--merge-abc', '--merge-abc -m'],
                   'es2abc': ['--merge-abc', '--merge-abc --module']}
        self.abcmode = abcmode[args.frontend][args.module]
        self.frontend_args = ''
        self.aot_args = ''
        self.jsvm_args = icu_arg
        if args.device:
            self.jsvm_args = ''
        if self.builtin:
            self.aot_args = f'{self.aot_args} --builtins-dts={self.builtin}.abc'
        self.pgo = False
        if args.pgo:
            self.pgo = True
            self.aot_args = (f'{self.aot_args} --enable-pgo-profiler=true --compiler-pgo-hotness-threshold={args.pgo_th}'
                             f' --compiler-pgo-profiler-path=pgo_file_name.ap')
        if args.frontend_args:
            self.frontend_args = f'{self.frontend_args} {args.frontend_args}'
        if args.aot_args:
            self.aot_args = f'{self.aot_args} {args.aot_args}'
        if args.jsvm_args:
            self.jsvm_args = f'{self.jsvm_args} {args.jsvm_args}'
        if args.info:
            self.aot_args = f'{self.aot_args} --log-level=info'
            self.jsvm_args = f'{self.jsvm_args} --log-level=info'
        self.runner = ''
        self.runnerd = 'gdb --args'
        if self.arm64 or args.device:
            if self.step[:3] != 'aot':
                self.runner = 'qemu-aarch64'
                self.runnerd = 'qemu-aarch64 -cpu max,sve=off -g 123456'
            self.aot_args = f'{self.aot_args} --compiler-target-triple=aarch64-unknown-linux-gnu'
        self.test_count = 0
        self.fail_cases = []
        os.environ['LD_LIBRARY_PATH'] = self.libs_dir
        if args.env:
            print(f'export LD_LIBRARY_PATH={self.libs_dir}')
            sys.exit(0)
        if args.copy_path:
            run_and_print(f'hdc shell mount -o remount,rw /')
            run_and_print(f'hdc file send {args.copy_path}\\ark_aot_compiler /system/bin/')
            run_and_print(f'hdc shell chmod a+x /system/bin/ark_aot_compiler')
            run_and_print(f'hdc file send {args.copy_path}\\ark_js_vm /system/bin/')
            run_and_print(f'hdc shell chmod a+x /system/bin/ark_js_vm')
            sys.exit(0)
        if args.npm:
            index_dir = f'{product_dir}/clang_x64/arkcompiler/ets_frontend/build/src'
            os.system(f'cd {index_dir}/.. && npm install')
            sys.exit(0)
        if args.sign:
            self.sign_hap(self.args.name)
            sys.exit(0)

    def run_cmd(self, cmd):
        print(cmd)
        ret = run_command(cmd, self.args.timeout)
        if ret[0]:
            print(ret[2])
        return ret

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
            module_name = get_module_name(hap_dir)
        abc_file = f'{os.path.splitext(file)[0]}.abc'
        if self.pgo:
            pgo_file = f'{hap_dir}/ap/{module_name}'
            self.aot_args = self.aot_args.replace('pgo_file_name', pgo_file)
        cmd_map = {
            'node': f'node {self.frontend_args} {file}',
            'qjs': f'qjs {self.frontend_args} {file}',
            'hermes': f'hermes {self.frontend_args} {file}',
            'abc': f'{self.frontend} {self.frontend_args} {self.abcmode} --output {abc_file} {file}',
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
            if self.frontend == self.ts2abc:
                cmd = f'{self.ts2abc} {self.builtin}.ts -m --merge-abc -q -b'
            elif self.frontend == self.es2abc:
                cmd = (f'{self.es2abc} --module --merge-abc --extension=ts '
                       f'--output={self.builtin}.abc {self.builtin}.ts')
            print(cmd)
            os.system(cmd)
        if self.step == 'hap':
            self.step = 'aot'
            perf_start = time.perf_counter()
            cmd = cmd_map[self.step]
            print(cmd)
            os.system(cmd)
            perf_end = time.perf_counter()
            abc_size = os.path.getsize(file) / 1024 / 1024
            an_size = os.path.getsize(f'{out_case_dir}/{name}.an') / 1024 / 1024
            print(f'test: {file}  abc_size: {abc_size: .1f}MB  an_size: {an_size:.1f}MB  '
                  f'expand: {an_size / abc_size: .1f}  time: {perf_end - perf_start: .1f}s')
            self.step = 'pack'
        if self.step == 'pack':
            for cmd in cmd_map[self.step]:
                print(cmd)
                os.system(cmd)
            print(f'packed hap: {hap_name}.hap')
            print(f'sign --------------------------------------------')
            self.sign_hap(f'{hap_name}.hap')
            return
        if self.step == 'clean':
            if os.path.isfile(f'{hap_dir}/{self.hap_abc}'):
                self.step = 'cleanhap'
        if self.args.tool == 'node':
            ret = self.run_cmd(cmd_map['node'])
            self.judge_test(file, ret)
            return
        if self.args.tool == 'qjs':
            ret = self.run_cmd(cmd_map['qjs'])
            self.judge_test(file, ret)
            return
        if self.args.tool == 'hermes':
            ret = self.run_cmd(cmd_map['hermes'])
            self.judge_test(file, ret)
            return
        if not self.args.tool:
            self.args.tool = 'aot'
        if self.args.tool not in ['aot', 'asmint', 'int']:
            print(f'not supported tool: {self.args.tool}')
            sys.exit(1)
        if self.args.device:
            ret = self.run_test_on_device(file)
            return
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
        if self.args.tool == 'aot':
            ret = self.run_cmd(cmd_map['aot'])
            if ret[0] and ret[2].find('aot compile success') < 0:
                self.judge_test(file, ret)
                return
            ret = self.run_cmd(cmd_map['run'])
        else:
            ret = self.run_cmd(cmd_map[self.args.tool])
        self.judge_test(file, ret)

    def run_test_on_device(self, file):
        basename = os.path.basename(f'{file}')
        name = os.path.splitext(basename)[0]
        out_case_dir = '/data/test'
        send_abc_file = f'{os.path.splitext(file)[0]}.abc'.replace('/', '\\')
        abc_file = f'{out_case_dir}/{name}.abc'
        cmd_map = {'abc': f'hdc file send {send_abc_file} {out_case_dir}/',
                   'aot': f'hdc shell ark_aot_compiler {self.aot_args} --aot-file={out_case_dir}/{name} {abc_file}',
                   'run': f'hdc shell ark_js_vm {self.jsvm_args} --aot-file={out_case_dir}/{name} --entry-point={name} {abc_file}',
                   'asmint': f'hdc shell ark_js_vm {self.jsvm_args} --entry-point={name} {abc_file}',
                   'int': f'hdc shell ark_js_vm {self.jsvm_args} --asm-interpreter=0 --entry-point={name} {abc_file}'}
        if self.step != 'all':
            run_and_print(cmd_map[self.step])
            return
        run_and_print(cmd_map['abc'])
        if self.args.tool == 'aot':
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
            if out[1].replace('\r', '') != expect.replace('\r', ''):
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

    def sign_hap(self, hap_name):
        name = os.path.splitext(hap_name)[0]
        sign_dir = f'{name}.sign'
        sign_tool_dir = f'{self.ohdir}/developtools/hapsigner/dist'
        name = os.path.splitext(sign_dir)[0]
        self_dir = os.path.abspath(sys.argv[0])
        os.system(f'mkdir -p {sign_dir} && unzip -o -q {hap_name} module.json -d {sign_dir}')
        bundle_name = get_bundle_name(sign_dir)
        if not self.args.sign or self.args.sign == 'system_core':
            bundle_apl = 'system_core'
            bundle_feature = 'hos_system_app'
        elif self.args.sign == 'system_basic':
            bundle_apl = self.args.sign
            bundle_feature = 'hos_system_app'
        elif self.args.sign == 'normal':
            bundle_apl = self.args.sign
            bundle_feature = 'hos_normal_app'
        else:
            print(f'sign not supported input: {self.args.sign}')
            return 1
        # modify sign config
        data_load = []
        data_save = []
        sign_config = 'UnsgnedReleasedProfileTemplate.json'
        with open(f'{sign_tool_dir}/{sign_config}') as f:
            data_load = json.load(f)
            data_load['bundle-info']['bundle-name'] = bundle_name
            data_load['bundle-info']['apl'] = bundle_apl
            data_load['bundle-info']['app-feature'] = bundle_feature
            data_save = json.dumps(data_load)
        with open(f'{sign_dir}/{sign_config}', 'w+') as f:
            f.write(data_save)
        # generate cert and sign
        gen_cert = f'java -jar {sign_tool_dir}/hap-sign-tool.jar sign-profile -keyAlias "openharmony application profile release" -signAlg "SHA256withECDSA" -mode "localSign" -profileCertFile "{sign_tool_dir}/OpenHarmonyProfileRelease.pem" -inFile "{sign_dir}/{sign_config}" -keystoreFile "{sign_tool_dir}/OpenHarmony.p12" -outFile "{sign_dir}/openharmony.p7b" -keyPwd "123456" -keystorePwd "123456"'
        sign_hap = f'java -jar {sign_tool_dir}/hap-sign-tool.jar sign-app -keyAlias "openharmony application release" -signAlg "SHA256withECDSA" -mode "localSign" -appCertFile "{sign_tool_dir}/OpenHarmonyApplication.pem" -profileFile "{sign_dir}/openharmony.p7b" -inFile "{hap_name}" -keystoreFile "{sign_tool_dir}/OpenHarmony.p12" -outFile "{name}.sign.hap" -keyPwd "123456" -keystorePwd "123456"'
        print(gen_cert)
        print(sign_hap)
        os.system(gen_cert)
        os.system(sign_hap)
        print(f'signed of {bundle_apl} for hap: {name}.sign.hap')

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
                print(f'only support file type: {self.types[self.step]}')
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
