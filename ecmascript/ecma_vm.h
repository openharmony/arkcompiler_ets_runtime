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
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda {
class JSNApi;
struct HmsMap;
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
class GCKeyStats;
class CpuProfiler;
class Tracing;
class RegExpExecResultCache;
class JSPromise;
enum class PromiseRejectionEvent : uint8_t;
enum class Concurrent { YES, NO };
class JSPandaFileManager;
class JSPandaFile;
class EcmaStringTable;
class SnapshotEnv;
class SnapshotSerialize;
class SnapshotProcessor;
using PGOProfiler = pgo::PGOProfiler;
#if !WIN_OR_MAC_OR_IOS_PLATFORM
class HeapProfilerInterface;
class HeapProfiler;
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
class SourceTextModule;
class Program;
class AOTFileManager;
class SlowRuntimeStub;
class RequireManager;
class QuickFixManager;
class ConstantPool;
class FunctionCallTimer;
class EcmaStringTable;
class JSObjectResizingStrategy;
class Jit;
class JitThread;

using NativePtrGetter = void* (*)(void* info);
using SourceMapCallback = std::function<std::string(const std::string& rawStack)>;
using SourceMapTranslateCallback = std::function<bool(std::string& url, int& line, int& column)>;
using ResolveBufferCallback = std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize)>;
using UnloadNativeModuleCallback = std::function<bool(const std::string &moduleKey)>;
using RequestAotCallback =
    std::function<int32_t(const std::string &bundleName, const std::string &moduleName, int32_t triggerMode)>;
using SearchHapPathCallBack = std::function<bool(const std::string moduleName, std::string &hapPath)>;
using DeviceDisconnectCallback = std::function<bool()>;
using UncatchableErrorHandler = std::function<void(panda::TryCatch&)>;
using NativePointerCallback = void (*)(void *, void *, void *);

class EcmaVM {
public:
    static EcmaVM *Create(const JSRuntimeOptions &options);

    static bool Destroy(EcmaVM *vm);

    EcmaVM(JSRuntimeOptions options, EcmaParamConfiguration config);

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

    ObjectFactory *GetFactory() const
    {
        return factory_;
    }

    void InitializePGOProfiler();
    void InitializeEnableAotCrash();
    void ResetPGOProfiler();

    bool PUBLIC_API IsEnablePGOProfiler() const;
    bool PUBLIC_API IsEnableElementsKind() const;

    bool Initialize();
    void InitializeForJit(JitThread *thread);

    GCStats *GetEcmaGCStats() const
    {
        return gcStats_;
    }

