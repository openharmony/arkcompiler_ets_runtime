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
#include "ecmascript/global_handle_collection.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/patch/patch_loader.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/waiter_list.h"
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
class RegExpParserCache;
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

class OptCodeProfiler;
class TypedOpProfiler;
class FunctionProtoTransitionTable;
struct CJSInfo;

namespace kungfu {
class PGOTypeManager;
} // namespace kungfu

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
class SustainingJSHandle;
class SustainingJSHandleList;
class AbcBufferCache;
struct CJSInfo;
enum class CompareStringsOption : uint8_t;

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
using StopPreLoadSoCallback = std::function<void()>;
using HostPromiseRejectionTracker = void (*)(const EcmaVM* vm,
                                             const JSHandle<JSPromise> promise,
                                             const JSHandle<JSTaggedValue> reason,
                                             PromiseRejectionEvent operation,
                                             void* data);
using PromiseRejectCallback = void (*)(void* info);

#ifdef USE_CMC_GC
using namespace panda;
#endif

enum class IcuFormatterType: uint8_t {
    SIMPLE_DATE_FORMAT_DEFAULT,
    SIMPLE_DATE_FORMAT_DATE,
    SIMPLE_DATE_FORMAT_TIME,
    NUMBER_FORMATTER,
    COLLATOR,
    ICU_FORMATTER_TYPE_COUNT
};

class IntlCache {
public:
    IntlCache() = default;

    void SetDefaultLocale(const std::string& locale)
    {
        defaultLocale_ = locale;
    }

    const std::string& GetDefaultLocale() const
    {
        return defaultLocale_;
    }

    void SetDefaultCompareStringsOption(const CompareStringsOption csOption)
    {
        defaultCompareStringsOption_ = csOption;
    }

    std::optional<CompareStringsOption> GetDefaultCompareStringsOption() const
    {
        return defaultCompareStringsOption_;
    }

    void SetIcuFormatterToCache(IcuFormatterType type, const std::string& locale, void* icuObj,
                                NativePointerCallback deleteEntry = nullptr)
    {
        icuObjCache_[static_cast<int>(type)] = IcuFormatter(locale, icuObj, deleteEntry);
    }

    void* GetIcuFormatterFromCache(IcuFormatterType type, const std::string& locale) const
    {
        const auto& icuFormatter = icuObjCache_[static_cast<int>(type)];
        return icuFormatter.locale == locale ? icuFormatter.icuObj : nullptr;
    }

    void ClearIcuCache(void* vmPtr)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(IcuFormatterType::ICU_FORMATTER_TYPE_COUNT); ++i) {
            auto& icuFormatter = icuObjCache_[i];
            if (icuFormatter.deleteEntry != nullptr) {
                // Fill nullptr into the IcuDeleteEntry's unused env (specifically for napi) field.
                icuFormatter.deleteEntry(nullptr, icuFormatter.icuObj, vmPtr);
            }
        }
    }

