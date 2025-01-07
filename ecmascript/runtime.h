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

#ifndef ECMASCRIPT_RUNTIME_H
#define ECMASCRIPT_RUNTIME_H

#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/mutator_lock.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/serializer/serialize_chunk.h"

#include "libpandabase/macros.h"

#include <list>
#include <memory>

namespace panda::ecmascript {
using AppfreezeFilterCallback = std::function<bool(const int32_t pid)>;
class Runtime {
public:
    PUBLIC_API static Runtime *GetInstance();

    static void CreateIfFirstVm(const JSRuntimeOptions &options);
    static void DestroyIfLastVm();
    void InitializeIfFirstVm(EcmaVM *vm);

    void RegisterThread(JSThread* newThread);
    void UnregisterThread(JSThread* thread);

    void SuspendAll(JSThread *current);
    void ResumeAll(JSThread *current);
    void IterateSerializeRoot(RootVisitor &v);

    JSThread *GetMainThread() const
    {
        return mainThread_;
    }

    MutatorLock *GetMutatorLock()
    {
        return &mutatorLock_;
    }

    const MutatorLock *GetMutatorLock() const
    {
        return &mutatorLock_;
    }

    template<class Callback>
    void GCIterateThreadList(const Callback &cb)
    {
        LockHolder lock(threadsLock_);
        GCIterateThreadListWithoutLock(cb);
    }

    template<class Callback>
    void GCIterateThreadListWithoutLock(const Callback &cb)
    {
        for (auto thread : threads_) {
            if (thread->ReadyForGCIterating()) {
                cb(thread);
            }
        }
    }

    // Result may be inaccurate, just an approximate value.
    size_t ApproximateThreadListSize()
    {
        return threads_.size();
    }

    inline const GlobalEnvConstants *GetGlobalEnvConstants()
    {
        return &globalConst_;
    }

    inline bool SharedConstInited()
    {
        return sharedConstInited_;
    }

    JSTaggedValue GetGlobalEnv() const
    {
        return globalEnv_;
    }

    inline EcmaStringTable *GetEcmaStringTable() const
    {
        return stringTable_.get();
    }

    inline SerializationChunk *GetSerializeRootMapValue([[maybe_unused]] JSThread *thread,
        uint32_t dataIndex)
    {
        ASSERT(thread->IsInManagedState());
        LockHolder lock(serializeLock_);
        auto iter = serializeRootMap_.find(dataIndex);
        if (iter == serializeRootMap_.end()) {
            return nullptr;
        }
        return iter->second.get();
    }

    uint32_t PushSerializationRoot([[maybe_unused]] JSThread *thread, std::unique_ptr<SerializationChunk> chunk)
    {
        ASSERT(thread->IsInManagedState());
        LockHolder lock(serializeLock_);
        uint32_t index = GetSerializeDataIndex();
        ASSERT(serializeRootMap_.find(index) == serializeRootMap_.end());
        serializeRootMap_.emplace(index, std::move(chunk));
        return index;
    }

    void RemoveSerializationRoot([[maybe_unused]] JSThread *thread, uint32_t index)
    {
        ASSERT(thread->IsInManagedState());
        LockHolder lock(serializeLock_);
        ASSERT(serializeRootMap_.find(index) != serializeRootMap_.end());
        serializeRootMap_.erase(index);
        serializeDataIndexVector_.emplace_back(index);
    }

    static bool SharedGCRequest()
    {
        LockHolder lock(*vmCreationLock_);
        destroyCount_++;
        if (destroyCount_ == WORKER_DESTRUCTION_COUNT || vmCount_ < MIN_GC_TRIGGER_VM_COUNT) {
            destroyCount_ = 0;
            return true;
        } else {
            return false;
        }
    }

    bool HasCachedConstpool(const JSPandaFile *jsPandaFile);
    PUBLIC_API JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, int32_t index);
    JSTaggedValue FindConstpoolUnlocked(const JSPandaFile *jsPandaFile, int32_t index);
    JSHandle<ConstantPool> AddOrUpdateConstpool(const JSPandaFile *jsPandaFile,
                                                JSHandle<ConstantPool> constpool,
                                                int32_t index = 0);
    std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> FindConstpools(
        const JSPandaFile *jsPandaFile);
    void EraseUnusedConstpool(const JSPandaFile *jsPandaFile, int32_t index, int32_t constpoolIndex);

    void ProcessNativeDeleteInSharedGC(const WeakRootVisitor &visitor);

    void ProcessSharedNativeDelete(const WeakRootVisitor &visitor);
    void InvokeSharedNativePointerCallbacks();
    void PushToSharedNativePointerList(JSNativePointer *pointer);

    inline bool CreateStringCacheTable(uint32_t size)
    {
        constexpr int32_t MAX_SIZE = 150;
        if ((size == 0) || (size > MAX_SIZE) || (externalRegisteredStringTable_ != nullptr)) {
            LOG_ECMA(WARN) << "invalid size of the string cache table or the table has been registered.";
            LOG_ECMA(WARN) << "Currently, maximum size of the table is " << MAX_SIZE;
            return false;
        }

        externalRegisteredStringTable_ = new JSTaggedValue[size];
        if (externalRegisteredStringTable_ == nullptr) {
            LOG_ECMA(ERROR) << "create string cache table failed";
            return false;
        }
        registeredStringTableSize_ = size;
        return true;
    }

