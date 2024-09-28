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
#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include "ecmascript/dfx/hprof/heap_profiler.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/base/block_hook_scope.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/mem/object_xray.h"

#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
#endif

namespace panda::ecmascript {
static pid_t ForkBySyscall(void)
{
#ifdef SYS_fork
    return syscall(SYS_fork);
#else
    return syscall(SYS_clone, SIGCHLD, 0);
#endif
}

std::pair<bool, NodeId> EntryIdMap::FindId(JSTaggedType addr)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        return std::make_pair(false, GetNextId()); // return nextId if entry not exits
    } else {
        return std::make_pair(true, it->second);
    }
}

bool EntryIdMap::InsertId(JSTaggedType addr, NodeId id)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        idMap_.emplace(addr, id);
        return true;
    }
    idMap_[addr] = id;
    return false;
}

bool EntryIdMap::EraseId(JSTaggedType addr)
{
    auto it = idMap_.find(addr);
    if (it == idMap_.end()) {
        return false;
    }
    idMap_.erase(it);
    return true;
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
        idMap_[forwardAddr] = id;
        return true;
    }
    return false;
}

void EntryIdMap::UpdateEntryIdMap(HeapSnapshot *snapshot)
{
    if (snapshot == nullptr) {
        LOG_ECMA(FATAL) << "EntryIdMap::UpdateEntryIdMap:snapshot is nullptr";
        UNREACHABLE();
    }
    auto nodes = snapshot->GetNodes();
    CUnorderedMap<JSTaggedType, NodeId> newIdMap;
    for (auto node : *nodes) {
        auto addr = node->GetAddress();
        auto it = idMap_.find(addr);
        if (it != idMap_.end()) {
            newIdMap.emplace(addr, it->second);
        }
    }
    idMap_.clear();
    idMap_ = newIdMap;
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

void HeapProfiler::DumpHeapSnapshot([[maybe_unused]] const DumpSnapShotOption &dumpOption)
{
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    // Write in faultlog for heap leak.
    int32_t fd;
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
    DumpHeapSnapshot(&stream, dumpOption);
#endif
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
    entryIdMap_->UpdateEntryIdMap(snapshot);
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

ChunkDecoder::ChunkDecoder(char *mAddr, uint64_t fSize) : mapAddr(mAddr), fileSize(fSize)
{
    auto u64Ptr = reinterpret_cast<uint64_t *>(mapAddr);
    size_t currInd = 0;
    heapObjCnt = u64Ptr[currInd++];
    rootObjCnt = u64Ptr[currInd++];
    shareObjCnt = u64Ptr[currInd++];
    strTableOffset = u64Ptr[currInd++];
    LOG_ECMA(INFO) << "ChunkDecoder init: heapObjCnt=" << heapObjCnt << ", rootObjCnt=" << rootObjCnt
                    << ", ShareObjCnt=" << shareObjCnt << std::hex << ", strTableOffset=0x" << strTableOffset;
    auto cPtr = mapAddr;
    CUnorderedSet<uint64_t> rootAddrSet;
    for (uint64_t i = 0; i < rootObjCnt; i++) {
        rootAddrSet.insert(u64Ptr[currInd++]);
    }
    auto &objInfoVec = rawHeapArgs.rawObjInfoVec;
    auto table = reinterpret_cast<AddrTableItem *>(&u64Ptr[currInd]);
    for (uint64_t i = 0; i < heapObjCnt; ++i) {
        auto objInfo = new RawHeapObjInfo();
        objInfo->tInfo = &table[i];
        if (rootAddrSet.find(objInfo->tInfo->addr) != rootAddrSet.end()) {
            objInfo->isRoot = true;
        } else {
            objInfo->isRoot = false;
        }
        objInfoVec.push_back(objInfo);
        auto objMem = cPtr + objInfo->tInfo->offset;
        objInfo->newAddr = objMem;
        rawHeapArgs.oldAddrMapObjInfo.emplace(objInfo->tInfo->addr, objInfo);
    }
    DecodeStrTable(cPtr);
}

void ChunkDecoder::DecodeStrTable(const char *charPtr)
{
    auto currInd = strTableOffset;
    if (currInd >= fileSize) {
        LOG_ECMA(ERROR) << "DecodeStrTable no str table: str=" << charPtr;
        return;
    }
    auto &strTableIdMap = rawHeapArgs.strTableIdMapNewStr;

    auto u64Ptr = reinterpret_cast<const uint64_t *>(charPtr + currInd);
    auto strCnt = *u64Ptr;
    LOG_ECMA(INFO) << "DecodeStrTable: strCnt=" << std::dec << strCnt;
    while (currInd < fileSize && strTableIdMap.size() < strCnt) {
        auto id = *reinterpret_cast<const uint64_t *>(charPtr + currInd);
        currInd += sizeof(uint64_t);
        if (currInd >= fileSize) {
            break;
        }
        auto *currPtr = &charPtr[currInd];
        auto currSize = strlen(currPtr) + 1;
        if (currSize == 1) {
            currInd += currSize;
            continue;
        }
        strTableIdMap[id] = currPtr;
        currInd += currSize;
    }
    LOG_ECMA(INFO) << "DecodeStrTable finished: strTableVec.size=" << strTableIdMap.size();
}

static uint64_t CheckAndRemoveWeak(JSTaggedValue &value, uint64_t originalAddr)
{
    if (!value.IsWeak()) {
        return originalAddr;
    }
    JSTaggedValue weakValue(originalAddr);
    weakValue.RemoveWeakTag();
    return weakValue.GetRawData();
}

static uint64_t CheckAndAddWeak(JSTaggedValue &value, uint64_t originalAddr)
{
    if (!value.IsWeak()) {
        return originalAddr;
    }
    JSTaggedValue weakValue(originalAddr);
    weakValue.CreateWeakRef();
    return weakValue.GetRawData();
}

void DecodeObj(RawHeapInfoArgs &rawHeapArgs, HeapSnapshot *snapshot)
{
    std::set<uint64_t> notFoundObj;
    CUnorderedSet<uint64_t> *refVec = nullptr;
    auto visitor = [&notFoundObj, &rawHeapArgs, &refVec] ([[maybe_unused]] TaggedObject *root,
                    ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::RAW_DATA || area == VisitObjectArea::NATIVE_POINTER) {
            return;
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto taggedPointerAddr = reinterpret_cast<uint64_t **>(slot.SlotAddress());
            JSTaggedValue value(reinterpret_cast<TaggedObject *>(*taggedPointerAddr));
            auto originalAddr = reinterpret_cast<uint64_t>(*taggedPointerAddr);
            originalAddr = CheckAndRemoveWeak(value, originalAddr);
            if (!value.IsHeapObject() || originalAddr == 0) {
                continue;
            }
            auto toItemInfo = rawHeapArgs.oldAddrMapObjInfo.find(originalAddr);
            if (toItemInfo == rawHeapArgs.oldAddrMapObjInfo.end()) {
                notFoundObj.insert(reinterpret_cast<uint64_t>(*taggedPointerAddr));
                continue;
            }
            auto newAddr = reinterpret_cast<uint64_t>(toItemInfo->second->newAddr);
            newAddr = CheckAndAddWeak(value, newAddr);
            refVec->insert(newAddr);
            slot.Update(reinterpret_cast<TaggedObject *>(newAddr));
        }
    };
    for (auto v : rawHeapArgs.rawObjInfoVec) {
        refVec = &v->refSet;
        auto jsHclassAddr = *reinterpret_cast<uint64_t *>(v->newAddr);
        auto jsHclassItem = rawHeapArgs.oldAddrMapObjInfo.find(jsHclassAddr);
        if (jsHclassItem == rawHeapArgs.oldAddrMapObjInfo.end()) {
            LOG_ECMA(ERROR) << "ark DecodeObj hclass not find jsHclassAddr=" << std::hex << jsHclassAddr;
            continue;
        }
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(v->newAddr);
        *reinterpret_cast<uint64_t *>(v->newAddr) = reinterpret_cast<uint64_t>(jsHclassItem->second->newAddr);
        auto hclassObj = reinterpret_cast<JSHClass *>(jsHclassItem->second->newAddr);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclassObj, visitor);
    }
    LOG_ECMA(INFO) << "ark visitor: not found obj num= " << notFoundObj.size() << ", generate nodes";
    for (auto v : rawHeapArgs.rawObjInfoVec) {
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(v->newAddr);
        snapshot->GenerateNodeForBinMod(obj, v, rawHeapArgs.strTableIdMapNewStr);
    }
}

