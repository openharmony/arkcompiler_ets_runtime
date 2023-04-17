/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/base/path_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#if !WIN_OR_MAC_OR_IOS_PLATFORM
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#endif
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/dfx/vmstat/runtime_stat.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/aot_file_manager.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/quick_fix_manager.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/taskpool/task.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_require_manager.h"

namespace panda::ecmascript {
using RandomGenerator = base::RandomGenerator;
using PathHelper = base::PathHelper;
/* static */
EcmaVM *EcmaVM::Create(const JSRuntimeOptions &options, EcmaParamConfiguration &config)
{
    JSRuntimeOptions newOptions = options;
    // only define SUPPORT_ENABLE_ASM_INTERP can enable asm-interpreter
#if !defined(SUPPORT_ENABLE_ASM_INTERP)
    newOptions.SetEnableAsmInterpreter(false);
#endif
    auto vm = new EcmaVM(newOptions, config);
    if (UNLIKELY(vm == nullptr)) {
        LOG_ECMA(ERROR) << "Failed to create jsvm";
        return nullptr;
    }
    auto jsThread = JSThread::Create(vm);
    vm->thread_ = jsThread;
    vm->Initialize();
    return vm;
}

// static
bool EcmaVM::Destroy(EcmaVM *vm)
{
    if (vm != nullptr) {
        delete vm;
        vm = nullptr;
        return true;
    }
    return false;
}

void EcmaVM::PreFork()
{
    heap_->CompactHeapBeforeFork();
    heap_->AdjustSpaceSizeForAppSpawn();
    heap_->GetReadOnlySpace()->SetReadOnly();
    heap_->DisableParallelGC();
}

void EcmaVM::PostFork()
{
    RandomGenerator::InitRandom();
    heap_->SetHeapMode(HeapMode::SHARE);
    GetAssociatedJSThread()->SetThreadId();
    heap_->EnableParallelGC();
}

EcmaVM::EcmaVM(JSRuntimeOptions options, EcmaParamConfiguration config)
    : stringTable_(new EcmaStringTable(this)),
      nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()),
      heapRegionAllocator_(std::make_unique<HeapRegionAllocator>()),
      chunk_(nativeAreaAllocator_.get()),
      ecmaParamConfiguration_(std::move(config))
{
    options_ = std::move(options);
    icEnabled_ = options_.EnableIC();
    optionalLogEnabled_ = options_.EnableOptionalLog();
    snapshotFileName_ = options_.GetSnapshotFile().c_str();
    frameworkAbcFileName_ = options_.GetFrameworkAbcFile().c_str();
    options_.ParseAsmInterOption();
}

void EcmaVM::ResetPGOProfiler()
{
    bool isEnablePGOProfiler = IsEnablePGOProfiler();
    if (options_.IsWorker()) {
        isEnablePGOProfiler = PGOProfilerManager::GetInstance()->IsEnable();
    }
    if (pgoProfiler_ == nullptr) {
        pgoProfiler_ = PGOProfilerManager::GetInstance()->Build(this, isEnablePGOProfiler);
    } else {
        PGOProfilerManager::GetInstance()->Reset(pgoProfiler_, isEnablePGOProfiler);
    }
    thread_->SetPGOProfilerEnable(isEnablePGOProfiler);
}

bool EcmaVM::IsEnablePGOProfiler() const
{
    return options_.GetEnableAsmInterpreter() && options_.IsEnablePGOProfiler();
}

bool EcmaVM::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "EcmaVM::Initialize");
    ResetPGOProfiler();
    Taskpool::GetCurrentTaskpool()->Initialize();
#ifndef PANDA_TARGET_WINDOWS
    RuntimeStubs::Initialize(thread_);