    inline bool SetCachedString(JSHandle<EcmaString> str, uint32_t propertyIndex)
    {
        if (propertyIndex >= registeredStringTableSize_ || (externalRegisteredStringTable_ == nullptr)) {
            LOG_ECMA(ERROR) << "invalid size of the string cache table or the table has never been registered.";
            return false;
        }
        externalRegisteredStringTable_[propertyIndex] = str.GetTaggedValue();
        return true;
    }

    inline JSHandle<EcmaString> GetCachedString(JSThread *thread, uint32_t propertyIndex)
    {
        if ((externalRegisteredStringTable_ == nullptr) || (propertyIndex >= registeredStringTableSize_)) {
            LOG_ECMA(ERROR) << "invalid size of the string cache table or the table has never been registered.";
            return JSHandle<EcmaString>(thread->GlobalConstants()->GetHandledEmptyString());
        }
        return JSHandle<EcmaString>(reinterpret_cast<uintptr_t>(&externalRegisteredStringTable_[propertyIndex]));
    }

    inline bool HasCachedString(uint32_t propertyIndex)
    {
        if ((externalRegisteredStringTable_ == nullptr) || propertyIndex >= registeredStringTableSize_) {
            LOG_ECMA(ERROR) << "invalid size of the string cache table or the table has never been registered.";
            return false;
        }

        if (externalRegisteredStringTable_[propertyIndex].GetRawData() != JSTaggedValue::NULL_POINTER) {
            return true;
        }
        return false;
    }

    void IterateCachedStringRoot(RootVisitor &v)
    {
        if ((externalRegisteredStringTable_ == nullptr) || (registeredStringTableSize_ <= 0)) {
            return;
        }
        auto begin = ObjectSlot(reinterpret_cast<uintptr_t>(externalRegisteredStringTable_));
        auto end = ObjectSlot(reinterpret_cast<uintptr_t>(externalRegisteredStringTable_ +
            registeredStringTableSize_));
        v.VisitRangeRoot(Root::ROOT_VM, begin, end);
    }

    AppfreezeFilterCallback GetAppFreezeFilterCallback() const
    {
        return appfreezeFilterCallback_;
    }

    void SetAppFreezeFilterCallback(AppfreezeFilterCallback cb)
    {
        appfreezeFilterCallback_ = cb;
    }

private:
    static constexpr int32_t WORKER_DESTRUCTION_COUNT = 3;
    static constexpr int32_t MIN_GC_TRIGGER_VM_COUNT = 4;
    static constexpr uint32_t MAX_SUSPEND_RETRIES = 5000;
    Runtime() = default;
    ~Runtime();
    void SuspendAllThreadsImpl(JSThread *current);
    void ResumeAllThreadsImpl(JSThread *current);

    void PreInitialization(const EcmaVM *vm);
    void PostInitialization(const EcmaVM *vm);

    uint32_t GetSerializeDataIndex()
    {
        if (!serializeDataIndexVector_.empty()) {
            uint32_t index = serializeDataIndexVector_.back();
            serializeDataIndexVector_.pop_back();
            return index;
        }
        return ++serializeDataIndex_;
    }

    int32_t GetAndIncreaseSharedConstpoolCount()
    {
        if (freeSharedConstpoolIndex_.size() > 0) {
            auto iter = freeSharedConstpoolIndex_.begin();
            int32_t freeCount = *iter;
            freeSharedConstpoolIndex_.erase(iter);
            return freeCount;
        }
        return sharedConstpoolCount_++;
    }

    std::vector<std::pair<NativePointerCallback, std::pair<void *, void *>>> &GetSharedNativePointerCallbacks()
    {
        return sharedNativePointerCallbacks_;
    }

    Mutex threadsLock_;
    ConditionVariable threadSuspendCondVar_;
    Mutex serializeLock_;
    std::list<JSThread*> threads_;
    uint32_t suspendNewCount_ {0};
    uint32_t serializeDataIndex_ {0};
    MutatorLock mutatorLock_;

    bool sharedConstInited_ {false};
    GlobalEnvConstants globalConst_;
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};
    JSThread *mainThread_ {nullptr};
    // for shared heap.
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    // for stringTable.
    std::unique_ptr<EcmaStringTable> stringTable_;
    std::unordered_map<uint32_t, std::unique_ptr<SerializationChunk>> serializeRootMap_;
    std::vector<uint32_t> serializeDataIndexVector_;

    // Shared constantpool cache
    Mutex constpoolLock_;
    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> globalSharedConstpools_ {};
    int32_t sharedConstpoolCount_ = 0; // shared constpool count.
    std::set<int32_t> freeSharedConstpoolIndex_ {}; // reuse shared constpool index.

    // Runtime instance and VMs creation.
    static int32_t vmCount_;
    static int32_t destroyCount_;
    static bool firstVmCreated_;
    static Mutex *vmCreationLock_;
    static Runtime *instance_;

    // for string cache
    JSTaggedValue *externalRegisteredStringTable_ {nullptr};
    uint32_t registeredStringTableSize_ = 0;

    // for shared native pointer
    std::vector<std::pair<NativePointerCallback, std::pair<void *, void *>>> sharedNativePointerCallbacks_ {};

    // for appfreeze filter function
    AppFreezeFilterCallback appfreezeFilterCallback_ {nullptr};
    
    friend class EcmaVM;
    friend class JSThread;
    friend class SharedHeap;

    NO_COPY_SEMANTIC(Runtime);
    NO_MOVE_SEMANTIC(Runtime);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_RUNTIME_H
