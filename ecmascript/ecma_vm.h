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

#ifndef ECMASCRIPT_ECMA_VM_H
#define ECMASCRIPT_ECMA_VM_H

#include "ecmascript/base/config.h"
#include "ecmascript/dfx/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/waiter_list.h"

namespace panda {
class JSNApi;
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class GlobalEnv;
class ObjectFactory;
class RegExpParserCache;
class EcmaRuntimeStat;
class Heap;
class HeapTracker;
class JSNativePointer;
class Program;
class GCStats;
class CpuProfiler;
class RegExpExecResultCache;
class JSPromise;
enum class PromiseRejectionEvent : uint8_t;
class JSPandaFileManager;
class JSPandaFile;
class EcmaStringTable;
class SnapshotEnv;
class SnapshotSerialize;
class SnapshotProcessor;
#if !WIN_OR_MAC_OR_IOS_PLATFORM
class HeapProfilerInterface;
#endif
namespace job {
class MicroJobQueue;
}  // namespace job

namespace tooling {
class JsDebuggerManager;
}  // namespace tooling

template<typename T>
class JSHandle;
class JSArrayBuffer;
class JSFunction;
class Program;
class TSManager;
class AOTFileManager;
class ModuleManager;
class CjsModule;
class CjsExports;
class CjsRequire;
class CjsModuleCache;
class SlowRuntimeStub;
class RequireManager;
struct CJSInfo;
class QuickFixManager;
class ConstantPool;

enum class MethodIndex : uint8_t {
    BUILTINS_GLOBAL_CALL_JS_BOUND_FUNCTION = 0,
    BUILTINS_GLOBAL_CALL_JS_PROXY,
    BUILTINS_OBJECT_CREATE_DATA_PROPERTY_ON_OBJECT_FUNCTIONS,
    BUILTINS_COLLATOR_ANONYMOUS_COLLATOR,
    BUILTINS_DATE_TIME_FORMAT_ANONYMOUS_DATE_TIME_FORMAT,
    BUILTINS_NUMBER_FORMAT_NUMBER_FORMAT_INTERNAL_FORMAT_NUMBER,
    BUILTINS_PROXY_INVALIDATE_PROXY_FUNCTION,
    BUILTINS_PROMISE_HANDLER_ASYNC_AWAIT_FULFILLED,
    BUILTINS_PROMISE_HANDLER_ASYNC_AWAIT_REJECTED,
    BUILTINS_PROMISE_HANDLER_RESOLVE_ELEMENT_FUNCTION,
    BUILTINS_PROMISE_HANDLER_RESOLVE,
    BUILTINS_PROMISE_HANDLER_REJECT,
    BUILTINS_PROMISE_HANDLER_EXECUTOR,
    BUILTINS_PROMISE_HANDLER_ANY_REJECT_ELEMENT_FUNCTION,
    BUILTINS_PROMISE_HANDLER_ALL_SETTLED_RESOLVE_ELEMENT_FUNCTION,
    BUILTINS_PROMISE_HANDLER_ALL_SETTLED_REJECT_ELEMENT_FUNCTION,
    BUILTINS_PROMISE_HANDLER_THEN_FINALLY_FUNCTION,
    BUILTINS_PROMISE_HANDLER_CATCH_FINALLY_FUNCTION,
    BUILTINS_PROMISE_HANDLER_VALUE_THUNK_FUNCTION,
    BUILTINS_PROMISE_HANDLER_THROWER_FUNCTION,
    BUILTINS_ASYNC_GENERATOR_NEXT_FULFILLED_FUNCTION,
    BUILTINS_ASYNC_GENERATOR_NEXT_REJECTED_FUNCTION,
    METHOD_END
};

using HostPromiseRejectionTracker = void (*)(const EcmaVM* vm,
                                             const JSHandle<JSPromise> promise,
                                             const JSHandle<JSTaggedValue> reason,
                                             PromiseRejectionEvent operation,
                                             void* data);
using PromiseRejectCallback = void (*)(void* info);

using NativePtrGetter = void* (*)(void* info);

using ResolvePathCallback = std::function<std::string(std::string dirPath, std::string requestPath)>;
using ResolveBufferCallback = std::function<std::vector<uint8_t>(std::string dirPath, std::string requestPath)>;

class EcmaVM {
public:
    static EcmaVM *Create(const JSRuntimeOptions &options, EcmaParamConfiguration &config);

    static bool Destroy(EcmaVM *vm);

    explicit EcmaVM(JSRuntimeOptions options, EcmaParamConfiguration config);

