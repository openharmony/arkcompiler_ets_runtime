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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MEM_REFERENCE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MEM_REFERENCE_H

#include "deps.h"
#include "aarch64_insn.h"

namespace maplebe {
class AArch64MemReference {
public:
    // If one of the memory is volatile, we need build dependency.
    static bool HasVolatile(const Insn &memInsn1, const Insn &memInsn2)
    {
        auto *memOpnd1 = static_cast<MemOperand *>(memInsn1.GetMemOpnd());
        auto *memOpnd2 = static_cast<MemOperand *>(memInsn2.GetMemOpnd());
        if (memOpnd1 != nullptr && memOpnd2 != nullptr) {
            if (memOpnd1->IsVolatile() || memOpnd2->IsVolatile()) {
                return true;
            }
        }
        return false;
    }

    // If the two memory reference sets do not have the same element(ostIdx),
    // there is no alias, return false.
    static bool HasMemoryReferenceAlias(const MemDefUseSet &refSet1, const MemDefUseSet &refSet2)
    {
        // Process conservatively when no alias info is available
        if (refSet1.empty() || refSet2.empty()) {
            return true;
        }
        for (const auto &ostIdx : refSet2) {
            if (refSet1.find(ostIdx) != refSet1.end()) {
                return true;
            }
        }
        return false;
    }

    // Check memory overlap of same baseAddress in basic analysis and
    // can be used for both [stack] and [heap] memory.
    static bool IsMemoryOverlap(const Insn &memInsn1, const Insn &memInsn2, const Insn *baseDefInsn1,
                                const Insn *baseDefInsn2)
    {
        auto *memOpnd1 = static_cast<MemOperand *>(memInsn1.GetMemOpnd());
        auto *memOpnd2 = static_cast<MemOperand *>(memInsn2.GetMemOpnd());
        ASSERT_NOT_NULL(memOpnd1);
        ASSERT_NOT_NULL(memOpnd2);
        // Must be BOI-mode
        DEBUG_ASSERT(memOpnd1->GetAddrMode() == MemOperand::kAddrModeBOi &&
                         memOpnd2->GetAddrMode() == MemOperand::kAddrModeBOi,
                     "invalid addressing mode");

        RegOperand *baseOpnd1 = memOpnd1->GetBaseRegister();
        RegOperand *baseOpnd2 = memOpnd2->GetBaseRegister();
        // BaseRegister will be nullptr in BOI-mode, in what cases?
        if (baseOpnd1 == nullptr || baseOpnd2 == nullptr) {
            return true;
        }

        // If the two base addresses are different, we can not analyze whether they overlap here,
        // so we process conservatively.
        if (baseOpnd1->GetRegisterNumber() != baseOpnd2->GetRegisterNumber()) {
            return true;
        }
        if (baseDefInsn1 != baseDefInsn2) {
            return true;
        }

        OfstOperand *ofstOpnd1 = memOpnd1->GetOffsetImmediate();
        OfstOperand *ofstOpnd2 = memOpnd2->GetOffsetImmediate();

        int64 ofstValue1 = (ofstOpnd1 == nullptr ? 0 : ofstOpnd1->GetOffsetValue());
        int64 ofstValue2 = (ofstOpnd2 == nullptr ? 0 : ofstOpnd2->GetOffsetValue());

        // Compatible with the load/store pair and load/store
        uint32 memByteSize1 = memInsn1.GetMemoryByteSize();
        uint32 memByteSize2 = memInsn2.GetMemoryByteSize();

        int64 memByteBoundary1 = ofstValue1 + static_cast<int64>(memByteSize1);
        int64 memByteBoundary2 = ofstValue2 + static_cast<int64>(memByteSize2);

        // no overlap
        // baseAddr     ofst2            ofst1              ofst2--->
        //     |________|___memSize2_____|_____memSize1_____|__________
        //                               |                  |
        //                               memBoundary2       memBoundary1
        if (ofstValue2 >= memByteBoundary1 || ofstValue1 >= memByteBoundary2) {
            return false;
        }
        return true;
    }

