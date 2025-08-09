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

#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"


#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

#if defined(ENABLE_FFRT_INTERFACES)
#include "c/executor_task.h"
#endif

#if !defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    #error "ECMASCRIPT_SUPPORT_CPUPROFILER not defined"
#endif

namespace panda::ecmascript {
Mutex CpuProfiler::synchronizationMutex_;
CMap<pthread_t, struct TaskInfo> CpuProfiler::profilerMap_ = CMap<pthread_t, struct TaskInfo>();
CpuProfiler::CpuProfiler(const EcmaVM *vm, const int interval) : vm_(vm), interval_(interval)
{
    enableVMTag_ = const_cast<EcmaVM *>(vm)->GetJSOptions().EnableCpuProfilerVMTag();
    generator_ = new SamplesRecord();
    generator_->SetEnableVMTag(enableVMTag_);
    generator_->SetSourceMapTranslateCallback(vm->GetSourceMapTranslateCallback());
    generator_->NodeInit();
    if (generator_->SemInit(0, 0, 0) != 0) {
        LOG_ECMA(ERROR) << "sem_[0] init failed";
    }
    if (generator_->SemInit(1, 0, 0) != 0) {
        LOG_ECMA(ERROR) << "sem_[1] init failed";
    }
    if (generator_->SemInit(2, 0, 0) != 0) { // 2: signal 2
        LOG_ECMA(ERROR) << "sem_[2] init failed";
    }
}

bool CpuProfiler::RegisterGetStackSignal()
{
    struct sigaction sa;
    sa.sa_sigaction = &GetStackSignalHandler;
    if (sigemptyset(&sa.sa_mask) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::RegisterGetStackSignal, sigemptyset failed, errno = " << errno;
        return false;
    }
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGPROF, &sa, nullptr) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::RegisterGetStackSignal, sigaction failed, errno = " << errno;
        return false;
    }
    return true;
}

bool CpuProfiler::StartCpuProfilerForInfo()
{
    LOG_ECMA(INFO) << "CpuProfiler::StartCpuProfilerForInfo, sampling interval = " << interval_;
    if (isProfiling_) {
        LOG_ECMA(ERROR) << "CpuProfiler::StartCpuProfilerForInfo, can not start when CpuProfiler is Profiling";
        return false;
    }
    if (!RegisterGetStackSignal()) {
        return false;
    }
    // when the ffrt is enabled for the thread, the tid_ will be task id
    tid_ = static_cast<pthread_t>(JSThread::GetCurrentThreadId());
    void *taskHandle = nullptr;
    // when the ffrt is enabled for the thread,
    // we record the task handle, which can be used to obtain the running thread id in the task
#if defined(ENABLE_FFRT_INTERFACES)
    taskHandle = ffrt_get_cur_task();
    TaskInfo taskInfo { vm_, taskHandle };
#else
    TaskInfo taskInfo { vm_, nullptr };
#endif // defined(ENABLE_FFRT_INTERFACES)
    {
        LockHolder lock(synchronizationMutex_);
        profilerMap_[tid_] = taskInfo;
    }

    JSPandaFileManager *pandaFileManager = JSPandaFileManager::GetInstance();
    pandaFileManager->EnumerateJSPandaFiles([&](const std::shared_ptr<JSPandaFile> &file) -> bool {
        pandaFileManager->CpuProfilerGetJSPtExtractor(file.get());
        return true;
    });

    generator_->SetTimeDeltaThreshold(interval_ * THRESHOLD_GROWTH_FACTORY + THRESHOLD_FIXED_INCREMENT);
    generator_->SetIsStart(true);
    uint64_t startTime = SamplingProcessor::GetMicrosecondsTimeStamp();
    generator_->SetThreadStartTime(startTime);
    params_ = new RunParams(generator_, static_cast<uint32_t>(interval_), pthread_self(), taskHandle);
    if (pthread_create(&tid_, nullptr, SamplingProcessor::Run, params_) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::StartCpuProfilerForInfo, pthread_create failed, errno = " << errno;
        return false;
    }
    isProfiling_ = true;
    vm_->GetJSThread()->SetIsProfiling(true);
    outToFile_ = false;
    return true;
}

