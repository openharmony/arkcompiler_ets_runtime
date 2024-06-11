/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "ecmascript/object_factory.h"
#include "ecmascript/global_env.h"
#include "ecmascript/compiler/aot_file/func_entry_des.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/patch/patch_loader.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/dfx/vmstat/jit_warmup_profiler.h"
#include "ecmascript/ohos/jit_tools.h"
#include "ecmascript/dfx/dump_code/jit_dump_elf.h"
#include "ecmascript/ohos/aot_crash_info.h"

namespace panda::ecmascript {

JitTaskpool *JitTaskpool::GetCurrentTaskpool()
{
    static JitTaskpool *taskpool = new JitTaskpool();
    return taskpool;
}

uint32_t JitTaskpool::TheMostSuitableThreadNum([[maybe_unused]] uint32_t threadNum) const
{
    return 1;
}

JitTask::JitTask(JSThread *hostThread, JSThread *compilerThread, Jit *jit, JSHandle<JSFunction> &jsFunction,
    CompilerTier tier, CString &methodName, int32_t offset, uint32_t taskThreadId,
    JitCompileMode mode, JitDfx *JitDfx)
    : hostThread_(hostThread),
    compilerThread_(compilerThread),
    jit_(jit),
    jsFunction_(jsFunction),
    compilerTask_(nullptr),
    state_(CompileState::SUCCESS),
    compilerTier_(tier),
    methodName_(methodName),
    offset_(offset),
    taskThreadId_(taskThreadId),
    ecmaContext_(nullptr),
    jitCompileMode_(mode),
    jitDfx_(JitDfx),
    runState_(RunState::INIT)
{
    jit->IncJitTaskCnt(hostThread);
    ecmaContext_ = hostThread->GetCurrentEcmaContext();
    sustainingJSHandle_ = std::make_unique<SustainingJSHandle>(hostThread->GetEcmaVM());
}

void JitTask::PrepareCompile()
{
    CloneProfileTypeInfo();
    SustainingJSHandles();
    compilerTask_ = jit_->CreateJitCompilerTask(this);

    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    JSTaggedValue constpool = method->GetConstantPool();
    if (!ConstantPool::CheckUnsharedConstpool(constpool)) {
        hostThread_->GetCurrentEcmaContext()->FindOrCreateUnsharedConstpool(constpool);
    }

    SetRunState(RunState::INIT);
}

void JitTask::Optimize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JIT::Compiler frontend");
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

    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JIT::Compiler backend");
    bool res = jit_->JitFinalize(compilerTask_, this);
    if (!res) {
        SetCompileFailed();
    }
}

void JitTask::InstallOsrCode(JSHandle<Method> &method, JSHandle<MachineCode> &codeObj)
{
    auto profile = jsFunction_->GetProfileTypeInfo();
    if (profile.IsUndefined()) {
        LOG_JIT(DEBUG) << "[OSR] Empty profile for installing code:" << GetMethodName();
        return;
    }
    FuncEntryDes *funcEntryDes = reinterpret_cast<FuncEntryDes*>(codeObj->GetFuncEntryDes());
    method->SetIsFastCall(funcEntryDes->isFastCall_);
    JSHandle<ProfileTypeInfo> profileInfoHandle =
        JSHandle<ProfileTypeInfo>::Cast(JSHandle<JSTaggedValue>(hostThread_, profile));
    uint32_t slotId = profileInfoHandle->GetCacheLength() - 1;  // 1 : get last slot
    auto profileData = profileInfoHandle->Get(slotId);
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
        newArr->Set(hostThread_, i, arr->Get(i));
    }
    newArr->Set(hostThread_, i, codeObj.GetTaggedValue());
    profileInfoHandle->Set(hostThread_, slotId, newArr.GetTaggedValue());
    LOG_JIT(DEBUG) << "[OSR] Install machine code:" << GetMethodName()
                   << ", code address: " << reinterpret_cast<void*>(codeObj->GetFuncAddr())
                   << ", index: " << newArr->GetLength() - 1;
    return;
}