#endif
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    regExpParserCache_ = new RegExpParserCache();
    heap_ = new Heap(this);
    heap_->Initialize();
    gcStats_ = chunk_.New<GCStats>(heap_, options_.GetLongPauseTime());
    factory_ = chunk_.New<ObjectFactory>(thread_, heap_);
    if (UNLIKELY(factory_ == nullptr)) {
        LOG_FULL(FATAL) << "alloc factory_ failed";
        UNREACHABLE();
    }
    debuggerManager_ = chunk_.New<tooling::JsDebuggerManager>(this);
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    if (!options_.EnableSnapshotDeserialize()) {
        LOG_ECMA(DEBUG) << "EcmaVM::Initialize run builtins";
        JSHandle<JSHClass> hClassHandle = factory_->InitClassClass();
        JSHandle<JSHClass> globalEnvClass = factory_->NewEcmaHClass(*hClassHandle,
                                                                   GlobalEnv::SIZE,
                                                                   JSType::GLOBAL_ENV);
        globalConst->Init(thread_, *hClassHandle);
        globalConstInitialized_ = true;
        JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(*globalEnvClass);
        globalEnv->Init(thread_);
        globalEnv_ = globalEnv.GetTaggedValue();
        thread_->SetGlueGlobalEnv(reinterpret_cast<GlobalEnv *>(globalEnv.GetTaggedType()));
        Builtins builtins;
        builtins.Initialize(globalEnv, thread_);
        if (!WIN_OR_MAC_OR_IOS_PLATFORM && options_.EnableSnapshotSerialize()) {
            const CString fileName = "builtins.snapshot";
            Snapshot snapshot(this);
            snapshot.SerializeBuiltins(fileName);
        }
    } else {
        const CString fileName = "builtins.snapshot";
        Snapshot snapshot(this);
        if (!WIN_OR_MAC_OR_IOS_PLATFORM) {
            snapshot.Deserialize(SnapshotType::BUILTINS, fileName, true);
        }
        globalConst->InitSpecialForSnapshot();
        Builtins builtins;
        builtins.InitializeForSnapshot(thread_);
        globalConstInitialized_ = true;
    }

    SetupRegExpResultCache();
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    GenerateInternalNativeMethods();
    thread_->SetGlobalObject(GetGlobalEnv()->GetGlobalObject());
    moduleManager_ = new ModuleManager(this);
    tsManager_ = new TSManager(this);
    quickFixManager_ = new QuickFixManager();
    snapshotEnv_ = new SnapshotEnv(this);
    if (!WIN_OR_MAC_OR_IOS_PLATFORM) {
        snapshotEnv_->Initialize();
    }
    aotFileManager_ = new AOTFileManager(this);
    if (options_.GetEnableAsmInterpreter()) {
        LoadStubFile();
    }

    if (options_.GetEnableAsmInterpreter() && options_.WasAOTOutputFileSet()) {
        AnFileDataManager::GetInstance()->SetEnable(true);
        std::string aotFilename = options_.GetAOTOutputFile();
        LoadAOTFiles(aotFilename);
    }

    optCodeProfiler_ = new OptCodeProfiler();

    initialized_ = true;
    return true;
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
        if (runtimeStat_->IsRuntimeStatEnabled()) {
            runtimeStat_->Print();
            runtimeStat_->ResetAllCount();
        }
    }
    runtimeStat_->SetRuntimeStatEnabled(flag);
}

EcmaVM::~EcmaVM()
{
    initialized_ = false;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    DFXJSNApi::StopCpuProfilerForFile(this);
#endif
    heap_->WaitAllTasksFinished();
    Taskpool::GetCurrentTaskpool()->Destroy(thread_->GetThreadId());

    if (runtimeStat_ != nullptr && runtimeStat_->IsRuntimeStatEnabled()) {
        runtimeStat_->Print();
    }

    if (optCodeProfiler_ != nullptr) {
        delete optCodeProfiler_;
        optCodeProfiler_ = nullptr;
    }

    // clear c_address: c++ pointer delete
    ClearBufferData();

    if (!isBundlePack_) {
        const JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(assetPath_);
        if (jsPandaFile != nullptr) {
            const_cast<JSPandaFile *>(jsPandaFile)->DeleteParsedConstpoolVM(this);
        }
    }
    // clear icu cache
    ClearIcuCache();

    if (gcStats_ != nullptr) {
        if (options_.EnableGCStatsPrint()) {
            gcStats_->PrintStatisticResult(true);
        }
        chunk_.Delete(gcStats_);
        gcStats_ = nullptr;
    }

    if (heap_ != nullptr) {
        heap_->Destroy();
        delete heap_;
        heap_ = nullptr;
    }

    if (regExpParserCache_ != nullptr) {
        delete regExpParserCache_;
        regExpParserCache_ = nullptr;
    }

    if (debuggerManager_ != nullptr) {
        chunk_.Delete(debuggerManager_);
        debuggerManager_ = nullptr;
    }

    if (factory_ != nullptr) {
        chunk_.Delete(factory_);
        factory_ = nullptr;
    }

    if (stringTable_ != nullptr) {
        delete stringTable_;
        stringTable_ = nullptr;
    }

    if (runtimeStat_ != nullptr) {
        chunk_.Delete(runtimeStat_);
        runtimeStat_ = nullptr;
    }

    if (moduleManager_ != nullptr) {
        delete moduleManager_;
        moduleManager_ = nullptr;
    }

    if (tsManager_ != nullptr) {
        delete tsManager_;
        tsManager_ = nullptr;
    }

    if (quickFixManager_ != nullptr) {
        delete quickFixManager_;
        quickFixManager_ = nullptr;
    }

    if (snapshotEnv_ != nullptr) {
        delete snapshotEnv_;
        snapshotEnv_ = nullptr;
    }

    if (aotFileManager_ != nullptr) {
        delete aotFileManager_;
        aotFileManager_  = nullptr;
    }

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }

    if (pgoProfiler_ != nullptr) {
        PGOProfilerManager::GetInstance()->Destroy(pgoProfiler_);
        pgoProfiler_ = nullptr;
    }
}

