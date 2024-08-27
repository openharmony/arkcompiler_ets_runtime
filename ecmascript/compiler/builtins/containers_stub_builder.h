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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/containers_arraylist_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_deque_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_hashmap_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_hashset_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_lightweightmap_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_lightweightset_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_linkedlist_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_list_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_plainarray_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_queue_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_stack_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_vector_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_api/js_api_vector.h"

namespace panda::ecmascript::kungfu {
// enumerate container functions that use function call
enum class ContainersType : uint8_t {
    VECTOR_FOREACH = 0,
    VECTOR_REPLACEALLELEMENTS,
    STACK_FOREACH,
    PLAINARRAY_FOREACH,
    QUEUE_FOREACH,
    DEQUE_FOREACH,
    LIGHTWEIGHTMAP_FOREACH,
    LIGHTWEIGHTSET_FOREACH,
    HASHMAP_FOREACH,
    HASHSET_FOREACH,
    LINKEDLIST_FOREACH,
    LIST_FOREACH,
    ARRAYLIST_FOREACH,
    ARRAYLIST_REPLACEALLELEMENTS,
};

class ContainersStubBuilder : public BuiltinsStubBuilder {
public:
    explicit ContainersStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~ContainersStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersStubBuilder);
    NO_COPY_SEMANTIC(ContainersStubBuilder);
    void GenerateCircuit() override {}

    void ContainersCommonFuncCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    void QueueCommonFuncCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    void DequeCommonFuncCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    void ContainersLightWeightCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    void ContainersHashCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    void ContainersLinkedListCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    GateRef IsContainer(GateRef obj, ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
                return IsJSAPIVector(obj);
            case ContainersType::STACK_FOREACH:
                return IsJSAPIStack(obj);
            case ContainersType::PLAINARRAY_FOREACH:
                return IsJSAPIPlainArray(obj);
            case ContainersType::QUEUE_FOREACH:
                return IsJSAPIQueue(obj);
            case ContainersType::DEQUE_FOREACH:
                return IsJSAPIDeque(obj);
            case ContainersType::LIGHTWEIGHTMAP_FOREACH:
                return IsJSAPILightWeightMap(obj);
            case ContainersType::LIGHTWEIGHTSET_FOREACH:
                return IsJSAPILightWeightSet(obj);
            case ContainersType::HASHMAP_FOREACH:
                return IsJSAPIHashMap(obj);
            case ContainersType::HASHSET_FOREACH:
                return IsJSAPIHashSet(obj);
            case ContainersType::LINKEDLIST_FOREACH:
                return IsJSAPILinkedList(obj);
            case ContainersType::LIST_FOREACH:
                return IsJSAPIList(obj);
            case ContainersType::ARRAYLIST_FOREACH:
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
                return IsJSAPIArrayList(obj);
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return False();
    }

