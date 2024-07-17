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

#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/js_function.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/compiler/aot_file/func_entry_des.h"
#include "ecmascript/dfx/vmstat/jit_warmup_profiler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
void (*Jit::initJitCompiler_)(JSRuntimeOptions options) = nullptr;
bool(*Jit::jitCompile_)(void*, JitTask*) = nullptr;
bool(*Jit::jitFinalize_)(void*, JitTask*) = nullptr;
void*(*Jit::createJitCompilerTask_)(JitTask*) = nullptr;
void(*Jit::deleteJitCompile_)(void*) = nullptr;
int (*Jit::jitVerifyAndCopy_)(void*, void*, void*, int) = nullptr;
void *Jit::libHandle_ = nullptr;

Jit *Jit::GetInstance()
{
    static Jit instance_;
    return &instance_;
}

void Jit::SetEnableOrDisable(const JSRuntimeOptions &options, bool isEnableFastJit, bool isEnableBaselineJit)
{
    LockHolder holder(setEnableLock_);

    bool needInitialize = false;
    if (!isEnableFastJit) {
        fastJitEnable_ = false;
    } else {
        needInitialize = true;
    }
    if (!isEnableBaselineJit) {
        baselineJitEnable_ = false;
    } else {
        needInitialize = true;
    }
    if (!needInitialize) {
        return;
    }
    if (!initialized_) {
        Initialize();
    }
    if (initialized_) {
        bool jitEnable = false;
        if (isEnableFastJit && !fastJitEnable_) {
            fastJitEnable_ = true;
            jitEnable = true;
        }
        if (isEnableBaselineJit && !baselineJitEnable_) {
            baselineJitEnable_ = true;
            jitEnable = true;
        }
        if (jitEnable) {
            isApp_ = options.IsEnableAPPJIT();
            hotnessThreshold_ = options.GetJitHotnessThreshold();
            initJitCompiler_(options);
            JitTaskpool::GetCurrentTaskpool()->Initialize();

            jitDfx_ = JitDfx::GetInstance();
            if (options.IsEnableJitDfxDump()) {
                jitDfx_->EnableDump();
            }
            jitDfx_->ResetCompilerTime();
            jitDfx_->ResetBlockUIEventTime();
        }
    }
}

void Jit::Destroy()
{
    if (!initialized_) {
        return;
    }

    LockHolder holder(setEnableLock_);

    JitTaskpool::GetCurrentTaskpool()->Destroy();
    initialized_ = false;
    fastJitEnable_ = false;
    baselineJitEnable_ = false;
    if (libHandle_ != nullptr) {
        CloseLib(libHandle_);
        libHandle_ = nullptr;
    }
}

bool Jit::IsEnableFastJit() const
{
    return fastJitEnable_;
}

bool Jit::IsEnableBaselineJit() const
{
    return baselineJitEnable_;
}

void Jit::Initialize()
{
    static const std::string CREATEJITCOMPILETASK = "CreateJitCompilerTask";
    static const std::string JITCOMPILEINIT = "InitJitCompiler";
    static const std::string JITCOMPILE = "JitCompile";
    static const std::string JITFINALIZE = "JitFinalize";
    static const std::string DELETEJITCOMPILE = "DeleteJitCompile";
    static const std::string JITVERIFYANDCOPY = "JitVerifyAndCopy";
    static const std::string LIBARK_JSOPTIMIZER = "libark_jsoptimizer.so";

    libHandle_ = LoadLib(LIBARK_JSOPTIMIZER);
    if (libHandle_ == nullptr) {
        LOG_JIT(ERROR) << "jit dlopen libark_jsoptimizer.so failed";
        return;
    }

    initJitCompiler_ = reinterpret_cast<void(*)(JSRuntimeOptions)>(FindSymbol(libHandle_, JITCOMPILEINIT.c_str()));
    if (initJitCompiler_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol initJitCompiler";
        return;
    }
    jitCompile_ = reinterpret_cast<bool(*)(void*, JitTask*)>(FindSymbol(libHandle_, JITCOMPILE.c_str()));
    if (jitCompile_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitCompile";
        return;
    }

    jitFinalize_ = reinterpret_cast<bool(*)(void*, JitTask*)>(FindSymbol(libHandle_, JITFINALIZE.c_str()));
    if (jitFinalize_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitFinalize";
        return;
    }

    createJitCompilerTask_ = reinterpret_cast<void*(*)(JitTask*)>(FindSymbol(libHandle_,
        CREATEJITCOMPILETASK.c_str()));
    if (createJitCompilerTask_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol createJitCompilertask";
        return;
    }

    jitVerifyAndCopy_ = reinterpret_cast<int(*)(void*, void*, void*, int)>(
        FindSymbol(libHandle_, JITVERIFYANDCOPY.c_str()));
    if (jitVerifyAndCopy_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol jitVerifyAndCopy";
        return;
    }

    deleteJitCompile_ = reinterpret_cast<void(*)(void*)>(FindSymbol(libHandle_, DELETEJITCOMPILE.c_str()));
    if (deleteJitCompile_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol deleteJitCompile";
        return;
    }
    initialized_= true;
    return;
}

