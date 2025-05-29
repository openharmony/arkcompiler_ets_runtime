/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/base/bit_helper.h"

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::base {
bool CheckBufferBounds(void* addr, void* buffer, size_t bufferSize, const char* name)
{
    if (reinterpret_cast<uintptr_t>(addr) > reinterpret_cast<uintptr_t>(buffer) + bufferSize ||
        reinterpret_cast<uintptr_t>(addr) < reinterpret_cast<uintptr_t>(buffer)) {
        LOG_PGO(ERROR) << "out of bounds at [" << name << "] addr: 0x" << std::hex
                       << reinterpret_cast<uintptr_t>(addr) << ", buffer start: 0x"
                       << reinterpret_cast<uintptr_t>(buffer) << ", buffer end: 0x"
                       << reinterpret_cast<uintptr_t>(buffer) + bufferSize << std::dec;
        return false;
    }
    return true;
}
} // panda::ecmascript::base
