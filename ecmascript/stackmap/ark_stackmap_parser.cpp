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
#include "ecmascript/deoptimizer/deoptimizer.h"

namespace panda::ecmascript::kungfu {

void ArkStackMapParser::ParseArkDeopt(const CallsiteHeader& callsiteHead,
                                      uint8_t *ptr,
                                      std::vector<ARKDeopt> &deopts) const
{
    ARKDeopt deopt;
    uint32_t deoptOffset = callsiteHead.deoptOffset;
    uint16_t deoptNum = callsiteHead.deoptNum;
    LLVMStackMapType::KindType kindType;
    LLVMStackMapType::DwarfRegType reg;
    LLVMStackMapType::OffsetType offsetType;
    ASSERT(deoptNum % DEOPT_ENTRY_SIZE == 0); // 2:<id, value>
    for (uint32_t j = 0; j < deoptNum; j += DEOPT_ENTRY_SIZE) { // DEOPT_ENTRY_SIZE:<id, value>
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

void ArkStackMapParser::GetArkDeopt(uint8_t *stackmapAddr,
                                    const CallsiteHeader& callsiteHead,
                                    std::vector<ARKDeopt>& deopts) const
{
    ParseArkDeopt(callsiteHead, stackmapAddr, deopts);
}

// implement simple binary-search is improve performance. if use std api, it'll trigger copy CallsiteHeader.
int ArkStackMapParser::BinaraySearch(CallsiteHeader *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const
{
    int low = 0;
    int high = static_cast<int>(callsiteNum) - 1;
    int mid = 0;
    uint32_t v = 0;
    while (low <= high) {
        mid = (low + high) >> 1;
        v = callsiteHead[mid].calliteOffsetInTxtSec;
        if (v == callSiteAddr) {
            return mid;
        } else if (v > callSiteAddr) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return -1;
}

void ArkStackMapParser::GetArkDeopt(uintptr_t callSiteAddr,
                                    uint8_t *stackmapAddr,
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
    ASSERT(mid != -1);
    if (mid == -1) {
        return;
    }
    CallsiteHeader *found = callsiteHead + mid;
    GetArkDeopt(stackmapAddr, *found, deopts);
}

uintptr_t ArkStackMapParser::GetStackSlotAddress(const LLVMStackMapType::DwarfRegAndOffsetType info,
                                                 uintptr_t callSiteSp,
                                                 uintptr_t callsiteFp) const
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

// this function will increase the value of 'offset'
uintptr_t ArkStackMapParser::GetStackSlotAddress(uint8_t *stackmapAddr, uintptr_t callSiteSp, uintptr_t callsiteFp,
                                                 uint32_t &offset) const
{
    LLVMStackMapType::DwarfRegType regType;
    LLVMStackMapType::OffsetType offsetType;
    LLVMStackMapType::SLeb128Type regOffset;
    size_t regOffsetSize;
    [[maybe_unused]] bool isFull;
    uintptr_t address = 0;
    std::tie(regOffset, regOffsetSize, isFull) =
        panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(stackmapAddr + offset);
    LLVMStackMapType::DecodeRegAndOffset(regOffset, regType, offsetType);
    if (regType == GCStackMapRegisters::SP) {
        address = callSiteSp + offsetType;
    } else if (regType == GCStackMapRegisters::FP) {
        address = callsiteFp + offsetType;
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    offset += regOffsetSize;

    return address;
}

uintptr_t ArkStackMapParser::GetDeoptStackSlotAddress(uint8_t *stackmapAddr,
                                                      uintptr_t callSiteSp,
                                                      uintptr_t callsiteFp,
                                                      uint32_t &offset) const
{
    ARKDeopt deopt;
    LLVMStackMapType::KindType kindType;
    LLVMStackMapType::DwarfRegType reg;
    LLVMStackMapType::OffsetType offsetType;
    auto [vregsInfo, vregsInfoSize, InfoIsFull] =
        panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(stackmapAddr + offset);
    LLVMStackMapType::DecodeVRegsInfo(vregsInfo, deopt.id, kindType);
    offset += vregsInfoSize;
    ASSERT(kindType == LLVMStackMapType::CONSTANT_TYPE || kindType == LLVMStackMapType::OFFSET_TYPE);
    if (kindType == LLVMStackMapType::CONSTANT_TYPE) {
        auto [constant, constantSize, constIsFull] =
            panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(stackmapAddr + offset);
        offset += constantSize;
        return 0;
    } else {
        auto [regOffset, regOffsetSize, regOffIsFull] =
            panda::leb128::DecodeSigned<LLVMStackMapType::SLeb128Type>(stackmapAddr + offset);
        LLVMStackMapType::DecodeRegAndOffset(regOffset, reg, offsetType);
        offset += regOffsetSize;
        LLVMStackMapType::DwarfRegAndOffsetType info = std::make_pair(reg, offsetType);
        return GetStackSlotAddress(info, callSiteSp, callsiteFp);
    }
}

int32_t ArkStackMapParser::GetPcOffset(const std::vector<ARKDeopt> &deopts, size_t currentDepth, size_t shift) const
{
    ARKDeopt target;
    LLVMStackMapType::VRegId pcId = static_cast<LLVMStackMapType::VRegId>(SpecVregIndex::PC_OFFSET_INDEX);
    target.id = Deoptimizier::EncodeDeoptVregIndex(pcId, currentDepth, shift);
    auto it = std::lower_bound(deopts.begin(), deopts.end(), target,
        [](const ARKDeopt& a, const ARKDeopt& b) {
            return a.id < b.id;
        });
    if (it == deopts.end() || (it->id > target.id)) {
        return -1;
    }
    ASSERT(it->kind == LocationTy::Kind::CONSTANT);
    ASSERT(std::holds_alternative<LLVMStackMapType::IntType>(it->value));
    auto v = std::get<LLVMStackMapType::IntType>(it->value);
    return static_cast<int32_t>(v);
}

JSTaggedType ArkStackMapParser::GetFunction(const std::vector<ARKDeopt> &deopts, size_t currentDepth, size_t shift,
                                            uintptr_t callsiteSp, uintptr_t callsiteFp) const
{
    ARKDeopt target;
    LLVMStackMapType::VRegId pcId = static_cast<LLVMStackMapType::VRegId>(SpecVregIndex::FUNC_INDEX);
    target.id = Deoptimizier::EncodeDeoptVregIndex(pcId, currentDepth, shift);
    auto it = std::lower_bound(deopts.begin(), deopts.end(), target,
        [](const ARKDeopt& a, const ARKDeopt& b) {
            return a.id < b.id;
        });
    if (it == deopts.end() || (it->id > target.id)) {
        return 0;
    }
    ASSERT(it->kind == LocationTy::Kind::INDIRECT);
    ASSERT(std::holds_alternative<LLVMStackMapType::DwarfRegAndOffsetType>(it->value));
    auto value = std::get<LLVMStackMapType::DwarfRegAndOffsetType>(it->value);
    uintptr_t addr = GetStackSlotAddress(value, callsiteSp, callsiteFp);
    JSTaggedType v = *(reinterpret_cast<JSTaggedType *>(addr));
    return v;
}

void ArkStackMapParser::CollectStackTraceInfos(uintptr_t callSiteAddr,
                                               std::vector<std::pair<JSTaggedType, uint32_t>> &info,
                                               uintptr_t callsiteSp,
                                               uintptr_t callsiteFp,
                                               uint8_t *stackmapAddr) const
{
    std::vector<ARKDeopt> deopts;
    GetArkDeopt(callSiteAddr, stackmapAddr, deopts);
    if (deopts.empty()) {
        return;
    }
    size_t depth = GetInlineDepth(deopts);
    size_t shift = Deoptimizier::ComputeShift(depth);
    for (int i = depth; i >= 0; i--) {
        int32_t pcOffset = GetPcOffset(deopts, i, shift);
        JSTaggedType function = GetFunction(deopts, i, shift, callsiteSp, callsiteFp);
        if (pcOffset < 0 || function == 0) {
            continue;
        }
        info.push_back(std::make_pair(function, pcOffset));
    }
}

size_t ArkStackMapParser::GetInlineDepth(uintptr_t callSiteAddr, uint8_t *stackmapAddr) const
{
    std::vector<ARKDeopt> deopts;
    GetArkDeopt(callSiteAddr, stackmapAddr, deopts);
    return GetInlineDepth(deopts);
}

size_t ArkStackMapParser::GetInlineDepth(const std::vector<ARKDeopt> &deopts) const
{
    if (deopts.empty()) {
        return 0;
    }

    ARKDeopt target;
    LLVMStackMapType::VRegId id = static_cast<LLVMStackMapType::VRegId>(SpecVregIndex::INLINE_DEPTH);
    target.id = id;
    auto it = std::lower_bound(deopts.begin(), deopts.end(), target,
        [](const ARKDeopt& a, const ARKDeopt& b) {
            return a.id < b.id;
        });
    if (it == deopts.end() || (it->id > target.id)) {
        LOG_ECMA(ERROR) << "Miss inline depth";
        return 0;
    }
    ASSERT(it->kind == LocationTy::Kind::CONSTANT);
    ASSERT(std::holds_alternative<LLVMStackMapType::IntType>(it->value));
    auto v = std::get<LLVMStackMapType::IntType>(it->value);
    return static_cast<size_t>(v);
}

void ArkStackMapParser::IteratorStackMap(RootVisitor& visitor, uintptr_t callsiteFp,
                                         uintptr_t callSiteSp, uint8_t *stackmapAddr,
                                         uint32_t offset, uint16_t stackmapNum,
                                         std::map<uintptr_t, uintptr_t> &baseSet) const
{
    ASSERT(stackmapNum % GC_ENTRY_SIZE == 0);
    ASSERT(callsiteFp != callSiteSp);
    for (size_t i = 0; i < stackmapNum; i += GC_ENTRY_SIZE) { // GC_ENTRY_SIZE=<base, derive>
        uintptr_t base = GetStackSlotAddress(stackmapAddr, callSiteSp, callsiteFp, offset);
        uintptr_t derived = GetStackSlotAddress(stackmapAddr, callSiteSp, callsiteFp, offset);
        if (*reinterpret_cast<uintptr_t *>(base) == 0) {
            base = derived;
        }
        if (*reinterpret_cast<uintptr_t *>(base) != 0) {
            // The base address may be marked repeatedly
            if (baseSet.find(base) == baseSet.end()) {
                baseSet.emplace(base, *reinterpret_cast<uintptr_t *>(base));
                visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(base));
            }

            if (base != derived) {
                baseSet.emplace(derived, *reinterpret_cast<uintptr_t *>(derived));
                visitor.VisitBaseAndDerivedRoot(Root::ROOT_FRAME, ObjectSlot(base), ObjectSlot(derived),
                                                baseSet[base]);
            }
        }
    }
}

void ArkStackMapParser::IteratorDeopt(RootVisitor& visitor, uintptr_t callsiteFp,
                                      uintptr_t callSiteSp, uint8_t *stackmapAddr,
                                      uint32_t offset, uint16_t num,
                                      std::map<uintptr_t, uintptr_t> &baseSet) const
{
    ASSERT(num % DEOPT_ENTRY_SIZE == 0);
    ASSERT(callsiteFp != callSiteSp);
    for (size_t i = 0; i < num; i += DEOPT_ENTRY_SIZE) { // DEOPT_ENTRY_SIZE=<id, value>
        uintptr_t base = GetDeoptStackSlotAddress(stackmapAddr, callSiteSp, callsiteFp, offset);
        if (base != 0 && *reinterpret_cast<uintptr_t *>(base) != 0) {
            // The base address may be marked repeatedly
            if (baseSet.find(base) == baseSet.end()) {
                baseSet.emplace(base, *reinterpret_cast<uintptr_t *>(base));
                visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(base));
            }
        }
    }
}

bool ArkStackMapParser::IteratorStackMapAndDeopt(RootVisitor& visitor,
                                                 uintptr_t callSiteAddr,
                                                 uintptr_t callsiteFp,
                                                 uintptr_t callSiteSp,
                                                 uint8_t *stackmapAddr) const
{
    ArkStackMapHeader *head = reinterpret_cast<ArkStackMapHeader *>(stackmapAddr);
    ASSERT(head != nullptr);
    uint32_t callsiteNum = head->callsiteNum;
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
    std::map<uintptr_t, uintptr_t> baseSet;

    uint32_t offset = found->stackmapOffsetInSMSec;
    uint16_t num = found->stackmapNum;
    if (num == 0) {
        ASSERT(found->deoptNum == 0);
        return false;
    }
    IteratorStackMap(visitor, callsiteFp, callSiteSp, stackmapAddr, offset, num, baseSet);

    // Not sure if this is necessary, but add it just to be on the safe side.
    offset = found->deoptOffset;
    num = found->deoptNum;
    IteratorDeopt(visitor, callsiteFp, callSiteSp, stackmapAddr, offset, num, baseSet);
    
    baseSet.clear();
    return true;
}

#ifndef NDEBUG
void ArkStackMapParser::ParseArkStackMap(const CallsiteHeader& callsiteHead,
                                         uint8_t *ptr,
                                         ArkStackMap& arkStackMaps) const
{
    LLVMStackMapType::DwarfRegType reg;
    LLVMStackMapType::OffsetType offsetType;
    uint32_t offset = callsiteHead.stackmapOffsetInSMSec;
    uint16_t stackmapNum = callsiteHead.stackmapNum;
    ASSERT(stackmapNum % GC_ENTRY_SIZE == 0);
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
        LOG_COMPILER(VERBOSE) << " calliteOffsetInTxtSec: 0x" << std::hex << callsiteHead.calliteOffsetInTxtSec
                              << " stackmap offset: 0x" << std::hex << callsiteHead.stackmapOffsetInSMSec
                              << " num:" << callsiteHead.stackmapNum
                              << " deopt Offset: 0x" << std::hex << callsiteHead.deoptOffset
                              << " num:" << callsiteHead.deoptNum;
        ParseArkStackMap(callsiteHead, ptr, arkStackMaps);
        ParseArkDeopt(callsiteHead, ptr, deopts);
    }
}
#endif
} // namespace panda::ecmascript::kungfu