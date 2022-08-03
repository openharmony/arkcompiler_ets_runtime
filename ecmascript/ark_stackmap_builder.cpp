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
#include "ecmascript/ark_stackmap_builder.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/file_loader.h"
#include "ecmascript/llvm_stackmap_parser.h"

namespace panda::ecmascript::kungfu {

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

std::pair<std::shared_ptr<uint8_t[]>, uint32_t> ArkStackMapBuilder::Run(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr)
{
    LLVMStackMapParser parser;
    auto result = parser.CalculateStackMap(std::move(stackMapAddr), hostCodeSectionAddr);
    if (!result) {
        UNREACHABLE();
    }
    auto pc2stackMaps = parser.GetPc2StackMap();
    auto pc2DeoptVec = parser.GetPc2Deopt();
    ARKCallsitePackInfo packInfo = GenArkCallsitePackInfo(pc2stackMaps, pc2DeoptVec);
    uint32_t totalSize = packInfo.secHead.totalSize;
    std::shared_ptr<uint8_t[]> ptr(new uint8_t[totalSize]());
    SaveArkCallsitePackInfo(ptr.get(), totalSize, packInfo);
    return std::make_pair(ptr, totalSize);;
}

void ArkStackMapBuilder::SaveArkStackMap(const ARKCallsitePackInfo& info, BinaryBufferWriter& writer)
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

void ArkStackMapBuilder::SaveArkDeopt(const ARKCallsitePackInfo& info, BinaryBufferWriter& writer)
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

ArkStackMap ArkStackMapParser::ParseArkStackMap(const CallsiteHead& callsiteHead, BinaryBufferParser& binBufparser,
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

std::vector<ARKDeopt> ArkStackMapParser::ParseArkDeopt(const CallsiteHead& callsiteHead,
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

void ArkStackMapParser::ParseArkStackMapAndDeopt(uint8_t *ptr, uint32_t length) const
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

void ArkStackMapBuilder::SaveArkCallsitePackInfo(uint8_t *ptr, uint32_t length, const ARKCallsitePackInfo& info)
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
    ArkStackMapParser parser;
    parser.ParseArkStackMapAndDeopt(ptr, length);
#endif
}

template <class Vec>
std::vector<std::pair<uintptr_t, Vec>> ArkStackMapBuilder::SortCallSite(
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

std::vector<intptr_t> ArkStackMapBuilder::CalcCallsitePc(std::vector<std::pair<uintptr_t, DeoptInfoType>> &pc2Deopt,
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

int ArkStackMapBuilder::FindLoc(std::vector<intptr_t> &CallsitePcs, intptr_t pc)
{
    for (size_t i = 0; i < CallsitePcs.size(); i++) {
        if (CallsitePcs[i] == pc) {
            return i;
        }
    }
    return -1;
}

std::pair<int, std::vector<ARKDeopt>> ArkStackMapBuilder::GenARKDeopt(const DeoptInfoType& deopt)
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

ARKCallsitePackInfo ArkStackMapBuilder::GenArkCallsitePackInfo(std::vector<Pc2CallSiteInfo> &pc2stackMaps,
    std::vector<Pc2Deopt>& pc2DeoptVec)
{
    ARKCallsitePackInfo pack;
    ARKCallsite callsite;
    uint32_t totalSize = 0;
    auto pc2stackMap = SortCallSite(pc2stackMaps);
    auto pc2Deopts = SortCallSite(pc2DeoptVec);
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

int ArkStackMapParser::BinaraySearch(CallsiteHead *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const
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

std::vector<ARKDeopt> ArkStackMapParser::GetArkDeopt(uint8_t *stackmapAddr, uint32_t length,
    const CallsiteHead& callsiteHead) const
{
    std::vector<ARKDeopt> deopts;
    BinaryBufferParser binBufparser(stackmapAddr, length);
    deopts = ParseArkDeopt(callsiteHead, binBufparser, stackmapAddr);
    return deopts;
}

ConstInfo ArkStackMapParser::GetConstInfo(uintptr_t callSiteAddr, uint8_t *stackmapAddr) const
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

uintptr_t ArkStackMapParser::GetStackSlotAddress(const DwarfRegAndOffsetType info,
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

bool ArkStackMapParser::CollectGCSlots(uintptr_t callSiteAddr, uintptr_t callsiteFp,
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
} // namespace panda::ecmascript::kungfu
