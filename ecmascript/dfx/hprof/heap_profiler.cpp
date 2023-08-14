/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_profiler.h"

#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {
std::pair<bool, uint32_t> EntryIdMap::FindId(Address addr)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        return std::make_pair(false, GetNextId()); // return nextId if entry not exits
    } else {
        return std::make_pair(true, it->second);
    }
}

bool EntryIdMap::InsertId(Address addr, uint32_t id)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        idMap_.emplace(addr, id);
        return true;
    }
    idMap_[addr] = id;
    return false;
}

bool EntryIdMap::EraseId(Address addr)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        return false;
    }
    idMap_.erase(it);
    return true;
}

bool EntryIdMap::Move(Address oldAddr, Address forwardAddr)
{
    if (oldAddr == forwardAddr) {
        return true;
    }
    auto it = idMap_.find(oldAddr);
    if (it != idMap_.end()) {
        uint32_t id = it->second;
        idMap_.erase(it);
        idMap_[forwardAddr] = id;
        return true;
    }
    return false;
}

void EntryIdMap::RemoveDeadEntryId(HeapSnapshot *snapshot)
{
    auto nodes = snapshot->GetNodes();
    CUnorderedMap<Address, uint32_t> newIdMap;
    for (auto node : *nodes) {
        auto addr = node->GetAddress();
        auto it = idMap_.find(addr);
        if (it != idMap_.end()) {
            newIdMap.emplace(addr, it->second);
        }
    }
    idMap_ = newIdMap;
}

HeapProfiler::HeapProfiler(const EcmaVM *vm) : vm_(vm), chunk_(vm->GetNativeAreaAllocator())
{
    isProfiling_ = false;
    entryIdMap_ = GetChunk()->New<EntryIdMap>();
    jsonSerializer_ = GetChunk()->New<HeapSnapshotJSONSerializer>();
    if (UNLIKELY(jsonSerializer_ == nullptr)) {
        LOG_FULL(FATAL) << "alloc snapshot json serializer failed";
        UNREACHABLE();
    }
}

HeapProfiler::~HeapProfiler()
{
    ClearSnapshot();
    GetChunk()->Delete(entryIdMap_);
    GetChunk()->Delete(jsonSerializer_);
    jsonSerializer_ = nullptr;
}

void HeapProfiler::AllocationEvent(TaggedObject *address, size_t size)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (isProfiling_) {
        // Id will be allocated later while add new node
        if (heapTracker_ != nullptr) {
            heapTracker_->AllocationEvent(address, size);
        }
    }
}

void HeapProfiler::MoveEvent(uintptr_t address, TaggedObject *forwardAddress, size_t size)
{
    os::memory::LockHolder lock(mutex_);
    if (isProfiling_) {
        entryIdMap_->Move(address, reinterpret_cast<Address>(forwardAddress));
        if (heapTracker_ != nullptr) {
            heapTracker_->MoveEvent(address, forwardAddress, size);
        }
    }
}

void HeapProfiler::UpdateHeapObjects(HeapSnapshot *snapshot)
{
    vm_->CollectGarbage(TriggerGCType::OLD_GC);
    vm_->GetHeap()->GetSweeper()->EnsureAllTaskFinished();
    snapshot->UpdateNodes();
}

bool HeapProfiler::DumpHeapSnapshot(DumpFormat dumpFormat, Stream *stream, Progress *progress,
                                    bool isVmMode, bool isPrivate, bool captureNumericValue)
{
    [[maybe_unused]] bool heapClean = ForceFullGC(vm_);
    ASSERT(heapClean);
    LOG_ECMA(INFO) << "HeapProfiler DumpSnapshot start";
    size_t heapSize = vm_->GetHeap()->GetHeapObjectSize();
    LOG_ECMA(INFO) << "HeapProfiler DumpSnapshot heap size " << heapSize;
    int32_t heapCount = static_cast<int32_t>(vm_->GetHeap()->GetHeapObjectCount());
    if (progress != nullptr) {
        progress->ReportProgress(0, heapCount);
    }
    HeapSnapshot *snapshot = MakeHeapSnapshot(SampleType::ONE_SHOT, isVmMode, isPrivate, captureNumericValue);
    ASSERT(snapshot != nullptr);
    entryIdMap_->RemoveDeadEntryId(snapshot);
    isProfiling_ = true;
    if (progress != nullptr) {
        progress->ReportProgress(heapCount, heapCount);
    }
    if (!stream->Good()) {
        FileStream newStream(GenDumpFileName(dumpFormat));
        auto serializerResult = jsonSerializer_->Serialize(snapshot, &newStream);
        GetChunk()->Delete(snapshot);
        return serializerResult;
    }
    auto serializerResult = jsonSerializer_->Serialize(snapshot, stream);
    GetChunk()->Delete(snapshot);
    return serializerResult;
}

bool HeapProfiler::StartHeapTracking(double timeInterval, bool isVmMode, Stream *stream,
                                     bool traceAllocation, bool newThread)
{
    HeapSnapshot *snapshot = MakeHeapSnapshot(SampleType::REAL_TIME, isVmMode, false, false, traceAllocation);
    if (snapshot == nullptr) {
        return false;
    }
    isProfiling_ = true;
    UpdateHeapObjects(snapshot);
    heapTracker_ = std::make_unique<HeapTracker>(snapshot, timeInterval, stream);
    const_cast<EcmaVM *>(vm_)->StartHeapTracking();
    if (newThread) {
        heapTracker_->StartTracing();
    }

    return true;
}

