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

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
const char *GetPageTagString(PageTagType type)
{
    switch (type) {
        case PageTagType::HEAP:
            return "ArkTS Heap";
        case PageTagType::MACHINE_CODE:
            return "ArkTS Code";
        case PageTagType::MEMPOOL_CACHE:
            return "ArkTS MemPoolCache";
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}
}  // namespace panda::ecmascript