JSHandle<GlobalEnv> EcmaVM::GetGlobalEnv() const
{
    return JSHandle<GlobalEnv>(reinterpret_cast<uintptr_t>(&globalEnv_));
}

JSHandle<job::MicroJobQueue> EcmaVM::GetMicroJobQueue() const
{
    return JSHandle<job::MicroJobQueue>(reinterpret_cast<uintptr_t>(&microJobQueue_));
}

bool EcmaVM::FindCatchBlock(Method *method, uint32_t pc) const
{
    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    if (thread_->IsAsmInterpreter()) {
        pcOffset = InterpreterAssembly::FindCatchBlock(method, pc);
    } else {
        pcOffset = EcmaInterpreter::FindCatchBlock(method, pc);
    }
    return pcOffset != panda_file::INVALID_OFFSET;
}

JSTaggedValue EcmaVM::InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                              const JSPandaFile *jsPandaFile, std::string_view entryPoint)
{
    aotFileManager_->SetAOTMainFuncEntry(mainFunc, jsPandaFile, entryPoint);
    Method *method = mainFunc->GetCallTarget();
    size_t actualNumArgs = method->GetNumArgs();
    size_t argsNum = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS;
    std::vector<JSTaggedType> args(argsNum, JSTaggedValue::Undefined().GetRawData());
    args[0] = mainFunc.GetTaggedValue().GetRawData();
    args[2] = thisArg.GetTaggedValue().GetRawData(); // 2: this
    const JSTaggedType *prevFp = thread_->GetLastLeaveFrame();
    // do not modify this log to INFO, this will call many times
    LOG_ECMA(DEBUG) << "start to execute aot entry: " << entryPoint;
    JSTaggedValue res = ExecuteAot(actualNumArgs, args.data(), prevFp, OptimizedEntryFrame::CallType::CALL_FUNC);
    if (thread_->HasPendingException()) {
        return thread_->GetException();
    }
    return res;
}

JSTaggedValue EcmaVM::ExecuteAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp,
                                 OptimizedEntryFrame::CallType callType)
{
    INTERPRETER_TRACE(thread_, ExecuteAot);
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    // do not modify this log to INFO, this will call many times
    LOG_ECMA(DEBUG) << "start to execute aot entry: " << (void*)entry;
    auto res = reinterpret_cast<JSFunctionEntryType>(entry)(thread_->GetGlueAddr(),
                                                            actualNumArgs,
                                                            args,
                                                            reinterpret_cast<uintptr_t>(prevFp),
                                                            static_cast<size_t>(callType));
    return res;
}

void EcmaVM::CheckStartCpuProfiler()
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (profiler_ == nullptr && !options_.IsWorker() &&
        options_.EnableCpuProfiler() && options_.GetArkBundleName().compare(bundleName_) == 0) {
        std::string fileName = options_.GetArkBundleName() + ".cpuprofile";
        if (!builtins::BuiltinsArkTools::CreateFile(fileName)) {
            LOG_ECMA(ERROR) << "createFile failed " << fileName;
        } else {
            DFXJSNApi::StartCpuProfilerForFile(this, fileName);
        }
    }
