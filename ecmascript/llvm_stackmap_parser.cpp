/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/llvm_stackmap_parser.h"
#include <algorithm>

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/file_loader.h"
#include "ecmascript/frames.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"

using namespace panda::ecmascript;

namespace panda::ecmascript::kungfu {
std::string LocationTy::TypeToString(Kind loc) const
{
    switch (loc) {
        case Kind::REGISTER:
            return "Register	Reg	Value in a register";
        case Kind::DIRECT:
            return "Direct	Reg + Offset	Frame index value";
        case Kind::INDIRECT:
            return "Indirect	[Reg + Offset]	Spilled value";
        case Kind::CONSTANT:
            return "Constant	Offset	Small constant";
        case Kind::CONSTANTNDEX:
            return "ConstIndex	constants[Offset]	Large constant";
        default:
            return "no know location";
    }
}

const CallSiteInfo* LLVMStackMapParser::GetCallSiteInfoByPc(uintptr_t callSiteAddr) const
{
    for (auto &pc2CallSiteInfo: pc2CallSiteInfoVec_) {
        auto it = pc2CallSiteInfo.find(callSiteAddr);
        if (it != pc2CallSiteInfo.end()) {
            return &(it->second);
        }
    }
    return nullptr;
}

void LLVMStackMapParser::PrintCallSiteSlotAddr(const CallSiteInfo& callsiteInfo, uintptr_t callSiteSp,
    uintptr_t callsiteFp) const
{
    bool flag = (callsiteInfo.size() % 2 != 0);
    size_t j = flag ? 1 : 0; // skip first element when size is odd number
    for (; j < callsiteInfo.size(); j += 2) { // 2: base and derived
        const DwarfRegAndOffsetType baseInfo = callsiteInfo[j];
        const DwarfRegAndOffsetType derivedInfo = callsiteInfo[j + 1];
        LOG_COMPILER(VERBOSE) << std::hex << " callSiteSp:0x" << callSiteSp << " callsiteFp:" << callsiteFp;
        LOG_COMPILER(VERBOSE) << std::dec << "base DWARF_REG:" << baseInfo.first
                    << " OFFSET:" << baseInfo.second;
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        if (base != derived) {
            LOG_COMPILER(VERBOSE) << std::dec << "derived DWARF_REG:" << derivedInfo.first
                << " OFFSET:" << derivedInfo.second;
        }
    }
}
void LLVMStackMapParser::PrintCallSiteInfo(const CallSiteInfo *infos, OptimizedLeaveFrame *frame) const
{
    uintptr_t callSiteSp = frame->GetCallSiteSp();
    uintptr_t callsiteFp = frame->callsiteFp;
    ASSERT(infos != nullptr);
    PrintCallSiteSlotAddr(*infos, callSiteSp, callsiteFp);
}

void LLVMStackMapParser::PrintCallSiteInfo(const CallSiteInfo *infos, uintptr_t callsiteFp, uintptr_t callSiteSp) const
{
    if (!IsLogEnabled()) {
        return;
    }

    CallSiteInfo callsiteInfo = *infos;
    PrintCallSiteSlotAddr(*infos, callSiteSp, callsiteFp);
}

uintptr_t LLVMStackMapParser::GetStackSlotAddress(const DwarfRegAndOffsetType info,
    uintptr_t callSiteSp, uintptr_t callsiteFp) const
{
    uintptr_t address = 0;
    if (info.first == GCStackMapRegisters::SP) {
        address = callSiteSp + info.second;
    } else if (info.first == GCStackMapRegisters::FP) {
        address = callsiteFp + info.second;
    } else {
        UNREACHABLE();
    }
    return address;
}

void LLVMStackMapParser::CollectBaseAndDerivedPointers(const CallSiteInfo* infos, std::set<uintptr_t> &baseSet,
    ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying,
    uintptr_t callsiteFp, uintptr_t callSiteSp) const
{
    bool flag = (infos->size() % 2 != 0);
    size_t j = flag ? 1 : 0; // skip first element when size is odd number
    for (; j < infos->size(); j += 2) { // 2: base and derived
        const DwarfRegAndOffsetType& baseInfo = infos->at(j);
        const DwarfRegAndOffsetType& derivedInfo = infos->at(j + 1);
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        if (*reinterpret_cast<uintptr_t *>(base) == 0) {
            base = derived;
        }
        if (*reinterpret_cast<uintptr_t *>(base) != 0) {
            baseSet.emplace(base);
        }
        if (base != derived) {
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                if (!isVerifying) {
#endif
                    (*data)[std::make_pair(base, derived)] = *reinterpret_cast<uintptr_t *>(base);
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                }
#endif
        }
    }
}

bool LLVMStackMapParser::CollectGCSlots(uintptr_t callSiteAddr, uintptr_t callsiteFp,
    std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying,
    uintptr_t callSiteSp) const
{
    const CallSiteInfo *infos = GetCallSiteInfoByPc(callSiteAddr);
    if (infos == nullptr) {
        return false;
    }
    ASSERT(callsiteFp != callSiteSp);
    CollectBaseAndDerivedPointers(infos, baseSet, data, isVerifying, callsiteFp, callSiteSp);

    if (IsLogEnabled()) {
        PrintCallSiteInfo(infos, callsiteFp, callSiteSp);
    }
    return true;
}

int LLVMStackMapParser::BinaraySearch(CallsiteHead *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const
{
    std::vector<CallsiteHead> vec(callsiteHead, callsiteHead + callsiteNum);
    CallsiteHead target;
    target.calliteOffset = callSiteAddr;
    auto it = std::lower_bound(vec.begin(), vec.end(), target,
        [](const CallsiteHead& a, const CallsiteHead& b) {
            return a.calliteOffset < b.calliteOffset;
        });
    if (it->calliteOffset != callSiteAddr) {
        return -1;
    }
    return std::distance(vec.begin(), it);
}

std::vector<ARKDeopt> LLVMStackMapParser::GetArkDeopt(uint8_t *stackmapAddr, uint32_t length,
    const CallsiteHead& callsiteHead) const
{
    std::vector<ARKDeopt> deopts;
    BinaryBufferParser binBufparser(stackmapAddr, length);
    deopts = ParseArkDeopt(callsiteHead, binBufparser, stackmapAddr);
    return deopts;
}

ConstInfo LLVMStackMapParser::GetConstInfo(uintptr_t callSiteAddr, uint8_t *stackmapAddr) const
{
    StackMapSecHead *head = reinterpret_cast<StackMapSecHead *>(stackmapAddr);
    ConstInfo info;
    ASSERT(head != nullptr);
    uint32_t callsiteNum = head->callsiteNum;
    [[maybe_unused]] uint32_t callsitStart = head->callsitStart;
    [[maybe_unused]] uint32_t callsitEnd = head->callsitEnd;
    uint32_t length = head->totalSize;
    CallSiteInfo infos;
    ASSERT((callsitEnd - callsitStart) == ((callsiteNum - 1) * sizeof(CallsiteHead)));

    CallsiteHead *callsiteHead = reinterpret_cast<CallsiteHead *>(stackmapAddr + sizeof(StackMapSecHead));
    int mid = BinaraySearch(callsiteHead, callsiteNum, callSiteAddr);
    if (mid == -1) {
        return {};
    }
    CallsiteHead *found = callsiteHead + mid;
    auto deopts = GetArkDeopt(stackmapAddr, length, *found);
    for (auto deopt: deopts) {
        if (deopt.Id == static_cast<OffsetType>(SpecVregIndex::BC_OFFSET_INDEX)) {
            ASSERT(deopt.kind == LocationTy::Kind::CONSTANT);
            ASSERT(std::holds_alternative<OffsetType>(deopt.value));
            auto v = std::get<OffsetType>(deopt.value);
            info.emplace_back(v);
            return info;
        }
    }
    return info;
}

bool LLVMStackMapParser::CollectGCSlots(uintptr_t callSiteAddr, uintptr_t callsiteFp,
    std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying,
    uintptr_t callSiteSp, uint8_t *stackmapAddr) const
{
    StackMapSecHead *head = reinterpret_cast<StackMapSecHead *>(stackmapAddr);
    ASSERT(head != nullptr);
    uint32_t callsiteNum = head->callsiteNum;
    [[maybe_unused]] uint32_t callsitStart = head->callsitStart;
    [[maybe_unused]] uint32_t callsitEnd = head->callsitEnd;
    uint32_t length = head->totalSize;
    ArkStackMap arkStackMap;
    ASSERT((callsitEnd - callsitStart) == ((callsiteNum - 1) * sizeof(CallsiteHead)));

    CallsiteHead *callsiteHead = reinterpret_cast<CallsiteHead *>(stackmapAddr + sizeof(StackMapSecHead));
    int mid = BinaraySearch(callsiteHead, callsiteNum, callSiteAddr);
    if (mid == -1) {
        return false;
    }
    CallsiteHead *found = callsiteHead + mid;
    BinaryBufferParser binBufparser(stackmapAddr, length);
    arkStackMap = ParseArkStackMap(*found, binBufparser, stackmapAddr);
    if (arkStackMap.size() == 0) {
        return false;
    }
    ASSERT(callsiteFp != callSiteSp);
    for (size_t i = 0; i < arkStackMap.size(); i += 2) { // 2:base and derive
        const DwarfRegAndOffsetType baseInfo = arkStackMap.at(i);
        const DwarfRegAndOffsetType derivedInfo = arkStackMap.at(i + 1);
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        baseSet.emplace(base);
        if (base != derived) {
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                if (!isVerifying) {
#endif
                    data->emplace(std::make_pair(base, derived),  *reinterpret_cast<uintptr_t *>(base));
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                }
#endif
        }
    }
    return true;
}

void LLVMStackMapParser::CalcCallSite()
{
    uint64_t recordNum = 0;
    Pc2CallSiteInfo pc2CallSiteInfo;
    Pc2Deopt deoptbundles;
    auto calStkMapRecordFunc =
        [this, &recordNum, &pc2CallSiteInfo, &deoptbundles](uintptr_t address, uint32_t recordId) {
        struct StkMapRecordHeadTy recordHead = llvmStackMap_.StkMapRecord[recordNum + recordId].head;
        int lastDeoptIndex = -1;
        for (int j = 0; j < recordHead.NumLocations; j++) {
            struct LocationTy loc = llvmStackMap_.StkMapRecord[recordNum + recordId].Locations[j];
            uint32_t instructionOffset = recordHead.InstructionOffset;
            uintptr_t callsite = address + instructionOffset;
            uint64_t  patchPointID = recordHead.PatchPointID;
            if (j == LocationTy::CONSTANT_DEOPT_CNT_INDEX) {
                ASSERT(loc.location == LocationTy::Kind::CONSTANT);
                lastDeoptIndex = loc.OffsetOrSmallConstant + LocationTy::CONSTANT_DEOPT_CNT_INDEX;
            }
            if (loc.location == LocationTy::Kind::INDIRECT) {
                OPTIONAL_LOG_COMPILER(DEBUG) << "DwarfRegNum:" << loc.DwarfRegNum << " loc.OffsetOrSmallConstant:"
                    << loc.OffsetOrSmallConstant << "address:" << address << " instructionOffset:" <<
                    instructionOffset << " callsite:" << "  patchPointID :" << std::hex << patchPointID <<
                    callsite;
                DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                ASSERT(loc.DwarfRegNum == GCStackMapRegisters::SP || loc.DwarfRegNum == GCStackMapRegisters::FP);
                auto it = pc2CallSiteInfo.find(callsite);
                if (j > lastDeoptIndex) {
                    if (pc2CallSiteInfo.find(callsite) == pc2CallSiteInfo.end()) {
                        pc2CallSiteInfo.insert(std::pair<uintptr_t, CallSiteInfo>(callsite, {info}));
                    } else {
                        it->second.emplace_back(info);
                    }
                } else if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX) {
                    deoptbundles[callsite].push_back(info);
                }
            } else if (loc.location == LocationTy::Kind::CONSTANT) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    deoptbundles[callsite].push_back(loc.OffsetOrSmallConstant);
                }
            } else if (loc.location == LocationTy::Kind::DIRECT) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    ASSERT(loc.DwarfRegNum == GCStackMapRegisters::SP || loc.DwarfRegNum == GCStackMapRegisters::FP);
                    DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                    deoptbundles[callsite].push_back(info);
                }
            } else if (loc.location == LocationTy::Kind::CONSTANTNDEX) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    LargeInt v = static_cast<LargeInt>(llvmStackMap_.
                        Constants[loc.OffsetOrSmallConstant].LargeConstant);
                    deoptbundles[callsite].push_back(v);
                }
            } else {
                UNREACHABLE();
            }
        }
    };
    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        // relative offset
        uintptr_t address =  llvmStackMap_.StkSizeRecords[i].functionAddress;
        uint64_t recordCount = llvmStackMap_.StkSizeRecords[i].recordCount;
        fun2RecordNum_.emplace_back(std::make_pair(address, recordCount));
        for (uint64_t k = 0; k < recordCount; k++) {
            calStkMapRecordFunc(address, k);
        }
        recordNum += recordCount;
    }
    pc2CallSiteInfoVec_.emplace_back(pc2CallSiteInfo);
    pc2DeoptVec_.emplace_back(deoptbundles);
}

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr)
{
    if (!stackMapAddr) {
        LOG_COMPILER(ERROR) << "stackMapAddr nullptr error ! ";
        return false;
    }
    dataInfo_ = std::make_unique<DataInfo>(std::move(stackMapAddr));
    llvmStackMap_.head = dataInfo_->Read<struct Header>();
    uint32_t numFunctions, numConstants, numRecords;
    numFunctions = dataInfo_->Read<uint32_t>();
    numConstants = dataInfo_->Read<uint32_t>();
    numRecords = dataInfo_->Read<uint32_t>();
    for (uint32_t i = 0; i < numFunctions; i++) {
        auto stkRecord = dataInfo_->Read<struct StkSizeRecordTy>();
        llvmStackMap_.StkSizeRecords.push_back(stkRecord);
    }

    for (uint32_t i = 0; i < numConstants; i++) {
        auto val = dataInfo_->Read<struct ConstantsTy>();
        llvmStackMap_.Constants.push_back(val);
    }
    for (uint32_t i = 0; i < numRecords; i++) {
        struct StkMapRecordTy stkSizeRecord;
        auto head = dataInfo_->Read<struct StkMapRecordHeadTy>();
        stkSizeRecord.head = head;
        for (uint16_t j = 0; j < head.NumLocations; j++) {
            auto location = dataInfo_->Read<struct LocationTy>();
            stkSizeRecord.Locations.push_back(location);
        }
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            dataInfo_->Read<uint16_t>();
        }
        uint32_t numLiveOuts = dataInfo_->Read<uint32_t>();
        if (numLiveOuts > 0) {
            for (uint32_t j = 0; j < numLiveOuts; j++) {
                auto liveOut = dataInfo_->Read<struct LiveOutsTy>();
                stkSizeRecord.LiveOuts.push_back(liveOut);
            }
        }
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            dataInfo_->Read<uint16_t>();
        }
        llvmStackMap_.StkMapRecord.push_back(stkSizeRecord);
    }
    CalcCallSite();
    return true;
}

