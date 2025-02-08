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

#include "ecmascript/ecma_vm.h"

#include "ecmascript/builtins/builtins_ark_tools.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_number_format.h"
#endif
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/jit/jit_task.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#if !WIN_OR_MAC_OR_IOS_PLATFORM
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#endif
#include "ecmascript/dfx/tracing/tracing.h"
#include "ecmascript/dfx/vmstat/function_call_timer.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/pgo_profiler/pgo_trace.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/ohos/jit_tools.h"
#include "ecmascript/ohos/aot_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/dfx/stackinfo/async_stack_trace.h"

#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

namespace panda::ecmascript {
using RandomGenerator = base::RandomGenerator;
using PGOProfilerManager = pgo::PGOProfilerManager;
using JitTools = ohos::JitTools;
AOTFileManager *JsStackInfo::loader = nullptr;
bool EcmaVM::multiThreadCheck_ = false;
bool EcmaVM::errorInfoEnhanced_ = false;

EcmaVM *EcmaVM::Create(const JSRuntimeOptions &options)
{
    Runtime::CreateIfFirstVm(options);
    auto heapType = options.IsWorker() ? EcmaParamConfiguration::HeapType::WORKER_HEAP :
        EcmaParamConfiguration::HeapType::DEFAULT_HEAP;
    size_t heapSize = options.GetHeapSize();
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
    switch (heapType) {
        case EcmaParamConfiguration::HeapType::WORKER_HEAP:
            heapSize = OHOS::system::GetUintParameter<size_t>("persist.ark.heap.workersize", 0) * 1_MB;
            break;
        default:
            heapSize = OHOS::system::GetUintParameter<size_t>("persist.ark.heap.defaultsize", 0) * 1_MB;
            break;
    }
#endif
    auto config = EcmaParamConfiguration(heapType,
                                         MemMapAllocator::GetInstance()->GetCapacity(),
                                         heapSize);
    JSRuntimeOptions newOptions = options;
    // only define SUPPORT_ENABLE_ASM_INTERP can enable asm-interpreter
#if !defined(SUPPORT_ENABLE_ASM_INTERP)
    newOptions.SetEnableAsmInterpreter(false);
#endif
    auto vm = new EcmaVM(newOptions, config);
    auto jsThread = JSThread::Create(vm);
    vm->thread_ = jsThread;
    Runtime::GetInstance()->InitializeIfFirstVm(vm);
    if (JsStackInfo::loader == nullptr) {
        JsStackInfo::loader = vm->GetAOTFileManager();
    }
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
    int arkProperties = OHOS::system::GetIntParameter<int>("persist.ark.properties", -1);
    vm->GetJSOptions().SetArkProperties(arkProperties);
#endif
    return vm;
}

// static
bool EcmaVM::Destroy(EcmaVM *vm)
{
    if (UNLIKELY(vm == nullptr)) {
        return false;
    }
    delete vm;
    Runtime::DestroyIfLastVm();
    return true;
}

void EcmaVM::PreFork()
{
    heap_->CompactHeapBeforeFork();
    heap_->AdjustSpaceSizeForAppSpawn();
    heap_->GetReadOnlySpace()->SetReadOnly();
    heap_->DisableParallelGC();
    SetPostForked(false);

    auto sHeap = SharedHeap::GetInstance();
    sHeap->CompactHeapBeforeFork(thread_);
    sHeap->DisableParallelGC(thread_);
    heap_->GetWorkManager()->FinishInPreFork();
    sHeap->GetWorkManager()->FinishInPreFork();
    Jit::GetInstance()->PreFork();
}

void EcmaVM::PostFork()
{
    RandomGenerator::InitRandom(GetAssociatedJSThread());
    heap_->SetHeapMode(HeapMode::SHARE);
    GetAssociatedJSThread()->PostFork();
    DaemonThread::GetInstance()->StartRunning();
    Taskpool::GetCurrentTaskpool()->Initialize();
    heap_->GetWorkManager()->InitializeInPostFork();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetWorkManager()->InitializeInPostFork();
    SetPostForked(true);
    LOG_ECMA(INFO) << "multi-thread check enabled: " << GetThreadCheckStatus();
    SignalAllReg();
    SharedHeap::GetInstance()->EnableParallelGC(GetJSOptions());
    heap_->EnableParallelGC();
    options_.SetPgoForceDump(false);
    std::string bundleName = PGOProfilerManager::GetInstance()->GetBundleName();
    pgo::PGOTrace::GetInstance()->SetEnable(ohos::AotTools::GetPgoTraceEnable());
    AotCrashInfo::GetInstance().SetOptionPGOProfiler(&options_, bundleName);
    ResetPGOProfiler();
    processStartRealtime_ = InitializeStartRealTime();

    Jit::GetInstance()->SetJitEnablePostFork(this, bundleName);
    ModuleLogger *moduleLogger = thread_->GetCurrentEcmaContext()->GetModuleLogger();
    if (moduleLogger != nullptr) {
        moduleLogger->PostModuleLoggerTask(thread_->GetThreadId(), this);
    }
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
    int arkProperties = OHOS::system::GetIntParameter<int>("persist.ark.properties", -1);
    GetJSOptions().SetArkProperties(arkProperties);
#endif
#ifdef ENABLE_POSTFORK_FORCEEXPAND
    heap_->NotifyPostFork();
    heap_->NotifyFinishColdStartSoon();
#endif
}

EcmaVM::EcmaVM(JSRuntimeOptions options, EcmaParamConfiguration config)
    : nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()),
      heapRegionAllocator_(std::make_unique<HeapRegionAllocator>(options)),
      chunk_(nativeAreaAllocator_.get()),
      ecmaParamConfiguration_(std::move(config))
{
    options_ = std::move(options);
    LOG_ECMA(DEBUG) << "multi-thread check enabled: " << GetThreadCheckStatus();
    icEnabled_ = options_.EnableIC();
    optionalLogEnabled_ = options_.EnableOptionalLog();
    options_.ParseAsmInterOption();
    SetEnableOsr(options_.IsEnableOSR() && options_.IsEnableJIT() && options_.GetEnableAsmInterpreter());
    processStartRealtime_ = InitializeStartRealTime();
}