#endif
}

Expected<JSTaggedValue, bool> EcmaVM::InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                                           bool excuteFromJob)
{
    JSTaggedValue result;
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program;
    if (jsPandaFile != frameworkPandaFile_) {
        program = JSPandaFileManager::GetInstance()->GenerateProgram(this, jsPandaFile, entryPoint);
    } else {
        program = JSHandle<Program>(thread_, frameworkProgram_);
        frameworkProgram_ = JSTaggedValue::Hole();
    }
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(false);
    }
    // for debugger
    debuggerManager_->GetNotificationManager()->LoadModuleEvent(jsPandaFile->GetJSPandaFileDesc(), entryPoint);

    JSHandle<JSFunction> func = JSHandle<JSFunction>(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    if (jsPandaFile->IsModule(thread_, entryPoint.data())) {
        global = JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        CString moduleName = jsPandaFile->GetJSPandaFileDesc();
        if (!jsPandaFile->IsBundlePack()) {
            moduleName = entryPoint.data();
        }
        JSHandle<SourceTextModule> module = moduleManager_->HostGetImportedModule(moduleName);
        func->SetModule(thread_, module);
    } else {
        // if it is Cjs at present, the module slot of the function is not used. We borrow it to store the recordName,
        // which can avoid the problem of larger memory caused by the new slot
        JSHandle<EcmaString> recordName =  factory_->NewFromUtf8(entryPoint.data());
        func->SetModule(thread_, recordName);
    }
    CheckStartCpuProfiler();

    if (aotFileManager_->IsLoadMain(jsPandaFile, entryPoint.data())) {
        thread_->SetPrintBCOffset(true);
        EcmaRuntimeStatScope runtimeStatScope(this);
        result = InvokeEcmaAotEntrypoint(func, global, jsPandaFile, entryPoint);
    } else {
        if (jsPandaFile->IsCjs(thread_, entryPoint.data())) {
            if (!thread_->HasPendingException()) {
                CJSExecution(func, global, jsPandaFile);
            }
        } else {
            JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(this);
            EcmaInterpreter::Execute(info);
        }
    }
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
    }

    // print exception information
    if (!excuteFromJob && thread_->HasPendingException()) {
        auto exception = thread_->GetException();
        HandleUncaughtException(exception.GetTaggedObject());
    }
    return result;
}

bool EcmaVM::HasCachedConstpool(const JSPandaFile *jsPandaFile) const
{
    return cachedConstpools_.find(jsPandaFile) != cachedConstpools_.end();
}

JSTaggedValue EcmaVM::FindConstpool(const JSPandaFile *jsPandaFile, int32_t index)
{
    auto iter = cachedConstpools_.find(jsPandaFile);
    if (iter == cachedConstpools_.end()) {
        return JSTaggedValue::Hole();
    }
    auto constpoolIter = iter->second.find(index);
    if (constpoolIter == iter->second.end()) {
        return JSTaggedValue::Hole();
    }
    return constpoolIter->second;
}

JSHandle<ConstantPool> EcmaVM::FindOrCreateConstPool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    JSTaggedValue constpool = FindConstpool(jsPandaFile, index);
    if (constpool.IsHole()) {
        JSHandle<ConstantPool> newConstpool = ConstantPool::CreateConstPool(this, jsPandaFile, id);
        AddConstpool(jsPandaFile, newConstpool.GetTaggedValue(), index);
        return newConstpool;
    }

    return JSHandle<ConstantPool>(thread_, constpool);
}

std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> EcmaVM::FindConstpools(
    const JSPandaFile *jsPandaFile)
{
    auto iter = cachedConstpools_.find(jsPandaFile);
    if (iter == cachedConstpools_.end()) {
        return std::nullopt;
    }
    return iter->second;
}

