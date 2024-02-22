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
#ifndef MAPLEBE_INCLUDE_CG_SCHEDULE_HEURISTIC_H
#define MAPLEBE_INCLUDE_CG_SCHEDULE_HEURISTIC_H

#include "deps.h"

// Define a series of priority comparison function objects.
// @ReturnValue:
//    - positive: node1 has higher priority
//    - negative: node2 has higher priority
//    - zero: node1 == node2
// And ensure the sort is stable.
namespace maplebe {

// Prefer max delay priority
class CompareDelay {
public:
    int operator()(const DepNode &node1, const DepNode &node2) const
    {
        return static_cast<int>(node1.GetDelay() - node2.GetDelay());
    }
};

// To make better use of cpu cache prefetch:
// (1) If both insns are memory operation and have same base address (same base register) and same access
//     byte, prefer lowest offset.
// The following (2) and (3) are not implemented currently, and can be optimized in the future:
// (2) If one insn is store and the other is not memory operation, prefer non-memory-insn scheduled before store.
// (3) If one insn is load and the other is not memory operation, prefer load scheduled before non-memory-insn.
class CompareDataCache {
public:
    int operator()(const DepNode &node1, const DepNode &node2)
    {
        Insn *insn1 = node1.GetInsn();
        ASSERT_NOT_NULL(insn1);
        Insn *insn2 = node2.GetInsn();
        ASSERT_NOT_NULL(insn2);
        // Exclude adrp
        if (insn1->IsMemAccess() && insn2->IsMemAccess()) {
            auto *memOpnd1 = static_cast<MemOperand *>(insn1->GetMemOpnd());
            auto *memOpnd2 = static_cast<MemOperand *>(insn2->GetMemOpnd());
            // Need the same access byte
            if ((insn1->IsLoadStorePair() && !insn2->IsLoadStorePair()) ||
                (!insn1->IsLoadStorePair() && insn2->IsLoadStorePair())) {
                return 0;
            }
            if (memOpnd1 != nullptr && memOpnd2 != nullptr) {  // Exclude load address
                if (memOpnd1->GetAddrMode() == MemOperand::kAddrModeBOi &&
                    memOpnd2->GetAddrMode() == MemOperand::kAddrModeBOi) {
                    RegOperand *baseOpnd1 = memOpnd1->GetBaseRegister();
                    ASSERT_NOT_NULL(baseOpnd1);
                    RegOperand *baseOpnd2 = memOpnd2->GetBaseRegister();
                    ASSERT_NOT_NULL(baseOpnd2);
                    if (baseOpnd1->GetRegisterNumber() == baseOpnd2->GetRegisterNumber()) {
                        ImmOperand *ofstOpnd1 = memOpnd1->GetOffsetOperand();
                        ImmOperand *ofstOpnd2 = memOpnd2->GetOffsetOperand();
                        if (ofstOpnd1 == nullptr && ofstOpnd2 == nullptr) {
                            return 0;
                        } else if (ofstOpnd1 == nullptr) {
                            return 1;
                        } else if (ofstOpnd2 == nullptr) {
                            return -1;
                        } else {
                            return static_cast<int>(ofstOpnd2->GetValue() - ofstOpnd1->GetValue());
                        }
                    }
                } else {
                    // Schedule load before store
                    if ((insn1->IsLoad() || insn1->IsLoadPair()) && (insn2->IsStore() || insn2->IsStorePair())) {
                        return 1;
                    } else if ((insn2->IsLoad() || insn2->IsLoadPair()) && (insn1->IsStore() || insn1->IsStorePair())) {
                        return -1;
                    }
                }
            }
        } else if (enableIrrelevant && insn1->IsMemAccess()) {
            return (insn1->IsLoad() || insn1->IsLoadPair()) ? 1 : -1;
        } else if (enableIrrelevant && insn2->IsMemAccess()) {
            return (insn2->IsLoad() || insn2->IsLoadPair()) ? -1 : 1;
        }
        return 0;
    }

    bool enableIrrelevant = false;
};

// Prefer the class of the edges of two insn and last scheduled insn with the highest class number:
// 1. true dependency
// 2. anti/output dependency
// 3. the rest of the dependency
// 4. independent of last scheduled insn
class CompareClassOfLastScheduledNode {
public:
    explicit CompareClassOfLastScheduledNode(uint32 lsid) : lastSchedInsnId(lsid) {}
    ~CompareClassOfLastScheduledNode() = default;