    EcmaVM();

    ~EcmaVM();

    void SetLoop(void *loop)
    {
        loop_ = loop;
    }

    void *GetLoop() const
    {
        return loop_;
    }

    bool IsInitialized() const
    {
        return initialized_;
    }

    bool IsGlobalConstInitialized() const
    {
        return globalConstInitialized_;
    }

    ObjectFactory *GetFactory() const
    {
        return factory_;
    }

    bool Initialize();

    GCStats *GetEcmaGCStats() const
    {
        return gcStats_;
    }

    JSThread *GetAssociatedJSThread() const
    {
        return thread_;
    }

    JSRuntimeOptions &GetJSOptions()
    {
        return options_;
    }

    const EcmaParamConfiguration &GetEcmaParamConfiguration() const
    {
        return ecmaParamConfiguration_;
    }

    JSHandle<GlobalEnv> GetGlobalEnv() const;

    JSHandle<job::MicroJobQueue> GetMicroJobQueue() const;

    bool ExecutePromisePendingJob();

    static EcmaVM *ConstCast(const EcmaVM *vm)
    {
        return const_cast<EcmaVM *>(vm);
    }

    RegExpParserCache *GetRegExpParserCache() const
    {
        ASSERT(regExpParserCache_ != nullptr);
        return regExpParserCache_;
    }

    EcmaStringTable *GetEcmaStringTable() const
    {
        ASSERT(stringTable_ != nullptr);
        return stringTable_;
    }

    JSThread *GetJSThread() const
    {
        // Exclude GC thread
        if (options_.EnableThreadCheck()) {
            if (thread_ == nullptr) {
                LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
            }
            if (!Taskpool::GetCurrentTaskpool()->IsInThreadPool(std::this_thread::get_id()) &&
                thread_->GetThreadId() != JSThread::GetCurrentThreadId()) {
                    LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                                        << " thread:" << thread_->GetThreadId()
                                        << " currentThread:" << JSThread::GetCurrentThreadId();
            }
        }
        return thread_;
    }

    bool ICEnabled() const
    {
        return icEnabled_;
    }

    void PushToNativePointerList(JSNativePointer *array);
    void RemoveFromNativePointerList(JSNativePointer *array);

    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
    void EnableUserUncaughtErrorHandler();

    EcmaRuntimeStat *GetRuntimeStat() const
    {
        return runtimeStat_;
    }

    void SetRuntimeStatEnable(bool flag);

    bool IsOptionalLogEnabled() const
    {
        return optionalLogEnabled_;
    }

    void Iterate(const RootVisitor &v, const RootRangeVisitor &rv);

    const Heap *GetHeap() const
    {
        return heap_;
    }

    void CollectGarbage(TriggerGCType gcType) const;

    void StartHeapTracking(HeapTracker *tracker);

    void StopHeapTracking();

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_.get();
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_.get();
    }

    Chunk *GetChunk() const
    {
        return const_cast<Chunk *>(&chunk_);
    }
    void ProcessNativeDelete(const WeakRootVisitor &visitor);
    void ProcessReferences(const WeakRootVisitor &visitor);

    ModuleManager *GetModuleManager() const
    {
        return moduleManager_;
    }

    TSManager *GetTSManager() const
    {
        return tsManager_;
    }

    AOTFileManager* PUBLIC_API GetAOTFileManager() const
    {
        return aotFileManager_;
    }

    SnapshotEnv *GetSnapshotEnv() const
    {
        return snapshotEnv_;
    }

    void SetupRegExpResultCache();