template <class Vec>
std::vector<std::pair<uintptr_t, Vec>> LLVMStackMapParser::SortCallSite(
    std::vector<std::unordered_map<uintptr_t, Vec>> &infos)
{
    ASSERT(infos.size() == 1);
    std::vector<std::pair<uintptr_t, Vec>> callsiteVec;
    for (auto &info: infos) {
        for (auto &it: info) {
            callsiteVec.emplace_back(it);
        }
    }
    std::sort(callsiteVec.begin(), callsiteVec.end(),
        [](const std::pair<uintptr_t, Vec> &x, const std::pair<uintptr_t, Vec> &y) {
            return x.first < y.first;
        });
    return callsiteVec;
}

std::vector<intptr_t> LLVMStackMapParser::CalcCallsitePc(std::vector<std::pair<uintptr_t, DeoptInfoType>> &pc2Deopt,
    std::vector<std::pair<uintptr_t, CallSiteInfo>> &pc2StackMap)
{
    std::set<uintptr_t> pcSet;
    for (auto &it: pc2Deopt) {
        pcSet.insert(it.first);
    }
    for (auto &it: pc2StackMap) {
        pcSet.insert(it.first);
    }
    std::vector<intptr_t> pcVec(pcSet.begin(), pcSet.end());
    return pcVec;
}

