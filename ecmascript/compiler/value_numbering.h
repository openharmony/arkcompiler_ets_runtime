/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_VALUE_NUMBERING_H
#define ECMASCRIPT_COMPILER_VALUE_NUMBERING_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class ValueNumbering : public PassVisitor {
public:
    ValueNumbering(Circuit *circuit, RPOVisitor *visitor, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor), entries_(chunk) {}

    ~ValueNumbering() = default;

    GateRef VisitGate(GateRef gate) override;
    bool CheckReplacement(GateRef lhs, GateRef rhs);
private:
    size_t HashCode(GateRef gate);
    GateRef GetEntry(size_t hash)
    {
        ASSERT(hash < entries_.size());
        return entries_[hash];
    }
    void SetEntry(size_t hash, GateRef gate)
    {
        ASSERT(hash < entries_.size());
        entries_[hash] = gate;
    }
    static const uint32_t CACHE_LENGTH_BIT = 8;
    static const uint32_t CACHE_LENGTH = (1U << CACHE_LENGTH_BIT);

    ChunkVector<GateRef> entries_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_VALUE_NUMBERING_H