    JSHandle<JSTaggedValue> GetRegExpCache() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&regexpCache_));
    }

    void SetRegExpCache(JSTaggedValue newCache)
    {
        regexpCache_ = newCache;
    }

    tooling::JsDebuggerManager *GetJsDebuggerManager() const
    {
        return debuggerManager_;
    }

    void SetEnableForceGC(bool enable)
    {
        options_.SetEnableForceGC(enable);
    }

    void SetData(void* data)
    {
        data_ = data;
    }

    void SetPromiseRejectCallback(PromiseRejectCallback cb)
    {
        promiseRejectCallback_ = cb;
    }

    PromiseRejectCallback GetPromiseRejectCallback() const
    {
        return promiseRejectCallback_;
    }

    void SetNativePtrGetter(NativePtrGetter cb)
    {
        nativePtrGetter_ = cb;
    }

    NativePtrGetter GetNativePtrGetter() const
    {
        return nativePtrGetter_;
    }

    void SetHostPromiseRejectionTracker(HostPromiseRejectionTracker cb)
    {
        hostPromiseRejectionTracker_ = cb;
    }

    void SetAllowAtomicWait(bool wait)
    {
        AllowAtomicWait_ = wait;
    }

    bool GetAllowAtomicWait() const
    {
        return AllowAtomicWait_;
    }

    WaiterListNode *GetWaiterListNode()
    {
        return &waiterListNode_;
    }

    void PromiseRejectionTracker(const JSHandle<JSPromise> &promise,
                                 const JSHandle<JSTaggedValue> &reason, PromiseRejectionEvent operation)
    {
        if (hostPromiseRejectionTracker_ != nullptr) {
            hostPromiseRejectionTracker_(this, promise, reason, operation, data_);
        }
    }

    // CJS callbacks
    void SetResolvePathCallback(ResolvePathCallback cb)
    {
        resolvePathCallback_ = cb;
    }

    ResolvePathCallback GetResolvePathCallback() const
    {
        return resolvePathCallback_;
    }

    void SetResolveBufferCallback(ResolveBufferCallback cb)
    {
        resolveBufferCallback_ = cb;
    }

    ResolveBufferCallback GetResolveBufferCallback() const
    {
        return resolveBufferCallback_;
    }

    void AddConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool, int32_t index = 0);

    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, int32_t index);

    JSHandle<ConstantPool> FindOrCreateConstPool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id);

    void StoreBCOffsetInfo(const std::string& methodName, int32_t bcOffset)
    {
        exceptionBCList_.emplace_back(std::pair<std::string, int32_t>(methodName, bcOffset));
    }

    std::vector<std::pair<std::string, int32_t>> GetBCOffsetInfoList() const
    {
        return exceptionBCList_;
    }

    void ClearExceptionBCList()
    {
        exceptionBCList_.clear();
    }

    void WorkersetInfo(EcmaVM *hostVm, EcmaVM *workerVm)
    {
        auto thread = workerVm->GetJSThread();
        if (thread != nullptr) {
            auto tid = thread->GetThreadId();
            if (tid != 0) {
                if (hostVm != nullptr && workerVm != nullptr) {
                    WorkerList_.emplace(tid, workerVm);
                }
            }
        }
    }

    EcmaVM *GetWorkerVm(uint32_t tid) const
    {
        EcmaVM *workerVm = nullptr;
        if (!WorkerList_.empty()) {
            auto iter = WorkerList_.find(tid);
            if (iter != WorkerList_.end()) {
                workerVm = iter->second;
            }
        }
        return workerVm;
    }

    bool DeleteWorker(EcmaVM *hostVm, EcmaVM *workerVm)
    {
        if (hostVm != nullptr && workerVm != nullptr) {
            auto tid = workerVm->GetJSThread()->GetThreadId();
            if (tid == 0) {return false;}
            auto iter = WorkerList_.find(tid);
            if (iter != WorkerList_.end()) {
                WorkerList_.erase(iter);
                return true;
            }
            return false;
        }
        return false;
    }

    bool IsBundlePack() const
    {
        return isBundlePack_;
    }

    void SetIsBundlePack(bool value)
    {
        isBundlePack_ = value;
    }

#if !WIN_OR_MAC_OR_IOS_PLATFORM
    void DeleteHeapProfile();
    HeapProfilerInterface *GetOrNewHeapProfile();
#endif

    void SetAssetPath(const CString &assetPath)
    {
        assetPath_ = assetPath;
    }

    CString GetAssetPath() const
    {
        return assetPath_;
    }

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    CpuProfiler *GetProfiler() const
    {
        return profiler_;
    }

    void SetProfiler(CpuProfiler *profiler)
    {
        profiler_ = profiler;
    }
#endif

    PGOProfiler *GetPGOProfiler() const
    {
        return pgoProfiler_;
    }

    bool FindCatchBlock(Method *method, uint32_t pc) const;

    void preFork();
    void postFork();

    // For Internal Native MethodLiteral.
    JSTaggedValue GetMethodByIndex(MethodIndex idx);

    QuickFixManager *GetQuickFixManager() const
    {
        return quickFixManager_;
    }

    JSTaggedValue ExecuteAot(size_t argsNum, JSHandle<JSFunction> &callTarget, const JSTaggedType *prevFp,
                             size_t actualNumArgs, size_t declareNumArgs, std::vector<JSTaggedType> &args);
    JSTaggedValue AotReentry(size_t actualNumArgs, JSTaggedType *args, bool isNew);
