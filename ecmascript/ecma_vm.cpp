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

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/base/config.h"
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/compiler/aot_constantpool_patcher.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "common_interfaces/base_runtime.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_number_format.h"
#endif
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#if !WIN_OR_MAC_OR_IOS_PLATFORM
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#endif
#include "ecmascript/dfx/stackinfo/async_stack_trace.h"
#include "ecmascript/dfx/tracing/tracing.h"
#include "ecmascript/dfx/vmstat/function_call_timer.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/jspandafile/abc_buffer_cache.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/ohos/aot_tools.h"
#include "ecmascript/ohos/jit_tools.h"
#include "ecmascript/pgo_profiler/pgo_trace.h"
#include "ecmascript/platform/ecma_context.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/sustaining_js_handle.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/ohos/jit_tools.h"
#include "ecmascript/ohos/aot_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/dfx/stackinfo/async_stack_trace.h"
#include "ecmascript/base/gc_helper.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

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
// To find the current js thread without parameters
thread_local void *g_currentThread = nullptr;

extern "C" uintptr_t GetGlueFromThreadLocal()
{
    return reinterpret_cast<JSThread *>(g_currentThread)->GetGlueAddr();
}

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
            if (Runtime::GetInstance()->GetEnableLargeHeap()) {
                heapSize = panda::ecmascript::MAX_WORKER_HEAP_SIZE;
            }
            break;
        default:
            heapSize = OHOS::system::GetUintParameter<size_t>("persist.ark.heap.defaultsize", 0) * 1_MB;
            if (Runtime::GetInstance()->GetEnableLargeHeap()) {
                heapSize = panda::ecmascript::MAX_HEAP_SIZE;
            }
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
    g_currentThread = jsThread;
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
    Runtime::GetInstance()->PreFork(thread_);
    auto sHeap = SharedHeap::GetInstance();
    if (!g_isEnableCMCGC) {
        heap_->CompactHeapBeforeFork();
        heap_->AdjustSpaceSizeForAppSpawn();
        heap_->GetReadOnlySpace()->SetReadOnly();
        sHeap->CompactHeapBeforeFork(thread_);
    }

    // CMC-GC threads and GC Taskpool Thread should be merged together.
    heap_->DisableParallelGC();
    sHeap->DisableParallelGC(thread_);
    heap_->GetWorkManager()->FinishInPreFork();
    sHeap->GetWorkManager()->FinishInPreFork();

    SetPreForked(true);
    SetPostForked(false);
    Runtime::GetInstance()->SetPostForked(false);
    Jit::GetInstance()->PreFork();
}

void EcmaVM::PostFork(const JSRuntimeOptions &option)
{
    if (Runtime::GetInstance()->GetEnableLargeHeap()) {
        // when enable large heap, reset some heap param which has initialized in appspawn
        MemMapAllocator::GetInstance()->ResetLargePoolSize();
        SharedHeap::GetInstance()->ResetLargeCapacity();
        heap_->ResetLargeCapacity();
    }
    Runtime::GetInstance()->PostFork();
    Runtime::GetInstance()->SetPostForked(true);
    RandomGenerator::InitRandom(GetAssociatedJSThread());
    heap_->SetHeapMode(HeapMode::SHARE);
    GetAssociatedJSThread()->PostFork();
    DaemonThread::GetInstance()->StartRunning();
    common::Taskpool::GetCurrentTaskpool()->Initialize();
    heap_->GetWorkManager()->InitializeInPostFork();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetWorkManager()->InitializeInPostFork();
    SharedHeap::GetInstance()->EnableParallelGC(GetJSOptions());
    heap_->EnableParallelGC();
    SetPreForked(false);
    SetPostForked(true);

    LOG_ECMA(INFO) << "multi-thread check enabled: " << GetThreadCheckStatus();
    SignalAllReg();
    options_.SetPgoForceDump(false);
    std::string bundleName = PGOProfilerManager::GetInstance()->GetBundleName();
    pgo::PGOTrace::GetInstance()->SetEnable(ohos::AotTools::GetPgoTraceEnable());
    AotCrashInfo::GetInstance().SetOptionPGOProfiler(&options_, bundleName);
    ResetPGOProfiler();
    processStartRealtime_ = InitializeStartRealTime();

    Jit::GetInstance()->SetJitEnablePostFork(this, bundleName);
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
    int arkProperties = OHOS::system::GetIntParameter<int>("persist.ark.properties", -1);
    GetJSOptions().SetArkProperties(arkProperties);
#endif
#ifdef ENABLE_POSTFORK_FORCEEXPAND
    if (option.GetEnableWarmStartupSmartGC()) {
        LOG_ECMA(WARN) << "SmartGC: process is start by premake/preload, skip cold start smrt gc."
                       << " replace with warm startup smart gc later";
    } else {
        heap_->NotifyPostFork();
        heap_->NotifyFinishColdStartSoon();
    }
#endif
    DaemonThread::GetInstance()->EnsureRunning();
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
    isEnableCMCGC_ = g_isEnableCMCGC;
    optionalLogEnabled_ = options_.EnableOptionalLog();
    options_.ParseAsmInterOption();
    SetEnableOsr(options_.IsEnableOSR() && options_.IsEnableJIT() && options_.GetEnableAsmInterpreter());
    processStartRealtime_ = InitializeStartRealTime();
}

// for jit
EcmaVM::EcmaVM()
    : nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()),
      heapRegionAllocator_(nullptr),
      chunk_(nativeAreaAllocator_.get())
{
    isEnableCMCGC_ = g_isEnableCMCGC;
}

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
        if (g_isEnableCMCGC) {
            ThreadNativeScope scope(thread_);
            pgoProfiler_ = PGOProfilerManager::GetInstance()->BuildProfiler(this, isEnablePGOProfiler);
        } else {
            pgoProfiler_ = PGOProfilerManager::GetInstance()->BuildProfiler(this, isEnablePGOProfiler);
        }
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
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "EcmaVM::Initialize", "");
    stringTable_ = Runtime::GetInstance()->GetEcmaStringTable();
    InitializePGOProfiler();
    common::Taskpool::GetCurrentTaskpool()->Initialize();
#ifndef PANDA_TARGET_WINDOWS
    RuntimeStubs::Initialize(thread_);
#endif
    heap_ = new Heap(this);
    heap_->Initialize();
#ifdef PANDA_JS_ETS_HYBRID_MODE
    crossVMOperator_ = new CrossVMOperator(this);