void EcmaVM::CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
                          const JSPandaFile *jsPandaFile)
{
    // create "module", "exports", "require", "filename", "dirname"
    JSHandle<CjsModule> module = factory_->NewCjsModule();
    JSHandle<JSTaggedValue> require = GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory_->NewCjsExports();
    JSMutableHandle<JSTaggedValue> filename(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> dirname(thread_, JSTaggedValue::Undefined());
    if (jsPandaFile->IsBundlePack()) {
        PathHelper::ResolveCurrentPath(thread_, dirname, filename, jsPandaFile);
    } else {
        filename.Update(JSFunction::Cast(func.GetTaggedValue().GetTaggedObject())->GetModule());
        ASSERT(filename->IsString());
        dirname.Update(PathHelper::ResolveDirPath(thread_, filename));
    }
    CJSInfo cjsInfo(module, require, exports, filename, dirname);
    RequireManager::InitializeCommonJS(thread_, cjsInfo);

    // Execute main function
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread_,
                                            JSHandle<JSTaggedValue>(func),
                                            thisArg, undefined, 5); // 5 : argument numbers
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    if (info == nullptr) {
        LOG_ECMA(ERROR) << "CJSExecution Stack overflow!";
        return;
    }
    info->SetCallArg(cjsInfo.exportsHdl.GetTaggedValue(),
                     cjsInfo.requireHdl.GetTaggedValue(),
                     cjsInfo.moduleHdl.GetTaggedValue(),
                     cjsInfo.filenameHdl.GetTaggedValue(),
                     cjsInfo.dirnameHdl.GetTaggedValue());
    EcmaRuntimeStatScope runtimeStatScope(this);
    EcmaInterpreter::Execute(info);
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
    }

    if (!thread_->HasPendingException()) {
        // Collecting module.exports : exports ---> module.exports --->Module._cache
        RequireManager::CollectExecutedExp(thread_, cjsInfo);
    }
}

