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

#ifndef ECMASCRIPT_ARKSTEED_HEAP_REF_H
#define ECMASCRIPT_ARKSTEED_HEAP_REF_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value_wrapper.h"

namespace panda::ecmascript::arksteed {

class ArkSteedHeapRef {
public:
    ArkSteedHeapRef() = default;
    explicit ArkSteedHeapRef(JSTaggedValue value) : value_(value) {}
    ArkSteedHeapRef(JSTaggedValue value, bool stable) : value_(value), stable_(stable) {}
    explicit ArkSteedHeapRef(JSHandle<JSTaggedValue> handle) : handle_(handle), stable_(true) {}

    ArkSteedHeapRef &operator=(JSTaggedValue value) = delete;

    JSTaggedValue Value() const
    {
        ASSERT(!HasHandle());
        return value_;
    }

    bool HasHandle() const
    {
        return handle_.GetAddress() != 0U;
    }

    bool IsUndefined() const = delete;

    bool IsHeapObject() const = delete;

    bool IsInt() const = delete;

    bool IsStable() const
    {
        return stable_;
    }

    bool IsSafeForCompile() const
    {
        return stable_;
    }

    bool IsSafeToEmbed() const = delete;

    uint64_t GetLargeUInt() const = delete;

    bool operator==(const ArkSteedHeapRef &other) const = delete;

    bool operator!=(const ArkSteedHeapRef &other) const = delete;

    operator JSTaggedValue() const = delete;

private:
    JSTaggedValue ValueAllowHandleDeref() const
    {
        if (HasHandle()) {
            return handle_.GetTaggedValue();
        }
        return value_;
    }

    JSTaggedValue value_ {JSTaggedValue::Undefined()};
    JSHandle<JSTaggedValue> handle_ {};
    bool stable_ {false};

    friend class ArkSteedHeapBroker;
};

using ArkSteedObjectRef = ArkSteedHeapRef;
using ArkSteedHClassRef = ArkSteedHeapRef;
using ArkSteedProtoCellRef = ArkSteedHeapRef;
using ArkSteedHandlerRef = ArkSteedHeapRef;
using ArkSteedNameRef = ArkSteedHeapRef;

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_HEAP_REF_H
