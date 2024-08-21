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

#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/compiler/aot_file/func_entry_des.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jit/jit.h"
#ifdef CODE_SIGN_ENABLE
#include "jit_buffer_integrity.h"
#endif

namespace panda::ecmascript {
#ifdef CODE_SIGN_ENABLE
using namespace OHOS::Security::CodeSign;
#endif

static bool SetPageProtect(uint8_t *textStart, size_t dataSize)
{
    if (!Jit::GetInstance()->IsEnableJitFort()) {
        constexpr size_t pageSize = 4096;
        uintptr_t startPage = AlignDown(reinterpret_cast<uintptr_t>(textStart), pageSize);
        uintptr_t endPage = AlignUp(reinterpret_cast<uintptr_t>(textStart) + dataSize, pageSize);
        size_t protSize = endPage - startPage;
        return PageProtect(reinterpret_cast<void*>(startPage), protSize, PAGE_PROT_EXEC_READWRITE);
    }
    return true;
}

bool MachineCode::SetText(const MachineCodeDesc &desc)
{
    uint8_t *textStart = reinterpret_cast<uint8_t*>(GetText());
    uint8_t *pText = textStart;
    if (desc.rodataSizeBeforeTextAlign != 0) {
        if (memcpy_s(pText, desc.rodataSizeBeforeTextAlign,
            reinterpret_cast<uint8_t*>(desc.rodataAddrBeforeText), desc.rodataSizeBeforeText) != EOK) {
            LOG_JIT(ERROR) << "memcpy fail in copy fast jit code";
            return false;
        }
        pText += desc.rodataSizeBeforeTextAlign;
    }
    if (!Jit::GetInstance()->IsEnableJitFort() || !Jit::GetInstance()->IsEnableAsyncCopyToFort() ||
        !desc.isAsyncCompileMode) {
#ifdef CODE_SIGN_ENABLE
        if ((uintptr_t)desc.codeSigner == 0) {
            if (memcpy_s(pText, desc.codeSizeAlign, reinterpret_cast<uint8_t*>(desc.codeAddr), desc.codeSize) != EOK) {
                LOG_JIT(ERROR) << "memcpy fail in copy fast jit code";
                return false;
            }
        } else {
            LOG_JIT(DEBUG) << "Call JitVerifyAndCopy: "
                           << std::hex << (uintptr_t)pText << " <- "
                           << std::hex << (uintptr_t)desc.codeAddr << " size: " << desc.codeSize;
            LOG_JIT(DEBUG) << "     codeSigner = " << std::hex << (uintptr_t)desc.codeSigner;
            if (Jit::GetInstance()->JitVerifyAndCopy(reinterpret_cast<void*>(desc.codeSigner),
                pText, reinterpret_cast<void*>(desc.codeAddr), desc.codeSize) != EOK) {
                LOG_JIT(ERROR) << "     JitVerifyAndCopy failed";
            } else {
                LOG_JIT(DEBUG) << "     JitVerifyAndCopy success!!";
            }
            delete reinterpret_cast<JitCodeSignerBase*>(desc.codeSigner);
        }
#else
        if (memcpy_s(pText, desc.codeSizeAlign, reinterpret_cast<uint8_t*>(desc.codeAddr), desc.codeSize) != EOK) {
            LOG_JIT(ERROR) << "memcpy fail in copy fast jit code";
            return false;
        }
#endif
    }
    pText += desc.codeSizeAlign;
    if (desc.rodataSizeAfterTextAlign != 0) {
        if (memcpy_s(pText, desc.rodataSizeAfterTextAlign,
            reinterpret_cast<uint8_t*>(desc.rodataAddrAfterText), desc.rodataSizeAfterText) != EOK) {
            LOG_JIT(ERROR) << "memcpy fail in copy fast jit code";
            return false;
        }
    }
    return true;
}

bool MachineCode::SetNonText(const MachineCodeDesc &desc, EntityId methodId)
{
    uint8_t *textStart = reinterpret_cast<uint8_t*>(GetText());
    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    if (memcpy_s(stackmapAddr, desc.stackMapOrOffsetTableSize,
                 reinterpret_cast<uint8_t*>(desc.stackMapOrOffsetTableAddr),
                 desc.stackMapOrOffsetTableSize) != EOK) {
        LOG_JIT(ERROR) << "memcpy fail in copy fast jit stackmap";
        return false;
    }

    FuncEntryDes *funcEntry = reinterpret_cast<FuncEntryDes*>(GetFuncEntryDesAddress());
    if (memcpy_s(funcEntry, desc.funcEntryDesSize, reinterpret_cast<uint8_t*>(desc.funcEntryDesAddr),
        desc.funcEntryDesSize) != EOK) {
        LOG_JIT(ERROR) << "memcpy fail in copy fast jit funcEntry";
        return false;
    }

    uint32_t cnt = desc.funcEntryDesSize / sizeof(FuncEntryDes);
    ASSERT(cnt <= 2); // 2: jsfunction + deoptimize stub
    for (uint32_t i = 0; i < cnt; i++) {
        funcEntry->codeAddr_ += reinterpret_cast<uintptr_t>(textStart);
        if (methodId == EntityId(funcEntry->indexInKindOrMethodId_)) {
            SetFuncAddr(funcEntry->codeAddr_);
        }
        funcEntry++;
    }
    return true;
}

bool MachineCode::SetData(const MachineCodeDesc &desc, JSHandle<Method> &method, size_t dataSize)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (desc.codeType == MachineCodeType::BASELINE_CODE) {
        return SetBaselineCodeData(desc, method, dataSize);
    }

