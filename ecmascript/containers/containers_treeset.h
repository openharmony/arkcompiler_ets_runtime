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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_TREESET_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_TREESET_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in TreeSet.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersTreeSet::func refers to the native implementation of TreeSet.prototype[name].
#define CONTAINER_TREESET_PROTOTYPE_FUNCTIONS(V)                                \
    V("add",                    Add,                1,          INVALID)        \
    V("remove",                 Remove,             1,          INVALID)        \
    V("has",                    Has,                1,          INVALID)        \
    V("getFirstValue",          GetFirstValue,      0,          INVALID)        \
    V("getLastValue",           GetLastValue,       0,          INVALID)        \
    V("clear",                  Clear,              0,          INVALID)        \
    V("getLowerValue",          GetLowerValue,      1,          INVALID)        \
    V("getHigherValue",         GetHigherValue,     1,          INVALID)        \
    V("popFirst",               PopFirst,           0,          INVALID)        \
    V("popLast",                PopLast,            0,          INVALID)        \
    V("isEmpty",                IsEmpty,            0,          INVALID)        \
    V("values",                 Values,             0,          INVALID)        \
    V("forEach",                ForEach,            2,          INVALID)        \
    V("entries",                Entries,            0,          INVALID)

namespace panda::ecmascript::containers {
/**
 * High performance container interface in jsapi.
 * TreeSet provides ordered sets.
 * */
class ContainersTreeSet : public base::BuiltinsBase {
public:
    static JSTaggedValue TreeSetConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetFirstValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastValue(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetLowerValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetHigherValue(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue PopFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue PopLast(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetTreeSetPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(TREESET_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_TREESET_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersTreeSet::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array TREESET_PROTOTYPE_FUNCTIONS = {
        CONTAINER_TREESET_PROTOTYPE_FUNCTIONS(CONTAINER_TREESET_FUNCTION_ENTRY)
    };
#undef CONTAINER_TREESET_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_TREESET_H_