#ifdef ENABLE_JITFORT
static size_t ComputePayLoadSize(MachineCodeDesc &codeDesc)
#else
static size_t ComputePayLoadSize(const MachineCodeDesc &codeDesc)
#endif
{
    if (codeDesc.codeType == MachineCodeType::BASELINE_CODE) {
        // only code section in BaselineCode
#ifdef ENABLE_JITFORT
        return AlignUp(codeDesc.codeSize, MachineCode::TEXT_ALIGN);
#else
        size_t stackMapSizeAlign = AlignUp(codeDesc.stackMapOrOffsetTableSize, MachineCode::DATA_ALIGN);
        size_t codeSizeAlign = AlignUp(codeDesc.codeSize, MachineCode::DATA_ALIGN);
        return stackMapSizeAlign + codeSizeAlign;
#endif
    }

    ASSERT(codeDesc.codeType == MachineCodeType::FAST_JIT_CODE);
    size_t funcEntryDesSizeAlign = AlignUp(codeDesc.funcEntryDesSize, MachineCode::TEXT_ALIGN);
    size_t rodataSizeBeforeTextAlign = AlignUp(codeDesc.rodataSizeBeforeText, MachineCode::TEXT_ALIGN);
    size_t codeSizeAlign = AlignUp(codeDesc.codeSize, MachineCode::DATA_ALIGN);
#ifdef ENABLE_JITFORT
    size_t rodataSizeAfterTextAlign = AlignUp(codeDesc.rodataSizeAfterText, MachineCode::TEXT_ALIGN);
#else
    size_t rodataSizeAfterTextAlign = AlignUp(codeDesc.rodataSizeAfterText, MachineCode::DATA_ALIGN);
#endif

    size_t stackMapSizeAlign = AlignUp(codeDesc.stackMapOrOffsetTableSize, MachineCode::DATA_ALIGN);
#ifdef ENABLE_JITFORT
    if (!codeDesc.rodataSizeAfterText) {
        // ensure proper align because multiple instruction blocks can be installed in JitFort
        codeSizeAlign = AlignUp(codeDesc.codeSize, MachineCode::TEXT_ALIGN);
    }
    // instructionsSize: size of JIT generated native instructions
    // payLoadSize: size of JIT generated output including native code
    size_t instructionsSize = rodataSizeBeforeTextAlign + codeSizeAlign + rodataSizeAfterTextAlign;
    size_t payLoadSize = funcEntryDesSizeAlign + instructionsSize + stackMapSizeAlign;
    size_t allocSize = AlignUp(payLoadSize + MachineCode::SIZE,  static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    LOG_JIT(INFO) << "InstallCode:: MachineCode Object size to allocate: "
        << allocSize << " (instruction size): " << instructionsSize;

    codeDesc.instructionsSize = instructionsSize;
    if (allocSize > MAX_REGULAR_HEAP_OBJECT_SIZE) {
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
        return payLoadSize;
    } else {
        // regular sized machine code object instructions are installed in separate jit fort space
        return payLoadSize - instructionsSize;
    }
#else
    return funcEntryDesSizeAlign + rodataSizeBeforeTextAlign + codeSizeAlign +
           rodataSizeAfterTextAlign + stackMapSizeAlign;
#endif
}

void DumpJitCode(JSHandle<MachineCode> &machineCode, JSHandle<Method> &method)
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
    const char *filename =  method->GetMethodName();
    std::string fileName = std::string(filename);
    uintptr_t addr = machineCode->GetFuncAddr();
    fileName = fileName + "_" + std::to_string(addr) + "+" + std::to_string(len);
    jitDumpElf.AppendSymbolToSymTab(0, 0, len, std::string(filename));
    std::string realOutPath;
    std::string sanboxPath = panda::os::file::File::GetExtendedFilePath(ohos::AotCrashInfo::GetSandBoxPath());
    if (!ecmascript::RealPath(sanboxPath, realOutPath, false)) {
        return;
    }
    std::string outFile = realOutPath + "/" + std::string(fileName);
    int fd = open(outFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0664);
    jitDumpElf.WriteJitElfFile(fd);
    close(fd);
}