protected:

    void HandleUncaughtException(TaggedObject *exception);

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo);

private:
    class CpuProfilingScope {
    public:
        explicit CpuProfilingScope(EcmaVM* vm);
        ~CpuProfilingScope();

    private:
        [[maybe_unused]] EcmaVM* vm_;
        [[maybe_unused]] CpuProfiler* profiler_;
    };

    void SetGlobalEnv(GlobalEnv *global);

    void SetMicroJobQueue(job::MicroJobQueue *queue);

    Expected<JSTaggedValue, bool> InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile, std::string_view entryPoint);

    JSTaggedValue InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                          const JSPandaFile *jsPandaFile, std::string_view entryPoint);

    void CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg, const JSPandaFile *jsPandaFile);

    void InitializeEcmaScriptRunStat();

    void ClearBufferData();

    void LoadAOTFiles();
    void LoadStubFile();

    // For Internal Native MethodLiteral.
    void GenerateInternalNativeMethods();

    NO_MOVE_SEMANTIC(EcmaVM);
    NO_COPY_SEMANTIC(EcmaVM);

    // VM startup states.
    JSRuntimeOptions options_;
    bool icEnabled_ {true};
    bool initialized_ {false};
    bool globalConstInitialized_ {false};
    GCStats *gcStats_ {nullptr};
    bool isUncaughtExceptionRegistered_ {false};
    // VM memory management.
    EcmaStringTable *stringTable_ {nullptr};
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    Chunk chunk_;
    Heap *heap_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    CList<JSNativePointer *> nativePointerList_;
    // VM execution states.
    JSThread *thread_ {nullptr};
    RegExpParserCache *regExpParserCache_ {nullptr};
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};
    JSTaggedValue regexpCache_ {JSTaggedValue::Hole()};
    JSTaggedValue microJobQueue_ {JSTaggedValue::Hole()};
    EcmaRuntimeStat *runtimeStat_ {nullptr};

    // For framewrok file snapshot.
    CString snapshotFileName_;
    CString frameworkAbcFileName_;
    JSTaggedValue frameworkProgram_ {JSTaggedValue::Hole()};
    const JSPandaFile *frameworkPandaFile_ {nullptr};
    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> cachedConstpools_ {};

    // VM resources.
    ModuleManager *moduleManager_ {nullptr};
    TSManager *tsManager_ {nullptr};
    SnapshotEnv *snapshotEnv_ {nullptr};
    bool optionalLogEnabled_ {false};
    AOTFileManager *aotFileManager_ {nullptr};

    // Debugger
    tooling::JsDebuggerManager *debuggerManager_ {nullptr};
    // merge abc
    bool isBundlePack_ {true}; // isBundle means app compile mode is JSBundle
#if !WIN_OR_MAC_OR_IOS_PLATFORM
    HeapProfilerInterface *heapProfile_ {nullptr};
#endif
    CString assetPath_;

    // Registered Callbacks
    PromiseRejectCallback promiseRejectCallback_ {nullptr};
    HostPromiseRejectionTracker hostPromiseRejectionTracker_ {nullptr};
    NativePtrGetter nativePtrGetter_ {nullptr};
    void* data_ {nullptr};

    bool isProcessingPendingJob_ = false;
	// atomics
    bool AllowAtomicWait_ {true};
    WaiterListNode waiterListNode_;
    std::vector<std::pair<std::string, int32_t>> exceptionBCList_;

    // CJS resolve path Callbacks
    ResolvePathCallback resolvePathCallback_ {nullptr};
    ResolveBufferCallback resolveBufferCallback_ {nullptr};

    // vm parameter configurations
    EcmaParamConfiguration ecmaParamConfiguration_;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    CpuProfiler *profiler_ {nullptr};
#endif

    // For Native MethodLiteral
    static void *InternalMethodTable[static_cast<uint8_t>(MethodIndex::METHOD_END)];
    CVector<JSTaggedValue> internalNativeMethods_;

    // For repair patch.
    QuickFixManager *quickFixManager_;

    // PGO Profiler
    PGOProfiler *pgoProfiler_;

    friend class Snapshot;
    friend class SnapshotProcessor;
    friend class ObjectFactory;
    friend class ValueSerializer;
    friend class panda::JSNApi;
    friend class JSPandaFileExecutor;
    CMap<uint32_t, EcmaVM *> WorkerList_ {};
    void *loop_ {nullptr};
};
}  // namespace ecmascript
}  // namespace panda

#endif