static uint64_t GetFileSize(std::string &inputFilePath)
{
    if (inputFilePath.empty()) {
        return 0;
    }
    struct stat fileInfo;
    if (stat(inputFilePath.c_str(), &fileInfo) == 0) {
        return fileInfo.st_size;
    }
    return 0;
}

bool HeapProfiler::GenerateHeapSnapshot(std::string &inputFilePath, std::string &outputPath)
{
    LOG_ECMA(INFO) << "ark raw heap GenerateHeapSnapshot start";
    uint64_t fileSize = GetFileSize(inputFilePath);
    if (fileSize == 0) {
        LOG_ECMA(ERROR) << "ark raw heap get file size=0";
        return false;
    }
    std::ifstream file(inputFilePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ECMA(ERROR) << "ark raw heap open file failed:" << inputFilePath.c_str();
        return false;
    }
    static uint64_t maxSupportFileSize8GB = 8 * 1024 * 1024 * 1024ULL;
    if (fileSize > maxSupportFileSize8GB) {
        LOG_ECMA(ERROR) << "ark raw heap get file size > 10GB, unsupported";
        return false;
    }
    auto buf = new char[fileSize];
    file.read(buf, fileSize);
    file.close();
    ChunkDecoder *chunk = new ChunkDecoder(buf, fileSize);
    auto &rawHeapArgs = chunk->GetRawHeapInfoArgs();
    auto &strTableIdMap = rawHeapArgs.strTableIdMapNewStr;
    auto &objInfoVec = rawHeapArgs.rawObjInfoVec;
    bool traceAllocation = false;
    DumpSnapShotOption dumpOption;
    LOG_ECMA(INFO) << "ark GenerateHeapSnapshot rebuild ref and generate nodes count=" << objInfoVec.size();
    for (auto item : strTableIdMap) {
        GetEcmaStringTable()->GetString(item.second);
    }
    auto *snapshot = new HeapSnapshot(vm_, GetEcmaStringTable(), dumpOption, traceAllocation, entryIdMap_, GetChunk());
    DecodeObj(rawHeapArgs, snapshot);
    LOG_ECMA(INFO) << "ark GenerateHeapSnapshot generate edges";
    snapshot->BuildSnapshotForBinMod(objInfoVec);
    delete[] buf;
    delete chunk;
    if (outputPath.empty()) {
        outputPath = GenDumpFileName(dumpOption.dumpFormat);
    } else if (outputPath.back() == '/') {
        outputPath += GenDumpFileName(dumpOption.dumpFormat);
    }
    LOG_GC(INFO) << "ark GenerateHeapSnapshot output file=" << outputPath.c_str();
    FileStream newStream(outputPath);
    auto serializerResult = HeapSnapshotJSONSerializer::Serialize(snapshot, &newStream);
    delete snapshot;
    LOG_ECMA(INFO) << "ark raw heap GenerateHeapSnapshot finish";
    return serializerResult;
}

