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

#include "ecmascript/ecma_context.h"

#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/compiler/aot_constantpool_patcher.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/jspandafile/abc_buffer_cache.h"
#include "ecmascript/platform/aot_crash_info.h"
#include "ecmascript/platform/ecma_context.h"
#include "ecmascript/platform/log.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/sustaining_js_handle.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

EcmaContext::EcmaContext(JSThread *thread)
    : thread_(thread),
      vm_(thread->GetEcmaVM()),
      factory_(vm_->GetFactory()),
      aotFileManager_(vm_->GetAOTFileManager())
{
}

/* static */
EcmaContext *EcmaContext::Create(JSThread *thread)
{
    LOG_ECMA(INFO) << "EcmaContext::Create";
    auto context = new EcmaContext(thread);
    return context;
}

// static
bool EcmaContext::Destroy(EcmaContext *context)
{
    if (context != nullptr) {
        delete context;
        context = nullptr;
        return true;
    }
    return false;
}

bool EcmaContext::Initialize()
{
    LOG_ECMA(DEBUG) << "EcmaContext::Initialize";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "EcmaContext::Initialize");
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    ecmaData_.propertiesCache_ = new PropertiesCache();
    if (vm_->GetJSOptions().IsEnableMegaIC()) {
        ecmaData_.loadMegaICCache_ = new MegaICCache();
        ecmaData_.storeMegaICCache_ = new MegaICCache();
    }
    regExpParserCache_ = new RegExpParserCache();
    unsharedConstpools_ = new(std::nothrow) JSTaggedValue[GetUnsharedConstpoolsArrayLen()];
    if (unsharedConstpools_ == nullptr) {
        LOG_ECMA(FATAL) << "allocate unshared constpool array fail during initing";
        UNREACHABLE();
    }
    std::fill(unsharedConstpools_, unsharedConstpools_ + GetUnsharedConstpoolsArrayLen(), JSTaggedValue::Hole());
    thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(unsharedConstpools_));
    thread_->SetUnsharedConstpoolsArrayLen(unsharedConstpoolsArrayLen_);

    thread_->SetGlobalConst(&globalConst_);
    globalConst_.Init(thread_);
    JSHandle<JSHClass> hClassHandle = JSHandle<JSHClass>(thread_, globalConst_.GetHClassClass());
    JSHandle<JSHClass> globalEnvClass = factory_->NewEcmaHClass(
        *hClassHandle,
        GlobalEnv::SIZE,
        JSType::GLOBAL_ENV);
    JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(*globalEnvClass);
    globalEnv->Init(thread_);
    globalEnv_ = globalEnv.GetTaggedValue();
    Builtins builtins;
    bool builtinsLazyEnabled = vm_->GetJSOptions().IsWorker() && vm_->GetJSOptions().GetEnableBuiltinsLazy();
    thread_->SetEnableLazyBuiltins(builtinsLazyEnabled);
    builtins.Initialize(globalEnv, thread_, builtinsLazyEnabled);

    InitializeDefaultLocale();
    InitializeDefaultCompareStringsOption();
    SetupRegExpResultCache();
    SetupRegExpGlobalResult();
    SetupNumberToStringResultCache();
    SetupStringSplitResultCache();
    SetupStringToListResultCache();
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    moduleManager_ = new ModuleManager(vm_);
    ptManager_ = new kungfu::PGOTypeManager(vm_);
    optCodeProfiler_ = new OptCodeProfiler();
    abcBufferCache_ = new AbcBufferCache();
    if (vm_->GetJSOptions().GetTypedOpProfiler()) {
        typedOpProfiler_ = new TypedOpProfiler();
    }
    if (vm_->GetJSOptions().EnableModuleLog() && !vm_->GetJSOptions().IsWorker()) {
        moduleLogger_ = new ModuleLogger(vm_);
    }
    functionProtoTransitionTable_ = new FunctionProtoTransitionTable(thread_);
    sustainingJSHandleList_ = new SustainingJSHandleList();
    initialized_ = true;
    return true;
}

void EcmaContext::InitializeEcmaScriptRunStat()
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
    runtimeStat_ = vm_->GetChunk()->New<EcmaRuntimeStat>(runtimeCallerNames, ecmascript::RUNTIME_CALLER_NUMBER);
    if (UNLIKELY(runtimeStat_ == nullptr)) {
        LOG_FULL(FATAL) << "alloc runtimeStat_ failed";
        UNREACHABLE();
    }
}

