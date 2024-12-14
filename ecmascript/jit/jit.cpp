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

#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/dfx/vmstat/jit_warmup_profiler.h"
#include "ecmascript/ohos/jit_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

namespace panda::ecmascript {

Jit *Jit::GetInstance()
{
    static Jit instance_;
    return &instance_;
}

void Jit::CreateJitResources()
{
    if (jitResources_ == nullptr) {
        jitResources_ = std::make_unique<JitResources>();
        jitResources_->ResolveLib();
    }
}

bool Jit::IsLibResourcesResolved() const
{
    if (jitResources_ != nullptr) {
        return jitResources_->IsLibResolved();
    }
    return false;
}

void Jit::PreFork()
{
    CreateJitResources();
}

void Jit::SetJitEnablePostFork(EcmaVM *vm, const std::string &bundleName)
{
    JSRuntimeOptions &options = vm->GetJSOptions();
    bool jitEnable = ohos::JitTools::GetJitEscapeDisable() || !AotCrashInfo::IsJitEscape();
    jitEnable &= ohos::EnableAotJitListHelper::GetInstance()->IsEnableJit(bundleName);
    jitEnable &= !vm->GetJSOptions().GetAOTHasException();
    jitEnable &= ohos::JitTools::IsSupportJitCodeSigner();
    if (jitEnable) {
        bool isEnableFastJit = options.IsEnableJIT() && options.GetEnableAsmInterpreter();
        bool isEnableBaselineJit = options.IsEnableBaselineJIT() && options.GetEnableAsmInterpreter();

        options.SetEnableJitFrame(ohos::JitTools::GetJitFrameEnable());
        options.SetEnableAPPJIT(true);
        isApp_ = true;
        // for app threshold
        uint32_t defaultSize = 3000;
        uint32_t threshold = ohos::JitTools::GetJitHotnessThreshold(defaultSize);
        options.SetJitHotnessThreshold(threshold);
        hotnessThreshold_ = threshold;
        bundleName_ = bundleName;
        isEnableAppPGO_ = pgo::PGOProfilerManager::GetInstance()->IsEnable();

        SetEnableOrDisable(options, isEnableFastJit, isEnableBaselineJit);
        if (fastJitEnable_ || baselineJitEnable_) {
            ConfigJit(vm);
        }
    }
}

void Jit::SwitchProfileStubs(EcmaVM *vm)
{
    JSThread *thread = vm->GetAssociatedJSThread();
    JSRuntimeOptions &options = vm->GetJSOptions();
    std::shared_ptr<PGOProfiler> pgoProfiler = vm->GetPGOProfiler();
    if (!options.IsEnableJITPGO() || pgoProfiler == nullptr || (isApp_ && !isEnableAppPGO_)) {
        thread->SwitchJitProfileStubs(false);
        options.SetEnableJITPGO(false);
    } else {
        // if not enable aot pgo
        if (!pgo::PGOProfilerManager::GetInstance()->IsEnable()) {
            // disable dump
            options.SetEnableProfileDump(false);
            SetProfileNeedDump(false);
            // enable profiler
            options.SetEnablePGOProfiler(true);
            pgoProfiler->Reset(true);
            // switch pgo stub
            thread->SwitchJitProfileStubs(true);
        }
        pgoProfiler->InitJITProfiler();
    }
}

void Jit::ConfigOptions(EcmaVM *vm) const
{
    JSRuntimeOptions &options = vm->GetJSOptions();

    options.SetEnableAPPJIT(isApp_);
    options.SetEnableProfileDump(isProfileNeedDump_);

    bool jitEnableLitecg = ohos::JitTools::IsJitEnableLitecg(options.IsCompilerEnableLiteCG());
    options.SetCompilerEnableLiteCG(jitEnableLitecg);

    uint8_t jitCallThreshold = ohos::JitTools::GetJitCallThreshold(options.GetJitCallThreshold());
    options.SetJitCallThreshold(jitCallThreshold);

    uint32_t jitHotnessThreshold = GetHotnessThreshold();
    options.SetJitHotnessThreshold(jitHotnessThreshold);

    bool jitDisableCodeSign = ohos::JitTools::GetCodeSignDisable(options.GetDisableCodeSign());
    options.SetDisableCodeSign(jitDisableCodeSign);

    bool jitEnableJitFort = ohos::JitTools::GetEnableJitFort(options.GetEnableJitFort());
    options.SetEnableJitFort(jitEnableJitFort);

    bool jitEnableAsyncCopyToFort = ohos::JitTools::GetEnableAsyncCopyToFort(options.GetEnableAsyncCopyToFort());
    options.SetEnableAsyncCopyToFort(jitEnableAsyncCopyToFort);

    vm->SetEnableJitLogSkip(ohos::JitTools::GetSkipJitLogEnable());

    LOG_JIT(INFO) << "enable jit bundle:" << bundleName_ <<
        ", litecg:" << jitEnableLitecg <<
        ", call threshold:" << static_cast<int>(jitCallThreshold) <<
        ", hotness threshold:" << jitHotnessThreshold <<
        ", disable codesigner:" << jitDisableCodeSign;
}

void Jit::ConfigJit(EcmaVM *vm)
{
    SwitchProfileStubs(vm);
    ConfigOptions(vm);
    ConfigJitFortOptions(vm);
}

void Jit::ConfigJitFortOptions(EcmaVM *vm)
{
    SetDisableCodeSign(vm->GetJSOptions().GetDisableCodeSign());
    SetEnableJitFort(vm->GetJSOptions().GetEnableJitFort());
    SetEnableAsyncCopyToFort(vm->GetJSOptions().GetEnableAsyncCopyToFort());
}

void Jit::SetEnableOrDisable(const JSRuntimeOptions &options, bool isEnableFastJit, bool isEnableBaselineJit)
{
    LockHolder holder(setEnableLock_);
    bool enableJit = isEnableFastJit || isEnableBaselineJit;
    if (enableJit) {
        CreateJitResources();
    }

    if (IsLibResourcesResolved()) {
        jitDfx_ = JitDfx::GetInstance();
        jitDfx_->Init(options, bundleName_);
        jitResources_->InitJitEnv(options);
        initialized_ = true;
    }

    if (initialized_) {
        fastJitEnable_ = isEnableFastJit;
        baselineJitEnable_ = isEnableBaselineJit;
    }
}

void Jit::Destroy()
{
    LockHolder holder(setEnableLock_);
    if (!initialized_) {
        return;
    }

    initialized_ = false;
    fastJitEnable_ = false;
    baselineJitEnable_ = false;
    ASSERT(jitResources_ != nullptr);
    jitResources_->Destroy();
    jitResources_ = nullptr;
}

bool Jit::IsEnableFastJit() const
{
    return fastJitEnable_;
}

bool Jit::IsEnableBaselineJit() const
{
    return baselineJitEnable_;
}

bool Jit::IsEnableJitFort() const
{
    return isEnableJitFort_;
}

void Jit::SetEnableJitFort(bool isEnableJitFort)
{
    isEnableJitFort_ = isEnableJitFort;
}

bool Jit::IsDisableCodeSign() const
{
    return isDisableCodeSign_;
}

void Jit::SetDisableCodeSign(bool isDisableCodeSign)
{
    isDisableCodeSign_ = isDisableCodeSign;
}

bool Jit::IsEnableAsyncCopyToFort() const
{
    return isEnableAsyncCopyToFort_;
}

void Jit::SetEnableAsyncCopyToFort(bool isEnableAsyncCopyToFort)
{
    isEnableAsyncCopyToFort_ = isEnableAsyncCopyToFort;
}

Jit::~Jit()
{
}

void Jit::CountInterpExecFuncs(JSHandle<JSFunction> &jsFunction)
{
    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    auto jSPandaFile = method->GetJSPandaFile();
    ASSERT(jSPandaFile != nullptr);
    CString fileDesc = jSPandaFile->GetJSPandaFileDesc();
    CString methodInfo = fileDesc + ":" + method->GetRecordNameStr() + "." + CString(method->GetMethodName());
    auto &profMap = JitWarmupProfiler::GetInstance()->profMap_;
    if (profMap.find(methodInfo) == profMap.end()) {
        profMap.insert({methodInfo, false});
    }
}

void Jit::Compile(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, CompilerTier tier,
                  int32_t osrOffset, JitCompileMode mode)
{
    auto jit = Jit::GetInstance();
    if ((!jit->IsEnableBaselineJit() && tier.IsBaseLine()) ||
        (!jit->IsEnableFastJit() && tier.IsFast())) {
        return;
    }

    if (!vm->IsEnableOsr() && osrOffset != MachineCode::INVALID_OSR_OFFSET) {
        return;
    }

    CompileDecision compileDecision(vm, jsFunction, tier, osrOffset, mode);
    if (!compileDecision.Decision()) {
        return;
    }

    jit->Compile(vm, compileDecision);
}

void Jit::Compile(EcmaVM *vm, const CompileDecision &decision)
{
    auto tier = decision.GetTier();
    auto jsFunction = decision.GetJsFunction();
    auto methodInfo = decision.GetMethodInfo();
    auto methodName = decision.GetMethodName();
    auto osrOffset = decision.GetOsrOffset();
    auto mode = decision.GetCompileMode();

    CString msg = "compile method:" + methodInfo + ", in work thread";
    TimeScope scope(vm, msg, tier, true, true);

    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, ConvertToStdString("JIT::Compile:" + methodInfo));
    if (tier.IsFast()) {
        jsFunction->SetJitCompilingFlag(true);
    } else {
        ASSERT(tier.IsBaseLine());
        jsFunction->SetBaselinejitCompilingFlag(true);
    }
    GetJitDfx()->SetTriggerCount(tier);

