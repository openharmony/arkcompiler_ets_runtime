#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Copyright (c) 2022 Huawei Device Co., Ltd.
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

from __future__ import print_function
import errno
import os
import platform
import re
import subprocess
import sys


USE_PTY = "linux" in sys.platform
if USE_PTY:
    import pty

BUILD_TARGETS_TEST = ["test262"]
# All arches that this script understands.
ARCHES = ["x64", "arm", "arm64", "android_arm", "android_arm64", "ios_arm"]
# Arches that get built/run when you don't specify any.
DEFAULT_ARCHES = ["x64"]
# Modes that this script understands.
MODES = ["release", "debug"]
# Modes that get built/run when you don't specify any.
DEFAULT_MODES = ["release"]
# Build targets that can be manually specified.
TARGETS = ["ark_js_vm"]
# Build targets that get built when you don't specify any (and specified tests
# don't imply any other targets).
DEFAULT_TARGETS = ["all"]
BUILD_TARGETS_ALL = ["all"]
DEFAULT_TESTS = ["test262"]


ACTIONS = {
    "all": {
        "targets": BUILD_TARGETS_ALL,
        "tests": [],
        "clean": False
    },
    "tests": {
        "targets": BUILD_TARGETS_TEST,
        "tests": DEFAULT_TESTS,
        "clean": False
    },
    "clean": {
        "targets": [],
        "tests": [],
        "clean": True
    },
}

HELP = """<arch> can be any of: %(arches)s
<mode> can be any of: %(modes)s
<target> can be any of:
 - %(targets)s (build respective binary)
 - all (build all binaries)
 formot like x64.release.ark_js_vm 
""" % {"arches": " ".join(ARCHES),
       "modes": " ".join(MODES),
       "targets": ", ".join(TARGETS)
}


OUTDIR = "out"

def _Which(cmd):
    for path in os.environ["PATH"].split(os.pathsep):
        if os.path.exists(os.path.join(path, cmd)):
            return os.path.join(path, cmd)
    return None

RELEASE_ARGS_TEMPLATE = """\
is_debug = false
is_standard_system = true
%s
"""

DEBUG_ARGS_TEMPLATE = """\
is_debug = true
is_standard_system = true
%s
"""

USER_ARGS_TEMPLATE = """
%s
"""

ARGS_TEMPLATES = {
    "release": RELEASE_ARGS_TEMPLATE,
    "debug": DEBUG_ARGS_TEMPLATE,
    "user":USER_ARGS_TEMPLATE,
}

def PrintHelp():
    print(__doc__)
    print(HELP)
    sys.exit(0)

def PrintCompletionsAndExit():
    for a in ARCHES:
        print("%s" % a)
        for m in MODES:
            print("%s" % m)
            print("%s.%s" % (a, m))
            for t in TARGETS:
                print("%s" % t)
                print("%s.%s.%s" % (a, m, t))
    sys.exit(0)

def _Call(cmd, silent=False):
    if not silent:
        print("# %s" % cmd)
    return subprocess.call(cmd, shell=True)

def _Write(filename, content, mode):
    with open(filename, mode) as f:
        f.write(content)

def _CallWithOutput(cmd, file):
    host, guest = pty.openpty()
    h = subprocess.Popen(cmd, shell=True, stdin=guest, stdout=guest, stderr=guest)
    os.close(guest)
    output_data = []
    while True:
        try:
            build_data = os.read(host, 512).decode('utf-8')  
        except OSError as error:
            if error == errno.ENOENT:
                print("no such file")
            elif error == errno.EPERM:
                print("permission denied")
            break 
        else:
            if not build_data:
                break
            print("build_data")
            print(build_data, end="")
            sys.stdout.flush()
            _Write(file, build_data, "a")
    os.close(host)
    h.wait()
    return h.returncode

def _Notify(summary, body):
    if (_Which('notify-send') is not None and
        os.environ.get("DISPLAY") is not None):
        _Call("notify-send '{}' '{}'".format(summary, body), silent=True)
    else:
        print("{} - {}".format(summary, body))

def _GetMachine():
    return platform.machine()

def GetPath(arch, mode):
    subdir = "%s.%s" % (arch, mode)
    return os.path.join(OUTDIR, subdir)