#endif // PANDA_JS_ETS_HYBRID_MODE
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
    abcBufferCache_ = new AbcBufferCache();
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    globalConst->Init(thread_);
    InitDataViewTypeTable(globalConst);
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    thread_->SetReadyForGCIterating(true);
    thread_->SetSharedMarkStatus(DaemonThread::GetInstance()->GetSharedMarkStatus());
    snapshotEnv_ = new SnapshotEnv(this);
    bool builtinsLazyEnabled = GetJSOptions().IsWorker() && GetJSOptions().GetEnableBuiltinsLazy();
    thread_->SetEnableLazyBuiltins(builtinsLazyEnabled);
    JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(builtinsLazyEnabled);
    thread_->SetGlueGlobalEnv(globalEnv.GetTaggedValue());
    ptManager_ = new kungfu::PGOTypeManager(this);
    optCodeProfiler_ = new OptCodeProfiler();
    if (options_.GetTypedOpProfiler()) {
        typedOpProfiler_ = new TypedOpProfiler();
    }
    functionProtoTransitionTable_ = new FunctionProtoTransitionTable(thread_);

    unsharedConstpools_ = new(std::nothrow) JSTaggedValue[GetUnsharedConstpoolsArrayLen()];
    if (unsharedConstpools_ == nullptr) {
        LOG_ECMA(FATAL) << "allocate unshared constpool array fail during initing";
        UNREACHABLE();
    }
    std::fill(unsharedConstpools_, unsharedConstpools_ + GetUnsharedConstpoolsArrayLen(), JSTaggedValue::Hole());
    thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(unsharedConstpools_));
    thread_->SetUnsharedConstpoolsArrayLen(unsharedConstpoolsArrayLen_);

    snapshotEnv_->AddGlobalConstToMap();
    GenerateInternalNativeMethods();
    quickFixManager_ = new QuickFixManager();
    if (options_.GetEnableAsmInterpreter()) {
        LoadStubFile();
    }

    callTimer_ = new FunctionCallTimer();
    strategy_ = new ThroughputJSObjectResizingStrategy();
    SetRegisterSymbols(SymbolTable::Create(thread_).GetTaggedValue());
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    if (IsEnableFastJit() || IsEnableBaselineJit()) {
        Jit::GetInstance()->ConfigJit(this);
    }
    sustainingJSHandleList_ = new SustainingJSHandleList();
    initialized_ = true;
    regExpParserCache_ = new RegExpParserCache();
    return true;
}

EcmaVM::~EcmaVM()
{
    if (g_isEnableCMCGC) {
        thread_->GetThreadHolder()->TransferToNative();
        common::BaseRuntime::EnterGCCriticalSection();
        thread_->GetThreadHolder()->TransferToRunning();
    }
    if (isJitCompileVM_) {
        if (factory_ != nullptr) {
            chunk_.Delete(factory_);
            factory_ = nullptr;
        }
        stringTable_ = nullptr;
        thread_ = nullptr;
        if (g_isEnableCMCGC) {
            common::BaseRuntime::ExitGCCriticalSection();
        }
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
    // Destroy pgoProfiler should add after JIT::ClearTaskWithVm, and before ClearBufferData
    if (pgoProfiler_ != nullptr) {
        PGOProfilerManager::GetInstance()->Destroy(thread_, pgoProfiler_);
        pgoProfiler_ = nullptr;
    }
    // clear c_address: c++ pointer delete
    ClearBufferData();
    heap_->WaitAllTasksFinished();
    common::Taskpool::GetCurrentTaskpool()->Destroy(thread_->GetThreadId());

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    DumpCallTimeInfo();
#endif

#if defined(ECMASCRIPT_SUPPORT_TRACING)
    if (tracing_) {
        DFXJSNApi::StopTracing(this);
    }
#endif
    moduleManagers_.DestroyAllNativeObj();

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

    if (sustainingJSHandleList_ != nullptr) {
        delete sustainingJSHandleList_;
        sustainingJSHandleList_ = nullptr;
    }

#ifdef PANDA_JS_ETS_HYBRID_MODE
    if (crossVMOperator_ != nullptr) {
        delete crossVMOperator_;
        crossVMOperator_ = nullptr;
    }
#endif  // PANDA_JS_ETS_HYBRID_MODE

    if (!g_isEnableCMCGC) {
        SharedHeap *sHeap = SharedHeap::GetInstance();
        const Heap *heap = Runtime::GetInstance()->GetMainThread()->GetEcmaVM()->GetHeap();
        if (IsWorkerThread() && Runtime::SharedGCRequest()) {
            // destory workervm to release mem.
            thread_->SetReadyForGCIterating(false);
            if (sHeap->CheckCanTriggerConcurrentMarking(thread_)) {
                sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::WORKER_DESTRUCTION>(thread_);
            } else if (heap && !heap->InSensitiveStatus() && !sHeap->GetConcurrentMarker()->IsEnabled()) {
                sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::WORKER_DESTRUCTION>(thread_);
            }
        }
    }

    intlCache_.ClearIcuCache(this);

    DeleteHandleStorage();
    DeletePrimitiveStorage();

    if (runtimeStat_ != nullptr) {
        chunk_.Delete(runtimeStat_);
        runtimeStat_ = nullptr;
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

    if (unsharedConstpools_ != nullptr) {
        delete[] unsharedConstpools_;
        unsharedConstpools_ = nullptr;
        thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(nullptr));
        thread_->SetUnsharedConstpoolsArrayLen(0);
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

    if (regExpParserCache_ != nullptr) {
        delete regExpParserCache_;
        regExpParserCache_ = nullptr;
    }

    if (abcBufferCache_ != nullptr) {
        delete abcBufferCache_;
        abcBufferCache_ = nullptr;
    }

    if (optCodeProfiler_ != nullptr) {
        delete optCodeProfiler_;
        optCodeProfiler_ = nullptr;
    }

    if (typedOpProfiler_ != nullptr) {
        delete typedOpProfiler_;
        typedOpProfiler_ = nullptr;
    }

    if (ptManager_ != nullptr) {
        delete ptManager_;
        ptManager_ = nullptr;
    }

    if (aotFileManager_ != nullptr) {
        aotFileManager_ = nullptr;
    }

    if (functionProtoTransitionTable_ != nullptr) {
        delete functionProtoTransitionTable_;
        functionProtoTransitionTable_ = nullptr;
    }

    moduleManagers_.Clear();

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }

    if (g_isEnableCMCGC) {
        common::BaseRuntime::ExitGCCriticalSection();
    }
}

void EcmaVM::InitializeEcmaScriptRunStat()
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char *runtimeCallerNames[] = {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_NAME(name) "Interpreter::" #name,
    INTERPRETER_CALLER_LIST(INTERPRETER_CALLER_NAME)  // NOLINTNEXTLINE(bugprone-suspicious-missing-comma)
#undef INTERPRETER_CALLER_NAME
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_NAME(class, name) "BuiltinsApi::" #class "_" #name,
    BUILTINS_API_LIST(BUILTINS_API_NAME)
#undef BUILTINS_API_NAME
#define ABSTRACT_OPERATION_NAME(class, name) "AbstractOperation::" #class "_" #name,
    ABSTRACT_OPERATION_LIST(ABSTRACT_OPERATION_NAME)
#undef ABSTRACT_OPERATION_NAME
#define MEM_ALLOCATE_AND_GC_NAME(name) "Memory::" #name,
    MEM_ALLOCATE_AND_GC_LIST(MEM_ALLOCATE_AND_GC_NAME)
#undef MEM_ALLOCATE_AND_GC_NAME
#define DEF_RUNTIME_ID(name) "Runtime::" #name,
    RUNTIME_STUB_WITH_GC_LIST(DEF_RUNTIME_ID)
    RUNTIME_STUB_WITH_DFX(DEF_RUNTIME_ID)
#undef DEF_RUNTIME_ID
    };
    static_assert(sizeof(runtimeCallerNames) == sizeof(const char *) * ecmascript::RUNTIME_CALLER_NUMBER,
                  "Invalid runtime caller number");
    runtimeStat_ = chunk_.New<EcmaRuntimeStat>(runtimeCallerNames, ecmascript::RUNTIME_CALLER_NUMBER);
    if (UNLIKELY(runtimeStat_ == nullptr)) {
        LOG_FULL(FATAL) << "alloc runtimeStat_ failed";
        UNREACHABLE();
    }
}

