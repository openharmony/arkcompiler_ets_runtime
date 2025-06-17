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
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "common_interfaces/base_runtime.h"
#include "ecmascript/dynamic_object_accessor.h"
#include "ecmascript/dynamic_object_descriptor.h"
#include "ecmascript/dynamic_type_converter.h"
#include "common_interfaces/thread/thread_holder_manager.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/dynamic_object_operator.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/platform/parameters.h"

namespace panda::ecmascript {
using PGOProfilerManager = pgo::PGOProfilerManager;
using namespace panda;
int32_t Runtime::vmCount_ = 0;
int32_t Runtime::destroyCount_ = 0;
bool Runtime::firstVmCreated_ = false;
Mutex *Runtime::vmCreationLock_ = new Mutex();
Runtime *Runtime::instance_ = nullptr;
common::BaseRuntime *Runtime::baseInstance_ = nullptr;

Runtime::Runtime()
{
    if (baseInstance_ == nullptr) {
        baseInstance_ = common::BaseRuntime::GetInstance();
    }
}

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
        LOG_ECMA(INFO) << "remove js pandafile by vm destruct, file: " << iter->first->GetJSPandaFileDesc();
        JSPandaFileManager::GetInstance()->RemoveJSPandaFile(iter->first);
        iter->second.clear();
        iter++;
    }
    globalSharedConstpools_.clear();
    delete externalRegisteredStringTable_;
    externalRegisteredStringTable_ = nullptr;
}

void Runtime::CreateIfFirstVm(const JSRuntimeOptions &options)
{
    LockHolder lock(*vmCreationLock_);
    if (!firstVmCreated_) {
        InitGCConfig(options);
        common::Log::Initialize(options.GetLogOptions());
        EcmaVM::InitializeIcuData(options);
        MemMapAllocator::GetInstance()->Initialize(ecmascript::DEFAULT_REGION_SIZE, options.GetLargeHeap());
        PGOProfilerManager::GetInstance()->Initialize(options.GetPGOProfilerPath(),
                                                      options.GetPGOHotnessThreshold());
        ASSERT(instance_ == nullptr);
        instance_ = new Runtime();
        instance_->SetEnableLargeHeap(options.GetLargeHeap());
        SharedHeap::CreateNewInstance();
        if (g_isEnableCMCGC) {
            // Init common::BaseRuntime before daemon thread because creating mutator may access gcphase in heap
            LOG_ECMA(INFO) << "start run with cmc gc";
            common::BaseRuntime::GetInstance()->Init(options.GetRuntimeParam());
        }
        DaemonThread::CreateNewInstance();
        firstVmCreated_ = true;
    } else {
        if (g_isEnableCMCGC) {
            JSThread *mainThread = Runtime::GetInstance()->GetMainThread();
            ASSERT(mainThread != nullptr);
            EcmaVM *mainVM = mainThread->GetEcmaVM();
            ASSERT(mainVM != nullptr);
            if (mainVM->IsPreForked() && !mainVM->IsPostForked()) {
                LOG_ECMA(ERROR) << "create ecmavm after pre fork, but not post pork";
                ASSERT(!DaemonThread::GetInstance()->IsRunning());
                mainVM->PostFork();
            }
        }
    }
}