    {
        JitTaskpool::GetCurrentTaskpool()->WaitForJitTaskPoolReady();
        EcmaVM *compilerVm = JitTaskpool::GetCurrentTaskpool()->GetCompilerVm();
        std::shared_ptr<JitTask> jitTask = std::make_shared<JitTask>(vm->GetJSThread(),
            // avoid check fail when enable multi-thread check
            compilerVm->GetJSThreadNoCheck(), this, jsFunction, tier, methodName, osrOffset, mode);

        jitTask->PrepareCompile();
        JitTaskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<JitTask::AsyncTask>(jitTask, vm->GetJSThread()->GetThreadId()));
        if (mode.IsSync()) {
            // sync mode, also compile in taskpool as litecg unsupport parallel compile,
            // wait task compile finish then install code
            jitTask->WaitFinish();
            jitTask->InstallCode();
        }
        int spendTime = scope.TotalSpentTimeInMicroseconds();
        jitTask->SetMainThreadCompilerTime(spendTime);
        GetJitDfx()->RecordSpentTimeAndPrintStatsLogInJsThread(spendTime);
    }
}

void Jit::RequestInstallCode(std::shared_ptr<JitTask> jitTask)
{
    LockHolder holder(threadTaskInfoLock_);
    ThreadTaskInfo &info = threadTaskInfo_[jitTask->GetHostThread()];
    if (info.skipInstallTask_) {
        return;
    }
    info.installJitTasks_.push_back(jitTask);

    // set
    jitTask->GetHostThread()->SetInstallMachineCode(true);
    jitTask->GetHostThread()->SetCheckSafePointStatus();
}