// for jit
EcmaVM::EcmaVM()
    : nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()),
      heapRegionAllocator_(nullptr),
      chunk_(nativeAreaAllocator_.get()) {}

void EcmaVM::InitializeForJit(JitThread *jitThread)
{
    thread_ = jitThread;
    stringTable_ = Runtime::GetInstance()->GetEcmaStringTable();
    ASSERT(stringTable_);
    // ObjectFactory only sypport alloc string in sharedheap
    factory_ = chunk_.New<ObjectFactory>(thread_, nullptr, SharedHeap::GetInstance());
    SetIsJitCompileVM(true);
}

void EcmaVM::InitializePGOProfiler()
{
    LOG_PGO(INFO) << "initializing pgo profiler, pgo is " << (IsEnablePGOProfiler() ? "enabled" : "disabled")
                  << ", worker is " << (options_.IsWorker() ? "enabled" : "disabled")
                  << ", profiler: " << pgoProfiler_;
    bool isEnablePGOProfiler = IsEnablePGOProfiler();
    if (pgoProfiler_ == nullptr) {
        pgoProfiler_ = PGOProfilerManager::GetInstance()->BuildProfiler(this, isEnablePGOProfiler);
    }
    pgo::PGOTrace::GetInstance()->SetEnable(options_.GetPGOTrace() || ohos::AotTools::GetPgoTraceEnable());
    thread_->SetPGOProfilerEnable(isEnablePGOProfiler);
}

void EcmaVM::ResetPGOProfiler()
{
    if (pgoProfiler_ != nullptr) {
        bool isEnablePGOProfiler = IsEnablePGOProfiler();
        PGOProfilerManager::GetInstance()->Reset(pgoProfiler_, isEnablePGOProfiler);
        thread_->SetPGOProfilerEnable(isEnablePGOProfiler);
        thread_->CheckOrSwitchPGOStubs();
    }
}

void EcmaVM::DisablePGOProfilerWithAOTFile(const std::string &aotFileName)
{
    if (AOTFileManager::AOTFileExist(aotFileName, AOTFileManager::FILE_EXTENSION_AN) ||
        AOTFileManager::AOTFileExist(aotFileName, AOTFileManager::FILE_EXTENSION_AI)) {
        LOG_PGO(INFO) << "disable pgo profiler due to aot file exist: " << aotFileName;
        options_.SetEnablePGOProfiler(false);
        PGOProfilerManager::GetInstance()->SetDisablePGO(true);
        ResetPGOProfiler();
    }
}

bool EcmaVM::IsEnablePGOProfiler() const
{
    if (options_.IsWorker()) {
        return PGOProfilerManager::GetInstance()->IsEnable();
    }
    return options_.GetEnableAsmInterpreter() && options_.IsEnablePGOProfiler();
}

bool EcmaVM::IsEnableMutantArray() const
{
    return options_.GetEnableAsmInterpreter() && options_.IsEnableMutantArray();
}

bool EcmaVM::IsEnableElementsKind() const
{
    return options_.GetEnableAsmInterpreter() && options_.IsEnableElementsKind();
}

bool EcmaVM::IsEnableFastJit() const
{
    return GetJit()->IsEnableFastJit();
}

bool EcmaVM::IsEnableBaselineJit() const
{
    return GetJit()->IsEnableBaselineJit();
}

uint32_t EcmaVM::GetTid() const
{
    return thread_->GetThreadId();
}

Jit *EcmaVM::GetJit() const
{
    return Jit::GetInstance();
}