void EcmaVM::SetRuntimeStatEnable(bool flag)
{
    static uint64_t start = 0;
    if (flag) {
        start = PandaRuntimeTimer::Now();
        if (runtimeStat_ == nullptr) {
            InitializeEcmaScriptRunStat();
        }
    } else {
        LOG_ECMA(INFO) << "Runtime State duration:" << PandaRuntimeTimer::Now() - start << "(ns)";
        if (runtimeStat_ != nullptr && runtimeStat_->IsRuntimeStatEnabled()) {
            runtimeStat_->Print();
            runtimeStat_->ResetAllCount();
        }
    }
    if (runtimeStat_ != nullptr) {
        runtimeStat_->SetRuntimeStatEnabled(flag);
    }
}

void EcmaVM::CheckThread() const
{
    // Exclude GC thread
    if (thread_ == nullptr) {
        LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
        UNREACHABLE();
    }
    DaemonThread *dThread = DaemonThread::GetInstance();
    if (!common::Taskpool::GetCurrentTaskpool()->IsInThreadPool(std::this_thread::get_id()) &&
        !(dThread != nullptr && dThread->GetThreadId() == JSThread::GetCurrentThreadId()) &&
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
    // When C++ enters ASM, save the current globalenv and restore to glue after call
    SaveEnv envScope(thread_);
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_OptimizedFastCallEntry);
    if (g_isEnableCMCGC) {
        base::GCHelper::CopyCallTarget(thread_, (void *)args[0]);
    }
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

void EcmaVM::ProcessNativeDelete(const WeakRootVisitor& visitor)
{
    heap_->ProcessNativeDelete(visitor);
}

void EcmaVM::ProcessReferences(const WeakRootVisitor& visitor)
{
    heap_->ProcessReferences(visitor);
    GetPGOProfiler()->ProcessReferences(visitor);
}

void EcmaVM::ProcessSnapShotEnv(const WeakRootVisitor& visitor)
{
    GetSnapshotEnv()->ProcessSnapShotEnv(visitor);
}

// for cmc-gc
void EcmaVM::IteratorSnapShotEnv(WeakVisitor & visitor)
{
    GetSnapshotEnv()->IteratorSnapShotEnv(visitor);
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
    ClearConstpoolBufferData();
    internalNativeMethods_.clear();
    workerList_.clear();
    deregisterModuleList_.clear();
}

void EcmaVM::CollectGarbage(TriggerGCType gcType, panda::ecmascript::GCReason reason) const
{
    if (g_isEnableCMCGC) {
        common::GCReason cmcReason = common::GC_REASON_USER;
        bool async = true;
        if (gcType == TriggerGCType::FULL_GC || gcType == TriggerGCType::SHARED_FULL_GC ||
            gcType == TriggerGCType::APPSPAWN_FULL_GC || gcType == TriggerGCType::APPSPAWN_SHARED_FULL_GC ||
            reason == GCReason::ALLOCATION_FAILED) {
            cmcReason = common::GC_REASON_BACKUP;
            async = false;
        }
        common::BaseRuntime::RequestGC(cmcReason, async, common::GC_TYPE_FULL);
        return;
    }
    heap_->CollectGarbage(gcType, reason);
}

void EcmaVM::IterateConcurrentRoots(RootVisitor &v)
{
    ASSERT(g_isEnableCMCGC);
    moduleManagers_.Iterate(v);
}

void EcmaVM::Iterate(RootVisitor &v)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "CMCGC::VisitRootEcmaVM", "");
    if (!internalNativeMethods_.empty()) {
        v.VisitRangeRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&internalNativeMethods_.front())),
            ObjectSlot(ToUintPtr(&internalNativeMethods_.back()) + JSTaggedValue::TaggedTypeSize()));
    }
    if (pgoProfiler_ != nullptr) {
        pgoProfiler_->Iterate(v);
    }
    if (aotFileManager_) {
        aotFileManager_->Iterate(v);
    }
    if (!microJobQueue_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&microJobQueue_)));
    }
    if (!registerSymbols_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&registerSymbols_)));
    }
    if (!finRegLists_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&finRegLists_)));
    }
    if (!options_.EnableGlobalLeakCheck() && currentHandleStorageIndex_ != -1) {
        // IterateHandle when disableGlobalLeakCheck.
        DISALLOW_HANDLE_ALLOC;
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "CMCGC::VisitRootHandleStorage", "");
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            v.VisitRangeRoot(Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
        }
    }

    if (regExpParserCache_ != nullptr) {
        regExpParserCache_->Clear();
    }
    // GC maybe happen before vm initialized.
    if (unsharedConstpools_ != nullptr) {
        v.VisitRangeRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(unsharedConstpools_)),
            ObjectSlot(ToUintPtr(&unsharedConstpools_[GetUnsharedConstpoolsArrayLen() - 1]) +
            JSTaggedValue::TaggedTypeSize()));
    }
    if (sustainingJSHandleList_) {
        sustainingJSHandleList_->Iterate(v);
    }

    if (functionProtoTransitionTable_) {
        functionProtoTransitionTable_->Iterate(v);
    }

    if (ptManager_) {
        ptManager_->Iterate(v);
    }
#ifdef ARK_USE_SATB_BARRIER
    auto iterator = cachedSharedConstpools_.begin();
    while (iterator != cachedSharedConstpools_.end()) {
        auto &constpools = iterator->second;
        auto constpoolIter = constpools.begin();
        while (constpoolIter != constpools.end()) {
            JSTaggedValue constpoolVal = constpoolIter->second;
            if (constpoolVal.IsHeapObject()) {
                v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&constpoolIter->second)));
            }
            ++constpoolIter;
        }
        ++iterator;
    }
#endif
    if (!g_isEnableCMCGC) {
        moduleManagers_.Iterate(v);
    }
}

size_t EcmaVM::IterateHandle(RootVisitor &visitor)
{
    // EnableGlobalLeakCheck.
    size_t handleCount = 0;
    if (currentHandleStorageIndex_ != -1) {
        DISALLOW_HANDLE_ALLOC;
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            visitor.VisitRangeRoot(Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
            handleCount += (ToUintPtr(end) - ToUintPtr(start)) / sizeof(JSTaggedType);
        }
    }
    return handleCount;
}

void EcmaVM::IterateWeakGlobalEnvList(WeakVisitor &visitor)
{
    for (auto it = globalEnvRecordList_.begin(); it != globalEnvRecordList_.end();) {
        bool isAlive = visitor.VisitRoot(Root::ROOT_VM, ecmascript::ObjectSlot(static_cast<uintptr_t>(*it)));
        if (isAlive) {
            it++;
        } else {
            it = globalEnvRecordList_.erase(it);
        }
    }
}

void EcmaVM::IterateGlobalEnvField(RootVisitor &visitor)
{
    for (auto value : globalEnvRecordList_) {
        GlobalEnv::Cast(JSTaggedValue(value).GetTaggedObject())->Iterate(visitor);
    }
}

