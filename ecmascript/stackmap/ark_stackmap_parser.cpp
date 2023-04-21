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
#include "ecmascript/stackmap/ark_stackmap_parser.h"

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/stackmap/ark_stackmap_builder.h"

namespace panda::ecmascript::kungfu {
// implement simple binary-search is improve performance. if use std api, it'll trigger copy CallsiteHeader.
int ArkStackMapParser::BinaraySearch(CallsiteHeader *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const
{
    int slow = 0;
    int high = static_cast<int>(callsiteNum) - 1;
    int mid = 0;
    uint32_t v = 0;
    while (slow <= high) {
        mid = (slow + high) >> 1;
        v = callsiteHead[mid].calliteOffset;
        if (v == callSiteAddr) {
            return mid;
        } else if (v > callSiteAddr) {
            high = mid - 1;
        } else {
            slow = mid + 1;
        }
    }
    return -1;
}

void ArkStackMapParser::GetArkDeopt(uint8_t *stackmapAddr,
    const CallsiteHeader& callsiteHead, std::vector<ARKDeopt>& deopts) const
{
    ParseArkDeopt(callsiteHead, stackmapAddr, deopts);
}

void ArkStackMapParser::GetArkDeopt(uintptr_t callSiteAddr, uint8_t *stackmapAddr,
    std::vector<ARKDeopt>& deopts) const
{
    ArkStackMapHeader *head = reinterpret_cast<ArkStackMapHeader *>(stackmapAddr);
    ASSERT(head != nullptr);
    if (head == nullptr) {
        return;
    }
    uint32_t callsiteNum = head->callsiteNum;

    CallsiteHeader *callsiteHead = reinterpret_cast<CallsiteHeader *>(stackmapAddr + sizeof(ArkStackMapHeader));
    int mid = BinaraySearch(callsiteHead, callsiteNum, callSiteAddr);
    if (mid == -1) {
        return;
    }
    CallsiteHeader *found = callsiteHead + mid;
    GetArkDeopt(stackmapAddr, *found, deopts);
}

void ArkStackMapParser::GetConstInfo(uintptr_t callSiteAddr, LLVMStackMapType::ConstInfo& info,
    uint8_t *stackmapAddr) const
{
    std::vector<ARKDeopt> deopts;
    GetArkDeopt(callSiteAddr, stackmapAddr, deopts);
    if (deopts.empty()) {
        return;
    }

    ARKDeopt target;
    LLVMStackMapType::VRegId id = static_cast<LLVMStackMapType::VRegId>(SpecVregIndex::PC_OFFSET_INDEX);
    target.id = id;
    auto it = std::lower_bound(deopts.begin(), deopts.end(), target,
        [](const ARKDeopt& a, const ARKDeopt& b) {
            return a.id < b.id;
        });
    if (it == deopts.end() || (it->id > id)) {
        return;
    }
    ASSERT(it->kind == LocationTy::Kind::CONSTANT);
    ASSERT(std::holds_alternative<LLVMStackMapType::IntType>(it->value));
    auto v = std::get<LLVMStackMapType::IntType>(it->value);
    info.emplace_back(v);
}

uintptr_t ArkStackMapParser::GetStackSlotAddress(const LLVMStackMapType::DwarfRegAndOffsetType info,
    uintptr_t callSiteSp, uintptr_t callsiteFp) const
{
    uintptr_t address = 0;
    if (info.first == GCStackMapRegisters::SP) {
        address = callSiteSp + info.second;
    } else if (info.first == GCStackMapRegisters::FP) {
        address = callsiteFp + info.second;
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    return address;
}

bool ArkStackMapParser::IteratorStackMap(const RootVisitor& visitor, const RootBaseAndDerivedVisitor& derivedVisitor,
    uintptr_t callSiteAddr, uintptr_t callsiteFp, uintptr_t callSiteSp, uint8_t *stackmapAddr) const
{
    ArkStackMapHeader *head = reinterpret_cast<ArkStackMapHeader *>(stackmapAddr);
    ASSERT(head != nullptr);
    uint32_t callsiteNum = head->callsiteNum;
    ArkStackMap arkStackMap;
    // BuiltinsStub current only sample stub, don't have stackmap&deopt.
    if (callsiteNum == 0) {
        return false;
    }

    CallsiteHeader *callsiteHead = reinterpret_cast<CallsiteHeader *>(stackmapAddr + sizeof(ArkStackMapHeader));
    int mid = BinaraySearch(callsiteHead, callsiteNum, callSiteAddr);
    if (mid == -1) {
        return false;
    }
    CallsiteHeader *found = callsiteHead + mid;
    ParseArkStackMap(*found, stackmapAddr, arkStackMap);
    if (arkStackMap.size() == 0) {
        return false;
    }
    ASSERT(callsiteFp != callSiteSp);
    std::map<uintptr_t, uintptr_t> baseSet;
    for (size_t i = 0; i < arkStackMap.size(); i += 2) { // 2:base and derive
        const LLVMStackMapType::DwarfRegAndOffsetType baseInfo = arkStackMap.at(i);
        const LLVMStackMapType::DwarfRegAndOffsetType derivedInfo = arkStackMap.at(i + 1);
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        if (*reinterpret_cast<uintptr_t *>(base) == 0) {
            base = derived;
        }
        if (*reinterpret_cast<uintptr_t *>(base) != 0) {
            // The base address may be marked repeatedly
            if (baseSet.find(base) == baseSet.end()) {
                baseSet.emplace(base, *reinterpret_cast<uintptr_t *>(base));
                visitor(Root::ROOT_FRAME, ObjectSlot(base));
            }

            if (base != derived) {
                derivedVisitor(Root::ROOT_FRAME, ObjectSlot(base), ObjectSlot(derived), baseSet[base]);
            }
        }
    }
    baseSet.clear();
    return true;
}

void ArkStackMapParser::ParseArkStackMap(const CallsiteHeader& callsiteHead, uint8_t *ptr,
    ArkStackMap& arkStackMaps) const
{
    LLVMStackMapType::DwarfRegType reg;
    LLVMStackMapType::OffsetType offsetType;
    uint32_t offset = callsiteHead.stackmapOffset;
    uint16_t stackmapNum = callsiteHead.stackmapNum;
    ASSERT(stackmapNum % 2 == 0); // 2:base and derive
    for (uint32_t j = 0; j < stackmapNum; j++) {
        auto [regOffset, regOffsetSize, is_full] =
            panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(ptr + offset);
        LLVMStackMapType::DecodeRegAndOffset(regOffset, reg, offsetType);
        offset += regOffsetSize;
        LOG_COMPILER(VERBOSE) << " reg: " << std::dec << reg << " offset:" <<  offsetType;
        arkStackMaps.emplace_back(std::make_pair(reg, offsetType));
    }
    offset = AlignUp(offset, LLVMStackMapType::STACKMAP_ALIGN_BYTES);
}

void ArkStackMapParser::ParseArkDeopt(const CallsiteHeader& callsiteHead, uint8_t *ptr,
    std::vector<ARKDeopt> &deopts) const
{
    ARKDeopt deopt;
    uint32_t deoptOffset = callsiteHead.deoptOffset;
    uint16_t deoptNum = callsiteHead.deoptNum;
    LLVMStackMapType::KindType kindType;
    LLVMStackMapType::DwarfRegType reg;
    LLVMStackMapType::OffsetType offsetType;
    ASSERT(deoptNum % 2 == 0); // 2:<id, value>
    for (uint32_t j = 0; j < deoptNum; j += 2) { // 2:<id, value>
        auto [vregsInfo, vregsInfoSize, InfoIsFull] =
            panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(ptr + deoptOffset);
        LLVMStackMapType::DecodeVRegsInfo(vregsInfo, deopt.id, kindType);
        deoptOffset += vregsInfoSize;
        ASSERT(kindType == LLVMStackMapType::CONSTANT_TYPE || kindType == LLVMStackMapType::OFFSET_TYPE);
        if (kindType == LLVMStackMapType::CONSTANT_TYPE) {
            auto [constant, constantSize, constIsFull] =
                panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(ptr + deoptOffset);
            if (constant > INT32_MAX || constant < INT32_MIN) {
                deopt.kind = LocationTy::Kind::CONSTANTNDEX;
                deopt.value = static_cast<LLVMStackMapType::LargeInt>(constant);
            } else {
                deopt.kind = LocationTy::Kind::CONSTANT;
                deopt.value = static_cast<LLVMStackMapType::IntType>(constant);
            }
            deoptOffset += constantSize;
        } else {
            auto [regOffset, regOffsetSize, regOffIsFull] =
                panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(ptr + deoptOffset);
            LLVMStackMapType::DecodeRegAndOffset(regOffset, reg, offsetType);
            deopt.kind = LocationTy::Kind::INDIRECT;
            deopt.value = std::make_pair(reg, offsetType);
            deoptOffset += regOffsetSize;
        }
        deopts.emplace_back(deopt);
    }
}

void ArkStackMapParser::ParseArkStackMapAndDeopt(uint8_t *ptr, uint32_t length) const
{
    CallsiteHeader callsiteHead;
    ArkStackMapHeader secHead;
    BinaryBufferParser binBufparser(ptr, length);
    binBufparser.ParseBuffer(&secHead, sizeof(ArkStackMapHeader));
    for (uint32_t i = 0; i < secHead.callsiteNum; i++) {
        binBufparser.ParseBuffer(&callsiteHead, sizeof(CallsiteHeader));
        std::vector<ARKDeopt> deopts;
        ArkStackMap arkStackMaps;
        LOG_COMPILER(VERBOSE) << " calliteOffset:0x" << std::hex << callsiteHead.calliteOffset
                              << " stackmap offset:0x" << std::hex << callsiteHead.stackmapOffset
                              << " num:" << callsiteHead.stackmapNum
                              << " deopt Offset:0x" << std::hex << callsiteHead.deoptOffset
                              << " num:" << callsiteHead.deoptNum;
        ParseArkStackMap(callsiteHead, ptr, arkStackMaps);
        ParseArkDeopt(callsiteHead, ptr, deopts);
    }
}
} // namespace panda::ecmascript::kungfu