[[maybe_unused]]static void WaitProcess(pid_t pid)
{
    time_t startTime = time(nullptr);
    constexpr int DUMP_TIME_OUT = 300;
    constexpr int DEFAULT_SLEEP_TIME = 100000;
    while (true) {
        int status = 0;
        pid_t p = waitpid(pid, &status, WNOHANG);
        if (p < 0 || p == pid) {
            break;
        }
        if (time(nullptr) > startTime + DUMP_TIME_OUT) {
            LOG_GC(ERROR) << "DumpHeapSnapshot kill thread, wait " << DUMP_TIME_OUT << " s";
            kill(pid, SIGTERM);
            break;
        }
        usleep(DEFAULT_SLEEP_TIME);
    }
}

template<typename Callback>
void IterateSharedHeap(Callback &cb)
{
    auto heap = SharedHeap::GetInstance();
    heap->GetOldSpace()->IterateOverObjects(cb);
    heap->GetNonMovableSpace()->IterateOverObjects(cb);
    heap->GetHugeObjectSpace()->IterateOverObjects(cb);
    heap->GetAppSpawnSpace()->IterateOverObjects(cb);
    heap->GetReadOnlySpace()->IterateOverObjects(cb);
}

std::pair<uint64_t, uint64_t> GetHeapCntAndSize(const EcmaVM *vm)
{
    uint64_t cnt = 0;
    uint64_t objectSize = 0;
    auto cb = [&objectSize, &cnt]([[maybe_unused]] TaggedObject *obj) {
        objectSize += obj->GetClass()->SizeFromJSHClass(obj);
        ++cnt;
    };
    vm->GetHeap()->IterateOverObjects(cb, false);
    return std::make_pair(cnt, objectSize);
}