bool EcmaVM::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "EcmaVM::Initialize");
    stringTable_ = Runtime::GetInstance()->GetEcmaStringTable();
    InitializePGOProfiler();
    Taskpool::GetCurrentTaskpool()->Initialize();
#ifndef PANDA_TARGET_WINDOWS
    RuntimeStubs::Initialize(thread_);
#endif
    heap_ = new Heap(this);
    heap_->Initialize();
    gcStats_ = chunk_.New<GCStats>(heap_, options_.GetLongPauseTime());
    gcKeyStats_ = chunk_.New<GCKeyStats>(heap_, gcStats_);
    factory_ = chunk_.New<ObjectFactory>(thread_, heap_, SharedHeap::GetInstance());
    if (UNLIKELY(factory_ == nullptr)) {
        LOG_FULL(FATAL) << "alloc factory_ failed";
        UNREACHABLE();
    }
    debuggerManager_ = new tooling::JsDebuggerManager(this);
    asyncStackTrace_ = new AsyncStackTrace(this);
    aotFileManager_ = new AOTFileManager(this);
    auto context = new EcmaContext(thread_);
    thread_->PushContext(context);
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    thread_->SetReadyForGCIterating(true);
    thread_->SetSharedMarkStatus(DaemonThread::GetInstance()->GetSharedMarkStatus());
    snapshotEnv_ = new SnapshotEnv(this);
    context->Initialize();
    snapshotEnv_->AddGlobalConstToMap();
    thread_->SetGlueGlobalEnv(reinterpret_cast<GlobalEnv *>(context->GetGlobalEnv().GetTaggedType()));
    thread_->SetGlobalObject(GetGlobalEnv()->GetGlobalObject());
    thread_->SetCurrentEcmaContext(context);
    GenerateInternalNativeMethods();
    quickFixManager_ = new QuickFixManager();
    if (options_.GetEnableAsmInterpreter()) {
        thread_->GetCurrentEcmaContext()->LoadStubFile();
    }

    callTimer_ = new FunctionCallTimer();
    strategy_ = new ThroughputJSObjectResizingStrategy();
    if (IsEnableFastJit() || IsEnableBaselineJit()) {
        Jit::GetInstance()->ConfigJit(this);
    }
    initialized_ = true;
    return true;
}

EcmaVM::~EcmaVM()
{
    if (isJitCompileVM_) {
        if (factory_ != nullptr) {
            chunk_.Delete(factory_);
            factory_ = nullptr;
        }
        stringTable_ = nullptr;
        thread_ = nullptr;
        return;
    }
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!thread_->IsInRunningStateOrProfiling())) {
        LOG_ECMA(FATAL) << "Destruct VM must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    initialized_ = false;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (profiler_ != nullptr) {
        if (profiler_->GetOutToFile()) {
            DFXJSNApi::StopCpuProfilerForFile(this);
        } else {
            DFXJSNApi::StopCpuProfilerForInfo(this);
        }
    }
    if (profiler_ != nullptr) {
        delete profiler_;
        profiler_ = nullptr;
    }
#endif
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    DeleteHeapProfile();
#endif
    if (IsEnableFastJit() || IsEnableBaselineJit()) {
        GetJit()->ClearTaskWithVm(this);
    }
    // clear c_address: c++ pointer delete
    ClearBufferData();
    heap_->WaitAllTasksFinished();
    Taskpool::GetCurrentTaskpool()->Destroy(thread_->GetThreadId());

    if (pgoProfiler_ != nullptr) {
        PGOProfilerManager::GetInstance()->Destroy(pgoProfiler_);
        pgoProfiler_ = nullptr;
    }

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    DumpCallTimeInfo();
#endif

#if defined(ECMASCRIPT_SUPPORT_TRACING)
    if (tracing_) {
        DFXJSNApi::StopTracing(this);
    }
