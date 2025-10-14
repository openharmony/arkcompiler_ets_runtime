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

#include "ecmascript/jit/jit_task.h"
#include "ecmascript/base/config.h"
#include "common_components/heap/heap_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ohos/jit_tools.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {

JitTaskpool *JitTaskpool::GetCurrentTaskpool()
{
    static JitTaskpool *taskpool = new JitTaskpool();
    return taskpool;
}

uint32_t JitTaskpool::TheMostSuitableThreadNum([[maybe_unused]]uint32_t threadNum) const
{
    return 1;
}

JitTask::JitTask(JSThread *hostThread, JSThread *compilerThread, Jit *jit, JSHandle<JSFunction> &jsFunction,
    CompilerTier tier, CString &methodName, int32_t offset, JitCompileMode mode)
    : hostThread_(hostThread),
    compilerThread_(compilerThread),
    jit_(jit),
    jsFunction_(jsFunction),
    compilerTask_(nullptr),
    state_(CompileState::SUCCESS),
    compilerTier_(tier),
    methodName_(methodName),
    offset_(offset),
    jitCompileMode_(mode),
    runState_(RunState::INIT)
{
    jit->IncJitTaskCnt(hostThread);
    dependencies_ = new kungfu::LazyDeoptAllDependencies();
    sustainingJSHandle_ = std::make_unique<SustainingJSHandle>(hostThread->GetEcmaVM());
}

void JitTask::PrepareCompile()
{
    CloneProfileTypeInfo();
    SustainingJSHandles();
    compilerTask_ = jit_->CreateJitCompilerTask(this);

    Method *method = Method::Cast(jsFunction_->GetMethod(hostThread_).GetTaggedObject());
    JSTaggedValue constpool = method->GetConstantPool(hostThread_);
    if (!ConstantPool::CheckUnsharedConstpool(constpool)) {
        hostThread_->GetEcmaVM()->FindOrCreateUnsharedConstpool(constpool);
    }

    SetRunState(RunState::INIT);
}

