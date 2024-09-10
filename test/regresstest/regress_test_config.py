#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Copyright (c) 2023-2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: run regress test case config
"""
import os


class RegressTestConfig:
    DEFAULT_TIMEOUT = 100
    DEFAULT_PROCESSES = 1
    DEFAULT_RETRIES = 3
    CURRENT_PATH = os.getcwd()
    TEST_TOOL_FILE_DIR = os.path.dirname(__file__)
    CODE_ROOT = os.path.abspath(os.path.join(TEST_TOOL_FILE_DIR, "../../../.."))
    PROJECT_ROOT = os.path.abspath(os.path.join(TEST_TOOL_FILE_DIR, "../.."))
    REGRESS_GIT_REPO = "ark-regress"
    TEST_TOOL_FILE_JS_NAME = 'mjsunit.js'
    TEST_TOOL_FILE_NAME = 'mjsunit.abc'
    REGRESS_GIT_URL = "https://gitee.com/chenjx-huawei/ark-regress.git"
    REGRESS_GIT_HASH = "36cd28467fa42ec45f132c4203c578223f3875d2"
    PROJECT_BASE_OUT_DIR = os.path.join(PROJECT_ROOT, "out")
    REGRESS_BASE_TEST_DIR = os.path.join(PROJECT_ROOT, "test", "regresstest")
    REGRESS_TEST_CASE_DIR = os.path.join(REGRESS_BASE_TEST_DIR, REGRESS_GIT_REPO)
    REGRESS_TEST_TOOL_DIR = os.path.join(REGRESS_TEST_CASE_DIR, TEST_TOOL_FILE_JS_NAME)
    SKIP_LIST_FILE = os.path.join(REGRESS_BASE_TEST_DIR, "regresstest_skip_tests.json")
    REGRESS_TEST_OPTIONS = os.path.join(REGRESS_BASE_TEST_DIR, "regress_options.json")
    DEFAULT_LOG_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'
    ICU_PATH_DATA = f"{CODE_ROOT}/third_party/icu/ohos_icu4j/data"
    ICU_PATH = f"{CODE_ROOT}/third_party/icu/"
    LLVM_DIR = f"{CODE_ROOT}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/"
    DEFAULT_LIBS_DIR = f"{ICU_PATH}:{ICU_PATH_DATA}:{LLVM_DIR}"

    ARK_ARCH_LIST = [
        "x64",
        "aarch64",
        "arm"
    ]

    DEFAULT_ARK_ARCH = ARK_ARCH_LIST[0]
