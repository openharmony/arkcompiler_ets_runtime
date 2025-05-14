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
class LinkedHashTableStubBuilder : public StubBuilder {
public:
    explicit LinkedHashTableStubBuilder(StubBuilder *parent, GateRef glue)
        : StubBuilder(parent), glue_(glue) {}
    ~LinkedHashTableStubBuilder() override = default;
    NO_MOVE_SEMANTIC(LinkedHashTableStubBuilder);
    NO_COPY_SEMANTIC(LinkedHashTableStubBuilder);
    void GenerateCircuit() override {}

    GateRef Create(GateRef numberOfElements);
    GateRef Clear(GateRef linkedTable);
    GateRef ForEach(GateRef thisValue, GateRef linkedTable, GateRef callbackFnHandle, GateRef thisArg);
    GateRef Insert(GateRef linkedTable, GateRef key, GateRef value);
    GateRef Delete(GateRef linkedTable, GateRef key);
    GateRef Has(GateRef linkedTable, GateRef key);
    GateRef Get(GateRef linkedTable, GateRef key);

    void GenMapSetConstructor(GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs,
        GateRef arg0, GateRef argv);

    GateRef GetLinked(GateRef jsThis);

    void SetLinked(GateRef jsThis, GateRef newTable);

private:
    template <typename IteratorType>
    friend class BuiltinsCollectionIteratorStubBuilder;

    GateRef IsKey(GateRef key)
    {
        return TaggedIsNotHole(key);
    }

    GateRef HashToBucket(GateRef linkedTable, GateRef hash)
    {
        GateRef cap = GetCapacity(linkedTable);
        return Int32And(hash, Int32Sub(cap, Int32(1)));
    }

    GateRef BucketToIndex(GateRef bucket)
    {
        return Int32Add(bucket, Int32(LinkedHashTableType::ELEMENTS_START_INDEX));
    }

    GateRef HashObjectIsMatch(GateRef key, GateRef other);
    GateRef FindElement(GateRef linkedTable, GateRef key, GateRef hash);
    GateRef GetKey(GateRef linkedTable, GateRef entry)
    {
        GateRef index = EntryToIndex(linkedTable, entry);
        return GetElement(linkedTable, index);
    }

    void SetKey(GateRef linkedTable, GateRef entry, GateRef key)
    {
        GateRef index = EntryToIndex(linkedTable, entry);
        SetElement(linkedTable, index, key);
    }

    GateRef GetValue(GateRef linkedTable, GateRef entry)
    {
        GateRef index = EntryToIndex(linkedTable, entry);
        GateRef valueIndex = Int32(LinkedHashTableObject::ENTRY_VALUE_INDEX);
        return GetElement(linkedTable, Int32Add(index, valueIndex));
    }

    void SetValue(GateRef linkedTable, GateRef entry, GateRef value)
    {
        GateRef index = EntryToIndex(linkedTable, entry);
        GateRef valueIndex = Int32(LinkedHashTableObject::ENTRY_VALUE_INDEX);
        SetElement(linkedTable, Int32Add(index, valueIndex), value);
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
        return GetValueFromTaggedArray(glue_, linkedTable, index);
    }

    void SetElement(GateRef linkedTable, GateRef index, GateRef value)
    {
        SetValueToTaggedArray(VariableType::JS_ANY(), glue_, linkedTable, index, value);
    }

    GateRef GetDeletedNum(GateRef linkedTable, GateRef entry)
    {
        return TaggedGetInt(GetNextEntry(linkedTable, entry));
    }

    void SetDeletedNum(GateRef linkedTable, GateRef entry, GateRef num)
    {
        SetNextEntry(linkedTable, entry, IntToTaggedInt(num));
    }

    GateRef GetNextEntry(GateRef linkedTable, GateRef entry)
    {
        GateRef entryIndex = EntryToIndex(linkedTable, entry);
        return GetElement(linkedTable, Int32Add(entryIndex, Int32(LinkedHashTableObject::ENTRY_SIZE)));
    }

    void SetNextEntry(GateRef linkedTable, GateRef entry, GateRef nextEntry)
    {
        GateRef entryIndex = EntryToIndex(linkedTable, entry);
        SetElement(linkedTable, Int32Add(entryIndex, Int32(LinkedHashTableObject::ENTRY_SIZE)), nextEntry);
    }

    GateRef GetCapacity(GateRef linkedTable)
    {
        GateRef capacityIndex = Int32(LinkedHashTableType::CAPACITY_INDEX);
        GateRef capacity = GetValueFromTaggedArray(glue_, linkedTable, capacityIndex);
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
        GateRef tmpNumberOfElements = GetValueFromTaggedArray(glue_, linkedTable, Int32(elementsIndex));
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
        GateRef tmpNumberOfDeletedElements = GetValueFromTaggedArray(glue_, linkedTable, deletedIndex);
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
        return GetValueFromTaggedArray(glue_, linkedTable, nextTableIndex);
    }

    void SetNextTable(GateRef linkedTable, GateRef nexTable)
    {
        GateRef nextTableIndex = Int32(LinkedHashTableType::NEXT_TABLE_INDEX);
        SetValueToTaggedArray(VariableType::JS_POINTER(), glue_, linkedTable, nextTableIndex, nexTable);
    }

    GateRef CalNewTaggedArrayLength(GateRef numberOfElements)
    {
        GateRef startIndex = Int32(LinkedHashTableType::ELEMENTS_START_INDEX);
        GateRef entrySize = Int32(LinkedHashTableObject::ENTRY_SIZE);
        GateRef nEntrySize = Int32Mul(numberOfElements, Int32Add(entrySize, Int32(1)));
        GateRef length = Int32Add(startIndex, Int32Add(numberOfElements, nEntrySize));
        return length;
    }

    void InsertNewEntry(GateRef linkedTable, GateRef bucket, GateRef entry)
    {
        GateRef bucketIndex = BucketToIndex(bucket);
        GateRef previousEntry = GetElement(linkedTable, bucketIndex);
        SetNextEntry(linkedTable, entry, previousEntry);
        SetElement(linkedTable, bucketIndex, IntToTaggedInt(entry));
    }

    GateRef GetDeletedElementsAt(GateRef linkedTable, GateRef entry);
    GateRef GrowCapacity(GateRef linkedTable, GateRef numberOfAddedElements);
    GateRef HasSufficientCapacity(GateRef linkedTable, GateRef numOfAddElements);
    void Rehash(GateRef linkedTable, GateRef newTable);
    GateRef ComputeCapacity(GateRef atLeastSpaceFor);
    void RemoveEntry(GateRef linkedTable, GateRef entry);
    void StoreHashTableToNewObject(GateRef newTargetHClass, Variable& returnValue);
    GateRef GetLinkedOffset();

    GateRef glue_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_LINKED_HASHTABLE_STUB_BUILDER_H