int LLVMStackMapParser::FindLoc(std::vector<intptr_t> &CallsitePcs, intptr_t pc)
{
    for (size_t i = 0; i < CallsitePcs.size(); i++) {
        if (CallsitePcs[i] == pc) {
            return i;
        }
    }
    return -1;
}

std::pair<int, std::vector<ARKDeopt>> LLVMStackMapParser::GenARKDeopt(const DeoptInfoType& deopt)
{
    ASSERT(deopt.size() % 2 == 0); // 2:<id, value>
    int total = 0;
    std::vector<ARKDeopt> deopts;
    ARKDeopt v;
    for (size_t i = 0; i < deopt.size(); i += 2) { // 2:<id, value>
        ASSERT(std::holds_alternative<OffsetType>(deopt[i]));
        auto &id = std::get<OffsetType>(deopt[i]);
        total += sizeof(OffsetType);
        v.Id = id;
        total += sizeof(LocationTy::Kind); // derive
        auto value = deopt[i + 1];
        if (std::holds_alternative<OffsetType>(value)) {
            total += sizeof(OffsetType);
            v.kind = LocationTy::Kind::CONSTANT;
            v.value = std::get<OffsetType>(value);
        } else if (std::holds_alternative<LargeInt>(value)) {
            total += sizeof(LargeInt);
            v.kind = LocationTy::Kind::CONSTANTNDEX;
            v.value = std::get<LargeInt>(value);
        } else if (std::holds_alternative<DwarfRegAndOffsetType>(value)) {
            total += (sizeof(DwarfRegType) + sizeof(OffsetType));
            v.kind = LocationTy::Kind::INDIRECT;
            v.value = std::get<DwarfRegAndOffsetType>(value);
        } else {
            UNREACHABLE();
        }
        deopts.emplace_back(v);
    }
    return std::pair(total, deopts);
}