void EcmaContext::ClearIcuCache(JSThread *thread)
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(IcuFormatterType::ICU_FORMATTER_TYPE_COUNT); i++) {
        auto &icuFormatter = icuObjCache_[i];
        NativePointerCallback deleteEntry = icuFormatter.deleteEntry;
        if (deleteEntry != nullptr) {
            deleteEntry(thread->GetEnv(), icuFormatter.icuObj, vm_);
        }
        icuFormatter = EcmaContext::IcuFormatter{};
    }
}

void EcmaContext::SetRuntimeStatEnable(bool flag)
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

EcmaContext::~EcmaContext()
{
    if (runtimeStat_ != nullptr && runtimeStat_->IsRuntimeStatEnabled()) {
        runtimeStat_->Print();
    }
    for (auto n : handleStorageNodes_) {
        delete n;
    }
    handleStorageNodes_.clear();
    currentHandleStorageIndex_ = -1;
    handleScopeStorageNext_ = handleScopeStorageEnd_ = nullptr;

    for (auto n : primitiveStorageNodes_) {
        delete n;
    }
    primitiveStorageNodes_.clear();
    currentPrimitiveStorageIndex_ = -1;
    primitiveScopeStorageNext_ = primitiveScopeStorageEnd_ = nullptr;

    if (vm_->IsEnableBaselineJit() || vm_->IsEnableFastJit()) {
        // clear jit task
        vm_->GetJit()->ClearTask(this);
    }

    ClearBufferData();
    // clear c_address: c++ pointer delete
    if (!vm_->IsBundlePack()) {
        std::shared_ptr<JSPandaFile> jsPandaFile =
            JSPandaFileManager::GetInstance()->FindJSPandaFile(vm_->GetAssetPath());
        if (jsPandaFile != nullptr) {
            jsPandaFile->DeleteParsedConstpoolVM(vm_);
        }
    }
    ClearDefaultLocale();
    ClearDefaultComapreStringsOption();
    // clear icu cache
    ClearIcuCache(thread_);

    if (runtimeStat_ != nullptr) {
        vm_->GetChunk()->Delete(runtimeStat_);
        runtimeStat_ = nullptr;
    }
    if (optCodeProfiler_ != nullptr) {
        delete optCodeProfiler_;
        optCodeProfiler_ = nullptr;
    }
    if (typedOpProfiler_ != nullptr) {
        delete typedOpProfiler_;
        typedOpProfiler_ = nullptr;
    }
    if (moduleManager_ != nullptr) {
        delete moduleManager_;
        moduleManager_ = nullptr;
    }
    if (moduleLogger_ != nullptr) {
        delete moduleLogger_;
        moduleLogger_ = nullptr;
    }
    if (ptManager_ != nullptr) {
        delete ptManager_;
        ptManager_ = nullptr;
    }
    if (regExpParserCache_ != nullptr) {
        delete regExpParserCache_;
        regExpParserCache_ = nullptr;
    }
    if (aotFileManager_ != nullptr) {
        aotFileManager_ = nullptr;
    }
    if (ecmaData_.loadMegaICCache_ != nullptr) {
        delete ecmaData_.loadMegaICCache_;
        ecmaData_.loadMegaICCache_ = nullptr;
    }
    if (ecmaData_.storeMegaICCache_ != nullptr) {
        delete ecmaData_.storeMegaICCache_;
        ecmaData_.storeMegaICCache_ = nullptr;
    }
    if (ecmaData_.propertiesCache_ != nullptr) {
        delete ecmaData_.propertiesCache_;
        ecmaData_.propertiesCache_ = nullptr;
    }
    if (sustainingJSHandleList_ != nullptr) {
        delete sustainingJSHandleList_;
        sustainingJSHandleList_ = nullptr;
    }
    if (functionProtoTransitionTable_ != nullptr) {
        delete functionProtoTransitionTable_;
        functionProtoTransitionTable_ = nullptr;
    }
    if (abcBufferCache_ != nullptr) {
        delete abcBufferCache_;
        abcBufferCache_ = nullptr;
    }
    if (unsharedConstpools_ != nullptr) {
        delete[] unsharedConstpools_;
        unsharedConstpools_ = nullptr;
        thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(nullptr));
        thread_->SetUnsharedConstpoolsArrayLen(0);
    }
    // clear join stack
    joinStack_.clear();
}

JSTaggedValue EcmaContext::InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                                   const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                                   CJSInfo* cjsInfo)
{
    aotFileManager_->SetAOTMainFuncEntry(mainFunc, jsPandaFile, entryPoint);
    return JSFunction::InvokeOptimizedEntrypoint(thread_, mainFunc, thisArg, cjsInfo);
}

