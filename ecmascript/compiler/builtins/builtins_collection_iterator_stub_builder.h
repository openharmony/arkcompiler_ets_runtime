/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_COLLECTION_ITERATOR_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_COLLECTION_ITERATOR_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript::kungfu {
template <typename IteratorType>
class BuiltinsCollectionIteratorStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsCollectionIteratorStubBuilder(StubBuilder *parent, GateRef glue, GateRef thisValue,
        GateRef numArgs, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv), glue_(glue), thisValue_(thisValue), numArgs_(numArgs) {}
    ~BuiltinsCollectionIteratorStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsCollectionIteratorStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsCollectionIteratorStubBuilder);
    void GenerateCircuit() override {}

    void Next(Variable *result, Label *exit);

private:
    GateRef CreateIterValueForEntry(GateRef key, GateRef value);
    // check target obj
    void CheckCollectionIteratorObj(Label *updateIter, Label *throwException);
    void UpdateIter(Label *iterUpdated);

    GateRef GetIteratedLinkedObjOffset()
    {
        int32_t linkedObjOffset = 0;
        if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
            linkedObjOffset = JSMapIterator::ITERATED_MAP_OFFSET;
        } else {
            linkedObjOffset = JSSetIterator::ITERATED_SET_OFFSET;
        }
        return IntPtr(linkedObjOffset);
    }

    GateRef GetIteratedLinkedObj()
    {
        GateRef iteratedLinkedObjOffset = GetIteratedLinkedObjOffset();
        return Load(VariableType::JS_ANY(), glue_, thisValue_, iteratedLinkedObjOffset);
    }

    void SetIteratedLinkedObj(GateRef newLinkedObj)
    {
        GateRef iteratedLinkedObjOffset = GetIteratedLinkedObjOffset();
        Store(VariableType::JS_ANY(), glue_, thisValue_, iteratedLinkedObjOffset, newLinkedObj);
    }

    GateRef GetNextIndexOffset()
    {
        return IntPtr(IteratorType::NEXT_INDEX_OFFSET);
    }

    GateRef GetNextIndex()
    {
        GateRef nextIndexOffset = GetNextIndexOffset();
        return LoadPrimitive(VariableType::INT32(), thisValue_, nextIndexOffset);
    }

    void SetNextIndex(GateRef newNextIndex)
    {
        GateRef nextIndexOffset = GetNextIndexOffset();
        Store(VariableType::INT32(), glue_, thisValue_, nextIndexOffset, newNextIndex);
    }

    GateRef GetIterationKind()
    {
        GateRef bitFieldOffset = IntPtr(IteratorType::BIT_FIELD_OFFSET);
        GateRef bitField = LoadPrimitive(VariableType::INT32(), thisValue_, bitFieldOffset);
        // decode
        GateRef mask = Int32((1LLU << IteratorType::ITERATION_KIND_BITS) - 1);
        return Int32And(bitField, mask);
    }

    GateRef GetNextTable(GateRef iteratedLinkedObj)
    {
        if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
            LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
            return linkedHashTableStubBuilder.GetNextTable(iteratedLinkedObj);
        } else {
            LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
            return linkedHashTableStubBuilder.GetNextTable(iteratedLinkedObj);
        }
    }

    GateRef glue_;
    GateRef thisValue_;
    GateRef numArgs_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_COLLECTION_ITERATOR_STUB_BUILDER_H