uintptr_t *EcmaVM::ExpandHandleStorage()
{
    uintptr_t *result = nullptr;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size()) - 1;
    if (currentHandleStorageIndex_ == lastIndex) {
        auto n = new std::array<JSTaggedType, NODE_BLOCK_SIZE>();
        handleStorageNodes_.push_back(n);
        currentHandleStorageIndex_++;
        result = reinterpret_cast<uintptr_t *>(&n->data()[0]);
        handleScopeStorageEnd_ = &n->data()[NODE_BLOCK_SIZE];
    } else {
        currentHandleStorageIndex_++;
        auto lastNode = handleStorageNodes_[currentHandleStorageIndex_];
        result = reinterpret_cast<uintptr_t *>(&lastNode->data()[0]);
        handleScopeStorageEnd_ = &lastNode->data()[NODE_BLOCK_SIZE];
    }

    return result;
}

void EcmaVM::DeleteHandleStorage()
{
    for (auto n : handleStorageNodes_) {
        delete n;
    }
    handleStorageNodes_.clear();
    currentHandleStorageIndex_ = -1;
    handleScopeStorageNext_ = handleScopeStorageEnd_ = nullptr;
}

void EcmaVM::ShrinkHandleStorage(int prevIndex)
{
    currentHandleStorageIndex_ = prevIndex;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size()) - 1;
