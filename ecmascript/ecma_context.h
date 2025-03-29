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
#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/base/config.h"
#include "ecmascript/common.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/frames.h"
#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/module/js_module_execute_type.h"
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
class AotConstantpoolPatcher;
class GlobalEnv;
class ObjectFactory;
class EcmaRuntimeStat;
class RegExpParserCache;
class JSPandaFileManager;
class JSPandaFile;
class ConstantPool;
class RegExpExecResultCache;
class SustainingJSHandleList;
class SustainingJSHandle;
enum class PromiseRejectionEvent : uint8_t;

template<typename T>
class JSHandle;
class JSThread;
class JSFunction;
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

namespace tooling {
class JsDebuggerManager;
}  // namespace tooling
namespace kungfu {
class PGOTypeManager;
} // namespace kungfu

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
    struct EcmaData
        : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                     base::AlignedPointer,
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
                                     base::AlignedPointer,
                                     base::AlignedUint64,
                                     base::AlignedUint64,
                                     base::AlignedUint64>
#else
                                     base::AlignedPointer>
#endif
                                     {
        enum class Index : size_t {
            LoadMegaICCacheIndex,
            StoreMegaICCacheIndex,
            PropertiesCacheIndex,
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
            megaUpdateCountIndex,
            megaProbesCountIndex,
            megaHitCountIndex,
#endif
            NumOfMembers
        };
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
        static_assert(static_cast<size_t>(Index::NumOfMembers) == 6);
#else
        static_assert(static_cast<size_t>(Index::NumOfMembers) == 3);
#endif
        static size_t GetLoadMegaICCacheOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::LoadMegaICCacheIndex)>(
                isArch32);
        }

        static size_t GetStoreMegaICCacheOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StoreMegaICCacheIndex)>(
                isArch32);
        }

        static size_t GetPropertiesCacheOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::PropertiesCacheIndex)>(
                isArch32);
        }
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
        static size_t GetMegaProbesCountOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::megaProbesCountIndex)>(
                isArch32);
        }

        static size_t GetMegaHitCountOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::megaHitCountIndex)>(
                isArch32);
        }
#endif
        alignas(EAS) MegaICCache *loadMegaICCache_{nullptr};
        alignas(EAS) MegaICCache *storeMegaICCache_{nullptr};
        alignas(EAS) PropertiesCache *propertiesCache_{nullptr};
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
        alignas(EAS) uint64_t megaUpdateCount_ {0};
        alignas(EAS) uint64_t megaProbesCount_ {0};
        alignas(EAS) uint64_t megaHitCount {0};
