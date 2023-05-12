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
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/assembler/assembler.h"
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

void LLVMStackMapParser::CalcCallSite()
{
    uint64_t recordNum = 0;
    LLVMStackMapType::Pc2CallSiteInfo pc2CallSiteInfo;
    LLVMStackMapType::Pc2Deopt deoptbundles;
    auto calStkMapRecordFunc =
        [this, &recordNum, &pc2CallSiteInfo, &deoptbundles](uintptr_t address, uint32_t recordId) {
        struct StkMapRecordHeadTy recordHead = llvmStackMap_.stkMapRecord[recordNum + recordId].head;
        int lastDeoptIndex = -1;
        for (int j = 0; j < recordHead.numLocations; j++) {
            struct LocationTy loc = llvmStackMap_.stkMapRecord[recordNum + recordId].locations[j];
            uint32_t instructionOffset = recordHead.instructionOffset;
            uintptr_t callsite = address + instructionOffset;
            uint64_t patchPointID = recordHead.patchPointID;
            if (j == LocationTy::CONSTANT_DEOPT_CNT_INDEX) {
                ASSERT(loc.location == LocationTy::Kind::CONSTANT);
                lastDeoptIndex = loc.offsetOrSmallConstant + LocationTy::CONSTANT_DEOPT_CNT_INDEX;
            }
            if (loc.location == LocationTy::Kind::INDIRECT) {
                OPTIONAL_LOG_COMPILER(DEBUG) << "DwarfRegNum:" << loc.dwarfRegNum << " loc.OffsetOrSmallConstant:"
                    << loc.offsetOrSmallConstant << "address:" << address << " instructionOffset:" <<
                    instructionOffset << " callsite:" << "  patchPointID :" << std::hex << patchPointID <<
                    callsite;
                LLVMStackMapType::DwarfRegAndOffsetType info(loc.dwarfRegNum, loc.offsetOrSmallConstant);
                auto it = pc2CallSiteInfo.find(callsite);
                if (j > lastDeoptIndex) {
                    if (pc2CallSiteInfo.find(callsite) == pc2CallSiteInfo.end()) {
                        pc2CallSiteInfo.insert(std::pair<uintptr_t, LLVMStackMapType::CallSiteInfo>(callsite, {info}));
                    } else {
                        it->second.emplace_back(info);
                    }
                } else if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX) {
                    deoptbundles[callsite].push_back(info);
                }
            } else if (loc.location == LocationTy::Kind::CONSTANT) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    deoptbundles[callsite].push_back(loc.offsetOrSmallConstant);
                }
            } else if (loc.location == LocationTy::Kind::DIRECT) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    LLVMStackMapType::DwarfRegAndOffsetType info(loc.dwarfRegNum, loc.offsetOrSmallConstant);
                    deoptbundles[callsite].push_back(info);
                }
            } else if (loc.location == LocationTy::Kind::CONSTANTNDEX) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    LLVMStackMapType::LargeInt v = static_cast<LLVMStackMapType::LargeInt>(llvmStackMap_.
                        constants[loc.offsetOrSmallConstant].largeConstant);
                    deoptbundles[callsite].push_back(v);
                }
            } else {
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
            }
        }
    };
    for (size_t i = 0; i < llvmStackMap_.stkSizeRecords.size(); i++) {
        // relative offset
        uintptr_t address = llvmStackMap_.stkSizeRecords[i].functionAddress;
        uint64_t recordCount = llvmStackMap_.stkSizeRecords[i].recordCount;
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

uint32_t ARKCallsite::CalStackMapSize(Triple triple) const
{
    size_t stackmapSize = 0;
    for (auto &x : stackmaps) {
        std::vector<uint8_t> value;
        size_t valueSize = 0;
        LLVMStackMapType::EncodeRegAndOffset(value, valueSize, x.first, x.second, triple);
        stackmapSize += value.size();
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
    pc2CallSiteInfoVec_.pop_back();
    pc2DeoptVec_.pop_back();
    fun2RecordNum_.clear();
    CalcCallSite();
    return true;
}

void LLVMStackMapParser::CalculateFuncFpDelta(LLVMStackMapType::Func2FpDelta info, uint32_t moduleIndex)
{
    std::vector<LLVMStackMapType::Func2FpDelta> fun2FpDelta;
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
}  // namespace panda::ecmascript::kungfu