void Runtime::InitializeIfFirstVm(EcmaVM *vm)
{
    {
        LockHolder lock(*vmCreationLock_);
        if (++vmCount_ == 1) {
            ThreadManagedScope managedScope(vm->GetAssociatedJSThread());
            PreInitialization(vm);
            bool isEnableFastJit = vm->GetJSOptions().IsEnableJIT() && vm->GetJSOptions().GetEnableAsmInterpreter();
            bool isEnableBaselineJit =
                vm->GetJSOptions().IsEnableBaselineJIT() && vm->GetJSOptions().GetEnableAsmInterpreter();
            Jit::GetInstance()->SetEnableOrDisable(vm->GetJSOptions(), isEnableFastJit, isEnableBaselineJit);
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
    DynamicObjectOperator::Initialize();
    if (g_isEnableCMCGC) {
        DynamicObjectAccessor::Initialize();
        DynamicObjectDescriptor::Initialize();
        DynamicTypeConverter::Initialize();
    }
    mainThread_ = vm->GetAssociatedJSThread();
    mainThread_->SetMainThread();
    nativeAreaAllocator_ = std::make_unique<NativeAreaAllocator>();
    heapRegionAllocator_ = std::make_unique<HeapRegionAllocator>();
    if (g_isEnableCMCGC) {
        auto& baseStringTable = common::BaseRuntime::GetInstance()->GetStringTable();
        stringTable_ = std::make_unique<EcmaStringTable>(true, &baseStringTable,
                                                         &static_cast<common::BaseStringTableImpl*>(&baseStringTable)->
                                                         GetHashTrieMap());
    } else {
        stringTable_ = std::make_unique<EcmaStringTable>(false);
    }
    SharedHeap::GetInstance()->Initialize(nativeAreaAllocator_.get(), heapRegionAllocator_.get(),
        const_cast<EcmaVM*>(vm)->GetJSOptions(), DaemonThread::GetInstance());
}

void Runtime::PostInitialization(const EcmaVM *vm)
{
    // Use the main thread's globalconst after it has initialized,
    // and copy shared parts to other thread's later.
    sharedConstInited_ = true;
    globalEnv_ = vm->GetGlobalEnv().GetTaggedValue();
    globalConst_.CopySharedConstantsFrom(mainThread_->GlobalConstants());
    SharedHeap::GetInstance()->PostInitialization(&globalConst_, const_cast<EcmaVM*>(vm)->GetJSOptions());
    SharedModuleManager::GetInstance()->Initialize();
}

void Runtime::InitGCConfig(const JSRuntimeOptions &options)
{
    bool defaultValue = options.IsEnableCMCGC();
    g_isEnableCMCGC = IsEnableCMCGC(defaultValue);
    if (g_isEnableCMCGC) {
        g_maxRegularHeapObjectSize = 32_KB;
    }
}

void Runtime::DestroyIfLastVm()
{
    LockHolder lock(*vmCreationLock_);
    if (--vmCount_ <= 0) {
        Jit::GetInstance()->Destroy();
        SharedModuleManager::GetInstance()->SharedNativeObjDestory();
        SharedHeap::GetInstance()->WaitAllTasksFinishedAfterAllJSThreadEliminated();
        DaemonThread::DestroyInstance();
        if (g_isEnableCMCGC) {
            // Finish common::BaseRuntime after daemon thread because it will unregister mutator
            common::BaseRuntime::GetInstance()->Fini();
        }
        SharedHeap::DestroyInstance();
        AnFileDataManager::GetInstance()->SafeDestroyAllData();
        MemMapAllocator::GetInstance()->Finalize();
        PGOProfilerManager::GetInstance()->Destroy();
        SharedModuleManager::GetInstance()->Destroy();
        if (g_isEnableCMCGC) {
            // Destroy common::BaseRuntime after daemon thread because it will unregister mutator
            common::BaseRuntime::DestroyInstance();
            // reset Base address offset
            TaggedStateWord::BASE_ADDRESS = 0;
        }
        ASSERT(instance_ != nullptr);
        delete instance_;
        instance_ = nullptr;
        firstVmCreated_ = false;
    }
}

void Runtime::RegisterThread(JSThread* newThread)
{
    {
        LockHolder lock(threadsLock_);
        ASSERT(std::find(threads_.begin(), threads_.end(), newThread) == threads_.end());
        threads_.emplace_back(newThread);
    }
    if (g_isEnableCMCGC) {
        ThreadHolder* threadHolder = newThread->GetThreadHolder();
        threadHolder->BindMutator();
        newThread->SetAllocBuffer(threadHolder->GetAllocBuffer());
        threadHolder->RegisterJSThread(newThread);
    } else {
        // send all current suspended requests to the new thread
        for (uint32_t i = 0; i < suspendNewCount_; i++) {
            newThread->SuspendThread(true);
        }
    }
}

// Note: currently only called when thread is to be destroyed.
void Runtime::UnregisterThread(JSThread* thread)
{
    {
        LockHolder lock(threadsLock_);
        ASSERT(std::find(threads_.begin(), threads_.end(), thread) != threads_.end());
        ASSERT(!thread->IsInRunningState());
        threads_.remove(thread);
    }
    if (g_isEnableCMCGC) {
        ThreadHolder *holder = thread->GetThreadHolder();
        void *mutator = holder->GetMutator();
        ASSERT(mutator != nullptr);
        holder->UnregisterJSThread(thread);
        holder->UnbindMutator();
    }
}

void Runtime::SuspendAll(JSThread *current)
{
    ASSERT(current != nullptr);
    ASSERT(!current->IsInRunningState());
#ifndef NDEBUG
    ASSERT(!current->HasLaunchedSuspendAll());
    current->LaunchSuspendAll();
#endif
    SuspendAllThreadsImpl(current);
}

void Runtime::ResumeAll(JSThread *current)
{
    ASSERT(current != nullptr);
    ASSERT(!current->IsInRunningState());
#ifndef NDEBUG
    ASSERT(current->HasLaunchedSuspendAll());
    current->CompleteSuspendAll();
#endif
    ResumeAllThreadsImpl(current);
}

void Runtime::SuspendAllThreadsImpl(JSThread *current)
{
    if (g_isEnableCMCGC) {
        common::BaseRuntime::GetInstance()->GetThreadHolderManager().SuspendAll(current->GetThreadHolder());
        return;
    }
    SuspendBarrier barrier;
    for (uint32_t iterCount = 1U;; ++iterCount) {
        {
            LockHolder lock(threadsLock_);
            if (suspendNewCount_ == 0) {
                suspendNewCount_++;
                if (threads_.size() == 1) {
                    ASSERT(current == mainThread_ || current->IsDaemonThread());
                    return;
                }
                if (threads_.size() < 1) {
                    return;
                }
                barrier.Initialize(threads_.size() - 1);
                for (const auto& thread: threads_) {
                    if (thread == current) {
                        continue;
                    }
                    thread->SuspendThread(+1, &barrier);
                    // The two flags, SUSPEND_REQUEST and ACTIVE_BARRIER, are set by Suspend-Thread guarded by
                    // suspendLock_, so the target thread-I may do not see these two flags in time. As a result, it
                    // can switch its state freely without responding to the ACTIVE_BARRIER flag and the
                    // suspend-thread will always wait it. However, as long as it sees the flags, the actions of
                    // passing barrier will be triggered. When the thread-I switches from NON_RUNNING to RUNNING,
                    // it will firstly pass the barrier and then be blocked by the SUSPEND_REQUEST flag. If the
                    // thread-I switches from RUNNING to NON_RUNNING, it will switch the state and then act on the
                    // barrier. If the thread-I go to checkpoint in RUNNING state, it will act on the barrier
                    // and be blocked by SUSPEND_REQUEST flag.
                    if (thread->IsSuspended()) {
                        // Because of the multi-threads situation, currently thread-I may be in RUNNING state or is
                        // goding to be RUNNING state even inside this branch. In both scenarios, for instance of
                        // RUNNING state, according to the modification order of atomic-variable stateAndFlags_,
                        // thread-I will see the SUSPEND_REQUEST and ACTIVE_BARRIER and act on them before switching
                        // to RUNNING. Besides, notice the using of suspendLock_ inside PassSuspendBarrier(), there
                        // is not data-race for passing barrier.
                        thread->PassSuspendBarrier();
                    }
                }
                break;
            }
        }
        if (iterCount < MAX_SUSPEND_RETRIES) {
            LockHolder lock(threadsLock_);
            if (suspendNewCount_ != 0) {
                // Someone has already suspended all threads.
                // Wait until it finishes.
                threadSuspendCondVar_.Wait(&threadsLock_);
            }
        } else {
            LOG_ECMA(FATAL) << "Too many SuspendAll retries!";
            UNREACHABLE();
        }
    }
    barrier.Wait();
}

void Runtime::ResumeAllThreadsImpl(JSThread *current)
{
    if (g_isEnableCMCGC) {
        common::BaseRuntime::GetInstance()->GetThreadHolderManager().ResumeAll(current->GetThreadHolder());
    } else {
        LockHolder lock(threadsLock_);
        if (suspendNewCount_ > 0) {
            suspendNewCount_--;
        }
        if (suspendNewCount_ == 0) {
            // Signal to waiting to suspend threads
            threadSuspendCondVar_.Signal();
        }
        for (const auto& thread : threads_) {
            if (thread != current) {
                thread->ResumeThread(true);
            }
        }
    }
}

void Runtime::IterateSharedRoot(RootVisitor &visitor)
{
    IterateSerializeRoot(visitor);
    SharedModuleManager::GetInstance()->Iterate(visitor);
    IterateCachedStringRoot(visitor);
}

void Runtime::IterateSerializeRoot(RootVisitor &v)
{
    LockHolder lock(serializeLock_);
    for (auto &it : serializeRootMap_) {
        it.second->Iterate(v);
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

JSHandle<ConstantPool> Runtime::AddOrUpdateConstpool(const JSPandaFile *jsPandaFile,
                                                     JSHandle<ConstantPool> constpool,
                                                     int32_t index)
{
    LockHolder lock(constpoolLock_);
    if (globalSharedConstpools_.find(jsPandaFile) == globalSharedConstpools_.end()) {
        globalSharedConstpools_[jsPandaFile] = CMap<int32_t, JSTaggedValue>();
    }
    auto &constpoolMap = globalSharedConstpools_[jsPandaFile];
    auto iter = constpoolMap.find(index);
    if (iter == constpoolMap.end()) {
        int32_t constpoolIndex = GetAndIncreaseSharedConstpoolCount();
        constpool->SetUnsharedConstpoolIndex(JSTaggedValue(constpoolIndex));
        constpoolMap.insert({index, constpool.GetTaggedValue()});
        return constpool;
    }
    return JSHandle<ConstantPool>(reinterpret_cast<uintptr_t>(&iter->second));
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

void Runtime::IteratorNativeDeleteInSharedGC(WeakVisitor &visitor)
{
    auto iterator = globalSharedConstpools_.begin();
    while (iterator != globalSharedConstpools_.end()) {
        auto &constpools = iterator->second;
        auto constpoolIter = constpools.begin();
        while (constpoolIter != constpools.end()) {
            JSTaggedValue constpoolVal = constpoolIter->second;
            if (constpoolVal.IsHeapObject()) {
                bool isAlive = visitor.VisitRoot(Root::ROOT_VM,
                    ObjectSlot(reinterpret_cast<uintptr_t>(&constpoolIter->second)));
                if (!isAlive) {
                    int32_t constpoolIndex =
                        ConstantPool::Cast(constpoolVal.GetTaggedObject())->GetUnsharedConstpoolIndex();
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

void Runtime::ProcessNativeDeleteInSharedGC(const WeakRootVisitor &visitor)
{
    // No need lock here, only shared gc will sweep shared constpool, meanwhile other threads are suspended.
    auto iterator = globalSharedConstpools_.begin();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("Constpools:" + std::to_string(globalSharedConstpools_.size())).c_str(), "");
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
                        ConstantPool::Cast(constpoolVal.GetTaggedObject())->GetUnsharedConstpoolIndex();
                    EraseUnusedConstpool(iterator->first, constpoolIter->first, constpoolIndex);
                    constpoolIter = constpools.erase(constpoolIter);
                    // when shared constpool is not referenced by any objects,
                    // global unshared constpool count can be reuse.
                    freeSharedConstpoolIndex_.insert(constpoolIndex);
                    continue;
                }
                if (fwd != reinterpret_cast<TaggedObject *>(obj)) {
                    constpoolIter->second = JSTaggedValue(fwd);
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
    GCIterateThreadList([jsPandaFile, index, constpoolIndex](JSThread* thread) {
        // adapt state to saferegion
        ASSERT(g_isEnableCMCGC || !thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        // GC maybe happen before vm initialized.
        if (vm->GetUnsharedConstpoolsPointer() != nullptr) {
            vm->EraseUnusedConstpool(jsPandaFile, index, constpoolIndex);
        }
    });
}

void Runtime::ProcessSharedNativeDelete(const WeakRootVisitor &visitor)
{
    SharedHeap::GetInstance()->ProcessSharedNativeDelete(visitor);
}

void Runtime::PushToSharedNativePointerList(JSNativePointer *pointer)
{
    SharedHeap::GetInstance()->PushToSharedNativePointerList(pointer);
}

void Runtime::InvokeSharedNativePointerCallbacks()
{
    auto &callbacks = GetSharedNativePointerCallbacks();
    while (!callbacks.empty()) {
        auto callbackPair = callbacks.back();
        callbacks.pop_back();
        ASSERT(callbackPair.first != nullptr && callbackPair.second.first != nullptr &&
               callbackPair.second.second != nullptr);
        auto callback = callbackPair.first;
        (*callback)(nullptr, callbackPair.second.first, callbackPair.second.second);
    }
}

void Runtime::PreFork(JSThread *thread)
{
    if (g_isEnableCMCGC) {
        baseInstance_->PreFork(thread->GetThreadHolder());
    }
}

void Runtime::PostFork()
{
    if (g_isEnableCMCGC) {
        baseInstance_->PostFork();
    }
}
}  // namespace panda::ecmascript