private:
    class IcuFormatter {
    public:
        std::string locale;
        void* icuObj {nullptr};
        NativePointerCallback deleteEntry {nullptr};

        IcuFormatter() = default;
        IcuFormatter(const std::string& locale, void* icuObj, NativePointerCallback deleteEntry = nullptr)
            : locale(locale), icuObj(icuObj), deleteEntry(deleteEntry) {}
    };

    std::string defaultLocale_;
    std::optional<CompareStringsOption> defaultCompareStringsOption_ {std::nullopt};
    IcuFormatter icuObjCache_[static_cast<uint32_t>(IcuFormatterType::ICU_FORMATTER_TYPE_COUNT)];
};

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

    void SetPreForked(bool preForked)
    {
        preForked_ = preForked;
    }

    void SetPostForked(bool postForked)
    {
        postForked_ = postForked;
    }

    bool IsPreForked() const
    {
        return preForked_;
    }

    bool IsPostForked() const
    {
        return postForked_;
    }

    void SetAgentCanSuspend(bool agentCanSuspend)
    {
        agentCanSuspend_ = agentCanSuspend;
    }

    bool GetAgentCanSuspend() const
    {
        return agentCanSuspend_;
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

    JSHandle<GlobalEnv> PUBLIC_API GetGlobalEnv() const
    {
        return thread_->GetGlobalEnv();
    }

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

    EcmaRuntimeStat *GetRuntimeStat() const
    {
        return runtimeStat_;
    }

    void InitializeEcmaScriptRunStat();
    void SetRuntimeStatEnable(bool flag);

    bool IsOptionalLogEnabled() const
    {
        return optionalLogEnabled_;
    }

    JSTaggedType *GetHandleScopeStorageNext() const
    {
        return handleScopeStorageNext_;
    }

    void SetHandleScopeStorageNext(JSTaggedType *value)
    {
        handleScopeStorageNext_ = value;
    }

    JSTaggedType *GetHandleScopeStorageEnd() const
    {
        return handleScopeStorageEnd_;
    }

    void SetHandleScopeStorageEnd(JSTaggedType *value)
    {
        handleScopeStorageEnd_ = value;
    }

    int GetCurrentHandleStorageIndex() const
    {
        return currentHandleStorageIndex_;
    }

    JSTaggedType *GetPrimitiveScopeStorageNext() const
    {
        return primitiveScopeStorageNext_;
    }

    void SetPrimitiveScopeStorageNext(JSTaggedType *value)
    {
        primitiveScopeStorageNext_ = value;
    }

    JSTaggedType *GetPrimitiveScopeStorageEnd() const
    {
        return primitiveScopeStorageEnd_;
    }

    void SetPrimitiveScopeStorageEnd(JSTaggedType *value)
    {
        primitiveScopeStorageEnd_ = value;
    }

    int GetCurrentPrimitiveStorageIndex() const
    {
        return currentPrimitiveStorageIndex_;
    }

    uintptr_t *ExpandHandleStorage();
    void ShrinkHandleStorage(int prevIndex);
    void DeleteHandleStorage();
    uintptr_t *ExpandPrimitiveStorage();
    void ShrinkPrimitiveStorage(int prevIndex);
    void DeletePrimitiveStorage();

    size_t IterateHandle(RootVisitor &visitor);
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

    PUBLIC_API AOTFileManager *GetAOTFileManager() const
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

    IntlCache& GetIntlCache()
    {
        return intlCache_;
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

    JSHandle<job::MicroJobQueue> GetMicroJobQueue() const;

    bool HasPendingJob() const;

    bool ExecutePromisePendingJob();

    void SetPromiseRejectCallback(PromiseRejectCallback cb)
    {
        promiseRejectCallback_ = cb;
    }

    PromiseRejectCallback GetPromiseRejectCallback() const
    {
        return promiseRejectCallback_;
    }

    void SetHostPromiseRejectionTracker(HostPromiseRejectionTracker cb)
    {
        hostPromiseRejectionTracker_ = cb;
    }

    void PromiseRejectionTracker(const JSHandle<JSPromise> &promise,
                                 const JSHandle<JSTaggedValue> &reason, PromiseRejectionEvent operation)
    {
        if (hostPromiseRejectionTracker_ != nullptr) {
            hostPromiseRejectionTracker_(this, promise, reason, operation, data_);
        }
    }

    void SetPromiseRejectInfoData(void* data)
    {
        data_ = data;
    }

    bool IsExecutingPendingJob() const
    {
        return isProcessingPendingJob_.load();
    }
    JSTaggedValue PUBLIC_API FindUnsharedConstpool(JSTaggedValue sharedConstpool);
    JSTaggedValue PUBLIC_API FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool);
    void EraseUnusedConstpool(const JSPandaFile *jsPandaFile, int32_t index, int32_t constpoolIndex);
    JSTaggedValue PUBLIC_API FindConstpool(const JSPandaFile *jsPandaFile, int32_t index);
    // For new version instruction.
    JSTaggedValue PUBLIC_API FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id);
    bool HasCachedConstpool(const JSPandaFile *jsPandaFile) const;
    JSHandle<ConstantPool> AddOrUpdateConstpool(const JSPandaFile *jsPandaFile,
                                                JSHandle<ConstantPool> constpool,
                                                int32_t index = 0);
    void SetUnsharedConstpool(JSHandle<ConstantPool> sharedConstpool, JSTaggedValue unsharedConstpool);
    void SetUnsharedConstpool(int32_t constpoolIndex, JSTaggedValue unsharedConstpool);
    void CreateAllConstpool(const JSPandaFile *jsPandaFile);
    std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> FindConstpools(const JSPandaFile *jsPandaFile);
    void UpdateConstpoolWhenDeserialAI(const std::string& fileName,
                                       JSHandle<ConstantPool> aiCP,
                                       int32_t index = 0);
    JSHandle<ConstantPool> PUBLIC_API FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
                                                            panda_file::File::EntityId id);
    void ClearCachedConstantPool()
    {
        cachedSharedConstpools_.clear();
    }

    JSTaggedValue *GetUnsharedConstpoolsPointer() const
    {
        return unsharedConstpools_;
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

    RegExpParserCache *GetRegExpParserCache() const
    {
        ASSERT(regExpParserCache_ != nullptr);
        return regExpParserCache_;
    }

    WaiterListNode *GetWaiterListNode()
    {
        return &waiterListNode_;
    }

    AbcBufferCache *GetAbcBufferCache() const
    {
        return abcBufferCache_;
    }

    void AddSustainingJSHandle(SustainingJSHandle *sustainingHandle);
    void RemoveSustainingJSHandle(SustainingJSHandle *sustainingHandle);

    kungfu::PGOTypeManager* GetPTManager() const
    {
        return ptManager_;
    }

    OptCodeProfiler* GetOptCodeProfiler() const
    {
        return optCodeProfiler_;
    }

    TypedOpProfiler* GetTypedOpProfiler() const
    {
        return typedOpProfiler_;
    }

    FunctionProtoTransitionTable* GetFunctionProtoTransitionTable() const
    {
        return functionProtoTransitionTable_;
    }

    void PrintOptStat();
    void DumpAOTInfo() const DUMP_API_ATTR;
    std::tuple<uint64_t, uint8_t*, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo(uintptr_t retAddr,
                                                                                       bool isDeopt) const;
    void LoadStubFile();
    bool LoadAOTFilesInternal(const std::string& aotFileName);
    bool LoadAOTFiles(const std::string& aotFileName);
    bool LoadAOTFiles(const std::string &aotFileName,
                      std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb);
    void PUBLIC_API LoadProtoTransitionTable(JSTaggedValue constpool);
    void PUBLIC_API ResetProtoTransitionTableOnConstpool(JSTaggedValue constpool);

    void AddPatchModule(const CString &recordName, const JSHandle<JSTaggedValue> moduleRecord)
    {
        cachedPatchModules_.emplace(recordName, moduleRecord);
    }

    JSHandle<JSTaggedValue> FindPatchModule(const CString &recordName) const
    {
        auto iter = cachedPatchModules_.find(recordName);
        if (iter != cachedPatchModules_.end()) {
            return iter->second;
        }
        return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Hole());
    }

    void ClearPatchModules()
    {
        GlobalHandleCollection globalHandleCollection(thread_);
        for (auto &item : cachedPatchModules_) {
            globalHandleCollection.Dispose(item.second);
        }
        cachedPatchModules_.clear();
    }

    StageOfColdReload GetStageOfColdReload() const
    {
        return stageOfColdReload_;
    }

    void SetStageOfColdReload(StageOfColdReload stageOfColdReload)
    {
        stageOfColdReload_ = stageOfColdReload;
    }

    JSTaggedValue ExecuteAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp,
                             bool needPushArgv);

    static void ClearKeptObjects(JSThread *thread);
    static void AddToKeptObjects(JSThread *thread, JSHandle<JSTaggedValue> value);
    void AddModuleManager(ModuleManager *moduleManager);

