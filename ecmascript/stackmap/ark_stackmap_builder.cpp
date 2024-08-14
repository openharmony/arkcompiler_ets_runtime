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
#include <map>

#include "ecmascript/stackmap/ark_stackmap_builder.h"
#include "ecmascript/stackmap/ark_stackmap_parser.h"
#include "ecmascript/stackmap/litecg/litecg_stackmap_type.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_parser.h"

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

void ArkStackMapBuilder::Dump(const StackMapDumper& dumpInfo) const
{
    LOG_COMPILER(INFO) << "total callsite num: " << dumpInfo.callsiteNum
                       << ", total ark stack map num: " << dumpInfo.stackmapNum
                       << ", total deopt num: " << dumpInfo.deoptNum;
    double callsiteHeadsSize = static_cast<double>(dumpInfo.callsiteHeadSize);
    double stackMapsSize = static_cast<double>(dumpInfo.arkStackMapSize);
    double deoptsSize = static_cast<double>(dumpInfo.deoptSize);
    LOG_COMPILER(INFO) << "total callsite head size: "
                       << std::fixed << std::setprecision(DECIMAL_LENS)
                       << (callsiteHeadsSize / 1_KB) << "KB, total stackmap size: "
                       << std::fixed << std::setprecision(DECIMAL_LENS)
                       << (stackMapsSize / 1_KB) << "KB, total deopt size: "
                       << std::fixed << std::setprecision(DECIMAL_LENS)
                       << (deoptsSize / 1_KB) << "KB";
}

std::pair<std::shared_ptr<uint8_t>, uint32_t> ArkStackMapBuilder::Run(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr, Triple triple)
{
    LLVMStackMapInfo stackMapInfo;
    LLVMStackMapParser parser(stackMapInfo);
    auto result = parser.CalculateStackMap(std::move(stackMapAddr), hostCodeSectionAddr, 0);
    if (!result) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    std::pair<std::shared_ptr<uint8_t>, uint32_t> info = GenerateArkStackMap(stackMapInfo, triple);
    return info;
}

