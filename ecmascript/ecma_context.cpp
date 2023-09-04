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

#include "ecmascript/ecma_context.h"

#include "ecmascript/base/path_helper.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/snapshot/mem/snapshot.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

EcmaContext::EcmaContext(JSThread *thread)
    : thread_(thread),
      vm_(thread->GetEcmaVM()),
      factory_(vm_->GetFactory())
{
}

/* static */
EcmaContext *EcmaContext::Create(JSThread *thread)
{
    LOG_ECMA(INFO) << "EcmaContext::Create";
    auto context = new EcmaContext(thread);
    if (UNLIKELY(context == nullptr)) {
        LOG_ECMA(ERROR) << "Failed to create ecma context";
        return nullptr;
    }
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
    aotFileManager_ = new AOTFileManager(vm_);
    propertiesCache_ = new PropertiesCache();
    regExpParserCache_ = new RegExpParserCache();

    JSHandle<JSHClass> hClassHandle = factory_->InitClassClass();

    JSHandle<JSHClass> globalEnvClass = factory_->NewEcmaHClass(
        *hClassHandle,
        GlobalEnv::SIZE,
        JSType::GLOBAL_ENV);
    thread_->SetGlobalConst(&globalConst_);
    globalConst_.Init(thread_, *hClassHandle);
    auto arrayHClassIndexMaps = Elements::InitializeHClassMap();
    thread_->SetArrayHClassIndexMap(arrayHClassIndexMaps);

    JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(*globalEnvClass);
    globalEnv->Init(thread_);
    globalEnv_ = globalEnv.GetTaggedValue();
    Builtins builtins;
    bool builtinsLazyEnabled = vm_->GetJSOptions().IsWorker() && vm_->GetJSOptions().GetEnableBuiltinsLazy();
    builtins.Initialize(globalEnv, thread_, builtinsLazyEnabled);

    SetupRegExpResultCache();
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    moduleManager_ = new ModuleManager(vm_);
    tsManager_ = new TSManager(vm_);
    optCodeProfiler_ = new OptCodeProfiler();
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
    handleScopeCount_ = 0;
    handleScopeStorageNext_ = handleScopeStorageEnd_ = nullptr;
    ClearBufferData();
    // clear c_address: c++ pointer delete
    if (!vm_->IsBundlePack()) {
        std::shared_ptr<JSPandaFile> jsPandaFile =
            JSPandaFileManager::GetInstance()->FindJSPandaFile(vm_->GetAssetPath());
        if (jsPandaFile != nullptr) {
            jsPandaFile->DeleteParsedConstpoolVM(vm_);
        }
    }
    // clear icu cache
    ClearIcuCache();

    if (runtimeStat_ != nullptr) {
        vm_->GetChunk()->Delete(runtimeStat_);
        runtimeStat_ = nullptr;
    }
    if (optCodeProfiler_ != nullptr) {
        delete optCodeProfiler_;
        optCodeProfiler_ = nullptr;
    }
    if (moduleManager_ != nullptr) {
        delete moduleManager_;
        moduleManager_ = nullptr;
    }
    if (tsManager_ != nullptr) {
        delete tsManager_;
        tsManager_ = nullptr;
    }
    if (regExpParserCache_ != nullptr) {
        delete regExpParserCache_;
        regExpParserCache_ = nullptr;
    }
    if (aotFileManager_ != nullptr) {
        delete aotFileManager_;
        aotFileManager_ = nullptr;
    }
    if (propertiesCache_ != nullptr) {
        delete propertiesCache_;
        propertiesCache_ = nullptr;
    }
    // clear join stack
    joinStack_.clear();

    for (auto v : stringifyCache_) {
        v.clear();
    }
}

JSTaggedValue EcmaContext::InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                                   const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                                   CJSInfo* cjsInfo)
{
    aotFileManager_->SetAOTMainFuncEntry(mainFunc, jsPandaFile, entryPoint);
    return JSFunction::InvokeOptimizedEntrypoint(thread_, mainFunc, thisArg, entryPoint, cjsInfo);
}

JSTaggedValue EcmaContext::ExecuteAot(size_t actualNumArgs, JSTaggedType *args,
                                      const JSTaggedType *prevFp, bool needPushUndefined)
{
    INTERPRETER_TRACE(thread_, ExecuteAot);
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    // do not modify this log to INFO, this will call many times
    LOG_ECMA(DEBUG) << "start to execute aot entry: " << (void*)entry;
    auto res = reinterpret_cast<JSFunctionEntryType>(entry)(thread_->GetGlueAddr(),
                                                            actualNumArgs,
                                                            args,
                                                            reinterpret_cast<uintptr_t>(prevFp),
                                                            needPushUndefined);
    return res;
}

