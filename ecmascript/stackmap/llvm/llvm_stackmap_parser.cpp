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
#include "ecmascript/stackmap/llvm/llvm_stackmap_parser.h"

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

void LLVMStackMapParser::FilterCallSiteInfo(LLVMStackMapType::CallSiteInfo &info)
{
    ASSERT(GC_PAIR_SIZE == 2); // 2 : The expected value of GC_PAIR_SIZE is 2
    ASSERT(info.size() % GC_PAIR_SIZE == 0);
    for (auto it = info.begin(); it != info.end();) {
        auto base = it;
        auto deri = ++it;
        bool baseIsConst = (base->first == LLVMStackMapType::INVALID_DWARF_REG);
        bool deriIsConst = (deri->first == LLVMStackMapType::INVALID_DWARF_REG);
        if (baseIsConst && deriIsConst) {
            it = info.erase(base, base + GC_PAIR_SIZE);
        } else if (baseIsConst && !deriIsConst) {
            base->first = deri->first;
            base->second = deri->second;
            it++;
        } else if (!baseIsConst && deriIsConst) {
            deri->first = base->first;
            deri->second = base->second;
            it++;
        } else {
            it++;
        }
    }
    ASSERT(info.size() % GC_PAIR_SIZE == 0);
}

void LLVMStackMapParser::CalcCallSite()
{
    uint64_t recordNum = 0;
    LLVMStackMapType::Pc2CallSiteInfo pc2CallSiteInfo;
    LLVMStackMapType::Pc2Deopt deoptbundles;

    auto calStkMapRecordFunc = [this, &recordNum, &pc2CallSiteInfo, &deoptbundles](uintptr_t address,
                                                                                   uint32_t recordId) {
        struct StkMapRecordTy &record = llvmStackMap_.stkMapRecord[recordNum + recordId];
        struct StkMapRecordHeadTy &recordHead = record.head;
        uint32_t instructionOffset = recordHead.instructionOffset;
        uintptr_t pc = address + instructionOffset;
        uint64_t pID = recordHead.patchPointID;

        if (pc2CallSiteInfo.find(pc) == pc2CallSiteInfo.end()) {
            auto p = std::pair<uintptr_t, LLVMStackMapType::CallSiteInfo>(pc, {});
            pc2CallSiteInfo.insert(p);
        }
        LLVMStackMapType::CallSiteInfo& callSiteInfo = pc2CallSiteInfo.find(pc)->second;

        ASSERT(recordHead.numLocations > LocationTy::CONSTANT_DEOPT_CNT_INDEX);
        const int lastDeoptIndex = record.locations[LocationTy::CONSTANT_DEOPT_CNT_INDEX].offsetOrSmallConstant +
                                   LocationTy::CONSTANT_DEOPT_CNT_INDEX;

        for (int j = LocationTy::CONSTANT_FIRST_ELEMENT_INDEX; j < recordHead.numLocations; j++) {
            const struct LocationTy &loc = record.locations[j];
            if (j <= lastDeoptIndex) {
                switch (loc.location) {
                    case LocationTy::Kind::REGISTER:
                    case LocationTy::Kind::DIRECT: {
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                        break;
                    }
                    case LocationTy::Kind::INDIRECT: {
                        OPTIONAL_LOG_COMPILER(DEBUG) << "DwarfRegNum:" << loc.dwarfRegNum
                                                     << " loc.OffsetOrSmallConstant:" << loc.offsetOrSmallConstant
                                                     << " address:" << address
                                                     << " instructionOffset:" << instructionOffset
                                                     << " callsite:" << "  patchPointID :" << std::hex
                                                     << pID << pc;
                        LLVMStackMapType::DwarfRegAndOffsetType info(loc.dwarfRegNum, loc.offsetOrSmallConstant);
                        deoptbundles[pc].push_back(info);
                        break;
                    }
                    case LocationTy::Kind::CONSTANT: {
                        deoptbundles[pc].push_back(loc.offsetOrSmallConstant);
                        break;
                    }
                    case LocationTy::Kind::CONSTANTNDEX: {
                        auto v = llvmStackMap_.constants[loc.offsetOrSmallConstant].largeConstant;
                        deoptbundles[pc].push_back(static_cast<LLVMStackMapType::LargeInt>(v));
                        break;
                    }
                    default: {
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                        break;
                    }
                }
            } else {
                switch (loc.location) {
                    case LocationTy::Kind::REGISTER:
                    case LocationTy::Kind::DIRECT: {
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                        break;
                    }
                    case LocationTy::Kind::INDIRECT:
                    case LocationTy::Kind::CONSTANT:
                    case LocationTy::Kind::CONSTANTNDEX: {
                        OPTIONAL_LOG_COMPILER(DEBUG) << "DwarfRegNum:" << loc.dwarfRegNum
                                                     << " loc.OffsetOrSmallConstant:" << loc.offsetOrSmallConstant
                                                     << " address:" << address
                                                     << " instructionOffset:" << instructionOffset
                                                     << " callsite:" << "  patchPointID :" << std::hex
                                                     << pID << pc;
                        uint16_t regNum = (loc.location == LocationTy::Kind::INDIRECT)
                                          ? loc.dwarfRegNum
                                          : LLVMStackMapType::INVALID_DWARF_REG;
                        int offset = (loc.location == LocationTy::Kind::INDIRECT) ? loc.offsetOrSmallConstant : 0;
                        LLVMStackMapType::DwarfRegAndOffsetType info(regNum, offset);
                        callSiteInfo.emplace_back(info);
                        break;
                    }
                    default: {
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                        break;
                    }
                }
            }
        }
        FilterCallSiteInfo(callSiteInfo);
    };

    const size_t count = llvmStackMap_.stkSizeRecords.size();
    for (size_t i = 0; i < count; i++) {
        // relative offset
        struct StkMapSizeRecordTy &sizeRec = llvmStackMap_.stkSizeRecords[i];
        uintptr_t address = sizeRec.functionAddress;
        uint64_t recordCount = sizeRec.recordCount;
        fun2RecordNum_.emplace_back(std::make_pair(address, recordCount));
        for (uint64_t k = 0; k < recordCount; k++) {
            calStkMapRecordFunc(address, k);
        }
        recordNum += recordCount;
    }

    stackMapInfo.AppendCallSiteInfo(pc2CallSiteInfo);
    stackMapInfo.AppendDeoptInfo(deoptbundles);
}

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr)
{
    if (!stackMapAddr) {
        LOG_COMPILER(ERROR) << "stackMapAddr nullptr error ! ";
        return false;
    }
    dataInfo_ = std::make_unique<DataInfo>(std::move(stackMapAddr));
    llvmStackMap_.head = dataInfo_->Read<struct Header>();
    uint32_t numFunctions = dataInfo_->Read<uint32_t>();
    uint32_t numConstants = dataInfo_->Read<uint32_t>();
    uint32_t numRecords = dataInfo_->Read<uint32_t>();
    for (uint32_t i = 0; i < numFunctions; i++) {
        auto stkRecord = dataInfo_->Read<struct StkMapSizeRecordTy>();
        llvmStackMap_.stkSizeRecords.push_back(stkRecord);
    }

    for (uint32_t i = 0; i < numConstants; i++) {
        auto val = dataInfo_->Read<struct ConstantsTy>();
        llvmStackMap_.constants.push_back(val);
    }
    for (uint32_t i = 0; i < numRecords; i++) {
        struct StkMapRecordTy stkSizeRecord;
        auto head = dataInfo_->Read<struct StkMapRecordHeadTy>();
        stkSizeRecord.head = head;
        for (uint16_t j = 0; j < head.numLocations; j++) {
            auto location = dataInfo_->Read<struct LocationTy>();
            stkSizeRecord.locations.push_back(location);
        }
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            dataInfo_->Read<uint16_t>();
        }
        uint32_t numLiveOuts = dataInfo_->Read<uint32_t>();
        if (numLiveOuts > 0) {
            for (uint32_t j = 0; j < numLiveOuts; j++) {
                auto liveOut = dataInfo_->Read<struct LiveOutsTy>();
                stkSizeRecord.liveOuts.push_back(liveOut);
            }
        }
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            dataInfo_->Read<uint16_t>();
        }
        llvmStackMap_.stkMapRecord.push_back(stkSizeRecord);
    }
    CalcCallSite();
    return true;
}