#if ECMASCRIPT_ENABLE_ZAP_MEM
    uintptr_t size = ToUintPtr(handleScopeStorageEnd_) - ToUintPtr(handleScopeStorageNext_);
    if (currentHandleStorageIndex_ != -1) {
        if (memset_s(handleScopeStorageNext_, size, 0, size) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
    for (int32_t i = currentHandleStorageIndex_ + 1; i < lastIndex; i++) {
        if (memset_s(handleStorageNodes_[i],
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType), 0,
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType)) !=
                     EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
#endif

    if (lastIndex > MIN_HANDLE_STORAGE_SIZE && currentHandleStorageIndex_ < MIN_HANDLE_STORAGE_SIZE) {
        for (int i = MIN_HANDLE_STORAGE_SIZE; i < lastIndex; i++) {
            auto node = handleStorageNodes_.back();
            delete node;
            handleStorageNodes_.pop_back();
        }
    }
}

void EcmaVM::PrintOptStat()
{
    if (optCodeProfiler_ != nullptr) {
        optCodeProfiler_->PrintAndReset();
    }
}

void EcmaVM::DumpAOTInfo() const
{
    aotFileManager_->DumpAOTInfo();
}

std::tuple<uint64_t, uint8_t*, int, kungfu::CalleeRegAndOffsetVec> EcmaVM::CalCallSiteInfo(uintptr_t retAddr,
                                                                                           bool isDeopt) const
{
    return aotFileManager_->CalCallSiteInfo(retAddr, isDeopt);
}

void EcmaVM::LoadStubFile()
{
    std::string stubFile = "";
    if (options_.WasStubFileSet()) {
        stubFile = options_.GetStubFile();
    }
    aotFileManager_->LoadStubFile(stubFile);
}

bool EcmaVM::LoadAOTFilesInternal(const std::string& aotFileName)
{
#ifdef AOT_ESCAPE_ENABLE
    std::string bundleName = pgo::PGOProfilerManager::GetInstance()->GetBundleName();
    if (AotCrashInfo::GetInstance().IsAotEscapedOrNotInEnableList(this, bundleName)) {
        return false;
    }
#endif
    std::string anFile = aotFileName + AOTFileManager::FILE_EXTENSION_AN;
    if (!aotFileManager_->LoadAnFile(anFile)) {
        LOG_ECMA(WARN) << "Load " << anFile << " failed. Destroy aot data and rollback to interpreter";
        ecmascript::AnFileDataManager::GetInstance()->SafeDestroyAnData(anFile);
        return false;
    }

    std::string aiFile = aotFileName + AOTFileManager::FILE_EXTENSION_AI;
    if (!aotFileManager_->LoadAiFile(aiFile)) {
        LOG_ECMA(WARN) << "Load " << aiFile << " failed. Destroy aot data and rollback to interpreter";
        ecmascript::AnFileDataManager::GetInstance()->SafeDestroyAnData(anFile);
        return false;
    }
    return true;
}

bool EcmaVM::LoadAOTFiles(const std::string& aotFileName)
{
    return LoadAOTFilesInternal(aotFileName);
}

bool EcmaVM::LoadAOTFiles(const std::string& aotFileName,
    std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb)
{
    GetAOTFileManager()->SetJsAotReader(cb);
    return LoadAOTFilesInternal(aotFileName);
}

void EcmaVM::LoadProtoTransitionTable(JSTaggedValue constpool)
{
    JSTaggedValue protoTransitionTable =
        ConstantPool::Cast(constpool.GetTaggedObject())->GetProtoTransTableInfo(thread_);
    functionProtoTransitionTable_->UpdateProtoTransitionTable(
        thread_, JSHandle<PointerToIndexDictionary>(thread_, protoTransitionTable));
}

void EcmaVM::ResetProtoTransitionTableOnConstpool(JSTaggedValue constpool)
{
    ConstantPool::Cast(constpool.GetTaggedObject())->SetProtoTransTableInfo(thread_, JSTaggedValue::Undefined());
}

uintptr_t *EcmaVM::ExpandPrimitiveStorage()
{
    uintptr_t *result = nullptr;
    int32_t lastIndex = static_cast<int32_t>(primitiveStorageNodes_.size()) - 1;
    if (currentPrimitiveStorageIndex_ == lastIndex) {
        auto n = new std::array<JSTaggedType, NODE_BLOCK_SIZE>();
        primitiveStorageNodes_.push_back(n);
        currentPrimitiveStorageIndex_++;
        result = reinterpret_cast<uintptr_t *>(&n->data()[0]);
        primitiveScopeStorageEnd_ = &n->data()[NODE_BLOCK_SIZE];
    } else {
        currentPrimitiveStorageIndex_++;
        auto lastNode = primitiveStorageNodes_[currentPrimitiveStorageIndex_];
        result = reinterpret_cast<uintptr_t *>(&lastNode->data()[0]);
        primitiveScopeStorageEnd_ = &lastNode->data()[NODE_BLOCK_SIZE];
    }

    return result;
}

void EcmaVM::ShrinkPrimitiveStorage(int prevIndex)
{
    currentPrimitiveStorageIndex_ = prevIndex;
    int32_t lastIndex = static_cast<int32_t>(primitiveStorageNodes_.size()) - 1;
#if ECMASCRIPT_ENABLE_ZAP_MEM
    uintptr_t size = ToUintPtr(primitiveScopeStorageEnd_) - ToUintPtr(primitiveScopeStorageNext_);
    if (currentPrimitiveStorageIndex_ != -1) {
        if (memset_s(primitiveScopeStorageNext_, size, 0, size) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
    for (int32_t i = currentPrimitiveStorageIndex_ + 1; i < lastIndex; i++) {
        if (memset_s(primitiveStorageNodes_[i],
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType), 0,
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType)) !=
                     EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
#endif

    if (lastIndex > MIN_PRIMITIVE_STORAGE_SIZE && currentPrimitiveStorageIndex_ < MIN_PRIMITIVE_STORAGE_SIZE) {
        for (int i = MIN_PRIMITIVE_STORAGE_SIZE; i < lastIndex; i++) {
            auto node = primitiveStorageNodes_.back();
            delete node;
            primitiveStorageNodes_.pop_back();
        }
    }
}

void EcmaVM::DeletePrimitiveStorage()
{
    for (auto n : primitiveStorageNodes_) {
        delete n;
    }
    primitiveStorageNodes_.clear();
    currentPrimitiveStorageIndex_ = -1;
    primitiveScopeStorageNext_ = primitiveScopeStorageEnd_ = nullptr;
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
    auto constants = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
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
            result = promise->GetPromiseResult(thread_);
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
        JSTaggedValue extraInfoValue = functionInfo->GetFunctionExtraInfo(thread_);
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

void EcmaVM::HandleUncatchableError()
{
    if (uncatchableErrorHandler_ != nullptr) {
        panda::TryCatch trycatch(this);
        ecmascript::ThreadNativeScope nativeScope(thread_);
        uncatchableErrorHandler_(trycatch);
    }
    LOG_ECMA_MEM(FATAL) << "Out of Memory";
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

JSHandle<job::MicroJobQueue> EcmaVM::GetMicroJobQueue() const
{
    return JSHandle<job::MicroJobQueue>(reinterpret_cast<uintptr_t>(&microJobQueue_));
}

void EcmaVM::SetMicroJobQueue(job::MicroJobQueue *queue)
{
    ASSERT(queue != nullptr);
    microJobQueue_ = JSTaggedValue(queue);
}

bool EcmaVM::HasPendingJob() const
{
    // This interface only determines whether PromiseJobQueue is empty, rather than ScriptJobQueue.
    if (UNLIKELY(thread_->HasTerminated())) {
        return false;
    }
    TaggedQueue *promiseQueue = TaggedQueue::Cast(GetMicroJobQueue()->GetPromiseJobQueue(thread_).GetTaggedObject());
    return !promiseQueue->Empty(thread_);
}

bool EcmaVM::ExecutePromisePendingJob()
{
    if (isProcessingPendingJob_) {
        LOG_ECMA(DEBUG) << "EcmaVM::ExecutePromisePendingJob can not reentrant";
        return false;
    }
    if (!thread_->HasPendingException()) {
        isProcessingPendingJob_ = true;
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
        if (thread_->HasPendingException()) {
            JsStackInfo::BuildCrashInfo(thread_);
        }
        isProcessingPendingJob_ = false;
        return true;
    }
    return false;
}
// just find unshared constpool, not create
JSTaggedValue EcmaVM::FindUnsharedConstpool(JSTaggedValue sharedConstpool)
{
    ConstantPool *shareCp = ConstantPool::Cast(sharedConstpool.GetTaggedObject());
    int32_t constpoolIndex = shareCp->GetUnsharedConstpoolIndex();
    // unshared constpool index is default INT32_MAX.
    ASSERT(0 <= constpoolIndex && constpoolIndex != ConstantPool::CONSTPOOL_TYPE_FLAG);
    if (constpoolIndex >= GetUnsharedConstpoolsArrayLen()) {
        return JSTaggedValue::Hole();
    }
    return unsharedConstpools_[constpoolIndex];
}

JSTaggedValue EcmaVM::FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool)
{
    JSTaggedValue unsharedConstpool = FindUnsharedConstpool(sharedConstpool);
    if (unsharedConstpool.IsHole()) {
        ConstantPool *shareCp = ConstantPool::Cast(sharedConstpool.GetTaggedObject());
        int32_t constpoolIndex = shareCp->GetUnsharedConstpoolIndex();
        // unshared constpool index is default INT32_MAX.
        ASSERT(0 <= constpoolIndex && constpoolIndex != INT32_MAX);
        JSHandle<ConstantPool> unshareCp = ConstantPool::CreateUnSharedConstPoolBySharedConstpool(
            thread_->GetEcmaVM(), shareCp->GetJSPandaFile(), shareCp);
        unsharedConstpool = unshareCp.GetTaggedValue();
        SetUnsharedConstpool(constpoolIndex, unsharedConstpool);
    }
    return unsharedConstpool;
}

void EcmaVM::EraseUnusedConstpool(const JSPandaFile *jsPandaFile, int32_t index, int32_t constpoolIndex)
{
    // unshared constpool index is default INT32_MAX.
    ASSERT(0 <= constpoolIndex && constpoolIndex < GetUnsharedConstpoolsArrayLen());

    SetUnsharedConstpool(constpoolIndex, JSTaggedValue::Hole());
    auto iter = cachedSharedConstpools_.find(jsPandaFile);
    if (iter == cachedSharedConstpools_.end()) {
        return;
    }
    auto constpoolIter = iter->second.find(index);
    if (constpoolIter == iter->second.end()) {
        return;
    }

    iter->second.erase(constpoolIter);
    if (iter->second.size() == 0) {
        cachedSharedConstpools_.erase(iter);
    }
}

JSTaggedValue EcmaVM::FindConstpoolFromContextCache(const JSPandaFile *jsPandaFile, int32_t index)
{
    auto iter = cachedSharedConstpools_.find(jsPandaFile);
    if (iter != cachedSharedConstpools_.end()) {
        auto constpoolIter = iter->second.find(index);
        if (constpoolIter != iter->second.end()) {
            return constpoolIter->second;
        }
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue EcmaVM::FindConstpool(const JSPandaFile *jsPandaFile, int32_t index)
{
    JSTaggedValue contextCache = FindConstpoolFromContextCache(jsPandaFile, index);
    if (!contextCache.IsHole()) {
        return contextCache;
    }
    return Runtime::GetInstance()->FindConstpool(jsPandaFile, index);
}

// For new version instruction.
JSTaggedValue EcmaVM::FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    return FindConstpool(jsPandaFile, index);
}

bool EcmaVM::HasCachedConstpool(const JSPandaFile *jsPandaFile) const
{
    if (cachedSharedConstpools_.find(jsPandaFile) != cachedSharedConstpools_.end()) {
        return true;
    }

    return Runtime::GetInstance()->HasCachedConstpool(jsPandaFile);
}

JSHandle<ConstantPool> EcmaVM::AddOrUpdateConstpool(const JSPandaFile *jsPandaFile,
                                                    JSHandle<ConstantPool> constpool,
                                                    int32_t index)
{
    constpool = Runtime::GetInstance()->AddOrUpdateConstpool(jsPandaFile, constpool, index);
    AddContextConstpoolCache(jsPandaFile, constpool, index);
    return constpool;
}

void EcmaVM::AddContextConstpoolCache(const JSPandaFile *jsPandaFile,
                                      JSHandle<ConstantPool> constpool,
                                      int32_t index)
{
    if (cachedSharedConstpools_.find(jsPandaFile) == cachedSharedConstpools_.end()) {
        cachedSharedConstpools_[jsPandaFile] = CMap<int32_t, JSTaggedValue>();
    }
    auto &constpoolMap = cachedSharedConstpools_[jsPandaFile];
    ASSERT(constpoolMap.find(index) == constpoolMap.end());
    constpoolMap.insert({index, constpool.GetTaggedValue()});
}

void EcmaVM::SetUnsharedConstpool(JSHandle<ConstantPool> sharedConstpool, JSTaggedValue unsharedConstpool)
{
    int32_t constpoolIndex = sharedConstpool->GetUnsharedConstpoolIndex();
    SetUnsharedConstpool(constpoolIndex, unsharedConstpool);
}

void EcmaVM::SetUnsharedConstpool(int32_t constpoolIndex, JSTaggedValue unsharedConstpool)
{
    GrowUnsharedConstpoolArray(constpoolIndex);
    ASSERT(0 <= constpoolIndex && constpoolIndex < ConstantPool::CONSTPOOL_TYPE_FLAG);
    unsharedConstpools_[constpoolIndex] = unsharedConstpool;
}

void EcmaVM::GrowUnsharedConstpoolArray(int32_t index)
{
    if (index == ConstantPool::CONSTPOOL_TYPE_FLAG) {
        LOG_ECMA(FATAL) << "index has exceed unshared constpool array limit";
        UNREACHABLE();
    }
    int32_t oldCapacity = GetUnsharedConstpoolsArrayLen();
    if (index >= oldCapacity) {
        int32_t minCapacity = index + 1;
        ResizeUnsharedConstpoolArray(oldCapacity, minCapacity);
    }
}

void EcmaVM::ResizeUnsharedConstpoolArray(int32_t oldCapacity, int32_t minCapacity)
{
    int32_t newCapacity = oldCapacity * 2; // 2: Double the value
    if (newCapacity - minCapacity < 0) {
        newCapacity = minCapacity;
    }

    if (newCapacity >= (INT32_MAX >> 1)) {
        newCapacity = INT32_MAX;
    }

    JSTaggedValue *newUnsharedConstpools = new(std::nothrow) JSTaggedValue[newCapacity];
    if (newUnsharedConstpools == nullptr) {
        LOG_ECMA(FATAL) << "allocate unshared constpool array fail during resizing";
        UNREACHABLE();
    }
    std::fill(newUnsharedConstpools, newUnsharedConstpools + newCapacity, JSTaggedValue::Hole());
    int32_t copyLen = GetUnsharedConstpoolsArrayLen();
    if (g_isEnableCMCGC) {
        Barriers::CopyObject<true, true>(thread_, nullptr, newUnsharedConstpools, unsharedConstpools_, copyLen);
    } else {
        std::copy(unsharedConstpools_, unsharedConstpools_ + copyLen, newUnsharedConstpools);
    }
    ClearUnsharedConstpoolArray();
    unsharedConstpools_ = newUnsharedConstpools;
    thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(unsharedConstpools_));
    thread_->SetUnsharedConstpoolsArrayLen(newCapacity);
    SetUnsharedConstpoolsArrayLen(newCapacity);
}

void EcmaVM::UpdateConstpoolWhenDeserialAI(const std::string& fileName,
                                           JSHandle<ConstantPool> aiCP, int32_t index)
{
    auto pf = JSPandaFileManager::GetInstance()->FindJSPandaFile(fileName.c_str());
    if (pf == nullptr) {
        return;
    }
    JSTaggedValue sharedConstpool = FindConstpool(pf.get(), index);
    JSHandle<ConstantPool> sharedCPHandle = JSHandle<ConstantPool>(thread_, sharedConstpool);
    if (sharedConstpool.IsHole()) {
        return;
    }
    JSTaggedValue unsharedConstpool = FindOrCreateUnsharedConstpool(sharedCPHandle.GetTaggedValue());
    JSHandle<ConstantPool> unsharedCP = JSHandle<ConstantPool>(thread_, unsharedConstpool);
    JSHandle<ConstantPool> sharedCP = JSHandle<ConstantPool>(thread_, sharedCPHandle.GetTaggedValue());
    ConstantPool::UpdateConstpoolWhenDeserialAI(thread_->GetEcmaVM(), aiCP, sharedCP, unsharedCP);
}

JSTaggedValue EcmaVM::FindCachedConstpoolAndLoadAiIfNeeded(const JSPandaFile *jsPandaFile, int32_t index)
{
    JSTaggedValue constpool = FindConstpoolFromContextCache(jsPandaFile, index);
    if (!constpool.IsHole()) {
        return constpool;
    }
    constpool = Runtime::GetInstance()->FindConstpool(jsPandaFile, index);
    if (!constpool.IsHole()) {
        AddContextConstpoolCache(jsPandaFile, JSHandle<ConstantPool>(thread_, constpool), index);
    }
    // Getting the cached constpool in runtime means the ai data has not been loaded in current thread.
    // And we need to reload it
    aotFileManager_->LoadAiFile(jsPandaFile, thread_->GetEcmaVM());
    return constpool;
}

JSHandle<ConstantPool> EcmaVM::FindOrCreateConstPool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    EcmaVM *vm = thread_->GetEcmaVM();
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    JSTaggedValue constpool = FindCachedConstpoolAndLoadAiIfNeeded(jsPandaFile, index);
    if (constpool.IsHole()) {
        JSHandle<ConstantPool> newConstpool = ConstantPool::CreateUnSharedConstPool(vm, jsPandaFile, id);
        JSHandle<ConstantPool> newSConstpool;
        if (jsPandaFile->IsLoadedAOT()) {
            AotConstantpoolPatcher::SetObjectFunctionFromConstPool(thread_, newConstpool);
            newSConstpool = ConstantPool::CreateSharedConstPoolForAOT(vm, newConstpool, index);
        } else {
            newSConstpool = ConstantPool::CreateSharedConstPool(vm, jsPandaFile, id, index);
        }
        newSConstpool = AddOrUpdateConstpool(jsPandaFile, newSConstpool, index);
        SetUnsharedConstpool(newSConstpool, newConstpool.GetTaggedValue());
        return newSConstpool;
    } else if (jsPandaFile->IsLoadedAOT()) {
        // For aot, after getting the cached shared constpool,
        // worker thread need to create and bind the correspoding unshared constpool.
        JSHandle<ConstantPool> newConstpool = JSHandle<ConstantPool>(thread_, FindOrCreateUnsharedConstpool(constpool));
        AotConstantpoolPatcher::SetObjectFunctionFromConstPool(thread_, newConstpool);
    }
    return JSHandle<ConstantPool>(thread_, constpool);
}

void EcmaVM::CreateAllConstpool(const JSPandaFile *jsPandaFile)
{
    auto headers = jsPandaFile->GetPandaFile()->GetIndexHeaders();
    uint32_t index = 0;
    for (const auto &header : headers) {
        auto constpoolSize = header.method_idx_size;
        JSHandle<ConstantPool> sconstpool = factory_->NewSConstantPool(constpoolSize);
        sconstpool->SetJSPandaFile(jsPandaFile);
        sconstpool->SetIndexHeader(&header);
        sconstpool->SetSharedConstpoolId(JSTaggedValue(index));
        sconstpool = AddOrUpdateConstpool(jsPandaFile, sconstpool, index);
        index++;

        JSHandle<ConstantPool> constpool = factory_->NewConstantPool(constpoolSize);
        constpool->SetJSPandaFile(jsPandaFile);
        constpool->SetIndexHeader(&header);
        SetUnsharedConstpool(sconstpool, constpool.GetTaggedValue());
    }
}

void EcmaVM::ClearConstpoolBufferData()
{
    cachedSharedConstpools_.clear();
    thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(nullptr));
    thread_->SetUnsharedConstpoolsArrayLen(0);
}

std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> EcmaVM::FindConstpools(
    const JSPandaFile *jsPandaFile)
{
    return Runtime::GetInstance()->FindConstpools(jsPandaFile);
}

void EcmaVM::AddSustainingJSHandle(SustainingJSHandle *sustainingHandle)
{
    if (sustainingJSHandleList_) {
        sustainingJSHandleList_->AddSustainingJSHandle(sustainingHandle);
    }
}

void EcmaVM::RemoveSustainingJSHandle(SustainingJSHandle *sustainingHandle)
{
    if (sustainingJSHandleList_) {
        sustainingJSHandleList_->RemoveSustainingJSHandle(sustainingHandle);
    }
}

JSTaggedValue EcmaVM::InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                              const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                              CJSInfo* cjsInfo)
{
    aotFileManager_->SetAOTMainFuncEntry(mainFunc, jsPandaFile, entryPoint);
    return JSFunction::InvokeOptimizedEntrypoint(thread_, mainFunc, thisArg, cjsInfo);
}

JSTaggedValue EcmaVM::ExecuteAot(size_t actualNumArgs, JSTaggedType *args,
                                 const JSTaggedType *prevFp, bool needPushArgv)
{
    INTERPRETER_TRACE(thread_, ExecuteAot);
    ASSERT(thread_->IsInManagedState());
    // When C++ enters ASM, save the current globalenv and restore to glue after call
    SaveEnv envScope(thread_);
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    // entry of aot
    auto res = reinterpret_cast<JSFunctionEntryType>(entry)(thread_->GetGlueAddr(),
                                                            actualNumArgs,
                                                            args,
                                                            reinterpret_cast<uintptr_t>(prevFp),
                                                            needPushArgv);
    return res;
}

Expected<JSTaggedValue, bool> EcmaVM::CommonInvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
    std::string_view entryPoint, JSHandle<JSFunction> &func, const ExecuteTypes &executeType)
{
    ASSERT(thread_->IsInManagedState());
    JSHandle<JSTaggedValue> global = GetGlobalEnv()->GetJSGlobalObject();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    if (IsEnablePGOProfiler()) {
        JSHandle<JSFunction> objectFunction(GetGlobalEnv()->GetObjectFunction());
        JSHandle<JSHClass> protoOrHClass(GetGlobalEnv()->GetObjectFunctionNapiClass());
        GetPGOProfiler()->ProfileNapiRootHClass(
            objectFunction.GetTaggedType(), protoOrHClass.GetTaggedType(), pgo::ProfileType::Kind::NapiId);
    }
    CString entry = entryPoint.data();
    JSRecordInfo *recordInfo = jsPandaFile->CheckAndGetRecordInfo(entry);
    if (recordInfo == nullptr) {
        CString msg = "Cannot find module '" + entry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread_, msg.c_str(), Unexpected(false));
    }

    ModuleLogger *moduleLogger = thread_->GetModuleLogger();
    if (moduleLogger != nullptr) {
        moduleLogger->SetStartTime(entry);
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        global = undefined;
        CString moduleName = jsPandaFile->GetJSPandaFileDesc();
        if (!jsPandaFile->IsBundlePack()) {
            moduleName = entry;
        }
        JSHandle<SourceTextModule> module;
        if (jsPandaFile->IsSharedModule(recordInfo)) {
            module = SharedModuleManager::GetInstance()->GetSModule(thread_, entry);
        } else {
            module = thread_->GetModuleManager()->HostGetImportedModule(moduleName);
        }
        // esm -> SourceTextModule; cjs or script -> string of recordName
        module->SetSendableEnv(thread_, JSTaggedValue::Undefined());
        func->SetModule(thread_, module);
    } else {
        // if it is Cjs at present, the module slot of the function is not used. We borrow it to store the recordName,
        // which can avoid the problem of larger memory caused by the new slot
        JSHandle<EcmaString> recordName = factory_->NewFromUtf8(entry);
        func->SetModule(thread_, recordName);
    }
    CheckStartCpuProfiler();

    JSTaggedValue result;
    if (jsPandaFile->IsCjs(recordInfo)) {
        CJSExecution(func, global, jsPandaFile, entryPoint);
        if (moduleLogger != nullptr) {
            moduleLogger->SetEndTime(entry);
        }
    } else {
        if (aotFileManager_->IsLoadMain(jsPandaFile, entry)) {
            EcmaRuntimeStatScope runtimeStatScope(this);
            result = InvokeEcmaAotEntrypoint(func, global, jsPandaFile, entryPoint);
        } else if (GetJSOptions().IsEnableForceJitCompileMain()) {
            Jit::Compile(this, func, CompilerTier::Tier::FAST);
            EcmaRuntimeStatScope runtimeStatScope(this);
            result = JSFunction::InvokeOptimizedEntrypoint(thread_, func, global, nullptr);
        } else if (GetJSOptions().IsEnableForceBaselineCompileMain()) {
            Jit::Compile(this, func, CompilerTier::Tier::BASELINE);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(this);
            result = EcmaInterpreter::Execute(info);
        } else {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(this);
            result = EcmaInterpreter::Execute(info);
        }
        if (moduleLogger != nullptr) {
            moduleLogger->SetEndTime(entry);
        }

        if (!thread_->HasPendingException() && IsStaticImport(executeType)) {
            JSHandle<JSTaggedValue> handleResult(thread_, result);
            job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
            result = handleResult.GetTaggedValue();
        }
    }

    if (thread_->HasPendingException()) {
        return GetPendingExceptionResult(result);
    }
    return result;
}