Expected<JSTaggedValue, bool> EcmaContext::InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
                                                                std::string_view entryPoint, bool excuteFromJob)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program = JSPandaFileManager::GetInstance()->GenerateProgram(vm_, jsPandaFile, entryPoint);
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(false);
    }
    // for debugger
    vm_->GetJsDebuggerManager()->GetNotificationManager()->LoadModuleEvent(
        jsPandaFile->GetJSPandaFileDesc(), entryPoint);

    JSHandle<JSFunction> func(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    CString moduleName = jsPandaFile->GetJSPandaFileDesc();
    CString entry = entryPoint.data();
    if (jsPandaFile->IsMergedPF()) {
        moduleName = entry;
    }
    JSRecordInfo recordInfo;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, recordInfo);
    if (!hasRecord) {
        CString msg = "cannot find record '" + entry + "', please check the request path.";
        LOG_FULL(ERROR) << msg;
        THROW_REFERENCE_ERROR_AND_RETURN(thread_, msg.c_str(), Unexpected(false));
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        global = undefined;
        JSHandle<SourceTextModule> module = moduleManager_->HostGetImportedModule(moduleName);
        func->SetModule(thread_, module);
    } else {
        // if it is Cjs at present, the module slot of the function is not used. We borrow it to store the recordName,
        // which can avoid the problem of larger memory caused by the new slot
        JSHandle<EcmaString> recordName = factory_->NewFromUtf8(moduleName);
        func->SetModule(thread_, recordName);
    }
    vm_->CheckStartCpuProfiler();

    JSTaggedValue result;
    if (jsPandaFile->IsCjs(recordInfo)) {
        CJSExecution(func, global, jsPandaFile, entryPoint);
    } else {
        if (aotFileManager_->IsLoadMain(jsPandaFile, entry)) {
            EcmaRuntimeStatScope runtimeStatScope(vm_);
            result = InvokeEcmaAotEntrypoint(func, global, jsPandaFile, entryPoint);
        } else {
            if (thread_->IsPGOProfilerEnable()) {
                vm_->GetPGOProfiler()->ProfileCall(JSTaggedValue::VALUE_UNDEFINED, func.GetTaggedType());
            }
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(vm_);
            EcmaInterpreter::Execute(info);
        }
    }
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
    }

    // print exception information
    if (!excuteFromJob && thread_->HasPendingException()) {
        HandleUncaughtException(thread_->GetException());
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
    JSMutableHandle<JSTaggedValue> filename(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> dirname(thread_, JSTaggedValue::Undefined());
    // Cjs's module slot of the function stores the recordName.
    filename.Update(func->GetModule());
    CString fullName = ConvertToString(filename.GetTaggedValue());
    dirname.Update(PathHelper::ResolveDirPath(thread_, fullName));
    CJSInfo cjsInfo(module, require, exports, filename, dirname);
    RequireManager::InitializeCommonJS(thread_, cjsInfo);
    if (aotFileManager_->IsLoadMain(jsPandaFile, entryPoint.data())) {
        EcmaRuntimeStatScope runtimeStateScope(vm_);
        isAotEntry_ = true;
        InvokeEcmaAotEntrypoint(func, thisArg, jsPandaFile, entryPoint, &cjsInfo);
    } else {
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
        EcmaRuntimeStatScope runtimeStatScope(vm_);
        EcmaInterpreter::Execute(info);
    }
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, thread_->GetCurrentEcmaContext()->GetMicroJobQueue());
    }

    if (!thread_->HasPendingException()) {
        // Collecting module.exports : exports ---> module.exports --->Module._cache
        RequireManager::CollectExecutedExp(thread_, cjsInfo);
    }
}

bool EcmaContext::HasCachedConstpool(const JSPandaFile *jsPandaFile) const
{
    return cachedConstpools_.find(jsPandaFile) != cachedConstpools_.end();
}

JSTaggedValue EcmaContext::FindConstpool(const JSPandaFile *jsPandaFile, int32_t index)
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

std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> EcmaContext::FindConstpools(
    const JSPandaFile *jsPandaFile)
{
    auto iter = cachedConstpools_.find(jsPandaFile);
    if (iter == cachedConstpools_.end()) {
        return std::nullopt;
    }
    return iter->second;
}

// For new version instruction.
JSTaggedValue EcmaContext::FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    return FindConstpool(jsPandaFile, index);
}