JSTaggedValue EcmaContext::ExecuteAot(size_t actualNumArgs, JSTaggedType *args,
                                      const JSTaggedType *prevFp, bool needPushArgv)
{
    INTERPRETER_TRACE(thread_, ExecuteAot);
    ASSERT(thread_->IsInManagedState());
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    // entry of aot
    auto res = reinterpret_cast<JSFunctionEntryType>(entry)(thread_->GetGlueAddr(),
                                                            actualNumArgs,
                                                            args,
                                                            reinterpret_cast<uintptr_t>(prevFp),
                                                            needPushArgv);
    return res;
}

Expected<JSTaggedValue, bool> EcmaContext::CommonInvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
    std::string_view entryPoint, JSHandle<JSFunction> &func, const ExecuteTypes &executeType)
{
    ASSERT(thread_->IsInManagedState());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    if (vm_->IsEnablePGOProfiler()) {
        JSHandle<JSFunction> objectFunction(GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetObjectFunction());
        JSHandle<JSHClass> protoOrHClass(GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetObjectFunctionNapiClass());
        vm_->GetPGOProfiler()->ProfileNapiRootHClass(
            objectFunction.GetTaggedType(), protoOrHClass.GetTaggedType(), pgo::ProfileType::Kind::NapiId);
    }
    CString entry = entryPoint.data();
    JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + entry + "' , which is application Entry Point";
        THROW_REFERENCE_ERROR_AND_RETURN(thread_, msg.c_str(), Unexpected(false));
    }
    ModuleLogger *moduleLogger = GetModuleLogger();
    if (moduleLogger != nullptr) {
        moduleLogger->SetStartTime(CString(entryPoint));
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
            module = moduleManager_->HostGetImportedModule(moduleName);
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
    vm_->CheckStartCpuProfiler();

    JSTaggedValue result;
    if (jsPandaFile->IsCjs(recordInfo)) {
        CJSExecution(func, global, jsPandaFile, entryPoint);
        if (moduleLogger != nullptr) {
            moduleLogger->SetEndTime(CString(entryPoint));
        }
    } else {
        if (aotFileManager_->IsLoadMain(jsPandaFile, entry)) {
            EcmaRuntimeStatScope runtimeStatScope(vm_);
            result = InvokeEcmaAotEntrypoint(func, global, jsPandaFile, entryPoint);
        } else if (vm_->GetJSOptions().IsEnableForceJitCompileMain()) {
            Jit::Compile(vm_, func, CompilerTier::Tier::FAST);
            EcmaRuntimeStatScope runtimeStatScope(vm_);
            result = JSFunction::InvokeOptimizedEntrypoint(thread_, func, global, nullptr);
        } else if (vm_->GetJSOptions().IsEnableForceBaselineCompileMain()) {
            Jit::Compile(vm_, func, CompilerTier::Tier::BASELINE);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(vm_);
            result = EcmaInterpreter::Execute(info);
        } else {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(vm_);
            result = EcmaInterpreter::Execute(info);
        }
        if (moduleLogger != nullptr) {
            moduleLogger->SetEndTime(CString(entryPoint));
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

Expected<JSTaggedValue, bool> EcmaContext::InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
                                                                std::string_view entryPoint,
                                                                const ExecuteTypes &executeType)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    auto &options = const_cast<EcmaVM *>(thread_->GetEcmaVM())->GetJSOptions();
    if (options.EnableModuleLog()) {
        LOG_FULL(INFO) << "current executing file's name " << entryPoint.data();
    }
    
    JSHandle<Program> program = JSPandaFileManager::GetInstance()->GenerateProgram(vm_, jsPandaFile, entryPoint);
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(false);
    }
    // for debugger
    vm_->GetJsDebuggerManager()->GetNotificationManager()->LoadModuleEvent(
        jsPandaFile->GetJSPandaFileDesc(), entryPoint);

    JSHandle<JSFunction> func(thread_, program->GetMainFunction());
    Expected<JSTaggedValue, bool> result = CommonInvokeEcmaEntrypoint(jsPandaFile, entryPoint, func, executeType);

    CheckHasPendingException(this, thread_);
    return result;
}

Expected<JSTaggedValue, bool> EcmaContext::InvokeEcmaEntrypointForHotReload(
    const JSPandaFile *jsPandaFile, std::string_view entryPoint, const ExecuteTypes &executeType)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program = JSPandaFileManager::GetInstance()->GenerateProgram(vm_, jsPandaFile, entryPoint);

    JSHandle<JSFunction> func(thread_, program->GetMainFunction());
    Expected<JSTaggedValue, bool> result = CommonInvokeEcmaEntrypoint(jsPandaFile, entryPoint, func, executeType);

    JSHandle<JSTaggedValue> finalModuleRecord(thread_, func->GetModule());
    // avoid GC problems.
    GlobalHandleCollection gloalHandleCollection(thread_);
    JSHandle<JSTaggedValue> moduleRecordHandle =
        gloalHandleCollection.NewHandle<JSTaggedValue>(finalModuleRecord->GetRawData());
    CString recordName = entryPoint.data();
    AddPatchModule(recordName, moduleRecordHandle);

    // print exception information
    if (thread_->HasPendingException() &&
        Method::Cast(func->GetMethod())->GetMethodName() != JSPandaFile::PATCH_FUNCTION_NAME_0) {
        return Unexpected(false);
    }
    return result;
}

