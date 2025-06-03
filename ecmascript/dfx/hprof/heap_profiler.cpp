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
#include <sys/wait.h>
#include <sys/prctl.h>
#include "ecmascript/dfx/hprof/heap_profiler.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/rawheap_dump.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/base/block_hook_scope.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/platform/backtrace.h"
#include "ecmascript/platform/file.h"

#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
#endif

namespace panda::ecmascript {

std::pair<bool, NodeId> EntryIdMap::FindId(JSTaggedType addr)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        return std::make_pair(false, GetNextId()); // return nextId if entry not exits
    } else {
        return std::make_pair(true, it->second);
    }
}

NodeId EntryIdMap::FindOrInsertNodeId(JSTaggedType addr)
{
    auto it = idMap_.find(addr);
    if (it != idMap_.end()) {
        return it->second;
    }
    NodeId id = GetNextId();
    idMap_.emplace(addr, id);
    return id;
}

bool EntryIdMap::InsertId(JSTaggedType addr, NodeId id)
{
    auto [iter, inserted] = idMap_.insert_or_assign(addr, id);
    return inserted;
}

bool EntryIdMap::EraseId(JSTaggedType addr)
{
    return idMap_.erase(addr);
}

bool EntryIdMap::Move(JSTaggedType oldAddr, JSTaggedType forwardAddr)
{
    if (oldAddr == forwardAddr) {
        return true;
    }
    auto it = idMap_.find(oldAddr);
    if (it != idMap_.end()) {
        NodeId id = it->second;
        idMap_.erase(it);
        idMap_.insert_or_assign(forwardAddr, id);
        return true;
    }
    return false;
}

void EntryIdMap::UpdateEntryIdMap(HeapSnapshot *snapshot)
{
    LOG_ECMA(INFO) << "EntryIdMap::UpdateEntryIdMap";
    if (snapshot == nullptr) {
        LOG_ECMA(FATAL) << "EntryIdMap::UpdateEntryIdMap:snapshot is nullptr";
        UNREACHABLE();
    }

    HeapMarker marker {};
    auto nodes = snapshot->GetNodes();
    for (auto node : *nodes) {
        auto addr = node->GetAddress();
        if (JSTaggedValue{addr}.IsHeapObject()) {
            marker.Mark(addr);
        }
    }
    RemoveUnmarkedObjects(marker);
}

void EntryIdMap::RemoveUnmarkedObjects(HeapMarker &marker)
{
    for (auto it = idMap_.begin(); it != idMap_.end();) {
        JSTaggedType addr = it->first;
        if (JSTaggedValue{addr}.IsHeapObject() && !marker.IsMarked(addr)) {
            it = idMap_.erase(it);
        } else {
            ++it;
        }
    }
}

HeapProfiler::HeapProfiler(const EcmaVM *vm) : vm_(vm), stringTable_(vm), chunk_(vm->GetNativeAreaAllocator())
{
    isProfiling_ = false;
    entryIdMap_ = GetChunk()->New<EntryIdMap>();
}

HeapProfiler::~HeapProfiler()
{
    JSPandaFileManager::GetInstance()->ClearNameMap();
    ClearSnapshot();
    GetChunk()->Delete(entryIdMap_);
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
    LockHolder lock(mutex_);
    if (isProfiling_) {
        entryIdMap_->Move(static_cast<JSTaggedType>(address), reinterpret_cast<JSTaggedType>(forwardAddress));
        if (heapTracker_ != nullptr) {
            heapTracker_->MoveEvent(address, forwardAddress, size);
        }
    }
}

void HeapProfiler::UpdateHeapObjects(HeapSnapshot *snapshot)
{
    SharedHeap::GetInstance()->GetSweeper()->WaitAllTaskFinished();
    snapshot->UpdateNodes();
}