    SetOSROffset(MachineCode::INVALID_OSR_OFFSET);
    SetOsrDeoptFlag(false);
    SetOsrExecuteCnt(0);

    size_t instrSize = desc.rodataSizeBeforeTextAlign + desc.codeSizeAlign + desc.rodataSizeAfterTextAlign;

    SetFuncEntryDesSize(desc.funcEntryDesSizeAlign);
    SetInstructionsSize(instrSize);
    SetStackMapOrOffsetTableSize(desc.stackMapSizeAlign);
    SetPayLoadSizeInBytes(dataSize);
    if (Jit::GetInstance()->IsEnableJitFort()) {
        SetInstructionsAddr(desc.instructionsAddr);
        ASSERT(desc.instructionsAddr != 0);
        ASSERT(dataSize == (desc.funcEntryDesSizeAlign + desc.stackMapSizeAlign) ||
               dataSize == (desc.funcEntryDesSizeAlign + instrSize + desc.stackMapSizeAlign));
    } else {
        ASSERT(dataSize == (desc.funcEntryDesSizeAlign + instrSize + desc.stackMapSizeAlign));
    }
    if (!SetText(desc)) {
        return false;
    }
    if (!SetNonText(desc, method->GetMethodId())) {
        return false;
    }

    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    uint32_t cnt = desc.funcEntryDesSize / sizeof(FuncEntryDes);
    uint8_t *textStart = reinterpret_cast<uint8_t*>(GetText());
    CString methodName = method->GetRecordNameStr() + "." + CString(method->GetMethodName());
    LOG_JIT(DEBUG) << "Fast JIT MachineCode :" << methodName << ", "  << " text addr:" <<
        reinterpret_cast<void*>(GetText()) << ", size:" << instrSize  <<
        ", stackMap addr:" << reinterpret_cast<void*>(stackmapAddr) <<
        ", size:" << desc.stackMapSizeAlign <<
        ", funcEntry addr:" << reinterpret_cast<void*>(GetFuncEntryDesAddress()) << ", count:" << cnt;

    if (!SetPageProtect(textStart, dataSize)) {
        LOG_JIT(ERROR) << "MachineCode::SetData SetPageProtect failed";
        return false;
    }
    return true;
}

bool MachineCode::SetBaselineCodeData(const MachineCodeDesc &desc,
                                      JSHandle<Method> &method, size_t dataSize)
{
    DISALLOW_GARBAGE_COLLECTION;
    SetFuncEntryDesSize(0);

    size_t instrSizeAlign = desc.codeSizeAlign;
    SetInstructionsSize(instrSizeAlign);

    SetStackMapOrOffsetTableSize(desc.stackMapSizeAlign);
    if (Jit::GetInstance()->IsEnableJitFort()) {
        ASSERT(desc.instructionsAddr != 0);
        ASSERT(dataSize == (desc.stackMapSizeAlign) ||  // reg. obj
               dataSize == (instrSizeAlign + desc.stackMapSizeAlign)); // huge obj
        SetInstructionsAddr(desc.instructionsAddr);
    } else {
        ASSERT(dataSize == (instrSizeAlign + desc.stackMapSizeAlign));
    }
    SetPayLoadSizeInBytes(dataSize);

    uint8_t *textStart = reinterpret_cast<uint8_t*>(GetText());
    if (Jit::GetInstance()->IsEnableJitFort()) {
        // relax assert for now until machine code padding for JitFort resolved
        ASSERT(IsAligned(reinterpret_cast<uintptr_t>(textStart), TEXT_ALIGN) ||
            IsAligned(reinterpret_cast<uintptr_t>(textStart), DATA_ALIGN));
    } else {
        ASSERT(IsAligned(reinterpret_cast<uintptr_t>(textStart), TEXT_ALIGN));
    }
    uint8_t *pText = textStart;
    if (memcpy_s(pText, instrSizeAlign, reinterpret_cast<uint8_t*>(desc.codeAddr), desc.codeSize) != EOK) {
        LOG_BASELINEJIT(ERROR) << "memcpy fail in copy baseline jit code";
        return false;
    }
    pText += instrSizeAlign;

    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    if (memcpy_s(stackmapAddr, desc.stackMapOrOffsetTableSize,
                 reinterpret_cast<uint8_t*>(desc.stackMapOrOffsetTableAddr),
                 desc.stackMapOrOffsetTableSize) != EOK) {
        LOG_BASELINEJIT(ERROR) << "memcpy fail in copy fast baselineJIT offsetTable";
        return false;
    }

    SetFuncAddr(reinterpret_cast<uintptr_t>(textStart));

    CString methodName = method->GetRecordNameStr() + "." + CString(method->GetMethodName());
    LOG_BASELINEJIT(DEBUG) << "BaselineCode :" << methodName << ", "  << " text addr:" <<
        reinterpret_cast<void*>(GetText()) << ", size:" << instrSizeAlign  <<
        ", stackMap addr: 0, size: 0, funcEntry addr: 0, count 0";

    if (!SetPageProtect(textStart, dataSize)) {
        LOG_BASELINEJIT(ERROR) << "MachineCode::SetBaseLineCodeData SetPageProtect failed";
        return false;
    }
    return true;
}