void EcmaContext::CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
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
        JSTaggedValue funcFileName = func->GetModule();
        ASSERT(funcFileName.IsString());
        fileNameStr = ModulePathHelper::Utf8ConvertToString(funcFileName);
        dirNameStr = PathHelper::ResolveDirPath(fileNameStr);
    }
    JSHandle<JSTaggedValue> fileName = JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8(fileNameStr));
    JSHandle<JSTaggedValue> dirName = JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8(dirNameStr));
    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread_, cjsInfo);
    if (aotFileManager_->IsLoadMain(jsPandaFile, entryPoint.data())) {
        EcmaRuntimeStatScope runtimeStateScope(vm_);
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
        EcmaRuntimeStatScope runtimeStatScope(vm_);
        EcmaInterpreter::Execute(info);
    }
    if (!thread_->HasPendingException()) {
        // Collecting module.exports : exports ---> module.exports --->Module._cache
        RequireManager::CollectExecutedExp(thread_, cjsInfo);
    }
}

void EcmaContext::LoadProtoTransitionTable(JSTaggedValue constpool)
{
    JSTaggedValue protoTransitionTable = ConstantPool::Cast(constpool.GetTaggedObject())->GetProtoTransTableInfo();
    functionProtoTransitionTable_->UpdateProtoTransitionTable(
        thread_, JSHandle<PointerToIndexDictionary>(thread_, protoTransitionTable));
}

void EcmaContext::ResetProtoTransitionTableOnConstpool(JSTaggedValue constpool)
{
    ConstantPool::Cast(constpool.GetTaggedObject())->SetProtoTransTableInfo(thread_, JSTaggedValue::Undefined());
}

// just find unshared constpool, not create
JSTaggedValue EcmaContext::FindUnsharedConstpool(JSTaggedValue sharedConstpool)
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

JSTaggedValue EcmaContext::FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool)
{
    JSTaggedValue unsharedConstpool = FindUnsharedConstpool(sharedConstpool);
    if (unsharedConstpool.IsHole()) {
        ConstantPool *shareCp = ConstantPool::Cast(sharedConstpool.GetTaggedObject());
        int32_t constpoolIndex = shareCp->GetUnsharedConstpoolIndex();
        // unshared constpool index is default INT32_MAX.
        ASSERT(0 <= constpoolIndex && constpoolIndex != INT32_MAX);
        JSHandle<ConstantPool> unshareCp =
            ConstantPool::CreateUnSharedConstPoolBySharedConstpool(vm_, shareCp->GetJSPandaFile(), shareCp);
        unsharedConstpool = unshareCp.GetTaggedValue();
        SetUnsharedConstpool(constpoolIndex, unsharedConstpool);
    }
    return unsharedConstpool;
}

void EcmaContext::EraseUnusedConstpool(const JSPandaFile *jsPandaFile, int32_t index, int32_t constpoolIndex)
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

std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> EcmaContext::FindConstpools(
    const JSPandaFile *jsPandaFile)
{
    return Runtime::GetInstance()->FindConstpools(jsPandaFile);
}

// For new version instruction.
JSTaggedValue EcmaContext::FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    return FindConstpool(jsPandaFile, index);
}

JSTaggedValue EcmaContext::FindConstpool(const JSPandaFile *jsPandaFile, int32_t index)
{
    JSTaggedValue contextCache = FindConstpoolFromContextCache(jsPandaFile, index);
    if (!contextCache.IsHole()) {
        return contextCache;
    }
    return Runtime::GetInstance()->FindConstpool(jsPandaFile, index);
}

JSTaggedValue EcmaContext::FindConstpoolFromContextCache(const JSPandaFile *jsPandaFile, int32_t index)
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

bool EcmaContext::HasCachedConstpool(const JSPandaFile *jsPandaFile) const
{
    if (cachedSharedConstpools_.find(jsPandaFile) != cachedSharedConstpools_.end()) {
        return true;
    }

    return Runtime::GetInstance()->HasCachedConstpool(jsPandaFile);
}