    bool IsReplaceAllElements(ContainersType type)
    {
        switch (type) {
            case ContainersType::STACK_FOREACH:
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::PLAINARRAY_FOREACH:
            case ContainersType::QUEUE_FOREACH:
            case ContainersType::DEQUE_FOREACH:
            case ContainersType::ARRAYLIST_FOREACH:
                return false;
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
                return true;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return false;
    }

    bool IsPlainArray(ContainersType type)
    {
        switch (type) {
            case ContainersType::STACK_FOREACH:
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
            case ContainersType::QUEUE_FOREACH:
            case ContainersType::DEQUE_FOREACH:
            case ContainersType::ARRAYLIST_FOREACH:
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
                return false;
            case ContainersType::PLAINARRAY_FOREACH:
                return true;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return false;
    }

    bool IsArrayListReplaceAllelements(ContainersType type)
    {
        switch (type) {
            case ContainersType::STACK_FOREACH:
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
            case ContainersType::QUEUE_FOREACH:
            case ContainersType::DEQUE_FOREACH:
            case ContainersType::ARRAYLIST_FOREACH:
            case ContainersType::PLAINARRAY_FOREACH:
                return false;
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
                return true;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return false;
    }

    void ContainerSet(GateRef glue, GateRef obj, GateRef index, GateRef value, ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_REPLACEALLELEMENTS: {
                ContainersVectorStubBuilder vectorBuilder(this);
                vectorBuilder.Set(glue, obj, index, value);
                break;
            }
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS: {
                ContainersArrayListStubBuilder arrayListBuilder(this);
                arrayListBuilder.Set(glue, obj, index, value);
                break;
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }

    GateRef ContainerGetSize(GateRef obj, ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS: {
                ContainersVectorStubBuilder vectorBuilder(this);
                return vectorBuilder.GetSize(obj);
            }
            case ContainersType::STACK_FOREACH: {
                ContainersStackStubBuilder stackBuilder(this);
                return stackBuilder.GetSize(obj);
            }
            case ContainersType::PLAINARRAY_FOREACH: {
                ContainersPlainArrayStubBuilder plainArrayBuilder(this);
                return plainArrayBuilder.GetSize(obj);
            }
            case ContainersType::QUEUE_FOREACH: {
                ContainersQueueStubBuilder queueBuilder(this);
                return queueBuilder.GetArrayLength(obj);
            }
            case ContainersType::DEQUE_FOREACH: {
                ContainersDequeStubBuilder dequeBuilder(this);
                return dequeBuilder.GetSize(obj);
            }
            case ContainersType::LIGHTWEIGHTMAP_FOREACH: {
                ContainersLightWeightMapStubBuilder lightWeightMapBuilder(this);
                return lightWeightMapBuilder.GetSize(obj);
            }
            case ContainersType::LIGHTWEIGHTSET_FOREACH: {
                ContainersLightWeightSetStubBuilder lightWeightSetBuilder(this);
                return lightWeightSetBuilder.GetSize(obj);
            }
            case ContainersType::HASHMAP_FOREACH: {
                ContainersHashMapStubBuilder hashMapBuilder(this);
                return hashMapBuilder.GetTableLength(obj);
            }
            case ContainersType::HASHSET_FOREACH: {
                ContainersHashSetStubBuilder hashSetBuilder(this);
                return hashSetBuilder.GetTableLength(obj);
            }
            case ContainersType::LINKEDLIST_FOREACH: {
                ContainersLinkedListStubBuilder linkedListBuilder(this);
                return linkedListBuilder.GetTableLength(obj);
            }
            case ContainersType::LIST_FOREACH: {
                ContainersListStubBuilder listBuilder(this);
                return listBuilder.GetTableLength(obj);
            }
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
            case ContainersType::ARRAYLIST_FOREACH: {
                ContainersArrayListStubBuilder arrayListBuilder(this);
                return arrayListBuilder.GetSize(obj);
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return False();
    }

    GateRef ContainerGetValue(GateRef obj, GateRef index, ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS: {
                ContainersVectorStubBuilder vectorBuilder(this);
                return vectorBuilder.Get(obj, index);
            }
            case ContainersType::STACK_FOREACH: {
                ContainersStackStubBuilder stackBuilder(this);
                return stackBuilder.Get(obj, index);
            }
            case ContainersType::PLAINARRAY_FOREACH: {
                ContainersPlainArrayStubBuilder plainArrayBuilder(this);
                return plainArrayBuilder.Get(obj, index);
            }
            case ContainersType::QUEUE_FOREACH: {
                ContainersQueueStubBuilder queueBuilder(this);
                return queueBuilder.Get(obj, index);
            }
            case ContainersType::DEQUE_FOREACH: {
                ContainersDequeStubBuilder dequeBuilder(this);
                return dequeBuilder.Get(obj, index);
            }
            case ContainersType::LIGHTWEIGHTMAP_FOREACH: {
                ContainersLightWeightMapStubBuilder lightWeightMapBuilder(this);
                return lightWeightMapBuilder.GetValue(obj, index);
            }
            case ContainersType::LIGHTWEIGHTSET_FOREACH: {
                ContainersLightWeightSetStubBuilder lightWeightSetBuilder(this);
                return lightWeightSetBuilder.GetValue(obj, index);
            }
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
            case ContainersType::ARRAYLIST_FOREACH: {
                ContainersArrayListStubBuilder arrayListBuilder(this);
                return arrayListBuilder.Get(obj, index);
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return False();
    }

    GateRef ContainerGetKey(GateRef obj, GateRef index, ContainersType type)
    {
        switch (type) {
            case ContainersType::LIGHTWEIGHTMAP_FOREACH: {
                ContainersLightWeightMapStubBuilder lightWeightMapBuilder(this);
                return lightWeightMapBuilder.GetKey(obj, index);
            }
            case ContainersType::LIGHTWEIGHTSET_FOREACH: {
                ContainersLightWeightSetStubBuilder lightWeightSetBuilder(this);
                return lightWeightSetBuilder.GetKey(obj, index);
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return False();
    }

    GateRef ContainerGetNode(GateRef obj, GateRef index, ContainersType type)
    {
        switch (type) {
            case ContainersType::HASHMAP_FOREACH: {
                ContainersHashMapStubBuilder hashMapBuilder(this);
                return hashMapBuilder.GetNode(obj, index);
            }
            case ContainersType::HASHSET_FOREACH: {
                ContainersHashSetStubBuilder hashSetBuilder(this);
                return hashSetBuilder.GetNode(obj, index);
            }
            case ContainersType::LINKEDLIST_FOREACH: {
                ContainersLinkedListStubBuilder linkedListBuilder(this);
                return linkedListBuilder.GetNode(obj, index);
            }
            case ContainersType::LIST_FOREACH: {
                ContainersListStubBuilder listBuilder(this);
                return listBuilder.GetNode(obj, index);
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return False();
    }

    GateRef PlainArrayGetKey(GateRef obj, GateRef index)
    {
        ContainersPlainArrayStubBuilder plainArrayBuilder(this);
        return plainArrayBuilder.GetKey(obj, index);
    }

    GateRef QueueGetNextPosition(GateRef obj, GateRef index)
    {
        ContainersQueueStubBuilder queueBuilder(this);
        return queueBuilder.GetNextPosition(obj, index);
    }

    GateRef QueueGetCurrentFront(GateRef obj)
    {
        ContainersQueueStubBuilder queueBuilder(this);
        return queueBuilder.GetCurrentFront(obj);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H