void HeapProfiler::DumpHeapSnapshotForOOM([[maybe_unused]] const DumpSnapShotOption &dumpOption,
                                          [[maybe_unused]] bool fromSharedGC)
{
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    // Write in faultlog for heap leak.
    int32_t fd;
#if defined(PANDA_TARGET_ARM32)
    DumpSnapShotOption doDumpOption;
    doDumpOption.dumpFormat = DumpFormat::JSON;
    doDumpOption.isFullGC = dumpOption.isFullGC;
    doDumpOption.isSimplify = true;
    doDumpOption.isBeforeFill = false;
    fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_HEAP_SNAPSHOT));
    if (fd < 0) {
        LOG_ECMA(ERROR) << "OOM Dump Write FD failed, fd" << fd;
        return;
    }
    FileDescriptorStream stream(fd);
    DumpHeapSnapshot(&stream, doDumpOption);
#else
    if (dumpOption.isDumpOOM && dumpOption.dumpFormat == DumpFormat::BINARY) {
        fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_RAW_SNAPSHOT));
    } else {
        fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_HEAP_SNAPSHOT));
    }
    if (fd < 0) {
        LOG_ECMA(ERROR) << "OOM Dump Write FD failed, fd" << fd;
        return;
    }
    FileDescriptorStream stream(fd);
    if (!fromSharedGC) {
        DumpHeapSnapshot(&stream, dumpOption);
    } else {
        DumpHeapSnapshotFromSharedGC(&stream, dumpOption);
    }
#endif
#endif
}

void HeapProfiler::DumpHeapSnapshotFromSharedGC(Stream *stream, const DumpSnapShotOption &dumpOption)
{
    base::BlockHookScope blockScope;
    const_cast<Heap*>(vm_->GetHeap())->Prepare();
    SharedHeap::GetInstance()->Prepare(true);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->FillBumpPointerForTlab();
    });
    BinaryDump(stream, dumpOption);
    stream->EndOfStream();
}

bool HeapProfiler::DoDump(Stream *stream, Progress *progress, const DumpSnapShotOption &dumpOption)
{
    DISALLOW_GARBAGE_COLLECTION;
    int32_t heapCount = 0;
    HeapSnapshot *snapshot = nullptr;
    {
        if (dumpOption.isFullGC) {
            size_t heapSize = vm_->GetHeap()->GetLiveObjectSize();
            LOG_ECMA(INFO) << "HeapProfiler DumpSnapshot heap size " << heapSize;
            heapCount = static_cast<int32_t>(vm_->GetHeap()->GetHeapObjectCount());
            if (progress != nullptr) {
                progress->ReportProgress(0, heapCount);
            }
        }
        snapshot = MakeHeapSnapshot(SampleType::ONE_SHOT, dumpOption);
        ASSERT(snapshot != nullptr);
    }
    // In async mode, EntryIdMap is filled and updated in parent-process,
    // so EntryIdMap needs to be updated only in sync mode.
    if (dumpOption.isSync) {
        entryIdMap_->UpdateEntryIdMap(snapshot);
    }
    isProfiling_ = true;
    if (progress != nullptr) {
        progress->ReportProgress(heapCount, heapCount);
    }
    if (!stream->Good()) {
        FileStream newStream(GenDumpFileName(dumpOption.dumpFormat));
        auto serializerResult = HeapSnapshotJSONSerializer::Serialize(snapshot, &newStream);
        GetChunk()->Delete(snapshot);
        return serializerResult;
    }
    auto serializerResult = HeapSnapshotJSONSerializer::Serialize(snapshot, stream);
    GetChunk()->Delete(snapshot);
    return serializerResult;
}

[[maybe_unused]]static void WaitProcess(pid_t pid, const std::function<void(uint8_t)> &callback)
{
    time_t startTime = time(nullptr);
    constexpr int DUMP_TIME_OUT = 300;
    constexpr int DEFAULT_SLEEP_TIME = 100000;
    while (true) {
        int status = 0;
        pid_t p = waitpid(pid, &status, WNOHANG);
        if (p < 0) {
            LOG_GC(ERROR) << "DumpHeapSnapshot wait failed ";
            if (callback) {
                callback(static_cast<uint8_t>(DumpHeapSnapshotStatus::FAILED_TO_WAIT));
            }
            return;
        }
        if (p == pid) {
            if (callback) {
                callback(static_cast<uint8_t>(DumpHeapSnapshotStatus::SUCCESS));
            }
            return;
        }
        if (time(nullptr) > startTime + DUMP_TIME_OUT) {
            LOG_GC(ERROR) << "DumpHeapSnapshot kill thread, wait " << DUMP_TIME_OUT << " s";
            kill(pid, SIGTERM);
            if (callback) {
                callback(static_cast<uint8_t>(DumpHeapSnapshotStatus::WAIT_FORK_PROCESS_TIMEOUT));
            }
            return;
        }
        usleep(DEFAULT_SLEEP_TIME);
    }
}