bool CpuProfiler::StartCpuProfilerForFile(const std::string &fileName)
{
    LOG_ECMA(INFO) << "CpuProfiler::StartCpuProfilerForFile, sampling interval = " << interval_;
    if (isProfiling_) {
        LOG_ECMA(ERROR) << "CpuProfiler::StartCpuProfilerForFile, can not start when CpuProfiler is Profiling";
        return false;
    }
    std::string absoluteFilePath("");
    if (!CheckFileName(fileName, absoluteFilePath)) {
        return false;
    }
    fileName_ = absoluteFilePath;
    generator_->SetFileName(fileName_);
    generator_->fileHandle_.open(fileName_.c_str());
    if (generator_->fileHandle_.fail()) {
        LOG_ECMA(ERROR) << "CpuProfiler::StartCpuProfilerForFile, fileHandle_ open failed";
        return false;
    }
    if (!RegisterGetStackSignal()) {
        return false;
    }
    // when the ffrt is enabled for the thread, the tid_ will be task id
    tid_ = static_cast<pthread_t>(JSThread::GetCurrentThreadId());
    void *taskHandle = nullptr;
    // when the ffrt is enabled for the thread,
    // we record the task handle, which can be used to obtain the running thread id in the task
#if defined(ENABLE_FFRT_INTERFACES)
    taskHandle = ffrt_get_cur_task();
    TaskInfo taskInfo { vm_, taskHandle };
#else
    TaskInfo taskInfo { vm_, nullptr };
#endif // defined(ENABLE_FFRT_INTERFACES)
    {
        LockHolder lock(synchronizationMutex_);
        profilerMap_[tid_] = taskInfo;
    }

    JSPandaFileManager *pandaFileManager = JSPandaFileManager::GetInstance();
    pandaFileManager->EnumerateJSPandaFiles([&](const std::shared_ptr<JSPandaFile> &file) -> bool {
        pandaFileManager->CpuProfilerGetJSPtExtractor(file.get());
        return true;
    });

    generator_->SetTimeDeltaThreshold(interval_ * THRESHOLD_GROWTH_FACTORY + THRESHOLD_FIXED_INCREMENT);
    generator_->SetIsStart(true);
    uint64_t startTime = SamplingProcessor::GetMicrosecondsTimeStamp();
    generator_->SetThreadStartTime(startTime);
    params_ = new RunParams(generator_, static_cast<uint32_t>(interval_), pthread_self(), taskHandle);
    if (pthread_create(&tid_, nullptr, SamplingProcessor::Run, params_) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::StartCpuProfilerForFile, pthread_create failed, errno = " << errno;
        return false;
    }
    isProfiling_ = true;
    vm_->GetJSThread()->SetIsProfiling(true);
    outToFile_ = true;
    return true;
}

bool CpuProfiler::StopCpuProfilerForInfo(std::unique_ptr<struct ProfileInfo> &profileInfo)
{
    LOG_ECMA(INFO) << "CpuProfiler::StopCpuProfilerForInfo enter";
    if (!isProfiling_) {
        LOG_ECMA(WARN) << "CpuProfiler::StopCpuProfilerForInfo, not isProfiling_";
        return true;
    }
    if (outToFile_) {
        LOG_ECMA(ERROR) << "CpuProfiler::StopCpuProfilerForInfo, is outToFile_";
        return false;
    }
    generator_->SetIsStart(false);
    if (generator_->SemPost(0) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::StopCpuProfilerForInfo, sem_[0] post failed, errno = " << errno;
        return false;
    }
    if (generator_->SemWait(1) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::StopCpuProfilerForInfo, sem_[1] wait failed, errno = " << errno;
        return false;
    }
    isProfiling_ = false;
    vm_->GetJSThread()->SetIsProfiling(false);
    profileInfo = generator_->GetProfileInfo();
    return true;
}