JSHandle<ConstantPool> EcmaContext::AddOrUpdateConstpool(const JSPandaFile *jsPandaFile,
                                                         JSHandle<ConstantPool> constpool,
                                                         int32_t index)
{
    constpool = Runtime::GetInstance()->AddOrUpdateConstpool(jsPandaFile, constpool, index);
    AddContextConstpoolCache(jsPandaFile, constpool, index);
    return constpool;
}

void EcmaContext::AddContextConstpoolCache(const JSPandaFile *jsPandaFile,
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

void EcmaContext::SetUnsharedConstpool(JSHandle<ConstantPool> sharedConstpool, JSTaggedValue unsharedConstpool)
{
    int32_t constpoolIndex = sharedConstpool->GetUnsharedConstpoolIndex();
    SetUnsharedConstpool(constpoolIndex, unsharedConstpool);
}

void EcmaContext::SetUnsharedConstpool(int32_t constpoolIndex, JSTaggedValue unsharedConstpool)
{
    GrowUnsharedConstpoolArray(constpoolIndex);
    ASSERT(0 <= constpoolIndex && constpoolIndex < ConstantPool::CONSTPOOL_TYPE_FLAG);
    unsharedConstpools_[constpoolIndex] = unsharedConstpool;
}

void EcmaContext::GrowUnsharedConstpoolArray(int32_t index)
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

void EcmaContext::ResizeUnsharedConstpoolArray(int32_t oldCapacity, int32_t minCapacity)
{
    int32_t newCapacity = oldCapacity << 1;
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
    std::copy(unsharedConstpools_, unsharedConstpools_ + GetUnsharedConstpoolsArrayLen(), newUnsharedConstpools);
    ClearUnsharedConstpoolArray();
    unsharedConstpools_ = newUnsharedConstpools;
    thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(unsharedConstpools_));
    thread_->SetUnsharedConstpoolsArrayLen(newCapacity);
    SetUnsharedConstpoolsArrayLen(newCapacity);
}

void EcmaContext::UpdateConstpoolWhenDeserialAI(const std::string& fileName,
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
    ConstantPool::UpdateConstpoolWhenDeserialAI(vm_, aiCP, sharedCP, unsharedCP);
}

JSTaggedValue EcmaContext::FindCachedConstpoolAndLoadAiIfNeeded(const JSPandaFile *jsPandaFile, int32_t index)
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
    aotFileManager_->LoadAiFile(jsPandaFile);
    return constpool;
}

