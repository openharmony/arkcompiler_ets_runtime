/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/code_generator.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/aot_file/aot_file_info.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

CodeInfo::CodeInfo(CodeSpaceOnDemand &codeSpaceOnDemand, bool useOwnSpace)
    : codeSpaceOnDemand_(codeSpaceOnDemand),
      useOwnSpace_(useOwnSpace)
{
    secInfos_.fill(std::make_pair(nullptr, 0));
    if (useOwnSpace_) {
        ownCodeSpace_ = std::make_unique<CodeSpace>();
    }
}

CodeInfo::~CodeInfo()
{
    Reset();
}

CodeInfo::CodeSpace *CodeInfo::CodeSpace::GetInstance()
{
    static CodeSpace *codeSpace = new CodeSpace();
    return codeSpace;
}

CodeInfo::CodeSpace::CodeSpace()
{
    ASSERT(REQUIRED_SECS_LIMIT == AlignUp(REQUIRED_SECS_LIMIT, PageSize()));
    reqSecs_ = static_cast<uint8_t *>(PageMap(REQUIRED_SECS_LIMIT, PAGE_PROT_READWRITE).GetMem());
    if (reqSecs_ == reinterpret_cast<uint8_t *>(-1)) {
        reqSecs_ = nullptr;
    }
    ASSERT(UNREQUIRED_SECS_LIMIT == AlignUp(UNREQUIRED_SECS_LIMIT, PageSize()));
    unreqSecs_ = static_cast<uint8_t *>(PageMap(UNREQUIRED_SECS_LIMIT, PAGE_PROT_READWRITE).GetMem());
    if (unreqSecs_ == reinterpret_cast<uint8_t *>(-1)) {
        unreqSecs_ = nullptr;
    }
}

CodeInfo::CodeSpace::~CodeSpace()
{
    reqBufPos_ = 0;
    unreqBufPos_ = 0;
    if (reqSecs_ != nullptr) {
        PageUnmap(MemMap(reqSecs_, REQUIRED_SECS_LIMIT));
    }
    reqSecs_ = nullptr;
    if (unreqSecs_ != nullptr) {
        PageUnmap(MemMap(unreqSecs_, UNREQUIRED_SECS_LIMIT));
    }
    unreqSecs_ = nullptr;
}

uint8_t *CodeInfo::CodeSpace::Alloca(uintptr_t size, bool isReq, size_t alignSize)
{
    // Wait other threads arrived here, then allocate in same time.
    ConcurrentMonitor::monitor_.ArriveAndWait();
    uint8_t *addr = nullptr;
    auto bufBegin = isReq ? reqSecs_ : unreqSecs_;
    auto &curPos = isReq ? reqBufPos_ : unreqBufPos_;
    size_t limit = isReq ? REQUIRED_SECS_LIMIT : UNREQUIRED_SECS_LIMIT;
    if (curPos + size > limit) {
        LOG_COMPILER(ERROR) << std::hex << "Alloca Section failed. Current curPos:" << curPos
                            << " plus size:" << size << "exceed limit:" << limit;
        exit(-1);
    }
    if (alignSize > 0) {
        curPos = AlignUp(curPos, alignSize);
    }
    addr = bufBegin + curPos;
    curPos += size;
    return addr;
}

uint8_t *CodeInfo::CodeSpaceOnDemand::Alloca(uintptr_t size, [[maybe_unused]] bool isReq, size_t alignSize)
{
    // Always apply for an aligned memory block here.
    auto alignedSize = alignSize > 0 ? AlignUp(size, alignSize) : size;
    // Verify the size and temporarily use REQUIREd_SECS.LIMITED as the online option, allowing for adjustments.
    if (alignedSize > SECTION_LIMIT) {
        LOG_COMPILER(FATAL) << std::hex << "invalid memory size: " << alignedSize;
        return nullptr;
    }
    uint8_t *addr = static_cast<uint8_t *>(malloc(alignedSize));
    if (addr == nullptr) {
        LOG_COMPILER(FATAL) << "malloc section failed.";
        return nullptr;
    }
    sections_.push_back({addr, alignedSize});
    return addr;
}

CodeInfo::CodeSpaceOnDemand::~CodeSpaceOnDemand()
{
    // release all used memory.
    for (auto &section : sections_) {
        if ((section.first != nullptr) && (section.second != 0)) {
            free(section.first);
        }
    }
    sections_.clear();
}