#endif

    thread_->GetCurrentEcmaContext()->GetModuleManager()->NativeObjDestory();

    if (!isBundlePack_) {
        std::shared_ptr<JSPandaFile> jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(assetPath_);
        if (jsPandaFile != nullptr) {
            jsPandaFile->DeleteParsedConstpoolVM(this);
        }
    }

    if (gcStats_ != nullptr) {
        if (options_.EnableGCStatsPrint()) {
            gcStats_->PrintStatisticResult();
        }
        chunk_.Delete(gcStats_);
        gcStats_ = nullptr;
    }

    if (gcKeyStats_ != nullptr) {
        chunk_.Delete(gcKeyStats_);
        gcKeyStats_ = nullptr;
    }

    if (JsStackInfo::loader == aotFileManager_) {
        JsStackInfo::loader = nullptr;
    }

    if (heap_ != nullptr) {
        heap_->Destroy();
        delete heap_;
        heap_ = nullptr;
    }

    SharedHeap *sHeap = SharedHeap::GetInstance();
    const Heap *heap = Runtime::GetInstance()->GetMainThread()->GetEcmaVM()->GetHeap();
    if (IsWorkerThread() && Runtime::SharedGCRequest()) {
        // destory workervm to release mem.
        thread_->SetReadyForGCIterating(false);
        if (sHeap->CheckCanTriggerConcurrentMarking(thread_)) {
            sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, GCReason::WORKER_DESTRUCTION>(thread_);
        } else if (heap && !heap->InSensitiveStatus() && !sHeap->GetConcurrentMarker()->IsEnabled()) {
            sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::WORKER_DESTRUCTION>(thread_);
        }
    }

    if (debuggerManager_ != nullptr) {
        delete debuggerManager_;
        debuggerManager_ = nullptr;
    }

    if (asyncStackTrace_ != nullptr) {
        delete asyncStackTrace_;
        asyncStackTrace_ = nullptr;
    }

    if (aotFileManager_ != nullptr) {
        delete aotFileManager_;
        aotFileManager_ = nullptr;
    }

    if (factory_ != nullptr) {
        chunk_.Delete(factory_);
        factory_ = nullptr;
    }

    if (stringTable_ != nullptr) {
        stringTable_ = nullptr;
    }

    if (quickFixManager_ != nullptr) {
        delete quickFixManager_;
        quickFixManager_ = nullptr;
    }

    if (snapshotEnv_ != nullptr) {
        snapshotEnv_->ClearEnvMap();
        delete snapshotEnv_;
        snapshotEnv_ = nullptr;
    }

    if (callTimer_ != nullptr) {
        delete callTimer_;
        callTimer_ = nullptr;
    }

    if (strategy_ != nullptr) {
        delete strategy_;
        strategy_ = nullptr;
    }

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }
}

JSHandle<GlobalEnv> EcmaVM::GetGlobalEnv() const
{
    return thread_->GetCurrentEcmaContext()->GetGlobalEnv();
}

void EcmaVM::CheckThread() const
{
    // Exclude GC thread
    if (thread_ == nullptr) {
        LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
        UNREACHABLE();
    }
    if (!Taskpool::GetCurrentTaskpool()->IsDaemonThreadOrInThreadPool(std::this_thread::get_id()) &&
        thread_->CheckMultiThread()) {
            LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                                << " thread:" << thread_->GetThreadId()
                                << " currentThread:" << JSThread::GetCurrentThreadId();
        UNREACHABLE();
    }
}

JSThread *EcmaVM::GetAndFastCheckJSThread() const
{
    if (thread_ == nullptr) {
        LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
    }
    if (thread_->CheckMultiThread()) {
        LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                                << " thread:" << thread_->GetThreadId()
                                << " currentThread:" << JSThread::GetCurrentThreadId();
    }
    return thread_;
}

bool EcmaVM::CheckSingleThread() const
{
    if (thread_ == nullptr) {
        LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
        return false;
    }
    if (thread_->GetThreadId() != JSThread::GetCurrentThreadId()) {
        LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                        << " thread:" << thread_->GetThreadId()
                        << " currentThread:" << JSThread::GetCurrentThreadId();
        return false;
    }
    return true;
}

JSTaggedValue EcmaVM::FastCallAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp)
{
    INTERPRETER_TRACE(thread_, ExecuteAot);
    ASSERT(thread_->IsInManagedState());
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_OptimizedFastCallEntry);
    // entry of aot
    auto res = reinterpret_cast<FastCallAotEntryType>(entry)(thread_->GetGlueAddr(),
                                                             actualNumArgs,
                                                             args,
                                                             reinterpret_cast<uintptr_t>(prevFp));
    return res;
}

void EcmaVM::CheckStartCpuProfiler()
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (options_.EnableCpuProfilerColdStartMainThread() && options_.GetArkBundleName().compare(bundleName_) == 0 &&
        !options_.IsWorker() && profiler_ == nullptr) {
        std::string fileName = options_.GetArkBundleName() + ".cpuprofile";
        if (!builtins::BuiltinsArkTools::CreateFile(fileName)) {
            LOG_ECMA(ERROR) << "createFile failed " << fileName;
            return;
        } else {
            DFXJSNApi::StartCpuProfilerForFile(this, fileName, CpuProfiler::INTERVAL_OF_INNER_START);
            return;
        }
    }

    if (options_.EnableCpuProfilerColdStartWorkerThread() && options_.GetArkBundleName().compare(bundleName_) == 0 &&
        options_.IsWorker() && profiler_ == nullptr) {
        std::string fileName = options_.GetArkBundleName() + "_"
                               + std::to_string(thread_->GetThreadId()) + ".cpuprofile";
        if (!builtins::BuiltinsArkTools::CreateFile(fileName)) {
            LOG_ECMA(ERROR) << "createFile failed " << fileName;
            return;
        } else {
            DFXJSNApi::StartCpuProfilerForFile(this, fileName, CpuProfiler::INTERVAL_OF_INNER_START);
            return;
        }
    }
