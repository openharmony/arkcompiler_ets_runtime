/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_task.h"

#include <limits>
#include <optional>
#include <utility>

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_dfx.h"
#include "ecmascript/jit/jit_thread.h"
#include "ecmascript/js_function.h"
#include "ecmascript/mem/embedded_code_ref_set.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::arksteed {
namespace {

void FillDeoptLiteralTable(JSHandle<MachineCode> &machineCodeObj, const std::vector<JSHandle<JSTaggedValue>> &literals)
{
    if (literals.empty()) {
        return;
    }
    ASSERT(!g_isEnableCMCGC);
    CHECK(literals.size() <= std::numeric_limits<size_t>::max() / sizeof(uint64_t));
    CHECK(literals.size() * sizeof(uint64_t) <= machineCodeObj->GetHeapConstantTableSize());

    auto *literalTable = reinterpret_cast<uint64_t *>(machineCodeObj->GetHeapConstantTableAddress());
    Region *machineCodeRegion = Region::ObjectAddressToRange(machineCodeObj.GetTaggedValue().GetRawHeapObject());
    for (size_t i = 0; i < literals.size(); ++i) {
        JSTaggedValue literal = literals[i].GetTaggedValue();
        CHECK(literal.IsHeapObject() && !literal.IsWeakForHeapObject());
        literalTable[i] = literal.GetRawData();

        Region *literalRegion = Region::ObjectAddressToRange(literal.GetRawHeapObject());
        uintptr_t slotAddress = reinterpret_cast<uintptr_t>(&literalTable[i]);
        if (literalRegion->InYoungSpace()) {
            machineCodeRegion->InsertOldToNewRSet<ReferenceType::NORMAL>(slotAddress);
        } else if (literalRegion->InSharedHeap()) {
            machineCodeRegion->InsertLocalToShareRSet<ReferenceType::NORMAL>(slotAddress);
        }
    }
}

}  // namespace

ArkSteedTask::ArkSteedTask(JSThread *hostThread, JSThread *compilerThread, Jit *jit, JSHandle<JSFunction> &jsFunction,
                           CompilerTier tier, CString &methodName, int32_t offset, JitCompileMode mode)
    : JitTask(hostThread, compilerThread, jit, jsFunction, tier, methodName, offset, mode)
{}

ArkSteedTask::~ArkSteedTask()
{
    auto &codeDesc = GetMachineCodeDesc();
    delete[] reinterpret_cast<uint8_t *>(codeDesc.stackMapOrOffsetTableAddr);
    delete[] reinterpret_cast<uint8_t *>(codeDesc.codeCommentsAddr);
}

void ArkSteedTask::SetDeoptTranslationData(std::vector<uint8_t> data)
{
    deoptTranslationData_ = std::move(data);
    auto &codeDesc = GetMachineCodeDesc();
    codeDesc.arkSteedTranslationAddr =
        deoptTranslationData_.empty() ? 0 : reinterpret_cast<uintptr_t>(deoptTranslationData_.data());
    codeDesc.arkSteedTranslationSize = deoptTranslationData_.size();
}

void ArkSteedTask::SetDeoptLiteralData(std::vector<JSHandle<JSTaggedValue>> literals)
{
    deoptLiteralHandles_ = std::move(literals);
    auto &codeDesc = GetMachineCodeDesc();
    codeDesc.heapConstantTableAddr =
        deoptLiteralHandles_.empty() ? 0 : reinterpret_cast<uintptr_t>(deoptLiteralHandles_.data());
    codeDesc.heapConstantTableSize = deoptLiteralHandles_.size() * sizeof(uint64_t);
}

void ArkSteedTask::SetEmbeddedRefData(const std::vector<EmbeddedCodeRefReloc> &relocations,
                                      const std::vector<JSHandle<JSTaggedValue>> &handles)
{
    embeddedRefRelocations_ = relocations;
    embeddedRefHandles_ = handles;
}