uint32_t ARKCallsite::CalHeadSize() const
{
    uint32_t headSize = sizeof(head);
    return headSize;
}

uint32_t ARKCallsite::CalStackMapSize(Triple triple, size_t &stackmapNumReduced) const
{
    size_t stackmapSize = 0;
    size_t stackmapsNum = stackmaps.size();
    for (size_t i = 0; i < stackmapsNum; i += LLVMStackMapType::STACKMAP_PAIR_SIZE) {
        std::vector<uint8_t> valBase;
        size_t valSizeBase = 0;
        auto &stackmapBase = stackmaps.at(i);
        auto &stackmapDerived = stackmaps.at(i + 1);
        LLVMStackMapType::EncodeRegAndOffset(valBase, valSizeBase, stackmapBase.first, stackmapBase.second, triple);
        stackmapSize += valBase.size();
        if (stackmapBase.first != stackmapDerived.first || stackmapBase.second != stackmapDerived.second) {
            std::vector<uint8_t> valDerived;
            size_t valSizeDerived = 0;
            LLVMStackMapType::EncodeRegAndOffset(valDerived, valSizeDerived, stackmapDerived.first,
                stackmapDerived.second, triple);
            stackmapSize += valDerived.size();
        } else {
        // base ref, base ref and derived ref equal ===> remove repeated derived ref reg&offset
            stackmapNumReduced++;
        }
    }
    return stackmapSize;
}

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr, uintptr_t hostCodeSectionOffset)
{
    bool ret = CalculateStackMap(std::move(stackMapAddr));
    if (!ret) {
        return false;
    }

    OPTIONAL_LOG_COMPILER(DEBUG) << "stackmap calculate update funcitonaddress ";

    for (size_t i = 0; i < llvmStackMap_.stkSizeRecords.size(); i++) {
        uintptr_t hostAddr = llvmStackMap_.stkSizeRecords[i].functionAddress;
        uintptr_t offset = hostAddr - hostCodeSectionAddr + hostCodeSectionOffset;
        llvmStackMap_.stkSizeRecords[i].functionAddress = offset;
        OPTIONAL_LOG_COMPILER(DEBUG) << std::dec << i << "th function " << std::hex << hostAddr << " ---> "
                                     << " offset:" << offset;
    }
    stackMapInfo.PopCallSiteInfo();
    stackMapInfo.PopDeoptInfo();
    fun2RecordNum_.clear();
    CalcCallSite();
    return true;
}
}  // namespace panda::ecmascript::kungfu