bool MachineCode::IsInText(const uintptr_t pc) const
{
    uintptr_t textStart = GetText();
    uintptr_t textEnd = textStart + GetTextSize();
    return textStart <= pc && pc < textEnd;
}

uintptr_t MachineCode::GetFuncEntryDes() const
{
    uint32_t funcEntryCnt = GetFuncEntryDesSize() / sizeof(FuncEntryDes);
    FuncEntryDes *funcEntryDes = reinterpret_cast<FuncEntryDes*>(GetFuncEntryDesAddress());
    for (uint32_t i = 0; i < funcEntryCnt; i++) {
        if (funcEntryDes->codeAddr_ == GetFuncAddr()) {
            return reinterpret_cast<uintptr_t>(funcEntryDes);
        }
        funcEntryDes++;
    }
    UNREACHABLE();
    return 0;
}

std::tuple<uint64_t, uint8_t*, int, kungfu::CalleeRegAndOffsetVec> MachineCode::CalCallSiteInfo(uintptr_t retAddr) const
{
    uintptr_t textStart = GetText();
    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    ASSERT(stackmapAddr != nullptr);

    uint32_t funcEntryCnt = GetFuncEntryDesSize() / sizeof(FuncEntryDes);
    FuncEntryDes *tmpFuncEntryDes = reinterpret_cast<FuncEntryDes*>(GetFuncEntryDesAddress());
    FuncEntryDes *funcEntryDes = nullptr;
    ASSERT(tmpFuncEntryDes != nullptr);
    uintptr_t pc = retAddr - 1;  // -1: for pc
    for (uint32_t i = 0; i < funcEntryCnt; i++) {
        if (tmpFuncEntryDes->codeAddr_ <= pc && pc < (tmpFuncEntryDes->codeAddr_ + tmpFuncEntryDes->funcSize_)) {
            funcEntryDes = tmpFuncEntryDes;
            break;
        }
        tmpFuncEntryDes++;
    }

    if (funcEntryDes == nullptr) {
        return {};
    }

    int delta = funcEntryDes->fpDeltaPrevFrameSp_;
    kungfu::CalleeRegAndOffsetVec calleeRegInfo;
    for (uint32_t j = 0; j < funcEntryDes->calleeRegisterNum_; j++) {
        kungfu::LLVMStackMapType::DwarfRegType reg =
            static_cast<kungfu::LLVMStackMapType::DwarfRegType>(funcEntryDes->CalleeReg2Offset_[2 * j]);
        kungfu::LLVMStackMapType::OffsetType offset =
            static_cast<kungfu::LLVMStackMapType::OffsetType>(funcEntryDes->CalleeReg2Offset_[2 * j + 1]);
        kungfu::LLVMStackMapType::DwarfRegAndOffsetType regAndOffset = std::make_pair(reg, offset);
        calleeRegInfo.emplace_back(regAndOffset);
    }
    auto ret = std::make_tuple(textStart, stackmapAddr, delta, calleeRegInfo);
    return ret;
}

uintptr_t MachineCode::GetText() const
{
    if (Jit::GetInstance()->IsEnableJitFort()) {
        return GetInstructionsAddr();
    } else {
        return GetFuncEntryDesAddress() + GetFuncEntryDesSize();
    }
}

uint8_t *MachineCode::GetStackMapOrOffsetTableAddress() const
{
    if (Jit::GetInstance()->IsEnableJitFort()) {
        // stackmap immediately follows FuncEntryDesc area
        return reinterpret_cast<uint8_t*>(GetFuncEntryDesAddress() + GetFuncEntryDesSize());
    } else {
        return reinterpret_cast<uint8_t*>(GetText() + GetInstructionsSize());
    }
}

}  // namespace panda::ecmascript
