/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_PROFILER_H
#define ECMASCRIPT_PGO_PROFILER_H

#include <chrono>
#include <memory>

#include "ecmascript/common.h"
#include "ecmascript/elements.h"
#include "ecmascript/global_index.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/pgo_profiler/types/pgo_type_generator.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/taskpool/task.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/pgo_profiler/pgo_extra_profiler.h"

namespace panda::ecmascript {
class ProfileTypeInfo;
class JSFunction;
class GlobalIndex;
class JITProfiler;
namespace pgo {
class PGORecordDetailInfos;

enum class SampleMode : uint8_t {
    HOTNESS_MODE,
    CALL_MODE,
};

class PGOProfiler {
public:
    NO_COPY_SEMANTIC(PGOProfiler);
    NO_MOVE_SEMANTIC(PGOProfiler);

    PGOProfiler(EcmaVM *vm, bool isEnable);

    virtual ~PGOProfiler();

    void PUBLIC_API RecordProfileType(JSHClass *hclass, JSPandaFile *pandaFile, int32_t traceId);

    static ProfileType CreateRecordProfileType(ApEntityId abcId, ApEntityId classId);
    void ProfileDefineClass(JSTaggedType ctor);
    void ProfileProtoTransitionClass(JSHandle<JSFunction> func,
                                     JSHandle<JSHClass> hclass,
                                     JSHandle<JSTaggedValue> proto);
    void ProfileProtoTransitionPrototype(JSHandle<JSFunction> func,
                                         JSHandle<JSTaggedValue> prototype,
                                         JSHandle<JSTaggedValue> oldPrototype,
                                         JSHandle<JSTaggedValue> baseIhc);
    void ProfileDefineGetterSetter(JSHClass *receverHClass,
                                   JSHClass *holderHClass,
                                   const JSHandle<JSTaggedValue> &func,
                                   int32_t pcOffset);
    void ProfileClassRootHClass(JSTaggedType ctor, JSTaggedType rootHcValue,
                                ProfileType::Kind kind = ProfileType::Kind::ClassId);
    void UpdateRootProfileTypeSafe(JSHClass* oldHClass, JSHClass* newHClass);

    void InitJITProfiler();
    void SetSaveTimestamp(std::chrono::system_clock::time_point timestamp)
    {
        saveTimestamp_ = timestamp;
    }
    JITProfiler *GetJITProfile()
    {
        return jitProfiler_;
    }
    void PGOPreDump(JSTaggedType func);
    void PGODump(JSTaggedType func);

    void SuspendByGC();
    void ResumeByGC();
    void WaitPGODumpFinish();

    void HandlePGOPreDump();
    void HandlePGODumpByDumpThread(bool force);

    void ProcessReferences(const WeakRootVisitor &visitor);
    void Iterate(const RootVisitor &visitor);

    void UpdateTrackArrayLength(JSTaggedValue trackInfoVal, uint32_t newSize);
    void UpdateTrackSpaceFlag(TaggedObject *object, RegionSpaceFlag spaceFlag);
    void UpdateTrackElementsKind(JSTaggedValue trackInfoVal, ElementsKind newKind);
    void UpdateTrackInfo(JSTaggedValue trackInfoVal);

    JSTaggedValue TryFindKeyInPrototypeChain(TaggedObject *currObj, JSHClass *currHC, JSTaggedValue key);

    void InsertSkipCtorMethodIdSafe(EntityId ctorMethodId)
    {
        LockHolder lock(skipCtorMethodIdMutex_);
        skipCtorMethodId_.insert(ctorMethodId.GetOffset());
    }

private:
    static constexpr uint32_t MERGED_EVERY_COUNT = 50;
    static constexpr uint32_t MS_PRE_SECOND = 1000;
    enum class BCType : uint8_t {
        STORE,
        LOAD,
    };

    enum class State : uint8_t {
        MERGE,
        STOP,
        PAUSE,
        START,
        FORCE_SAVE,
        FORCE_SAVE_PAUSE,
    };
    static std::string StateToString(State state)
    {
        switch (state) {
            case State::STOP:
                return "STOP";
            case State::MERGE:
                return "MERGE";
            case State::PAUSE:
                return "PAUSE";
            case State::START:
                return "START";
            case State::FORCE_SAVE:
                return "FORCE SAVE";
            case State::FORCE_SAVE_PAUSE:
                return "FORCE SAVE PAUSE";
            default:
                return "UNKNOWN";
        }
    }
    State GetState() const;
    void SetState(State state);
    void NotifyGC(std::string tag = "");
    void NotifyAll(std::string tag = "");
    void WaitingPGODump();
    void StopPGODump();
    void StartPGODump();
    bool IsGCWaitingWithLock();
    bool IsGCWaiting();
    void DispatchPGODumpTask();