    int operator()(const DepNode &node1, const DepNode &node2) const
    {
        DepType type1 = kDependenceTypeNone;
        DepType type2 = kDependenceTypeNone;
        for (auto *predLink : node1.GetPreds()) {
            DepNode &predNode = predLink->GetFrom();
            if (lastSchedInsnId != 0 && predNode.GetInsn()->GetId() == lastSchedInsnId) {
                type1 = predLink->GetDepType();
            }
        }
        for (auto *predLink : node2.GetPreds()) {
            DepNode &predNode = predLink->GetFrom();
            if (lastSchedInsnId != 0 && predNode.GetInsn()->GetId() == lastSchedInsnId) {
                type2 = predLink->GetDepType();
            }
        }
        CHECK_FATAL(type1 != kDependenceTypeSeparator && type2 != kDependenceTypeSeparator, "invalid dependency type");

        uint32 typeClass1 = 0;
        switch (type1) {
            case kDependenceTypeTrue:
                typeClass1 = kNumOne;
                break;
            case kDependenceTypeAnti:
            case kDependenceTypeOutput:
                typeClass1 = kNumTwo;
                break;
            case kDependenceTypeSeparator:
                CHECK_FATAL(false, "invalid dependency type");
                break;
            case kDependenceTypeNone:
                typeClass1 = kNumFour;
                break;
            default:
                typeClass1 = kNumThree;
                break;
        }

        uint32 typeClass2 = 0;
        switch (type2) {
            case kDependenceTypeTrue:
                typeClass2 = kNumOne;
                break;
            case kDependenceTypeAnti:
            case kDependenceTypeOutput:
                typeClass2 = kNumTwo;
                break;
            case kDependenceTypeSeparator:
                CHECK_FATAL(false, "invalid dependency type");
            case kDependenceTypeNone:
                typeClass2 = kNumFour;
                break;
            default:
                typeClass2 = kNumThree;
                break;
        }

        return static_cast<int>(typeClass1 - typeClass2);
    }

    uint32 lastSchedInsnId = 0;
};

// Prefer min eStart
class CompareEStart {
public:
    int operator()(const DepNode &node1, const DepNode &node2) const
    {
        return static_cast<int>(node2.GetEStart() - node1.GetEStart());
    }
};

// Prefer min lStart
class CompareLStart {
public:
    int operator()(const DepNode &node1, const DepNode &node2) const
    {
        return static_cast<int>(node2.GetLStart() - node1.GetLStart());
    }
};

// Prefer max cost of insn
class CompareCost {
public:
    int operator()(const DepNode &node1, const DepNode &node2)
    {
        return (node1.GetReservation()->GetLatency() - node2.GetReservation()->GetLatency());
    }
};

// Prefer using more unit kind
class CompareUnitKindNum {
public:
    explicit CompareUnitKindNum(uint32 maxUnitIndex) : maxUnitIdx(maxUnitIndex) {}

    int operator()(const DepNode &node1, const DepNode &node2) const
    {
        bool use1 = IsUseUnitKind(node1);
        bool use2 = IsUseUnitKind(node2);
        if ((use1 && use2) || (!use1 && !use2)) {
            return 0;
        } else if (!use2) {
            return 1;
        } else {
            return -1;
        }
    }

private:
    // Check if a node use a specific unit kind
    bool IsUseUnitKind(const DepNode &depNode) const
    {
        uint32 unitKind = depNode.GetUnitKind();
        auto idx = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
        while (idx != 0) {
            DEBUG_ASSERT(maxUnitIdx < kUnitKindLast, "invalid unit index");
            if (idx == maxUnitIdx) {
                return true;
            }
            unitKind &= ~(1u << (idx - 1u));
            idx = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
        }
        return false;
    }

    uint32 maxUnitIdx = 0;
};

// Prefer slot0
class CompareSlotType {
public:
    int operator()(const DepNode &node1, const DepNode &node2)
    {
        SlotType slotType1 = node1.GetReservation()->GetSlot();
        SlotType slotType2 = node2.GetReservation()->GetSlot();
        if (slotType1 == kSlots) {
            slotType1 = kSlot0;
        }
        if (slotType2 == kSlots) {
            slotType2 = kSlot0;
        }
        return (slotType2 - slotType1);
    }
};

// Prefer more succNodes
class CompareSuccNodeSize {
public:
    int operator()(const DepNode &node1, const DepNode &node2) const
    {
        return static_cast<int>(node1.GetSuccs().size() - node2.GetSuccs().size());
    }
};

// Default order
class CompareInsnID {
public:
    int operator()(const DepNode &node1, const DepNode &node2)
    {
        Insn *insn1 = node1.GetInsn();
        DEBUG_ASSERT(insn1 != nullptr, "get insn from depNode failed");
        Insn *insn2 = node2.GetInsn();
        DEBUG_ASSERT(insn2 != nullptr, "get insn from depNode failed");
        return static_cast<int>(insn2->GetId() - insn1->GetId());
    }
};
}  // namespace maplebe
#endif  // MAPLEBE_INCLUDE_CG_SCHEDULE_HEURISTIC_H