    // Simply distinguish irrelevant stack memory.
    static bool HasBasicMemoryDep(const Insn &memInsn1, const Insn &memInsn2, Insn *baseDefInsn1, Insn *baseDefInsn2)
    {
        // The callee-save and callee-reload instructions do not conflict with any other memory access instructions,
        // we use the $isIndependent field to check that.
        const MemDefUse *memReference1 = memInsn1.GetReferenceOsts();
        const MemDefUse *memReference2 = memInsn2.GetReferenceOsts();
        if (memReference1 != nullptr && memReference1->IsIndependent()) {
            return false;
        }
        if (memReference2 != nullptr && memReference2->IsIndependent()) {
            return false;
        }

        auto *memOpnd1 = static_cast<MemOperand *>(memInsn1.GetMemOpnd());
        auto *memOpnd2 = static_cast<MemOperand *>(memInsn2.GetMemOpnd());
        // Not here to consider StImmOperand(e.g. adrp) or call(e.g. bl)
        if (memOpnd1 == nullptr || memOpnd2 == nullptr) {
            return true;
        }
        // 1. Check MIRSymbol
        if (memOpnd1->GetSymbol() != nullptr && memOpnd2->GetSymbol() != nullptr &&
            memOpnd1->GetSymbol() != memOpnd2->GetSymbol()) {
            return false;
        }
        // Only consider the BOI-mode for basic overlap analysis
        if (memOpnd1->GetAddrMode() != MemOperand::kAddrModeBOi ||
            memOpnd2->GetAddrMode() != MemOperand::kAddrModeBOi) {
            return true;
        }
        RegOperand *baseOpnd1 = memOpnd1->GetBaseRegister();
        RegOperand *baseOpnd2 = memOpnd2->GetBaseRegister();
        // BaseRegister will be nullptr in BOI-mode, in what cases?
        if (baseOpnd1 == nullptr || baseOpnd2 == nullptr) {
            return true;
        }
        // 2. Check memory overlap
        if (!IsMemoryOverlap(memInsn1, memInsn2, baseDefInsn1, baseDefInsn2)) {
            return false;
        }
        return true;
    }

    static bool HasAliasMemoryDep(const Insn &fromMemInsn, const Insn &toMemInsn, DepType depType)
    {
        const MemDefUse *fromReferenceOsts = fromMemInsn.GetReferenceOsts();
        const MemDefUse *toReferenceOsts = toMemInsn.GetReferenceOsts();
        // Process conservatively when no alias info is available
        if (fromReferenceOsts == nullptr || toReferenceOsts == nullptr) {
            return true;
        }
        const MemDefUseSet &fromMemDefSet = fromReferenceOsts->GetDefSet();
        const MemDefUseSet &fromMemUseSet = fromReferenceOsts->GetUseSet();
        const MemDefUseSet &toMemDefSet = toReferenceOsts->GetDefSet();
        const MemDefUseSet &toMemUseSet = toReferenceOsts->GetUseSet();

        switch (depType) {
            // Check write->read dependency
            case kDependenceTypeTrue:
                return HasMemoryReferenceAlias(fromMemDefSet, toMemUseSet);
            // Check read->write dependency
            case kDependenceTypeAnti:
                return HasMemoryReferenceAlias(fromMemUseSet, toMemDefSet);
            // Check write->write dependency
            case kDependenceTypeOutput:
                return HasMemoryReferenceAlias(fromMemDefSet, toMemDefSet);
            case kDependenceTypeNone: {
                if (HasMemoryReferenceAlias(fromMemDefSet, toMemDefSet) ||
                    HasMemoryReferenceAlias(fromMemDefSet, toMemUseSet) ||
                    HasMemoryReferenceAlias(fromMemUseSet, toMemDefSet)) {
                    return true;
                } else {
                    return false;
                }
            }
            default:
                CHECK_FATAL(false, "invalid memory dependency type");
        }
    }

    // The entrance: if memory dependency needs to be build, return true.
    // fromMemInsn & toMemInsn: will be [load, store, adrp, call]
    // baseDefInsns: for basic memory dependency analysis, they define the base address and can be nullptr.
    // depType: for memory analysis based on me-alias-info, only consider TRUE, ANTI, OUTPUT and NONE dependency type
    //          from fromMemInsn to toMemInsn, if it is NONE, we take care of both def and use sets.
    static bool NeedBuildMemoryDependency(const Insn &fromMemInsn, const Insn &toMemInsn, Insn *baseDefInsn1,
                                          Insn *baseDefInsn2, DepType depType)
    {
        // 0. Check volatile
        if (HasVolatile(fromMemInsn, toMemInsn)) {
            return true;
        }
        // 1. Do basic analysis of memory-related instructions based on cg-ir
        if (!HasBasicMemoryDep(fromMemInsn, toMemInsn, baseDefInsn1, baseDefInsn2)) {
            return false;
        }
        // 2. Do alias analysis of memory-related instructions based on me-alias-info
        return HasAliasMemoryDep(fromMemInsn, toMemInsn, depType);
    }
};
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MEM_REFERENCE_H