#endif
}

JSHandle<JSTaggedValue> EcmaVM::GetAndClearEcmaUncaughtException() const
{
    JSHandle<JSTaggedValue> exceptionHandle = GetEcmaUncaughtException();
    thread_->ClearException();  // clear for ohos app
    return exceptionHandle;
}

JSHandle<JSTaggedValue> EcmaVM::GetEcmaUncaughtException() const
{
    if (!thread_->HasPendingException()) {
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> exceptionHandle(thread_, thread_->GetException());
    return exceptionHandle;
}

#if ECMASCRIPT_ENABLE_COLLECTING_OPCODES
void EcmaVM::PrintCollectedByteCode()
{
    std::unordered_map<BytecodeInstruction::Opcode, int> bytecodeStatsMap_ = bytecodeStatsStack_.top();
    LOG_ECMA(ERROR) << "panda runtime stat:";
    static constexpr int nameRightAdjustment = 45;
    static constexpr int numberRightAdjustment = 12;
    LOG_ECMA(ERROR) << std::right << std::setw(nameRightAdjustment) << "Hotness Function ByteCode"
                   << std::setw(numberRightAdjustment) << "Count";
    LOG_ECMA(ERROR) << "============================================================"
                      << "=========================================================";
    std::vector<std::pair<std::string, int>> bytecodeStatsVector;
    for (auto& iter: bytecodeStatsMap_) {
        bytecodeStatsVector.push_back(
            std::make_pair(kungfu::GetEcmaOpcodeStr(static_cast<EcmaOpcode>(iter.first)), iter.second));
    }
    std::sort(bytecodeStatsVector.begin(), bytecodeStatsVector.end(),
              [](std::pair<std::string, int> &a, std::pair<std::string, int> &b) {
        return a.second > b.second;
    });
    for (size_t i = 0; i < bytecodeStatsVector.size(); ++i) {
        LOG_ECMA(ERROR) << std::right << std::setw(nameRightAdjustment) << bytecodeStatsVector[i].first
                       << std::setw(numberRightAdjustment) << bytecodeStatsVector[i].second;
    }
    LOG_ECMA(ERROR) << "============================================================"
                      << "=========================================================";
}
#endif

void EcmaVM::PrintAOTSnapShotStats()
{
    static constexpr int nameRightAdjustment = 30;
    static constexpr int numberRightAdjustment = 30;
    LOG_ECMA(ERROR) << std::right << std::setw(nameRightAdjustment) << "AOT Snapshot Genre"
                    << std::setw(numberRightAdjustment) << "Count";
    LOG_ECMA(ERROR) << "==========================================================================";
    for (const auto &iter: aotSnapShotStatsMap_) {
        LOG_ECMA(ERROR) << std::right << std::setw(nameRightAdjustment) << iter.first
                        << std::setw(numberRightAdjustment) << iter.second;
    }
    LOG_ECMA(ERROR) << "==========================================================================";
    aotSnapShotStatsMap_.clear();
}

void EcmaVM::PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo) const
{
    EcmaContext::PrintJSErrorInfo(thread_, exceptionInfo);
}

void EcmaVM::ProcessNativeDelete(const WeakRootVisitor& visitor)
{
    heap_->ProcessNativeDelete(visitor);
}

void EcmaVM::ProcessReferences(const WeakRootVisitor& visitor)
{
    heap_->ProcessReferences(visitor);
    GetPGOProfiler()->ProcessReferences(visitor);
}

void EcmaVM::PushToNativePointerList(JSNativePointer* pointer, Concurrent isConcurrent)
{
    heap_->PushToNativePointerList(pointer, isConcurrent == Concurrent::YES);
}

void EcmaVM::RemoveFromNativePointerList(JSNativePointer* pointer)
{
    heap_->RemoveFromNativePointerList(pointer);
}

void EcmaVM::PushToDeregisterModuleList(const CString &module)
{
    deregisterModuleList_.emplace_back(module);
}

void EcmaVM::RemoveFromDeregisterModuleList(CString module)
{
    auto iter = std::find(deregisterModuleList_.begin(), deregisterModuleList_.end(), module);
    if (iter != deregisterModuleList_.end()) {
        deregisterModuleList_.erase(iter);
    }
}

bool EcmaVM::ContainInDeregisterModuleList(CString module)
{
    return (std::find(deregisterModuleList_.begin(), deregisterModuleList_.end(), module)
        != deregisterModuleList_.end());
}

void EcmaVM::ClearBufferData()
{
    heap_->ClearNativePointerList();
    thread_->GetCurrentEcmaContext()->ClearBufferData();
    internalNativeMethods_.clear();
    workerList_.clear();
    deregisterModuleList_.clear();
}