void ArkStackMapBuilder::Collect(
    std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr,
    uintptr_t hostCodeSectionOffset,
    CGStackMapInfo &stackMapInfo)
{
    LLVMStackMapInfo &llvmStackMapInfo = static_cast<LLVMStackMapInfo&>(stackMapInfo);
    LLVMStackMapParser parser(llvmStackMapInfo);
    auto result = parser.CalculateStackMap(std::move(stackMapAddr), hostCodeSectionAddr, hostCodeSectionOffset);
    if (!result) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

std::pair<std::shared_ptr<uint8_t>, uint32_t> ArkStackMapBuilder::GenerateArkStackMap(
    CGStackMapInfo &stackMapInfo, Triple triple)
{
    ARKCallsiteAOTFileInfo AOTFileInfo;
    GenArkCallsiteAOTFileInfo(stackMapInfo, AOTFileInfo, triple);
    uint32_t secSize = AOTFileInfo.secHead.secSize;
    uint8_t *p = new(std::nothrow) uint8_t[secSize];
    if (p == nullptr) {
        LOG_FULL(FATAL) << "new secSize:0x" << std::hex << secSize << " failed";
    }
    std::shared_ptr<uint8_t> ptr(p, [](uint8_t *p) { delete []p;});
    SaveArkCallsiteAOTFileInfo(ptr.get(), secSize, AOTFileInfo, triple);
    if (traceStackMap_) {
        Dump(dumper_);
    }
    return std::make_pair(ptr, secSize);
}

void ArkStackMapBuilder::SaveArkStackMap(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer, Triple triple)
{
    size_t n = info.callsites.size();
    for (size_t i = 0; i < n; i++) {
        auto &callSite = info.callsites.at(i);
        LLVMStackMapType::CallSiteInfo stackmaps = callSite.stackmaps;
        size_t m = stackmaps.size();
        for (size_t j = 0; j < m; j++) {
            auto &stackmap = stackmaps.at(j);
            LLVMStackMapType::DwarfRegType reg = stackmap.first;
            LLVMStackMapType::OffsetType offset = stackmap.second;
            if (j == 0) {
                ASSERT(callSite.head.stackmapOffsetInSMSec == writer.GetOffset());
            }
            std::vector<uint8_t> regOffset;
            size_t regOffsetSize = 0;
            LLVMStackMapType::EncodeRegAndOffset(regOffset, regOffsetSize, reg, offset, triple);
            writer.WriteBuffer(reinterpret_cast<const uint8_t *>(regOffset.data()), regOffset.size());
            dumper_.arkStackMapSize += regOffsetSize;
            if (j == m - 1) {
                ASSERT((callSite.head.stackmapOffsetInSMSec + callSite.CalStackMapSize(triple)) == writer.GetOffset());
            }
        }
    }
    writer.AlignOffset();
}

void ArkStackMapBuilder::SaveArkDeopt(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer, Triple triple)
{
    for (auto &it: info.callsites) {
        auto& callsite2Deopt = it.callsite2Deopt;
        size_t m = callsite2Deopt.size();
        for (size_t j = 0; j < m; j++) {
            auto &deopt = callsite2Deopt.at(j);
            if (j == 0) {
                ASSERT(it.head.deoptOffset == writer.GetOffset());
            }
            std::vector<uint8_t> vregsInfo;
            size_t vregsInfoSize = 0;
            LLVMStackMapType::EncodeVRegsInfo(vregsInfo, vregsInfoSize, deopt.id, deopt.kind);
            writer.WriteBuffer(reinterpret_cast<const uint8_t *>(vregsInfo.data()), vregsInfoSize);
            dumper_.deoptSize += vregsInfoSize;
            auto& value = deopt.value;
            if (std::holds_alternative<LLVMStackMapType::IntType>(value)) {
                LLVMStackMapType::IntType v = std::get<LLVMStackMapType::IntType>(value);
                std::vector<uint8_t> num;
                size_t numSize = 0;
                LLVMStackMapType::EncodeData(num, numSize, v);
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(num.data()), numSize);
                dumper_.deoptSize += numSize;
            } else if (std::holds_alternative<LLVMStackMapType::LargeInt>(value)) {
                LLVMStackMapType::LargeInt v = std::get<LLVMStackMapType::LargeInt>(value);
                std::vector<uint8_t> num;
                size_t numSize = 0;
                LLVMStackMapType::EncodeData(num, numSize, v);
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(num.data()), numSize);
                dumper_.deoptSize += numSize;
            } else if (std::holds_alternative<LLVMStackMapType::DwarfRegAndOffsetType>(value)) {
                LLVMStackMapType::DwarfRegAndOffsetType v = std::get<LLVMStackMapType::DwarfRegAndOffsetType>(value);
                std::vector<uint8_t> regOffset;
                size_t regOffsetSize = 0;
                LLVMStackMapType::EncodeRegAndOffset(regOffset, regOffsetSize, v.first, v.second, triple);
                writer.WriteBuffer(reinterpret_cast<const uint8_t *>(regOffset.data()), regOffset.size());
                dumper_.arkStackMapSize += regOffsetSize;
            } else {
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
            }
        }
    }
}

void ArkStackMapBuilder::SaveArkCallsiteAOTFileInfo(uint8_t *ptr, uint32_t length,
    const ARKCallsiteAOTFileInfo& info, Triple triple)
{
    BinaryBufferWriter writer(ptr, length);
    ASSERT(length >= info.secHead.secSize);
    writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(info.secHead)), sizeof(ArkStackMapHeader));
    dumper_.callsiteHeadSize += sizeof(ArkStackMapHeader);
    for (auto &it: info.callsites) {
        writer.WriteBuffer(reinterpret_cast<const uint8_t *>(&(it.head)), sizeof(CallsiteHeader));
        dumper_.callsiteHeadSize += sizeof(CallsiteHeader);
    }
    SaveArkStackMap(info, writer, triple);
    SaveArkDeopt(info, writer, triple);
#ifndef NDEBUG
    ArkStackMapParser parser;
    parser.ParseArkStackMapAndDeopt(ptr, length);
#endif
}