void ArkSteedTask::Compile()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ArkSteed::Compile", "");
    bool res = GetJit()->JitCompile(GetCompilerTask(), this);
    if (!res) {
        SetCompileFailed();
    }

    if (!IsCompileSuccess()) {
        return;
    }

    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ArkSteed::Finalize", "");
    res = GetJit()->JitFinalize(GetCompilerTask(), this);
    if (!res) {
        SetCompileFailed();
    }
}

void ArkSteedTask::InstallCode()
{
    if (!IsCompileSuccess()) {
        return;
    }
    auto hostThread = GetHostThread();
    auto jsFunction = GetJsFunction();
    auto &codeDesc = GetMachineCodeDesc();
    [[maybe_unused]] EcmaHandleScope handleScope(hostThread);
    JSHandle<Method> methodHandle(hostThread, Method::Cast(jsFunction->GetMethod(hostThread).GetTaggedObject()));
    size_t size = ComputePayLoadSize(codeDesc);
    codeDesc.isAsyncCompileMode = IsAsyncTask();

    if (!kungfu::LazyDeoptAllDependencies::Commit(GetDependencies(), hostThread, jsFunction.GetTaggedValue())) {
        return;
    }

    JSHandle<MachineCode> machineCodeObj;
    if (Jit::GetInstance()->IsEnableJitFort()) {
        TaggedObject *machineCode = hostThread->GetEcmaVM()->GetFactory()->NewMachineCodeObject(size, codeDesc);
        if (machineCode == nullptr) {
            LOG_JIT(DEBUG) << "InstallCode skipped. NewMachineCode NULL for size " << size;
            if (hostThread->HasPendingException()) {
                hostThread->SetMachineCodeLowMemory(true);
                hostThread->ClearExceptionAndExtraErrorMessage();
            }
            return;
        }
        machineCodeObj =
            hostThread->GetEcmaVM()->GetFactory()->SetMachineCodeObjectData(machineCode, size, codeDesc, methodHandle);
    } else {
        machineCodeObj = hostThread->GetEcmaVM()->GetFactory()->NewMachineCodeObject(size, codeDesc, methodHandle);
    }
    if (machineCodeObj.GetAddress() == ToUintPtr(nullptr)) {
        return;
    }
#ifndef NDEBUG
    LOG_COMPILER(INFO) << "JIT code installed to address 0x" << std::hex << machineCodeObj->GetFuncAddr() << std::dec;
#endif
    if (hostThread->HasPendingException()) {
        hostThread->SetMachineCodeLowMemory(true);
        hostThread->ClearExceptionAndExtraErrorMessage();
    }

    // Async installation already holds JitGCLockHolder in JSThread::CheckSafepoint.
    // Sync installation needs the same no-move boundary before resolving handles.
    std::optional<Jit::JitGCLockHolder> syncInstallLock;
    if (!IsAsyncTask()) {
        syncInstallLock.emplace(hostThread);
    }

    Heap *heap = hostThread->GetEcmaVM()->GetHeap();
    MachineCode *machineCode = machineCodeObj.GetObject<MachineCode>();
    FillDeoptLiteralTable(machineCodeObj, deoptLiteralHandles_);
    auto abortEmbeddedRefInstall = [&]() {
        heap->GetEmbeddedCodeRefSet()->RemoveOwner(machineCode);
        if (Jit::GetInstance()->IsEnableJitFort()) {
            auto *jitFort = heap->GetOrCreateJitFort();
            ASSERT(jitFort != nullptr);
            jitFort->MarkJitFortMemInstalled(machineCode, codeDesc.isHugeObj);
        }
        jsFunction->SetJitCompilingFlag(false);
    };

    std::vector<EmbeddedCodeRefEntry> embeddedRefEntries;
    embeddedRefEntries.reserve(embeddedRefRelocations_.size());
    for (const auto &relocation : embeddedRefRelocations_) {
        if (relocation.handleIndex >= embeddedRefHandles_.size()) {
            LOG_JIT(ERROR) << "ArkSteed embedded reference has an invalid handle index";
            abortEmbeddedRefInstall();
            return;
        }
        JSTaggedValue target = embeddedRefHandles_[relocation.handleIndex].GetTaggedValue();
        if (!target.IsHeapObject() || target.IsWeakForHeapObject()) {
            LOG_JIT(ERROR) << "ArkSteed embedded reference target is not a strong heap object";
            abortEmbeddedRefInstall();
            return;
        }
        embeddedRefEntries.push_back({relocation.codeOffset, relocation.kind, relocation.width, target.GetRawData()});
    }
    if (!heap->GetEmbeddedCodeRefSet()->InstallOwner(machineCode, std::move(embeddedRefEntries))) {
        LOG_JIT(ERROR) << "ArkSteed failed to install embedded heap references";
        abortEmbeddedRefInstall();
        return;
    }

    JSHandle<ProfileTypeInfoCell> rawProfileTypeInfoCell(hostThread, jsFunction->GetRawProfileTypeInfo(hostThread));
    if (rawProfileTypeInfoCell->IsEmptyProfileTypeInfoCell(hostThread)) {
        JSHandle<JSTaggedValue> undefinedValue(hostThread, JSTaggedValue::Undefined());
        JSFunction::SetProfileTypeInfo(hostThread, jsFunction, undefinedValue);
    }

    InstallCodeByCompilerTier(machineCodeObj, methodHandle);

    uintptr_t codeAddr = machineCodeObj->GetFuncAddr();
    uintptr_t codeAddrEnd = codeAddr + machineCodeObj->GetInstructionsSize();
    __builtin___clear_cache(reinterpret_cast<char *>(codeAddr), reinterpret_cast<char *>(codeAddrEnd));

    if (Jit::GetInstance()->IsEnableJitFort()) {
        auto *jitFort = heap->GetOrCreateJitFort();
        ASSERT(jitFort != nullptr);
        ASSERT(codeDesc.isHugeObj == jitFort->InHugeRange(machineCodeObj->GetText()));
        jitFort->MarkJitFortMemInstalled(machineCodeObj.GetObject<MachineCode>(), codeDesc.isHugeObj);
    }

    jsFunction->SetJitCompilingFlag(false);
}

