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

#include <mutex>

#include "ecmascript/base/config.h"
#include "ecmascript/builtins/builtins_method_index.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/taskpool/taskpool.h"
// #include "ecmascript/waiter_list.h"

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
class PGOProfiler;
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
class CjsModule;
class CjsExports;
class CjsRequire;
class CjsModuleCache;
class SlowRuntimeStub;
class RequireManager;
struct CJSInfo;
class QuickFixManager;
class ConstantPool;
class FunctionCallTimer;

// enum class IcuFormatterType {
//     SIMPLE_DATE_FORMAT_DEFAULT,
//     SIMPLE_DATE_FORMAT_DATE,
//     SIMPLE_DATE_FORMAT_TIME,
//     NUMBER_FORMATTER,
//     COLLATOR
// };

using NativePtrGetter = void* (*)(void* info);

using ResolvePathCallback = std::function<std::string(std::string dirPath, std::string requestPath)>;
using ResolveBufferCallback = std::function<std::vector<uint8_t>(std::string dirPath)>;
// using IcuDeleteEntry = void(*)(void *pointer, void *data);

class EcmaVM {
public:
    static EcmaVM *Create(const JSRuntimeOptions &options, EcmaParamConfiguration &config);

    static bool Destroy(EcmaVM *vm);

    EcmaVM(JSRuntimeOptions options, EcmaParamConfiguration config);

    EcmaVM();

    ~EcmaVM();

    bool IsInitialized() const
    {
        return initialized_;
    }

    ObjectFactory *GetFactory() const
    {
        return factory_;
    }

    void InitializePGOProfiler();
    void ResetPGOProfiler();

    bool IsEnablePGOProfiler() const;

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

    static EcmaVM *ConstCast(const EcmaVM *vm)
    {
        return const_cast<EcmaVM *>(vm);
    }

    void CheckThread() const
    {
        // Exclude GC thread
        if (thread_ == nullptr) {
            LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
        }
        if (!Taskpool::GetCurrentTaskpool()->IsInThreadPool(std::this_thread::get_id()) &&
            thread_->GetThreadId() != JSThread::GetCurrentThreadId() && !thread_->IsCrossThreadExecutionEnable()) {
                LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                                    << " thread:" << thread_->GetThreadId()
                                    << " currentThread:" << JSThread::GetCurrentThreadId();
        }
    }

    ARK_INLINE JSThread *GetJSThread() const
    {
        if (options_.EnableThreadCheck()) {
            CheckThread();
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
    void CollectGarbage(TriggerGCType gcType, GCReason reason = GCReason::OTHER) const;

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

    AOTFileManager *GetAOTFileManager() const
    {
        return aotFileManager_;
    }

    SnapshotEnv *GetSnapshotEnv() const
    {
        return snapshotEnv_;
    }

    tooling::JsDebuggerManager *GetJsDebuggerManager() const
    {
        return debuggerManager_;
    }

    void SetEnableForceGC(bool enable)
    {
        options_.SetEnableForceGC(enable);
    }

    void SetNativePtrGetter(NativePtrGetter cb)
    {
        nativePtrGetter_ = cb;
    }

    NativePtrGetter GetNativePtrGetter() const
    {
        return nativePtrGetter_;
    }

    size_t GetNativePointerListSize()
    {
        return nativePointerList_.size();
    }

    void SetResolveBufferCallback(ResolveBufferCallback cb)
    {
        resolveBufferCallback_ = cb;
    }

    ResolveBufferCallback GetResolveBufferCallback() const
    {
        return resolveBufferCallback_;
    }

    void SetConcurrentCallback(ConcurrentCallback callback, void *data)
    {
        concurrentCallback_ = callback;
        concurrentData_ = data;
    }

    void TriggerConcurrentCallback(JSTaggedValue result, JSTaggedValue hint);

    void AddConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool, int32_t index = 0);

    bool HasCachedConstpool(const JSPandaFile *jsPandaFile) const;

    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, int32_t index);
    // For new version instruction.
    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id);
    std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> FindConstpools(
        const JSPandaFile *jsPandaFile);

    JSHandle<ConstantPool> PUBLIC_API FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
                                                            panda_file::File::EntityId id);
    void CreateAllConstpool(const JSPandaFile *jsPandaFile);

    void WorkersetInfo(EcmaVM *hostVm, EcmaVM *workerVm)
    {
        os::memory::LockHolder lock(mutex_);
        auto thread = workerVm->GetJSThread();
        if (thread != nullptr && hostVm != nullptr) {
            auto tid = thread->GetThreadId();
            if (tid != 0) {
                workerList_.emplace(tid, workerVm);
            }
        }
    }

    EcmaVM *GetWorkerVm(uint32_t tid) const
    {
        EcmaVM *workerVm = nullptr;
        if (!workerList_.empty()) {
            auto iter = workerList_.find(tid);
            if (iter != workerList_.end()) {
                workerVm = iter->second;
            }
        }
        return workerVm;
    }

    bool DeleteWorker(EcmaVM *hostVm, EcmaVM *workerVm)
    {
        os::memory::LockHolder lock(mutex_);
        auto thread = workerVm->GetJSThread();
        if (hostVm != nullptr && thread != nullptr) {
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

    bool IsWorkerThread()
    {
        return options_.IsWorker();
    }

    bool IsBundlePack() const
    {
        return isBundlePack_;
    }

    void SetIsBundlePack(bool value)
    {
        isBundlePack_ = value;
    }

#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    void DeleteHeapProfile();
    HeapProfilerInterface *GetOrNewHeapProfile();
#endif

    bool EnableReportModuleResolvingFailure() const
    {
        return options_.EnableReportModuleResolvingFailure();
    }

    void SetAssetPath(const CString &assetPath)
    {
        assetPath_ = assetPath;
    }

    CString GetAssetPath() const
    {
        return assetPath_;
    }

    void SetBundleName(const CString &bundleName)
    {
        bundleName_ = bundleName;
    }

    CString GetBundleName() const
    {
        return bundleName_;
    }

    void SetModuleName(const CString &moduleName)
    {
        moduleName_ = moduleName;
    }

    CString GetModuleName() const
    {
        return moduleName_;
    }

    void DumpAOTInfo() const DUMP_API_ATTR;

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

    void PreFork();
    void PostFork();

    // For Internal Native MethodLiteral.
    JSTaggedValue GetMethodByIndex(MethodIndex idx);

    QuickFixManager *GetQuickFixManager() const
    {
        return quickFixManager_;
    }

    JSTaggedValue ExecuteAot(size_t actualNumArgs, JSTaggedType *args,
        const JSTaggedType *prevFp, bool needPushUndefined);

    JSTaggedValue FastCallAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp);

    void HandleUncaughtException(JSTaggedValue exception);
    void DumpCallTimeInfo();
    void PrintOptStat();

    FunctionCallTimer *GetCallTimer() const
    {
        return callTimer_;
    }


    void SetGlobalEnv(GlobalEnv *global);
    void CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
                      const JSPandaFile *jsPandaFile);
