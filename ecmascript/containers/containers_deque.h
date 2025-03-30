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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_DEQUE_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_DEQUE_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in Deque.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersDeque::func refers to the native implementation of Deque.prototype[name].
#define CONTAINER_DEQUE_PROTOTYPE_FUNCTIONS(V)                                                  \
    V("insertFront",            InsertFront,        1,          INVALID)                        \
    V("insertEnd",              InsertEnd,          1,          INVALID)                        \
    V("getFirst",               GetFirst,           0,          INVALID)                        \
    V("getLast",                GetLast,            0,          INVALID)                        \
    V("has",                    Has,                1,          INVALID)                        \
    V("popFirst",               PopFirst,           0,          INVALID)                        \
    V("popLast",                PopLast,            0,          INVALID)                        \
    V("forEach",                ForEach,            2,          DequeForEach)

namespace panda::ecmascript::containers {
class ContainersDeque : public base::BuiltinsBase {
public:
    static JSTaggedValue DequeConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue InsertFront(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue InsertEnd(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLast(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue PopFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue PopLast(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetDequePrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(DEQUE_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_DEQUE_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersDeque::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array DEQUE_PROTOTYPE_FUNCTIONS = {
        CONTAINER_DEQUE_PROTOTYPE_FUNCTIONS(CONTAINER_DEQUE_FUNCTION_ENTRY)
    };
#undef CONTAINER_DEQUE_FUNCTION_ENTRY
};
} // namespace panda::ecmascript::containers
#endif // ECMASCRIPT_CONTAINERS_CONTAINERS_DEQUE_H