void JitTask::Optimize()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JIT::Compiler frontend", "");
    bool res = jit_->JitCompile(compilerTask_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::Finalize()
{
    if (!IsCompileSuccess()) {
        return;
    }

    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JIT::Compiler backend", "");
    bool res = jit_->JitFinalize(compilerTask_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::InstallOsrCode(JSHandle<MachineCode> &codeObj)
{
    auto profile = jsFunction_->GetProfileTypeInfo(hostThread_);
    if (profile.IsUndefined()) {
        LOG_JIT(DEBUG) << "[OSR] Empty profile for installing code:" << GetMethodName();
        return;
    }
    jsFunction_->SetIsCompiledFastCall(codeObj->GetIsFastCall());
    JSHandle<ProfileTypeInfo> profileInfoHandle =
        JSHandle<ProfileTypeInfo>::Cast(JSHandle<JSTaggedValue>(hostThread_, profile));
    uint32_t slotId = profileInfoHandle->GetIcSlotToOsrLength() - 1; // 1 : get last slot
    auto profileData = profileInfoHandle->Get(hostThread_, slotId);
    auto factory = hostThread_->GetEcmaVM()->GetFactory();
    if (!profileData.IsTaggedArray()) {
        const uint32_t initLen = 1;
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(initLen);
        newArr->Set(hostThread_, 0, codeObj.GetTaggedValue());
        profileInfoHandle->Set(hostThread_, slotId, newArr.GetTaggedValue());
        LOG_JIT(DEBUG) << "[OSR] Install machine code:" << GetMethodName()
                       << ", code address: " << reinterpret_cast<void*>(codeObj->GetFuncAddr())
                       << ", index: " << newArr->GetLength() - 1;
        return;
    }
    JSHandle<TaggedArray> arr(hostThread_, profileData);
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(arr->GetLength() + 1);  // 1 : added for current codeObj
    uint32_t i = 0;
    for (; i < arr->GetLength(); i++) {
        newArr->Set(hostThread_, i, arr->Get(hostThread_, i));
    }
    newArr->Set(hostThread_, i, codeObj.GetTaggedValue());
    profileInfoHandle->Set(hostThread_, slotId, newArr.GetTaggedValue());
    LOG_JIT(DEBUG) << "[OSR] Install machine code:" << GetMethodName()
                   << ", code address: " << reinterpret_cast<void*>(codeObj->GetFuncAddr())
                   << ", index: " << newArr->GetLength() - 1;
    return;
}

static void ComputeAlignedSizes(MachineCodeDesc &desc)
{
    desc.funcEntryDesSizeAlign = AlignUp(desc.funcEntryDesSize, MachineCode::TEXT_ALIGN);
    desc.stackMapSizeAlign = AlignUp(desc.stackMapOrOffsetTableSize, MachineCode::DATA_ALIGN);
    desc.heapConstantTableSizeAlign = AlignUp(desc.heapConstantTableSize, MachineCode::DATA_ALIGN);
    desc.rodataSizeBeforeTextAlign = AlignUp(desc.rodataSizeBeforeText, MachineCode::TEXT_ALIGN);

    if (desc.codeType == MachineCodeType::BASELINE_CODE) {
        desc.codeSizeAlign = Jit::GetInstance()->IsEnableJitFort() ?
            AlignUp(desc.codeSize, MachineCode::TEXT_ALIGN) :
            AlignUp(desc.codeSize, MachineCode::DATA_ALIGN);
        return;
    }

    // FastJit
    if (Jit::GetInstance()->IsEnableJitFort()) {
        // align for multiple instruction blocks installed in JitFort
        if (desc.rodataSizeAfterText) {
            desc.codeSizeAlign = AlignUp(desc.codeSize, MachineCode::DATA_ALIGN);
            desc.rodataSizeAfterTextAlign = AlignUp(desc.rodataSizeAfterText, MachineCode::TEXT_ALIGN);
        } else {
            desc.codeSizeAlign = AlignUp(desc.codeSize, MachineCode::TEXT_ALIGN);
        }
    } else {
        desc.codeSizeAlign = AlignUp(desc.codeSize, MachineCode::DATA_ALIGN);
        desc.rodataSizeAfterTextAlign = AlignUp(desc.rodataSizeAfterText, MachineCode::DATA_ALIGN);
    }
}

size_t JitTask::ComputePayLoadSize(MachineCodeDesc &codeDesc)
{
    ComputeAlignedSizes(codeDesc);
    if (codeDesc.codeType == MachineCodeType::BASELINE_CODE) {
        // only code section in BaselineCode
        if (Jit::GetInstance()->IsEnableJitFort()) {
            size_t payLoadSize = codeDesc.stackMapSizeAlign + codeDesc.codeSizeAlign;
            size_t allocSize = AlignUp(payLoadSize + MachineCode::SIZE,
                static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            codeDesc.instructionsSize = codeDesc.codeSizeAlign;
            LOG_JIT(DEBUG) << "InstallCode:: MachineCode Object size to allocate: "
                << allocSize << " (instruction size): " << codeDesc.codeSizeAlign;
            if (allocSize > g_maxRegularHeapObjectSize) {
                return payLoadSize;
            } else {
                // regular sized machine code object instructions are installed in separate jit fort space
                return payLoadSize - codeDesc.codeSizeAlign;
            }
        } else {
            return codeDesc.stackMapSizeAlign + codeDesc.codeSizeAlign;
        }
    }

    ASSERT(codeDesc.codeType == MachineCodeType::FAST_JIT_CODE);
    if (Jit::GetInstance()->IsEnableJitFort()) {
        // instructionsSize: size of JIT generated native instructions
        // payLoadSize: size of JIT generated output including native code
        size_t instructionsSize =
            codeDesc.rodataSizeBeforeTextAlign + codeDesc.codeSizeAlign + codeDesc.rodataSizeAfterTextAlign;
        size_t payLoadSize = codeDesc.funcEntryDesSizeAlign + instructionsSize +
                             codeDesc.stackMapSizeAlign + codeDesc.heapConstantTableSizeAlign;
        size_t allocSize = AlignUp(payLoadSize + MachineCode::SIZE,
            static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        LOG_JIT(DEBUG) << "InstallCode:: MachineCode Object size to allocate: "
            << allocSize << " (instruction size): " << instructionsSize;

        codeDesc.instructionsSize = instructionsSize;
        if (allocSize > g_maxRegularHeapObjectSize) {
            //
            // A Huge machine code object is consisted of contiguous 256Kb aligned blocks.
            // With JitFort, a huge machine code object starts with a page aligned mutable area
            // (that holds Region and MachineCode object header, FuncEntryDesc and StackMap), followed
            // by a page aligned nonmutable (JitFort space) area of JIT generated native instructions.
            // i.e.
            // mutableSize = align up to PageSize
            //     (sizeof(Region) + HUGE_OBJECT_BITSET_SIZE +MachineCode::SIZE + payLoadSize - instructionsSize)
            // immutableSize = instructionsSize (native page boundary aligned)
            // See comments at HugeMachineCodeSpace::Allocate()
            //
            codeDesc.isHugeObj = true;
            return payLoadSize;
        } else {
            // regular sized machine code object instructions are installed in separate jit fort space
            return payLoadSize - instructionsSize;
        }
    } else {
        return codeDesc.funcEntryDesSizeAlign + codeDesc.rodataSizeBeforeTextAlign + codeDesc.codeSizeAlign +
               codeDesc.rodataSizeAfterTextAlign + codeDesc.stackMapSizeAlign + codeDesc.heapConstantTableSizeAlign;
    }
}

void DumpJitCode(const JSThread *thread, JSHandle<MachineCode> &machineCode, JSHandle<Method> &method)
{
    if (!ohos::JitTools::GetJitDumpObjEanble()) {
        return;
    }
    JsJitDumpElf jitDumpElf;
    jitDumpElf.Init();
    char *funcAddr = reinterpret_cast<char *>(machineCode->GetFuncAddr());
    size_t len = machineCode->GetTextSize();
    std::vector<uint8> vec(len);
    if (memmove_s(vec.data(), len, funcAddr, len) != EOK) {
        LOG_JIT(DEBUG) << "Fail to get machineCode on function addr: " << funcAddr;
    }
    jitDumpElf.AppendData(vec);
    const char *filename =  method->GetMethodName(thread);
    std::string fileName = std::string(filename);
    uintptr_t addr = machineCode->GetFuncAddr();
    fileName = fileName + "_" + std::to_string(addr) + "+" + std::to_string(len);
    jitDumpElf.AppendSymbolToSymTab(0, 0, len, std::string(filename));
    std::string realOutPath;
    std::string sanboxPath = panda::os::file::File::GetExtendedFilePath(AotCrashInfo::GetSandBoxPath());
    if (!ecmascript::RealPath(sanboxPath, realOutPath, false)) {
        return;
    }
    std::string outFile = realOutPath + "/" + std::string(fileName);
    if (!ecmascript::FileExist(outFile.c_str())) {
        return;
    }
    int fd = open(outFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
    jitDumpElf.WriteJitElfFile(fd);
    Close(reinterpret_cast<fd_t>(fd));
}

static void FillHeapConstantTable(JSHandle<MachineCode> &machineCodeObj, const MachineCodeDesc &codeDesc)
{
    if (codeDesc.heapConstantTableAddr == 0) {
        return;
    }
    ASSERT(!g_isEnableCMCGC);
    uint64_t *heapConstantTableAddr = reinterpret_cast<uint64_t*>(machineCodeObj->GetHeapConstantTableAddress());
    JSHandle<JSTaggedValue> *heapConstantTableInCodeDesc =
        reinterpret_cast<JSHandle<JSTaggedValue>*>(codeDesc.heapConstantTableAddr);

    uint64_t constTableSlotNum = codeDesc.heapConstantTableSize / sizeof(uint64_t);
    LOG_JIT(DEBUG) << "constant table size: " << constTableSlotNum << "\n";
    for (uint64_t i = 0; i < constTableSlotNum; ++i) {
        JSHandle<JSTaggedValue> heapObj = heapConstantTableInCodeDesc[i];
        heapConstantTableAddr[i] = heapObj->GetRawData();
        Region *heapObjRegion = Region::ObjectAddressToRange(heapObj->GetRawData());
        Region *curMachineCodeObjRegion =
            Region::ObjectAddressToRange(machineCodeObj.GetTaggedValue().GetRawHeapObject());
        if (heapObjRegion->InYoungSpace()) {
            curMachineCodeObjRegion->InsertOldToNewRSet(reinterpret_cast<uintptr_t>(&(heapConstantTableAddr[i])));
        } else if (heapObjRegion->InSharedHeap()) {
            curMachineCodeObjRegion->InsertLocalToShareRSet(reinterpret_cast<uintptr_t>(&(heapConstantTableAddr[i])));
        }
    }
}

// This should only be entered from hostVM, i.e., execution jsthread
void JitTask::InstallCode()
{
    if (!IsCompileSuccess()) {
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(hostThread_);
    JSHandle<Method> methodHandle(hostThread_, Method::Cast(jsFunction_->GetMethod(hostThread_).GetTaggedObject()));
    size_t size = ComputePayLoadSize(codeDesc_);
    codeDesc_.isAsyncCompileMode = IsAsyncTask();

    if (!kungfu::LazyDeoptAllDependencies::Commit(
        GetDependencies(), hostThread_, jsFunction_.GetTaggedValue())) {
        return;
    }

    JSHandle<MachineCode> machineCodeObj;
    if (Jit::GetInstance()->IsEnableJitFort()) {
        // skip install if JitFort out of memory
        TaggedObject *machineCode = hostThread_->GetEcmaVM()->GetFactory()->NewMachineCodeObject(size, codeDesc_);
        if (machineCode == nullptr) {
            LOG_JIT(DEBUG) << "InstallCode skipped. NewMachineCode NULL for size " << size;
            if (hostThread_->HasPendingException()) {
                hostThread_->SetMachineCodeLowMemory(true);
                hostThread_->ClearException();
            }
            return;
        }
        machineCodeObj = hostThread_->GetEcmaVM()->GetFactory()->SetMachineCodeObjectData(
            machineCode, size, codeDesc_, methodHandle);
    } else {
        machineCodeObj = hostThread_->GetEcmaVM()->GetFactory()->NewMachineCodeObject(
            size, codeDesc_, methodHandle);
    }
    if (machineCodeObj.GetAddress() == ToUintPtr(nullptr)) {
        // skip install
        return;
    }
    machineCodeObj->SetOSROffset(offset_);
    FillHeapConstantTable(machineCodeObj, codeDesc_);

    if (hostThread_->HasPendingException()) {
        // check is oom exception
        hostThread_->SetMachineCodeLowMemory(true);
        hostThread_->ClearException();
    }

    if (IsOsrTask()) {
        InstallOsrCode(machineCodeObj);
    } else {
        InstallCodeByCompilerTier(machineCodeObj, methodHandle);
    }

    // sometimes get ILL_ILLOPC error if i-cache  not flushed for Jit code
    uintptr_t codeAddr = machineCodeObj->GetFuncAddr();
    uintptr_t codeAddrEnd = codeAddr + machineCodeObj->GetInstructionsSize();
    __builtin___clear_cache(reinterpret_cast<char *>(codeAddr), reinterpret_cast<char*>(codeAddrEnd));

    if (Jit::GetInstance()->IsEnableJitFort()) {
        if (g_isEnableCMCGC) {
            common::BaseRuntime::GetInstance()->GetHeapManager().MarkJitFortMemInstalled(
                codeDesc_.isHugeObj ? nullptr : hostThread_, machineCodeObj.GetObject<MachineCode>());
        } else {
            if (!codeDesc_.isHugeObj) {
                const Heap *heap = this->GetHostThread()->GetEcmaVM()->GetHeap();
                heap->GetMachineCodeSpace()->MarkJitFortMemInstalled(machineCodeObj.GetObject<MachineCode>());
            }
        }
    }

    if (compilerTier_.IsFast()) {
        jsFunction_->SetJitCompilingFlag(false);
    } else {
        ASSERT(compilerTier_.IsBaseLine());
        jsFunction_->SetBaselinejitCompilingFlag(false);
    }
}

void JitTask::InstallCodeByCompilerTier(JSHandle<MachineCode> &machineCodeObj,
    JSHandle<Method> &methodHandle)
{
    uintptr_t codeAddr = machineCodeObj->GetFuncAddr();
    if (compilerTier_.IsFast()) {
        jsFunction_->SetCompiledFuncEntry(codeAddr, machineCodeObj->GetIsFastCall());
        methodHandle->SetDeoptThreshold(hostThread_->GetEcmaVM()->GetJSOptions().GetDeoptThreshold());
        jsFunction_->SetMachineCode(hostThread_, machineCodeObj);
        if (!jit_->IsAppJit()) {
            jsFunction_->SetJitMachineCodeCache(hostThread_, machineCodeObj);
        }
        uintptr_t codeAddrEnd = codeAddr + machineCodeObj->GetInstructionsSize();
        LOG_JIT(DEBUG) <<"Install fast jit machine code:" << GetMethodName() << ", code range:" <<
            reinterpret_cast<void*>(codeAddr) <<"--" << reinterpret_cast<void*>(codeAddrEnd);
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER
        auto &profMap = JitWarmupProfiler::GetInstance()->profMap_;
        if (profMap.find(GetMethodName()) != profMap.end()) {
            profMap[GetMethodName()] = true;
        }
#endif
    } else {
        ASSERT(compilerTier_.IsBaseLine());
        methodHandle->SetDeoptThreshold(hostThread_->GetEcmaVM()->GetJSOptions().GetDeoptThreshold());
        jsFunction_->SetBaselineCode(hostThread_, machineCodeObj);
        LOG_BASELINEJIT(DEBUG) <<"Install baseline jit machine code:" << GetMethodName();
    }
}

void JitTask::SustainingJSHandles()
{
    // transfer to sustaining handle
    JSHandle<JSFunction> sustainingJsFunctionHandle = sustainingJSHandle_->NewHandle(jsFunction_);
    SetJsFunction(sustainingJsFunctionHandle);

    JSHandle<ProfileTypeInfo> profileTypeInfo = sustainingJSHandle_->NewHandle(profileTypeInfo_);
    SetProfileTypeInfo(profileTypeInfo);
}

void JitTask::ReleaseSustainingJSHandle()
{
    // in abort case, vm exit before task finish, release by explict
    sustainingJSHandle_ = nullptr;
}

void JitTask::CloneProfileTypeInfo()
{
    Method *method = Method::Cast(jsFunction_->GetMethod(hostThread_).GetTaggedObject());
    uint32_t slotSize = method->GetSlotSize();
    JSTaggedValue profileTypeInfoVal = jsFunction_->GetProfileTypeInfo(hostThread_);
    JSHandle<ProfileTypeInfo> newProfileTypeInfo;
    ObjectFactory *factory = hostThread_->GetEcmaVM()->GetFactory();
    if (profileTypeInfoVal.IsUndefined() || slotSize == 0) {
        slotSize = slotSize == 0 ? 1 : slotSize; // there's no profiletypeinfo, just generate a temp profiletypeinfo
        newProfileTypeInfo = factory->NewProfileTypeInfo(slotSize);
    } else {
        JSHandle<ProfileTypeInfo> profileTypeInfo(hostThread_,
            ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject()));
        newProfileTypeInfo = factory->NewProfileTypeInfo(slotSize);
        for (uint32_t i = 0; i < slotSize; i++) {
            JSTaggedValue value = profileTypeInfo->Get(hostThread_, i);
            newProfileTypeInfo->Set(hostThread_, i, value);
        }
    }
    SetProfileTypeInfo(newProfileTypeInfo);
}

JitTask::~JitTask()
{
    ReleaseSustainingJSHandle();
    jit_->DeleteJitCompilerTask(compilerTask_);
    jit_->DecJitTaskCnt(hostThread_);
    ASSERT(dependencies_ != nullptr);
    delete dependencies_;
    dependencies_ = nullptr;
}

void JitTask::WaitFinish()
{
    LockHolder lock(runStateMutex_);
    if (!IsFinish()) {
        runStateCondition_.Wait(&runStateMutex_);
    }
}

bool JitTask::AsyncTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    if (IsTerminate() || !jitTask_->GetHostThread()->GetEcmaVM()->IsInitialized()) {
        return false;
    }
    DISALLOW_HEAP_ACCESS;

    CString info = "compile method:" + jitTask_->GetMethodName();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ConvertToStdString("JIT::Compile:" + info).c_str(), "");

    AsyncTaskRunScope asyncTaskRunScope(jitTask_.get());

    if (jitTask_->GetJsFunction().GetAddress() == 0) {
        // for unit test
    } else {
        info = info + ", in jit thread";
        Jit::TimeScope scope(jitTask_->GetHostThread()->GetEcmaVM(), info, jitTask_->GetCompilerTier());

        jitTask_->Optimize();
        jitTask_->Finalize();

        // info main thread compile complete
        if (!jitTask_->IsCompileSuccess()) {
            return false;
        }

        if (jitTask_->IsAsyncTask()) {
            jitTask_->jit_->RequestInstallCode(jitTask_);
        }

        MachineCodeDesc codeDesc = jitTask_->GetMachineCodeDesc();
        size_t instrSize = codeDesc.codeSizeAlign + codeDesc.rodataSizeBeforeTextAlign
                           + codeDesc.rodataSizeAfterTextAlign;
        CString sizeInfo = ": text size: ";
        sizeInfo.append(std::to_string(instrSize)).append("bytes");
        scope.appendMessage(sizeInfo);

        int compilerTime = scope.TotalSpentTimeInMicroseconds();
        JitDfx::GetInstance()->RecordSpentTimeAndPrintStatsLogInJitThread(compilerTime, jitTask_->methodName_,
            jitTask_->compilerTier_.IsBaseLine(), jitTask_->mainThreadCompileTime_);
    }
    return true;
}

JitTask::AsyncTask::AsyncTaskRunScope::AsyncTaskRunScope(JitTask *jitTask)
{
    jitTask_ = jitTask;
    jitTask_->SetRunState(RunState::RUNNING);
    JSThread *compilerThread = jitTask_->GetCompilerThread();
    ASSERT(compilerThread->IsJitThread());
    JitThread *jitThread = static_cast<JitThread*>(compilerThread);
    jitvm_ = jitThread->GetJitVM();
    jitvm_->SetHostVM(jitTask_->GetHostThread());
    jitThread->SetCurrentTask(jitTask);
}

JitTask::AsyncTask::AsyncTaskRunScope::~AsyncTaskRunScope()
{
    JSThread *compilerThread = jitTask_->GetCompilerThread();
    JitThread *jitThread = static_cast<JitThread*>(compilerThread);
    jitThread->SetCurrentTask(nullptr);
    jitvm_->ReSetHostVM();
    jitTask_->SetRunStateFinish();
}
}  // namespace panda::ecmascript
