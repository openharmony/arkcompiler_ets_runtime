/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/runtime.h"
#include <memory>

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "libpandafile/index_accessor.h"

namespace panda::ecmascript {
using PGOProfilerManager = pgo::PGOProfilerManager;

int32_t Runtime::vmCount_ = 0;
int32_t Runtime::destroyCount_ = 0;
bool Runtime::firstVmCreated_ = false;
Mutex *Runtime::vmCreationLock_ = new Mutex();
Runtime *Runtime::instance_ = nullptr;

Runtime *Runtime::GetInstance()
{
    ASSERT(instance_ != nullptr);
    return instance_;
}

Runtime::~Runtime()
{
    LockHolder lock(constpoolLock_);
    auto iter = globalSharedConstpools_.begin();
    while (iter != globalSharedConstpools_.end()) {
        LOG_ECMA(INFO) << "remove js pandafile by vm destruct, file:" << iter->first->GetJSPandaFileDesc();
        JSPandaFileManager::GetInstance()->RemoveJSPandaFile(iter->first);
        iter->second.clear();
        iter++;
    }
    globalSharedConstpools_.clear();
}

void Runtime::CreateIfFirstVm(const JSRuntimeOptions &options)
{
    LockHolder lock(*vmCreationLock_);
    if (!firstVmCreated_) {
        Log::Initialize(options);
        EcmaVM::InitializeIcuData(options);
        MemMapAllocator::GetInstance()->Initialize(ecmascript::DEFAULT_REGION_SIZE);
        PGOProfilerManager::GetInstance()->Initialize(options.GetPGOProfilerPath(),
                                                      options.GetPGOHotnessThreshold());
        bool isEnableJit = options.IsEnableJIT() && options.GetEnableAsmInterpreter();
        Jit::GetInstance()->SetEnableOrDisable(options, isEnableJit);
        ASSERT(instance_ == nullptr);
        instance_ = new Runtime();
        firstVmCreated_ = true;
    }
}

void Runtime::InitializeIfFirstVm(EcmaVM *vm)
{
    {
        LockHolder lock(*vmCreationLock_);
        if (++vmCount_ == 1) {
            ThreadManagedScope managedScope(vm->GetAssociatedJSThread());
            PreInitialization(vm);
            vm->Initialize();
            PostInitialization(vm);
        }
    }
    if (!vm->IsInitialized()) {
        ThreadManagedScope managedScope(vm->GetAssociatedJSThread());
        vm->Initialize();
    }
}

void Runtime::PreInitialization(const EcmaVM *vm)
{
    mainThread_ = vm->GetAssociatedJSThread();
    nativeAreaAllocator_ = std::make_unique<NativeAreaAllocator>();
    heapRegionAllocator_ = std::make_unique<HeapRegionAllocator>();
    stringTable_ = std::make_unique<EcmaStringTable>();
    SharedHeap::GetInstance()->Initialize(nativeAreaAllocator_.get(), heapRegionAllocator_.get(),
        const_cast<EcmaVM*>(vm)->GetJSOptions());
}

void Runtime::PostInitialization(const EcmaVM *vm)
{
    // Use the main thread's globalconst after it has initialized,
    // and copy shared parts to other thread's later.
    globalConstants_ = mainThread_->GlobalConstants();
    globalEnv_ = vm->GetGlobalEnv().GetTaggedValue();
    SharedHeap::GetInstance()->PostInitialization(globalConstants_, const_cast<EcmaVM*>(vm)->GetJSOptions());
    // [[todo::DaiHN]] need adding root iterate.
    SharedModuleManager::GetInstance()->Initialize(vm);
}

void Runtime::DestroyIfLastVm()
{
    LockHolder lock(*vmCreationLock_);
    if (--vmCount_ <= 0) {
        AnFileDataManager::GetInstance()->SafeDestroyAllData();
        MemMapAllocator::GetInstance()->Finalize();
        PGOProfilerManager::GetInstance()->Destroy();
        ASSERT(instance_ != nullptr);
        delete instance_;
        instance_ = nullptr;
        firstVmCreated_ = false;
    }
}

void Runtime::RegisterThread(JSThread* newThread)
{
    LockHolder lock(threadsLock_);
    threads_.emplace_back(newThread);
    // send all current suspended requests to the new thread
    for (uint32_t i = 0; i < suspendNewCount_; i++) {
        newThread->SuspendThread(true);
    }
}

// Note: currently only called when thread is to be destroyed.
void Runtime::UnregisterThread(JSThread* thread)
{
    LockHolder lock(threadsLock_);
    ASSERT(!thread->IsInRunningState());
    threads_.remove(thread);
}

void Runtime::SuspendAll(JSThread *current)
{
    ASSERT(current != nullptr);
    ASSERT(!current->IsInRunningState());
    ASSERT(!mutatorLock_.HasLock());
    SuspendAllThreadsImpl(current);
    mutatorLock_.WriteLock();
}

void Runtime::ResumeAll(JSThread *current)
{
    ASSERT(current != nullptr);
    ASSERT(!current->IsInRunningState());
    ASSERT(mutatorLock_.HasLock());
    mutatorLock_.Unlock();
    ResumeAllThreadsImpl(current);
}

void Runtime::SuspendAllThreadsImpl(JSThread *current)
{
    LockHolder lock(threadsLock_);
    suspendNewCount_++;
    for (auto i : threads_) {
        if (i != current) {
            i->SuspendThread(true);
        }
    }
}

void Runtime::ResumeAllThreadsImpl(JSThread *current)
{
    LockHolder lock(threadsLock_);
    if (suspendNewCount_ > 0) {
        suspendNewCount_--;
    }
    for (auto i : threads_) {
        if (i != current) {
            i->ResumeThread(true);
        }
    }
}

void Runtime::IterateSerializeRoot(const RootVisitor &v)
{
    for (auto &it : serializeRootMap_) {
        for (auto &rootObj : it.second) {
            v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&rootObj)));
        }
    }
}

