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
#include <cstddef>
    
namespace panda::ecmascript::kungfu {

GateRef ValueNumbering::VisitGate(GateRef gate)
{
    size_t hash = HashCode(gate);
    auto opcode = acc_.GetOpCode(gate);
    if (opcode != OpCode::CONVERT && !useNewGVN_) {
        return Circuit::NullGate();
    }
    if (acc_.GetStateCount(gate) > 0 || acc_.GetDependCount(gate) > 0) {
        return Circuit::NullGate();
    }
    
    if (entries_ == nullptr) {
        InitEntries(entriesLength_);
        SetEntry(hash, gate);
        entriesSize_++;
        return Circuit::NullGate();
    }
    ASSERT(entriesSize_ + entriesSize_ / LOAD_FACTOR_THRESHOLD < entriesLength_ && entriesLength_ >= 1);
    const size_t mask = entriesLength_ - 1;
    size_t dead = entriesLength_;
    for (size_t i = hash & mask;; i = (i + 1) & mask) {
        GateRef entry = entries_[i];
        if (entry == Circuit::NullGate()) {
            if (dead != entriesLength_) {
                entries_[dead] = gate;
            } else {
                // Have to insert a new entry.
                entries_[i] = gate;
                entriesSize_++;
                // Resize to keep load factor below 80%
                EnsureCapacity();
            }
            ASSERT(entriesSize_ + entriesSize_ / LOAD_FACTOR_THRESHOLD < entriesLength_);
            return Circuit::NullGate();
        }

        if (entry == gate) {
            return Circuit::NullGate();
        }

        // Skip dead entries, but remember their indices so we can reuse them.
        if (acc_.IsNop(entry)) {
            dead = i;
            continue;
        }

        if (CheckReplacement(gate, entry)) {
            optimizedGateCount++;
            if (enableLog_) {
                LOG_COMPILER(INFO) << "Found a replaceable node, before -> after";
                acc_.Print(gate);
                acc_.Print(entry);
            }
            return entry;
        }
    }
    return Circuit::NullGate();
}

void ValueNumbering::EnsureCapacity()
{
    if (entriesSize_ + entriesSize_ / LOAD_FACTOR_THRESHOLD >= entriesLength_) {
        Grow();
    }
}

void ValueNumbering::Grow()
{
    GateRef *const oldEntries = entries_;
    size_t const oldSize = entriesLength_;
    entriesLength_ *= 2; // 2 : entriesLength
    InitEntries(entriesLength_);
    ASSERT(entriesLength_ > 0);
    size_t const mask = entriesLength_ - 1;

    for (size_t i = 0; i < oldSize; i++) {
        GateRef oldEnrty = oldEntries[i];
        if (oldEnrty == Circuit::NullGate() || acc_.IsNop(oldEnrty)) {
            continue;
        }
        for (size_t j = HashCode(oldEnrty) & mask;; j = (j + 1) & mask) {
            GateRef entry = entries_[j];
            if (entry == oldEnrty) {
                break;
            }
            if (entry == Circuit::NullGate()) {
                entries_[j] = oldEnrty;
                entriesSize_++;
                break;
            }
        }
    }
    chunk_->Free(oldEntries);
    if (enableLog_) {
        LOG_COMPILER(INFO) << "Grow happend";
    }
}

void ValueNumbering::InitEntries(size_t initSize)
{
    entries_ = chunk_->NewArray<int32_t>(initSize);
    for (size_t i = 0; i < initSize; i++) {
        entries_[i] = Circuit::NullGate();
    }
    entriesSize_ = 0;
}


size_t HashCombine(size_t seed, size_t value)
{
    // In the meantime, we're not considering 32-bit systems
    assert(sizeof(void *) == 8); // 8 : make sure the void* pointer is 8 bytes in size
    const uint64_t m = uint64_t{0xC6A4A7935BD1E995};
    const uint32_t r = 47;

    value *= m;
    value ^= value >> r;
    value *= m;

    seed ^= value;
    seed *= m;
    return seed;
}

size_t ValueNumbering::HashCode(GateRef gate)
{
    size_t valueCount = acc_.GetNumValueIn(gate);
    size_t hash = HashCombine(static_cast<size_t>(acc_.GetOpCode(gate)), valueCount);
    for (size_t i = 0; i < valueCount; i++) {
        GateRef input = acc_.GetValueIn(gate, i);
        auto id = acc_.GetId(input);
        hash = HashCombine(hash, id);
    }
    return hash;
}

// Gates are considered replaceable only when their values are identical
bool ValueNumbering::CheckReplacement(GateRef lhs, GateRef rhs)
{
    if (acc_.GetOpCode(lhs) != acc_.GetOpCode(rhs))
        return false;

    size_t valueCount1 = acc_.GetNumIns(lhs);
    size_t valueCount2 = acc_.GetNumIns(rhs);
    if (valueCount1 != valueCount2)
        return false;
    for (size_t i = 0; i < valueCount1; i++) {
        if (acc_.GetIn(lhs, i) != acc_.GetIn(rhs, i)) {
            return false;
        }
    }

    if (acc_.GetGateType(lhs) != acc_.GetGateType(rhs)) {
        return false;
    }

    if (acc_.GetMachineType(lhs) != acc_.GetMachineType(rhs)) {
        return false;
    }

    if (!acc_.MetaDataValueEqu(lhs, rhs)) {
        return false;
    }

    return true;
}
} // namespace panda::ecmascript::kungfu