Jit::~Jit()
{
}

bool Jit::MethodHasTryCatch(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral) const
{
    auto pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, methodLiteral->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf, mda.GetCodeId().value());
    return cda.GetTriesSize() != 0;
}

bool Jit::SupportJIT(const Method *method, EcmaVM *vm) const
{
    const JSPandaFile* jSPandaFile_ = method->GetJSPandaFile();
    MethodLiteral* methodLiteral_ = method->GetMethodLiteral();
    if (!vm->GetJSOptions().IsEnableTryCatchFunction() && MethodHasTryCatch(jSPandaFile_, methodLiteral_)) {
        return false;
    }

    FunctionKind kind = method->GetFunctionKind();
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
        case FunctionKind::GETTER_FUNCTION:
        case FunctionKind::SETTER_FUNCTION:
        case FunctionKind::ARROW_FUNCTION:
        case FunctionKind::BASE_CONSTRUCTOR:
        case FunctionKind::CLASS_CONSTRUCTOR:
        case FunctionKind::DERIVED_CONSTRUCTOR:
        case FunctionKind::NONE_FUNCTION:
            return true;
        default:
            return false;
    }
}

void Jit::DeleteJitCompile(void *compiler)
{
    deleteJitCompile_(compiler);
}

void Jit::CountInterpExecFuncs(JSHandle<JSFunction> &jsFunction)
{
    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    CString fileDesc = method->GetJSPandaFile()->GetJSPandaFileDesc();
    CString methodInfo = fileDesc + ":" + method->GetRecordNameStr() + "." + CString(method->GetMethodName());
    auto &profMap = JitWarmupProfiler::GetInstance()->profMap_;
    if (profMap.find(methodInfo) == profMap.end()) {
        profMap.insert({methodInfo, false});
    }
}

// Used for jit machine code reusing of inner functions have the same method to improve performance.
void Jit::ReuseCompiledFunc(JSThread *thread, JSHandle<JSFunction> &jsFunction)
{
    JSHandle<ProfileTypeInfoCell> profCell(thread, jsFunction->GetRawProfileTypeInfo());
    JSTaggedValue machineCode = profCell->GetMachineCode().GetWeakRawValue();
    if (machineCode.IsHole()) {
        return;
    }
    if (machineCode.IsUndefined()) {
        LOG_JIT(DEBUG) << "reset fuction jit hotness count";
        // if old gc triggered, jit hotness cnt need to be recounted
        ProfileTypeInfo::Cast(profCell->GetValue().GetTaggedObject())->SetJitHotnessCnt(0);
        profCell->SetMachineCode(thread, JSTaggedValue::Hole());
        return;
    }
    JSHandle<MachineCode> machineCodeHandle(thread, machineCode.GetTaggedObject());
    JSHandle<Method> method(thread, Method::Cast(jsFunction->GetMethod().GetTaggedObject()));
    LOG_JIT(DEBUG) << "reuse fuction machine code : " << method->GetJSPandaFile()->GetJSPandaFileDesc()
        << ":" << method->GetRecordNameStr() << "." << CString(method->GetMethodName());
    uintptr_t codeAddr = machineCodeHandle->GetFuncAddr();
    FuncEntryDes *funcEntryDes = reinterpret_cast<FuncEntryDes *>(machineCodeHandle->GetFuncEntryDes());
    jsFunction->SetCompiledFuncEntry(codeAddr, funcEntryDes->isFastCall_);
    jsFunction->SetMachineCode(thread, machineCodeHandle);
}