bool HeapProfiler::BinaryDump(Stream *stream, const DumpSnapShotOption &dumpOption)
{
    [[maybe_unused]] EcmaHandleScope ecmaHandleScope(vm_->GetJSThread());
    DumpSnapShotOption option;
    auto stringTable = chunk_.New<StringHashMap>(vm_);
    auto snapshot = chunk_.New<HeapSnapshot>(vm_, stringTable, option, false, entryIdMap_);

    if (const_cast<EcmaVM *>(vm_)->GetJSOptions().EnableRawHeapCrop()) {
        Runtime::GetInstance()->SetRawHeapDumpCropLevel(RawHeapDumpCropLevel::LEVEL_V2);
    }

    RawHeapDump *rawHeapDump = nullptr;
    RawHeapDumpCropLevel cropLevel = Runtime::GetInstance()->GetRawHeapDumpCropLevel();
    switch (cropLevel) {
        case RawHeapDumpCropLevel::LEVEL_V1:
            rawHeapDump = new RawHeapDumpV1(vm_, stream, snapshot, entryIdMap_, dumpOption);
            break;
        case RawHeapDumpCropLevel::LEVEL_V2:
            rawHeapDump = new RawHeapDumpV2(vm_, stream, snapshot, entryIdMap_, dumpOption);
            break;
        default:
            LOG_ECMA(FATAL) << "rawheap dump, do not supported crop level " << static_cast<int>(cropLevel);
            UNREACHABLE();
            break;
    }

    rawHeapDump->BinaryDump();
    chunk_.Delete<StringHashMap>(stringTable);
    chunk_.Delete<HeapSnapshot>(snapshot);
    delete rawHeapDump;
    return true;
}

void HeapProfiler::FillIdMap()
{
    HeapMarker marker {};

    // Iterate SharedHeap Object
    SharedHeap* sHeap = SharedHeap::GetInstance();
    if (sHeap != nullptr) {
        sHeap->IterateOverObjects([this, &marker](TaggedObject *obj) {
            JSTaggedType addr = ((JSTaggedValue)obj).GetRawData();
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            entryIdMap_->InsertId(addr, sequenceId);
            marker.Mark(addr);
        });
        sHeap->GetReadOnlySpace()->IterateOverObjects([this, &marker](TaggedObject *obj) {
            JSTaggedType addr = ((JSTaggedValue)obj).GetRawData();
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            entryIdMap_->InsertId(addr, sequenceId);
            marker.Mark(addr);
        });
    }

    // Iterate LocalHeap Object
    auto heap = vm_->GetHeap();
    if (heap != nullptr) {
        heap->IterateOverObjects([this, &marker](TaggedObject *obj) {
            JSTaggedType addr = ((JSTaggedValue)obj).GetRawData();
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            entryIdMap_->InsertId(addr, sequenceId);
            marker.Mark(addr);
        });
    }

    entryIdMap_->RemoveUnmarkedObjects(marker);
}