uint8_t *CodeInfo::AllocaOnDemand(uintptr_t size, size_t alignSize)
{
    return codeSpaceOnDemand_.Alloca(size, true, alignSize);
}

uint8_t *CodeInfo::AllocaInReqSecBuffer(uintptr_t size, size_t alignSize)
{
    if (useOwnSpace_) {
        return ownCodeSpace_->Alloca(size, true, alignSize);
    }
    return CodeSpace::GetInstance()->Alloca(size, true, alignSize);
}

uint8_t *CodeInfo::AllocaInNotReqSecBuffer(uintptr_t size, size_t alignSize)
{
    if (useOwnSpace_) {
        return ownCodeSpace_->Alloca(size, false, alignSize);
    }
    return CodeSpace::GetInstance()->Alloca(size, false, alignSize);
}

uint8_t *CodeInfo::AllocaCodeSectionImp(uintptr_t size, const char *sectionName,
                                        AllocaSectionCallback allocaInReqSecBuffer)
{
    uint8_t *addr = nullptr;
    auto curSec = ElfSection(sectionName);
    if (curSec.isValidAOTSec()) {
        if (!alreadyPageAlign_) {
            addr = (this->*allocaInReqSecBuffer)(size, AOTFileInfo::PAGE_ALIGN);
            alreadyPageAlign_ = true;
            VerifyAddress(reinterpret_cast<uintptr_t>(addr), size, AOTFileInfo::PAGE_ALIGN);
        } else {
            addr = (this->*allocaInReqSecBuffer)(size, AOTFileInfo::TEXT_SEC_ALIGN);
            VerifyAddress(reinterpret_cast<uintptr_t>(addr), size, AOTFileInfo::TEXT_SEC_ALIGN);
        }
    } else {
        addr = (this->*allocaInReqSecBuffer)(size, 0);
        VerifyAddress(reinterpret_cast<uintptr_t>(addr), size, 0);
    }
    codeInfo_.push_back({addr, size});
    if (curSec.isValidAOTSec()) {
        secInfos_[curSec.GetIntIndex()] = std::make_pair(addr, size);
    }
    return addr;
}

uint8_t *CodeInfo::AllocaCodeSection(uintptr_t size, const char *sectionName)
{
    return AllocaCodeSectionImp(size, sectionName, &CodeInfo::AllocaInReqSecBuffer);
}

uint8_t *CodeInfo::AllocaCodeSectionOnDemand(uintptr_t size, const char *sectionName)
{
    return AllocaCodeSectionImp(size, sectionName, &CodeInfo::AllocaOnDemand);
}

void CodeInfo::VerifyAddress(uintptr_t addr, uintptr_t size, uintptr_t alignSize)
{
    if (!useOwnSpace_) {
        return;
    }
    if (lastAddr_ == 0) {
        lastAddr_ = addr;
        lastSize_ = size;
        return;
    }
    uintptr_t expectAddr = lastAddr_ + lastSize_;
    if (alignSize != 0 && !IsAligned(expectAddr, alignSize)) {
        expectAddr = AlignUp(expectAddr, alignSize);
    }
    if (expectAddr != addr) {
        LOG_COMPILER(FATAL) << "VerifyAddress failed: " << lastAddr_ << " " << lastSize_ << " " << alignSize <<
                ", addr: " << addr;
    }
    lastAddr_ = addr;
    lastSize_ = size;
}

uint8_t *CodeInfo::AllocaDataSectionImp(uintptr_t size, const char *sectionName,
                                        AllocaSectionCallback allocaInReqSecBuffer,
                                        AllocaSectionCallback allocaInNotReqSecBuffer)
{
    uint8_t *addr = nullptr;
    auto curSec = ElfSection(sectionName);
    // rodata section needs 16 bytes alignment
    if (curSec.InRodataSection()) {
        size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        if (!alreadyPageAlign_) {
            addr = curSec.isSequentialAOTSec() ? (this->*allocaInReqSecBuffer)(size, AOTFileInfo::PAGE_ALIGN)
                                               : (this->*allocaInNotReqSecBuffer)(size, AOTFileInfo::PAGE_ALIGN);
            alreadyPageAlign_ = true;
        } else {
            uint32_t alignedSize = curSec.InRodataSection() ? AOTFileInfo::RODATA_SEC_ALIGN
                                                            : AOTFileInfo::DATA_SEC_ALIGN;
            addr = curSec.isSequentialAOTSec() ? (this->*allocaInReqSecBuffer)(size, alignedSize)
                                               : (this->*allocaInNotReqSecBuffer)(size, alignedSize);
        }
    } else {
        addr = curSec.isSequentialAOTSec() ? (this->*allocaInReqSecBuffer)(size, 0)
                                           : (this->*allocaInNotReqSecBuffer)(size, 0);
    }
    if (curSec.isValidAOTSec()) {
        secInfos_[curSec.GetIntIndex()] = std::make_pair(addr, size);
    }
    return addr;

}

