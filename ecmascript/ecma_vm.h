/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/taskpool/taskpool.h"
#include "libpandafile/bytecode_instruction-inl.h"

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
class AsyncStackTrace;
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
class JSThread;

namespace pgo {
    class PGOProfiler;
} // namespace pgo

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
using SourceMapTranslateCallback = std::function<bool(std::string& url, int& line, int& column,
    std::string &packageName)>;
using ResolveBufferCallback =
    std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize, std::string &errorMsg)>;
using TimerCallbackFunc = void (*)(void *data);
using TimerTaskCallback = void* (*)(EcmaVM *vm, void *data, TimerCallbackFunc func, uint64_t timeout, bool repeat);
using CancelTimerCallback = void (*)(void *timerCallbackInfo);
using UnloadNativeModuleCallback = std::function<bool(const std::string &moduleKey)>;
using RequestAotCallback =
    std::function<int32_t(const std::string &bundleName, const std::string &moduleName, int32_t triggerMode)>;
using SearchHapPathCallBack = std::function<bool(const std::string moduleName, std::string &hapPath)>;
using DeviceDisconnectCallback = std::function<bool()>;
using UncatchableErrorHandler = std::function<void(panda::TryCatch&)>;
using OnErrorCallback = std::function<void(Local<ObjectRef> value, void *data)>;
using StopPreLoadSoCallback = std::function<void()>;

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

    void SetPostForked(bool isPostForked)
    {
        isPostForked_ = isPostForked;
    }

    bool IsPostForked() const
    {
        return isPostForked_;
    }

    bool IsAsynTranslateClasses()
    {
        if (!GetJSOptions().IsAsyncLoadAbcTest()) {
            return IsPostForked() && GetJSOptions().IsAsyncLoadAbc();
        }
        return GetJSOptions().IsAsyncLoadAbc();
    }

    ObjectFactory *GetFactory() const
    {
        return factory_;
    }

    void InitializePGOProfiler();
    void ResetPGOProfiler();
    void DisablePGOProfilerWithAOTFile(const std::string &aotFileName);

    bool PUBLIC_API IsEnablePGOProfiler() const;
    bool PUBLIC_API IsEnableMutantArray() const;
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

    void PUBLIC_API CheckThread() const;
    JSThread *GetAndFastCheckJSThread() const;
    bool CheckSingleThread() const;

    ARK_INLINE bool GetThreadCheckStatus() const
    {
        return options_.EnableThreadCheck() || EcmaVM::GetMultiThreadCheck();
    }

    ARK_INLINE JSThread *GetJSThread() const
    {
        // default enable multi-thread check in asan
#ifdef ECMASCRIPT_ENABLE_ASAN_THREAD_CHECK
        CheckThread();
#else
        if (GetThreadCheckStatus()) {
            CheckThread();
        }
#endif
        return thread_;
    }

    JSThread *GetJSThreadNoCheck() const
    {
        return thread_;
    }

    bool ICEnabled() const
    {
        return icEnabled_;
    }

    void PushToNativePointerList(JSNativePointer *pointer, Concurrent isConcurrent = Concurrent::NO);
    void RemoveFromNativePointerList(JSNativePointer *pointer);
    void PushToDeregisterModuleList(const CString &module);
    void RemoveFromDeregisterModuleList(CString module);
    bool ContainInDeregisterModuleList(CString module);
    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
    bool IsOptionalLogEnabled() const
    {
        return optionalLogEnabled_;
    }

    void Iterate(RootVisitor &v, VMRootVisitType type);

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

    AsyncStackTrace *GetAsyncStackTrace() const
    {
        return asyncStackTrace_;
    }

    uint32_t GetAsyncTaskId();

    bool InsertAsyncStackTrace(const JSHandle<JSPromise> &promise);

    bool RemoveAsyncStackTrace(const JSHandle<JSPromise> &promise);

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

    void SetResolveBufferCallback(ResolveBufferCallback cb)
    {
        resolveBufferCallback_ = cb;
    }

    ResolveBufferCallback GetResolveBufferCallback() const
    {
        return resolveBufferCallback_;
    }

    void SetTimerTaskCallback(TimerTaskCallback callback)
    {
        timerTaskCallback_ = callback;
    }

    TimerTaskCallback GetTimerTaskCallback() const
    {
        return timerTaskCallback_;
    }

    void SetCancelTimerCallback(CancelTimerCallback callback)
    {
        cancelTimerCallback_ = callback;
    }

    CancelTimerCallback GetCancelTimerCallback() const
    {
        return cancelTimerCallback_;
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

    void SetOnErrorCallback(OnErrorCallback callback, void* data)
    {
        onErrorCallback_ = callback;
        onErrorData_ = data;
    }

    OnErrorCallback GetOnErrorCallback()
    {
        return onErrorCallback_;
    }

    void* GetOnAllData()
    {
        return onErrorData_;
    }
    
    void AddStopPreLoadCallback(const StopPreLoadSoCallback &cb)
    {
        stopPreLoadCallbacks_.emplace_back(cb);
    }

    CVector<StopPreLoadSoCallback> GetStopPreLoadCallbacks() const
    {
        return stopPreLoadCallbacks_;
    }

    void StopPreLoadSoOrAbc();

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
    bool IsNormalizedOhmUrlPack()
    {
        ReadLockHolder lock(pkgContextInfoLock_);
        return !pkgContextInfoList_.empty();
    }

    void SetPkgNameList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgNameListLock_);
        pkgNameList_ = list;
    }

    void UpdatePkgNameList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgNameListLock_);
        pkgNameList_.insert(list.begin(), list.end());
    }

    CMap<CString, CString> GetPkgNameList()
    {
        ReadLockHolder lock(pkgNameListLock_);
        return pkgNameList_;
    }

    inline CString GetPkgName(const CString &moduleName)
    {
        ReadLockHolder lock(pkgNameListLock_);
        auto it = pkgNameList_.find(moduleName);
        if (it == pkgNameList_.end()) {
            LOG_ECMA(INFO) << " Get Pkg Name failed";
            return moduleName;
        }
        return it->second;
    }

    void UpdatePkgContextInfoList(const CMap<CString, CMap<CString, CVector<CString>>> &list)
    {
        WriteLockHolder lock(pkgContextInfoLock_);
        pkgContextInfoList_.insert(list.begin(), list.end());
    }

    inline CMap<CString, CMap<CString, CVector<CString>>> GetPkgContextInfoList()
    {
        ReadLockHolder lock(pkgContextInfoLock_);
        return pkgContextInfoList_;
    }

    inline CString GetPkgNameWithAlias(const CString &alias)
    {
        ReadLockHolder lock(pkgAliasListLock_);
        auto it = pkgAliasList_.find(alias);
        if (it == pkgAliasList_.end()) {
            return alias;
        }
        return it->second;
    }

    void SetPkgAliasList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgAliasListLock_);
        pkgAliasList_ = list;
    }

    void UpdatePkgAliasList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgAliasListLock_);
        pkgAliasList_.insert(list.begin(), list.end());
    }

    CMap<CString, CString> GetPkgAliasList()
    {
        ReadLockHolder lock(pkgAliasListLock_);
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
        LOG_ECMA_MEM(FATAL) << "Out of Memory";
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

    int GetProcessStartRealtime() const
    {
        return processStartRealtime_;
    }
    
    void SetProcessStartRealtime(int value)
    {
        processStartRealtime_ = value;
    }

    Jit *GetJit() const;
    bool PUBLIC_API IsEnableFastJit() const;
    bool PUBLIC_API IsEnableBaselineJit() const;

    bool IsEnableOsr() const
    {
        return isEnableOsr_;
    }

    void SetEnableOsr(bool state)
    {
        isEnableOsr_ = state;
    }

    AOTFileManager *GetAOTFileManager() const
    {
        return aotFileManager_;
    }

    uint32_t GetTid() const;

    std::vector<NativePointerCallbackData> &GetConcurrentNativePointerCallbacks()
    {
        return concurrentNativeCallbacks_;
    }

    AsyncNativeCallbacksPack &GetAsyncNativePointerCallbacksPack()
    {
        return asyncNativeCallbacksPack_;
    }

    void SetIsJitCompileVM(bool isJitCompileVM)
    {
        isJitCompileVM_ = isJitCompileVM;
    }

    bool IsJitCompileVM() const
    {
        return isJitCompileVM_;
    }

    static void SetMultiThreadCheck(bool multiThreadCheck)
    {
        multiThreadCheck_ = multiThreadCheck;
    }

    PUBLIC_API static bool GetMultiThreadCheck()
    {
        return multiThreadCheck_;
    }

    static void SetErrorInfoEnhance(bool errorInfoEnhance)
    {
        errorInfoEnhanced_ = errorInfoEnhance;
    }

    static bool GetErrorInfoEnhance()
    {
        return errorInfoEnhanced_;
    }

    static void InitializeIcuData(const JSRuntimeOptions &options);

    static int InitializeStartRealTime();

#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    void ResetScopeLockStats()
    {
        enterThreadManagedScopeCount_ = 0;
        enterJsiNativeScopeCount_ = 0;
        enterFastNativeScopeCount_ = 0;
        updateThreadStateTransCount_ = 0;
        stringTableLockCount_ = 0;
    }

    bool IsCollectingScopeLockStats() const
    {
        return isCollectingScopeLockStats_;
    }

    void StartCollectingScopeLockStats()
    {
        isCollectingScopeLockStats_ = true;
    }
    
    void StopCollectingScopeLockStats()
    {
        isCollectingScopeLockStats_ = false;
    }
    
    int GetEnterThreadManagedScopeCount() const
    {
        return enterThreadManagedScopeCount_;
    }

    void IncreaseEnterThreadManagedScopeCount()
    {
        enterThreadManagedScopeCount_++;
    }

    int GetEnterFastNativeScopeCount() const
    {
        return enterFastNativeScopeCount_;
    }

    void IncreaseEnterFastNativeScopeCount()
    {
        enterFastNativeScopeCount_++;
    }

    int GetEnterJsiNativeScopeCount() const
    {
        return enterJsiNativeScopeCount_;
    }

    void IncreaseEnterJsiNativeScopeCount()
    {
        enterJsiNativeScopeCount_++;
    }

    int GetUpdateThreadStateTransCount() const
    {
        return updateThreadStateTransCount_;
    }

    void IncreaseUpdateThreadStateTransCount()
    {
        updateThreadStateTransCount_++;
    }

    int GetStringTableLockCount() const
    {
        return stringTableLockCount_;
    }

    void IncreaseStringTableLockCount()
    {
        stringTableLockCount_++;
    }
#endif

    bool GetEnableJitLogSkip() const
    {
        return enableJitLogSkip_;
    }

    void SetEnableJitLogSkip(bool flag)
    {
        enableJitLogSkip_ = flag;
    }

    void AddAOTSnapShotStats(std::string tag, uint32_t count = 1)
    {
        aotSnapShotStatsMap_[tag] += count;
    }

    void PUBLIC_API PrintAOTSnapShotStats();

    void SetVMAPIVersion(uint32_t APIVersion)
    {
        apiVersion_ = APIVersion;
    }

    uint32_t GetVMAPIVersion()
    {
        return apiVersion_;
    }

#if ECMASCRIPT_ENABLE_COLLECTING_OPCODES
    void SetBytecodeStatsStack(std::unordered_map<BytecodeInstruction::Opcode, int> &bytecodeStatsMap)
    {
        bytecodeStatsStack_.push(bytecodeStatsMap);
    }

    std::stack<std::unordered_map<BytecodeInstruction::Opcode, int>>& GetBytecodeStatsStack()
    {
        return bytecodeStatsStack_;
    }

    void PrintCollectedByteCode();
#endif

protected:

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo) const;