bool HeapProfiler::DumpHeapSnapshot(Stream *stream, const DumpSnapShotOption &dumpOption, Progress *progress,
                                    std::function<void(uint8_t)> callback)
{
    bool res = false;
    base::BlockHookScope blockScope;
    ThreadManagedScope managedScope(vm_->GetJSThread());
    pid_t pid = -1;
    {
        if (dumpOption.isFullGC) {
#ifdef USE_CMC_GC
            BaseRuntime::RequestGC(GcType::FULL);
#else
            [[maybe_unused]] bool heapClean = ForceFullGC(vm_);
            ForceSharedGC();
            ASSERT(heapClean);
#endif
        }
        SuspendAllScope suspendScope(vm_->GetAssociatedJSThread()); // suspend All.
        const_cast<Heap*>(vm_->GetHeap())->Prepare();
        SharedHeap::GetInstance()->Prepare(true);
        Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
            ASSERT(!thread->IsInRunningState());
            const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->FillBumpPointerForTlab();
        });
        // OOM and ThresholdReachedDump.
        if (dumpOption.isDumpOOM) {
            res = BinaryDump(stream, dumpOption);
            stream->EndOfStream();
            return res;
        }
        // ide.
        if (dumpOption.isSync) {
            return DoDump(stream, progress, dumpOption);
        }
        AppFreezeFilterCallback appfreezeCallback = Runtime::GetInstance()->GetAppFreezeFilterCallback();
        if (appfreezeCallback != nullptr && !appfreezeCallback(getpid(), false)) {
            LOG_ECMA(ERROR) << "failed to set appfreeze filter";
            return false;
        }
        // hidumper do fork and fillmap.
        if (dumpOption.isBeforeFill) {
            FillIdMap();
        }
        // fork
        if ((pid = fork()) < 0) {
            LOG_ECMA(ERROR) << "DumpHeapSnapshot fork failed!";
            if (callback) {
                callback(static_cast<uint8_t>(DumpHeapSnapshotStatus::FORK_FAILED));
            }
            return false;
        }
        if (pid == 0) {
            vm_->GetAssociatedJSThread()->EnableCrossThreadExecution();
            prctl(PR_SET_NAME, reinterpret_cast<unsigned long>("dump_process"), 0, 0, 0);
            if (dumpOption.dumpFormat == DumpFormat::BINARY) {
                res = BinaryDump(stream, dumpOption);
                stream->EndOfStream();
            } else {
                res = DoDump(stream, progress, dumpOption);
            }
            _exit(0);
        }
    }
    if (pid != 0) {
        std::thread thread(&WaitProcess, pid, callback);
        thread.detach();
    }
    isProfiling_ = true;
    return res;
}

bool HeapProfiler::StartHeapTracking(double timeInterval, bool isVmMode, Stream *stream,
                                     bool traceAllocation, bool newThread)
{
#ifdef USE_CMC_GC
    BaseRuntime::RequestGC(GcType::FULL);
#else
    vm_->CollectGarbage(TriggerGCType::OLD_GC);
    ForceSharedGC();
#endif
    SuspendAllScope suspendScope(vm_->GetAssociatedJSThread());
    DumpSnapShotOption dumpOption;
    dumpOption.isVmMode = isVmMode;
    dumpOption.isPrivate = false;
    dumpOption.captureNumericValue = false;
    HeapSnapshot *snapshot = MakeHeapSnapshot(SampleType::REAL_TIME, dumpOption, traceAllocation);
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

    {
#ifdef USE_CMC_GC
        BaseRuntime::RequestGC(GcType::FULL);
#else
        vm_->CollectGarbage(TriggerGCType::OLD_GC);
        ForceSharedGC();
#endif
        SuspendAllScope suspendScope(vm_->GetAssociatedJSThread());
        UpdateHeapObjects(snapshot);
        snapshot->RecordSampleTime();
    }

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
    {
#ifdef USE_CMC_GC
        BaseRuntime::RequestGC(GcType::FULL);
#else
        ForceSharedGC();
#endif
        SuspendAllScope suspendScope(vm_->GetAssociatedJSThread());
#ifndef USE_CMC_GC
        SharedHeap::GetInstance()->GetSweeper()->WaitAllTaskFinished();
#endif
        snapshot->FinishSnapshot();
    }

    isProfiling_ = false;
    if (progress != nullptr) {
        progress->ReportProgress(heapCount, heapCount);
    }
    return HeapSnapshotJSONSerializer::Serialize(snapshot, stream);
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

void HeapProfiler::ForceSharedGC()
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(vm_->GetAssociatedJSThread());
    sHeap->GetSweeper()->WaitAllTaskFinished();
}