class Config(object):
    def __init__(self,
                 arch,
                 mode,
                 targets,
                 tests=[],
                 clean=False,
                 testrunner_args=[]):
        self.arch = arch
        self.mode = mode
        self.targets = set(targets)
        self.tests = set(tests)
        self.testrunner_args = testrunner_args
        self.clean = clean

    def Extend(self, targets, tests=[], clean=False):
        self.targets.update(targets)
        self.tests.update(tests)
        self.clean |= clean

    def GetARKTargetCpu(self):
        if self.arch in ("android_arm", "ios_arm"):
            ARK_cpu = "arm"
        elif self.arch == "android_arm64":
            ARK_cpu = "arm64"
        elif self.arch in ("arm", "arm64"):
            ARK_cpu = self.arch
        else:
            ARK_cpu = "x64"
        return ["target_cpu = \"%s\"" % ARK_cpu]

    def GetTargetOS(self):
        if self.arch in ("android_arm", "android_arm64"):
            target_os = "android"
        elif self.arch in ("ios_arm", "ios_arm64"):
            target_os = "ios"
        elif self.arch == "ohos":
            target_os = "ohos"
        else:
            target_os = "ohos"
        return ["target_os = \"%s\"" % target_os]

    def GetGnArgs(self):
        if (self.testrunner_args != []):
            template = ARGS_TEMPLATES["user"]
            arch_specific = self.testrunner_args[0].split("=", 1)[1].split(" ")
        else:
            mode = re.match("([^-]+)", self.mode).group(1)
            template = ARGS_TEMPLATES[mode]
            arch_specific = ''.join([self.GetTargetOS(), self.GetARKTargetCpu()])
        return template % "\n".join(arch_specific)

    def Build(self):
        path = GetPath(self.arch, self.mode)
        if not os.path.exists(path):
            print("# mkdir -p %s" % path)
            os.makedirs(path)
        if self.clean:
            print("=== start clean ===")
            code = _Call("./prebuilts/build-tools/linux-x86/bin/gn clean %s" % path)
            code = ''.join([code, _Call("./prebuilts/build-tools/linux-x86/bin/ninja -C %s -t clean" % path)])
            if code != 0:
                return code
            print("=== clean success! ===")
        build_log = os.path.join(path, "build.log")
        if not os.path.exists("args.gn"):
            args_gn = os.path.join(path, "args.gn")
            _Write(args_gn, self.GetGnArgs(), "w")
        if not os.path.exists("build.ninja"):
            build_ninja = os.path.join(path, "build.ninja")
            code = _Call("./prebuilts/build-tools/linux-x86/bin/gn gen %s" % path)
            print("=== gn success! ===")
            if code != 0:
                return code
        targets = " ".join(self.targets)
        pass_code = _CallWithOutput("./prebuilts/build-tools/linux-x86/bin/ninja -C %s %s" %
                                              (path, targets), build_log)
        if pass_code == 0:
            print("=== ninja success! ===")
        return pass_code

    def RunTest(self):
        self.test_dir = ''.join([self.arch, '.', self.mode])
        test262_code = '''cd ets_frontend 
        python3 test262/run_test262.py 
        --es2021 
        --libs-dir ../out/%s:../prebuilts/clang/ohos/linux-x86_64/llvm/lib 
        --ark-tool=../out/%s/ark_js_vm 
        --ark-frontend=ts2panda''' % (self.test_dir, self.test_dir)

        if ("-test262" in self.tests):
            print("=== come to test ===")
            return _Call(test262_code)
        else:
            print("=== nothing to test ===")
            return 0

class Argsgetter(object):
    def __init__(self):
        self.global_targets = []
        self.global_tests = []
        self.global_actions = []
        self.configs = {}
        self.testrunner_args = []

    def PopulateConfigs(self, arches, modes, targets, tests, clean):
        for a in arches:
            for m in modes:
                path = GetPath(a, m)
                if path not in self.configs:
                    self.configs[path] = Config(a, m, targets, tests, clean,
                            self.testrunner_args)
                else:
                    self.configs[path].Extend(targets, tests)

    def ParseArg(self, arg_string):
        if arg_string == "-h":
            PrintHelp()
        if arg_string == "-v":
            PrintCompletionsAndExit()
        arches = []
        modes = []
        targets = []
        actions = []
        tests = []
        clean = False
        # Specifying a single unit test looks like test262
        if arg_string.startswith("-test262"):
            values = [arg_string]
        elif arg_string.startswith("--args"):
            # Pass all other flags to test runner.
            self.testrunner_args.append(arg_string)
            return 
        else:
            # Assume it's a word like "x64.release" -> split at the dot.
            values = arg_string.split('.')
        if len(values) == 1:
            value = values[0]
            if value in ACTIONS:
                self.global_actions.append(value)
                return
            if value in TARGETS:
                self.global_targets.append(value)
                return
        else:
            for value in values:
                if value in ARCHES:
                    arches.append(value)
                elif value in MODES:
                    modes.append(value)
                elif value in TARGETS:
                    targets.append(value)
                elif value in ACTIONS:
                    actions.append(value)
                else:
                    print("\033[34mCant Identify: %s\033[0m" % value)
                    sys.exit(1)
        # Process actions.
        for act in actions:
            get_target = ACTIONS[act]
            targets = ''.join([targets, get_target["targets"]])
            tests = ''.join([tests, get_target["tests"]])
            clean |= get_target["clean"]
        # Fill in defaults for things that weren't specified.
        if arches != []:
            arches = arches
        else:
            arches = DEFAULT_ARCHES
        if modes != []:
            modes = modes
        else:
            modes = DEFAULT_MODES
        if targets != []:
            targets = targets
        else:
            targets = DEFAULT_TARGETS
        # Produce configs.
        self.PopulateConfigs(arches, modes, targets, tests, clean)

    def Analytical(self, argvs):
        if len(argvs) != 0:
            for arg_string in argvs:
                self.ParseArg(arg_string)
            return self.configs
        else:
            PrintHelp()

def Main(argvs):
    get_argns = Argsgetter()
    configs = get_argns.Analytical(argvs[1:])
    for config in configs:
        pass_code = configs[config].Build()
    if pass_code == 0:
        for config in configs:
            pass_code = configs[config].RunTest()
    if pass_code == 0:
        _Notify('\033[32mDone!\033[0m',
            '\033[32mARK_{} compilation finished successfully.\033[0m'.format(argvs[1].split('.')[0]))
    else:
        _Notify('\033[31mError!\033[0m',
            '\033[31mARK_{} compilation finished with errors.\033[0m'.format(argvs[1].split('.')[0]))
    return pass_code

if __name__ == "__main__":
    sys.exit(Main(sys.argv))
