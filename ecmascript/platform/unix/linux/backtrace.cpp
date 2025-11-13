/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/backtrace.h"

#include <execinfo.h>

#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
static const int MAX_STACK_SIZE = 16;

void Backtrace(std::ostringstream &stack, [[maybe_unused]] bool enableCache)
{
    void *buffer[MAX_STACK_SIZE];
    char **stackList = nullptr;
    int framesLen = backtrace(buffer, MAX_STACK_SIZE);
    stackList = backtrace_symbols(buffer, framesLen);
    if (stackList == nullptr) {
        return;
    }

    stack << "=====================Backtrace========================";
    for (int i = 0; i < framesLen; ++i) {
        if (stackList[i] == nullptr) {
            break;
        }
        stack << stackList[i];
    }

    free(stackList);
}

int BacktraceHybrid([[maybe_unused]] void** pcArray, [[maybe_unused]] uint32_t maxSize)
{
    LOG_ECMA(INFO) << "BacktraceHybrid in linux not support";
    return 0;
}

std::string SymbolicAddress([[maybe_unused]] const void* const *pc,
                            [[maybe_unused]] int size,
                            [[maybe_unused]] const EcmaVM *vm)
{
    std::string stack;
    LOG_ECMA(INFO) << "SymbolicAddress in linux not support";
    return stack;
}
} // namespace panda::ecmascript
