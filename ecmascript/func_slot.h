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

#ifndef ECMASCRIPT_FUNC_SLOT_H
#define ECMASCRIPT_FUNC_SLOT_H

#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class TaggedArray;

// Function slot, this should be stored on profile type info as the slot used for function calls.
// It has fixed length 2, the 1st value is a js function and the 2nd value is the call count.
class FuncSlot : public TaggedArray {
public:
    static constexpr uint32_t FUNC_SLOT_SIZE = 2;
    static constexpr uint32_t FUNCTION_INDEX = 0;
    static constexpr uint32_t CALL_CNT_INDEX = 1;

    CAST_CHECK(FuncSlot, IsFuncSlot);
    DECL_DUMP()

    JSTaggedValue GetFunction(const JSThread *thread) const
    {
        return Get(thread, FUNCTION_INDEX);
    }

    JSTaggedValue GetCallCnt(const JSThread *thread) const
    {
        return Get(thread, CALL_CNT_INDEX);
    }
private:
    friend class ObjectFactory;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_FUNC_SLOT_H