void EcmaVM::CollectGarbage(TriggerGCType gcType, panda::ecmascript::GCReason reason) const
{
    heap_->CollectGarbage(gcType, reason);
}

void EcmaVM::Iterate(RootVisitor &v, VMRootVisitType type)
{
    v.VisitRangeRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&internalNativeMethods_.front())),
        ObjectSlot(ToUintPtr(&internalNativeMethods_.back()) + JSTaggedValue::TaggedTypeSize()));
    if (!WIN_OR_MAC_OR_IOS_PLATFORM && snapshotEnv_!= nullptr) {
        snapshotEnv_->Iterate(v, type);
    }
    if (pgoProfiler_ != nullptr) {
        pgoProfiler_->Iterate(v);
    }
    if (aotFileManager_) {
        aotFileManager_->Iterate(v);
    }
}

#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
void EcmaVM::DeleteHeapProfile()
{
    if (heapProfile_ == nullptr) {
        return;
    }
    delete heapProfile_;
    heapProfile_ = nullptr;
}

HeapProfilerInterface *EcmaVM::GetHeapProfile()
{
    if (heapProfile_ != nullptr) {
        return heapProfile_;
    }
    return nullptr;
}

HeapProfilerInterface *EcmaVM::GetOrNewHeapProfile()
{
    if (heapProfile_ != nullptr) {
        return heapProfile_;
    }
    heapProfile_ = new HeapProfiler(this);
    ASSERT(heapProfile_ != nullptr);
    return heapProfile_;
}

void EcmaVM::StartHeapTracking()
{
    heap_->StartHeapTracking();
}

void EcmaVM::StopHeapTracking()
{
    heap_->StopHeapTracking();
}
#endif

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
void *EcmaVM::InternalMethodTable[] = {
    reinterpret_cast<void *>(builtins::BuiltinsGlobal::CallJsBoundFunction),
    reinterpret_cast<void *>(builtins::BuiltinsGlobal::CallJsProxy),
    reinterpret_cast<void *>(builtins::BuiltinsObject::CreateDataPropertyOnObjectFunctions),
#ifdef ARK_SUPPORT_INTL
    reinterpret_cast<void *>(builtins::BuiltinsCollator::AnonymousCollator),
    reinterpret_cast<void *>(builtins::BuiltinsDateTimeFormat::AnonymousDateTimeFormat),
    reinterpret_cast<void *>(builtins::BuiltinsNumberFormat::NumberFormatInternalFormatNumber),
#endif
    reinterpret_cast<void *>(builtins::BuiltinsProxy::InvalidateProxyFunction),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::AsyncAwaitFulfilled),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::AsyncAwaitRejected),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::ResolveElementFunction),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::Resolve),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::Reject),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::Executor),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::AnyRejectElementFunction),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::AllSettledResolveElementFunction),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::AllSettledRejectElementFunction),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::ThenFinally),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::CatchFinally),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::valueThunkFunction),
    reinterpret_cast<void *>(builtins::BuiltinsPromiseHandler::throwerFunction),
    reinterpret_cast<void *>(JSAsyncGeneratorObject::ProcessorFulfilledFunc),
    reinterpret_cast<void *>(JSAsyncGeneratorObject::ProcessorRejectedFunc),
    reinterpret_cast<void *>(JSAsyncFromSyncIterator::AsyncFromSyncIterUnwarpFunction),
    reinterpret_cast<void *>(SourceTextModule::AsyncModuleFulfilledFunc),
    reinterpret_cast<void *>(SourceTextModule::AsyncModuleRejectedFunc)
};

void EcmaVM::GenerateInternalNativeMethods()
{
    size_t length = static_cast<size_t>(MethodIndex::METHOD_END);
    constexpr uint32_t numArgs = 2;  // function object and this
    for (size_t i = 0; i < length; i++) {
        auto method = factory_->NewSMethod(nullptr, MemSpaceType::SHARED_NON_MOVABLE);
        method->SetNativePointer(InternalMethodTable[i]);
        method->SetNativeBit(true);
        method->SetNumArgsWithCallField(numArgs);
        method->SetFunctionKind(FunctionKind::NORMAL_FUNCTION);
        internalNativeMethods_.emplace_back(method.GetTaggedValue());
    }
    // cache to global constants shared because context may change
    CacheToGlobalConstants(GetMethodByIndex(MethodIndex::BUILTINS_GLOBAL_CALL_JS_BOUND_FUNCTION),
                           ConstantIndex::BOUND_FUNCTION_METHOD_INDEX);
    CacheToGlobalConstants(GetMethodByIndex(MethodIndex::BUILTINS_GLOBAL_CALL_JS_PROXY),
                           ConstantIndex::PROXY_METHOD_INDEX);
}

void EcmaVM::CacheToGlobalConstants(JSTaggedValue value, ConstantIndex idx)
{
    auto thread = GetJSThread();
    auto context = thread->GetCurrentEcmaContext();
    auto constants = const_cast<GlobalEnvConstants *>(context->GlobalConstants());
    constants->SetConstant(idx, value);
}