JSHandle<ConstantPool> EcmaContext::FindOrCreateConstPool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    JSTaggedValue constpool = FindConstpool(jsPandaFile, index);
    // In the taskpool thread, there is a case where the Function object is serialized before InitForCurrentThread.
    // A constpool is created when a Function is serialized. Slowpath, the default deserialized constpool,
    // string is non-lazy load mode. A hole is returned if you access the constpool of the serialized Function
    if (constpool.IsHole() && ecmascript::AnFileDataManager::GetInstance()->IsEnable()) {
        bool result = GetAOTFileManager()->LoadAiFile(jsPandaFile);
        if (result) {
            constpool = FindConstpool(jsPandaFile, index);
        }
    }
    if (constpool.IsHole()) {
        JSHandle<ConstantPool> newConstpool = ConstantPool::CreateConstPool(vm_, jsPandaFile, id);
        AddConstpool(jsPandaFile, newConstpool.GetTaggedValue(), index);
        return newConstpool;
    }

    return JSHandle<ConstantPool>(thread_, constpool);
}

void EcmaContext::CreateAllConstpool(const JSPandaFile *jsPandaFile)
{
    auto headers = jsPandaFile->GetPandaFile()->GetIndexHeaders();
    uint32_t index = 0;
    for (const auto &header : headers) {
        auto constpoolSize = header.method_idx_size;
        JSHandle<ConstantPool> constpool = factory_->NewConstantPool(constpoolSize);
        constpool->SetJSPandaFile(jsPandaFile);
        constpool->SetIndexHeader(&header);
        AddConstpool(jsPandaFile, constpool.GetTaggedValue(), index++);
    }
}

void EcmaContext::AddConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool, int32_t index)
{
    ASSERT(constpool.IsConstantPool());
    if (cachedConstpools_.find(jsPandaFile) == cachedConstpools_.end()) {
        cachedConstpools_[jsPandaFile] = CMap<int32_t, JSTaggedValue>();
    }
    auto &constpoolMap = cachedConstpools_[jsPandaFile];
    ASSERT(constpoolMap.find(index) == constpoolMap.end());
    constpoolMap.insert({index, constpool});
}

JSHandle<JSTaggedValue> EcmaContext::GetAndClearEcmaUncaughtException() const
{
    JSHandle<JSTaggedValue> exceptionHandle = GetEcmaUncaughtException();
    thread_->ClearException();  // clear for ohos app
    return exceptionHandle;
}

void EcmaContext::ProcessNativeDelete(const WeakRootVisitor &visitor)
{
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
            LOG_ECMA(INFO) << "remove js pandafile by gc, file:" << iterator->first->GetJSPandaFileDesc();
            JSPandaFileManager::GetInstance()->RemoveJSPandaFileVm(vm_, iterator->first);
            iterator = cachedConstpools_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void EcmaContext::ProcessReferences(const WeakRootVisitor &visitor)
{
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
            LOG_ECMA(INFO) << "remove js pandafile by gc, file:" << iterator->first->GetJSPandaFileDesc();
            JSPandaFileManager::GetInstance()->RemoveJSPandaFileVm(vm_, iterator->first);
            iterator = cachedConstpools_.erase(iterator);
        } else {
            ++iterator;
        }
    }
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
    if (isUncaughtExceptionRegistered_) {
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    JSHandle<JSTaggedValue> exceptionHandle(thread_, exception);
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

// static
void EcmaContext::PrintJSErrorInfo(JSThread *thread, const JSHandle<JSTaggedValue> &exceptionInfo)
{
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> nameValue = JSObject::GetProperty(thread, exceptionInfo, nameKey).GetValue();
    JSHandle<EcmaString> name = JSTaggedValue::ToString(thread, nameValue);
    // JSTaggedValue::ToString may cause exception. In this case, do not return, use "<error>" instead.
    if (thread->HasPendingException()) {
        thread->ClearException();
        name = thread->GetEcmaVM()->GetFactory()->NewFromStdString("<error>");
    }
    JSHandle<JSTaggedValue> msgKey = thread->GlobalConstants()->GetHandledMessageString();
    JSHandle<JSTaggedValue> msgValue = JSObject::GetProperty(thread, exceptionInfo, msgKey).GetValue();
    JSHandle<EcmaString> msg = JSTaggedValue::ToString(thread, msgValue);
    // JSTaggedValue::ToString may cause exception. In this case, do not return, use "<error>" instead.
    if (thread->HasPendingException()) {
        thread->ClearException();
        msg = thread->GetEcmaVM()->GetFactory()->NewFromStdString("<error>");
    }
    JSHandle<JSTaggedValue> stackKey = thread->GlobalConstants()->GetHandledStackString();
    JSHandle<JSTaggedValue> stackValue = JSObject::GetProperty(thread, exceptionInfo, stackKey).GetValue();
    JSHandle<EcmaString> stack = JSTaggedValue::ToString(thread, stackValue);
    // JSTaggedValue::ToString may cause exception. In this case, do not return, use "<error>" instead.
    if (thread->HasPendingException()) {
        thread->ClearException();
        stack = thread->GetEcmaVM()->GetFactory()->NewFromStdString("<error>");
    }

    CString nameBuffer = ConvertToString(*name);
    CString msgBuffer = ConvertToString(*msg);
    CString stackBuffer = ConvertToString(*stack);
    LOG_NO_TAG(ERROR) << nameBuffer << ": " << msgBuffer << "\n" << stackBuffer;
}

bool EcmaContext::HasPendingJob()
{
    if (isProcessingPendingJob_) {
        return true;
    }
    return job::MicroJobQueue::HasPendingJob(thread_, GetMicroJobQueue());
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
        isProcessingPendingJob_ = false;
        return true;
    }
    return false;
}

