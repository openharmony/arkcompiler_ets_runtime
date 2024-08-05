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
    ValueNumbering(Circuit *circuit, RPOVisitor *visitor, Chunk* chunk, bool useNewGVN, bool enableLog)
        : PassVisitor(circuit, chunk, visitor), entries_(nullptr), useNewGVN_(useNewGVN),
          enableLog_(enableLog) {}

    ~ValueNumbering() = default;

    GateRef VisitGate(GateRef gate) override;
    bool CheckReplacement(GateRef lhs, GateRef rhs);
    int GetoptimizedGateCount()
    {
        return optimizedGateCount;
    }

private:
    void Grow();
    void EnsureCapacity();
    void InitEntries(size_t initSize);
    size_t HashCode(GateRef gate);
    void SetEntry(size_t hash, GateRef gate)
    {
        ASSERT(entriesLength_ > 0);
        entries_[hash & (entriesLength_ - 1)] = gate;
    }
    static const uint32_t CACHE_LENGTH_BIT = 8;
    static const uint32_t CACHE_LENGTH = (1U << CACHE_LENGTH_BIT);
    const uint8_t LOAD_FACTOR_THRESHOLD = 4;
    uint32_t entriesLength_ = (1U << CACHE_LENGTH_BIT);
    uint32_t entriesSize_ = 0;
    GateRef* entries_;
    bool useNewGVN_;
    int optimizedGateCount = 0;
    bool enableLog_ = false;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_VALUE_NUMBERING_H