bool ArkSteedTask::AsyncTask::Run(uint32_t threadIndex)
{
    if (IsTerminate() || !task_->GetHostThread()->GetEcmaVM()->IsInitialized()) {
        return false;
    }
    DISALLOW_HEAP_ACCESS;

    CString info = "compile method (ArkSteed): " + task_->GetMethodName();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
                      ConvertToStdString("ArkSteed::Compile:" + info).c_str(), "");

    JitAsyncTaskRunScope asyncTaskRunScope(task_.get());

    if (task_->GetJsFunction().GetAddress() == 0) {
    } else {
        Jit::TimeScope scope(task_->GetHostThread()->GetEcmaVM(), info, task_->GetCompilerTier());

        task_->Compile();

        // info main thread compile complete
        if (!task_->IsCompileSuccess()) {
            return false;
        }

        if (task_->IsAsyncTask()) {
            task_->GetJit()->RequestInstallCode(task_);
        }

        MachineCodeDesc codeDesc = task_->GetMachineCodeDesc();
        size_t instrSize = codeDesc.codeSizeAlign;
        CString sizeInfo = "text size : ";
        sizeInfo.append(std::to_string(instrSize)).append("bytes");
        scope.appendMessage(sizeInfo);

        int compilerTime = scope.TotalSpentTimeInMicroseconds();
        JitDfx::GetInstance()->RecordSpentTimeAndPrintStatsLogInJitThread(compilerTime, task_->GetMethodName(),
                                                                          task_->GetCompilerTier().IsBaseLine(),
                                                                          task_->GetMainThreadCompilerTime());
    }
    return true;
}

}  // namespace panda::ecmascript::arksteed
