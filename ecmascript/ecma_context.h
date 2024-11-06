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
#ifndef ECMASCRIPT_ECMA_CONTEXT_H
#define ECMASCRIPT_ECMA_CONTEXT_H

#include <cstdint>
#include <optional>
#include "ecmascript/base/config.h"
#include "ecmascript/common.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/patch/patch_loader.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/waiter_list.h"
#include "global_handle_collection.h"
#include "libpandafile/file.h"

namespace panda {
class JSNApi;
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class GlobalEnv;
class ObjectFactory;
class EcmaRuntimeStat;
class RegExpParserCache;
class JSPandaFileManager;
class JSPandaFile;
class ConstantPool;
class JSPromise;
class RegExpExecResultCache;
class EcmaHandleScope;
class GlobalIndexMap;
class SustainingJSHandleList;
class SustainingJSHandle;
enum class PromiseRejectionEvent : uint8_t;
enum class CompareStringsOption : uint8_t;

template<typename T>
class JSHandle;
class JSThread;
class JSFunction;
class JSPromise;
class JSTaggedValue;
class EcmaVM;
class ModuleManager;
class AOTFileManager;
class QuickFixManager;
class OptCodeProfiler;
class TypedOpProfiler;
class AbcBufferCache;
struct CJSInfo;
class FunctionProtoTransitionTable;
class ModuleLogger;

namespace job {
class MicroJobQueue;
}  // namespace job
namespace tooling {
class JsDebuggerManager;
}  // namespace tooling
namespace kungfu {
class PGOTypeManager;
} // namespace kungfu

enum class IcuFormatterType {
    SIMPLE_DATE_FORMAT_DEFAULT,
    SIMPLE_DATE_FORMAT_DATE,
    SIMPLE_DATE_FORMAT_TIME,
    NUMBER_FORMATTER,
    COLLATOR,
    ICU_FORMATTER_TYPE_COUNT
};

using HostPromiseRejectionTracker = void (*)(const EcmaVM* vm,
                                             const JSHandle<JSPromise> promise,
                                             const JSHandle<JSTaggedValue> reason,
                                             PromiseRejectionEvent operation,
                                             void* data);
using PromiseRejectCallback = void (*)(void* info);
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
using JsAotReaderCallback = std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)>;
#endif
class EcmaContext {
public:
    static EcmaContext *CreateAndInitialize(JSThread *thread);
    static void CheckAndDestroy(JSThread *thread, EcmaContext *context);

    static EcmaContext *Create(JSThread *thread);
    static bool Destroy(EcmaContext *context);

    EcmaContext(JSThread *thread);
    ~EcmaContext();

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    bool Initialize();

    bool IsExecutingPendingJob() const
    {
        return isProcessingPendingJob_.load();
    }

    bool HasPendingJob();

    bool ExecutePromisePendingJob();

    bool IsInPendingJob() const
    {
        return pendingJobEnterCount > 0;
    }

    void AddPendingJobEnterCount()
    {
        ASSERT(pendingJobEnterCount < std::numeric_limits<uint64_t>::max());
        ++pendingJobEnterCount;
    }

    void MinusPendingJobEnterCount()
    {
        ASSERT(IsInPendingJob());
        --pendingJobEnterCount;
    }

    static EcmaContext *ConstCast(const EcmaContext *context)
    {
        return const_cast<EcmaContext *>(context);
    }

    bool IsInitialized() const
    {
        return initialized_;
    }

    ModuleManager *GetModuleManager() const
    {
        return moduleManager_;
    }

    AbcBufferCache *GetAbcBufferCache() const
    {
        return abcBufferCache_;
    }

    kungfu::PGOTypeManager *GetPTManager() const
    {
        return ptManager_;
    }

    ARK_INLINE JSThread *GetJSThread() const
    {
        return thread_;
    }
    PromiseRejectCallback GetPromiseRejectCallback() const
    {
        return promiseRejectCallback_;
    }