std::pair<uint64_t, uint64_t> GetSharedCntAndSize()
{
    uint64_t cnt = 0;
    uint64_t size = 0;
    auto cb = [&cnt, &size](TaggedObject *obj) {
        cnt++;
        size += obj->GetClass()->SizeFromJSHClass(obj);
    };
    IterateSharedHeap(cb);
    return std::make_pair(cnt, size);
}

static CUnorderedSet<TaggedObject*> GetRootObjects(const EcmaVM *vm)
{
    CUnorderedSet<TaggedObject*> result {};
    HeapRootVisitor visitor;
    uint32_t rootCnt1 = 0;
    RootVisitor rootEdgeBuilder = [&result, &rootCnt1](
        [[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value((slot).GetTaggedType());
        if (!value.IsHeapObject()) {
            return;
        }
        ++rootCnt1;
        TaggedObject *root = value.GetTaggedObject();
        result.insert(root);
    };
    RootBaseAndDerivedVisitor rootBaseEdgeBuilder = []
        ([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base, [[maybe_unused]] ObjectSlot derived,
         [[maybe_unused]] uintptr_t baseOldObject) {
    };
    uint32_t rootCnt2 = 0;
    RootRangeVisitor rootRangeEdgeBuilder = [&result, &rootCnt2]([[maybe_unused]] Root type,
        ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value((slot).GetTaggedType());
            if (!value.IsHeapObject()) {
                continue;
            }
            ++rootCnt2;
            TaggedObject *root = value.GetTaggedObject();
            result.insert(root);
        }
    };
    visitor.VisitHeapRoots(vm->GetJSThread(), rootEdgeBuilder, rootRangeEdgeBuilder, rootBaseEdgeBuilder);
    SharedModuleManager::GetInstance()->Iterate(rootEdgeBuilder);
    Runtime::GetInstance()->IterateCachedStringRoot(rootRangeEdgeBuilder);
    return result;
}

size_t GetNotFoundObj(const EcmaVM *vm)
{
    size_t heapTotalSize = 0;
    CUnorderedSet<TaggedObject*> allHeapObjSet {};
    auto handleObj = [&allHeapObjSet, &heapTotalSize](TaggedObject *obj) {
        allHeapObjSet.insert(obj);
        uint64_t objSize = obj->GetClass()->SizeFromJSHClass(obj);
        heapTotalSize += objSize;
    };
    vm->GetHeap()->IterateOverObjects(handleObj, false);
    IterateSharedHeap(handleObj);
    LOG_ECMA(INFO) << "ark GetNotFound heap count:" << allHeapObjSet.size() << ", heap size=" << heapTotalSize;
    CUnorderedSet<TaggedObject *> notFoundObjSet {};
    auto visitor = [&notFoundObjSet, &allHeapObjSet] ([[maybe_unused]]TaggedObject *root, ObjectSlot start,
                                                      ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::RAW_DATA || area == VisitObjectArea::NATIVE_POINTER) {
            return;
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto taggedPointerAddr = reinterpret_cast<uint64_t **>(slot.SlotAddress());
            JSTaggedValue value(reinterpret_cast<TaggedObject *>(*taggedPointerAddr));
            auto originalAddr = reinterpret_cast<uint64_t>(*taggedPointerAddr);
            if (!value.IsHeapObject() || originalAddr == 0) {
                continue;
            }
            if (value.IsWeakForHeapObject()) {
                originalAddr -= 1;
            }
            if (allHeapObjSet.find(reinterpret_cast<TaggedObject *>(originalAddr)) != allHeapObjSet.end()) {
                continue;
            }
            auto obj = reinterpret_cast<TaggedObject *>(*taggedPointerAddr);
            if (notFoundObjSet.find(obj) != notFoundObjSet.end()) {
                continue;
            }
            notFoundObjSet.insert(obj);
        }
    };
    for (auto obj : allHeapObjSet) {
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, obj->GetClass(), visitor);
    }
    LOG_ECMA(INFO) << "ark GetNotFound not found count:" << notFoundObjSet.size();
    return notFoundObjSet.size();
}