void CpuProfiler::SetCpuSamplingInterval(int interval)
{
    interval_ = static_cast<uint32_t>(interval);
}

bool CpuProfiler::StopCpuProfilerForFile()
{
    LOG_ECMA(INFO) << "CpuProfiler::StopCpuProfilerForFile enter";
    if (!isProfiling_) {
        LOG_ECMA(WARN) << "CpuProfiler::StopCpuProfilerForFile, not isProfiling_";
        return true;
    }
    if (!outToFile_) {
        LOG_ECMA(ERROR) << "CpuProfiler::StopCpuProfilerForFile, not outToFile_";
        return false;
    }
    generator_->SetIsStart(false);
    if (generator_->SemPost(0) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::StopCpuProfilerForFile, sem_[0] post failed, errno = " << errno;
        return false;
    }
    if (generator_->SemWait(1) != 0) {
        LOG_ECMA(ERROR) << "CpuProfiler::StopCpuProfilerForFile, sem_[1] wait failed, errno = " << errno;
        return false;
    }
    isProfiling_ = false;
    vm_->GetJSThread()->SetIsProfiling(false);
    generator_->StringifySampleData();
    std::string fileData = generator_->GetSampleData();
    generator_->fileHandle_ << fileData;
    return true;
}

CpuProfiler::~CpuProfiler()
{
    if (generator_->SemDestroy(0) != 0) {
        LOG_ECMA(ERROR) << "sem_[0] destroy failed";
    }
    if (generator_->SemDestroy(1) != 0) {
        LOG_ECMA(ERROR) << "sem_[1] destroy failed";
    }
    if (generator_->SemDestroy(2) != 0) { // 2: signal 2
        LOG_ECMA(ERROR) << "sem_[2] destroy failed";
    }
    if (generator_ != nullptr) {
        delete generator_;
        generator_ = nullptr;
    }
    if (params_ != nullptr) {
        delete params_;
        params_ = nullptr;
    }
}

void CpuProfiler::GetStack(FrameIterator &it)
{
    const CMap<struct MethodKey, struct FrameInfo> &stackInfo = generator_->GetStackInfo();
    bool topFrame = true;
    generator_->ResetFrameLength();
    for (; !it.Done(); it.Advance<>()) {
        auto method = it.CheckAndGetMethod();
        if (method == nullptr || !JSTaggedValue(method).IsMethod()) {
            continue;
        }
        bool isNative = method->IsNativeWithCallField();
        struct MethodKey methodKey;
        methodKey.deoptType = method->GetDeoptType();
        if (topFrame) {
            methodKey.state = JsStackGetter::GetRunningState(it, vm_, isNative, true, enableVMTag_);
            topFrame = false;
        } else {
            methodKey.state = JsStackGetter::GetRunningState(it, vm_, isNative, false, enableVMTag_);
        }
        if (isNative) {
            JsStackGetter::GetCallLineNumber(it, vm_, methodKey.lineNumber);
        }
        void *methodIdentifier = JsStackGetter::GetMethodIdentifier(method, it, vm_);
        if (methodIdentifier == nullptr) {
            continue;
        }
        methodKey.methodIdentifier = methodIdentifier;
        if (stackInfo.count(methodKey) == 0) {
            struct FrameInfoTemp codeEntry;
            if (UNLIKELY(!JsStackGetter::ParseMethodInfo(methodKey, it, vm_, codeEntry, true))) {
                continue;
            }
            if (UNLIKELY(!generator_->PushStackInfo(codeEntry))) {
                return;
            }
        }
        if (UNLIKELY(!generator_->PushFrameStack(methodKey))) {
            return;
        }
    }
    generator_->PostFrame();
}