    void SetPromiseRejectCallback(PromiseRejectCallback cb)
    {
        promiseRejectCallback_ = cb;
    }

    void SetData(void* data)
    {
        data_ = data;
    }

    void PromiseRejectionTracker(const JSHandle<JSPromise> &promise,
                                 const JSHandle<JSTaggedValue> &reason, PromiseRejectionEvent operation)
    {
        if (hostPromiseRejectionTracker_ != nullptr) {
            hostPromiseRejectionTracker_(vm_, promise, reason, operation, data_);
        }
    }

    void SetHostPromiseRejectionTracker(HostPromiseRejectionTracker cb)
    {
        hostPromiseRejectionTracker_ = cb;
    }
    void SetupRegExpResultCache();
    void SetupNumberToStringResultCache();
    void SetupStringSplitResultCache();
    void SetupStringToListResultCache();
    JSHandle<JSTaggedValue> GetRegExpCache() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&regexpCache_));
    }

    RegExpParserCache *GetRegExpParserCache() const
    {
        ASSERT(regExpParserCache_ != nullptr);
        return regExpParserCache_;
    }

    void SetRegExpCache(JSTaggedValue newCache)
    {
        regexpCache_ = newCache;
    }
    uintptr_t GetExpCacheAddress()
    {
        return reinterpret_cast<uintptr_t>(&regexpCache_);
    }

    void SetupRegExpGlobalResult();

    JSHandle<JSTaggedValue> GetRegExpGlobalResult() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&regexpGlobal_));
    }

    void SetRegExpGlobalResult(JSTaggedValue newResult)
    {
        regexpGlobal_ = newResult;
    }

    WaiterListNode *GetWaiterListNode()
    {
        return &waiterListNode_;
    }

    void SetAllowAtomicWait(bool wait)
    {
        AllowAtomicWait_ = wait;
    }

    bool GetAllowAtomicWait() const
    {
        return AllowAtomicWait_;
    }
    JSHandle<JSTaggedValue> GetNumberToStringResultCache() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&numberToStringResultCache_));
    }

    void SetNumberToStringResultCache(JSTaggedValue newCache)
    {
        numberToStringResultCache_ = newCache;
    }

    JSHandle<JSTaggedValue> GetStringSplitResultCache() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&stringSplitResultCache_));
    }

    JSHandle<JSTaggedValue> GetStringToListResultCache() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&stringToListResultCache_));
    }

    void SetStringSplitResultCache(JSTaggedValue newCache)
    {
        stringSplitResultCache_ = newCache;
    }
    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
    void EnableUserUncaughtErrorHandler();

    JSHandle<ConstantPool> AddOrUpdateConstpool(const JSPandaFile *jsPandaFile,
                                                JSHandle<ConstantPool> constpool,
                                                int32_t index = 0);
    void AddContextConstpoolCache(const JSPandaFile *jsPandaFile,
                                  JSHandle<ConstantPool> constpool,
                                  int32_t index);

    void UpdateConstpoolWhenDeserialAI(const std::string& fileName,
                                       JSHandle<ConstantPool> aiCP,
                                       int32_t index = 0);

    bool HasCachedConstpool(const JSPandaFile *jsPandaFile) const;

    void SetUnsharedConstpool(JSHandle<ConstantPool> sharedConstpool, JSTaggedValue unsharedConstpool);
    void SetUnsharedConstpool(int32_t constpoolIndex, JSTaggedValue unsharedConstpool);

    JSTaggedValue PUBLIC_API FindConstpool(const JSPandaFile *jsPandaFile, int32_t index);
    // For new version instruction.
    JSTaggedValue PUBLIC_API FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id);
    JSTaggedValue PUBLIC_API FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool);
    JSTaggedValue PUBLIC_API FindUnsharedConstpool(JSTaggedValue sharedConstpool);
    void PUBLIC_API LoadProtoTransitionTable(JSTaggedValue constpool);
    void PUBLIC_API ResetProtoTransitionTableOnConstpool(JSTaggedValue constpool);
    JSTaggedValue FindCachedConstpoolAndLoadAiIfNeeded(const JSPandaFile *jsPandaFile, int32_t index);
    void EraseUnusedConstpool(const JSPandaFile *jsPandaFile, int32_t index, int32_t constpoolIndex);
    std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> FindConstpools(
        const JSPandaFile *jsPandaFile);

    JSHandle<ConstantPool> PUBLIC_API FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
                                                            panda_file::File::EntityId id);
    void CreateAllConstpool(const JSPandaFile *jsPandaFile);

    void HandleUncaughtException(JSTaggedValue exception);
    void HandleUncaughtException();
    JSHandle<GlobalEnv> GetGlobalEnv() const;
    bool GlobalEnvIsHole()
    {
        return globalEnv_.IsHole();
    }

    JSHandle<job::MicroJobQueue> GetMicroJobQueue() const;

    static void PrintJSErrorInfo(JSThread *thread, const JSHandle<JSTaggedValue> &exceptionInfo);
    void Iterate(const RootVisitor &v, const RootRangeVisitor &rv);
    static void MountContext(JSThread *thread);
    static void UnmountContext(JSThread *thread);
    void SetMicroJobQueue(job::MicroJobQueue *queue);
    void SetGlobalEnv(GlobalEnv *global);
    void PrintOptStat();

    OptCodeProfiler *GetOptCodeProfiler() const
    {
        return optCodeProfiler_;
    }

    TypedOpProfiler *GetTypdOpProfiler() const
    {
        return typedOpProfiler_;
    }

    ModuleLogger *GetModuleLogger() const
    {
        return moduleLogger_;
    }

    void SetModuleLogger(ModuleLogger *moduleLogger)
    {
        moduleLogger_ = moduleLogger;
    }

    void SetDefaultLocale(const std::string& locale)
    {
        defaultLocale_ = locale;
    }

    const std::string& GetDefaultLocale() const
    {
        return defaultLocale_;
    }

    void InitializeDefaultLocale()
    {
        defaultLocale_ = "";
    }

    void ClearDefaultLocale()
    {
        defaultLocale_.clear();
    }

    void SetDefaultCompareStringsOption(const CompareStringsOption csOption)
    {
        defaultComapreStringsOption_ = csOption;
    }

    const std::optional<CompareStringsOption> GetDefaultCompareStringsOption() const
    {
        return defaultComapreStringsOption_;
    }

    void InitializeDefaultCompareStringsOption()
    {
        defaultComapreStringsOption_ = std::nullopt;
    }

    void ClearDefaultComapreStringsOption()
    {
        defaultComapreStringsOption_ = std::nullopt;
    }

    FunctionProtoTransitionTable *GetFunctionProtoTransitionTable() const
    {
        return functionProtoTransitionTable_;
    }

    // For icu objects cache
    void SetIcuFormatterToCache(IcuFormatterType type, const std::string &locale, void *icuObj,
                                NativePointerCallback deleteEntry = nullptr)
    {
        EcmaContext::IcuFormatter icuFormatter = IcuFormatter(locale, icuObj, deleteEntry);
        icuObjCache_[static_cast<int>(type)] = icuFormatter;
    }

    ARK_INLINE void *GetIcuFormatterFromCache(IcuFormatterType type, std::string &locale)
    {
        auto &icuFormatter = icuObjCache_[static_cast<int>(type)];
        if (icuFormatter.locale == locale) {
            return icuFormatter.icuObj;
        }
        return nullptr;
    }

    void ClearIcuCache(JSThread *thread);

    EcmaRuntimeStat *GetRuntimeStat() const
    {
        return runtimeStat_;
    }

    void SetRuntimeStatEnable(bool flag);
    void InitializeEcmaScriptRunStat();
    void DumpAOTInfo() const DUMP_API_ATTR;

    JSTaggedValue ExecuteAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp,
                             bool needPushArgv);
    void LoadStubFile();

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

