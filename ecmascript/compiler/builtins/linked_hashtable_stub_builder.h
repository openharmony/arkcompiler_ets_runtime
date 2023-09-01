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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_LINKED_HASHTABLE_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_LINKED_HASHTABLE_STUB_BUILDER_H
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {

template<typename LinkedHashTableType, typename LinkedHashTableObject>
class LinkedHashTableStubBuilder : public BuiltinsStubBuilder {
public:
    explicit LinkedHashTableStubBuilder(BuiltinsStubBuilder *parent, GateRef glue)
        : BuiltinsStubBuilder(parent), glue_(glue) {}
    ~LinkedHashTableStubBuilder() override = default;
    NO_MOVE_SEMANTIC(LinkedHashTableStubBuilder);
    NO_COPY_SEMANTIC(LinkedHashTableStubBuilder);
    void GenerateCircuit() override {}

    GateRef Create(int32_t numberOfElements);
    GateRef Clear(GateRef linkedTable);
    GateRef ForEach(GateRef thisValue, GateRef linkedTable, GateRef numArgs);

private:
    GateRef GetKey(GateRef linkedTable, GateRef entry)
    {
        GateRef index = EntryToIndex(linkedTable, entry);
        return GetElement(linkedTable, index);
    }

    GateRef GetValue(GateRef linkedTable, GateRef entry)
    {
        GateRef index = EntryToIndex(linkedTable, entry);
        GateRef valueIndex = Int32(LinkedHashTableObject::ENTRY_VALUE_INDEX);
        return GetElement(linkedTable, Int32Add(index, valueIndex));
    }

    GateRef EntryToIndex(GateRef linkedTable, GateRef entry)
    {
        int32_t startIndex = LinkedHashTableType::ELEMENTS_START_INDEX;
        int32_t entrySize = LinkedHashTableObject::ENTRY_SIZE;
        GateRef sumEntrySize = Int32Mul(entry, Int32Add(Int32(entrySize), Int32(1)));
        return Int32Add(Int32(startIndex), Int32Add(GetCapacity(linkedTable), sumEntrySize));
    }

    GateRef GetElement(GateRef linkedTable, GateRef index)
    {
        return GetValueFromTaggedArray(linkedTable, index);
    }

    GateRef GetDeletedNum(GateRef linkedTable, GateRef entry)
    {
        return TaggedGetInt(GetNextEntry(linkedTable, entry));
    }

    GateRef GetNextEntry(GateRef linkedTable, GateRef entry)
    {
        GateRef entryIndex = EntryToIndex(linkedTable, entry);
        return GetElement(linkedTable, Int32Add(entryIndex, Int32(LinkedHashTableObject::ENTRY_SIZE)));
    }

    GateRef GetCapacity(GateRef linkedTable)
    {
        GateRef capacityIndex = Int32(LinkedHashTableType::CAPACITY_INDEX);
        GateRef capacity = GetValueFromTaggedArray(linkedTable, capacityIndex);
        return TaggedGetInt(capacity);
    }

    void SetCapacity(GateRef linkedTable, GateRef numberOfElements)
    {
        GateRef capacityIndex = Int32(LinkedHashTableType::CAPACITY_INDEX);
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, capacityIndex,
            IntToTaggedInt(numberOfElements));
    }

    GateRef GetNumberOfElements(GateRef linkedTable)
    {
        int32_t elementsIndex = LinkedHashTableType::NUMBER_OF_ELEMENTS_INDEX;
        GateRef tmpNumberOfElements = GetValueFromTaggedArray(linkedTable, Int32(elementsIndex));
        return TaggedGetInt(tmpNumberOfElements);
    }

    void SetNumberOfElements(GateRef linkedTable, GateRef num)
    {
        int32_t elementsIndex = LinkedHashTableType::NUMBER_OF_ELEMENTS_INDEX;
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, Int32(elementsIndex),
            IntToTaggedInt(num));
    }

    GateRef GetNumberOfDeletedElements(GateRef linkedTable)
    {
        GateRef deletedIndex = Int32(LinkedHashTableType::NUMBER_OF_DELETED_ELEMENTS_INDEX);
        GateRef tmpNumberOfDeletedElements = GetValueFromTaggedArray(linkedTable, deletedIndex);
        return TaggedGetInt(tmpNumberOfDeletedElements);
    }

    void SetNumberOfDeletedElements(GateRef linkedTable, GateRef num)
    {
        GateRef deletedIndex = Int32(LinkedHashTableType::NUMBER_OF_DELETED_ELEMENTS_INDEX);
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, deletedIndex, IntToTaggedInt(num));
    }

    GateRef GetNextTable(GateRef linkedTable)
    {
        GateRef nextTableIndex = Int32(LinkedHashTableType::NEXT_TABLE_INDEX);
        return GetValueFromTaggedArray(linkedTable, nextTableIndex);
    }

    void SetNextTable(GateRef linkedTable, GateRef nexTable)
    {
        GateRef nextTableIndex = Int32(LinkedHashTableType::NEXT_TABLE_INDEX);
        SetValueToTaggedArray(VariableType::JS_POINTER(), glue_, linkedTable, nextTableIndex, nexTable);
    }

    GateRef CalNewTaggedArrayLength(int32_t numberOfElements)
    {
        int32_t startIndex = LinkedHashTableType::ELEMENTS_START_INDEX;
        int32_t entrySize = LinkedHashTableObject::ENTRY_SIZE;
        int32_t length = startIndex + numberOfElements + numberOfElements * (entrySize + 1);
        return Int32(length);
    }

    GateRef GetDeletedElementsAt(GateRef linkedTable, GateRef entry);

    GateRef glue_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_LINKED_HASHTABLE_STUB_BUILDER_H