bool FillAddrTable(const EcmaVM *vm, EntryIdMap &idMap, AddrTableItem *table, HeapSnapshot *snapshot)
{
    uint64_t index = 0;
    auto handleObj = [&index, &table, &idMap, &snapshot](TaggedObject *obj) {
        auto taggedType = JSTaggedValue(obj).GetRawData();
        auto [exist, id] = idMap.FindId(taggedType);
        if (!exist) {
            idMap.InsertId(taggedType, id);
        }
        table[index].addr = reinterpret_cast<uint64_t>(obj);
        table[index].id = id;
        table[index].stringId = snapshot->GenerateStringId(obj);
        index++;
    };
    vm->GetHeap()->IterateOverObjects(handleObj, false);
    IterateSharedHeap(handleObj);
    LOG_ECMA(INFO) << "ark FillAddrTable obj count: " << index;
#ifdef OHOS_UNIT_TEST
    size_t ret = GetNotFoundObj(vm);
    return ret == 0;
#else
    return true;
#endif
}

void CopyObjectMem(char *chunk, AddrTableItem *table, uint64_t len, std::atomic<uint64_t> &index,
                   std::atomic<uint64_t> &offset, uint64_t offBase)
{
    auto curIdx = index.fetch_add(1);
    while (curIdx < len) {
        auto& item = table[curIdx];
        auto obj = reinterpret_cast<TaggedObject *>(item.addr);
        uint64_t objSize = obj->GetClass()->SizeFromJSHClass(obj);
        auto curOffset = offset.fetch_add(objSize);
        item.objSize = objSize;
        item.offset = curOffset + offBase;
        auto ret = memcpy_s(chunk + curOffset, objSize, reinterpret_cast<void *>(item.addr), objSize);
        if (ret != 0) {
            LOG_ECMA(ERROR) << "ark BinaryDump CopyObjectMem memcpy_s failed";
            return;
        }
        curIdx = index.fetch_add(1);
    }
}

bool HeapProfiler::BinaryDump(Stream *stream, [[maybe_unused]] const DumpSnapShotOption &dumpOption)
{
    LOG_ECMA(INFO) << "ark BinaryDump dump raw heap start";
    auto [localCnt, heapSize] = GetHeapCntAndSize(vm_);
    auto [sharedCnt, sharedSize] = GetSharedCntAndSize();
    auto roots = GetRootObjects(vm_);
    uint64_t heapTotalCnt = localCnt + sharedCnt;
    uint64_t totalSize = sizeof(uint64_t) * (4 + roots.size()) + sizeof(AddrTableItem) * heapTotalCnt; // 4 : file head
    uint64_t heapTotalSize = heapSize + sharedSize;
    LOG_ECMA(INFO) << "ark rootNum=" << roots.size() << ", ObjSize=" << heapTotalSize << ", ObjNum=" << heapTotalCnt;
    char *chunk = new char[totalSize];
    uint64_t *header = reinterpret_cast<uint64_t *>(chunk);
    header[0] = heapTotalCnt; // 0: obj total count offset in file
    header[1] = roots.size(); // 1: root obj num offset in file
    header[2] = sharedCnt; // 2: share obj num offset in file
    auto currInd = 4; // 4 : file head num is 4, then is obj table
    for (auto root : roots) {
        header[currInd++] = reinterpret_cast<uint64_t>(root);
    }
    auto table = reinterpret_cast<AddrTableItem *>(&header[currInd]);
    DumpSnapShotOption op;
    auto *snapshotPtr = GetChunk()->New<HeapSnapshot>(vm_, GetEcmaStringTable(), op, false, entryIdMap_, GetChunk());
    auto ret = FillAddrTable(vm_, *entryIdMap_, table, snapshotPtr);
    char *heapObjData = new char[heapTotalSize];
    uint64_t objMemStart = reinterpret_cast<uint64_t>(&table[heapTotalCnt]);
    uint64_t offBase = objMemStart - reinterpret_cast<uint64_t>(chunk);
    std::atomic<uint64_t> offset(0);
    std::atomic<uint64_t> index(0);
    auto threadMain = [&offset, &index, heapObjData, table, heapTotalCnt, offBase]() {
        CopyObjectMem(heapObjData, table, heapTotalCnt, index, offset, offBase);
    };
    std::vector<std::thread> threads;
    const uint32_t THREAD_NUM = 8; // 8 : thread num is 8
    for (uint32_t i = 0; i < THREAD_NUM; i++) {
        threads.emplace_back(threadMain);
    }
    for (uint32_t i = 0; i < THREAD_NUM; i++) {
        threads[i].join();
    }
    header[3] = offBase + heapTotalSize; // 3: string table offset
    LOG_ECMA(INFO) << "ark BinaryDump write to file";
    stream->WriteBinBlock(chunk, offBase);
    stream->WriteBinBlock(heapObjData, heapTotalSize);
    delete[] heapObjData;
    delete[] chunk;
    LOG_ECMA(INFO) << "ark BinaryDump dump DumpStringTable";
    HeapSnapshotJSONSerializer::DumpStringTable(snapshotPtr, stream);
    GetChunk()->Delete(snapshotPtr);
    LOG_ECMA(INFO) << "ark BinaryDump dump raw heap finished";
    return ret;
}