template <class Vec>
void ArkStackMapBuilder::SortCallSite(
    const std::vector<std::unordered_map<uintptr_t, Vec>> &infos,
    std::vector<std::pair<uintptr_t, Vec>>& result)
{
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

void ArkStackMapBuilder::CalcCallsitePc(std::vector<std::pair<uintptr_t, LLVMStackMapType::DeoptInfoType>> &pc2Deopt,
    std::vector<std::pair<uintptr_t, LLVMStackMapType::CallSiteInfo>> &pc2StackMap, std::vector<intptr_t> &callsitePcs)
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

void ArkStackMapBuilder::GenARKDeopt(const LLVMStackMapType::DeoptInfoType& deopt, std::pair<uint32_t,
                                     std::vector<ARKDeopt>> &sizeAndArkDeopt, Triple triple)
{
    ASSERT(deopt.size() % DEOPT_ENTRY_SIZE == 0); // 2:<id, value>
    uint32_t total = 0;
    ARKDeopt v;
    for (size_t i = 0; i < deopt.size(); i += 2) { // 2:<id, value>
        ASSERT(std::holds_alternative<LLVMStackMapType::IntType>(deopt[i]));
        LLVMStackMapType::VRegId id = static_cast<LLVMStackMapType::VRegId>(
            std::get<LLVMStackMapType::IntType>(deopt[i]));
        v.id = id;
        auto value = deopt[i + 1];
        if (std::holds_alternative<LLVMStackMapType::IntType>(value)) {
            v.kind = LocationTy::Kind::CONSTANT;
            v.value = std::get<LLVMStackMapType::IntType>(value);
            std::vector<uint8_t> vregsInfo;
            size_t vregsInfoSize = 0;
            LLVMStackMapType::EncodeVRegsInfo(vregsInfo, vregsInfoSize, v.id, v.kind);
            size_t valueSize = panda::leb128::SignedEncodingSize(std::get<LLVMStackMapType::IntType>(value));
            total += (vregsInfoSize + valueSize);
        } else if (std::holds_alternative<LLVMStackMapType::LargeInt>(value)) {
            v.kind = LocationTy::Kind::CONSTANTNDEX;
            v.value = std::get<LLVMStackMapType::LargeInt>(value);
            std::vector<uint8_t> vregsInfo;
            size_t vregsInfoSize = 0;
            LLVMStackMapType::EncodeVRegsInfo(vregsInfo, vregsInfoSize, v.id, v.kind);
            size_t valueSize = panda::leb128::SignedEncodingSize(std::get<LLVMStackMapType::LargeInt>(value));
            total += (vregsInfoSize + valueSize);
        } else if (std::holds_alternative<LLVMStackMapType::DwarfRegAndOffsetType>(value)) {
            v.kind = LocationTy::Kind::INDIRECT;
            v.value = std::get<LLVMStackMapType::DwarfRegAndOffsetType>(value);
            std::vector<uint8_t> vregsInfo;
            size_t vregsInfoSize = 0;
            LLVMStackMapType::EncodeVRegsInfo(vregsInfo, vregsInfoSize, v.id, v.kind);
            LLVMStackMapType::DwarfRegType reg = std::get<LLVMStackMapType::DwarfRegAndOffsetType>(value).first;
            LLVMStackMapType::OffsetType offset = std::get<LLVMStackMapType::DwarfRegAndOffsetType>(value).second;
            std::vector<uint8_t> regOffset;
            size_t regOffsetSize = 0;
            LLVMStackMapType::EncodeRegAndOffset(regOffset, regOffsetSize, reg, offset, triple);
            total += (vregsInfoSize + regOffsetSize);
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
        sizeAndArkDeopt.second.emplace_back(v);
    }
    std::sort(sizeAndArkDeopt.second.begin(), sizeAndArkDeopt.second.end(),
        [](const ARKDeopt &a, const ARKDeopt &b) {
            return a.id < b.id;
        });
    sizeAndArkDeopt.first = total;
}

void ArkStackMapBuilder::GenArkCallsiteAOTFileInfo(const CGStackMapInfo &stackMapInfo,
                                                   ARKCallsiteAOTFileInfo &result, Triple triple)
{
    std::vector<std::pair<uintptr_t, LLVMStackMapType::CallSiteInfo>> pc2StackMaps;
    std::vector<std::pair<uintptr_t, LLVMStackMapType::DeoptInfoType>> pc2Deopts;
    if (stackMapInfo.GetStackMapKind() == CGStackMapInfo::kLiteCGStackMapInfo) {
        std::vector<LLVMStackMapType::Pc2CallSiteInfo> pc2StackMapsVec;
        std::vector<LLVMStackMapType::Pc2Deopt> pc2DeoptInfoVec;
        const auto &liteCGStackMapInfo = static_cast<const LiteCGStackMapInfo&>(stackMapInfo);
        liteCGStackMapInfo.ConvertToLLVMStackMapInfo(pc2StackMapsVec, pc2DeoptInfoVec, triple);
        SortCallSite(pc2StackMapsVec, pc2StackMaps);
        SortCallSite(pc2DeoptInfoVec, pc2Deopts);
    } else {
        const auto &llvmStackMapInfo = static_cast<const LLVMStackMapInfo&>(stackMapInfo);
        SortCallSite(llvmStackMapInfo.GetCallSiteInfoVec(), pc2StackMaps);
        SortCallSite(llvmStackMapInfo.GetDeoptInfoVec(), pc2Deopts);
    }
    ARKCallsite callsite;
    uint32_t secSize = 0;

    std::vector<intptr_t> CallsitePcs;

    CalcCallsitePc(pc2Deopts, pc2StackMaps, CallsitePcs);
    uint32_t callsiteNum = CallsitePcs.size();
    dumper_.callsiteNum = callsiteNum;
    result.callsites.resize(callsiteNum);
    uint32_t stackmapOffset = sizeof(ArkStackMapHeader) + sizeof(CallsiteHeader) * callsiteNum;
    for (auto &x: pc2StackMaps) {
        LLVMStackMapType::CallSiteInfo i = x.second;
        callsite.head.calliteOffsetInTxtSec = x.first;
        ASSERT(std::numeric_limits<uint16_t>::min() <= i.size() && i.size() <= std::numeric_limits<uint16_t>::max());
        callsite.head.stackmapNum = i.size();
        callsite.head.stackmapOffsetInSMSec = stackmapOffset;
        callsite.head.deoptOffset = 0;
        callsite.head.deoptNum = 0;
        callsite.stackmaps = i;
        stackmapOffset += callsite.CalStackMapSize(triple);
        int loc = FindLoc(CallsitePcs, x.first);
        ASSERT(loc >= 0 && loc < static_cast<int>(callsiteNum));
        result.callsites[static_cast<uint32_t>(loc)] = callsite;
        dumper_.stackmapNum += i.size();
    }
    stackmapOffset = AlignUp(stackmapOffset, LLVMStackMapType::STACKMAP_ALIGN_BYTES);
    secSize = stackmapOffset;
    for (auto &x: pc2Deopts) {
        int loc = FindLoc(CallsitePcs, x.first);
        ASSERT(loc >= 0 && loc < static_cast<int>(callsiteNum));
        LLVMStackMapType::DeoptInfoType deopt = x.second;
        result.callsites[static_cast<uint32_t>(loc)].head.calliteOffsetInTxtSec = x.first;
        ASSERT(std::numeric_limits<uint16_t>::min() <= deopt.size()
            && deopt.size() <= std::numeric_limits<uint16_t>::max());
        result.callsites[static_cast<uint32_t>(loc)].head.deoptNum = deopt.size();
        result.callsites[static_cast<uint32_t>(loc)].head.deoptOffset = secSize;
        std::pair<uint32_t, std::vector<ARKDeopt>> sizeAndArkDeopt;
        GenARKDeopt(deopt, sizeAndArkDeopt, triple);
        secSize += sizeAndArkDeopt.first;
        result.callsites[static_cast<uint32_t>(loc)].callsite2Deopt = sizeAndArkDeopt.second;
        dumper_.deoptNum += deopt.size();
    }
    result.secHead.callsiteNum = callsiteNum;
    result.secHead.secSize = secSize;
}
} // namespace panda::ecmascript::kungfu
