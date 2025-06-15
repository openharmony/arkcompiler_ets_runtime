/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_HPROF_HEAP_PROFILER_H
#define ECMASCRIPT_DFX_HPROF_HEAP_PROFILER_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/dfx/hprof/heap_sampling.h"
#include "ecmascript/dfx/hprof/progress.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "ecmascript/dfx/hprof/heap_marker.h"
#include "ecmascript/mem/c_containers.h"
#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
#include "ecmascript/mem/clock_scope.h"
#endif

namespace panda::ecmascript {
class HeapSnapshot;
class EcmaVM;
using NodeId = uint64_t;

class EntryIdMap {
public:
    EntryIdMap() = default;
    ~EntryIdMap() = default;
    NO_COPY_SEMANTIC(EntryIdMap);
    NO_MOVE_SEMANTIC(EntryIdMap);

    static constexpr uint64_t SEQ_STEP = 2;
    std::pair<bool, NodeId> FindId(JSTaggedType addr);
    NodeId FindOrInsertNodeId(JSTaggedType addr);
    bool InsertId(JSTaggedType addr, NodeId id);
    bool EraseId(JSTaggedType addr);
    bool Move(JSTaggedType oldAddr, JSTaggedType forwardAddr);
    void UpdateEntryIdMap(HeapSnapshot *snapshot);
    void RemoveUnmarkedObjects(HeapMarker &marker);
    NodeId GetNextId()
    {
        nextId_ += SEQ_STEP;
        return nextId_ - SEQ_STEP;
    }
    NodeId GetLastId()
    {
        return nextId_ - SEQ_STEP;
    }
    size_t GetIdCount()
    {
        return idMap_.size();
    }
    CUnorderedMap<JSTaggedType, NodeId>* GetIdMap()
    {
        return &idMap_;
    }
    NodeId GetId()
    {
        return nextId_;
    }
    void SetId(NodeId id)
    {
        nextId_ = id;
    }

private:
    NodeId nextId_ {3U};  // 1 Reversed for SyntheticRoot
    CUnorderedMap<JSTaggedType, NodeId> idMap_ {};
};

#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
struct ScopeWrapper {
    LocalScope *localScope_ {nullptr};
    EcmaHandleScope *ecmaHandleScope_ {nullptr};
    ClockScope clockScope_;

    ScopeWrapper(LocalScope *localScope, EcmaHandleScope *ecmaHandleScope)
        : localScope_(localScope), ecmaHandleScope_(ecmaHandleScope) {}
};
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT

enum class DumpHeapSnapshotStatus : uint8_t {
    SUCCESS,
    FORK_FAILED,
    FAILED_TO_WAIT,
    WAIT_FORK_PROCESS_TIMEOUT,
};

class HeapProfiler : public HeapProfilerInterface {
public:
    NO_MOVE_SEMANTIC(HeapProfiler);
    NO_COPY_SEMANTIC(HeapProfiler);
    explicit HeapProfiler(const EcmaVM *vm);
    ~HeapProfiler() override;

    enum class SampleType { ONE_SHOT, REAL_TIME };

    void AllocationEvent(TaggedObject *address, size_t size) override;
    void MoveEvent(uintptr_t address, TaggedObject *forwardAddress, size_t size) override;
    /**
     * dump the specific snapshot in target format
     */
    bool DumpHeapSnapshot(Stream *stream, const DumpSnapShotOption &dumpOption, Progress *progress = nullptr,
                          std::function<void(uint8_t)> callback = [] (uint8_t) {}) override;
    void DumpHeapSnapshotForOOM(const DumpSnapShotOption &dumpOption, bool fromSharedGC = false) override;
    void AddSnapshot(HeapSnapshot *snapshot);

    bool StartHeapTracking(double timeInterval, bool isVmMode = true, Stream *stream = nullptr,
                           bool traceAllocation = false, bool newThread = true) override;
    bool StopHeapTracking(Stream *stream, Progress *progress = nullptr, bool newThread = true) override;
    bool UpdateHeapTracking(Stream *stream) override;
    bool StartHeapSampling(uint64_t samplingInterval, int stackDepth = 128) override;
    void StopHeapSampling() override;
    const struct SamplingInfo *GetAllocationProfile() override;
    size_t GetIdCount() override
    {
        return entryIdMap_->GetIdCount();
    }
    EntryIdMap *GetEntryIdMap() const
    {
        return const_cast<EntryIdMap *>(entryIdMap_);
    }
    Chunk *GetChunk() const
    {
        return const_cast<Chunk *>(&chunk_);
    }
    StringHashMap *GetEcmaStringTable() const
    {
        return const_cast<StringHashMap *>(&stringTable_);
    }
#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
    bool IsStartLocalHandleLeakDetect() const;
    void SwitchStartLocalHandleLeakDetect();
    void IncreaseScopeCount();
    void DecreaseScopeCount();
    void PushToActiveScopeStack(LocalScope *localScope, EcmaHandleScope *ecmaHandleScope);
    void PopFromActiveScopeStack();
    void ClearHandleBackTrace();
    std::string_view GetBackTraceOfHandle(uintptr_t handle) const;
    void WriteToLeakStackTraceFd(std::ostringstream &buffer) const;
    void SetLeakStackTraceFd(int32_t fd);
    int32_t GetLeakStackTraceFd() const;
    void CloseLeakStackTraceFd();
    void StorePotentiallyLeakHandles(uintptr_t handle);
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT

private:
    /**
     * trigger full gc to make sure no unreachable objects in heap
     */
    bool ForceFullGC(const EcmaVM *vm);
    void ForceSharedGC();
    void DumpHeapSnapshotFromSharedGC(Stream *stream, const DumpSnapShotOption &dumpOption);

    /**
     * make a new heap snapshot and put it into a container eg, vector
     */
    HeapSnapshot *MakeHeapSnapshot(SampleType sampleType, const DumpSnapShotOption &dumpOption,
                                   bool traceAllocation = false);
    bool DoDump(Stream *stream, Progress *progress, const DumpSnapShotOption &dumpOption);
    std::string GenDumpFileName(DumpFormat dumpFormat);
    CString GetTimeStamp();
    void UpdateHeapObjects(HeapSnapshot *snapshot);
    void ClearSnapshot();
    void FillIdMap();
    bool BinaryDump(Stream *stream, const DumpSnapShotOption &dumpOption);
#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
    uint32_t GetScopeCount() const;
    std::shared_ptr<ScopeWrapper> GetLastActiveScope() const;
    bool InsertHandleBackTrace(uintptr_t handle, const std::string &backTrace);
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT

    const size_t MAX_NUM_HPROF = 5;  // ~10MB
    const EcmaVM *vm_;
    CVector<HeapSnapshot *> hprofs_;
    StringHashMap stringTable_;
    bool isProfiling_ {false};
    EntryIdMap *entryIdMap_;
    std::unique_ptr<HeapTracker> heapTracker_;
    Chunk chunk_;
    std::unique_ptr<HeapSampling> heapSampling_ {nullptr};
    Mutex mutex_;
#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
    static const long LOCAL_HANDLE_LEAK_TIME_MS {5000};
    bool startLocalHandleLeakDetect_ {false};
    uint32_t scopeCount_ {0};
    std::stack<std::shared_ptr<ScopeWrapper>> activeScopeStack_;
    std::map<uintptr_t, std::string> handleBackTrace_;
    int32_t leakStackTraceFd_ {-1};
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT

    friend class HeapProfilerFriendTest;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_PROFILER_H
