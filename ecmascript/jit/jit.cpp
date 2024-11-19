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
void (*Jit::initJitCompiler_)(JSRuntimeOptions options) = nullptr;
bool(*Jit::jitCompile_)(void*, JitTask*) = nullptr;
bool(*Jit::jitFinalize_)(void*, JitTask*) = nullptr;
void*(*Jit::createJitCompilerTask_)(JitTask*) = nullptr;
void(*Jit::deleteJitCompile_)(void*) = nullptr;
void *Jit::libHandle_ = nullptr;

Jit *Jit::GetInstance()
{
    static Jit instance_;
    return &instance_;
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
        // for app threshold
        uint32_t defaultSize = 3000;
        uint32_t threshold = ohos::JitTools::GetJitHotnessThreshold(defaultSize);
        options.SetJitHotnessThreshold(threshold);
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
            jitDfx_ = JitDfx::GetInstance();
            jitDfx_->Init(options, bundleName_);

            isApp_ = options.IsEnableAPPJIT();
            hotnessThreshold_ = options.GetJitHotnessThreshold();
            if (initJitCompiler_ != nullptr) {
                initJitCompiler_(options);
            }
            bool enableCodeSign = !ohos::JitTools::GetCodeSignDisable(options.GetDisableCodeSign());
            bool shouldCompileMain =
                options.IsEnableForceJitCompileMain() || options.IsEnableForceBaselineCompileMain();
            if (enableCodeSign && shouldCompileMain) {
                JitFort::InitJitFortResource();
            }
            JitTaskpool::GetCurrentTaskpool()->Initialize(enableCodeSign && !shouldCompileMain);
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

void Jit::Initialize()
{
#if defined(OHOS_UNIT_TEST)
#else
    static const std::string CREATEJITCOMPILETASK = "CreateJitCompilerTask";
    static const std::string JITCOMPILEINIT = "InitJitCompiler";
    static const std::string JITCOMPILE = "JitCompile";
    static const std::string JITFINALIZE = "JitFinalize";
    static const std::string DELETEJITCOMPILE = "DeleteJitCompile";
    static const std::string LIBARK_JSOPTIMIZER = "libark_jsoptimizer.so";

    libHandle_ = LoadLib(LIBARK_JSOPTIMIZER);
    if (libHandle_ == nullptr) {
        char *error = LoadLibError();
        LOG_JIT(ERROR) << "jit dlopen libark_jsoptimizer.so failed, as:" <<
            ((error == nullptr) ? "unknown error" : error);
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

    deleteJitCompile_ = reinterpret_cast<void(*)(void*)>(FindSymbol(libHandle_, DELETEJITCOMPILE.c_str()));
    if (deleteJitCompile_ == nullptr) {
        LOG_JIT(ERROR) << "jit can't find symbol deleteJitCompile";
        return;
    }
#endif
    initialized_= true;
    return;
}

Jit::~Jit()
{
}

void Jit::DeleteJitCompile(void *compiler)
{
    if (deleteJitCompile_ != nullptr) {
        deleteJitCompile_(compiler);
    }
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
    jsFunction->SetJitCompilingFlag(true);
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
    if (createJitCompilerTask_ == nullptr) {
        return nullptr;
    }
    return createJitCompilerTask_(jitTask);
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