private:
    void ClearBufferData();
    void CheckStartCpuProfiler();

    // For Internal Native MethodLiteral.
    void GenerateInternalNativeMethods();
    void CacheToGlobalConstants(JSTaggedValue value, ConstantIndex constant);

    NO_MOVE_SEMANTIC(EcmaVM);
    NO_COPY_SEMANTIC(EcmaVM);

    // VM startup states.
    JSRuntimeOptions options_;
    bool icEnabled_ {true};
    bool initialized_ {false};
    bool isPostForked_ {false};
    GCStats *gcStats_ {nullptr};
    GCKeyStats *gcKeyStats_ {nullptr};
    EcmaStringTable *stringTable_ {nullptr};
    PUBLIC_API static bool multiThreadCheck_;
    static bool errorInfoEnhanced_;

    //apiVersion states
    uint32_t apiVersion_ = 8;

    // VM memory management.
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    Chunk chunk_;
    Heap *heap_ {nullptr};
    ObjectFactory *factory_ {nullptr};

    std::vector<NativePointerCallbackData> concurrentNativeCallbacks_ {};
    AsyncNativeCallbacksPack asyncNativeCallbacksPack_ {};
    // VM execution states.
    JSThread *thread_ {nullptr};

    CUnorderedMap<std::string, uint32_t> aotSnapShotStatsMap_;

    // VM resources.
    SnapshotEnv *snapshotEnv_ {nullptr};
    bool optionalLogEnabled_ {false};
    // Debugger
    tooling::JsDebuggerManager *debuggerManager_ {nullptr};

    // DFX
    AsyncStackTrace *asyncStackTrace_ {nullptr};

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
    std::map<CString, HmsMap> hmsModuleList_;
    CMap<CString, CString> pkgNameList_;
    CMap<CString, CMap<CString, CVector<CString>>> pkgContextInfoList_;
    CMap<CString, CString> pkgAliasList_;
    RWLock pkgContextInfoLock_;
    RWLock pkgAliasListLock_;
    RWLock pkgNameListLock_;

    CVector<StopPreLoadSoCallback> stopPreLoadCallbacks_;
    NativePtrGetter nativePtrGetter_ {nullptr};
    SourceMapCallback sourceMapCallback_ {nullptr};
    SourceMapTranslateCallback sourceMapTranslateCallback_ {nullptr};
    void *loop_ {nullptr};

    // resolve path to get abc's buffer
    ResolveBufferCallback resolveBufferCallback_ {nullptr};

    // set timer task to execute callback on time
    TimerTaskCallback timerTaskCallback_ {nullptr};
    // set cancel timer task to execute callback on time
    CancelTimerCallback cancelTimerCallback_ {nullptr};

    // delete the native module and dlclose so from NativeModuleManager
    UnloadNativeModuleCallback unloadNativeModuleCallback_ {nullptr};

    // Concurrent taskpool callback and data
    ConcurrentCallback concurrentCallback_ {nullptr};
    void *concurrentData_ {nullptr};

    // Error callback
    OnErrorCallback onErrorCallback_ {nullptr};
    void *onErrorData_ {nullptr};

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

    // process StartRealTime
    int processStartRealtime_ = 0;

    bool enableJitLogSkip_ = true;

#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    // Stats for Thread-State-Transition and String-Table Locks
    bool isCollectingScopeLockStats_ = false;
    int enterThreadManagedScopeCount_ = 0;
    int enterFastNativeScopeCount_ = 0;
    int enterJsiNativeScopeCount_ = 0;
    int updateThreadStateTransCount_ = 0;
    int stringTableLockCount_ = 0;
#endif

#if ECMASCRIPT_ENABLE_COLLECTING_OPCODES
    std::stack<std::unordered_map<BytecodeInstruction::Opcode, int>> bytecodeStatsStack_;
#endif
};
}  // namespace ecmascript
}  // namespace panda

#endif
