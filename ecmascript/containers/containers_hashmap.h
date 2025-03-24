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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_HASHMAP_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_HASHMAP_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in HashMap.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersHashMap::func refers to the native implementation of HashMap.prototype[name].
#define CONTAINER_HASHMAP_PROTOTYPE_FUNCTIONS(V)                                    \
    V("set",            Set,            2,          INVALID)                        \
    V("setAll",         SetAll,         1,          INVALID)                        \
    V("isEmpty",        IsEmpty,        0,          INVALID)                        \
    V("remove",         Remove,         1,          INVALID)                        \
    V("clear",          Clear,          0,          INVALID)                        \
    V("get",            Get,            1,          INVALID)                        \
    V("forEach",        ForEach,        2,          HashMapForEach)                 \
    V("hasKey",         HasKey,         1,          INVALID)                        \
    V("hasValue",       HasValue,       1,          INVALID)                        \
    V("replace",        Replace,        2,          INVALID)                        \
    V("keys",           Keys,           0,          INVALID)                        \
    V("values",         Values,         0,          INVALID)                        \
    V("entries",        Entries,        0,          INVALID)

namespace panda::ecmascript::containers {
class ContainersHashMap : public base::BuiltinsBase {
public:
    static JSTaggedValue HashMapConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasKey(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Replace(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SetAll(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetHashMapPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(HASHMAP_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_HASHMAP_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersHashMap::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array HASHMAP_PROTOTYPE_FUNCTIONS = {
        CONTAINER_HASHMAP_PROTOTYPE_FUNCTIONS(CONTAINER_HASHMAP_FUNCTION_ENTRY)
    };
#undef CONTAINER_HASHMAP_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_HASHMAP_H
