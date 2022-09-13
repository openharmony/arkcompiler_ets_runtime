/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/dfx/native_dfx/backtrace.h"

#include <execinfo.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"
#include "mem/mem.h"

namespace panda::ecmascript {
static const int MAX_STACK_SIZE = 256;
static const int FRAMES_LEN = 16;

void PrintBacktrace(uintptr_t value)
{
    void *stack[MAX_STACK_SIZE];
    char **stackList = nullptr;
    int framesLen = backtrace(stack, MAX_STACK_SIZE);
    stackList = backtrace_symbols(stack, framesLen);
    if (stackList == nullptr) {
        return;
    }

    LOG_ECMA(INFO) << "=====================Backtrace(" << std::hex << value <<")========================";
    for (int i = 0; i < FRAMES_LEN; ++i) {
        if (stackList[i] == nullptr) {
            break;
        }
        LOG_ECMA(INFO) << stackList[i];
    }

    free(stackList);
}
} // namespace panda::ecmascript
