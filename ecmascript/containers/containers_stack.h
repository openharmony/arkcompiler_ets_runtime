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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_STACK_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_STACK_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in Stack.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersStack::func refers to the native implementation of Stack.prototype[name].
#define CONTAINER_STACK_PROTOTYPE_FUNCTIONS(V)                                  \
    V("push",               Push,           1,          INVALID)                \
    V("isEmpty",            IsEmpty,        0,          INVALID)                \
    V("peek",               Peek,           0,          INVALID)                \
    V("pop",                Pop,            0,          INVALID)                \
    V("locate",             Locate,         1,          INVALID)                \
    V("forEach",            ForEach,        2,          StackForEach)

namespace panda::ecmascript::containers {
class ContainersStack : public base::BuiltinsBase {
public:
    static JSTaggedValue StackConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Iterator(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Push(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Peek(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Locate(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetStackPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(STACK_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_STACK_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersStack::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array STACK_PROTOTYPE_FUNCTIONS = {
        CONTAINER_STACK_PROTOTYPE_FUNCTIONS(CONTAINER_STACK_FUNCTION_ENTRY)
    };
#undef CONTAINER_STACK_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_STACK_H
