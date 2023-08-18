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

#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/dfx/hprof/heap_sampling.h"
#include "ecmascript/dfx/hprof/progress.h"

namespace panda::ecmascript {
class HeapSnapshot;
class EcmaVM;

class EntryIdMap {
public:
    EntryIdMap() = default;
    ~EntryIdMap() = default;
    NO_COPY_SEMANTIC(EntryIdMap);
    NO_MOVE_SEMANTIC(EntryIdMap);

    static constexpr uint32_t SEQ_STEP = 2;
    std::pair<bool, uint32_t> FindId(Address addr);
    bool InsertId(Address addr, uint32_t id);
    bool EraseId(Address addr);
    bool Move(Address oldAddr, Address forwardAddr);
    void RemoveDeadEntryId(HeapSnapshot *snapshot);
    uint32_t GetNextId()
    {
        nextId_ += SEQ_STEP;
        return nextId_ - SEQ_STEP;
    }
    uint32_t GetLastId()
    {
        return nextId_ - SEQ_STEP;
    }
    size_t GetIdCount()
    {
        return idMap_.size();
    }

private:
    uint32_t nextId_ {3U};  // 1 Reversed for SyntheticRoot
    CUnorderedMap<Address, uint32_t> idMap_ {};
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
    bool DumpHeapSnapshot(DumpFormat dumpFormat, Stream *stream, Progress *progress = nullptr,
                          bool isVmMode = true, bool isPrivate = false, bool captureNumericValue = false) override;

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
private:
    /**
     * trigger full gc to make sure no unreachable objects in heap
     */
    bool ForceFullGC(const EcmaVM *vm);

    /**
     * make a new heap snapshot and put it into a container eg, vector
     */
    HeapSnapshot *MakeHeapSnapshot(SampleType sampleType, bool isVmMode = true,
                                   bool isPrivate = false, bool captureNumericValue = false,
                                   bool traceAllocation = false);
    std::string GenDumpFileName(DumpFormat dumpFormat);
    CString GetTimeStamp();
    void UpdateHeapObjects(HeapSnapshot *snapshot);
    void ClearSnapshot();

    const size_t MAX_NUM_HPROF = 5;  // ~10MB
    const EcmaVM *vm_;
    CVector<HeapSnapshot *> hprofs_;
    HeapSnapshotJSONSerializer *jsonSerializer_ {nullptr};
    bool isProfiling_ {false};
    EntryIdMap* entryIdMap_;
    std::unique_ptr<HeapTracker> heapTracker_;
    Chunk chunk_;
    std::unique_ptr<HeapSampling> heapSampling_ {nullptr};
    os::memory::Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_PROFILER_H
