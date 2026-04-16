/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_SHARED_LINKED_HASHTABLE_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_SHARED_LINKED_HASHTABLE_STUB_BUILDER_H
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
template<typename LinkedHashTableType, typename LinkedHashTableObject>
class SharedLinkedHashTableStubBuilder : public StubBuilder {
public:
    explicit SharedLinkedHashTableStubBuilder(StubBuilder *parent, GateRef glue, GateRef globalEnv)
        : StubBuilder(parent, globalEnv), glue_(glue) {}
    ~SharedLinkedHashTableStubBuilder() override = default;
    NO_MOVE_SEMANTIC(SharedLinkedHashTableStubBuilder);
    NO_COPY_SEMANTIC(SharedLinkedHashTableStubBuilder);
    void GenerateCircuit() override {}

    GateRef Create(GateRef numberOfElements);

    void GenSharedMapSetConstructor(GateRef nativeCode, GateRef func, GateRef newTarget,
        GateRef thisValue, GateRef numArgs, GateRef arg0, GateRef argv);

private:
    GateRef CalNewTaggedArrayLength(GateRef numberOfElements)
    {
        GateRef startIndex = Int32(LinkedHashTableType::ELEMENTS_START_INDEX);
        GateRef entrySize = Int32(LinkedHashTableObject::ENTRY_SIZE);
        GateRef nEntrySize = Int32Mul(numberOfElements, Int32Add(entrySize, Int32(1)));
        GateRef length = Int32Add(startIndex, Int32Add(numberOfElements, nEntrySize));
        return length;
    }

    void SetNumberOfElements(GateRef linkedTable, GateRef num)
    {
        int32_t elementsIndex = LinkedHashTableType::NUMBER_OF_ELEMENTS_INDEX;
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, Int32(elementsIndex),
            IntToTaggedInt(num));
    }

    void SetNumberOfDeletedElements(GateRef linkedTable, GateRef num)
    {
        GateRef deletedIndex = Int32(LinkedHashTableType::NUMBER_OF_DELETED_ELEMENTS_INDEX);
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, deletedIndex, IntToTaggedInt(num));
    }

    void SetCapacity(GateRef linkedTable, GateRef numberOfElements)
    {
        GateRef capacityIndex = Int32(LinkedHashTableType::CAPACITY_INDEX);
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, capacityIndex,
            IntToTaggedInt(numberOfElements));
    }

    void StoreHashTableToNewObject(GateRef newTargetHClass, Variable& returnValue);

    GateRef glue_;
};

}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_SHARED_LINKED_HASHTABLE_STUB_BUILDER_H
