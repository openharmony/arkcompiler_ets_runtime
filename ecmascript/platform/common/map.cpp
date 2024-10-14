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

#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
const CString GetPageTagString(PageTagType type, const std::string &spaceName, const uint32_t threadId)
{
    switch (type) {
        case PageTagType::HEAP:
            if (threadId == 0) {
                return CString(HEAP_TAG).append(spaceName);
            }
            return CString(HEAP_TAG).append(std::to_string(threadId)).append(spaceName);
        case PageTagType::MACHINE_CODE:
            if (threadId == 0) {
                return CODE_TAG;
            }
            return CString(CODE_TAG).append(std::to_string(threadId));
        case PageTagType::MEMPOOL_CACHE:
            return "ArkTS MemPoolCache";
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}
}  // namespace panda::ecmascript