#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    void HandleScopeCountAdd()
    {
        handleScopeCount_++;
    }

    void HandleScopeCountDec()
    {
        handleScopeCount_--;
    }

    void PrimitiveScopeCountAdd()
    {
        primitiveScopeCount_++;
    }

    void PrimitiveScopeCountDec()
    {
        primitiveScopeCount_--;
    }
#endif

    void SetLastHandleScope(EcmaHandleScope *scope)
    {
        lastHandleScope_ = scope;
    }

    EcmaHandleScope *GetLastHandleScope() const
    {
        return lastHandleScope_;
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

    void SetLastPrimitiveScope(EcmaHandleScope *scope)
    {
        lastPrimitiveScope_ = scope;
    }

    EcmaHandleScope *GetLastPrimitiveScope() const
    {
        return lastPrimitiveScope_;
    }

    size_t IterateHandle(const RootRangeVisitor &rangeVisitor);
    uintptr_t *ExpandHandleStorage();
    void ShrinkHandleStorage(int prevIndex);
    uintptr_t *ExpandPrimitiveStorage();
    void ShrinkPrimitiveStorage(int prevIndex);

    JSTaggedType *GetCurrentFrame() const
    {
        return currentFrame_;
    }

    JSTaggedType *GetLeaveFrame() const
    {
        return leaveFrame_;
    }

    JSTaggedType *GetLastFp() const
    {
        return lastFp_;
    }

    void SetFramePointers(JSTaggedType *currentFrame, JSTaggedType *leaveFrame, JSTaggedType *lastFp)
    {
        currentFrame_ = currentFrame;
        leaveFrame_ = leaveFrame;
        lastFp_ = lastFp;
    }
    void SetFrameBase(JSTaggedType *frameBase)
    {
        frameBase_ = frameBase;
    }
    JSTaggedType *GetFrameBase() const
    {
        return frameBase_;
    }

    void SetStackStart(uint64_t stackStart)
    {
        stackStart_ = stackStart;
    }
    uint64_t GetStackStart() const
    {
        return stackStart_;
    }
    void SetStackLimit(uint64_t stackLimit)
    {
        stackLimit_ = stackLimit;
    }
    uint64_t GetStackLimit() const
    {
        return stackLimit_;
    }

    PropertiesCache *GetPropertiesCache() const
    {
        return propertiesCache_;
    }
    void ClearBufferData();
    const GlobalEnvConstants *GlobalConstants() const
    {
        return &globalConst_;
    }

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
        GlobalHandleCollection gloalHandleCollection(thread_);
        for (auto &item : cachedPatchModules_) {
            gloalHandleCollection.Dispose(item.second);
        }
        cachedPatchModules_.clear();
    }

    StageOfHotReload GetStageOfHotReload() const
    {
        return stageOfHotReload_;
    }
    void SetStageOfHotReload(StageOfHotReload stageOfHotReload)
    {
        stageOfHotReload_ = stageOfHotReload;
    }

    StageOfColdReload GetStageOfColdReload() const
    {
        return stageOfColdReload_;
    }
    void SetStageOfColdReload(StageOfColdReload stageOfColdReload)
    {
        stageOfColdReload_ = stageOfColdReload;
    }

    bool JoinStackPushFastPath(JSHandle<JSTaggedValue> receiver);
    bool JoinStackPush(JSHandle<JSTaggedValue> receiver);
    void JoinStackPopFastPath(JSHandle<JSTaggedValue> receiver);
    void JoinStackPop(JSHandle<JSTaggedValue> receiver);

    void SetJsonStringifyCache(size_t index, CVector<std::pair<CString, int>> &value)
    {
        stringifyCache_[index] = value;
    }

    CVector<std::pair<CString, int>> GetJsonStringifyCache(size_t index)
    {
        return stringifyCache_[index];
    }

    bool IsAotEntry()
    {
        return isAotEntry_;
    }

    std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo(uintptr_t retAddr,
                                                                                        bool isDeopt) const;

    void AddSustainingJSHandle(SustainingJSHandle*);
    void RemoveSustainingJSHandle(SustainingJSHandle*);
    void ClearKeptObjects();
    void AddToKeptObjects(JSHandle<JSTaggedValue> value);
    inline bool HasKeptObjects() const
    {
        return hasKeptObjects_;
    }

    void ClearCachedConstantPool()
    {
        cachedSharedConstpools_.clear();
    }