ARKCallsitePackInfo LLVMStackMapParser::GenArkCallsitePackInfo()
{
    ARKCallsitePackInfo pack;
    ARKCallsite callsite;
    uint32_t totalSize = 0;
    auto pc2stackMap = SortCallSite(pc2CallSiteInfoVec_);
    auto pc2Deopts = SortCallSite(pc2DeoptVec_);
    std::vector<intptr_t> CallsitePcs = CalcCallsitePc(pc2Deopts, pc2stackMap);
    uint32_t callsiteNum = CallsitePcs.size();
    ASSERT(callsiteNum > 0);
    pack.callsites.resize(callsiteNum);
    uint32_t stackmapOffset = sizeof(StackMapSecHead) + sizeof(CallsiteHead) * callsiteNum;
    for (auto &x: pc2stackMap) {
        CallSiteInfo i = x.second;
        callsite.head.calliteOffset = x.first;
        callsite.head.arkStackMapNum = i.size();
        callsite.head.stackmapOffset = stackmapOffset;
        callsite.head.deoptOffset = 0;
        callsite.head.deoptNum = 0;
        callsite.stackmaps = i;
        stackmapOffset += callsite.CalStackMapSize();
        int loc = FindLoc(CallsitePcs, x.first);
        ASSERT(loc >= 0 && loc < static_cast<int>(callsiteNum));
        pack.callsites[loc] = callsite;
    }
    totalSize = stackmapOffset;
    for (auto &x: pc2Deopts) {
        int loc = FindLoc(CallsitePcs, x.first);
        ASSERT(loc >= 0 && loc < static_cast<int>(callsiteNum));
        DeoptInfoType deopt = x.second;
        pack.callsites[loc].head.calliteOffset = x.first;
        pack.callsites[loc].head.deoptNum = deopt.size();
        pack.callsites[loc].head.deoptOffset = totalSize;
        int size;
        std::vector<ARKDeopt> arkDeopt;
        std::tie(size, arkDeopt) = GenARKDeopt(deopt);
        totalSize += size;
        pack.callsites[loc].callsite2Deopt = arkDeopt;
    }
    pack.secHead.callsiteNum = callsiteNum;
    pack.secHead.callsitStart = sizeof(StackMapSecHead);
    pack.secHead.callsitEnd =  pack.secHead.callsitStart + (pack.secHead.callsiteNum - 1) * sizeof(CallsiteHead);
    pack.secHead.totalSize = totalSize;
    return pack;
}

