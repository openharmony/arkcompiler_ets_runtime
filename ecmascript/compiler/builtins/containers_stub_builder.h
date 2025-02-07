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
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_stack.h"
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

class ContainersCommonStubBuilder : public BuiltinsStubBuilder {
public:
    explicit ContainersCommonStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~ContainersCommonStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersCommonStubBuilder);
    NO_COPY_SEMANTIC(ContainersCommonStubBuilder);
    void GenerateCircuit() override {}

    void ContainersCommonFuncCall(GateRef glue, GateRef thisValue, GateRef numArgs,
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

    GateRef ContainerGetSize(GateRef obj, ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS: {
                return Load(VariableType::INT32(), obj, IntPtr(JSAPIVector::ELEMENT_COUNT_OFFSET));
            }
            case ContainersType::STACK_FOREACH: {
                GateRef top = Load(VariableType::INT32(), obj, IntPtr(JSAPIStack::TOP_OFFSET));
                return Int32Add(top, Int32(1));
            }
            case ContainersType::PLAINARRAY_FOREACH: {
                return Load(VariableType::INT32(), obj, IntPtr(JSAPIPlainArray::LENGTH_OFFSET));
            }
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
            case ContainersType::ARRAYLIST_FOREACH: {
                GateRef len = Load(VariableType::JS_ANY(), obj, IntPtr(JSAPIArrayList::LENGTH_OFFSET));
                return TaggedGetInt(len);
            }
            case ContainersType::LIGHTWEIGHTSET_FOREACH: {
                return Load(VariableType::INT32(), obj, IntPtr(JSAPILightWeightSet::LENGTH_OFFSET));
            }
            case ContainersType::LIGHTWEIGHTMAP_FOREACH: {
                return Load(VariableType::INT32(), obj, IntPtr(JSAPILightWeightMap::LWP_LENGTH_OFFSET));
            }
            case ContainersType::HASHMAP_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPIHashMap::HASHMAP_TABLE_INDEX);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                return GetLengthOfTaggedArray(table);
            }
            case ContainersType::HASHSET_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPIHashSet::HASHSET_TABLE_INDEX);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                return GetLengthOfTaggedArray(table);
            }
            case ContainersType::LINKEDLIST_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPILinkedList::DOUBLE_LIST_OFFSET);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                GateRef value = GetValueFromTaggedArray(table, Int32(TaggedDoubleList::NUMBER_OF_NODE_INDEX));
                return TaggedGetInt(value);
            }
            case ContainersType::LIST_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPIList::SINGLY_LIST_OFFSET);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                GateRef value = GetValueFromTaggedArray(table, Int32(TaggedSingleList::NUMBER_OF_NODE_INDEX));
                return TaggedGetInt(value);
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
                GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
                GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
                return GetValueFromTaggedArray(elements, index);
            }
            case ContainersType::STACK_FOREACH: {
                GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
                GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
                return GetValueFromTaggedArray(elements, index);
            }
            case ContainersType::PLAINARRAY_FOREACH: {
                GateRef elementsOffset = IntPtr(JSAPIPlainArray::VALUES_OFFSET);
                GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
                return GetValueFromTaggedArray(elements, index);
            }
            case ContainersType::ARRAYLIST_REPLACEALLELEMENTS:
            case ContainersType::ARRAYLIST_FOREACH: {
                GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
                GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
                return GetValueFromTaggedArray(elements, index);
            }
            case ContainersType::LIGHTWEIGHTSET_FOREACH: {
                GateRef valuesOffset = IntPtr(JSAPILightWeightSet::VALUES_OFFSET);
                GateRef values = Load(VariableType::JS_POINTER(), obj, valuesOffset);
                return GetValueFromTaggedArray(values, index);
            }
            case ContainersType::LIGHTWEIGHTMAP_FOREACH: {
                GateRef valuesOffset = IntPtr(JSAPILightWeightMap::LWP_VALUES_OFFSET);
                GateRef values = Load(VariableType::JS_POINTER(), obj, valuesOffset);
                return GetValueFromTaggedArray(values, index);
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
                GateRef keysOffset = IntPtr(JSAPILightWeightMap::LWP_KEYS_OFFSET);
                GateRef keys = Load(VariableType::JS_POINTER(), obj, keysOffset);
                return GetValueFromTaggedArray(keys, index);
            }
            case ContainersType::LIGHTWEIGHTSET_FOREACH: {
                GateRef valuesOffset = IntPtr(JSAPILightWeightSet::VALUES_OFFSET);
                GateRef values = Load(VariableType::JS_POINTER(), obj, valuesOffset);
                return GetValueFromTaggedArray(values, index);
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
                GateRef tableOffset = IntPtr(JSAPIHashMap::HASHMAP_TABLE_INDEX);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                return GetValueFromTaggedArray(table, index);
            }
            case ContainersType::HASHSET_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPIHashSet::HASHSET_TABLE_INDEX);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                return GetValueFromTaggedArray(table, index);
            }
            case ContainersType::LINKEDLIST_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPILinkedList::DOUBLE_LIST_OFFSET);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                return GetValueFromTaggedArray(table, index);
            }
            case ContainersType::LIST_FOREACH: {
                GateRef tableOffset = IntPtr(JSAPIList::SINGLY_LIST_OFFSET);
                GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
                return GetValueFromTaggedArray(table, index);
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return False();
    }

    GateRef PlainArrayGetKey(GateRef obj, GateRef index)
    {
        GateRef elementsOffset = IntPtr(JSAPIPlainArray::KEYS_OFFSET);
        GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
        return GetValueFromTaggedArray(elements, index);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H
