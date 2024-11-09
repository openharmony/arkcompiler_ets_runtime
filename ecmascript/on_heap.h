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

#ifndef ECMASCRIPT_ON_HEAP_H
#define ECMASCRIPT_ON_HEAP_H

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
enum class OnHeapMode : uint8_t {
    NONE = 0,
    ON_HEAP,
    NOT_ON_HEAP,
};

class OnHeap {
public:
    static bool IsNone(OnHeapMode mode)
    {
        return !(mode == OnHeapMode::ON_HEAP || mode == OnHeapMode::NOT_ON_HEAP);
    }

    static OnHeapMode Merge(OnHeapMode first, OnHeapMode second)
    {
        if (IsNone(first) || IsNone(second) || (first != second)) {
            return OnHeapMode::NONE;
        }
        return first;
    }

    static bool ToBoolean(OnHeapMode mode)
    {
        ASSERT(!IsNone(mode));
        return mode == OnHeapMode::ON_HEAP;
    }
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_ON_HEAP_H