uint32_t ARKCallsite::CalHeadSize() const
{
    uint32_t headSize = sizeof(head);
    return headSize;
}

uint32_t ARKCallsite::CalStackMapSize() const
{
    size_t stackmapSize = stackmaps.size() * (sizeof(OffsetType) + sizeof(DwarfRegType));
    return stackmapSize;
}

ArkStackMap LLVMStackMapParser::ParseArkStackMap(const CallsiteHead& callsiteHead, BinaryBufferParser& binBufparser,
    uint8_t *ptr) const
{
    ArkStackMap arkStackMaps;
    DwarfRegType reg;
    OffsetType offsetType;
    uint32_t offset = callsiteHead.stackmapOffset;
    uint32_t arkStackMapNum = callsiteHead.arkStackMapNum;
    ASSERT(arkStackMapNum % 2 == 0); // 2:base and derive
    for (uint32_t j = 0; j < arkStackMapNum; j++) {
        binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&reg), sizeof(DwarfRegType), ptr + offset);
        offset += sizeof(DwarfRegType);
        binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&offsetType), sizeof(OffsetType), ptr + offset);
        offset += sizeof(OffsetType);
        LOG_COMPILER(DEBUG) << " reg: " << std::dec << reg << " offset:" <<  offsetType;
        arkStackMaps.emplace_back(std::pair(reg, offsetType));
        ASSERT(reg == GCStackMapRegisters::SP || reg == GCStackMapRegisters::FP);
    }
    return arkStackMaps;
}