    void ProfileBytecode(ApEntityId abcId, const CString& recordName, JSTaggedValue funcValue);
    void DumpICByName(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
                      ProfileTypeInfo *profileTypeInfo, BCType type);
    void DumpICByValue(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                       uint32_t slotId, ProfileTypeInfo *profileTypeInfo, BCType type);

    void DumpICByNameWithPoly(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                              JSTaggedValue cacheValue, BCType type);
    void DumpICByValueWithPoly(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                               JSTaggedValue cacheValue, BCType type);

    bool DumpICByNameWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                 JSHClass *hclass, JSTaggedValue secondValue, BCType type);
    bool DumpICLoadByNameWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                     JSHClass *hclass, JSTaggedValue secondValue);
    void DumpICByValueWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                  JSHClass *hclass, JSTaggedValue secondValue, BCType type);

    void TryDumpProtoTransitionType(JSHClass *hclass);

    void DumpByForce();

    void DumpOpType(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
                    ProfileTypeInfo *profileTypeInfo);
    void DumpDefineClass(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                         uint32_t slotId, ProfileTypeInfo *profileTypeInfo);
    bool FunctionKindVerify(const JSFunction *ctorFunction);
    void DumpCreateObject(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                          uint32_t slotId, ProfileTypeInfo *profileTypeInfo, int32_t traceId);
    void DumpCall(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
                  ProfileTypeInfo *profileTypeInfo);
    void DumpNewObjRange(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                         uint32_t slotId, ProfileTypeInfo *profileTypeInfo);
    void DumpGetIterator(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                         uint32_t slotId, ProfileTypeInfo *profileTypeInfo);
    void DumpInstanceof(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                         uint32_t slotId, ProfileTypeInfo *profileTypeInfo);

    void UpdateLayout(JSHClass *hclass);
    void UpdateTransitionLayout(JSHClass* parent, JSHClass* child);
    bool AddTransitionObjectInfo(ProfileType recordType,
                                 EntityId methodId,
                                 int32_t bcOffset,
                                 JSHClass* receiver,
                                 JSHClass* hold,
                                 JSHClass* holdTra,
                                 PGOSampleType accessorMethod);
    void UpdatePrototypeChainInfo(JSHClass *receiver, JSHClass *holder, PGOObjectInfo &info);

    bool AddObjectInfo(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                       JSHClass *receiver, JSHClass *hold, JSHClass *holdTra, uint32_t accessorMethodId = 0);
    void AddObjectInfoWithMega(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset);
    bool AddBuiltinsInfoByNameInInstance(ApEntityId abcId, const CString &recordName, EntityId methodId,
        int32_t bcOffset, JSHClass *receiver);
    bool AddBuiltinsInfoByNameInProt(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
        JSHClass *receiver, JSHClass *hold);
    bool AddBuiltinsInfo(
        ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *receiver,
        JSHClass *transitionHClass, OnHeapMode onHeap = OnHeapMode::NONE, bool everOutOfBounds = false);
    void AddBuiltinsGlobalInfo(ApEntityId abcId, const CString &recordName, EntityId methodId,
                               int32_t bcOffset, GlobalIndex globalId);

    void SetRootProfileType(JSHClass *root, ApEntityId abcId, uint32_t type, ProfileType::Kind kind);
    ProfileType FindRootProfileType(JSHClass *hclass);

    ProfileType GetOrInsertProfileType(JSHClass *child, ProfileType rootType);
    ProfileType GetProfileType(JSHClass *hclass, bool check = false);

    bool IsRecoredTransRootType(ProfileType type);
    bool HasValidExtraProfileTypeInfo(JSFunction *func);
    class WorkNode;
    void ProcessExtraProfileTypeInfo(JSFunction *func, ApEntityId abcId, const CString &recordName,
                                JSTaggedValue methodValue, WorkNode *current);
    void UpdateExtraProfileTypeInfo(ApEntityId abcId, const CString &recordName, EntityId methodId, WorkNode* current);
    WorkNode* PopFromProfileQueue();
    void MergeProfilerAndDispatchAsyncSaveTask(bool force);
    bool IsJSHClassNotEqual(JSHClass *receiver, JSHClass *hold, JSHClass *exceptRecvHClass,
                            JSHClass *exceptRecvHClassOnHeap, JSHClass *exceptHoldHClass,
                            JSHClass *exceptPrototypeOfPrototypeHClass);
    bool CheckProtoChangeMarker(JSTaggedValue cellValue) const;

    class PGOProfilerTask : public Task {
    public:
        explicit PGOProfilerTask(PGOProfiler *profiler, int32_t id)
            : Task(id), profiler_(profiler){};
        virtual ~PGOProfilerTask() override = default;

        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfilerTask::Run");
            profiler_->HandlePGODumpByDumpThread(profiler_->isForce_);
            profiler_->StopPGODump();
            return true;
        }

        NO_COPY_SEMANTIC(PGOProfilerTask);
        NO_MOVE_SEMANTIC(PGOProfilerTask);
    private:
        PGOProfiler *profiler_;
    };

    using PcOffset = int32_t;

    class WorkList;
    class WorkNode {
    public:
        WorkNode(JSTaggedType value) : value_(value) {}
        void SetPrev(WorkNode *prev)
        {
            prev_ = prev;
        }

        void SetNext(WorkNode *next)
        {
            next_ = next;
        }

        void SetValue(JSTaggedType value)
        {
            value_ = value;
        }

        void SetWorkList(WorkList *workList)
        {
            workList_ = workList;
        }

        WorkNode *GetPrev() const
        {
            return prev_;
        }

        WorkNode *GetNext() const
        {
            return next_;
        }

        JSTaggedType GetValue() const
        {
            return value_;
        }

        uintptr_t GetValueAddr() const
        {
            return reinterpret_cast<uintptr_t>(&value_);
        }

        WorkList *GetWorkList() const
        {
            return workList_;
        }

    private:
        WorkList *workList_ { nullptr };
        WorkNode *prev_ { nullptr };
        WorkNode *next_ { nullptr };
        JSTaggedType value_ { JSTaggedValue::Undefined().GetRawData() };
    };

    class WorkList {
    public:
        using Callback = std::function<void(WorkNode *node)>;
        bool IsEmpty() const
        {
            return first_ == nullptr;
        }
        void PushBack(WorkNode *node);
        WorkNode *PopFront();
        void Remove(WorkNode *node);
        void Iterate(Callback callback) const;
    private:
        WorkNode *first_ { nullptr };
        WorkNode *last_ { nullptr };
    };