JSHandle<ConstantPool> EcmaContext::FindOrCreateConstPool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    JSTaggedValue constpool = FindCachedConstpoolAndLoadAiIfNeeded(jsPandaFile, index);
    if (constpool.IsHole()) {
        JSHandle<ConstantPool> newConstpool = ConstantPool::CreateUnSharedConstPool(vm_, jsPandaFile, id);
        JSHandle<ConstantPool> newSConstpool;
        if (jsPandaFile->IsLoadedAOT()) {
            AotConstantpoolPatcher::SetObjectFunctionFromConstPool(thread_, newConstpool);
            newSConstpool = ConstantPool::CreateSharedConstPoolForAOT(vm_, newConstpool, index);
        } else {
            newSConstpool = ConstantPool::CreateSharedConstPool(vm_, jsPandaFile, id, index);
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

void EcmaContext::CreateAllConstpool(const JSPandaFile *jsPandaFile)
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

JSHandle<JSTaggedValue> EcmaContext::GetAndClearEcmaUncaughtException() const
{
    JSHandle<JSTaggedValue> exceptionHandle = GetEcmaUncaughtException();
    thread_->ClearException();  // clear for ohos app
    return exceptionHandle;
}

void EcmaContext::RelocateConstantString(const JSPandaFile *jsPandaFile)
{
    if (!jsPandaFile->IsFirstMergedAbc()) {
        return;
    }
    vm_->GetEcmaStringTable()->RelocateConstantData(vm_, jsPandaFile);
}

JSHandle<JSTaggedValue> EcmaContext::GetEcmaUncaughtException() const
{
    if (!thread_->HasPendingException()) {
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> exceptionHandle(thread_, thread_->GetException());
    return exceptionHandle;
}

void EcmaContext::EnableUserUncaughtErrorHandler()
{
    isUncaughtExceptionRegistered_ = true;
}

void EcmaContext::HandleUncaughtException(JSTaggedValue exception)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    JSHandle<JSTaggedValue> exceptionHandle(thread_, exception);
    if (isUncaughtExceptionRegistered_) {
        if (vm_->GetJSThread()->IsMainThread()) {
            return;
        }
        auto callback = vm_->GetOnErrorCallback();
        if (callback) {
            thread_->ClearException();
            Local<ObjectRef> exceptionRef = JSNApiHelper::ToLocal<ObjectRef>(exceptionHandle);
            callback(exceptionRef, vm_->GetOnAllData());
        }
    }
    // if caught exceptionHandle type is JSError
    thread_->ClearException();
    if (exceptionHandle->IsJSError()) {
        PrintJSErrorInfo(thread_, exceptionHandle);
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(thread_, exceptionHandle);
    CString string = ConvertToString(*result);
    LOG_NO_TAG(ERROR) << string;
}

void EcmaContext::HandleUncaughtException()
{
    if (!thread_->HasPendingException()) {
        return;
    }
    JSTaggedValue exception = thread_->GetException();
    HandleUncaughtException(exception);
}

// static
void EcmaContext::PrintJSErrorInfo(JSThread *thread, const JSHandle<JSTaggedValue> &exceptionInfo)
{
    CString nameBuffer = GetJSErrorInfo(thread, exceptionInfo, JSErrorProps::NAME);
    CString msgBuffer = GetJSErrorInfo(thread, exceptionInfo, JSErrorProps::MESSAGE);
    CString stackBuffer = GetJSErrorInfo(thread, exceptionInfo, JSErrorProps::STACK);
    LOG_NO_TAG(ERROR) << panda::ecmascript::previewerTag << nameBuffer << ": " << msgBuffer << "\n"
                      << (panda::ecmascript::previewerTag.empty()
                              ? stackBuffer
                              : std::regex_replace(stackBuffer, std::regex(".+(\n|$)"),
                                                   panda::ecmascript::previewerTag + "$0"));
}

CString EcmaContext::GetJSErrorInfo(JSThread *thread, const JSHandle<JSTaggedValue> exceptionInfo, JSErrorProps key)
{
    JSHandle<JSTaggedValue> keyStr(thread, JSTaggedValue::Undefined());
    switch (key) {
        case JSErrorProps::NAME:
            keyStr = thread->GlobalConstants()->GetHandledNameString();
            break;
        case JSErrorProps::MESSAGE:
            keyStr = thread->GlobalConstants()->GetHandledMessageString();
            break;
        case JSErrorProps::STACK:
            keyStr = thread->GlobalConstants()->GetHandledStackString();
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable " << key;
            UNREACHABLE();
    }
    JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, exceptionInfo, keyStr).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, CString());
    JSHandle<EcmaString> errStr = JSTaggedValue::ToString(thread, value);
    // JSTaggedValue::ToString may cause exception. In this case, do not return, use "<error>" instead.
    if (thread->HasPendingException()) {
        thread->ClearException();
        errStr = thread->GetEcmaVM()->GetFactory()->NewFromStdString("<error>");
    }
    return ConvertToString(*errStr);
}

bool EcmaContext::HasPendingJob()
{
    // This interface only determines whether PromiseJobQueue is empty, rather than ScriptJobQueue.
    if (UNLIKELY(thread_->HasTerminated())) {
        return false;
    }
    TaggedQueue* promiseQueue = TaggedQueue::Cast(GetMicroJobQueue()->GetPromiseJobQueue().GetTaggedObject());
    return !promiseQueue->Empty();
}

bool EcmaContext::ExecutePromisePendingJob()
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

void EcmaContext::ClearBufferData()
{
    cachedSharedConstpools_.clear();
    thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(nullptr));
    thread_->SetUnsharedConstpoolsArrayLen(0);
}

void EcmaContext::SetGlobalEnv(GlobalEnv *global)
{
    // In jsthread iteration, SwitchCurrentContext is called to iterate each context.
    // If the target context is not fully initialized, the variable "global" will be nullptr.
    if (global != nullptr) {
        globalEnv_ = JSTaggedValue(global);
    }
}

void EcmaContext::SetMicroJobQueue(job::MicroJobQueue *queue)
{
    ASSERT(queue != nullptr);
    microJobQueue_ = JSTaggedValue(queue);
}

JSHandle<GlobalEnv> EcmaContext::GetGlobalEnv() const
{
    return JSHandle<GlobalEnv>(reinterpret_cast<uintptr_t>(&globalEnv_));
}

JSHandle<job::MicroJobQueue> EcmaContext::GetMicroJobQueue() const
{
    return JSHandle<job::MicroJobQueue>(reinterpret_cast<uintptr_t>(&microJobQueue_));
}

void EcmaContext::MountContext(JSThread *thread)
{
    EcmaContext *context = EcmaContext::CreateAndInitialize(thread);
    thread->SwitchCurrentContext(context);
}

void EcmaContext::UnmountContext(JSThread *thread)
{
    EcmaContext *context = thread->GetCurrentEcmaContext();
    thread->PopContext();
    Destroy(context);
}

