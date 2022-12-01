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
#include "ecmascript/stackmap/ark_stackmap_builder.h"
#include <map>
#include "ecmascript/aot_file_manager.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/stackmap/ark_stackmap_parser.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

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

std::pair<std::shared_ptr<uint8_t>, uint32_t> ArkStackMapBuilder::Run(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr)
{
    LLVMStackMapParser parser;
    auto result = parser.CalculateStackMap(std::move(stackMapAddr), hostCodeSectionAddr);
    if (!result) {
        UNREACHABLE();
    }
    auto pc2stackMapVec = parser.GetPc2StackMapVec();
    auto pc2DeoptVec = parser.GetPc2Deopt();
    ARKCallsiteAOTFileInfo AOTFileInfo;
    GenArkCallsiteAOTFileInfo(pc2stackMapVec, pc2DeoptVec, AOTFileInfo);
    uint32_t totalSize = AOTFileInfo.secHead.totalSize;
    uint8_t *p = new(std::nothrow) uint8_t[totalSize];
    if (p == nullptr) {
        LOG_FULL(FATAL) << "new totalSize:0x" << std::hex << totalSize << " failed";
    }
    std::shared_ptr<uint8_t> ptr(p, [](uint8_t *p) { delete []p;});
    SaveArkCallsiteAOTFileInfo(ptr.get(), totalSize, AOTFileInfo);
    return std::make_pair(ptr, totalSize);
}

void ArkStackMapBuilder::SaveArkStackMap(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer)
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

void ArkStackMapBuilder::SaveArkDeopt(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer)
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

void ArkStackMapParser::ParseArkStackMap(const CallsiteHead& callsiteHead, BinaryBufferParser& binBufparser,
    uint8_t *ptr, ArkStackMap &arkStackMaps) const
{
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
        LOG_COMPILER(VERBOSE) << " reg: " << std::dec << reg << " offset:" <<  offsetType;
        arkStackMaps.emplace_back(std::make_pair(reg, offsetType));
    }
}

void ArkStackMapParser::ParseArkDeopt(const CallsiteHead& callsiteHead,
    BinaryBufferParser& binBufparser, uint8_t *ptr, std::vector<kungfu::ARKDeopt> &deopts) const
{
    kungfu::ARKDeopt deopt;
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
                LOG_COMPILER(VERBOSE) << "const offset:" << deoptOffset;
                deopt.value = v;
                break;
            }
            case LocationTy::Kind::CONSTANTNDEX: {
                LargeInt v;
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&v), sizeof(v), ptr + deoptOffset);
                deoptOffset += sizeof(v);
                LOG_COMPILER(VERBOSE) << "large Int:" << v;
                deopt.value = v;
                break;
            }
            case LocationTy::Kind::INDIRECT: {
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&reg), sizeof(reg), ptr + deoptOffset);
                deoptOffset += sizeof(reg);
                binBufparser.ParseBuffer(reinterpret_cast<uint8_t *>(&offsetType),
                    sizeof(offsetType), ptr + deoptOffset);
                deoptOffset += sizeof(offsetType);
                LOG_COMPILER(VERBOSE) << " reg:" << std::dec << reg << " offset:" << static_cast<int>(offsetType);
                deopt.value = std::make_pair(reg, offsetType);
                break;
            }
            default: {
                UNREACHABLE();
            }
        }
        deopts.emplace_back(deopt);
    }
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
        std::vector<kungfu::ARKDeopt> deopts;
        ArkStackMap arkStackMaps;
        LOG_COMPILER(VERBOSE) << " calliteOffset:0x" << std::hex << callsiteHead.calliteOffset
            << " stackmap offset:0x" << std::hex << offset << " num:" << arkStackMapNum
            <<  "  deopt Offset:0x" << deoptOffset << " num:" << deoptNum;
        ParseArkStackMap(callsiteHead, binBufparser, ptr, arkStackMaps);
        ParseArkDeopt(callsiteHead, binBufparser, ptr, deopts);
    }
}

void ArkStackMapBuilder::SaveArkCallsiteAOTFileInfo(uint8_t *ptr, uint32_t length, const ARKCallsiteAOTFileInfo& info)
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
void ArkStackMapBuilder::SortCallSite(
    std::vector<std::unordered_map<uintptr_t, Vec>> &infos,
    std::vector<std::pair<uintptr_t, Vec>>& result)
{
    ASSERT(infos.size() == 1);
    for (auto &info: infos) {
        for (auto &it: info) {
            result.emplace_back(it);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const std::pair<uintptr_t, Vec> &x, const std::pair<uintptr_t, Vec> &y) {
            return x.first < y.first;
        });
}