bool CpuProfiler::GetStackBeforeCallNapi(JSThread *thread)
{
    uint64_t tempTimeStamp = SamplingProcessor::GetMicrosecondsTimeStamp();
    if (tempTimeStamp - beforeCallNapiTimeStamp_ < interval_) {
        return false;
    }

    if (GetStackCallNapi(thread, true)) {
        beforeCallNapiTimeStamp_ = tempTimeStamp;
        return true;
    }
    return false;
}

void CpuProfiler::GetStackAfterCallNapi(JSThread *thread)
{
    GetStackCallNapi(thread, false);
}

bool CpuProfiler::GetStackCallNapi(JSThread *thread, bool beforeCallNapi)
{
    [[maybe_unused]] CallNapiScope scope(this);
    const CMap<struct MethodKey, struct FrameInfo> &stackInfo = generator_->GetStackInfo();
    generator_->ClearNapiStack();
    bool topFrame = true;
    auto currentFrame = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(currentFrame, thread);
    if (!beforeCallNapi) {
        it.Advance<GCVisitedFlag::IGNORED>();
    }
    for (; !it.Done(); it.Advance<GCVisitedFlag::IGNORED>()) {
        auto method = it.CheckAndGetMethod();
        if (method == nullptr || !JSTaggedValue(method).IsMethod()) {
            continue;
        }

        bool isNative = method->IsNativeWithCallField();
        struct MethodKey methodKey;
        methodKey.deoptType = method->GetDeoptType();
        if (topFrame) {
            if (beforeCallNapi) {
                methodKey.state = RunningState::NAPI;
            } else {
                methodKey.state = JsStackGetter::GetRunningState(it, vm_, isNative, true, enableVMTag_);
            }
            topFrame = false;
        } else {
            methodKey.state = JsStackGetter::GetRunningState(it, vm_, isNative, false, enableVMTag_);
        }
        if (isNative) {
            JsStackGetter::GetCallLineNumber(it, vm_, methodKey.lineNumber);
        }
        void *methodIdentifier = JsStackGetter::GetMethodIdentifier(method, it, vm_);
        if (methodIdentifier == nullptr) {
            continue;
        }
        methodKey.methodIdentifier = methodIdentifier;
        if (stackInfo.count(methodKey) == 0) {
            struct FrameInfoTemp codeEntry;
            if (UNLIKELY(!JsStackGetter::ParseMethodInfo(methodKey, it, vm_, codeEntry, true))) {
                continue;
            }
            if (UNLIKELY(!generator_->PushNapiStackInfo(codeEntry))) {
                return false;
            }
        }
        if (UNLIKELY(!generator_->PushNapiFrameStack(methodKey))) {
            return false;
        }
    }
    generator_->PostNapiFrame();
    return true;
}