std::vector<ARKDeopt> LLVMStackMapParser::ParseArkDeopt(const CallsiteHead& callsiteHead,
    BinaryBufferParser& binBufparser, uint8_t *ptr) const
{
    std::vector<ARKDeopt> deopts;
    ARKDeopt deopt;
    uint32_t deoptOffset = callsiteHead.deoptOffset;
    uint32_t deoptNum = callsiteHead.deoptNum;
    OffsetType id;
    LocationTy::Kind kind;
    DwarfRegType reg;
    OffsetType offsetType;
    ASSERT(deoptNum % 2 == 0); // 2:<id, value>
    for (uint32_t j = 0; j < deoptNum; j += 2) { // 2:<id, value>
        binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&id), sizeof(id), ptr + deoptOffset);
        deoptOffset += sizeof(id);
        deopt.Id = id;
        binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&kind), sizeof(kind), ptr + deoptOffset);
        deoptOffset += sizeof(kind);
        deopt.kind = kind;
        switch (kind) {
            case LocationTy::Kind::CONSTANT: {
                OffsetType v;
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&v), sizeof(v), ptr + deoptOffset);
                deoptOffset += sizeof(v);
                LOG_COMPILER(DEBUG) << "const offset:" << deoptOffset;
                deopt.value = v;
                break;
            }
            case LocationTy::Kind::CONSTANTNDEX: {
                LargeInt v;
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&v), sizeof(v), ptr + deoptOffset);
                deoptOffset += sizeof(v);
                LOG_COMPILER(DEBUG) << "large Int:" << v;
                deopt.value = v;
                break;
            }
            case LocationTy::Kind::INDIRECT: {
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&reg), sizeof(reg), ptr + deoptOffset);
                deoptOffset += sizeof(reg);
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&offsetType),
                    sizeof(offsetType), ptr + deoptOffset);
                deoptOffset += sizeof(offsetType);
                ASSERT(reg == GCStackMapRegisters::SP || reg == GCStackMapRegisters::FP);
                LOG_COMPILER(DEBUG) << " reg:" << std::dec << reg << " offset:" << static_cast<int>(offsetType);
                deopt.value = std::make_pair(reg, offsetType);
                break;
            }
            default: {
                UNREACHABLE();
            }
        }
        deopts.emplace_back(deopt);
    }
    return deopts;
}