uint32_t Jit::GetRunningTaskCnt(EcmaVM *vm)
{
    uint32_t cnt = 0;
    JitTaskpool::GetCurrentTaskpool()->ForEachTask([&cnt, &vm](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        if (asyncTask->GetHostVM() == vm) {
            cnt ++;
        }
    });
    LockHolder holder(threadTaskInfoLock_);
    ThreadTaskInfo &info = threadTaskInfo_[vm->GetJSThread()];
    auto &taskQueue = info.installJitTasks_;
    return taskQueue.size() + cnt;
}

void Jit::InstallTasks(JSThread *jsThread)
{
    LockHolder holder(threadTaskInfoLock_);
    ThreadTaskInfo &info = threadTaskInfo_[jsThread];
    auto &taskQueue = info.installJitTasks_;

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
    return jitResources_->Compile(compiler, jitTask);
}

bool Jit::JitFinalize(void *compiler, JitTask *jitTask)
{
    return jitResources_->Finalize(compiler, jitTask);
}

void *Jit::CreateJitCompilerTask(JitTask *jitTask)
{
    return jitResources_->CreateJitCompilerTask(jitTask);
}

void Jit::DeleteJitCompilerTask(void *compiler)
{
    jitResources_->DeleteJitCompilerTask(compiler);
}

void Jit::ClearTask(const std::function<bool(Task *task)> &checkClear)
{
    JitTaskpool::GetCurrentTaskpool()->ForEachTask([&checkClear](Task *task) {
        JitTask::AsyncTask *asyncTask = static_cast<JitTask::AsyncTask*>(task);
        if (checkClear(asyncTask)) {
            asyncTask->Terminated();
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
        LockHolder holder(threadTaskInfoLock_);
        ThreadTaskInfo &info = threadTaskInfo_[vm->GetJSThread()];
        info.skipInstallTask_ = true;
        auto &taskQueue = info.installJitTasks_;
        taskQueue.clear();

        if (info.jitTaskCnt_.load() != 0) {
            ThreadNativeScope threadNativeScope(vm->GetJSThread());
            info.jitTaskCntCv_.Wait(&threadTaskInfoLock_);
        }
    }
}

void Jit::IncJitTaskCnt(JSThread *thread)
{
    LockHolder holder(threadTaskInfoLock_);
    ThreadTaskInfo &info = threadTaskInfo_[thread];
    info.jitTaskCnt_.fetch_add(1);
}

void Jit::DecJitTaskCnt(JSThread *thread)
{
    LockHolder holder(threadTaskInfoLock_);
    ThreadTaskInfo &info = threadTaskInfo_[thread];
    uint32_t old = info.jitTaskCnt_.fetch_sub(1);
    if (old == 1) {
        info.jitTaskCntCv_.Signal();
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
        LOG_JIT(DEBUG) << tier_ << message_ << ": " << TotalSpentTime() << "ms";
    } else {
        auto bundleName = vm_->GetBundleName();
        if (vm_->GetEnableJitLogSkip() && bundleName != "" && message_.find(bundleName) == std::string::npos) {
            return;
        }
        LOG_JIT(INFO) << tier_ << message_ << ", compile time: " << TotalSpentTime() << "ms";
    }
}
}  // namespace panda::ecmascript
