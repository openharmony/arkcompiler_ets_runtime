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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_QUEUE_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_QUEUE_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in Queue.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where ContainersQueue::func refers to the native implementation of Queue.prototype[name].
#define CONTAINER_QUEUE_PROTOTYPE_FUNCTIONS(V)                                  \
    V("add",                Add,            1,          INVALID)                \
    V("getFirst",           GetFirst,       0,          INVALID)                \
    V("pop",                Pop,            0,          INVALID)                \
    V("forEach",            ForEach,        2,          QueueForEach)

namespace panda::ecmascript::containers {
class ContainersQueue : public base::BuiltinsBase {
public:
    static JSTaggedValue QueueConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetQueuePrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(QUEUE_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_QUEUE_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersQueue::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array QUEUE_PROTOTYPE_FUNCTIONS = {
        CONTAINER_QUEUE_PROTOTYPE_FUNCTIONS(CONTAINER_QUEUE_FUNCTION_ENTRY)
    };
#undef CONTAINER_QUEUE_FUNCTION_ENTRY
};
} // namespace panda::ecmascript::containers
#endif // ECMASCRIPT_CONTAINERS_CONTAINERS_QUEUE_H