void LLVMStackMapParser::ParseArkStackMapAndDeopt(uint8_t *ptr, uint32_t length) const
{
    CallsiteHead callsiteHead;
    StackMapSecHead secHead;
    BinaryBufferParser binBufparser(ptr, length);
    binBufparser.ParseBuffer(&secHead, sizeof(StackMapSecHead));
    for (uint32_t i = 0; i < secHead.callsiteNum; i++) {
        binBufparser.ParseBuffer(&callsiteHead, sizeof(CallsiteHead));
        uint32_t offset = callsiteHead.stackmapOffset;
        uint32_t arkStackMapNum = callsiteHead.arkStackMapNum;
        uint32_t deoptOffset = callsiteHead.deoptOffset;
        uint32_t deoptNum = callsiteHead.deoptNum;
        LOG_COMPILER(DEBUG) << " calliteOffset:0x" << std::hex << callsiteHead.calliteOffset
            << " stackmap offset:0x" << std::hex << offset << " num:" << arkStackMapNum
            <<  "  deopt Offset:0x" << deoptOffset << " num:" << deoptNum;
        ParseArkStackMap(callsiteHead, binBufparser, ptr);
        ParseArkDeopt(callsiteHead, binBufparser, ptr);
    }
}

void LLVMStackMapParser::SaveArkStackMap(const ARKCallsitePackInfo& info, BinaryBufferWriter& writer)
{
    size_t n = info.callsites.size();
    for (size_t i = 0; i < n; i++) {
        auto &callSite = info.callsites.at(i);
        CallSiteInfo stackmaps = callSite.stackmaps;
        size_t m = stackmaps.size();
        for (size_t j = 0; j < m; j++) {
            auto &stackmap = stackmaps.at(j);
            DwarfRegType reg = stackmap.first;
            OffsetType offset = stackmap.second;
            if (j == 0) {
                ASSERT(callSite.head.stackmapOffset == writer.GetOffset());
            }
            writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(reg)), sizeof(DwarfRegType));
            writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(offset)), sizeof(OffsetType));
            if (j == m - 1) {
                ASSERT((callSite.head.stackmapOffset + callSite.CalStackMapSize()) == writer.GetOffset());
            }
        }
    }
}

void LLVMStackMapParser::SaveArkDeopt(const ARKCallsitePackInfo& info, BinaryBufferWriter& writer)
{
    for (auto &it: info.callsites) {
        auto& callsite2Deopt = it.callsite2Deopt;
        size_t m = callsite2Deopt.size();
        for (size_t j = 0; j < m; j++) {
            auto &deopt = callsite2Deopt.at(j);
            if (j == 0) {
                ASSERT(it.head.deoptOffset == writer.GetOffset());
            }
            writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(deopt.Id)), sizeof(deopt.Id));
            writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(deopt.kind)), sizeof(deopt.kind));
            auto& value = deopt.value;
            if (std::holds_alternative<OffsetType>(value)) {
                OffsetType v = std::get<OffsetType>(value);
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(v)), sizeof(v));
            } else if (std::holds_alternative<LargeInt>(value)) {
                LargeInt v = std::get<LargeInt>(value);
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(v)), sizeof(v));
            } else if (std::holds_alternative<DwarfRegAndOffsetType>(value)) {
                DwarfRegAndOffsetType v = std::get<DwarfRegAndOffsetType>(value);
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(v.first)), sizeof(v.first));
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(v.second)), sizeof(v.second));
            } else {
                UNREACHABLE();
            }
        }
    }
}

void LLVMStackMapParser::SaveArkCallsitePackInfo(uint8_t *ptr, uint32_t length, const ARKCallsitePackInfo& info)
{
    BinaryBufferWriter writer(ptr, length);
    ASSERT(length >= info.secHead.totalSize);
    writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(info.secHead)), sizeof(StackMapSecHead));
    for (auto &it: info.callsites) {
        writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(it.head)), sizeof(CallsiteHead));
    }
    SaveArkStackMap(info, writer);
    SaveArkDeopt(info, writer);
