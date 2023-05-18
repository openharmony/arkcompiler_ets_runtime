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

#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot.h"
namespace panda::ecmascript {
EcmaContext::EcmaContext(JSThread *thread)
    : thread_(thread),
      vm_(thread->GetEcmaVM()),
      factory_(vm_->GetFactory()),
      stringTable_(new EcmaStringTable(vm_))
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
    context->Initialize();
    return context;
}

// static
bool EcmaContext::Destroy(EcmaContext *context)
{
    LOG_ECMA(INFO) << "EcmaContext::Destroy";
    if (context != nullptr) {
        delete context;
        context = nullptr;
        return true;
    }
    return false;
}

bool EcmaContext::Initialize()
{
    LOG_ECMA(INFO) << "EcmaContext::Initialize";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "EcmaContext::Initialize");
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    regExpParserCache_ = new RegExpParserCache();
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    if (!vm_->GetJSOptions().EnableSnapshotDeserialize()) {
        LOG_ECMA(DEBUG) << "EcmaVM::Initialize run builtins";
        JSHandle<JSHClass> hClassHandle = factory_->InitClassClass();
        JSHandle<JSHClass> globalEnvClass = factory_->NewEcmaHClass(*hClassHandle,
                                                                   GlobalEnv::SIZE,
                                                                   JSType::GLOBAL_ENV);
        globalConst->Init(thread_, *hClassHandle);
        JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(*globalEnvClass);
        globalEnv->Init(thread_);
        globalEnv_ = globalEnv.GetTaggedValue();
        thread_->SetGlueGlobalEnv(reinterpret_cast<GlobalEnv *>(globalEnv.GetTaggedType()));
        Builtins builtins;
        builtins.Initialize(globalEnv, thread_);
        if (!WIN_OR_MAC_OR_IOS_PLATFORM && vm_->GetJSOptions().EnableSnapshotSerialize()) {
            const CString fileName = "builtins.snapshot";
            Snapshot snapshot(vm_);
            snapshot.SerializeBuiltins(fileName);
        }
    } else {
        const CString fileName = "builtins.snapshot";
        Snapshot snapshot(vm_);
        if (!WIN_OR_MAC_OR_IOS_PLATFORM) {
            snapshot.Deserialize(SnapshotType::BUILTINS, fileName, true);
        }
        globalConst->InitSpecialForSnapshot();
        Builtins builtins;
       builtins.InitializeForSnapshot(thread_);
   }

    SetupRegExpResultCache();
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    moduleManager_ = new ModuleManager(vm_);
    tsManager_ = new TSManager(vm_);
    optCodeProfiler_ = new OptCodeProfiler();
    return true;
}

EcmaContext::~EcmaContext()
{
    LOG_ECMA(INFO) << "~EcmaContext";
    ClearBufferData();
    // clear c_address: c++ pointer delete
    if (!vm_->IsBundlePack()) {
        std::shared_ptr<JSPandaFile> jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(assetPath_);
        if (jsPandaFile != nullptr) {
            jsPandaFile->DeleteParsedConstpoolVM(vm_);
        }
    }
    // clear icu cache
    ClearIcuCache();

    if (optCodeProfiler_ != nullptr) {
        delete optCodeProfiler_;
        optCodeProfiler_ = nullptr;
    }
    if (stringTable_ != nullptr) {
        delete stringTable_;
        stringTable_ = nullptr;
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
    debuggerManager_->GetNotificationManager()->LoadModuleEvent(jsPandaFile->GetJSPandaFileDesc(), entryPoint);

    JSHandle<JSFunction> func(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    if (jsPandaFile->IsModule(thread_, entryPoint.data())) {
        global = undefined;
        CString moduleName = jsPandaFile->GetJSPandaFileDesc();
        if (!jsPandaFile->IsBundlePack()) {
            moduleName = entryPoint.data();
        }
        JSHandle<SourceTextModule> module = moduleManager_->HostGetImportedModule(moduleName);
        func->SetModule(thread_, module);
    } else {
        // if it is Cjs at present, the module slot of the function is not used. We borrow it to store the recordName,
        // which can avoid the problem of larger memory caused by the new slot
        JSHandle<EcmaString> recordName = factory_->NewFromUtf8(entryPoint.data());
        func->SetModule(thread_, recordName);
    }
    vm_->CheckStartCpuProfiler();

    JSTaggedValue result;
    if (jsPandaFile->IsCjs(thread_, entryPoint.data())) {
            if (!thread_->HasPendingException()) {
                vm_->CJSExecution(func, global, jsPandaFile, entryPoint);
            }
    } else {
        if (aotFileManager_->IsLoadMain(jsPandaFile, entryPoint.data())) {
            EcmaRuntimeStatScope runtimeStatScope(this);
            result = InvokeEcmaAotEntrypoint(func, global, jsPandaFile, entryPoint);
        } else {
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
        HandleUncaughtException(thread_->GetException());
    }
    return result;
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
    if (constpool.IsHole()) {
        // JSHandle<ConstantPool> newConstpool = ConstantPool::CreateConstPool(this, jsPandaFile, id);
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
        PrintJSErrorInfo(exceptionHandle);
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(thread_, exceptionHandle);
    CString string = ConvertToString(*result);
    LOG_NO_TAG(ERROR) << string;
}

void EcmaContext::PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo)
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
    EcmaContext *context = EcmaContext::Create(thread);
    thread->PushContext(context);
}

void EcmaContext::UnmountContext(JSThread *thread)
{
    EcmaContext *context = thread->GetCurrentEcmaContext();
    thread->PopContext();
    Destroy(context);
}


void EcmaContext::Iterate(const RootVisitor &v, [[maybe_unused]] const RootRangeVisitor &rv)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpCache_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalEnv_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&microJobQueue_)));
    moduleManager_->Iterate(v);
    tsManager_->Iterate(v);
}

void EcmaContext::SetupRegExpResultCache()
{
    regexpCache_ = builtins::RegExpExecResultCache::CreateCacheTable(thread_);
}
}  // namespace panda::ecmascript