void EcmaContext::ClearBufferData()
{
    auto iter = cachedConstpools_.begin();
    while (iter != cachedConstpools_.end()) {
        LOG_ECMA(INFO) << "remove js pandafile by vm destruct, file:" << iter->first->GetJSPandaFileDesc();
        JSPandaFileManager::GetInstance()->RemoveJSPandaFileVm(vm_, iter->first);
        iter++;
    }
    cachedConstpools_.clear();
}

void EcmaContext::SetGlobalEnv(GlobalEnv *global)
{
    ASSERT(global != nullptr);
    globalEnv_ = JSTaggedValue(global);
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

void EcmaContext::Iterate(const RootVisitor &v, const RootRangeVisitor &rv)
{
    // visit global Constant
    globalConst_.VisitRangeSlot(rv);

    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalEnv_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpCache_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&microJobQueue_)));
    if (moduleManager_) {
        moduleManager_->Iterate(v);
    }
    if (tsManager_) {
        tsManager_->Iterate(v);
    }
    if (aotFileManager_) {
        aotFileManager_->Iterate(v);
    }
    if (propertiesCache_ != nullptr) {
        propertiesCache_->Clear();
    }
    if (!vm_->GetJSOptions().EnableGlobalLeakCheck() && currentHandleStorageIndex_ != -1) {
        // IterateHandle when disableGlobalLeakCheck.
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            rv(ecmascript::Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
        }
    }

    if (!joinStack_.empty()) {
        rv(Root::ROOT_VM, ObjectSlot(ToUintPtr(&joinStack_.front())),
            ObjectSlot(ToUintPtr(&joinStack_.back()) + JSTaggedValue::TaggedTypeSize()));
    }
}

size_t EcmaContext::IterateHandle(const RootRangeVisitor &rangeVisitor)
{
    // EnableGlobalLeakCheck.
    size_t handleCount = 0;
    if (currentHandleStorageIndex_ != -1) {
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            rangeVisitor(ecmascript::Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
            handleCount += (ToUintPtr(end) - ToUintPtr(start)) / sizeof(JSTaggedType);
        }
    }
    return handleCount;
}

uintptr_t *EcmaContext::ExpandHandleStorage()
{
    uintptr_t *result = nullptr;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size() - 1);
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
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size() - 1);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    uintptr_t size = ToUintPtr(handleScopeStorageEnd_) - ToUintPtr(handleScopeStorageNext_);
    if (memset_s(handleScopeStorageNext_, size, 0, size) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
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

void EcmaContext::LoadStubFile()
{
    std::string stubFile = vm_->GetJSOptions().GetStubFile();
    aotFileManager_->LoadStubFile(stubFile);
}

bool EcmaContext::LoadAOTFiles(const std::string& aotFileName)
{
    std::string anFile = aotFileName + AOTFileManager::FILE_EXTENSION_AN;
    if (!aotFileManager_->LoadAnFile(anFile)) {
        LOG_ECMA(ERROR) << "Load " << anFile << " failed. Destroy aot data and rollback to interpreter";
        ecmascript::AnFileDataManager::GetInstance()->SafeDestroyAnData(anFile);
        return false;
    }

    std::string aiFile = aotFileName + AOTFileManager::FILE_EXTENSION_AI;
    if (!aotFileManager_->LoadAiFile(aiFile)) {
        LOG_ECMA(ERROR) << "Load " << aiFile << " failed. Destroy aot data and rollback to interpreter";
        ecmascript::AnFileDataManager::GetInstance()->SafeDestroyAnData(anFile);
        return false;
    }
    return true;
}

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
}  // namespace panda::ecmascript
