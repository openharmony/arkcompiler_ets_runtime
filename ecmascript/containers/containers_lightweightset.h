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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_LIGHT_WEIGHT_SET_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_LIGHT_WEIGHT_SET_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in LightWeightSet.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersLightWeightSet::func refers to the native implementation of LightWeightSet.prototype[name].
#define CONTAINER_LIGHTWEIGHTSET_PROTOTYPE_FUNCTIONS(V)                                             \
    V("add",                    Add,                    1,          INVALID)                        \
    V("addAll",                 AddAll,                 1,          INVALID)                        \
    V("isEmpty",                IsEmpty,                0,          INVALID)                        \
    V("getValueAt",             GetValueAt,             1,          INVALID)                        \
    V("hasAll",                 HasAll,                 1,          INVALID)                        \
    V("has",                    Has,                    1,          INVALID)                        \
    V("equal",                  Equal,                  1,          INVALID)                        \
    V("increaseCapacityTo",     IncreaseCapacityTo,     1,          INVALID)                        \
    V("forEach",                ForEach,                2,          LightWeightSetForEach)          \
    V("getIndexOf",             GetIndexOf,             1,          INVALID)                        \
    V("remove",                 Remove,                 1,          INVALID)                        \
    V("removeAt",               RemoveAt,               1,          INVALID)                        \
    V("clear",                  Clear,                  0,          INVALID)                        \
    V("toString",               ToString,               0,          INVALID)                        \
    V("toArray",                ToArray,                0,          INVALID)                        \
    V("values",                 Values,                 0,          INVALID)                        \
    V("entries",                Entries,                0,          INVALID)

namespace panda::ecmascript::containers {
class ContainersLightWeightSet : public base::BuiltinsBase {
public:
    static JSTaggedValue LightWeightSetConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue AddAll(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetValueAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasAll(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasHash(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Equal(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IncreaseCapacityTo(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetLightWeightSetPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(LIGHTWEIGHTSET_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_LIGHTWEIGHTSET_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersLightWeightSet::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array LIGHTWEIGHTSET_PROTOTYPE_FUNCTIONS = {
        CONTAINER_LIGHTWEIGHTSET_PROTOTYPE_FUNCTIONS(CONTAINER_LIGHTWEIGHTSET_FUNCTION_ENTRY)
    };
#undef CONTAINER_LIGHTWEIGHTSET_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_LIGHT_WEIGHT_SET_H
