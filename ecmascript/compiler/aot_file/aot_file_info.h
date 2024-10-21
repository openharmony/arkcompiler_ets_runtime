/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_INFO_H
#define ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_INFO_H

#include "ecmascript/common.h"
#include "ecmascript/compiler/aot_file/executed_memory_allocator.h"
#include "ecmascript/compiler/aot_file/module_section_des.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/deoptimizer/calleeReg.h"
#include "ecmascript/compiler/aot_file/func_entry_des.h"
#include "ecmascript/stackmap/ark_stackmap.h"

namespace panda::ecmascript {
class PUBLIC_API AOTFileInfo {
public:
    using FuncEntryDes = ecmascript::FuncEntryDes;
    using CallSignature = kungfu::CallSignature;
    using CalleeRegAndOffsetVec = kungfu::CalleeRegAndOffsetVec;
    using DwarfRegType = kungfu::LLVMStackMapType::DwarfRegType;
    using OffsetType = kungfu::LLVMStackMapType::OffsetType;
    using DwarfRegAndOffsetType = kungfu::LLVMStackMapType::DwarfRegAndOffsetType;
    AOTFileInfo() = default;
    virtual ~AOTFileInfo() = default;

    static constexpr uint32_t TEXT_SEC_ALIGN = 16;
    static constexpr uint32_t DATA_SEC_ALIGN = 8;
    static constexpr uint32_t PAGE_ALIGN = 4096;

    const FuncEntryDes &GetStubDes(int index) const
    {
        return entries_[index];
    }

    uint32_t GetEntrySize() const
    {
        return entries_.size();
    }

    const std::vector<FuncEntryDes> &GetStubs() const
    {
        return entries_;
    }

    const std::vector<ModuleSectionDes> &GetCodeUnits() const
    {
        return des_;
    }

    uint32_t GetStubNum() const
    {
        return entryNum_;
    }

    void SetStubNum(uint32_t n)
    {
        entryNum_ = n;
    }

    void AddEntry(CallSignature::TargetKind kind, bool isMainFunc, bool isFastCall, int indexInKind, uint64_t offset,
                  uint32_t fileIndex, uint32_t moduleIndex, int delta, uint32_t size, CalleeRegAndOffsetVec info = {})
    {
        FuncEntryDes des;
        if (memset_s(&des, sizeof(des), 0, sizeof(des)) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            return;
        }
        des.kind_ = kind;
        des.isMainFunc_ = isMainFunc;
        des.isFastCall_ = isFastCall;
        des.indexInKindOrMethodId_ = static_cast<uint32_t>(indexInKind);
        des.abcIndexInAi_ = fileIndex;
        des.codeAddr_ = offset;
        des.moduleIndex_ = moduleIndex;
        des.fpDeltaPrevFrameSp_ = delta;
        des.funcSize_ = size;
        des.calleeRegisterNum_ = info.size();
        DwarfRegType reg = 0;
        OffsetType regOffset = 0;
        for (size_t i = 0; i < info.size(); i++) {
            std::tie(reg, regOffset) = info[i];
            des.CalleeReg2Offset_[2 * i] = static_cast<int32_t>(reg);
            des.CalleeReg2Offset_[2 * i + 1] = static_cast<int32_t>(regOffset);
        }
        entries_.emplace_back(des);
    }

    const std::vector<ModuleSectionDes> &GetModuleSectionDes() const
    {
        return des_;
    }

    void UpdateStackMap(std::shared_ptr<uint8_t> ptr, uint32_t size, uint32_t moduleIdx)
    {
        ASSERT(moduleIdx < des_.size());
        ModuleSectionDes &des = des_[moduleIdx];
        des.SetArkStackMapPtr(ptr);
        des.SetArkStackMapSize(size);
    }

    size_t GetCodeUnitsNum() const
    {
        return des_.size();
    }

    void accumulateTotalSize(uint32_t size)
    {
        totalCodeSize_ += size;
    }

    uint32_t GetTotalCodeSize() const
    {
        return totalCodeSize_;
    }

    using CallSiteInfo = std::tuple<uint64_t, uint8_t *, int, CalleeRegAndOffsetVec>;

    bool CalCallSiteInfo(uintptr_t retAddr, CallSiteInfo &ret, bool isInStub, bool isDeopt) const;

    virtual void Destroy();

protected:
    ExecutedMemoryAllocator::ExeMem &GetStubsMem()
    {
        return stubsMem_;
    }

    MemMap &GetFileMapMem()
    {
        return fileMapMem_;
    }

    uint32_t entryNum_ {0};
    uint32_t moduleNum_ {0};
    uint32_t totalCodeSize_ {0};
    std::vector<FuncEntryDes> entries_ {};
    std::vector<ModuleSectionDes> des_ {};
    ExecutedMemoryAllocator::ExeMem stubsMem_ {};
    MemMap fileMapMem_ {};

private:
    AOTFileInfo::FuncEntryDes GetFuncEntryDesWithCallsite(uintptr_t codeAddr, uint32_t startIndex,
                                                          uint32_t funcCount) const;

    void StoreCalleeRegInfo(uint32_t calleeRegNum, int32_t *calleeReg2Offset,
                            CalleeRegAndOffsetVec &calleeRegInfo) const;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_INFO_H