private:
    void CJSExecution(JSHandle<JSFunction> &func, JSHandle<JSTaggedValue> &thisArg,
                      const JSPandaFile *jsPandaFile, std::string_view entryPoint);
    JSTaggedValue InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, JSHandle<JSTaggedValue> &thisArg,
                                          const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                          CJSInfo *cjsInfo = nullptr);
    Expected<JSTaggedValue, bool> InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                                       bool executeFromJob = false);
    Expected<JSTaggedValue, bool> InvokeEcmaEntrypointForHotReload(
        const JSPandaFile *jsPandaFile, std::string_view entryPoint, bool executeFromJob);
    Expected<JSTaggedValue, bool> CommonInvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
        std::string_view entryPoint, JSHandle<JSFunction> &func, bool executeFromJob);
    bool LoadAOTFilesInternal(const std::string& aotFileName);
    bool LoadAOTFiles(const std::string &aotFileName);
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    bool LoadAOTFiles(const std::string &aotFileName,
                      std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb);
#endif
    void RelocateConstantString(const JSPandaFile *jsPandaFile);
    JSTaggedValue FindConstpoolFromContextCache(const JSPandaFile *jsPandaFile, int32_t index);

    void CheckUnsharedConstpoolArrayLimit(int32_t index)
    {
        if (index >= UNSHARED_CONSTANTPOOL_COUNT) {
            LOG_ECMA(FATAL) << "the unshared constpool array need to expanding capacity, index :" << index;
            UNREACHABLE();
        }
    }

    NO_MOVE_SEMANTIC(EcmaContext);
    NO_COPY_SEMANTIC(EcmaContext);

    PropertiesCache *propertiesCache_ {nullptr};
    JSThread *thread_ {nullptr};
    EcmaVM *vm_ {nullptr};

    bool isUncaughtExceptionRegistered_ {false};
    bool initialized_ {false};
    std::atomic<bool> isProcessingPendingJob_ {false};
    ObjectFactory *factory_ {nullptr};

    // VM execution states.
    RegExpParserCache *regExpParserCache_ {nullptr};
    JSTaggedValue numberToStringResultCache_ {JSTaggedValue::Hole()};
    JSTaggedValue stringSplitResultCache_ {JSTaggedValue::Hole()};
    JSTaggedValue stringToListResultCache_ {JSTaggedValue::Hole()};
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};
    JSTaggedValue pointerToIndexDictionary_ {JSTaggedValue::Hole()};
    JSTaggedValue regexpCache_ {JSTaggedValue::Hole()};
    JSTaggedValue regexpGlobal_ {JSTaggedValue::Hole()};
    JSTaggedValue microJobQueue_ {JSTaggedValue::Hole()};
    EcmaRuntimeStat *runtimeStat_ {nullptr};

    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> cachedSharedConstpools_ {};
    std::array<JSTaggedValue, UNSHARED_CONSTANTPOOL_COUNT> unsharedConstpools_ {};
    static constexpr int32_t SHARED_CONSTPOOL_KEY_NOT_FOUND = INT32_MAX; // INT32_MAX :invalid value.

    // for HotReload of module.
    CMap<CString, JSHandle<JSTaggedValue>> cachedPatchModules_ {};
    StageOfHotReload stageOfHotReload_ = StageOfHotReload::INITIALIZE_STAGE_OF_HOTRELOAD;
    StageOfColdReload stageOfColdReload_ = StageOfColdReload::NOT_COLD_RELOAD;

    // VM resources.
    ModuleManager *moduleManager_ {nullptr};
    kungfu::PGOTypeManager *ptManager_ {nullptr};
    AOTFileManager *aotFileManager_ {nullptr};
    AbcBufferCache *abcBufferCache_ {nullptr};

    // for recording the transition of function prototype
    FunctionProtoTransitionTable *functionProtoTransitionTable_ {nullptr};

    // atomics
    bool AllowAtomicWait_ {true};
    WaiterListNode waiterListNode_;

    // Registered Callbacks
    PromiseRejectCallback promiseRejectCallback_ {nullptr};
    HostPromiseRejectionTracker hostPromiseRejectionTracker_ {nullptr};
    void* data_{nullptr};

    // opt code Profiler
    OptCodeProfiler *optCodeProfiler_ {nullptr};

    // opt code loop hoist
    TypedOpProfiler *typedOpProfiler_ {nullptr};

    ModuleLogger *moduleLogger_ {nullptr};

    std::string defaultLocale_;
    std::optional<CompareStringsOption> defaultComapreStringsOption_;

    // For icu objects cache
    struct IcuFormatter {
        std::string locale;
        void *icuObj {nullptr};
        NativePointerCallback deleteEntry {nullptr};

        IcuFormatter() = default;
        IcuFormatter(const std::string &locale, void *icuObj, NativePointerCallback deleteEntry = nullptr)
            : locale(locale), icuObj(icuObj), deleteEntry(deleteEntry) {}
    };
    IcuFormatter icuObjCache_[static_cast<uint32_t>(IcuFormatterType::ICU_FORMATTER_TYPE_COUNT)];
    // Handlescope
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;
    static constexpr int32_t MIN_HANDLE_STORAGE_SIZE = 2;
    JSTaggedType *handleScopeStorageNext_ {nullptr};
    JSTaggedType *handleScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_ {};
    int32_t currentHandleStorageIndex_ {-1};
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    int32_t handleScopeCount_ {0};
    int32_t primitiveScopeCount_ {0};