void EcmaVM::AddConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool, int32_t index)
{
    ASSERT(constpool.IsConstantPool());
    if (cachedConstpools_.find(jsPandaFile) == cachedConstpools_.end()) {
        cachedConstpools_[jsPandaFile] = CMap<int32_t, JSTaggedValue>();
    }
    auto &constpoolMap = cachedConstpools_[jsPandaFile];
    ASSERT(constpoolMap.find(index) == constpoolMap.end());

    constpoolMap.insert({index, constpool});
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

void EcmaVM::EnableUserUncaughtErrorHandler()
{
    isUncaughtExceptionRegistered_ = true;
}

void EcmaVM::HandleUncaughtException(TaggedObject *exception)
{
    if (isUncaughtExceptionRegistered_) {
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    JSHandle<JSTaggedValue> exceptionHandle(thread_, JSTaggedValue(exception));
    // if caught exceptionHandle type is JSError
    thread_->ClearException();
    if (exceptionHandle->IsJSError()) {
        PrintJSErrorInfo(exceptionHandle);
        if (thread_->IsPrintBCOffset() && exceptionBCList_.size() != 0) {
            for (auto info : exceptionBCList_) {
                LOG_ECMA(ERROR) << "Exception at function " << info.first << ": " << info.second;
            }
        }
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(thread_, exceptionHandle);
    CString string = ConvertToString(*result);
    LOG_NO_TAG(ERROR) << string;
}

void EcmaVM::PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo)
{
    JSHandle<JSTaggedValue> nameKey = thread_->GlobalConstants()->GetHandledNameString();
    JSHandle<EcmaString> name(JSObject::GetProperty(thread_, exceptionInfo, nameKey).GetValue());
    JSHandle<JSTaggedValue> msgKey = thread_->GlobalConstants()->GetHandledMessageString();
    JSHandle<EcmaString> msg(JSObject::GetProperty(thread_, exceptionInfo, msgKey).GetValue());
    JSHandle<JSTaggedValue> stackKey = thread_->GlobalConstants()->GetHandledStackString();
    JSHandle<EcmaString> stack(JSObject::GetProperty(thread_, exceptionInfo, stackKey).GetValue());

    CString nameBuffer = ConvertToString(*name);
    CString msgBuffer = ConvertToString(*msg);
    CString stackBuffer = ConvertToString(*stack);
    LOG_NO_TAG(ERROR) << nameBuffer << ": " << msgBuffer << "\n" << stackBuffer;
}

void EcmaVM::ProcessNativeDelete(const WeakRootVisitor &visitor)
{
    auto iter = nativePointerList_.begin();
    while (iter != nativePointerList_.end()) {
        JSNativePointer *object = *iter;
        auto fwd = visitor(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            object->Destroy();
            iter = nativePointerList_.erase(iter);
        } else {
            ++iter;
        }
    }

    auto iterator = cachedConstpools_.begin();
    while (iterator != cachedConstpools_.end()) {
        auto &constpools = iterator->second;
        auto constpoolIter = constpools.begin();
        while (constpoolIter != constpools.end()) {
            JSTaggedValue constpoolVal = constpoolIter->second;
            if (constpoolVal.IsHeapObject()) {
                TaggedObject *obj = constpoolVal.GetTaggedObject();
                auto fwd = visitor(obj);
                if (fwd == nullptr) {
                    constpoolIter = constpools.erase(constpoolIter);
                    continue;
                }
            }
            ++constpoolIter;
        }
        if (constpools.size() == 0) {
            JSPandaFileManager::RemoveJSPandaFile(const_cast<JSPandaFile *>(iterator->first));
            iterator = cachedConstpools_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}
void EcmaVM::ProcessReferences(const WeakRootVisitor &visitor)
{
    if (regExpParserCache_ != nullptr) {
        regExpParserCache_->Clear();
    }
    heap_->ResetNativeBindingSize();
    // array buffer
    auto iter = nativePointerList_.begin();
    while (iter != nativePointerList_.end()) {
        JSNativePointer *object = *iter;
        auto fwd = visitor(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            object->Destroy();
            iter = nativePointerList_.erase(iter);
            continue;
        }
        heap_->IncreaseNativeBindingSize(JSNativePointer::Cast(fwd));
        if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            *iter = JSNativePointer::Cast(fwd);
        }
        ++iter;
    }

    // program maps
    auto iterator = cachedConstpools_.begin();
    while (iterator != cachedConstpools_.end()) {
        auto &constpools = iterator->second;
        auto constpoolIter = constpools.begin();
        while (constpoolIter != constpools.end()) {
            JSTaggedValue constpoolVal = constpoolIter->second;
            if (constpoolVal.IsHeapObject()) {
                TaggedObject *obj = constpoolVal.GetTaggedObject();
                auto fwd = visitor(obj);
                if (fwd == nullptr) {
                    constpoolIter = constpools.erase(constpoolIter);
                    continue;
                } else if (fwd != obj) {
                    constpoolIter->second = JSTaggedValue(fwd);
                }
            }
            ++constpoolIter;
        }
        if (constpools.size() == 0) {
            JSPandaFileManager::RemoveJSPandaFile(const_cast<JSPandaFile *>(iterator->first));
            iterator = cachedConstpools_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void EcmaVM::PushToNativePointerList(JSNativePointer *array)
{
    nativePointerList_.emplace_back(array);
}

void EcmaVM::RemoveFromNativePointerList(JSNativePointer *array)
{
    auto iter = std::find(nativePointerList_.begin(), nativePointerList_.end(), array);
    if (iter != nativePointerList_.end()) {
        JSNativePointer *object = *iter;
        object->Destroy();
        nativePointerList_.erase(iter);
    }
}

void EcmaVM::ClearBufferData()
{
    for (auto iter : nativePointerList_) {
        iter->Destroy();
    }
    nativePointerList_.clear();

    cachedConstpools_.clear();
    internalNativeMethods_.clear();
    WorkerList_.clear();
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
        isProcessingPendingJob_ = false;
        return true;
    }
    return false;
}

void EcmaVM::CollectGarbage(TriggerGCType gcType) const
{
    heap_->CollectGarbage(gcType);
}

void EcmaVM::StartHeapTracking(HeapTracker *tracker)
{
    heap_->StartHeapTracking(tracker);
}

void EcmaVM::StopHeapTracking()
{
    heap_->StopHeapTracking();
}

void EcmaVM::Iterate(const RootVisitor &v, const RootRangeVisitor &rv)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalEnv_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&microJobQueue_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpCache_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&frameworkProgram_)));
    rv(Root::ROOT_VM, ObjectSlot(ToUintPtr(&internalNativeMethods_.front())),
        ObjectSlot(ToUintPtr(&internalNativeMethods_.back()) + JSTaggedValue::TaggedTypeSize()));
    moduleManager_->Iterate(v);
    tsManager_->Iterate(v);
    aotFileManager_->Iterate(v);
    if (!WIN_OR_MAC_OR_IOS_PLATFORM) {
        snapshotEnv_->Iterate(v);
    }
}

void EcmaVM::SetGlobalEnv(GlobalEnv *global)
{
    ASSERT(global != nullptr);
    globalEnv_ = JSTaggedValue(global);
}

void EcmaVM::SetMicroJobQueue(job::MicroJobQueue *queue)
{
    ASSERT(queue != nullptr);
    microJobQueue_ = JSTaggedValue(queue);
}

void EcmaVM::SetupRegExpResultCache()
{
    regexpCache_ = builtins::RegExpExecResultCache::CreateCacheTable(thread_);
}

void EcmaVM::LoadStubFile()
{
    std::string stubFile = options_.GetStubFile();
    aotFileManager_->LoadStubFile(stubFile);
}

void EcmaVM::LoadAOTFiles(const std::string& aotFileName)
{
    std::string anFile = aotFileName + AOTFileManager::FILE_EXTENSION_AN;
    if (!aotFileManager_->LoadAnFile(anFile)) {
        LOG_ECMA(ERROR) << "Load " << anFile << " failed. Destroy aot data and rollback to interpreter";
        ecmascript::AnFileDataManager::GetInstance()->SafeDestroyAnData(anFile);
        return;
    }

    std::string aiFile = aotFileName + AOTFileManager::FILE_EXTENSION_AI;
    aotFileManager_->LoadAiFile(aiFile);
}

#if !WIN_OR_MAC_OR_IOS_PLATFORM
void EcmaVM::DeleteHeapProfile()
{
    if (heapProfile_ == nullptr) {
        return;
    }
    const_cast<NativeAreaAllocator *>(GetNativeAreaAllocator())->Delete(heapProfile_);
    heapProfile_ = nullptr;
}

HeapProfilerInterface *EcmaVM::GetOrNewHeapProfile()
{
    if (heapProfile_ != nullptr) {
        return heapProfile_;
    }
    heapProfile_ = const_cast<NativeAreaAllocator *>(GetNativeAreaAllocator())->New<HeapProfiler>(this);
    ASSERT(heapProfile_ != nullptr);
    return heapProfile_;
}
#endif

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
void *EcmaVM::InternalMethodTable[] = {
    reinterpret_cast<void *>(builtins::BuiltinsGlobal::CallJsBoundFunction),
    reinterpret_cast<void *>(builtins::BuiltinsGlobal::CallJsProxy),
    reinterpret_cast<void *>(builtins::BuiltinsObject::CreateDataPropertyOnObjectFunctions),
    reinterpret_cast<void *>(builtins::BuiltinsCollator::AnonymousCollator),
    reinterpret_cast<void *>(builtins::BuiltinsDateTimeFormat::AnonymousDateTimeFormat),
    reinterpret_cast<void *>(builtins::BuiltinsNumberFormat::NumberFormatInternalFormatNumber),
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
    reinterpret_cast<void *>(JSAsyncFromSyncIterator::AsyncFromSyncIterUnwarpFunction)
};

void EcmaVM::GenerateInternalNativeMethods()
{
    size_t length = static_cast<size_t>(MethodIndex::METHOD_END);
    for (size_t i = 0; i < length; i++) {
        uint32_t numArgs = 2;  // function object and this
        auto method = factory_->NewMethod(nullptr);
        method->SetNativePointer(InternalMethodTable[i]);
        method->SetNativeBit(true);
        method->SetNumArgsWithCallField(numArgs);
        method->SetFunctionKind(FunctionKind::NORMAL_FUNCTION);
        internalNativeMethods_.emplace_back(method.GetTaggedValue());
    }
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
        LOG_ECMA(INFO) << "Only trigger concurrent callback in taskpool thread";
        return;
    }

    if (result.IsJSPromise()) {
        // Async concurrent will return Promise
        auto promise = JSPromise::Cast(result.GetTaggedObject());
        if (promise->GetPromiseState() == PromiseState::PENDING) {
            LOG_ECMA(ERROR) << "Promise is in pending state, don't return";
            return;
        }
        result = promise->GetPromiseResult();
    }

    auto ret = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread_, result));
    auto data = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread_, hint));
    concurrentCallback_(ret, data, concurrentData_);
}
}  // namespace panda::ecmascript