private:
    void ClearBufferData();
    void ClearConstpoolBufferData();
    void CheckStartCpuProfiler();
    void SetMicroJobQueue(job::MicroJobQueue *queue);
    // For Internal Native MethodLiteral.
    void GenerateInternalNativeMethods();
    void CacheToGlobalConstants(JSTaggedValue value, ConstantIndex constant);

    JSTaggedValue FindConstpoolFromContextCache(const JSPandaFile *jsPandaFile, int32_t index);
    void AddContextConstpoolCache(const JSPandaFile *jsPandaFile,
                                  JSHandle<ConstantPool> constpool,
                                  int32_t index);
    JSTaggedValue FindCachedConstpoolAndLoadAiIfNeeded(const JSPandaFile *jsPandaFile, int32_t index);
    void GrowUnsharedConstpoolArray(int32_t index);
    void ResizeUnsharedConstpoolArray(int32_t oldCapacity, int32_t minCapacity);
    void ClearUnsharedConstpoolArray()
    {
        if (unsharedConstpools_ != nullptr) {
            delete[] unsharedConstpools_;
            unsharedConstpools_ = nullptr;
            thread_->SetUnsharedConstpools(reinterpret_cast<uintptr_t>(nullptr));
            thread_->SetUnsharedConstpoolsArrayLen(0);
        }
    }

    int32_t GetUnsharedConstpoolsArrayLen() const
    {
        return unsharedConstpoolsArrayLen_;
    }

    void SetUnsharedConstpoolsArrayLen(int32_t len)
    {
        unsharedConstpoolsArrayLen_ = len;
    }

    void CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
                      const JSPandaFile *jsPandaFile, std::string_view entryPoint);
    JSTaggedValue InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                          const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                          CJSInfo *cjsInfo = nullptr);
    Expected<JSTaggedValue, bool> InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                                       const ExecuteTypes &executeType = ExecuteTypes::STATIC);
    Expected<JSTaggedValue, bool> InvokeEcmaEntrypointForHotReload(
        const JSPandaFile *jsPandaFile, std::string_view entryPoint, const ExecuteTypes &executeType);
    Expected<JSTaggedValue, bool> CommonInvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
        std::string_view entryPoint, JSHandle<JSFunction> &func, const ExecuteTypes &executeType);

    NO_MOVE_SEMANTIC(EcmaVM);
    NO_COPY_SEMANTIC(EcmaVM);

    // VM startup states.
    JSRuntimeOptions options_;
    bool icEnabled_ {true};
    bool initialized_ {false};
    bool preForked_ {false};
    bool postForked_ {false};
    bool agentCanSuspend_ {true};
    GCStats *gcStats_ {nullptr};
    GCKeyStats *gcKeyStats_ {nullptr};
    EcmaStringTable *stringTable_ {nullptr};
    PUBLIC_API static bool multiThreadCheck_;
    static bool errorInfoEnhanced_;

    // for constpool.The shared constpool includes string, method, sendable classLiteral, etc.
    // The unshared constpool includes objectLiteral, arrayLiteral, unsendable classLiteral, etc.
    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> cachedSharedConstpools_ {};
    JSTaggedValue* unsharedConstpools_ = nullptr;
    int32_t unsharedConstpoolsArrayLen_ = UNSHARED_CONSTANTPOOL_COUNT;
    static constexpr int32_t SHARED_CONSTPOOL_KEY_NOT_FOUND = INT32_MAX; // INT32_MAX :invalid value.

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

    EcmaRuntimeStat *runtimeStat_ {nullptr};

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
    kungfu::PGOTypeManager* ptManager_ {nullptr};

    // c++ call js
    size_t callDepth_ {0};

    bool isProfiling_ {false};

    DeviceDisconnectCallback deviceDisconnectCallback_ {nullptr};

    UncatchableErrorHandler uncatchableErrorHandler_ {nullptr};

    IntlCache intlCache_;

    friend class Snapshot;
    friend class SnapshotProcessor;
    friend class ObjectFactory;
    friend class ValueSerializer;
    friend class panda::JSNApi;
    friend class JSPandaFileExecutor;
    friend class JitVM;
    CMap<uint32_t, EcmaVM *> workerList_ {};
    Mutex mutex_;
    bool isEnableOsr_ {false};
    bool isJitCompileVM_ {false};
    // SustainingJSHandleList for jit compile hold ref
    SustainingJSHandleList *sustainingJSHandleList_ {nullptr};

    // process StartRealTime
    int processStartRealtime_ = 0;

    bool enableJitLogSkip_ = true;

    // Registered Callbacks
    PromiseRejectCallback promiseRejectCallback_ {nullptr};
    HostPromiseRejectionTracker hostPromiseRejectionTracker_ {nullptr};
    void* data_{nullptr};

    JSTaggedValue microJobQueue_ {JSTaggedValue::Hole()};
    std::atomic<bool> isProcessingPendingJob_ {false};

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

    // HandleScope
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;
    static constexpr int32_t MIN_HANDLE_STORAGE_SIZE = 2;
    JSTaggedType *handleScopeStorageNext_ {nullptr};
    JSTaggedType *handleScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_ {};
    int32_t currentHandleStorageIndex_ {-1};
    // PrimitveScope
    static constexpr int32_t MIN_PRIMITIVE_STORAGE_SIZE = 2;
    JSTaggedType *primitiveScopeStorageNext_ {nullptr};
    JSTaggedType *primitiveScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> primitiveStorageNodes_ {};
    int32_t currentPrimitiveStorageIndex_ {-1};
    // for recording the transition of function prototype
    FunctionProtoTransitionTable* functionProtoTransitionTable_ {nullptr};
    // opt code Profiler
    OptCodeProfiler* optCodeProfiler_ {nullptr};
    // opt code loop hoist
    TypedOpProfiler* typedOpProfiler_ {nullptr};
    // RegExpParserCache
    RegExpParserCache *regExpParserCache_ {nullptr};
    // WaiterListNode(atomics)
    WaiterListNode waiterListNode_;
    AbcBufferCache *abcBufferCache_ {nullptr};

    // for HotReload of module.
    CMap<CString, JSHandle<JSTaggedValue>> cachedPatchModules_;
    StageOfColdReload stageOfColdReload_ = StageOfColdReload::NOT_COLD_RELOAD;

    // store multi-context module manager
    std::vector<ModuleManager *> moduleManagers_ {};
};
}  // namespace ecmascript
}  // namespace panda

#endif