public:
    static ApEntityId PUBLIC_API GetMethodAbcId(JSFunction *jsFunction);
    static ApEntityId PUBLIC_API GetMethodAbcId(JSTaggedValue jsMethod);
    void Reset(bool isEnable);
private:
    ProfileType GetRecordProfileType(JSFunction *jsFunction, const CString &recordName);
    ProfileType GetRecordProfileType(ApEntityId abcId, const CString &recordName);
    ProfileType GetRecordProfileType(const std::shared_ptr<JSPandaFile> &pf, ApEntityId abcId,
                                     const CString &recordName);

    bool IsSkippableObjectTypeSafe(ProfileType type)
    {
        if (type.IsGeneralizedClassType() || type.IsConstructor() || type.IsGeneralizedPrototype()) {
            uint32_t ctorId = type.GetId();
            LockHolder lock(skipCtorMethodIdMutex_);
            return skipCtorMethodId_.find(ctorId) != skipCtorMethodId_.end();
        }
        return false;
    }

    bool IsSkippableCtor(uint32_t entityId)
    {
        return entityId == 0 || skipCtorMethodId_.find(entityId) != skipCtorMethodId_.end();
    }

    bool InsertDefinedCtor(uint32_t entityId)
    {
        if (definedCtorMethodId_.find(entityId) == definedCtorMethodId_.end()) {
            definedCtorMethodId_.insert(entityId);
            return true;
        }
        return false;
    }

    ConcurrentGuardValues v_;
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    EcmaVM *vm_ { nullptr };
    bool isEnable_ { false };
    bool isForce_ {false};
    std::atomic<State> state_ {State::STOP};
    uint32_t methodCount_ { 0 };
    std::chrono::system_clock::time_point saveTimestamp_;
    Mutex mutex_;
    Mutex recordInfoMutex_;
    ConditionVariable condition_;
    WorkList dumpWorkList_;
    WorkList preDumpWorkList_;
    std::unique_ptr<PGORecordDetailInfos> recordInfos_;
    // AOT only supports executing Defineclass bc once currently.
    // If defineclass executed multiple times, It will gives up collection.
    CUnorderedSet<uint32_t> definedCtorMethodId_;
    CUnorderedSet<uint32_t> skipCtorMethodId_;
    Mutex skipCtorMethodIdMutex_;
    JITProfiler *jitProfiler_ {nullptr};
    CVector<ProfileType> recordedTransRootType_;
    friend class PGOProfilerManager;
};

class PGODumpPauseScope {
public:
    explicit PGODumpPauseScope(std::shared_ptr<PGOProfiler> profiler): profiler_(profiler)
    {
        profiler_->SuspendByGC();
    }

    ~PGODumpPauseScope()
    {
        profiler_->ResumeByGC();
    }

    NO_COPY_SEMANTIC(PGODumpPauseScope);
    NO_MOVE_SEMANTIC(PGODumpPauseScope);

private:
    std::shared_ptr<PGOProfiler> profiler_;
};
} // namespace pgo
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_H