void Jit::Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, CompilerTier tier,
                  int32_t offset, JitCompileMode mode)
{
    auto jit = Jit::GetInstance();
    if ((!jit->IsEnableBaselineJit() && tier == CompilerTier::BASELINE) ||
        (!jit->IsEnableFastJit() && tier == CompilerTier::FAST)) {
        return;
    }

    if (!vm->IsEnableOsr() && offset != MachineCode::INVALID_OSR_OFFSET) {
        return;
    }

    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    CString fileDesc = method->GetJSPandaFile()->GetJSPandaFileDesc();
    CString methodName = fileDesc + ":" + method->GetRecordNameStr() + "." + CString(method->GetMethodName());
    uint32_t codeSize = method->GetCodeSize();
    jit->GetJitDfx()->SetBundleName(vm->GetBundleName());
    jit->GetJitDfx()->SetPidNumber(vm->GetJSThread()->GetThreadId());
    CString methodInfo = methodName + ", bytecode size:" + ToCString(codeSize);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, ConvertToStdString("JIT::Compile:" + methodInfo));

    uint32_t maxSize = 9000;
    if (vm->GetJSOptions().IsEnableJitFastCompile()) {
        maxSize = 15; // 15 is method codesize threshold during fast compiling
    }
    if (codeSize > maxSize) {
        if (tier == CompilerTier::BASELINE) {
            LOG_BASELINEJIT(DEBUG) << "skip jit task, as too large:" << methodInfo;
        } else {
            LOG_JIT(DEBUG) << "skip jit task, as too large:" << methodInfo;
        }

        return;
    }
    if (vm->IsEnableOsr() && offset != MachineCode::INVALID_OSR_OFFSET && method->HasCatchBlock()) {
        LOG_JIT(DEBUG) << "skip jit task, as osr does not support catch blocks: " << methodInfo;
        return;
    }

    CString msg = "compile method:" + methodInfo + ", in work thread";
    TimeScope scope(vm, msg, tier, true, true);
    if (vm->GetJSThread()->IsMachineCodeLowMemory()) {
        if (tier == CompilerTier::BASELINE) {
            LOG_BASELINEJIT(DEBUG) << "skip jit task, as low code memory:" << methodInfo;
        } else {
            LOG_JIT(DEBUG) << "skip jit task, as low code memory:" << methodInfo;
        }

        return;
    }
    bool isJSSharedFunction = jsFunction.GetTaggedValue().IsJSSharedFunction();
    if (!jit->SupportJIT(method, vm) || isJSSharedFunction) {
        FunctionKind kind = method->GetFunctionKind();
        std::stringstream msgStr;
        msgStr << "method does not support jit:" << methodInfo << ", kind:" << static_cast<int>(kind)
               <<", JSSharedFunction:" << isJSSharedFunction;
        if (tier == CompilerTier::BASELINE) {
            LOG_BASELINEJIT(DEBUG) << msgStr.str();
        } else {
            LOG_JIT(DEBUG) << msgStr.str();
        }
        return;
    }
    bool needCompile = jit->CheckJitCompileStatus(jsFunction, methodName, tier);
    if (!needCompile) {
        return;
    }

    // using hole value to indecate compiling. todo: reset when failed
    if (tier == CompilerTier::FAST) {
        jsFunction->SetMachineCode(vm->GetJSThread(), JSTaggedValue::Hole());
        jit->GetJitDfx()->SetTriggerCount(false);
    } else {
        ASSERT(tier == CompilerTier::BASELINE);
        jsFunction->SetBaselineCode(vm->GetJSThread(), JSTaggedValue::Hole());
        jit->GetJitDfx()->SetTriggerCount(true);
    }

    {
        JitTaskpool::GetCurrentTaskpool()->WaitForJitTaskPoolReady();
        EcmaVM *compilerVm = JitTaskpool::GetCurrentTaskpool()->GetCompilerVm();
        std::shared_ptr<JitTask> jitTask = std::make_shared<JitTask>(vm->GetJSThread(), compilerVm->GetJSThread(),
            jit, jsFunction, tier, methodName, offset, vm->GetJSThread()->GetThreadId(), mode, jit->GetJitDfx());

        jitTask->PrepareCompile();
        JitTaskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<JitTask::AsyncTask>(jitTask, vm->GetJSThread()->GetThreadId()));
        if (mode == SYNC) {
            // sync mode, also compile in taskpool as litecg unsupport parallel compile,
            // wait task compile finish then install code
            jitTask->WaitFinish();
            jitTask->InstallCode();
        }
        int spendTime = scope.TotalSpentTimeInMicroseconds();
        jitTask->SetMainThreadCompilerTime(spendTime);
        jit->GetJitDfx()->SetTotalTimeOnMainThread(spendTime);
        jit->GetJitDfx()->PrintJitStatsLog();
    }
}