uint8_t *CodeInfo::AllocaDataSection(uintptr_t size, const char *sectionName)
{
    return AllocaDataSectionImp(size, sectionName, &CodeInfo::AllocaInReqSecBuffer, &CodeInfo::AllocaInNotReqSecBuffer);
}

uint8_t *CodeInfo::AllocaDataSectionOnDemand(uintptr_t size, const char *sectionName)
{
    return AllocaDataSectionImp(size, sectionName, &CodeInfo::AllocaOnDemand, &CodeInfo::AllocaOnDemand);
}

void CodeInfo::SaveFunc2Addr(std::string funcName, uint32_t address)
{
    auto itr = func2FuncInfo.find(funcName);
    if (itr != func2FuncInfo.end()) {
        itr->second.addr = address;
        return;
    }
    func2FuncInfo.insert(
        std::pair<std::string, FuncInfo>(funcName, {address, 0, kungfu::CalleeRegAndOffsetVec()}));
}

void CodeInfo::SaveFunc2FPtoPrevSPDelta(std::string funcName, int32_t fp2PrevSpDelta)
{
    auto itr = func2FuncInfo.find(funcName);
    if (itr != func2FuncInfo.end()) {
        itr->second.fp2PrevFrameSpDelta = fp2PrevSpDelta;
        return;
    }
    func2FuncInfo.insert(
        std::pair<std::string, FuncInfo>(funcName, {0, fp2PrevSpDelta, kungfu::CalleeRegAndOffsetVec()}));
}

void CodeInfo::SaveFunc2CalleeOffsetInfo(std::string funcName, kungfu::CalleeRegAndOffsetVec calleeRegInfo)
{
    auto itr = func2FuncInfo.find(funcName);
    if (itr != func2FuncInfo.end()) {
        itr->second.calleeRegInfo = calleeRegInfo;
        return;
    }
    func2FuncInfo.insert(
        std::pair<std::string, FuncInfo>(funcName, {0, 0, calleeRegInfo}));
}

void CodeInfo::SavePC2DeoptInfo(uint64_t pc, std::vector<uint8_t> deoptInfo)
{
    pc2DeoptInfo.insert(std::pair<uint64_t, std::vector<uint8_t>>(pc, deoptInfo));
}

void CodeInfo::SavePC2CallSiteInfo(uint64_t pc, std::vector<uint8_t> callSiteInfo)
{
    pc2CallsiteInfo.insert(std::pair<uint64_t, std::vector<uint8_t>>(pc, callSiteInfo));
}

const std::map<std::string, CodeInfo::FuncInfo> &CodeInfo::GetFuncInfos() const
{
    return func2FuncInfo;
}

const std::map<uint64_t, std::vector<uint8_t>> &CodeInfo::GetPC2DeoptInfo() const
{
    return pc2DeoptInfo;
}

const std::unordered_map<uint64_t, std::vector<uint8_t>> &CodeInfo::GetPC2CallsiteInfo() const
{
    return pc2CallsiteInfo;
}

void CodeInfo::Reset()
{
    codeInfo_.clear();
}

uint8_t *CodeInfo::GetSectionAddr(ElfSecName sec) const
{
    auto curSection = ElfSection(sec);
    auto idx = curSection.GetIntIndex();
    return const_cast<uint8_t *>(secInfos_[idx].first);
}

size_t CodeInfo::GetSectionSize(ElfSecName sec) const
{
    auto curSection = ElfSection(sec);
    auto idx = curSection.GetIntIndex();
    return secInfos_[idx].second;
}

const std::vector<std::pair<uint8_t *, uintptr_t>> &CodeInfo::GetCodeInfo() const
{
    return codeInfo_;
}
}  // namespace panda::ecmascript::kungfu