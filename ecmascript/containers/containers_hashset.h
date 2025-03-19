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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_HASHSET_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_HASHSET_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in HashSet.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersHashSet::func refers to the native implementation of HashSet.prototype[name].
#define CONTAINER_HASHSET_PROTOTYPE_FUNCTIONS(V)                                    \
    V("isEmpty",        IsEmpty,        0,          INVALID)                        \
    V("has",            Has,            1,          INVALID)                        \
    V("add",            Add,            1,          INVALID)                        \
    V("remove",         Remove,         1,          INVALID)                        \
    V("clear",          Clear,          0,          INVALID)                        \
    V("values",         Values,         0,          INVALID)                        \
    V("entries",        Entries,        0,          INVALID)                        \
    V("forEach",        ForEach,        2,          HashSetForEach)

namespace panda::ecmascript::containers {
class ContainersHashSet : public base::BuiltinsBase {
public:
    static JSTaggedValue HashSetConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetHashSetPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(HASHSET_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_HASHSET_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersHashSet::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array HASHSET_PROTOTYPE_FUNCTIONS = {
        CONTAINER_HASHSET_PROTOTYPE_FUNCTIONS(CONTAINER_HASHSET_FUNCTION_ENTRY)
    };
#undef CONTAINER_HASHSET_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_HASHSET_H
