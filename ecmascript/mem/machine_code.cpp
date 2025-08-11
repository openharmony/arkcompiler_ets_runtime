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
#include "ecmascript/base/config.h"
#include "ecmascript/compiler/aot_file/func_entry_des.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#if ECMASCRIPT_ENABLE_CAST_CHECK
#include "ecmascript/js_tagged_value-inl.h"
#endif

namespace panda::ecmascript {

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

static int MachineCodeCopyToCache([[maybe_unused]] const MachineCodeDesc &desc, [[maybe_unused]] uint8_t *pText)
{
#ifdef JIT_ENABLE_CODE_SIGN
    if ((uintptr_t)desc.codeSigner == 0) {
        if (memcpy_s(pText, desc.codeSizeAlign, reinterpret_cast<uint8_t*>(desc.codeAddr), desc.codeSize) != EOK) {
            LOG_JIT(ERROR) << "memcpy failed in CopyToCache";
            return false;
        }
    } else {
        LOG_JIT(DEBUG) << "Copy: " << std::hex << (uintptr_t)pText << " <- " << std::hex << (uintptr_t)desc.codeAddr
                       << " size: " << desc.codeSize;
        LOG_JIT(DEBUG) << "     codeSigner = " << std::hex << (uintptr_t)desc.codeSigner;
        OHOS::Security::CodeSign::JitCodeSigner* signer =
            reinterpret_cast<OHOS::Security::CodeSign::JitCodeSigner*>(desc.codeSigner);
        int err = OHOS::Security::CodeSign::CopyToJitCode(signer, pText, reinterpret_cast<void*>(desc.codeAddr),
                                                          desc.codeSize);
        if (err != EOK) {
            LOG_JIT(ERROR) << "     CopyToJitCode failed, err: " << err;
            return false;
        } else {
            LOG_JIT(DEBUG) << "     CopyToJitCode success!!";
        }
        delete reinterpret_cast<OHOS::Security::CodeSign::JitCodeSigner*>(desc.codeSigner);
    }
#else
    if (memcpy_s(pText, desc.codeSizeAlign, // LCOV_EXCL_BR_LINE
        reinterpret_cast<uint8_t*>(desc.codeAddr),
        desc.codeSize) != EOK) {
        LOG_JIT(ERROR) << "memcpy failed in CopyToCache";
        return false;
    }
#endif
    return true;
}

bool MachineCode::SetText(const MachineCodeDesc &desc)
{
    uint8_t *textStart = reinterpret_cast<uint8_t*>(GetText());
    uint8_t *pText = textStart;
    if (desc.rodataSizeBeforeTextAlign != 0) {
        if (memcpy_s(pText, desc.rodataSizeBeforeTextAlign, // LCOV_EXCL_BR_LINE
            reinterpret_cast<uint8_t*>(desc.rodataAddrBeforeText),
            desc.rodataSizeBeforeText) != EOK) {
            LOG_JIT(ERROR) << "memcpy fail in copy fast jit code";
            return false;
        }
        pText += desc.rodataSizeBeforeTextAlign;
    }
    if (!Jit::GetInstance()->IsEnableJitFort() || !Jit::GetInstance()->IsEnableAsyncCopyToFort() ||
        !desc.isAsyncCompileMode) {
        if (MachineCodeCopyToCache(desc, pText) == false) {
            return false;
        }
    }
    pText += desc.codeSizeAlign;
    if (desc.rodataSizeAfterTextAlign != 0) {
        if (memcpy_s(pText, desc.rodataSizeAfterTextAlign, // LCOV_EXCL_BR_LINE
            reinterpret_cast<uint8_t*>(desc.rodataAddrAfterText),
            desc.rodataSizeAfterText) != EOK) {
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
    if (memcpy_s(stackmapAddr, desc.stackMapOrOffsetTableSize, // LCOV_EXCL_BR_LINE
                 reinterpret_cast<uint8_t*>(desc.stackMapOrOffsetTableAddr),
                 desc.stackMapOrOffsetTableSize) != EOK) {
        LOG_JIT(ERROR) << "memcpy fail in copy fast jit stackmap";
        return false;
    }

    FuncEntryDes *funcEntry = reinterpret_cast<FuncEntryDes*>(desc.funcEntryDesAddr);
    if (!funcEntry) {
        LOG_JIT(ERROR) << "funcEntry is null.";
        return false;
    }
    uint32_t cnt = desc.funcEntryDesSize / sizeof(FuncEntryDes);
    ASSERT(cnt <= 2); // 2: jsfunction + deoptimize stub
    for (uint32_t i = 0; i < cnt; i++) {
        if (methodId == EntityId(funcEntry->indexInKindOrMethodId_)) {
            uint64_t codeAddr = funcEntry->codeAddr_ +
                                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(textStart));
            SetFuncAddr(codeAddr);
            break;
        }
        funcEntry++;
    }

    SetIsFastCall(funcEntry->isFastCall_);
    SetFpDeltaPrevFrameSp(funcEntry->fpDeltaPrevFrameSp_);
    SetFuncSize(funcEntry->funcSize_);
    SetCalleeRegisterNum(funcEntry->calleeRegisterNum_);
    SetCalleeReg2OffsetArray(funcEntry->CalleeReg2Offset_);
    return true;
}

bool MachineCode::SetData(JSThread *thread, const MachineCodeDesc &desc, JSHandle<Method> &method, size_t dataSize)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (desc.codeType == MachineCodeType::BASELINE_CODE) {
        return SetBaselineCodeData(thread, desc, method, dataSize);
    }

    if (desc.isHugeObj) {
        SetLocalHeapAddress(0);
    } else {
        SetLocalHeapAddress(reinterpret_cast<uint64_t>(thread->GetEcmaVM()->GetHeap()));
    }

    SetOSROffset(MachineCode::INVALID_OSR_OFFSET);
    SetOsrDeoptFlag(false);
    SetOsrExecuteCnt(0);

    size_t instrSize = desc.rodataSizeBeforeTextAlign + desc.codeSizeAlign + desc.rodataSizeAfterTextAlign;

    SetInstructionsSize(instrSize);
    SetStackMapOrOffsetTableSize(desc.stackMapSizeAlign);
    SetHeapConstantTableSize(desc.heapConstantTableSizeAlign);
    SetHeapConstantTableAddr(reinterpret_cast<uint64_t>(GetHeapConstantTableAddress()));
    SetPayLoadSizeInBytes(dataSize);
    if (Jit::GetInstance()->IsEnableJitFort()) {
        SetInstructionsAddr(desc.instructionsAddr);
        ASSERT(desc.instructionsAddr != 0);
        ASSERT(dataSize == (desc.funcEntryDesSizeAlign + desc.stackMapSizeAlign + desc.heapConstantTableSizeAlign) ||
               dataSize == (desc.funcEntryDesSizeAlign + instrSize + desc.stackMapSizeAlign +
                            desc.heapConstantTableSizeAlign));
    } else {
        ASSERT(dataSize == (desc.funcEntryDesSizeAlign + instrSize +
                            desc.stackMapSizeAlign + desc.heapConstantTableSizeAlign));
    }
    if (!SetText(desc)) {
        return false;
    }
    if (!SetNonText(desc, method->GetMethodId())) {
        return false;
    }

    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    uint8_t *textStart = reinterpret_cast<uint8_t*>(GetText());
    CString methodName = method->GetRecordNameStr(thread) + "." + CString(method->GetMethodName(thread));
    LOG_JIT(DEBUG) << "Fast JIT MachineCode :" << methodName << ", "  << " text addr:" <<
        reinterpret_cast<void*>(GetText()) << ", size:" << instrSize  <<
        ", stackMap addr:" << reinterpret_cast<void*>(stackmapAddr) <<
        ", size:" << desc.stackMapSizeAlign;

    if (!SetPageProtect(textStart, dataSize)) {
        LOG_JIT(ERROR) << "MachineCode::SetData SetPageProtect failed";
        return false;
    }
    return true;
}

bool MachineCode::SetBaselineCodeData(JSThread *thread, const MachineCodeDesc &desc,
                                      JSHandle<Method> &method, size_t dataSize)
{
    DISALLOW_GARBAGE_COLLECTION;

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

    if (!Jit::GetInstance()->IsEnableJitFort() || !Jit::GetInstance()->IsEnableAsyncCopyToFort() ||
        !desc.isAsyncCompileMode) {
        if (MachineCodeCopyToCache(desc, pText) == false) {
            return false;
        }
    }
    pText += instrSizeAlign;

    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    if (memcpy_s(stackmapAddr, desc.stackMapOrOffsetTableSize, // LCOV_EXCL_BR_LINE
                 reinterpret_cast<uint8_t*>(desc.stackMapOrOffsetTableAddr),
                 desc.stackMapOrOffsetTableSize) != EOK) {
        LOG_BASELINEJIT(ERROR) << "memcpy fail in copy fast baselineJIT offsetTable";
        return false;
    }

    SetFuncAddr(reinterpret_cast<uintptr_t>(textStart));

    CString methodName = method->GetRecordNameStr(thread) + "." + CString(method->GetMethodName(thread));
    LOG_BASELINEJIT(DEBUG) << "BaselineCode :" << methodName << ", "  << " text addr:" <<
        reinterpret_cast<void*>(GetText()) << ", size:" << instrSizeAlign  <<
        ", stackMap addr: 0, size: 0";

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

std::tuple<uint64_t, uint8_t*, int, kungfu::CalleeRegAndOffsetVec> MachineCode::CalCallSiteInfo() const
{
    uintptr_t textStart = GetText();
    uint8_t *stackmapAddr = GetStackMapOrOffsetTableAddress();
    ASSERT(stackmapAddr != nullptr);

    int delta = GetFpDeltaPrevFrameSp();
    kungfu::CalleeRegAndOffsetVec calleeRegInfo;
    for (uint32_t j = 0; j < GetCalleeRegisterNum(); j++) {
        kungfu::LLVMStackMapType::DwarfRegType reg =
            static_cast<kungfu::LLVMStackMapType::DwarfRegType>(GetCalleeReg2OffsetArray(2 * j));
        kungfu::LLVMStackMapType::OffsetType offset =
            static_cast<kungfu::LLVMStackMapType::OffsetType>(GetCalleeReg2OffsetArray(2 * j + 1));
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
        return GetNonTextAddress();
    }
}

uint8_t *MachineCode::GetStackMapOrOffsetTableAddress() const
{
    if (Jit::GetInstance()->IsEnableJitFort()) {
        // stackmap immediately follows MachineCode NonText area
        return reinterpret_cast<uint8_t*>(GetNonTextAddress());
    } else {
        return reinterpret_cast<uint8_t*>(GetText() + GetInstructionsSize());
    }
}

uint8_t *MachineCode::GetHeapConstantTableAddress() const
{
    return reinterpret_cast<uint8_t*>(GetStackMapOrOffsetTableAddress() + GetStackMapOrOffsetTableSize());
}

void MachineCode::ProcessMarkObject()
{
    if (g_isEnableCMCGC) {
        Heap* heap = reinterpret_cast<Heap*>(this->GetLocalHeapAddress());
        // Skip HugeMachinecode or VM that is already destoryed
        // We should implement a proper wait for VM destructor
        if (heap && heap->GetMachineCodeSpace()) {
            heap->GetMachineCodeSpace()->MarkJitFortMemAlive(this);
        }
    } else {
        Region *region = Region::ObjectAddressToRange(this);
        Heap *localHeap = reinterpret_cast<Heap *>(region->GetLocalHeap());
        if (!localHeap) {
            // it is a huge machine code object. skip
            return;
        }
        ASSERT(localHeap->GetMachineCodeSpace());
        if (localHeap->GetMachineCodeSpace()) {
            localHeap->GetMachineCodeSpace()->MarkJitFortMemAlive(this);
        }
    }
}

}  // namespace panda::ecmascript