#ifndef NDEBUG
    ParseArkStackMapAndDeopt(ptr, length);
#endif
}

std::pair<uint8_t *, uint32_t> LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr)
{
    bool ret = CalculateStackMap(std::move(stackMapAddr));
    if (!ret) {
        return std::make_pair(nullptr, 0);
    }

    // update functionAddress from host side to device side
    OPTIONAL_LOG_COMPILER(DEBUG) << "stackmap calculate update funcitonaddress ";

    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t hostAddr = llvmStackMap_.StkSizeRecords[i].functionAddress;
        uintptr_t offset = hostAddr - hostCodeSectionAddr;
        llvmStackMap_.StkSizeRecords[i].functionAddress = offset;
        OPTIONAL_LOG_COMPILER(DEBUG) << std::dec << i << "th function " << std::hex << hostAddr << " ---> "
                                     << " offset:" << offset;
    }
    pc2CallSiteInfoVec_.clear();
    fun2RecordNum_.clear();
    pc2DeoptVec_.clear();
    CalcCallSite();
    ARKCallsitePackInfo packInfo = GenArkCallsitePackInfo();
    uint32_t totalSize = packInfo.secHead.totalSize;
    uint8_t *ptr = new uint8_t[totalSize];
    SaveArkCallsitePackInfo(ptr, totalSize, packInfo);
    return std::make_pair(ptr, totalSize);
}

void LLVMStackMapParser::CalculateFuncFpDelta(Func2FpDelta info, uint32_t moduleIndex)
{
    std::vector<Func2FpDelta> fun2FpDelta;
    auto it = module2fun2FpDelta_.find(moduleIndex);
    if (it != module2fun2FpDelta_.end()) {
        fun2FpDelta = module2fun2FpDelta_.at(moduleIndex);
    }
    bool find = std::find(fun2FpDelta.begin(), fun2FpDelta.end(), info) == fun2FpDelta.end();
    if (!info.empty() && find) {
        fun2FpDelta.emplace_back(info);
    }
    module2fun2FpDelta_.erase(moduleIndex);
    module2fun2FpDelta_[moduleIndex] = fun2FpDelta;

    std::set<uintptr_t> funAddr;
    auto i = module2funAddr_.find(moduleIndex);
    if (i != module2funAddr_.end()) {
        funAddr = module2funAddr_.at(moduleIndex);
        module2funAddr_.erase(moduleIndex);
    }
    for (auto &iterator: info) {
        funAddr.insert(iterator.first);
    }
    module2funAddr_[moduleIndex] = funAddr;
}

int LLVMStackMapParser::FindFpDelta(uintptr_t funcAddr, uintptr_t callsitePc, uint32_t moduleIndex) const
{
    int delta = 0;
    // next optimization can be performed via sorted/map.
    std::vector<Func2FpDelta> fun2FpDelta;
    auto it = module2fun2FpDelta_.find(moduleIndex);
    if (it != module2fun2FpDelta_.end()) {
        fun2FpDelta = module2fun2FpDelta_.at(moduleIndex);
    } else {
        return delta;
    }

    for (auto &info: fun2FpDelta) {
        if (info.find(funcAddr) != info.end()) {
            delta = info.at(funcAddr).first;
            uint32_t funcSize = info.at(funcAddr).second;
            if (callsitePc <= funcAddr + funcSize && callsitePc >= funcAddr) {
                return delta;
            }
        }
    }
    return delta;
}

void BinaryBufferWriter::WriteBuffer(const uint8_t *src, uint32_t count, bool flag)
{
    uint8_t *dst = buffer_ + offset_;
    if (flag) {
        std::cout << "buffer_:0x" << std::hex << buffer_ << " offset_:0x" << offset_ << std::endl;
    }
    if (dst >= buffer_ && dst + count <= buffer_ + length_) {
        if (memcpy_s(dst, buffer_ + length_ - dst, src, count) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            return;
        };
        offset_ = offset_ + count;
    }  else {
        LOG_FULL(FATAL) << "parse buffer error, length is 0 or overflow";
    }
}
}  // namespace panda::ecmascript::kungfu