bool Runtime::HasCachedConstpool(const JSPandaFile *jsPandaFile)
{
    LockHolder lock(constpoolLock_);
    return globalSharedConstpools_.find(jsPandaFile) != globalSharedConstpools_.end();
}

JSTaggedValue Runtime::FindConstpool(const JSPandaFile *jsPandaFile, int32_t index)
{
    LockHolder lock(constpoolLock_);
    return FindConstpoolUnlocked(jsPandaFile, index);
}

JSTaggedValue Runtime::FindConstpoolUnlocked(const JSPandaFile *jsPandaFile, int32_t index)
{
    auto iter = globalSharedConstpools_.find(jsPandaFile);
    if (iter == globalSharedConstpools_.end()) {
        return JSTaggedValue::Hole();
    }
    auto constpoolIter = iter->second.find(index);
    if (constpoolIter == iter->second.end()) {
        return JSTaggedValue::Hole();
    }
    return constpoolIter->second;
}

void Runtime::AddConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool, int32_t index)
{
    LockHolder lock(constpoolLock_);
    ASSERT(constpool.IsConstantPool());
    if (globalSharedConstpools_.find(jsPandaFile) == globalSharedConstpools_.end()) {
        globalSharedConstpools_[jsPandaFile] = CMap<int32_t, JSTaggedValue>();
    }
    auto &constpoolMap = globalSharedConstpools_[jsPandaFile];
    ASSERT(constpoolMap.find(index) == constpoolMap.end());
    constpoolMap.insert({index, constpool});
}

std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> Runtime::FindConstpools(
    const JSPandaFile *jsPandaFile)
{
    LockHolder lock(constpoolLock_);
    auto iter = globalSharedConstpools_.find(jsPandaFile);
    if (iter == globalSharedConstpools_.end()) {
        return std::nullopt;
    }
    return iter->second;
}

void Runtime::ProcessNativeDeleteInSharedGC(const WeakRootVisitor &visitor)
{
    // No need lock here, only shared gc will sweep shared constpool, meanwhile other threads are suspended.
    auto iterator = globalSharedConstpools_.begin();
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "Constpools:" + std::to_string(globalSharedConstpools_.size()));
    while (iterator != globalSharedConstpools_.end()) {
        auto &constpools = iterator->second;
        auto constpoolIter = constpools.begin();
        while (constpoolIter != constpools.end()) {
            JSTaggedValue constpoolVal = constpoolIter->second;
            if (constpoolVal.IsHeapObject()) {
                TaggedObject *obj = constpoolVal.GetTaggedObject();
                auto fwd = visitor(obj);
                if (fwd == nullptr) {
                    int32_t constpoolIndex =
                        ConstantPool::Cast(constpoolVal.GetTaggedObject())->GetUnsharedConstpoolIndex().GetInt();
                    ASSERT(0 <= constpoolIndex && constpoolIndex != ConstantPool::CONSTPOOL_TYPE_FLAG &&
                        constpoolIndex < UNSHARED_CONSTANTPOOL_COUNT);
                    EraseUnusedConstpool(iterator->first, constpoolIter->first, constpoolIndex);
                    constpoolIter = constpools.erase(constpoolIter);
                    // when shared constpool is not referenced by any objects,
                    // global unshared constpool count can be reuse.
                    freeSharedConstpoolIndex_.insert(constpoolIndex);
                    continue;
                }
            }
            ++constpoolIter;
        }
        if (constpools.size() == 0) {
            LOG_ECMA(INFO) << "remove js pandafile by gc, file:" << iterator->first->GetJSPandaFileDesc();
            JSPandaFileManager::GetInstance()->RemoveJSPandaFile(iterator->first);
            iterator = globalSharedConstpools_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void Runtime::EraseUnusedConstpool(const JSPandaFile *jsPandaFile, int32_t index, int32_t constpoolIndex)
{
    LockHolder lock(threadsLock_);
    for (auto thread : threads_) {
        auto context = thread->GetCurrentEcmaContext();
        context->EraseUnusedConstpool(jsPandaFile, index, constpoolIndex);
    }
}
}  // namespace panda::ecmascript