JSTaggedValue EcmaVM::GetMethodByIndex(MethodIndex idx)
{
    auto index = static_cast<uint8_t>(idx);
    ASSERT(index < internalNativeMethods_.size());
    return internalNativeMethods_[index];
}

void EcmaVM::TriggerConcurrentCallback(JSTaggedValue result, JSTaggedValue hint)
{
    if (concurrentCallback_ == nullptr) {
        LOG_ECMA(DEBUG) << "Only trigger concurrent callback in taskpool thread";
        return;
    }

    bool success = true;
    if (result.IsJSPromise()) {
        // Async concurrent will return Promise
        auto promise = JSPromise::Cast(result.GetTaggedObject());
        auto status = promise->GetPromiseState();
        if (status == PromiseState::PENDING) {
            result = JSHandle<JSTaggedValue>::Cast(factory_->GetJSError(
                ErrorType::ERROR, "Can't return Promise in pending state", StackCheck::NO)).GetTaggedValue();
        } else {
            result = promise->GetPromiseResult();
        }

        if (status != PromiseState::FULFILLED) {
            success = false;
        }
    }

    JSHandle<JSTaggedValue> functionValue(thread_, hint);
    if (!functionValue->IsJSFunction()) {
        LOG_ECMA(ERROR) << "TriggerConcurrentCallback hint is not function";
        return;
    }
    JSHandle<JSFunction> functionInfo(functionValue);
    if (!functionInfo->GetTaskConcurrentFuncFlag()) {
        LOG_ECMA(INFO) << "Function is not Concurrent Function";
        return;
    }

    void *taskInfo = reinterpret_cast<void*>(thread_->GetTaskInfo());
    if (UNLIKELY(taskInfo == nullptr)) {
        JSTaggedValue extraInfoValue = functionInfo->GetFunctionExtraInfo();
        if (!extraInfoValue.IsJSNativePointer()) {
            LOG_ECMA(INFO) << "FunctionExtraInfo is not JSNativePointer";
            return;
        }
        JSHandle<JSNativePointer> extraInfo(thread_, extraInfoValue);
        taskInfo = extraInfo->GetData();
    }
    // clear the taskInfo when return, which can prevent the callback to get it
    thread_->SetTaskInfo(reinterpret_cast<uintptr_t>(nullptr));
    auto localResultRef = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread_, result));
    ThreadNativeScope nativeScope(thread_);
    concurrentCallback_(localResultRef, success, taskInfo, concurrentData_);
}

void EcmaVM::DumpCallTimeInfo()
{
    if (callTimer_ != nullptr) {
        callTimer_->PrintAllStats();
    }
}

void EcmaVM::WorkersetInfo(EcmaVM *workerVm)
{
    LockHolder lock(mutex_);
    auto thread = workerVm->GetJSThread();
    if (thread != nullptr) {
        auto tid = thread->GetThreadId();
        if (tid != 0) {
            workerList_.emplace(tid, workerVm);
        }
    }
}

EcmaVM *EcmaVM::GetWorkerVm(uint32_t tid)
{
    LockHolder lock(mutex_);
    EcmaVM *workerVm = nullptr;
    if (!workerList_.empty()) {
        auto iter = workerList_.find(tid);
        if (iter != workerList_.end()) {
            workerVm = iter->second;
        }
    }
    return workerVm;
}

bool EcmaVM::DeleteWorker(EcmaVM *workerVm)
{
    LockHolder lock(mutex_);
    auto thread = workerVm->GetJSThread();
    if (thread != nullptr) {
        auto tid = thread->GetThreadId();
        if (tid == 0) {
            return false;
        }
        auto iter = workerList_.find(tid);
        if (iter != workerList_.end()) {
            workerList_.erase(iter);
            return true;
        }
        return false;
    }
    return false;
}

bool EcmaVM::SuspendWorkerVm(uint32_t tid)
{
    LockHolder lock(mutex_);
    if (!workerList_.empty()) {
        auto iter = workerList_.find(tid);
        if (iter != workerList_.end()) {
            return DFXJSNApi::SuspendVM(iter->second);
        }
    }
    return false;
}

void EcmaVM::ResumeWorkerVm(uint32_t tid)
{
    LockHolder lock(mutex_);
    if (!workerList_.empty()) {
        auto iter = workerList_.find(tid);
        if (iter != workerList_.end()) {
            DFXJSNApi::ResumeVM(iter->second);
        }
    }
}

