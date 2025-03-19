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
    uint32_t objSize;
    uint32_t offset; // offset to the file
};

#define PER_GROUP_MEM_SIZE (128 * 1024 * 1024) // 128 MB
#define HEAD_NUM_PER_GROUP (PER_GROUP_MEM_SIZE / sizeof(AddrTableItem))
#define VERSION_ID_SIZE 8
#define MAX_FILE_SIZE (4 * 1024 * 1024 * 1024ULL) // 4 * 1024 * 1024 * 1024 : file size bigger than 4GB
#define MAX_OBJ_SIZE (MAX_FILE_SIZE >> 1)

struct NewAddr {
    char *data;
    uint32_t objSize;

    NewAddr(uint32_t actSize, uint32_t objSize) : objSize(objSize)
    {
        if (actSize == 0 || actSize > MAX_OBJ_SIZE) {
            LOG_ECMA(ERROR) << "ark raw heap decode abnormal obj size=" << actSize;
            data = nullptr;
            return;
        }
        data = new char[actSize];
    }
    ~NewAddr()
    {
        delete[] data;
    }
    char *Data()
    {
        return data;
    }
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
    bool GenerateHeapSnapshot(std::string &inputFilePath, std::string &outputPath) override;
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
    uint32_t WriteToBinFile(Stream *stream, char *objTab, uint32_t objNum,
                            CVector<std::pair<char *, uint32_t>> &memBuf);
    uint32_t CopyObjectMem2Buf(char *objTable, uint32_t objNum, CVector<std::pair<char *, uint32_t>> &memBufVec);
    uint32_t GenObjTable(CUnorderedMap<char *, uint32_t> &headerMap, HeapSnapshot *snapshot,
                         CUnorderedMap<uint64_t, CVector<uint64_t>> &strIdMapObjVec);
    uint32_t GenRootTable(Stream *stream);
    bool DumpRawHeap(Stream *stream, uint32_t &fileOffset, CVector<uint32_t> &secIndexVec);
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
    EntryIdMap* entryIdMap_;
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

class RawHeapDump final : public RootVisitor, public EcmaObjectRangeVisitor<RawHeapDump> {
public:
    RawHeapDump(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot, EntryIdMap* entryIdMap)
        : vm_(vm), stream_(stream), snapshot_(snapshot), entryIdMap_(entryIdMap), chunk_(vm->GetNativeAreaAllocator()),
          startTime_(std::chrono::steady_clock::now())
    {
        buffer_ = chunk_.NewArray<char>(PER_GROUP_MEM_SIZE);
    }

    ~RawHeapDump()
    {
        auto endTime = std::chrono::steady_clock::now();
        double duration = std::chrono::duration<double>(endTime - startTime_).count();
        LOG_ECMA(INFO) << "rawheap dump sucess, cost " << duration << 's';
        chunk_.Delete<char>(buffer_);
        strIdMapObjVec_.clear();
        secIndexVec_.clear();
        roots_.clear();
        readOnlyObjects_.clear();
    }

    void BinaryDump();

    void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override
    {
        HandleRootValue(slot.GetTaggedValue());
    }

    void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
    {
        for (ObjectSlot slot = start; slot < end; slot++) {
            HandleRootValue(slot.GetTaggedValue());
        }
    }

    void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
        [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override
    {
        // do nothing
    }

    void VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) override;

private:
    void DumpVersion();
    void DumpRootTable();
    void DumpObjectTable();
    void DumpObjectMemory();
    void DumpStringTable();
    void DumpSectionIndex();

    void WriteU64(uint64_t number);
    void WriteChunk(char *data, size_t size);
    void MaybeWriteBuffer();
    void WriteBinBlock();
    void UpdateStringTable(TaggedObject *object);
    void HandleRootValue(JSTaggedValue value);
    void IterateMarkedObjects(const std::function<void(void *)> &visitor);
    void ProcessMarkObjectsFromRoot(TaggedObject *root);
    bool MarkObject(TaggedObject *object);
    void ClearVisitMark();
    void EndVisitMark();

    const char versionID[VERSION_ID_SIZE] = {0};  // 0: current version id

    const EcmaVM *vm_ {nullptr};
    Stream *stream_ {nullptr};
    HeapSnapshot *snapshot_ {nullptr};
    EntryIdMap *entryIdMap_ {nullptr};
    char *buffer_ {nullptr};
    Chunk chunk_;
    CUnorderedMap<uint64_t, CVector<uint64_t>> strIdMapObjVec_ {};
    CUnorderedSet<TaggedObject*> roots_ {};
    CUnorderedSet<TaggedObject*> readOnlyObjects_ {};
    CVector<uint32_t> secIndexVec_ {};
    CQueue<TaggedObject *> bfsQueue_ {};
    size_t objCnt_ = 0;
    size_t bufIndex_ = 0;
    uint32_t fileOffset_ = 0;
    std::chrono::time_point<std::chrono::steady_clock> startTime_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_PROFILER_H