#endif
    };

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    bool Initialize();

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

    RegExpParserCache *GetRegExpParserCache() const
    {
        ASSERT(regExpParserCache_ != nullptr);
        return regExpParserCache_;
    }

    WaiterListNode *GetWaiterListNode()
    {
        return &waiterListNode_;
    }

    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;

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
    void SetPrototypeForTransitions(JSTaggedValue trans, JSTaggedValue proto);
    void SetObjectFunctionFromConstPool(JSHandle<ConstantPool> newConstPool);
    void CreateAllConstpool(const JSPandaFile *jsPandaFile);

    JSHandle<GlobalEnv> GetGlobalEnv() const;
    bool GlobalEnvIsHole()
    {
        return globalEnv_.IsHole();
    }

    void IterateMegaIC(RootVisitor &v);
    void Iterate(RootVisitor &v);
    static void MountContext(JSThread *thread);
    static void UnmountContext(JSThread *thread);
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

    FunctionProtoTransitionTable *GetFunctionProtoTransitionTable() const
    {
        return functionProtoTransitionTable_;
    }

    void DumpAOTInfo() const DUMP_API_ATTR;

    JSTaggedValue ExecuteAot(size_t actualNumArgs, JSTaggedType *args, const JSTaggedType *prevFp,
                             bool needPushArgv);
    void LoadStubFile();

    PropertiesCache *GetPropertiesCache() const
    {
        return ecmaData_.propertiesCache_;
    }

    MegaICCache *GetLoadMegaICCache() const
    {
        return ecmaData_.loadMegaICCache_;
    }

    MegaICCache *GetStoreMegaICCache() const
    {
        return ecmaData_.storeMegaICCache_;
    }
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
    uint64_t GetMegaProbeCount() const
    {
        return ecmaData_.megaProbesCount_;
    }

    uint64_t GetMegaHitCount() const
    {
        return ecmaData_.megaHitCount;
    }

    uint64_t GetMegaUpdateCount() const
    {
        return ecmaData_.megaUpdateCount_;
    }

    void IncMegaUpdateCount()
    {
        ecmaData_.megaUpdateCount_++;
    }

    void ClearMegaStat()
    {
        ecmaData_.megaHitCount = 0;
        ecmaData_.megaProbesCount_ = 0;
        ecmaData_.megaUpdateCount_ = 0;
    }
    void PrintMegaICStat()
    {
        const int precision = 2;
        const double percent = 100.0;
        LOG_ECMA(INFO)
            << "------------------------------------------------------------"
            << "---------------------------------------------------------";
        LOG_ECMA(INFO) << "MegaUpdateCount: " << GetMegaUpdateCount();
        LOG_ECMA(INFO) << "MegaHitCount: " << GetMegaHitCount();
        LOG_ECMA(INFO) << "MegaProbeCount: " << GetMegaProbeCount();
        LOG_ECMA(INFO) << "MegaHitRate: " << std::fixed
                       << std::setprecision(precision)
                       << (GetMegaProbeCount() > 0
                               ? static_cast<double>(GetMegaHitCount()) /
                                     GetMegaProbeCount() * percent
                               : 0.0)
                       << "%";
        LOG_ECMA(INFO)
            << "------------------------------------------------------------"
            << "---------------------------------------------------------";
        ClearMegaStat();
    }
#endif
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

    std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo(uintptr_t retAddr,
                                                                                        bool isDeopt) const;

    void AddSustainingJSHandle(SustainingJSHandle*);
    void RemoveSustainingJSHandle(SustainingJSHandle*);
    void ClearKeptObjects();
    void AddToKeptObjects(JSHandle<JSTaggedValue> value);

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
                                                       const ExecuteTypes &executeType = ExecuteTypes::STATIC);
    Expected<JSTaggedValue, bool> InvokeEcmaEntrypointForHotReload(
        const JSPandaFile *jsPandaFile, std::string_view entryPoint, const ExecuteTypes &executeType);
    Expected<JSTaggedValue, bool> CommonInvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
        std::string_view entryPoint, JSHandle<JSFunction> &func, const ExecuteTypes &executeType);
    bool LoadAOTFilesInternal(const std::string& aotFileName);
    bool LoadAOTFiles(const std::string &aotFileName);
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    bool LoadAOTFiles(const std::string &aotFileName,
                      std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb);
#endif
    JSTaggedValue FindConstpoolFromContextCache(const JSPandaFile *jsPandaFile, int32_t index);

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

    NO_MOVE_SEMANTIC(EcmaContext);
    NO_COPY_SEMANTIC(EcmaContext);
    EcmaData ecmaData_;
    JSThread *thread_{nullptr};
    EcmaVM *vm_{nullptr};

    bool initialized_ {false};
    ObjectFactory *factory_ {nullptr};

    // VM execution states.
    RegExpParserCache *regExpParserCache_ {nullptr};
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};

    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> cachedSharedConstpools_ {};
    JSTaggedValue* unsharedConstpools_ = nullptr;
    int32_t unsharedConstpoolsArrayLen_ = UNSHARED_CONSTANTPOOL_COUNT;
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
    WaiterListNode waiterListNode_;

    // opt code Profiler
    OptCodeProfiler *optCodeProfiler_ {nullptr};

    // opt code loop hoist
    TypedOpProfiler *typedOpProfiler_ {nullptr};

    ModuleLogger *moduleLogger_ {nullptr};

    GlobalEnvConstants globalConst_;

    // SustainingJSHandleList for jit compile hold ref
    SustainingJSHandleList *sustainingJSHandleList_ {nullptr};

    friend class JSPandaFileExecutor;
    friend class ObjectFactory;
    friend class panda::JSNApi;
    friend class AOTFileManager;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_ECMA_CONTEXT_H