    GCKeyStats *GetEcmaGCKeyStats() const
    {
        return gcKeyStats_;
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

    JSHandle<GlobalEnv> PUBLIC_API GetGlobalEnv() const;

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

    JSThread *GetAndFastCheckJSThread() const
    {
        if (thread_ == nullptr) {
            LOG_FULL(FATAL) << "Fatal: ecma_vm has been destructed! vm address is: " << this;
        }
        if (thread_->GetThreadId() != JSThread::GetCurrentThreadId() && !thread_->IsCrossThreadExecutionEnable()) {
            LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                                    << " thread:" << thread_->GetThreadId()
                                    << " currentThread:" << JSThread::GetCurrentThreadId();
        }
        return thread_;
    }

    ARK_INLINE JSThread *GetJSThread() const
    {
        if (options_.EnableThreadCheck() || EcmaVM::GetMultiThreadCheck()) {
            CheckThread();
        }
        return thread_;
    }

    bool ICEnabled() const
    {
        return icEnabled_;
    }

    void PushToNativePointerList(JSNativePointer *pointer, Concurrent isConcurrent = Concurrent::NO);
    void RemoveFromNativePointerList(JSNativePointer *pointer);
    void PushToDeregisterModuleList(CString module);
    void RemoveFromDeregisterModuleList(CString module);
    bool ContainInDeregisterModuleList(CString module);
    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
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

    void PushToSharedNativePointerList(JSNativePointer *pointer);
    void ProcessSharedNativeDelete(const WeakRootVisitor &visitor);

    SnapshotEnv *GetSnapshotEnv() const
    {
        return snapshotEnv_;
    }

    tooling::JsDebuggerManager *GetJsDebuggerManager() const
    {
        return debuggerManager_;
    }

    void SetDeviceDisconnectCallback(DeviceDisconnectCallback cb)
    {
        deviceDisconnectCallback_ = cb;
    }

    DeviceDisconnectCallback GetDeviceDisconnectCallback() const
    {
        return deviceDisconnectCallback_;
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

    void SetSourceMapCallback(SourceMapCallback cb)
    {
        sourceMapCallback_ = cb;
    }

    SourceMapCallback GetSourceMapCallback() const
    {
        return sourceMapCallback_;
    }

    void SetSourceMapTranslateCallback(SourceMapTranslateCallback cb)
    {
        sourceMapTranslateCallback_ = cb;
    }

    SourceMapTranslateCallback GetSourceMapTranslateCallback() const
    {
        return sourceMapTranslateCallback_;
    }

    size_t GetNativePointerListSize()
    {
        return nativePointerList_.size();
    }

    const CList<JSNativePointer *> GetNativePointerList() const
    {
        return nativePointerList_;
    }

    size_t GetConcurrentNativePointerListSize() const
    {
        return concurrentNativePointerList_.size();
    }

    void SetResolveBufferCallback(ResolveBufferCallback cb)
    {
        resolveBufferCallback_ = cb;
    }

    ResolveBufferCallback GetResolveBufferCallback() const
    {
        return resolveBufferCallback_;
    }

    void SetSearchHapPathCallBack(SearchHapPathCallBack cb)
    {
        SearchHapPathCallBack_ = cb;
    }

    SearchHapPathCallBack GetSearchHapPathCallBack() const
    {
        return SearchHapPathCallBack_;
    }

    void SetUnloadNativeModuleCallback(const UnloadNativeModuleCallback &cb)
    {
        unloadNativeModuleCallback_ = cb;
    }

    UnloadNativeModuleCallback GetUnloadNativeModuleCallback() const
    {
        return unloadNativeModuleCallback_;
    }

    void SetConcurrentCallback(ConcurrentCallback callback, void *data)
    {
        concurrentCallback_ = callback;
        concurrentData_ = data;
    }

    void TriggerConcurrentCallback(JSTaggedValue result, JSTaggedValue hint);

    void WorkersetInfo(EcmaVM *workerVm);

    EcmaVM *GetWorkerVm(uint32_t tid);

    bool DeleteWorker(EcmaVM *workerVm);

    bool SuspendWorkerVm(uint32_t tid);

    void ResumeWorkerVm(uint32_t tid);

    template<typename Callback>
    void EnumerateWorkerVm(Callback cb)
    {
        // since there is a lock, so cannot mark function const
        LockHolder lock(mutex_);
        for (const auto &item : workerList_) {
            cb(item.second);
        }
    }

    bool IsWorkerThread() const
    {
        return options_.IsWorker();
    }

    bool IsRestrictedWorkerThread() const
    {
        return options_.IsRestrictedWorker();
    }

    bool IsBundlePack() const
    {
        return isBundlePack_;
    }

    void SetIsBundlePack(bool value)
    {
        isBundlePack_ = value;
    }

    // UnifiedOhmUrlPack means app compiles ohmurl using old format like "@bundle:",
    // or new unified rules like "@normalize:"
    // if pkgContextInfoList is empty, means use old ohmurl packing.
    bool IsNormalizedOhmUrlPack() const
    {
        return !pkgContextInfoList_.empty();
    }

    void SetPkgNameList(const CMap<CString, CString> &list)
    {
        pkgNameList_ = list;
    }

    CMap<CString, CString> GetPkgNameList() const
    {
        return pkgNameList_;
    }

    inline CString GetPkgName(const CString &moduleName) const
    {
        auto it = pkgNameList_.find(moduleName);
        if (it == pkgNameList_.end()) {
            LOG_ECMA(INFO) << " Get Pkg Name failed";
            return moduleName;
        }
        return it->second;
    }

    inline CMap<CString, CMap<CString, CVector<CString>>> GetPkgContextInfoLit() const
    {
        return pkgContextInfoList_;
    }

    inline CString GetPkgNameWithAlias(const CString &alias) const
    {
        auto it = pkgAliasList_.find(alias);
        if (it == pkgAliasList_.end()) {
            return alias;
        }
        return it->second;
    }

    void SetPkgAliasList(const CMap<CString, CString> &list)
    {
        pkgAliasList_ = list;
    }

    CMap<CString, CString> GetPkgAliasList() const
    {
        return pkgAliasList_;
    }

    void SetMockModuleList(const std::map<std::string, std::string> &list)
    {
        for (auto it = list.begin(); it != list.end(); ++it) {
            mockModuleList_.emplace(it->first.c_str(), it->second.c_str());
        }
    }

    inline bool IsMockModule(const CString &moduleStr) const
    {
        if (mockModuleList_.empty()) {
            return false;
        }
        auto it = mockModuleList_.find(moduleStr);
        if (it == mockModuleList_.end()) {
            return false;
        }
        return true;
    }

    inline CString GetMockModule(const CString &module) const
    {
        auto it = mockModuleList_.find(module);
        if (it == mockModuleList_.end()) {
            LOG_ECMA(FATAL) << " Get Mock Module failed";
        }
        return it->second;
    }

#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    void DeleteHeapProfile();
    HeapProfilerInterface *GetHeapProfile();
    void  SetHeapProfile(HeapProfilerInterface *heapProfile) { heapProfile_ = heapProfile; }
    HeapProfilerInterface *GetOrNewHeapProfile();
    void StartHeapTracking();
    void StopHeapTracking();
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

    std::pair<std::string, std::string> GetCurrentModuleInfo(bool needRecordName = false);

    void SetHmsModuleList(const std::vector<panda::HmsMap> &list);

    bool IsHmsModule(const CString &moduleStr) const;

    CString GetHmsModule(const CString &module) const;

    void SetpkgContextInfoList(const CMap<CString, CMap<CString, CVector<CString>>> &list);

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

#if defined(ECMASCRIPT_SUPPORT_TRACING)
    Tracing *GetTracing() const
    {
        return tracing_;
    }

    void SetTracing(Tracing *tracing)
    {
        tracing_ = tracing;
    }
#endif

    std::shared_ptr<PGOProfiler> GetPGOProfiler() const
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

    JSTaggedValue FastCallAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp);

    void RegisterUncatchableErrorHandler(const UncatchableErrorHandler &uncatchableErrorHandler)
    {
        uncatchableErrorHandler_ = uncatchableErrorHandler;
    }

    // handle uncatchable errors, such as oom
    void HandleUncatchableError()
    {
        if (uncatchableErrorHandler_ != nullptr) {
            panda::TryCatch trycatch(this);
            uncatchableErrorHandler_(trycatch);
        }
    }

    void DumpCallTimeInfo();

    FunctionCallTimer *GetCallTimer() const
    {
        return callTimer_;
    }

    EcmaStringTable *GetEcmaStringTable() const
    {
        ASSERT(stringTable_ != nullptr);
        return stringTable_;
    }

    void IncreaseCallDepth()
    {
        callDepth_++;
    }

    void DecreaseCallDepth()
    {
        ASSERT(callDepth_ > 0);
        callDepth_--;
    }

    bool IsTopLevelCallDepth()
    {
        return callDepth_ == 0;
    }

    void SetProfilerState(bool state)
    {
        isProfiling_ = state;
    }

    bool GetProfilerState()
    {
        return isProfiling_;
    }

    JSObjectResizingStrategy *GetJSObjectResizingStrategy()
    {
        return strategy_;
    }

    CMap<uint32_t, EcmaVM *> GetWorkList() const
    {
        return workerList_;
    }

    Jit *GetJit() const;
    bool PUBLIC_API IsEnableFastJit() const;
    bool PUBLIC_API IsEnableBaselineJit() const;
    void EnableJit();

    bool IsEnableOsr() const
    {
        return isEnableOsr_;
    }

    void SetEnableOsr(bool state)
    {
        isEnableOsr_ = state;
    }

    bool isOverLimit() const
    {
        return overLimit_;
    }

    void SetOverLimit(bool state)
    {
        overLimit_ = state;
    }

    AOTFileManager *GetAOTFileManager() const
    {
        return aotFileManager_;
    }

    uint32_t GetTid() const
    {
        return thread_->GetThreadId();
    }

    std::vector<std::pair<NativePointerCallback, std::pair<void *, void *>>> &GetNativePointerCallbacks()
    {
        return nativePointerCallbacks_;
    }

    void SetIsJitCompileVM(bool isJitCompileVM)
    {
        isJitCompileVM_ = isJitCompileVM;
    }

    bool IsJitCompileVM() const
    {
        return isJitCompileVM_;
    }

    void SetEnableAotCrashEscapeVM(bool enableAotCrashEscape)
    {
        enableAotCrashEscape_ = enableAotCrashEscape;
    }

    bool IsEnableAotCrashEscapeVM() const
    {
        return enableAotCrashEscape_;
    }

    static void SetMultiThreadCheck(bool multiThreadCheck)
    {
        multiThreadCheck_ = multiThreadCheck;
    }

    PUBLIC_API static bool GetMultiThreadCheck()
    {
        return multiThreadCheck_;
    }

    static void InitializeIcuData(const JSRuntimeOptions &options);

    std::vector<std::pair<NativePointerCallback, std::pair<void *, void *>>> &GetSharedNativePointerCallbacks()
    {
        return sharedNativePointerCallbacks_;
    }
protected:

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo) const;

private:
    void ClearBufferData();
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
    GCKeyStats *gcKeyStats_ {nullptr};
    EcmaStringTable *stringTable_ {nullptr};
    PUBLIC_API static bool multiThreadCheck_;