void Jit::RequestInstallCode(std::shared_ptr<JitTask> jitTask)
{
    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[jitTask->GetTaskThreadId()];
    taskQueue.push_back(jitTask);

    // set
    jitTask->GetHostThread()->SetInstallMachineCode(true);
    jitTask->GetHostThread()->SetCheckSafePointStatus();
}

bool Jit::CheckJitCompileStatus(JSHandle<JSFunction> &jsFunction,
                                const CString &methodName, CompilerTier tier)
{
    if (tier == CompilerTier::FAST &&
        jsFunction->GetMachineCode() == JSTaggedValue::Hole()) {
        LOG_JIT(DEBUG) << "skip method, as it compiling:" << methodName;
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER
        auto &profMap = JitWarmupProfiler::GetInstance()->profMap_;
        if (profMap.find(methodName) != profMap.end()) {
            profMap.erase(methodName);
        }
#endif
        return false;
    }

    if (tier == CompilerTier::BASELINE &&
        jsFunction->GetBaselineCode() == JSTaggedValue::Hole()) {
        LOG_BASELINEJIT(DEBUG) << "skip method, as it compiling:" << methodName;
        return false;
    }

    if (tier == CompilerTier::FAST && jsFunction->IsCompiledCode()) {
        JSTaggedValue machineCode = jsFunction->GetMachineCode();
        if (machineCode.IsMachineCodeObject() &&
            MachineCode::Cast(machineCode.GetTaggedObject())->GetOSROffset() == MachineCode::INVALID_OSR_OFFSET) {
            LOG_JIT(DEBUG) << "skip method, as it has been jit compiled:" << methodName;
            return false;
        }
        return true;
    }

    if (tier == CompilerTier::BASELINE &&
        jsFunction->GetBaselineCode() != JSTaggedValue::Undefined()) {
        LOG_BASELINEJIT(DEBUG) << "skip method, as it has been jit compiled:" << methodName;
        return false;
    }
    return true;
}

void Jit::InstallTasks(uint32_t threadId)
{
    LockHolder holder(installJitTasksDequeMtx_);
    auto &taskQueue = installJitTasks_[threadId];
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, ConvertToStdString("Jit::InstallTasks count:" + ToCString(taskQueue.size())));

    for (auto it = taskQueue.begin(); it != taskQueue.end(); it++) {
        std::shared_ptr<JitTask> task = *it;
        // check task state
        task->InstallCode();
    }
    taskQueue.clear();
}

bool Jit::JitCompile(void *compiler, JitTask *jitTask)
{
    ASSERT(jitCompile_ != nullptr);
    return jitCompile_(compiler, jitTask);
}

bool Jit::JitFinalize(void *compiler, JitTask *jitTask)
{
    ASSERT(jitFinalize_ != nullptr);
    return jitFinalize_(compiler, jitTask);
}

void *Jit::CreateJitCompilerTask(JitTask *jitTask)
{
    ASSERT(createJitCompilerTask_ != nullptr);
    return createJitCompilerTask_(jitTask);
}