#endif
    EcmaHandleScope *lastHandleScope_ {nullptr};
    // PrimitveScope
    static constexpr int32_t MIN_PRIMITIVE_STORAGE_SIZE = 2;
    JSTaggedType *primitiveScopeStorageNext_ {nullptr};
    JSTaggedType *primitiveScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> primitiveStorageNodes_ {};
    int32_t currentPrimitiveStorageIndex_ {-1};
    EcmaHandleScope *lastPrimitiveScope_ {nullptr};

    // Frame pointer
    JSTaggedType *currentFrame_ {nullptr};
    JSTaggedType *leaveFrame_ {nullptr};
    JSTaggedType *lastFp_ {nullptr};
    JSTaggedType *frameBase_ {nullptr};
    uint64_t stackStart_ {0};
    uint64_t stackLimit_ {0};
    GlobalEnvConstants globalConst_;
    // Join Stack
    static constexpr uint32_t MIN_JOIN_STACK_SIZE = 2;
    CVector<JSTaggedValue> joinStack_ {JSTaggedValue::Hole(), JSTaggedValue::Hole()};
    // json stringify cache
    static constexpr uint32_t STRINGIFY_CACHE_SIZE = 64;
    std::array<CVector<std::pair<CString, int>>, STRINGIFY_CACHE_SIZE> stringifyCache_ {};
    bool isAotEntry_ { false };

    // SustainingJSHandleList for jit compile hold ref
    SustainingJSHandleList *sustainingJSHandleList_ {nullptr};

    bool hasKeptObjects_ {false};

    uint64_t pendingJobEnterCount {0};

    friend class EcmaHandleScope;
    friend class JSPandaFileExecutor;
    friend class ObjectFactory;
    friend class panda::JSNApi;
    friend class AOTFileManager;
    friend class GlobalIndexMap;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_ECMA_CONTEXT_H