void JitTask::InstallCode()
{
    if (!IsCompileSuccess()) {
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(hostThread_);

    JSHandle<Method> methodHandle(hostThread_, Method::Cast(jsFunction_->GetMethod().GetTaggedObject()));

    size_t size = ComputePayLoadSize(codeDesc_);

    JSHandle<Method> newMethodHandle = hostThread_->GetEcmaVM()->GetFactory()->CloneMethodTemporaryForJIT(methodHandle);
    jsFunction_->SetMethod(hostThread_, newMethodHandle);
#ifdef ENABLE_JITFORT
    // skip install if JitFort out of memory
    TaggedObject *machineCode = hostThread_->GetEcmaVM()->GetFactory()->NewMachineCodeObject(size, codeDesc_);
    if (machineCode == nullptr) {
        LOG_JIT(INFO) << "InstallCode skipped. NewMachineCode NULL for size " << size;
        if (hostThread_->HasPendingException()) {
            hostThread_->SetMachineCodeLowMemory(true);
            hostThread_->ClearException();
        }
        return;
    }
    JSHandle<MachineCode> machineCodeObj =
        hostThread_->GetEcmaVM()->GetFactory()->SetMachineCodeObjectData(machineCode, size, codeDesc_, newMethodHandle);
#else
    JSHandle<MachineCode> machineCodeObj =
        hostThread_->GetEcmaVM()->GetFactory()->NewMachineCodeObject(size, codeDesc_, newMethodHandle);
#endif
    machineCodeObj->SetOSROffset(offset_);

    if (hostThread_->HasPendingException()) {
        // check is oom exception
        hostThread_->SetMachineCodeLowMemory(true);
        hostThread_->ClearException();
    }

    if (IsOsrTask()) {
        InstallOsrCode(newMethodHandle, machineCodeObj);
        return;
    }

    InstallCodeByCompilerTier(machineCodeObj, methodHandle, newMethodHandle);
}

void JitTask::InstallCodeByCompilerTier(JSHandle<MachineCode> &machineCodeObj,
    JSHandle<Method> &methodHandle, JSHandle<Method> &newMethodHandle)
{
    uintptr_t codeAddr = machineCodeObj->GetFuncAddr();
    DumpJitCode(machineCodeObj, methodHandle);
    if (compilerTier_ == CompilerTier::FAST) {
        FuncEntryDes *funcEntryDes = reinterpret_cast<FuncEntryDes*>(machineCodeObj->GetFuncEntryDes());
        jsFunction_->SetCompiledFuncEntry(codeAddr, funcEntryDes->isFastCall_);
        newMethodHandle->SetDeoptThreshold(hostThread_->GetEcmaVM()->GetJSOptions().GetDeoptThreshold());
        jsFunction_->SetMachineCode(hostThread_, machineCodeObj);
        jsFunction_->SetJitMachineCodeCache(hostThread_, machineCodeObj);
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
        ASSERT(compilerTier_ == CompilerTier::BASELINE);
        newMethodHandle->SetDeoptThreshold(hostThread_->GetEcmaVM()->GetJSOptions().GetDeoptThreshold());
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
    [[maybe_unused]] EcmaHandleScope handleScope(hostThread_);

    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    uint32_t slotSize = method->GetSlotSize();
    JSTaggedValue profileTypeInfoVal = jsFunction_->GetProfileTypeInfo();
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
            JSTaggedValue value = profileTypeInfo->Get(i);
            newProfileTypeInfo->Set(hostThread_, i, value);
        }
    }
    SetProfileTypeInfo(newProfileTypeInfo);
}

JitTask::~JitTask()
{
    ReleaseSustainingJSHandle();
    jit_->DeleteJitCompile(compilerTask_);
    jit_->DecJitTaskCnt(hostThread_);
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

    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JIT::Compile");
    // JitCompileMode ASYNC
    // check init ok
    jitTask_->SetRunState(RunState::RUNNING);

    JSThread *compilerThread = jitTask_->GetCompilerThread();
    ASSERT(compilerThread->IsJitThread());
    JitThread *jitThread = static_cast<JitThread*>(compilerThread);
    JitVM *jitvm = jitThread->GetJitVM();
    jitvm->SetHostVM(jitTask_->GetHostThread());

    if (jitTask_->GetJsFunction().GetAddress() == 0) {
        // for unit test
    } else {
        CString info = "compile method:" + jitTask_->GetMethodName() + ", in jit thread";
        Jit::TimeScope scope(jitTask_->GetHostThread()->GetEcmaVM(), info, jitTask_->GetCompilerTier());

        jitTask_->Optimize();
        jitTask_->Finalize();

        if (jitTask_->IsAsyncTask()) {
            // info main thread compile complete
            jitTask_->jit_->RequestInstallCode(jitTask_);
        }
        int compilerTime = scope.TotalSpentTimeInMicroseconds();
        jitTask_->jitDfx_->SetTotalTimeOnJitThread(compilerTime);
        if (jitTask_->jitDfx_->ReportBlockUIEvent(jitTask_->mainThreadCompileTime_)) {
            jitTask_->jitDfx_->SetBlockUIEventInfo(
                jitTask_->methodName_,
                jitTask_->compilerTier_ == CompilerTier::BASELINE ? true : false,
                jitTask_->mainThreadCompileTime_, compilerTime);
        }
        jitTask_->jitDfx_->PrintJitStatsLog();
    }
    jitvm->ReSetHostVM();
    jitTask_->SetRunStateFinish();
    return true;
}
}  // namespace panda::ecmascript
