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
#include "ecmascript/compiler/value_numbering.h"

namespace panda::ecmascript::kungfu {

GateRef ValueNumbering::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    if (opcode != OpCode::CONVERT) {
        return Circuit::NullGate();
    }
    size_t hash = HashCode(gate);
    if (entries_.size() == 0) {
        entries_.resize(CACHE_LENGTH, Circuit::NullGate());
        SetEntry(hash, gate);
        return Circuit::NullGate();
    }
    GateRef replacement = GetEntry(hash);
    if (replacement != Circuit::NullGate() &&
        CheckReplacement(gate, replacement)) {
        return replacement;
    }
    SetEntry(hash, gate);
    return Circuit::NullGate();
}

size_t ValueNumbering::HashCode(GateRef gate)
{
    size_t hash = static_cast<size_t>(acc_.GetOpCode(gate));
    hash ^= acc_.TryGetValue(gate);
    size_t valueCount = acc_.GetNumValueIn(gate);
    for (size_t i = 0; i < valueCount; i++) {
        GateRef input = acc_.GetValueIn(gate);
        auto id = acc_.GetId(input);
        hash ^= id;
    }
    return hash % CACHE_LENGTH;
}

bool ValueNumbering::CheckReplacement(GateRef lhs, GateRef rhs)
{
    if (!acc_.MetaDataEqu(lhs, rhs)) {
        if (acc_.GetOpCode(lhs) != acc_.GetOpCode(rhs)) {
            return false;
        }
    }
    size_t valueCount = acc_.GetNumValueIn(lhs);
    for (size_t i = 0; i < valueCount; i++) {
        if (acc_.GetValueIn(lhs, i) != acc_.GetValueIn(rhs, i)) {
            return false;
        }
    }
    if (acc_.HasFrameState(lhs)) {
        ASSERT(acc_.HasFrameState(rhs));
        if (acc_.GetFrameState(lhs) != acc_.GetFrameState(rhs)) {
            return false;
        }
    }
    auto opcode = acc_.GetOpCode(lhs);
    if (opcode == OpCode::CONVERT) {
        if (acc_.GetSrcType(lhs) != acc_.GetSrcType(rhs)) {
            return false;
        }
        if (acc_.GetDstType(lhs) != acc_.GetDstType(rhs)) {
            return false;
        }
    }
    return true;
}
}  // namespace panda::ecmascript::kungfu