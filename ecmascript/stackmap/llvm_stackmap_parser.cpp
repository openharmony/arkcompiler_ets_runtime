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

#include "ecmascript/aot_file_manager.h"
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

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr)
{
    bool ret = CalculateStackMap(std::move(stackMapAddr));
    if (!ret) {
        return false;
    }

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
    return true;
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
}  // namespace panda::ecmascript::kungfu
