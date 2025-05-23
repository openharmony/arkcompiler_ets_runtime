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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_PLAIN_ARRAY_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_PLAIN_ARRAY_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in PlainArray.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersPlainArray::func refers to the native implementation of PlainArray.prototype[name].
#define CONTAINER_PLAINARRAY_PROTOTYPE_FUNCTIONS(V)                                     \
    V("add",                    Add,                2,          INVALID)                \
    V("clear",                  Clear,              0,          INVALID)                \
    V("clone",                  Clone,              0,          INVALID)                \
    V("has",                    Has,                1,          INVALID)                \
    V("get",                    Get,                1,          INVALID)                \
    V("forEach",                ForEach,            2,          PlainArrayForEach)      \
    V("toString",               ToString,           0,          INVALID)                \
    V("getIndexOfKey",          GetIndexOfKey,      1,          INVALID)                \
    V("getIndexOfValue",        GetIndexOfValue,    1,          INVALID)                \
    V("isEmpty",                IsEmpty,            0,          INVALID)                \
    V("getKeyAt",               GetKeyAt,           1,          INVALID)                \
    V("remove",                 Remove,             1,          INVALID)                \
    V("removeAt",               RemoveAt,           1,          INVALID)                \
    V("removeRangeFrom",        RemoveRangeFrom,    2,          INVALID)                \
    V("setValueAt",             SetValueAt,         2,          INVALID)                \
    V("getValueAt",             GetValueAt,         1,          INVALID)

namespace panda::ecmascript::containers {
class ContainersPlainArray : public base::BuiltinsBase {
public:
    static JSTaggedValue PlainArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clone(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOfKey(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOfValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetKeyAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveRangeFrom(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SetValueAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetValueAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetPlainArrayPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(PLAINARRAY_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_PLAINARRAY_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersPlainArray::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array PLAINARRAY_PROTOTYPE_FUNCTIONS = {
        CONTAINER_PLAINARRAY_PROTOTYPE_FUNCTIONS(CONTAINER_PLAINARRAY_FUNCTION_ENTRY)
    };
#undef CONTAINER_PLAINARRAY_FUNCTION_ENTRY
};
} // namespace panda::ecmascript::containers
#endif // ECMASCRIPT_CONTAINERS_CONTAINERS_PLAIN_ARRAY_H