EcmaContext *EcmaContext::CreateAndInitialize(JSThread *thread)
{
    EcmaContext *context = EcmaContext::Create(thread);
    thread->PushContext(context);
    context->Initialize();
    return context;
}

void EcmaContext::CheckAndDestroy(JSThread *thread, EcmaContext *context)
{
    if (thread->EraseContext(context)) {
        Destroy(context);
        return;
    }
    LOG_ECMA(FATAL) << "CheckAndDestroy a nonexistent context.";
}

void EcmaContext::SetupRegExpResultCache()
{
    regexpCache_ = builtins::RegExpExecResultCache::CreateCacheTable(thread_);
}

void EcmaContext::SetupRegExpGlobalResult()
{
    regexpGlobal_ = builtins::RegExpGlobalResult::CreateGlobalResultTable(thread_);
}

void EcmaContext::SetupNumberToStringResultCache()
{
    numberToStringResultCache_ = builtins::NumberToStringResultCache::CreateCacheTable(thread_);
}

void EcmaContext::SetupStringSplitResultCache()
{
    stringSplitResultCache_ = builtins::StringSplitResultCache::CreateCacheTable(thread_);
}

void EcmaContext::SetupStringToListResultCache()
{
    stringToListResultCache_ = builtins::StringToListResultCache::CreateCacheTable(thread_);
}

void EcmaContext::IterateMegaIC(RootVisitor &v)
{
    if (ecmaData_.loadMegaICCache_ != nullptr) {
        ecmaData_.loadMegaICCache_->Iterate(v);
    }
    if (ecmaData_.storeMegaICCache_ != nullptr) {
        ecmaData_.storeMegaICCache_->Iterate(v);
    }
}

void EcmaContext::Iterate(RootVisitor &v)
{
    // visit global Constant
    globalConst_.Iterate(v);

    v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalEnv_)));
    if (!regexpCache_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpCache_)));
    }
    if (!regexpGlobal_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpGlobal_)));
    }
    if (!numberToStringResultCache_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&numberToStringResultCache_)));
    }
    if (!stringSplitResultCache_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&stringSplitResultCache_)));
    }
    if (!stringToListResultCache_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&stringToListResultCache_)));
    }
    if (!microJobQueue_.IsHole()) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&microJobQueue_)));
    }

    if (functionProtoTransitionTable_) {
        functionProtoTransitionTable_->Iterate(v);
    }
    if (moduleManager_) {
        moduleManager_->Iterate(v);
    }
    if (ptManager_) {
        ptManager_->Iterate(v);
    }
    if (ecmaData_.propertiesCache_ != nullptr) {
        ecmaData_.propertiesCache_->Clear();
    }
    if (regExpParserCache_ != nullptr) {
        regExpParserCache_->Clear();
    }
    IterateMegaIC(v);
    if (!vm_->GetJSOptions().EnableGlobalLeakCheck() && currentHandleStorageIndex_ != -1) {
        // IterateHandle when disableGlobalLeakCheck.
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            v.VisitRangeRoot(Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
        }
    }

    if (sustainingJSHandleList_) {
        sustainingJSHandleList_->Iterate(v);
    }

    if (!joinStack_.empty()) {
        v.VisitRangeRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&joinStack_.front())),
            ObjectSlot(ToUintPtr(&joinStack_.back()) + JSTaggedValue::TaggedTypeSize()));
    }

    auto start = ObjectSlot(ToUintPtr(unsharedConstpools_));
    auto end = ObjectSlot(ToUintPtr(&unsharedConstpools_[GetUnsharedConstpoolsArrayLen() - 1]) +
        JSTaggedValue::TaggedTypeSize());
    v.VisitRangeRoot(Root::ROOT_VM, start, end);
}