    // VM memory management.
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    Chunk chunk_;
    Heap *heap_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    CList<JSNativePointer *> nativePointerList_;
    CList<JSNativePointer *> concurrentNativePointerList_;
    std::vector<std::pair<NativePointerCallback, std::pair<void *, void *>>> nativePointerCallbacks_ {};
    CList<JSNativePointer *> sharedNativePointerList_;
    std::vector<std::pair<NativePointerCallback, std::pair<void *, void *>>> sharedNativePointerCallbacks_ {};
    // VM execution states.
    JSThread *thread_ {nullptr};

    // VM resources.
    SnapshotEnv *snapshotEnv_ {nullptr};
    bool optionalLogEnabled_ {false};
    // Debugger
    tooling::JsDebuggerManager *debuggerManager_ {nullptr};
    // isBundle means app compile mode is JSBundle
    bool isBundlePack_ {true};
#if !WIN_OR_MAC_OR_IOS_PLATFORM
    HeapProfilerInterface *heapProfile_ {nullptr};
#endif
    CString assetPath_;
    CString bundleName_;
    CString moduleName_;
    CList<CString> deregisterModuleList_;
    CMap<CString, CString> mockModuleList_;
    CMap<CString, HmsMap> hmsModuleList_;
    CMap<CString, CString> pkgNameList_;
    CMap<CString, CMap<CString, CVector<CString>>> pkgContextInfoList_;
    CMap<CString, CString> pkgAliasList_;
    NativePtrGetter nativePtrGetter_ {nullptr};
    SourceMapCallback sourceMapCallback_ {nullptr};
    SourceMapTranslateCallback sourceMapTranslateCallback_ {nullptr};
    void *loop_ {nullptr};