Expected<JSTaggedValue, bool> EcmaVM::InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
                                                           std::string_view entryPoint,
                                                           const ExecuteTypes &executeType)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    auto &options = const_cast<EcmaVM *>(thread_->GetEcmaVM())->GetJSOptions();
    if (options.EnableModuleLog()) {
        LOG_FULL(INFO) << "current executing file's name " << entryPoint.data();
    }

    JSHandle<Program> program = JSPandaFileManager::GetInstance()->GenerateProgram(this, jsPandaFile, entryPoint);
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(false);
    }
    // for debugger
    GetJsDebuggerManager()->GetNotificationManager()->LoadModuleEvent(
        jsPandaFile->GetJSPandaFileDesc(), entryPoint);

    JSHandle<JSFunction> func(thread_, program->GetMainFunction(thread_));
    Expected<JSTaggedValue, bool> result = CommonInvokeEcmaEntrypoint(jsPandaFile, entryPoint, func, executeType);

    CheckHasPendingException(thread_);
    return result;
}

Expected<JSTaggedValue, bool> EcmaVM::InvokeEcmaEntrypointForHotReload(
    const JSPandaFile *jsPandaFile, std::string_view entryPoint, const ExecuteTypes &executeType)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program = JSPandaFileManager::GetInstance()->GenerateProgram(this, jsPandaFile, entryPoint);

    JSHandle<JSFunction> func(thread_, program->GetMainFunction(thread_));
    Expected<JSTaggedValue, bool> result = CommonInvokeEcmaEntrypoint(jsPandaFile, entryPoint, func, executeType);

    JSHandle<JSTaggedValue> finalModuleRecord(thread_, func->GetModule(thread_));
    // avoid GC problems.
    GlobalHandleCollection gloalHandleCollection(thread_);
    JSHandle<JSTaggedValue> moduleRecordHandle =
        gloalHandleCollection.NewHandle<JSTaggedValue>(finalModuleRecord->GetRawData());
    CString recordName = entryPoint.data();
    AddPatchModule(recordName, moduleRecordHandle);

    // print exception information
    if (thread_->HasPendingException() &&
        Method::Cast(func->GetMethod(thread_))->GetMethodName(thread_) != JSPandaFile::PATCH_FUNCTION_NAME_0) {
        return Unexpected(false);
    }
    return result;
}