int Jit::JitVerifyAndCopy(void *codeSigner, void *jit_memory, void *tmpBuffer, int size)
{
    ASSERT(jitVerifyAndCopy_ != nullptr);
    LOG_JIT(DEBUG) << "In Jit::JitVerifyAndCopy "
        << std::hex << (uintptr_t)codeSigner << " "
        << std::hex << (uintptr_t)jit_memory << " "
        << std::hex << (uintptr_t)tmpBuffer << " " << std::dec << size;
    return jitVerifyAndCopy_(codeSigner, jit_memory, tmpBuffer, size);
}

void Jit::ClearTask(const std::function<bool(Task *task)> &checkClear)
{
    JitTaskpool::GetCurrentTaskpool()->ForEachTask([&checkClear](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        if (checkClear(asyncTask)) {
            asyncTask->Terminated();
            if (asyncTask->IsRunning()) {
                asyncTask->WaitFinish();
            }
            asyncTask->ReleaseSustainingJSHandle();
        }
    });
}

void Jit::ClearTask(EcmaContext *ecmaContext)
{
    ClearTask([ecmaContext](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        return ecmaContext == asyncTask->GetEcmaContext();
    });
}

void Jit::ClearTaskWithVm(EcmaVM *vm)
{
    ClearTask([vm](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        return vm == asyncTask->GetHostVM();
    });

    {
        LockHolder holder(installJitTasksDequeMtx_);
        auto &taskQueue = installJitTasks_[vm->GetJSThread()->GetThreadId()];
        taskQueue.clear();
    }
    {
        LockHolder holder(jitTaskCntMtx_);
        auto &cnt = jitTaskCnt_[vm->GetJSThread()->GetThreadId()];
        if (cnt.first.load() != 0) {
            cnt.second.Wait(&jitTaskCntMtx_);
        }
    }
}

void Jit::IncJitTaskCnt(JSThread *thread)
{
    LockHolder holder(jitTaskCntMtx_);
    auto &cnt = jitTaskCnt_[thread->GetThreadId()];
    cnt.first.fetch_add(1);
}

void Jit::DecJitTaskCnt(JSThread *thread)
{
    LockHolder holder(jitTaskCntMtx_);
    auto &cnt = jitTaskCnt_[thread->GetThreadId()];
    uint32_t old = cnt.first.fetch_sub(1);
    if (old == 1) {
        cnt.second.Signal();
    }
}

void Jit::CheckMechineCodeSpaceMemory(JSThread *thread, int remainSize)
{
    if (!thread->IsMachineCodeLowMemory()) {
        return;
    }
    if (remainSize > MIN_CODE_SPACE_SIZE) {
        thread->SetMachineCodeLowMemory(false);
    }
}

void Jit::ChangeTaskPoolState(bool inBackground)
{
    if (fastJitEnable_ || baselineJitEnable_) {
        if (inBackground) {
            JitTaskpool::GetCurrentTaskpool()->SetThreadPriority(PriorityMode::BACKGROUND);
        } else {
            JitTaskpool::GetCurrentTaskpool()->SetThreadPriority(PriorityMode::FOREGROUND);
        }
    }
}

Jit::TimeScope::~TimeScope()
{
    if (!outPutLog_) {
        return;
    }
    if (isDebugLevel_) {
        if (tier_ == CompilerTier::BASELINE) {
            LOG_BASELINEJIT(DEBUG) << message_ << ": " << TotalSpentTime() << "ms";
            return;
        }
        ASSERT(tier_ == CompilerTier::FAST);
        LOG_JIT(DEBUG) << message_ << ": " << TotalSpentTime() << "ms";
    } else {
        if (tier_ == CompilerTier::BASELINE) {
            LOG_BASELINEJIT(INFO) << message_ << ": " << TotalSpentTime() << "ms";
            return;
        }
        ASSERT(tier_ == CompilerTier::FAST);
        auto bundleName = vm_->GetBundleName();
        if (vm_->GetEnableJitLogSkip() && bundleName != "" && message_.find(bundleName) == std::string::npos) {
            return;
        }
        LOG_JIT(INFO) << message_ << ": " << TotalSpentTime() << "ms";
    }
}
}  // namespace panda::ecmascript