    // resolve path to get abc's buffer
    ResolveBufferCallback resolveBufferCallback_ {nullptr};

    // delete the native module and dlclose so from NativeModuleManager
    UnloadNativeModuleCallback unloadNativeModuleCallback_ {nullptr};

    // Concurrent taskpool callback and data
    ConcurrentCallback concurrentCallback_ {nullptr};
    void *concurrentData_ {nullptr};

    // serch happath callback
    SearchHapPathCallBack SearchHapPathCallBack_ {nullptr};

    // vm parameter configurations
    EcmaParamConfiguration ecmaParamConfiguration_;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    CpuProfiler *profiler_ {nullptr};
#endif
#if defined(ECMASCRIPT_SUPPORT_TRACING)
    Tracing *tracing_ {nullptr};
#endif
    FunctionCallTimer *callTimer_ {nullptr};
    JSObjectResizingStrategy *strategy_ {nullptr};

    // For Native MethodLiteral
    static void *InternalMethodTable[static_cast<uint8_t>(MethodIndex::METHOD_END)];
    CVector<JSTaggedValue> internalNativeMethods_;

    // For repair patch.
    QuickFixManager *quickFixManager_ {nullptr};

    // PGO Profiler
    std::shared_ptr<PGOProfiler> pgoProfiler_ {nullptr};

    //AOT File Manager
    AOTFileManager *aotFileManager_ {nullptr};

    // c++ call js
    size_t callDepth_ {0};

    bool isProfiling_ {false};

    DeviceDisconnectCallback deviceDisconnectCallback_ {nullptr};

    UncatchableErrorHandler uncatchableErrorHandler_ {nullptr};

    friend class Snapshot;
    friend class SnapshotProcessor;
    friend class ObjectFactory;
    friend class ValueSerializer;
    friend class panda::JSNApi;
    friend class JSPandaFileExecutor;
    friend class EcmaContext;
    friend class JitVM;
    CMap<uint32_t, EcmaVM *> workerList_ {};
    Mutex mutex_;
    bool isEnableOsr_ {false};
    bool isJitCompileVM_ {false};
    bool enableAotCrashEscape_ {true};
    bool overLimit_ {false};
};
}  // namespace ecmascript
}  // namespace panda

#endif