void EcmaVM::CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
                          const JSPandaFile *jsPandaFile, std::string_view entryPoint)
{
    // create "module", "exports", "require", "filename", "dirname"
    JSHandle<CjsModule> module = factory_->NewCjsModule();
    JSHandle<JSTaggedValue> require = GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory_->NewCjsExports();
    CString fileNameStr;
    CString dirNameStr;
    if (jsPandaFile->IsBundlePack()) {
        ModulePathHelper::ResolveCurrentPath(dirNameStr, fileNameStr, jsPandaFile);
    } else {
        JSTaggedValue funcFileName = func->GetModule(thread_);
        ASSERT(funcFileName.IsString());
        fileNameStr = ModulePathHelper::Utf8ConvertToString(thread_, funcFileName);
        dirNameStr = PathHelper::ResolveDirPath(fileNameStr);
    }
    JSHandle<JSTaggedValue> fileName = JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8(fileNameStr));
    JSHandle<JSTaggedValue> dirName = JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8(dirNameStr));
    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread_, cjsInfo);
    if (aotFileManager_->IsLoadMain(jsPandaFile, entryPoint.data())) {
        EcmaRuntimeStatScope runtimeStateScope(this);
        InvokeEcmaAotEntrypoint(func, thisArg, jsPandaFile, entryPoint, &cjsInfo);
    } else {
        // Execute main function
        JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread_,
                                                JSHandle<JSTaggedValue>(func),
                                                thisArg, undefined, 5); // 5 : argument numbers
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        info->SetCallArg(cjsInfo.exportsHdl.GetTaggedValue(),
            cjsInfo.requireHdl.GetTaggedValue(),
            cjsInfo.moduleHdl.GetTaggedValue(),
            cjsInfo.filenameHdl.GetTaggedValue(),
            cjsInfo.dirnameHdl.GetTaggedValue());
        EcmaRuntimeStatScope runtimeStatScope(this);
        EcmaInterpreter::Execute(info);
    }
    if (!thread_->HasPendingException()) {
        // Collecting module.exports : exports ---> module.exports --->Module._cache
        RequireManager::CollectExecutedExp(thread_, cjsInfo);
    }
}