void HeapProfiler::FillIdMap()
{
    EntryIdMap* newEntryIdMap = GetChunk()->New<EntryIdMap>();
    // Iterate SharedHeap Object
    SharedHeap* sHeap = SharedHeap::GetInstance();
    if (sHeap != nullptr) {
        sHeap->IterateOverObjects([newEntryIdMap, this](TaggedObject *obj) {
            JSTaggedType addr = ((JSTaggedValue)obj).GetRawData();
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            newEntryIdMap->InsertId(addr, sequenceId);
        });
        sHeap->GetReadOnlySpace()->IterateOverObjects([newEntryIdMap, this](TaggedObject *obj) {
            JSTaggedType addr = ((JSTaggedValue)obj).GetRawData();
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            newEntryIdMap->InsertId(addr, sequenceId);
        });
    }

    // Iterate LocalHeap Object
    auto heap = vm_->GetHeap();
    if (heap != nullptr) {
        heap->IterateOverObjects([newEntryIdMap, this](TaggedObject *obj) {
            JSTaggedType addr = ((JSTaggedValue)obj).GetRawData();
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            newEntryIdMap->InsertId(addr, sequenceId);
        });
    }

    // copy entryIdMap
    CUnorderedMap<JSTaggedType, NodeId>* idMap = entryIdMap_->GetIdMap();
    CUnorderedMap<JSTaggedType, NodeId>* newIdMap = newEntryIdMap->GetIdMap();
    *idMap = *newIdMap;

    GetChunk()->Delete(newEntryIdMap);
}

bool HeapProfiler::DumpHeapSnapshot(Stream *stream, const DumpSnapShotOption &dumpOption, Progress *progress)
{
    bool res = false;
    base::BlockHookScope blockScope;
    ThreadManagedScope managedScope(vm_->GetJSThread());
    pid_t pid = -1;
    {
        if (dumpOption.isFullGC) {
            [[maybe_unused]] bool heapClean = ForceFullGC(vm_);
            ForceSharedGC();
            ASSERT(heapClean);
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
        // hidumper do fork and fillmap.
        if (dumpOption.isBeforeFill) {
            FillIdMap();
        }
        // fork
        if ((pid = ForkBySyscall()) < 0) {
            LOG_ECMA(ERROR) << "DumpHeapSnapshot fork failed!";
            return false;
        }
        if (pid == 0) {
            vm_->GetAssociatedJSThread()->EnableCrossThreadExecution();
            prctl(PR_SET_NAME, reinterpret_cast<unsigned long>("dump_process"), 0, 0, 0);
            res = DoDump(stream, progress, dumpOption);
            _exit(0);
        }
    }
    if (pid != 0) {
        std::thread thread(&WaitProcess, pid);
        thread.detach();
        stream->EndOfStream();
    }
    isProfiling_ = true;
    return res;
}

bool HeapProfiler::StartHeapTracking(double timeInterval, bool isVmMode, Stream *stream,
                                     bool traceAllocation, bool newThread)
{
    vm_->CollectGarbage(TriggerGCType::OLD_GC);
    ForceSharedGC();
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
        vm_->CollectGarbage(TriggerGCType::OLD_GC);
        ForceSharedGC();
        SuspendAllScope suspendScope(vm_->GetAssociatedJSThread());
        snapshot->RecordSampleTime();
        UpdateHeapObjects(snapshot);
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
        ForceSharedGC();
        SuspendAllScope suspendScope(vm_->GetAssociatedJSThread());
        SharedHeap::GetInstance()->GetSweeper()->WaitAllTaskFinished();
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
                                                           traceAllocation, entryIdMap_, GetChunk());
            if (snapshot == nullptr) {
                LOG_FULL(FATAL) << "alloc snapshot failed";
                UNREACHABLE();
            }
            snapshot->BuildUp(dumpOption.isSimplify);
            return snapshot;
        }
        case SampleType::REAL_TIME: {
            auto *snapshot = GetChunk()->New<HeapSnapshot>(vm_, GetEcmaStringTable(), dumpOption,
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