bool HeapProfiler::UpdateHeapTracking(Stream *stream)
{
    if (heapTracker_ == nullptr) {
        return false;
    }
    HeapSnapshot *snapshot = heapTracker_->GetHeapSnapshot();
    if (snapshot == nullptr) {
        return false;
    }
    snapshot->RecordSampleTime();
    UpdateHeapObjects(snapshot);

    if (stream != nullptr) {
        snapshot->PushHeapStat(stream);
    }

    return true;
}

bool HeapProfiler::StopHeapTracking(Stream *stream, Progress *progress, bool newThread)
{
    if (heapTracker_ == nullptr) {
        return false;
    }
    int32_t heapCount = static_cast<int32_t>(vm_->GetHeap()->GetHeapObjectCount());

    const_cast<EcmaVM *>(vm_)->StopHeapTracking();
    if (newThread) {
        heapTracker_->StopTracing();
    }

    HeapSnapshot *snapshot = heapTracker_->GetHeapSnapshot();
    if (snapshot == nullptr) {
        return false;
    }

    if (progress != nullptr) {
        progress->ReportProgress(0, heapCount);
    }
    snapshot->FinishSnapshot();
    isProfiling_ = false;
    if (progress != nullptr) {
        progress->ReportProgress(heapCount, heapCount);
    }
    return jsonSerializer_->Serialize(snapshot, stream);
}

std::string HeapProfiler::GenDumpFileName(DumpFormat dumpFormat)
{
    CString filename("hprof_");
    switch (dumpFormat) {
        case DumpFormat::JSON:
            filename.append(GetTimeStamp());
            break;
        case DumpFormat::BINARY:
            filename.append("unimplemented");
            break;
        case DumpFormat::OTHER:
            filename.append("unimplemented");
            break;
        default:
            filename.append("unimplemented");
            break;
    }
    filename.append(".heapsnapshot");
    return ConvertToStdString(filename);
}

CString HeapProfiler::GetTimeStamp()
{
    std::time_t timeSource = std::time(nullptr);
    struct tm tm {
    };
    struct tm *timeData = localtime_r(&timeSource, &tm);
    if (timeData == nullptr) {
        LOG_FULL(FATAL) << "localtime_r failed";
        UNREACHABLE();
    }
    CString stamp;
    const int TIME_START = 1900;
    stamp.append(ToCString(timeData->tm_year + TIME_START))
        .append("-")
        .append(ToCString(timeData->tm_mon + 1))
        .append("-")
        .append(ToCString(timeData->tm_mday))
        .append("_")
        .append(ToCString(timeData->tm_hour))
        .append("-")
        .append(ToCString(timeData->tm_min))
        .append("-")
        .append(ToCString(timeData->tm_sec));
    return stamp;
}

bool HeapProfiler::ForceFullGC(const EcmaVM *vm)
{
    if (vm->IsInitialized()) {
        const_cast<Heap *>(vm->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
        return true;
    }
    return false;
}

HeapSnapshot *HeapProfiler::MakeHeapSnapshot(SampleType sampleType, bool isVmMode, bool isPrivate,
                                             bool captureNumericValue, bool traceAllocation)
{
    LOG_ECMA(INFO) << "HeapProfiler::MakeHeapSnapshot";
    DISALLOW_GARBAGE_COLLECTION;
    const_cast<Heap *>(vm_->GetHeap())->Prepare();
    switch (sampleType) {
        case SampleType::ONE_SHOT: {
            auto *snapshot = GetChunk()->New<HeapSnapshot>(vm_, isVmMode, isPrivate, captureNumericValue,
                                                           traceAllocation, entryIdMap_, GetChunk());
            if (snapshot == nullptr) {
                LOG_FULL(FATAL) << "alloc snapshot failed";
                UNREACHABLE();
            }
            snapshot->BuildUp();
            return snapshot;
        }
        case SampleType::REAL_TIME: {
            auto *snapshot = GetChunk()->New<HeapSnapshot>(vm_, isVmMode, isPrivate, captureNumericValue,
                                                           traceAllocation, entryIdMap_, GetChunk());
            if (snapshot == nullptr) {
                LOG_FULL(FATAL) << "alloc snapshot failed";
                UNREACHABLE();
            }
            AddSnapshot(snapshot);
            snapshot->PrepareSnapshot();
            return snapshot;
        }
        default:
            return nullptr;
    }
}

void HeapProfiler::AddSnapshot(HeapSnapshot *snapshot)
{
    if (hprofs_.size() >= MAX_NUM_HPROF) {
        ClearSnapshot();
    }
    ASSERT(snapshot != nullptr);
    hprofs_.emplace_back(snapshot);
}

void HeapProfiler::ClearSnapshot()
{
    for (auto *snapshot : hprofs_) {
        GetChunk()->Delete(snapshot);
    }
    hprofs_.clear();
}

bool HeapProfiler::StartHeapSampling(uint64_t samplingInterval, int stackDepth)
{
    if (heapSampling_.get()) {
        LOG_ECMA(ERROR) << "Do not start heap sampling twice in a row.";
        return false;
    }
    heapSampling_ = std::make_unique<HeapSampling>(vm_, const_cast<Heap *>(vm_->GetHeap()),
                                                   samplingInterval, stackDepth);
    return true;
}

void HeapProfiler::StopHeapSampling()
{
    heapSampling_.reset();
}

const struct SamplingInfo *HeapProfiler::GetAllocationProfile()
{
    if (!heapSampling_.get()) {
        LOG_ECMA(ERROR) << "Heap sampling was not started, please start firstly.";
        return nullptr;
    }
    return heapSampling_->GetAllocationProfile();
}
}  // namespace panda::ecmascript