void EcmaVM::ClearKeptObjects(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    if (LIKELY(globalEnv->GetTaggedWeakRefKeepObjects().IsUndefined())) {
        return;
    }
    globalEnv->SetWeakRefKeepObjects(thread, JSTaggedValue::Undefined());
}

void EcmaVM::AddToKeptObjects(JSThread *thread, JSHandle<JSTaggedValue> value)
{
    if (value->IsInSharedHeap()) {
        return;
    }

    JSHandle<GlobalEnv> globalEnv = thread->GetGlobalEnv();
    JSHandle<LinkedHashSet> linkedSet;
    if (globalEnv->GetWeakRefKeepObjects()->IsUndefined()) {
        linkedSet = LinkedHashSet::Create(thread);
    } else {
        linkedSet = JSHandle<LinkedHashSet>(thread,
            LinkedHashSet::Cast(globalEnv->GetWeakRefKeepObjects()->GetTaggedObject()));
    }
    linkedSet = LinkedHashSet::Add(thread, linkedSet, value);
    globalEnv->SetWeakRefKeepObjects(thread, linkedSet);
}

void EcmaVM::AddModuleManager(ModuleManager *moduleManager)
{
    moduleManagers_.PushBack(moduleManager);
}

void EcmaVM::ModuleManagers::Iterate(RootVisitor &v)
{
    LockHolder lock(CMCGCMutex_);
    for (ModuleManager *moduleManager : moduleManagersVec_) {
        moduleManager->Iterate(v);
    }
}

template <typename T>
void EcmaVM::ModuleManagers::PushBack(T v)
{
    LockHolder lock(CMCGCMutex_);
    moduleManagersVec_.push_back(v);
}

void EcmaVM::ModuleManagers::DestroyAllNativeObj()
{
    LockHolder lock(CMCGCMutex_);
    for (auto &moduleManager : moduleManagersVec_) {
        moduleManager->NativeObjDestory();
    }
}

void EcmaVM::ModuleManagers::Clear()
{
    LockHolder lock(CMCGCMutex_);
    for (auto &moduleManager : moduleManagersVec_) {
        delete moduleManager;
        moduleManager = nullptr;
    }
    moduleManagersVec_.clear();
}

void EcmaVM::InitDataViewTypeTable(const GlobalEnvConstants *constant)
{
    dataViewTypeTable_.emplace(constant->GetInt8ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::INT8));
    dataViewTypeTable_.emplace(
        constant->GetSharedInt8ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::INT8));
    dataViewTypeTable_.emplace(constant->GetUint8ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT8));
    dataViewTypeTable_.emplace(
        constant->GetSharedUint8ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT8));
    dataViewTypeTable_.emplace(
        constant->GetUint8ClampedArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT8_CLAMPED));
    dataViewTypeTable_.emplace(
        constant->GetSharedUint8ClampedArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT8_CLAMPED));
    dataViewTypeTable_.emplace(constant->GetInt16ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::INT16));
    dataViewTypeTable_.emplace(
        constant->GetSharedInt16ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::INT16));
    dataViewTypeTable_.emplace(
        constant->GetUint16ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT16));
    dataViewTypeTable_.emplace(
        constant->GetSharedUint16ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT16));
    dataViewTypeTable_.emplace(constant->GetInt32ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::INT32));
    dataViewTypeTable_.emplace(
        constant->GetSharedInt32ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::INT32));
    dataViewTypeTable_.emplace(
        constant->GetUint32ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT32));
    dataViewTypeTable_.emplace(
        constant->GetSharedUint32ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::UINT32));
    dataViewTypeTable_.emplace(
        constant->GetFloat32ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::FLOAT32));
    dataViewTypeTable_.emplace(
        constant->GetSharedFloat32ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::FLOAT32));
    dataViewTypeTable_.emplace(
        constant->GetFloat64ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::FLOAT64));
    dataViewTypeTable_.emplace(
        constant->GetSharedFloat64ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::FLOAT64));
    dataViewTypeTable_.emplace(
        constant->GetBigInt64ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::BIGINT64));
    dataViewTypeTable_.emplace(
        constant->GetSharedBigInt64ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::BIGINT64));
    dataViewTypeTable_.emplace(
        constant->GetBigUint64ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::BIGUINT64));
    dataViewTypeTable_.emplace(
        constant->GetSharedBigUint64ArrayString().GetRawData(), static_cast<int8_t>(DataViewType::BIGUINT64));
}
}  // namespace panda::ecmascript