/*  This moduleName is a readOnly variable for napi, represent which abc is running in current vm.
*   Get Current recordName: bundleName/moduleName/ets/xxx/xxx
*                           pkg_modules@xxx/xxx/xxx
*   Get Current fileName: /data/storage/el1/bundle/moduleName/ets/modules.abc
*   output: moduleName: moduleName
*   if needRecordName then fileName is: moduleName/ets/modules.abc
*/
std::pair<std::string, std::string> EcmaVM::GetCurrentModuleInfo(bool needRecordName)
{
    std::pair<CString, CString> moduleInfo = EcmaInterpreter::GetCurrentEntryPoint(thread_);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, std::make_pair("", ""));
    CString recordName = moduleInfo.first;
    CString fileName = moduleInfo.second;
    LOG_FULL(INFO) << "Current recordName is " << recordName <<", current fileName is " << fileName;
    if (needRecordName) {
        if (fileName.length() > ModulePathHelper::BUNDLE_INSTALL_PATH_LEN &&
            fileName.find(ModulePathHelper::BUNDLE_INSTALL_PATH) == 0) {
            fileName = fileName.substr(ModulePathHelper::BUNDLE_INSTALL_PATH_LEN);
        } else {
            LOG_FULL(ERROR) << " GetCurrentModuleName Fail, fileName is " << fileName;
        }
        return std::make_pair(recordName.c_str(), fileName.c_str());
    }
    CString moduleName;
    if (IsNormalizedOhmUrlPack()) {
        moduleName = ModulePathHelper::GetModuleNameWithNormalizedName(recordName);
    } else {
        moduleName = ModulePathHelper::GetModuleName(recordName);
    }
    if (moduleName.empty()) {
        LOG_FULL(ERROR) << " GetCurrentModuleName Fail, recordName is " << recordName;
    }
    return std::make_pair(moduleName.c_str(), fileName.c_str());
}

void EcmaVM::SetHmsModuleList(const std::vector<panda::HmsMap> &list)
{
    for (size_t i = 0; i < list.size(); i++) {
        HmsMap hmsMap = list[i];
        hmsModuleList_.emplace(hmsMap.originalPath.c_str(), hmsMap);
    }
}

CString EcmaVM::GetHmsModule(const CString &module) const
{
    auto it = hmsModuleList_.find(module);
    if (it == hmsModuleList_.end()) {
        LOG_ECMA(FATAL) << " Get Hms Module failed";
    }
    HmsMap hmsMap = it->second;
    return hmsMap.targetPath.c_str();
}

bool EcmaVM::IsHmsModule(const CString &moduleStr) const
{
    if (hmsModuleList_.empty()) {
        return false;
    }
    auto it = hmsModuleList_.find(moduleStr);
    if (it == hmsModuleList_.end()) {
        return false;
    }
    return true;
}

void EcmaVM::SetpkgContextInfoList(const CMap<CString, CMap<CString, CVector<CString>>> &list)
{
    WriteLockHolder lock(pkgContextInfoLock_);
    pkgContextInfoList_ = list;
}

void EcmaVM::StopPreLoadSoOrAbc()
{
    if (!stopPreLoadCallbacks_.empty()) {
        for (StopPreLoadSoCallback &cb: stopPreLoadCallbacks_) {
            if (cb != nullptr) {
                cb();
            }
        }
        stopPreLoadCallbacks_.clear();
    }
}

// Initialize IcuData Path
void EcmaVM::InitializeIcuData(const JSRuntimeOptions &options)
{
    std::string icuPath = options.GetIcuDataPath();
    if (icuPath == "default") {
#if !WIN_OR_MAC_OR_IOS_PLATFORM && !defined(PANDA_TARGET_LINUX)
        SetHwIcuDirectory();
#endif
    } else {
        std::string absPath;
        if (ecmascript::RealPath(icuPath, absPath)) {
            u_setDataDirectory(absPath.c_str());
        }
    }
}

// Initialize Process StartRealTime
int EcmaVM::InitializeStartRealTime()
{
    int startRealTime = 0;
    struct timespec timespro = {0, 0};
    struct timespec timessys = {0, 0};
    auto res = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timespro);
    if (res) {
        return startRealTime;
    }
    auto res1 = clock_gettime(CLOCK_MONOTONIC, &timessys);
    if (res1) {
        return startRealTime;
    }

    int whenpro = int(timespro.tv_sec * 1000) + int(timespro.tv_nsec / 1000000);
    int whensys = int(timessys.tv_sec * 1000) + int(timessys.tv_nsec / 1000000);
    startRealTime = (whensys - whenpro);
    return startRealTime;
}

uint32_t EcmaVM::GetAsyncTaskId()
{
    return asyncStackTrace_->GetAsyncTaskId();
}

bool EcmaVM::InsertAsyncStackTrace(const JSHandle<JSPromise> &promise)
{
    return asyncStackTrace_->InsertAsyncStackTrace(promise);
}

bool EcmaVM::RemoveAsyncStackTrace(const JSHandle<JSPromise> &promise)
{
    return asyncStackTrace_->RemoveAsyncStackTrace(promise);
}
}  // namespace panda::ecmascript