void CpuProfiler::GetStackSignalHandler(int signal, [[maybe_unused]] siginfo_t *siginfo, void *context)
{
    if (signal != SIGPROF) {
        return;
    }
    CpuProfiler *profiler = nullptr;
    JSThread *thread = nullptr;
    {
        LockHolder lock(synchronizationMutex_);
        // If no task running in this thread, we get the id of the last task that ran in this thread
        pthread_t tid = static_cast<pthread_t>(GetThreadIdOrCachedTaskId());
        const EcmaVM *vm = profilerMap_[tid].vm_;
        if (vm == nullptr) {
            LOG_ECMA(ERROR) << "CpuProfiler GetStackSignalHandler vm is nullptr";
            return;
        }
        profiler = vm->GetProfiler();
        thread = vm->GetAssociatedJSThread();
        if (profiler == nullptr) {
            LOG_ECMA(ERROR) << "CpuProfiler GetStackSignalHandler profiler is nullptr";
            return;
        }
    }
    [[maybe_unused]] SignalStateScope scope(thread->GetEcmaVM()->GetJsDebuggerManager());

    if (profiler->GetBuildNapiStack() || thread->GetGcState()) {
        if (profiler->generator_->SemPost(0) != 0) {
            LOG_ECMA(ERROR) << "sem_[0] post failed";
        }
        return;
    }

    uint64_t pc = 0;
    if (thread->IsAsmInterpreter()) {
        // If the attempt fails, the callback will be terminated directly to avoid the reentrancy deadlock,
        // and a sampling will be abandoned. Failures are rare, so the impact on the overall sampling results
        // is very limited.
        if (!thread->GetEcmaVM()->GetAOTFileManager()->TryReadLock()) {
            if (profiler->generator_->SemPost(0) != 0) {
                LOG_ECMA(ERROR) << "sem_[0] post failed";
            }
            return;
        }
        pc = GetPcFromContext(context);
    }
    if (thread->IsAsmInterpreter() && profiler->IsAddrAtStubOrAot(pc) &&
        !profiler->IsEntryFrameHeaderOrTail(thread, pc)) {
        if (profiler->IfNeedSkipBarrierStubHeaderOrTail(thread, pc)) {
            return;
        }
        [[maybe_unused]] ucontext_t *ucontext = reinterpret_cast<ucontext_t*>(context);
        [[maybe_unused]] mcontext_t &mcontext = ucontext->uc_mcontext;
        [[maybe_unused]] void *fp = nullptr;
        [[maybe_unused]] void *sp = nullptr;
#if defined(PANDA_TARGET_AMD64)
        fp = reinterpret_cast<void*>(mcontext.gregs[REG_RBP]);
        sp = reinterpret_cast<void*>(mcontext.gregs[REG_RSP]);
#elif defined(PANDA_TARGET_ARM64)
        fp = reinterpret_cast<void*>(mcontext.regs[29]); // FP is an alias for x29.
        sp = reinterpret_cast<void*>(mcontext.sp);
#else
        LOG_FULL(FATAL) << "AsmInterpreter does not currently support other platforms, please run on x64 and arm64";
        return;
#endif
        if (reinterpret_cast<uint64_t*>(sp) > reinterpret_cast<uint64_t*>(fp)) {
            LOG_ECMA(ERROR) << "sp > fp, stack frame exception";
            if (profiler->generator_->SemPost(0) != 0) {
                LOG_ECMA(ERROR) << "sem_[0] post failed";
            }
            return;
        }
        if (JsStackGetter::CheckFrameType(thread, reinterpret_cast<JSTaggedType *>(fp))) {
            FrameIterator it(reinterpret_cast<JSTaggedType *>(fp), thread);
            profiler->GetStack(it);
        }
    } else if (thread->IsAsmInterpreter()) {
        if (thread->GetLastLeaveFrame() != nullptr) {
            JSTaggedType *leaveFrame = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
            if (JsStackGetter::CheckFrameType(thread, leaveFrame)) {
                FrameIterator it(leaveFrame, thread);
                profiler->GetStack(it);
            }
        }
    } else {
        if (thread->GetCurrentFrame() != nullptr) {
            if (JsStackGetter::CheckFrameType(thread, const_cast<JSTaggedType *>(thread->GetCurrentFrame()))) {
                FrameHandler frameHandler(thread);
                FrameIterator it(frameHandler.GetSp(), thread);
                profiler->GetStack(it);
            }
        }
    }
    if (profiler->generator_->SemPost(0) != 0) {
        LOG_ECMA(ERROR) << "sem_[0] post failed";
        return;
    }
}

bool CpuProfiler::InHeaderOrTail(uint64_t pc, uint64_t entryBegin, uint64_t entryDuration, uint64_t headerSize,
                                 uint64_t tailSize) const
{
    uintptr_t entryEnd = entryBegin + entryDuration;
    if (pc >= entryBegin && pc <= (entryBegin + headerSize)) {
        return true;
    }
    if (pc <= entryEnd && pc >= (entryEnd - tailSize)) {
        return true;
    }
    return false;
}

