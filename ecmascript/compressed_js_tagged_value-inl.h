/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPRESSED_JS_TAGGED_VALUE_INL_H
#define ECMASCRIPT_COMPRESSED_JS_TAGGED_VALUE_INL_H

#include "ecmascript/compressed_js_tagged_value.h"

#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
inline bool TemporaryJSTaggedValue::IsInSharedSweepableSpace() const
{
    if (IsHeapObject()) {
        Region *region = Region::ObjectAddressToRange(value_);
        return region->InSharedSweepableSpace();
    }
    return false;
}

inline bool TemporaryJSTaggedValue::IsAOTLiteralInfo() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsAOTLiteralInfo();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_TAGGED_VALUE_INL_H
