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
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/taskpool/task.h"

namespace panda::ecmascript {
class ProfileTypeInfo;
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

    void ProfileCall(JSTaggedType callTarget, SampleMode mode = SampleMode::CALL_MODE, int32_t incCount = 1);
    void ProfileCreateObject(JSTaggedType object, int32_t traceId);
    void ProfileDefineClass(JSTaggedType ctor);

    void SetSaveTimestamp(std::chrono::system_clock::time_point timestamp)
    {
        saveTimestamp_ = timestamp;
    }

    void PGOPreDump(JSTaggedType func);
    void PGODump(JSTaggedType func);

    void PausePGODump();
    void ResumePGODump();
    void WaitPGODumpFinish();

    void HandlePGOPreDump();
    void HandlePGODump();

    void ProcessReferences(const WeakRootVisitor &visitor);
    void Iterate(const RootVisitor &visitor);

    void UpdateTrackInfo(JSTaggedValue trackInfoVal, ElementsKind newKind);
    int32_t InsertLiteralId(JSTaggedType hclass, int32_t traceId);

private:
    static constexpr uint32_t MERGED_EVERY_COUNT = 50;
    static constexpr auto MERGED_MIN_INTERVAL = std::chrono::milliseconds(1000);
    enum class BCType : uint8_t {
        STORE,
        LOAD,
    };

    enum class State : uint8_t {
        STOP,
        PAUSE,
        START,
    };

    void ProfileByteCode(CString recordName, JSTaggedValue value);

    void DumpICByName(const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
        ProfileTypeInfo *profileTypeInfo, BCType type);
    void DumpICByValue(const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
        ProfileTypeInfo *profileTypeInfo, BCType type);

    void DumpICByNameWithPoly(
        const CString &recordName, EntityId methodId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type);
    void DumpICByValueWithPoly(
        const CString &recordName, EntityId methodId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type);

    void DumpICByNameWithHandler(const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *hclass,
        JSTaggedValue secondValue, BCType type);
    void DumpICByValueWithHandler(const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *hclass,
        JSTaggedValue secondValue, BCType type);

    void DumpOpType(const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
        ProfileTypeInfo *profileTypeInfo);
    void DumpDefineClass(const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
        ProfileTypeInfo *profileTypeInfo);
    void DumpCreateObject(const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
        ProfileTypeInfo *profileTypeInfo, int32_t traceId);
    void DumpCall(const CString &recordName, EntityId methodId, int32_t bcOffset, uint32_t slotId,
        ProfileTypeInfo *profileTypeInfo);

    void AddObjectInfo(
        const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *hclass, PGOObjKind kind);
    bool AddObjectInfoByTraceId(const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *hclass,
        ProfileType::Kind classKind, PGOObjKind kind);

    JSTaggedValue PopFromProfileQueue();

    class PGOProfilerTask : public Task {
    public:
        explicit PGOProfilerTask(PGOProfiler *profiler, int32_t id) : Task(id), profiler_(profiler) {};
        virtual ~PGOProfilerTask() override = default;

        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            profiler_->HandlePGODump();
            return true;
        }

        NO_COPY_SEMANTIC(PGOProfilerTask);
        NO_MOVE_SEMANTIC(PGOProfilerTask);
    private:
        PGOProfiler *profiler_;
    };

    PGOProfiler(EcmaVM *vm, bool isEnable);

    virtual ~PGOProfiler();

    void Reset(bool isEnable);

    EcmaVM *vm_ { nullptr };
    bool isEnable_ { false };
    State state_ { State::STOP };
    uint32_t methodCount_ { 0 };
    std::chrono::system_clock::time_point saveTimestamp_;
    os::memory::Mutex mutex_;
    os::memory::ConditionVariable condition_;
    CList<JSTaggedType> dumpWorkList_;
    CSet<JSTaggedType> preDumpWorkList_;
    CMap<JSTaggedType, int32_t> traceIds_;
    std::unique_ptr<PGORecordDetailInfos> recordInfos_;
    friend class PGOProfilerManager;
};
} // namespace panda::ecmascript
} // namespace pgo
#endif // ECMASCRIPT_PGO_PROFILER_H
