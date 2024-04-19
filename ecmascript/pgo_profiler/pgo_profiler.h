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

    static ProfileType CreateRecordProfileType(ApEntityId abcId, ApEntityId classId);
    void PUBLIC_API ProfileCreateObject(JSTaggedType object, ApEntityId abcId, int32_t traceId);
    void ProfileDefineClass(JSTaggedType ctor);
    void ProfileDefineGetterSetter(
        JSHClass *receverHClass, JSHClass *holderHClass, const JSHandle<JSTaggedValue> &func, int32_t pcOffset);
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

    void WaitPGODumpPause();
    void WaitPGODumpResume();
    void WaitPGODumpFinish();

    void HandlePGOPreDump();
    void HandlePGODump(bool force);

    void ProcessReferences(const WeakRootVisitor &visitor);
    void Iterate(const RootVisitor &visitor);

    void UpdateTrackArrayLength(JSTaggedValue trackInfoVal, uint32_t newSize);
    void UpdateTrackSpaceFlag(TaggedObject *object, RegionSpaceFlag spaceFlag);
    void UpdateTrackElementsKind(JSTaggedValue trackInfoVal, ElementsKind newKind);
    void UpdateTrackInfo(JSTaggedValue trackInfoVal);

    JSTaggedValue TryFindKeyInPrototypeChain(TaggedObject *currObj, JSHClass *currHC, JSTaggedValue key);

    void InsertSkipCtorMethodId(EntityId ctorMethodId)
    {
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
        STOP,
        PAUSE,
        START,
        FORCE_SAVE,
        FORCE_SAVE_PAUSE,
    };

    void ProfileBytecode(ApEntityId abcId, const CString &recordName, JSTaggedValue funcValue);
    bool PausePGODump();

    void DumpICByName(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
                      ProfileTypeInfo *profileTypeInfo, BCType type);
    void DumpICByValue(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                       uint32_t slotId, ProfileTypeInfo *profileTypeInfo, BCType type);

    void DumpICByNameWithPoly(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                              JSTaggedValue cacheValue, BCType type);
    void DumpICByValueWithPoly(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                               JSTaggedValue cacheValue, BCType type);

    void DumpICByNameWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                 JSHClass *hclass, JSTaggedValue secondValue, BCType type);
    void DumpICByValueWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                  JSHClass *hclass, JSTaggedValue secondValue, BCType type);
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
    void UpdateTranstionLayout(JSHClass *parent, JSHClass *child);
    bool AddTranstionObjectInfo(ProfileType recordType, EntityId methodId, int32_t bcOffset, JSHClass *receiver,
        JSHClass *hold, JSHClass *holdTra, PGOSampleType accessorMethod);
    void UpdatePrototypeChainInfo(JSHClass *receiver, JSHClass *holder, PGOObjectInfo &info);

    bool AddObjectInfo(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                       JSHClass *receiver, JSHClass *hold, JSHClass *holdTra, uint32_t accessorMethodId = 0);
    void AddObjectInfoWithMega(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset);
    void AddBuiltinsInfoByNameInInstance(ApEntityId abcId, const CString &recordName, EntityId methodId,
        int32_t bcOffset, JSHClass *receiver);
    void AddBuiltinsInfoByNameInProt(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
        JSHClass *receiver, JSHClass *hold);
    void AddBuiltinsInfo(
        ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *receiver,
        JSHClass *transitionHClass, OnHeapMode onHeap = OnHeapMode::NONE);
    void AddBuiltinsGlobalInfo(ApEntityId abcId, const CString &recordName, EntityId methodId,
                               int32_t bcOffset, GlobalIndex globalId);

    ProfileType GetProfileType(JSTaggedType root, JSTaggedType child);
    ProfileType GetProfileTypeSafe(JSTaggedType root, JSTaggedType child);
    ProfileType GetOrInsertProfileTypeSafe(JSTaggedType root, JSTaggedType child);
    bool InsertProfileTypeSafe(JSTaggedType root, JSTaggedType child, ProfileType traceType);

    class WorkNode;
    void UpdateExtraProfileTypeInfo(ApEntityId abcId, const CString& recordName, EntityId methodId, WorkNode* current);
    WorkNode* PopFromProfileQueue();
    void SaveProfiler(bool force);
    bool IsJSHClassNotEqual(JSHClass *receiver, JSHClass *hold, JSHClass *exceptRecvHClass,
                            JSHClass *exceptRecvHClassOnHeap, JSHClass *exceptHoldHClass,
                            JSHClass *exceptPrototypeOfPrototypeHClass);

    class PGOProfilerTask : public Task {
    public:
        explicit PGOProfilerTask(PGOProfiler *profiler, int32_t id)
            : Task(id), profiler_(profiler){};
        virtual ~PGOProfilerTask() override = default;

        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            profiler_->HandlePGODump(profiler_->isForce_);
            return true;
        }

        NO_COPY_SEMANTIC(PGOProfilerTask);
        NO_MOVE_SEMANTIC(PGOProfilerTask);
    private:
        PGOProfiler *profiler_;
    };

    using PcOffset = int32_t;
    class ExtraProfileTypeInfo {
    public:
        uintptr_t GetReceiverAddr() const
        {
            return reinterpret_cast<uintptr_t>(&receiver);
        }

        uintptr_t GetHolderAddr() const
        {
            return reinterpret_cast<uintptr_t>(&holder);
        }

        JSHClass* GetReceiverHClass() const
        {
            return receiver.GetTaggedObject()->GetClass();
        }

        JSHClass* GetHolderHClass() const
        {
            return holder.GetTaggedObject()->GetClass();
        }

        void SetReceiver(JSHClass* hclass)
        {
            receiver = JSTaggedValue::Cast(hclass);
        }

        void SetHolder(JSHClass* hclass)
        {
            holder = JSTaggedValue::Cast(hclass);
        }

        void SetReceiver(JSTaggedValue value)
        {
            receiver = value;
        }

        void SetHolder(JSTaggedValue value)
        {
            holder = value;
        }

        void SetReceiver(TaggedObject* value)
        {
            receiver = JSTaggedValue::Cast(value);
        }

        void SetHolder(TaggedObject* value)
        {
            holder = JSTaggedValue::Cast(value);
        }

        JSTaggedValue GetReceiver() const
        {
            return receiver;
        }

        JSTaggedValue GetHolder() const
        {
            return holder;
        }

        void Clear()
        {
            receiver = JSTaggedValue::Hole();
            holder = JSTaggedValue::Hole();
        }

        void ClearReceiver()
        {
            receiver = JSTaggedValue::Hole();
        }

        void ClearHolder()
        {
            holder = JSTaggedValue::Hole();
        }

        bool operator==(const ExtraProfileTypeInfo& other) const
        {
            return receiver == other.receiver && holder == other.holder;
        }

        bool IsHole() const
        {
            return receiver.IsHole() && holder.IsHole();
        }

        void ProcessReceiver(const WeakRootVisitor& visitor)
        {
            if (!receiver.IsHeapObject()) {
                return;
            }
            auto obj = receiver.GetTaggedObject();
            auto fwd = visitor(obj);
            if (fwd == nullptr) {
                ClearReceiver();
            } else if (fwd != obj) {
                SetReceiver(fwd);
            }
        }

        void ProcessHolder(const WeakRootVisitor& visitor)
        {
            if (!holder.IsHeapObject()) {
                return;
            }
            auto obj = holder.GetTaggedObject();
            auto fwd = visitor(obj);
            if (fwd == nullptr) {
                ClearHolder();
            } else if (fwd != obj) {
                SetHolder(fwd);
            }
        }

    private:
        JSTaggedValue receiver {JSTaggedValue::Hole()};
        JSTaggedValue holder {JSTaggedValue::Hole()};
    };

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

        void SetExtraProfileTypeInfo(int32_t pcOffset, JSHClass* receiverHClass, JSHClass* holderHClass)
        {
            auto it = map_.find(pcOffset);

            ExtraProfileTypeInfo info;
            info.SetReceiver(receiverHClass);
            info.SetHolder(holderHClass);

            if (it == map_.end()) {
                map_.insert(std::make_pair(pcOffset, info));
                return;
            }

            if (it->second == info) {
                return;
            }

            it->second.Clear();
        }

        bool HasExtraProfileTypeInfo()
        {
            return !map_.empty();
        }

        void ProcessExtraProfileTypeInfo(const WeakRootVisitor& visitor)
        {
            for (auto& iter: map_) {
                auto& info = iter.second;
                info.ProcessReceiver(visitor);
                info.ProcessHolder(visitor);
            }
        }

        void IterateExtraProfileTypeInfo(const RootVisitor& visitor)
        {
            if (HasExtraProfileTypeInfo()) {
                for (auto& iter: map_) {
                    auto& info = iter.second;
                    visitor(Root::ROOT_VM, ObjectSlot(info.GetReceiverAddr()));
                    visitor(Root::ROOT_VM, ObjectSlot(info.GetHolderAddr()));
                }
            }
        }

        std::unordered_map<PcOffset, ExtraProfileTypeInfo>& GetExtraProfileTypeInfo()
        {
            return map_;
        }

    private:
        WorkList *workList_ { nullptr };
        WorkNode *prev_ { nullptr };
        WorkNode *next_ { nullptr };
        JSTaggedType value_ { JSTaggedValue::Undefined().GetRawData() };
        std::unordered_map<PcOffset, ExtraProfileTypeInfo> map_;
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
private:
    ProfileType GetRecordProfileType(JSFunction *jsFunction, const CString &recordName);
    ProfileType GetRecordProfileType(ApEntityId abcId, const CString &recordName);
    ProfileType GetRecordProfileType(const std::shared_ptr<JSPandaFile> &pf, ApEntityId abcId,
                                     const CString &recordName);

    void Reset(bool isEnable);

    bool IsSkippableObjectType(ProfileType type)
    {
        if (type.IsClassType() || type.IsConstructor() || type.IsPrototype()) {
            uint32_t ctorId = type.GetId();
            return skipCtorMethodId_.find(ctorId) != skipCtorMethodId_.end();
        }
        return false;
    }

    EcmaVM *vm_ { nullptr };
    bool isEnable_ { false };
    bool isForce_ {false};
    State state_ { State::STOP };
    uint32_t methodCount_ { 0 };
    std::chrono::system_clock::time_point saveTimestamp_;
    Mutex mutex_;
    ConditionVariable condition_;
    WorkList dumpWorkList_;
    WorkList preDumpWorkList_;
    Mutex tracedProfilesMutex_;
    CMap<JSTaggedType, PGOTypeGenerator *> tracedProfiles_;
    std::unique_ptr<PGORecordDetailInfos> recordInfos_;
    CUnorderedSet<uint32_t> skipCtorMethodId_;
    JITProfiler *jitProfiler_ {nullptr};
    friend class PGOProfilerManager;
};
} // namespace pgo
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_H
