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
#include "ecmascript/mem/c_containers.h"

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
    bool InsertId(JSTaggedType addr, NodeId id);
    bool EraseId(JSTaggedType addr);
    bool Move(JSTaggedType oldAddr, JSTaggedType forwardAddr);
    void UpdateEntryIdMap(HeapSnapshot *snapshot);
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

struct AddrTableItem {
    uint64_t addr;
    uint64_t id;
    uint64_t objSize;
    uint64_t offset; // offset to the file
    uint64_t stringId;
};

struct RawHeapObjInfo {
    AddrTableItem *tInfo;
    char *newAddr;
    bool isRoot;
    CUnorderedSet<uint64_t> refSet;
};

struct RawHeapInfoArgs {
    CVector<RawHeapObjInfo *> rawObjInfoVec;
    CUnorderedMap<uint64_t, RawHeapObjInfo *> oldAddrMapObjInfo;
    CUnorderedMap<uint64_t, const char *> strTableIdMapNewStr;
};

class ChunkDecoder {
public:
    explicit ChunkDecoder(char *mAddr, uint64_t fSize);

    ~ChunkDecoder()
    {
        auto &objInfoVec = rawHeapArgs.rawObjInfoVec;
        for (auto obj : objInfoVec) {
            delete obj;
        }
    }

    RawHeapInfoArgs &GetRawHeapInfoArgs()
    {
        return rawHeapArgs;
    }

private:
    void DecodeStrTable(const char *charPtr);

    uint64_t heapObjCnt;
    uint64_t rootObjCnt;
    uint64_t shareObjCnt;
    uint64_t strTableOffset;
    RawHeapInfoArgs rawHeapArgs;
    char *mapAddr;
    uint64_t fileSize;
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
    bool DumpHeapSnapshot(Stream *stream, const DumpSnapShotOption &dumpOption, Progress *progress = nullptr) override;
    void DumpHeapSnapshot(const DumpSnapShotOption &dumpOption) override;
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
    bool GenerateHeapSnapshot(std::string &inputFilePath, std::string &outputPath) override;

private:
    /**
     * trigger full gc to make sure no unreachable objects in heap
     */
    bool ForceFullGC(const EcmaVM *vm);
    void ForceSharedGC();

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

    const size_t MAX_NUM_HPROF = 5;  // ~10MB
    const EcmaVM *vm_;
    CVector<HeapSnapshot *> hprofs_;
    StringHashMap stringTable_;
    bool isProfiling_ {false};
    EntryIdMap* entryIdMap_;
    std::unique_ptr<HeapTracker> heapTracker_;
    Chunk chunk_;
    std::unique_ptr<HeapSampling> heapSampling_ {nullptr};
    Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_PROFILER_H