void ArkStackMapBuilder::CalcCallsitePc(std::vector<std::pair<uintptr_t, DeoptInfoType>> &pc2Deopt,
    std::vector<std::pair<uintptr_t, CallSiteInfo>> &pc2StackMap, std::vector<intptr_t> &callsitePcs)
{
    std::set<uintptr_t> pcSet;
    for (auto &it: pc2Deopt) {
        pcSet.insert(it.first);
    }
    for (auto &it: pc2StackMap) {
        pcSet.insert(it.first);
    }
    callsitePcs.assign(pcSet.begin(), pcSet.end());
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

void ArkStackMapBuilder::GenARKDeopt(const DeoptInfoType& deopt, std::pair<uint32_t,
                                     std::vector<kungfu::ARKDeopt>> &sizeAndArkDeopt)
{
    ASSERT(deopt.size() % 2 == 0); // 2:<id, value>
    uint32_t total = 0;
    ARKDeopt v;
    for (size_t i = 0; i < deopt.size(); i += 2) { // 2:<id, value>
        ASSERT(std::holds_alternative<OffsetType>(deopt[i]));
        const auto &id = std::get<OffsetType>(deopt[i]);
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
        sizeAndArkDeopt.second.emplace_back(v);
    }
    std::sort(sizeAndArkDeopt.second.begin(), sizeAndArkDeopt.second.end(),
        [](const ARKDeopt &a, const ARKDeopt &b) {
            return a.Id < b.Id;
        });
    sizeAndArkDeopt.first = total;
}

void ArkStackMapBuilder::GenArkCallsiteAOTFileInfo(std::vector<Pc2CallSiteInfo> &pc2stackMapVec,
    std::vector<Pc2Deopt>& pc2DeoptVec, ARKCallsiteAOTFileInfo &result)
{
    ARKCallsite callsite;
    uint32_t totalSize = 0;
    std::vector<std::pair<uintptr_t, CallSiteInfo>> pc2stackMaps;
    std::vector<std::pair<uintptr_t, DeoptInfoType>> pc2Deopts;
    std::vector<intptr_t> CallsitePcs;
    SortCallSite(pc2stackMapVec, pc2stackMaps);
    SortCallSite(pc2DeoptVec, pc2Deopts);
    CalcCallsitePc(pc2Deopts, pc2stackMaps, CallsitePcs);
    uint32_t callsiteNum = CallsitePcs.size();
    result.callsites.resize(callsiteNum);
    uint32_t stackmapOffset = sizeof(StackMapSecHead) + sizeof(CallsiteHead) * callsiteNum;
    for (auto &x: pc2stackMaps) {
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
        result.callsites[static_cast<uint32_t>(loc)] = callsite;
    }
    totalSize = stackmapOffset;
    for (auto &x: pc2Deopts) {
        int loc = FindLoc(CallsitePcs, x.first);
        ASSERT(loc >= 0 && loc < static_cast<int>(callsiteNum));
        DeoptInfoType deopt = x.second;
        result.callsites[static_cast<uint32_t>(loc)].head.calliteOffset = x.first;
        result.callsites[static_cast<uint32_t>(loc)].head.deoptNum = deopt.size();
        result.callsites[static_cast<uint32_t>(loc)].head.deoptOffset = totalSize;
        std::pair<uint32_t, std::vector<kungfu::ARKDeopt>> sizeAndArkDeopt;
        GenARKDeopt(deopt, sizeAndArkDeopt);
        totalSize += sizeAndArkDeopt.first;
        result.callsites[static_cast<uint32_t>(loc)].callsite2Deopt = sizeAndArkDeopt.second;
    }
    result.secHead.callsiteNum = callsiteNum;
    result.secHead.callsitStart = sizeof(StackMapSecHead);
    result.secHead.callsitEnd =  result.secHead.callsitStart + (result.secHead.callsiteNum - 1) * sizeof(CallsiteHead);
    result.secHead.totalSize = totalSize;
}
} // namespace panda::ecmascript::kungfu
