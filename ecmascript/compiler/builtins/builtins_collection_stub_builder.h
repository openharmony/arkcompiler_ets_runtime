/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_COLLECTION_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_COLLECTION_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"

namespace panda::ecmascript::kungfu {
template <typename CollectionType>
class BuiltinsCollectionStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsCollectionStubBuilder(BuiltinsStubBuilder *parent, GateRef glue, GateRef thisValue,
        GateRef numArgs) : BuiltinsStubBuilder(parent), glue_(glue), thisValue_(thisValue), numArgs_(numArgs) {}
    ~BuiltinsCollectionStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsCollectionStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsCollectionStubBuilder);
    void GenerateCircuit() override {}

    void Clear(Variable *result, Label *exit, Label *slowPath);
    void Values(Variable *result, Label *exit, Label *slowPath);
    void Entries(Variable *result, Label *exit, Label *slowPath);
    void Keys(Variable *result, Label *exit, Label *slowPath);
    void ForEach(Variable *result, Label *exit, Label *slowPath);
    void Set(Variable *result, Label *exit, Label *slowPath);
    void Add(Variable *result, Label *exit, Label *slowPath);
    void Delete(Variable *result, Label *exit, Label *slowPath);
    void Has(Variable *result, Label *exit, Label *slowPath);
    void Get(Variable *result, Label *exit, Label *slowPath);

private:
    // check target obj
    void CheckCollectionObj(Label *exit, Label *slowPath);
    void CreateIterator(Variable *result, Label *exit, Label *slowPath, GateRef iterationKind);
    void MapSetOrSetAdd(Variable *result, Label *exit, Label *slowPath, bool isJsMapSet);

    GateRef GetLinkedOffset()
    {
        int32_t linkedTableOffset = 0;
        if constexpr (std::is_same_v<CollectionType, JSMap>) {
            linkedTableOffset = CollectionType::LINKED_MAP_OFFSET;
        } else {
            linkedTableOffset = CollectionType::LINKED_SET_OFFSET;
        }
        return IntPtr(linkedTableOffset);
    }

    GateRef GetLinked()
    {
        GateRef linkedTableOffset = GetLinkedOffset();
        return Load(VariableType::JS_ANY(), thisValue_, linkedTableOffset);
    }

    void SetLinked(GateRef newTable)
    {
        GateRef linkedTableOffset = GetLinkedOffset();
        Store(VariableType::JS_ANY(), glue_, thisValue_, linkedTableOffset, newTable);
    }

    GateRef glue_;
    GateRef thisValue_;
    GateRef numArgs_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_COLLECTION_STUB_BUILDER_H