protected:

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo) const;

private:
    JSTaggedValue InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                          const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                          CJSInfo* cjsInfo = nullptr);

    void CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
                      const JSPandaFile *jsPandaFile, std::string_view entryPoint);

    void InitializeEcmaScriptRunStat();

    void ClearBufferData();

    bool LoadAOTFiles(const std::string& aotFileName);
    void LoadStubFile();
    void CheckStartCpuProfiler();

    // For Internal Native MethodLiteral.
    void GenerateInternalNativeMethods();

    NO_MOVE_SEMANTIC(EcmaVM);
    NO_COPY_SEMANTIC(EcmaVM);

    // VM startup states.
    JSRuntimeOptions options_;
    bool icEnabled_ {true};
    bool initialized_ {false};
    GCStats *gcStats_ {nullptr};
    // VM memory management.
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    Chunk chunk_;
    Heap *heap_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    CList<JSNativePointer *> nativePointerList_;
    // VM execution states.
    JSThread *thread_ {nullptr};
    EcmaRuntimeStat *runtimeStat_ {nullptr};

    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> cachedConstpools_ {};

    // VM resources.
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
    CString bundleName_;
    CString moduleName_;
    // Registered Callbacks
    NativePtrGetter nativePtrGetter_ {nullptr};

    // CJS resolve path Callbacks
    ResolvePathCallback resolvePathCallback_ {nullptr};
    ResolveBufferCallback resolveBufferCallback_ {nullptr};

    // Concurrent taskpool callback and data
    ConcurrentCallback concurrentCallback_ {nullptr};
    void *concurrentData_ {nullptr};

    // vm parameter configurations
    EcmaParamConfiguration ecmaParamConfiguration_;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    CpuProfiler *profiler_ {nullptr};
#endif
    FunctionCallTimer *callTimer_ {nullptr};

    // For Native MethodLiteral
    static void *InternalMethodTable[static_cast<uint8_t>(MethodIndex::METHOD_END)];
    CVector<JSTaggedValue> internalNativeMethods_;

    // For repair patch.
    QuickFixManager *quickFixManager_ {nullptr};

    // PGO Profiler
    PGOProfiler *pgoProfiler_ {nullptr};

    friend class Snapshot;
    friend class SnapshotProcessor;
    friend class ObjectFactory;
    friend class ValueSerializer;
    friend class panda::JSNApi;
    friend class JSPandaFileExecutor;
    friend class EcmaContext;
    CMap<uint32_t, EcmaVM *> workerList_ {};
    os::memory::Mutex mutex_;
};
}  // namespace ecmascript
}  // namespace panda

#endif
