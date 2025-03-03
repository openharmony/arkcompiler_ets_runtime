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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_ARRAYLIST_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_ARRAYLIST_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in ArrayList.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersArrayList::func refers to the native implementation of ArrayList.prototype[name].
#define CONTAINER_ARRAYLIST_PROTOTYPE_FUNCTIONS(V)                                              \
    V("add",                    Add,                    1,      INVALID)                        \
    V("insert",                 Insert,                 2,      INVALID)                        \
    V("clear",                  Clear,                  0,      INVALID)                        \
    V("clone",                  Clone,                  0,      INVALID)                        \
    V("has",                    Has,                    1,      INVALID)                        \
    V("getCapacity",            GetCapacity,            0,      INVALID)                        \
    V("increaseCapacityTo",     IncreaseCapacityTo,     1,      INVALID)                        \
    V("trimToCurrentLength",    TrimToCurrentLength,    0,      INVALID)                        \
    V("getIndexOf",             GetIndexOf,             1,      INVALID)                        \
    V("isEmpty",                IsEmpty,                0,      INVALID)                        \
    V("getLastIndexOf",         GetLastIndexOf,         1,      INVALID)                        \
    V("removeByIndex",          RemoveByIndex,          1,      INVALID)                        \
    V("remove",                 Remove,                 1,      INVALID)                        \
    V("removeByRange",          RemoveByRange,          2,      INVALID)                        \
    V("replaceAllElements",     ReplaceAllElements,     2,      ArrayListReplaceAllElements)    \
    V("sort",                   Sort,                   1,      INVALID)                        \
    V("subArrayList",           SubArrayList,           2,      INVALID)                        \
    V("convertToArray",         ConvertToArray,         0,      INVALID)                        \
    V("forEach",                ForEach,                2,      ArrayListForEach)

namespace panda::ecmascript::containers {
/**
 * High performance container interface in jsapi.
 * */
class ContainersArrayList : public base::BuiltinsBase {
public:
    constexpr static uint32_t API18 = 18;
    static JSTaggedValue ArrayListConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Insert(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clone(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetCapacity(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IncreaseCapacityTo(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue TrimToCurrentLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveByIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveByRange(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReplaceAllElements(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SubArrayList(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ConvertToArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetArrayListPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(ARRAYLIST_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_ARRAYLIST_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersArrayList::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array ARRAYLIST_PROTOTYPE_FUNCTIONS = {
        CONTAINER_ARRAYLIST_PROTOTYPE_FUNCTIONS(CONTAINER_ARRAYLIST_FUNCTION_ENTRY)
    };
#undef CONTAINER_ARRAYLIST_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_ARRAYLIST_H