HeapSnapshot *HeapProfiler::MakeHeapSnapshot(SampleType sampleType, const DumpSnapShotOption &dumpOption,
                                             bool traceAllocation)
{
    LOG_ECMA(INFO) << "HeapProfiler::MakeHeapSnapshot";
    if (dumpOption.isFullGC) {
        DISALLOW_GARBAGE_COLLECTION;
        const_cast<Heap *>(vm_->GetHeap())->Prepare();
    }
    switch (sampleType) {
        case SampleType::ONE_SHOT: {
            auto *snapshot = GetChunk()->New<HeapSnapshot>(vm_, GetEcmaStringTable(), dumpOption,
                                                           traceAllocation, entryIdMap_);
            if (snapshot == nullptr) {
                LOG_FULL(FATAL) << "alloc snapshot failed";
                UNREACHABLE();
            }
            snapshot->BuildUp(dumpOption.isSimplify);
            return snapshot;
        }
        case SampleType::REAL_TIME: {
            auto *snapshot = GetChunk()->New<HeapSnapshot>(vm_, GetEcmaStringTable(), dumpOption,
                                                           traceAllocation, entryIdMap_);
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

#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
bool HeapProfiler::IsStartLocalHandleLeakDetect() const
{
    return startLocalHandleLeakDetect_;
}

void HeapProfiler::SwitchStartLocalHandleLeakDetect()
{
    startLocalHandleLeakDetect_ = !startLocalHandleLeakDetect_;
}

void HeapProfiler::IncreaseScopeCount()
{
    ++scopeCount_;
}

void HeapProfiler::DecreaseScopeCount()
{
    --scopeCount_;
}

uint32_t HeapProfiler::GetScopeCount() const
{
    return scopeCount_;
}

void HeapProfiler::PushToActiveScopeStack(LocalScope *localScope, EcmaHandleScope *ecmaHandleScope)
{
    activeScopeStack_.emplace(std::make_shared<ScopeWrapper>(localScope, ecmaHandleScope));
}

void HeapProfiler::PopFromActiveScopeStack()
{
    if (!activeScopeStack_.empty()) {
        activeScopeStack_.pop();
    }
}

std::shared_ptr<ScopeWrapper> HeapProfiler::GetLastActiveScope() const
{
    if (!activeScopeStack_.empty()) {
        return activeScopeStack_.top();
    }
    return nullptr;
}

void HeapProfiler::ClearHandleBackTrace()
{
    handleBackTrace_.clear();
}

std::string_view HeapProfiler::GetBackTraceOfHandle(const uintptr_t handle) const
{
    const auto it = handleBackTrace_.find(handle);
    if (it != handleBackTrace_.end()) {
        return std::string_view(it->second);
    }
    return "";
}

bool HeapProfiler::InsertHandleBackTrace(uintptr_t handle, const std::string &backTrace)
{
    auto [iter, inserted] = handleBackTrace_.insert_or_assign(handle, backTrace);
    return inserted;
}

void HeapProfiler::WriteToLeakStackTraceFd(std::ostringstream &buffer) const
{
    if (leakStackTraceFd_ < 0) {
        return;
    }
    buffer << std::endl;
    DPrintf(reinterpret_cast<fd_t>(leakStackTraceFd_), buffer.str());
    buffer.str("");
}

void HeapProfiler::SetLeakStackTraceFd(const int32_t fd)
{
    FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
    leakStackTraceFd_ = fd;
}

int32_t HeapProfiler::GetLeakStackTraceFd() const
{
    return leakStackTraceFd_;
}

void HeapProfiler::CloseLeakStackTraceFd()
{
    if (leakStackTraceFd_ != -1) {
        FSync(reinterpret_cast<fd_t>(leakStackTraceFd_));
        Close(reinterpret_cast<fd_t>(leakStackTraceFd_));
        leakStackTraceFd_ = -1;
    }
}

void HeapProfiler::StorePotentiallyLeakHandles(const uintptr_t handle)
{
    bool isDetectedByScopeCount { GetScopeCount() <= 1 };
    bool isDetectedByScopeTime { false };
    if (auto lastScope = GetLastActiveScope()) {
        auto timeSinceLastScopeCreate = lastScope->clockScope_.TotalSpentTime();
        isDetectedByScopeTime = timeSinceLastScopeCreate >= LOCAL_HANDLE_LEAK_TIME_MS;
    }
    if (isDetectedByScopeCount || isDetectedByScopeTime) {
        std::ostringstream stack;
        Backtrace(stack, true);
        InsertHandleBackTrace(handle, stack.str());
    }
}
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT
}  // namespace panda::ecmascript
