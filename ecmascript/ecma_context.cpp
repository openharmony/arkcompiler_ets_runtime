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
#include "ecmascript/platform/aot_crash_info.h"
#include "ecmascript/platform/ecma_context.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/sustaining_js_handle.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

EcmaContext::EcmaContext(JSThread* thread): thread_(thread), vm_(thread->GetEcmaVM()), factory_(vm_->GetFactory())
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
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hClassHandle = JSHandle<JSHClass>(thread_, globalConst->GetHClassClass());
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
    initialized_ = true;
    return true;
}

EcmaContext::~EcmaContext()
{
    if (vm_->IsEnableBaselineJit() || vm_->IsEnableFastJit()) {
        // clear jit task
        vm_->GetJit()->ClearTask(this);
    }

    // clear c_address: c++ pointer delete
    if (!vm_->IsBundlePack()) {
        std::shared_ptr<JSPandaFile> jsPandaFile =
            JSPandaFileManager::GetInstance()->FindJSPandaFile(vm_->GetAssetPath());
        if (jsPandaFile != nullptr) {
            jsPandaFile->DeleteParsedConstpoolVM(vm_);
        }
    }
}

void EcmaContext::SetGlobalEnv(GlobalEnv *global)
{
    // In jsthread iteration, SwitchCurrentContext is called to iterate each context.
    // If the target context is not fully initialized, the variable "global" will be nullptr.
    if (global != nullptr) {
        globalEnv_ = JSTaggedValue(global);
    }
}

JSHandle<GlobalEnv> EcmaContext::GetGlobalEnv() const
{
    return JSHandle<GlobalEnv>(reinterpret_cast<uintptr_t>(&globalEnv_));
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

void EcmaContext::Iterate(RootVisitor &v)
{
    v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalEnv_)));
}

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
bool EcmaContext::LoadAOTFiles(const std::string& aotFileName,
                               std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb)
{
    vm_->GetAOTFileManager()->SetJsAotReader(cb);
    return vm_->LoadAOTFilesInternal(aotFileName);
}
#endif

void EcmaContext::ClearKeptObjects()
{
    if (LIKELY(GetGlobalEnv()->GetTaggedWeakRefKeepObjects().IsUndefined())) {
        return;
    }
    GetGlobalEnv()->SetWeakRefKeepObjects(thread_, JSTaggedValue::Undefined());
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
}
}  // namespace panda::ecmascript