size_t EcmaContext::IterateHandle(RootVisitor &visitor)
{
    // EnableGlobalLeakCheck.
    size_t handleCount = 0;
    if (currentHandleStorageIndex_ != -1) {
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

uintptr_t *EcmaContext::ExpandHandleStorage()
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

void EcmaContext::ShrinkHandleStorage(int prevIndex)
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

uintptr_t *EcmaContext::ExpandPrimitiveStorage()
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

void EcmaContext::ShrinkPrimitiveStorage(int prevIndex)
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

void EcmaContext::LoadStubFile()
{
    std::string stubFile = "";
    if (vm_->GetJSOptions().WasStubFileSet()) {
        stubFile = vm_->GetJSOptions().GetStubFile();
    }
    aotFileManager_->LoadStubFile(stubFile);
}

bool EcmaContext::LoadAOTFilesInternal(const std::string& aotFileName)
{
#ifdef AOT_ESCAPE_ENABLE
    std::string bundleName = pgo::PGOProfilerManager::GetInstance()->GetBundleName();
    if (AotCrashInfo::GetInstance().IsAotEscapedOrNotInEnableList(vm_, bundleName)) {
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

bool EcmaContext::LoadAOTFiles(const std::string& aotFileName)
{
    return LoadAOTFilesInternal(aotFileName);
}

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
bool EcmaContext::LoadAOTFiles(const std::string& aotFileName,
                               std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb)
{
    aotFileManager_->SetJsAotReader(cb);
    return LoadAOTFilesInternal(aotFileName);
}
#endif

void EcmaContext::PrintOptStat()
{
    if (optCodeProfiler_ != nullptr) {
        optCodeProfiler_->PrintAndReset();
    }
}

void EcmaContext::DumpAOTInfo() const
{
    aotFileManager_->DumpAOTInfo();
}

bool EcmaContext::JoinStackPushFastPath(JSHandle<JSTaggedValue> receiver)
{
    if (JSTaggedValue::SameValue(joinStack_[0], JSTaggedValue::Hole())) {
        joinStack_[0] = receiver.GetTaggedValue();
        return true;
    }
    return JoinStackPush(receiver);
}

bool EcmaContext::JoinStackPush(JSHandle<JSTaggedValue> receiver)
{
    uint32_t capacity = joinStack_.size();
    JSTaggedValue receiverValue = receiver.GetTaggedValue();
    for (size_t i = 0; i < capacity; ++i) {
        if (JSTaggedValue::SameValue(joinStack_[i], JSTaggedValue::Hole())) {
            joinStack_[i] = receiverValue;
            return true;
        }
        if (JSTaggedValue::SameValue(joinStack_[i], receiverValue)) {
            return false;
        }
    }
    joinStack_.emplace_back(receiverValue);
    return true;
}

void EcmaContext::JoinStackPopFastPath(JSHandle<JSTaggedValue> receiver)
{
    uint32_t length = joinStack_.size();
    if (JSTaggedValue::SameValue(joinStack_[0], receiver.GetTaggedValue()) && length == MIN_JOIN_STACK_SIZE) {
        joinStack_[0] = JSTaggedValue::Hole();
    } else {
        JoinStackPop(receiver);
    }
}

void EcmaContext::JoinStackPop(JSHandle<JSTaggedValue> receiver)
{
    uint32_t length = joinStack_.size();
    for (size_t i = 0; i < length; ++i) {
        if (JSTaggedValue::SameValue(joinStack_[i], receiver.GetTaggedValue())) {
            if (i == 0 && length > MIN_JOIN_STACK_SIZE) {
                joinStack_ = {JSTaggedValue::Hole(), JSTaggedValue::Hole()};
                break;
            } else {
                joinStack_[i] = JSTaggedValue::Hole();
                break;
            }
        }
    }
}

std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> EcmaContext::CalCallSiteInfo(
    uintptr_t retAddr, bool isDeopt) const
{
    auto loader = aotFileManager_;
    return loader->CalCallSiteInfo(retAddr, isDeopt);
}

void EcmaContext::AddSustainingJSHandle(SustainingJSHandle *sustainingHandle)
{
    if (sustainingJSHandleList_) {
        sustainingJSHandleList_->AddSustainingJSHandle(sustainingHandle);
    }
}

void EcmaContext::RemoveSustainingJSHandle(SustainingJSHandle *sustainingHandle)
{
    if (sustainingJSHandleList_) {
        sustainingJSHandleList_->RemoveSustainingJSHandle(sustainingHandle);
    }
}

void EcmaContext::ClearKeptObjects()
{
    if (LIKELY(GetGlobalEnv()->GetTaggedWeakRefKeepObjects().IsUndefined())) {
        return;
    }
    GetGlobalEnv()->SetWeakRefKeepObjects(thread_, JSTaggedValue::Undefined());
    hasKeptObjects_ = false;
}

void EcmaContext::AddToKeptObjects(JSHandle<JSTaggedValue> value)
{
    if (value->IsInSharedHeap()) {
        return;
    }

    JSHandle<GlobalEnv> globalEnv = GetGlobalEnv();
    JSHandle<LinkedHashSet> linkedSet;
    if (globalEnv->GetWeakRefKeepObjects()->IsUndefined()) {
        linkedSet = LinkedHashSet::Create(thread_);
    } else {
        linkedSet = JSHandle<LinkedHashSet>(thread_,
            LinkedHashSet::Cast(globalEnv->GetWeakRefKeepObjects()->GetTaggedObject()));
    }
    linkedSet = LinkedHashSet::Add(thread_, linkedSet, value);
    globalEnv->SetWeakRefKeepObjects(thread_, linkedSet);
    hasKeptObjects_ = true;
}
}  // namespace panda::ecmascript