bool CpuProfiler::IfNeedSkipBarrierStubHeaderOrTail(JSThread *thread, uint64_t pc) const
{
    uint64_t headerSize = 20;
    uintptr_t entryBegin = thread->GetFastStubEntry(kungfu::CommonStubCSigns::ID::GetValueWithBarrier);
    bool needSkip = InHeaderOrTail(pc, entryBegin, 0, headerSize, 0);
    return needSkip;
}

bool CpuProfiler::IsEntryFrameHeaderOrTail(JSThread *thread, uint64_t pc) const
{
    uint64_t headerSize = 0;
    uint64_t tailSize = 0;
    uint64_t entryDuration = 0;
    Assembler::GetFrameCompletionPos(headerSize, tailSize, entryDuration);
    uintptr_t entryBegin = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AsmInterpreterEntry);
    bool inAsmInterpreterEntry = InHeaderOrTail(pc, entryBegin, entryDuration, headerSize, tailSize);
    entryBegin = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_GeneratorReEnterAsmInterp);
    bool inGeneratorReEnterAsmInterp = InHeaderOrTail(pc, entryBegin, entryDuration, headerSize, tailSize);
    return (inAsmInterpreterEntry || inGeneratorReEnterAsmInterp);
}

uint64_t CpuProfiler::GetPcFromContext(void *context)
{
    [[maybe_unused]] ucontext_t *ucontext = reinterpret_cast<ucontext_t*>(context);
    [[maybe_unused]] mcontext_t &mcontext = ucontext->uc_mcontext;
    uint64_t pc = 0;
#if defined(PANDA_TARGET_AMD64)
    pc = static_cast<uint64_t>(mcontext.gregs[REG_RIP]);
#elif defined(PANDA_TARGET_ARM64)
    pc = static_cast<uint64_t>(mcontext.pc);
#else
    LOG_FULL(FATAL) << "AsmInterpreter does not currently support other platforms, please run on x64 and arm64";
    pc = 0;
#endif
    return pc;
}

bool CpuProfiler::IsAddrAtStubOrAot(uint64_t pc) const
{
    AOTFileManager *loader = vm_->GetAOTFileManager();
    return loader->InsideStub(pc) || loader->InsideAOT(pc);
}

bool CpuProfiler::CheckFileName(const std::string &fileName, std::string &absoluteFilePath) const
{
    if (fileName.empty()) {
        LOG_ECMA(ERROR) << "CpuProfiler::CheckFileName, fileName is empty";
        return false;
    }

    if (fileName.size() > PATH_MAX) {
        LOG_ECMA(ERROR) << "CpuProfiler::CheckFileName, fileName exceed PATH_MAX";
        return false;
    }

    CVector<char> resolvedPath(PATH_MAX);
    auto result = realpath(fileName.c_str(), resolvedPath.data());
    if (result == nullptr) {
        LOG_ECMA(ERROR) << "CpuProfiler::CheckFileName, realpath fail, errno = " << errno;
        return false;
    }
    std::ofstream file(resolvedPath.data());
    if (!file.good()) {
        LOG_ECMA(ERROR) << "CpuProfiler::CheckFileName, file is not good, errno = " << errno;
        return false;
    }
    file.close();
    absoluteFilePath = resolvedPath.data();
    return true;
}

void CpuProfiler::SetBuildNapiStack(bool flag)
{
    isBuildNapiStack_.store(flag);
}

bool CpuProfiler::GetBuildNapiStack()
{
    return isBuildNapiStack_.load();
}

bool CpuProfiler::GetOutToFile()
{
    return outToFile_;
}

EcmaVM* CpuProfiler::GetVmbyTid(pthread_t tid)
{
    LockHolder lock(synchronizationMutex_);
    return const_cast<EcmaVM *>(profilerMap_[tid].vm_);
}
} // namespace panda::ecmascript
