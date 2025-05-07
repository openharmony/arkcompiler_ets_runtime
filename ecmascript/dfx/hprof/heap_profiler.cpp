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
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/base/block_hook_scope.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/platform/backtrace.h"
#include "ecmascript/platform/file.h"

#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
#include "heap_profiler.h"
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

static uint64_t VisitMember(ObjectSlot &slot, uint64_t objAddr, CUnorderedSet<uint64_t> &notFoundObj,
                            JSHClass *jsHclass, CUnorderedMap<uint64_t, NewAddr *> &objMap)
{
    auto taggedPointerAddr = reinterpret_cast<uint64_t **>(slot.SlotAddress());
    JSTaggedValue value(reinterpret_cast<TaggedObject *>(*taggedPointerAddr));
    auto originalAddr = reinterpret_cast<uint64_t>(*taggedPointerAddr);
    originalAddr = CheckAndRemoveWeak(value, originalAddr);
    if (!value.IsHeapObject() || originalAddr == 0) {
        return 0LL;
    }
    auto toItemInfo = objMap.find(originalAddr);
    if (toItemInfo == objMap.end()) {
        LOG_ECMA(ERROR) << "ark raw heap decode visit " << std::hex << objAddr << ", type="
                        << JSHClass::DumpJSType(jsHclass->GetObjectType())
                        << ", not found member old addr=" << originalAddr;
        notFoundObj.insert(reinterpret_cast<uint64_t>(*taggedPointerAddr));
        return 0LL;
    }
    auto newAddr = reinterpret_cast<uint64_t>(toItemInfo->second->Data());
    newAddr = CheckAndAddWeak(value, newAddr);
    slot.Update(reinterpret_cast<TaggedObject *>(newAddr));
    return newAddr;
}

class VisitObjVisitor final : public EcmaObjectRangeVisitor<VisitObjVisitor> {
public:
    explicit VisitObjVisitor(CUnorderedSet<uint64_t> &notFoundObj, CUnorderedMap<uint64_t, NewAddr *> &objMap,
                             CUnorderedMap<uint64_t, CUnorderedSet<uint64_t>> &refSetMap)
        : notFoundObj_(notFoundObj), objMap_(objMap), refSetMap_(refSetMap) {}
    ~VisitObjVisitor() = default;

    void VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area)
    {
        if (area == VisitObjectArea::RAW_DATA || area == VisitObjectArea::NATIVE_POINTER) {
            return;
        }
        auto jsHclass = root->GetClass();
        auto objAddr = reinterpret_cast<uint64_t>(root);
        CUnorderedSet<uint64_t> *refSet = nullptr;
        if (refSetMap_.find(objAddr) != refSetMap_.end()) {
            refSet = &refSetMap_[objAddr];
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto newAddr = VisitMember(slot, objAddr, notFoundObj_, jsHclass, objMap_);
            if (jsHclass->IsJsGlobalEnv() && refSet != nullptr && newAddr != 0LL) {
                refSet->insert(newAddr);
            }
        }
    }
private:
    CUnorderedSet<uint64_t> &notFoundObj_;
    CUnorderedMap<uint64_t, NewAddr *> &objMap_;
    CUnorderedMap<uint64_t, CUnorderedSet<uint64_t>> &refSetMap_;
};

CUnorderedMap<uint64_t, CUnorderedSet<uint64_t>> VisitObj(CUnorderedMap<uint64_t, NewAddr *> &objMap)
{
    CUnorderedSet<uint64_t> notFoundObj;
    CUnorderedMap<uint64_t, CUnorderedSet<uint64_t>> refSetMap; // old addr map to ref set
    VisitObjVisitor visitor(notFoundObj, objMap, refSetMap);
    for (auto objInfo : objMap) {
        auto newAddr = objInfo.second->Data();
        auto jsHclassAddr = *reinterpret_cast<uint64_t *>(newAddr);
        auto jsHclassItem = objMap.find(jsHclassAddr);
        if (jsHclassItem == objMap.end()) {
            LOG_ECMA(ERROR) << "ark raw heap decode hclass not find jsHclassAddr=" << std::hex << jsHclassAddr;
            continue;
        }
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(newAddr);
        *reinterpret_cast<uint64_t *>(newAddr) = reinterpret_cast<uint64_t>(jsHclassItem->second->Data());
        auto jsHclass = reinterpret_cast<JSHClass *>(jsHclassItem->second->Data());
        if (jsHclass->IsString()) {
            continue;
        }
        if (jsHclass->IsJsGlobalEnv()) {
            refSetMap.emplace(reinterpret_cast<uint64_t>(newAddr), CUnorderedSet<uint64_t>());
        }
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
    }
    if (notFoundObj.size() > 0) {
        LOG_ECMA(ERROR) << "ark raw heap decode visit obj: not found obj num=" << notFoundObj.size();
    }
    return refSetMap;
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

bool ReadFileAtOffset(std::ifstream &file, uint32_t offset, char *buf, uint32_t size)
{
    if (buf == nullptr) {
        LOG_ECMA(ERROR) << "ark raw heap decode file buf is nullptr";
        return false;
    }
    if (!file.is_open()) {
        LOG_ECMA(ERROR) << "ark raw heap decode file not open";
        return false;
    }
    file.clear();
    if (!file.seekg(offset)) {
        LOG_ECMA(ERROR) << "ark raw heap decode file set offset failed, offset=" << offset;
        return false;
    }
    if (file.read(buf, size).fail()) {
        LOG_ECMA(ERROR) << "ark raw heap decode file read failed, offset=" << offset;
        return false;
    }
    return true;
}

CUnorderedMap<uint64_t, NewAddr *> DecodeMemObj(std::ifstream &file, CVector<uint32_t> &sections)
{
    CUnorderedMap<uint64_t, NewAddr *> objMap; // old addr map to new obj
    uint32_t heapTotalSize = 0;
    uint32_t objTotalNum = 0;
    for (uint32_t sec = 4; sec + 1 < sections.size(); sec += 2) { // 2 ：step is 2
        uint32_t offset = sections[sec];
        uint32_t secHead[2];
        if (!ReadFileAtOffset(file, offset, reinterpret_cast<char *>(secHead), sizeof(secHead))) {
            LOG_ECMA(ERROR) << "ark raw heap decode read obj section failed, sec=" << sec << ", offset="
                            << offset << ", size=" << sections[sec + 1];
            return objMap;
        }
        LOG_ECMA(INFO) << "ark raw heap decode read obj section failed, sec=" << sec << ", offset=" << offset
                        << ", size=" << sections[sec + 1] << ", obj num=" << secHead[0];
        auto tbSize = secHead[0] * sizeof(AddrTableItem);
        if (secHead[1] != sizeof(AddrTableItem) || tbSize == 0 || tbSize > MAX_OBJ_SIZE) {
            LOG_ECMA(ERROR) << "ark raw heap decode check obj table section=" << sections[sec] << ", head size="
                            << sizeof(AddrTableItem) << ", but=" << secHead[1] << "or error table size=" << tbSize;
            continue;
        }
        CVector<char> objTabBuf(tbSize);
        file.read(objTabBuf.data(), tbSize);
        auto objTab = reinterpret_cast<AddrTableItem *>(objTabBuf.data());
        offset += sizeof(secHead);
        objTotalNum += secHead[0];
        for (uint32_t i = 0; i < secHead[0]; i++) {
            heapTotalSize += objTab[i].objSize;
            auto actSize = i + 1 < secHead[0] ? objTab[i + 1].offset - objTab[i].offset :
                           sections[sec + 1] - objTab[i].offset - sizeof(secHead);
            if (actSize != objTab[i].objSize && actSize != sizeof(uint64_t)) {
                auto tabOffset = offset + i * sizeof(AddrTableItem);
                LOG_ECMA(ERROR) << "ark raw heap decode check obj size i=" << i << std::hex << ", offset=" << tabOffset
                                << ", addr=" << objTab[i].addr << ", size=" << objTab[i].objSize << " but=" << actSize;
                continue;
            }
            objMap.emplace(objTab[i].addr, new NewAddr(actSize, objTab[i].objSize));
            auto result = ReadFileAtOffset(file, offset + objTab[i].offset, objMap[objTab[i].addr]->Data(), actSize);
            if (!result) {
                LOG_ECMA(ERROR) << "ark raw heap decode read failed, i=" << i << ", base offset=" << offset
                                << ", obj addr=" << objTab[i].addr << ", read size=" << actSize;
                return objMap;
            }
        }
    }
    LOG_ECMA(INFO) << "ark raw heap decode read obj, num=" << objTotalNum << ", size=" << heapTotalSize;
    return objMap;
}

CUnorderedMap<uint64_t, CString *> DecodeStrTable(StringHashMap *strTable, std::ifstream &file,
                                                  uint32_t offset, uint32_t secSize)
{
    uint32_t secHead[2];
    if (!ReadFileAtOffset(file, offset, reinterpret_cast<char *>(secHead), sizeof(secHead))) {
        LOG_ECMA(ERROR) << "ark raw heap decode read str table failed, offset=" << offset << ", size=" << secSize;
        return CUnorderedMap<uint64_t, CString *>(0);
    }
    uint32_t byteNum = secSize - sizeof(secHead);
    char *charPtr = new char[byteNum];
    file.read(charPtr, byteNum);
    CUnorderedMap<uint64_t, CString *> strTabMap; // old addr map to str id
    uint32_t cnt = 0;
    uint32_t baseOff = 0;
    while (cnt++ < secHead[0]) {
        uint32_t *u32Ptr = reinterpret_cast<uint32_t *>(charPtr + baseOff);
        auto strOffset = (u32Ptr[1] + 1) * sizeof(uint64_t) + baseOff;
        auto getSize = strlen(charPtr + strOffset);
        if (u32Ptr[0] != getSize) {
            LOG_ECMA(ERROR) << cnt << " ark raw heap decode check str size=" << u32Ptr[0] << ", but=" << getSize<<"\n";
        }
        auto strAddr = strTable->GetString(charPtr + strOffset);
        uint32_t num = 0;
        uint64_t *u64Ptr = reinterpret_cast<uint64_t *>(&u32Ptr[2]);
        while (num < u32Ptr[1]) {
            strTabMap[u64Ptr[num]] = strAddr;
            num++;
        }
        baseOff = strOffset + u32Ptr[0] + 1;
    }
    delete[] charPtr;
    LOG_ECMA(INFO) << "ark raw heap decode string table size=" << strTable->GetCapcity();
    return strTabMap;
}

CUnorderedSet<uint64_t> DecodeRootTable(std::ifstream &file, uint32_t offset, uint32_t secSize)
{
    uint32_t secHead[2];
    if (!ReadFileAtOffset(file, offset, reinterpret_cast<char *>(secHead), sizeof(secHead))) {
        LOG_ECMA(ERROR) << "ark raw heap decode read root table failed, offset=" << offset << ", size=" << secSize;
        return CUnorderedSet<uint64_t>(0);
    }
    if (secHead[1] != sizeof(uint64_t)) {
        LOG_ECMA(ERROR) << "ark raw heap decode error root size, need=" << sizeof(uint32_t) << ", but=" << secHead[0];
        return CUnorderedSet<uint64_t>(0);
    }
    auto checkSize = sizeof(uint64_t) * secHead[0] + sizeof(secHead);
    if (secSize != checkSize) {
        LOG_ECMA(ERROR) << "ark raw heap decode check root section size=" << secSize << ", but=" << checkSize;
        return CUnorderedSet<uint64_t>(0);
    }
    CVector<uint64_t> rootVec(secHead[0]);
    file.read(reinterpret_cast<char *>(rootVec.data()), sizeof(uint64_t) * secHead[0]);
    CUnorderedSet<uint64_t> rootSet;
    for (auto addr : rootVec) {
        rootSet.insert(addr);
    }
    LOG_ECMA(INFO) << "ark raw heap decode root obj num=" << rootSet.size();
    return rootSet;
}

CVector<uint32_t> GetSectionInfo(std::ifstream &file, uint64_t fileSize)
{
    uint32_t secHead[2];
    uint32_t fileOffset = fileSize - sizeof(uint32_t) * 2; // 2 : last 2 uint32
    file.seekg(fileOffset);
    file.read(reinterpret_cast<char *>(secHead), sizeof(secHead));
    if (secHead[1] != sizeof(uint32_t)) {
        LOG_ECMA(ERROR) << "ark raw heap decode unexpect head, need=" << sizeof(uint32_t) << ", but=" << secHead[0];
        return CVector<uint32_t>(0);
    }
    CVector<uint32_t> secInfo(secHead[0]); // last 4 byte is section num
    auto secInfoSize = secHead[0] * secHead[1];
    fileOffset -= secInfoSize;
    file.seekg(fileOffset);
    file.read(reinterpret_cast<char *>(secInfo.data()), secInfoSize);
    return secInfo;
}

void ClearObjMem(CUnorderedMap<uint64_t, NewAddr *> &objMap)
{
    for (auto objItem : objMap) {
        delete objItem.second;
    }
    objMap.clear();
}

static bool GetValidFileSize(std::string &inputFilePath, uint64_t &fileSize)
{
    fileSize = GetFileSize(inputFilePath);
    if (fileSize == 0) {
        LOG_ECMA(ERROR) << "ark raw heap decode get file size=0";
        return false;
    }
    if (fileSize > MAX_FILE_SIZE) {
        LOG_ECMA(ERROR) << "ark raw heap decode get file size > 4GB, unsupported";
        return false;
    }
    return true;
}

bool HeapProfiler::GenerateHeapSnapshot(std::string &inputFilePath, std::string &outputPath)
{
    LOG_ECMA(INFO) << "ark raw heap decode start target=" << outputPath;
    std::string realPath;
    if (!RealPath(inputFilePath, realPath)) {
        LOG_ECMA(ERROR) << "get real path failed:" << inputFilePath;
        return false;
    }
    uint64_t fileSize;
    if (!GetValidFileSize(realPath, fileSize)) {
        return false;
    }
    std::ifstream file(realPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ECMA(ERROR) << "ark raw heap decode file failed:" << realPath;
        return false;
    }
    CVector<uint32_t> sections = GetSectionInfo(file, fileSize);
    if (sections.size() == 0) {
        LOG_ECMA(ERROR) << "ark raw heap decode not found section data";
        return false;
    }
    auto objMap = DecodeMemObj(file, sections);
    auto refSetMap = VisitObj(objMap);
    auto rootSet = DecodeRootTable(file, sections[0], sections[1]);
    auto strTabMap = DecodeStrTable(GetEcmaStringTable(), file, sections[2], sections[3]);
    file.close();
    DumpSnapShotOption dp;
    auto *snapshot = new HeapSnapshot(vm_, GetEcmaStringTable(), dp, false, entryIdMap_);
    LOG_ECMA(INFO) << "ark raw heap decode generate nodes=" << objMap.size();
    snapshot->GenerateNodeForBinMod(objMap, rootSet, strTabMap);
    rootSet.clear();
    strTabMap.clear();
    LOG_ECMA(INFO) << "ark raw heap decode fill edges=" << objMap.size();
    snapshot->BuildSnapshotForBinMod(objMap, refSetMap);
    refSetMap.clear();
    ClearObjMem(objMap);
    if (outputPath.empty()) {
        outputPath = GenDumpFileName(dp.dumpFormat);
    } else if (outputPath.back() == '/') {
        outputPath += GenDumpFileName(dp.dumpFormat);
    }
    LOG_ECMA(INFO) << "ark raw heap decode serialize file=" << outputPath.c_str();
    FileStream newStream(outputPath);
    auto serializerResult = HeapSnapshotJSONSerializer::Serialize(snapshot, &newStream);
    delete snapshot;
    LOG_ECMA(INFO) << "ark raw heap decode finish";
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

template<typename Callback>
void IterateSharedHeap(Callback &cb)
{
    auto heap = SharedHeap::GetInstance();
    heap->GetOldSpace()->IterateOverObjects(cb);
    heap->GetCompressSpace()->IterateOverObjects(cb);
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

    class EdgeBuilderRootVisitor final : public RootVisitor {
    public:
        explicit EdgeBuilderRootVisitor(CUnorderedSet<TaggedObject*> &result) : result_(result) {}
        ~EdgeBuilderRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override
        {
            JSTaggedValue value((slot).GetTaggedType());
            if (!value.IsHeapObject()) {
                return;
            }
            TaggedObject *root = value.GetTaggedObject();
            result_.insert(root);
        }

        void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
        {
            for (ObjectSlot slot = start; slot < end; slot++) {
                JSTaggedValue value((slot).GetTaggedType());
                if (!value.IsHeapObject()) {
                    continue;
                }
                TaggedObject *root = value.GetTaggedObject();
                result_.insert(root);
            }
        }

        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
            [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}
    private:
        CUnorderedSet<TaggedObject*> &result_;
    };
    EdgeBuilderRootVisitor edgeBuilderRootVisitor(result);

    visitor.VisitHeapRoots(vm->GetJSThread(), edgeBuilderRootVisitor);
    SharedModuleManager::GetInstance()->Iterate(edgeBuilderRootVisitor);
    Runtime::GetInstance()->IterateCachedStringRoot(edgeBuilderRootVisitor);
    return result;
}

class GetNotFoundObjVisitor final : public EcmaObjectRangeVisitor<GetNotFoundObjVisitor> {
public:
    explicit GetNotFoundObjVisitor(CUnorderedSet<TaggedObject *> &notFoundObjSet,
                                   CUnorderedSet<TaggedObject*> &allHeapObjSet)
        : notFoundObjSet_(notFoundObjSet), allHeapObjSet_(allHeapObjSet) {}
    ~GetNotFoundObjVisitor() = default;

    void VisitObjectRangeImpl([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end,
                              VisitObjectArea area)
    {
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
            if (allHeapObjSet_.find(reinterpret_cast<TaggedObject *>(originalAddr)) != allHeapObjSet_.end()) {
                continue;
            }
            auto obj = reinterpret_cast<TaggedObject *>(*taggedPointerAddr);
            if (notFoundObjSet_.find(obj) != notFoundObjSet_.end()) {
                continue;
            }
            notFoundObjSet_.insert(obj);
        }
    }
private:
    CUnorderedSet<TaggedObject *> &notFoundObjSet_;
    CUnorderedSet<TaggedObject*> &allHeapObjSet_;
};

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
    vm->GetHeap()->GetCompressSpace()->IterateOverObjects(handleObj);
    IterateSharedHeap(handleObj);
    LOG_ECMA(INFO) << "ark raw heap dump GetNotFound heap count:" << allHeapObjSet.size()
                   << ", heap size=" << heapTotalSize;
    CUnorderedSet<TaggedObject *> notFoundObjSet {};
    GetNotFoundObjVisitor visitor(notFoundObjSet, allHeapObjSet);
    for (auto obj : allHeapObjSet) {
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, obj->GetClass(), visitor);
    }
    LOG_ECMA(INFO) << "ark raw heap dump GetNotFound not found count:" << notFoundObjSet.size();
    return notFoundObjSet.size();
}

uint32_t HeapProfiler::CopyObjectMem2Buf(char *objTable, uint32_t objNum,
                                         CVector<std::pair<char *, uint32_t>> &memBufMap)
{
    char *currMemBuf = nullptr;
    uint32_t currSize = 0;
    uint32_t totalSize = 0;
    uint32_t curOffset = objNum * sizeof(AddrTableItem);
    auto objHeaders = reinterpret_cast<AddrTableItem *>(objTable);
    for (uint32_t j = 0; j < objNum; ++j) {
        auto obj = reinterpret_cast<TaggedObject *>(objHeaders[j].addr);
        JSTaggedValue value(obj);
        uint64_t objSize = obj->GetClass()->SizeFromJSHClass(obj);
        totalSize += objSize;
        if (currSize + objSize > PER_GROUP_MEM_SIZE || currMemBuf == nullptr) {
            if (currMemBuf != nullptr) {
                memBufMap.push_back({currMemBuf, currSize});
            }
            currSize = 0;
            currMemBuf = chunk_.NewArray<char>(objSize > PER_GROUP_MEM_SIZE? objSize : PER_GROUP_MEM_SIZE);
        }
        objHeaders[j].objSize = objSize;
        objHeaders[j].offset = curOffset;
        int32_t ret;
        if (value.IsString()) {
            CVector<uint64_t> strTmp(objSize / sizeof(uint64_t), 0);
            strTmp[0] = *reinterpret_cast<uint64_t *>(objHeaders[j].addr);
            ret = memcpy_s(currMemBuf + currSize, objSize, reinterpret_cast<void *>(strTmp.data()), objSize);
        } else {
            ret = memcpy_s(currMemBuf + currSize, objSize, reinterpret_cast<void *>(objHeaders[j].addr), objSize);
        }
        if (ret != 0) {
            LOG_ECMA(ERROR) << "ark raw heap dump CopyObjectMem memcpy_s failed, currSize="
                            << currSize << ",objSize=" << objSize << ",addr=" << objHeaders[j].addr;
            return totalSize;
        }
        curOffset += objSize;
        currSize += objSize;
    }
    if (currSize > 0) {
        memBufMap.push_back({currMemBuf, currSize});
    } else if (currMemBuf != nullptr) {
        chunk_.Delete<char>(currMemBuf);
    }
    return totalSize;
}

uint32_t HeapProfiler::GenObjTable(CUnorderedMap<char *, uint32_t> &headerMap, HeapSnapshot *snapshot,
                                   CUnorderedMap<uint64_t, CVector<uint64_t>> &strIdMap)
{
    char *currBuf = chunk_.NewArray<char>(PER_GROUP_MEM_SIZE);
    uint32_t index = 0;
    uint32_t objNum = 0;
    auto table = reinterpret_cast<AddrTableItem *>(currBuf);
    auto handleObj = [&index, &table, &objNum, &headerMap, &currBuf, &snapshot, &strIdMap, this](TaggedObject *obj) {
        JSTaggedValue value(obj);
        auto taggedType = value.GetRawData();
        auto [exist, id] = entryIdMap_->FindId(taggedType);
        if (!exist) {
            entryIdMap_->InsertId(taggedType, id);
        }
        table[index].addr = reinterpret_cast<uint64_t>(obj);
        table[index].id = id;
        auto strId = snapshot->GenerateStringId(obj);
        if (strId != 1) { // 1 : invalid str id
            if (strIdMap.find(strId) == strIdMap.end()) {
                strIdMap.emplace(strId, CVector<uint64_t>());
            }
            strIdMap[strId].push_back(table[index].addr);
        }
        index++;
        if (index == HEAD_NUM_PER_GROUP) {
            headerMap.emplace(currBuf, index);
            objNum += HEAD_NUM_PER_GROUP;
            index = 0;
            currBuf = chunk_.NewArray<char>(PER_GROUP_MEM_SIZE);
            table = reinterpret_cast<AddrTableItem *>(currBuf);
        }
    };
    vm_->GetHeap()->IterateOverObjects(handleObj, false);
    vm_->GetHeap()->GetCompressSpace()->IterateOverObjects(handleObj);
    IterateSharedHeap(handleObj);
    objNum += index;
    if (index != 0) {
        headerMap.emplace(currBuf, index);
    } else {
        chunk_.Delete<char>(currBuf);
    }
    return objNum;
}

// 4 byte: root_num
// 4 byte: unit size = sizeof(addr), 8 byte here
// {8 byte: root obj addr} * root_num
uint32_t HeapProfiler::GenRootTable(Stream *stream)
{
    auto roots = GetRootObjects(vm_);
    uint32_t rootSecHeadSize = 8; // 8 : root num 、 unit size
    auto rootSecSize = roots.size() * (sizeof(TaggedObject *)) + rootSecHeadSize;
    auto memBuf = chunk_.NewArray<char>(rootSecSize);
    uint32_t *rootHeader = reinterpret_cast<uint32_t *>(memBuf);
    uint64_t *rootBuf = reinterpret_cast<uint64_t *>(memBuf + rootSecHeadSize); // 8 : root addr start offset
    rootHeader[0] = roots.size(); // 0: root num
    rootHeader[1] = sizeof(TaggedObject *); // 1: unit size
    auto currInd = 0;
    for (auto root : roots) {
        rootBuf[currInd++] = reinterpret_cast<uint64_t>(root);
    }
    LOG_ECMA(INFO) << "ark raw heap dump GenRootTable root cnt="<<roots.size();
    stream->WriteBinBlock(memBuf, rootSecSize);
    chunk_.Delete<char>(memBuf);
    return rootSecSize;
}


// 4 byte: obj_num
// 4 byte: unit size = sizeof(AddrTableItem)
// {AddrTableItem} * obj_num
// {obj contents} * obj_num
uint32_t HeapProfiler::WriteToBinFile(Stream *stream, char *objTab, uint32_t objNum,
                                      CVector<std::pair<char *, uint32_t>> &memBuf)
{
    uint32_t secHeader[] = {objNum, sizeof(AddrTableItem)};
    uint32_t secTotalSize = sizeof(secHeader);
    stream->WriteBinBlock(reinterpret_cast<char *>(secHeader), secTotalSize);
    uint32_t headerSize = objNum * sizeof(AddrTableItem);
    secTotalSize += headerSize;
    stream->WriteBinBlock(objTab, headerSize); // write obj header
    chunk_.Delete<char>(objTab);
    for (auto memItem : memBuf) {
        stream->WriteBinBlock(memItem.first, memItem.second);
        secTotalSize += memItem.second;
        chunk_.Delete<char>(memItem.first);
    }
    return secTotalSize;
}

bool HeapProfiler::DumpRawHeap(Stream *stream, uint32_t &fileOffset, CVector<uint32_t> &secIndexVec)
{
    CUnorderedMap<char *, uint32_t> objTabMap; // buf map table num
    CUnorderedMap<uint64_t, CVector<uint64_t>> strIdMapObjVec; // string id map to objs vector
    DumpSnapShotOption op;
    auto snapshot = GetChunk()->New<HeapSnapshot>(vm_, GetEcmaStringTable(), op, false, entryIdMap_);
    uint32_t objTotalNum = GenObjTable(objTabMap, snapshot, strIdMapObjVec);
    LOG_ECMA(INFO) << "ark raw heap dump DumpRawHeap totalObjNumber=" << objTotalNum;
    CVector<CVector<std::pair<char *, uint32_t>>> allMemBuf(objTabMap.size(), CVector<std::pair<char *, uint32_t>>());
    CVector<std::thread> threadsVec;
    CVector<char *> objTabVec(objTabMap.size());
    uint32_t index = 0;
    LOG_ECMA(INFO) << "ark raw heap dump DumpRawHeap start to copy, thread num=" << objTabMap.size();
    for (auto tableItem : objTabMap) {
        auto tdCb = [this, &tableItem, &allMemBuf, &index] () {
            CopyObjectMem2Buf(tableItem.first, tableItem.second, allMemBuf[index]);
        };
        threadsVec.emplace_back(tdCb);
        objTabVec[index] = tableItem.first;
        threadsVec[index].join();
        ++index;
    }
    LOG_ECMA(INFO) << "ark raw heap dump DumpRawHeap write string, num=" << strIdMapObjVec.size();
    secIndexVec.push_back(fileOffset); // string table section offset
    auto size = HeapSnapshotJSONSerializer::DumpStringTable(GetEcmaStringTable(), stream, strIdMapObjVec);
    secIndexVec.push_back(size); // string table section size
    GetChunk()->Delete(snapshot);
    fileOffset += size;
    strIdMapObjVec.clear();
    uint32_t finCnt = 0;
    LOG_ECMA(INFO) << "ark raw heap dump DumpRawHeap write obj, offset=" << fileOffset;
    while (finCnt < threadsVec.size()) {
        for (index = 0; index < threadsVec.size(); ++index) {
            if (threadsVec[index].joinable()) { // thread not finished
                continue;
            }
            ++finCnt;
            secIndexVec.push_back(fileOffset); // current section offset
            auto objNum = objTabMap[objTabVec[index]];
            auto currSecSize = WriteToBinFile(stream, objTabVec[index], objNum, allMemBuf[index]);
            LOG_ECMA(INFO) << "ark raw heap dump DumpRawHeap write offset=" << fileOffset << ", size=" << currSecSize;
            secIndexVec.push_back(currSecSize); // current section size
            fileOffset += currSecSize;
        }
    }
    return true;
}

//  * 8 byte: version id
//  * root table section
//  * string table section
//  * {heap section / share heap section} * thread_num
//  * 4 byte: root table section offset
//  * 4 byte: root table section size
//  * 4 byte: string table section offset
//  * 4 byte: string table section size
//  * {
//  * 4 byte: obj section offset
//  * 4 byte: obj section size
//  * } * thread_num
//  * 4 byte: section_offset_num size, 4 byte here
//  * 4 byte: section_num
bool HeapProfiler::BinaryDump(Stream *stream, const DumpSnapShotOption &dumpOption)
{
    [[maybe_unused]] EcmaHandleScope ecmaHandleScope(vm_->GetJSThread());
    DumpSnapShotOption option;
    auto stringTable = chunk_.New<StringHashMap>(vm_);
    auto snapshot = chunk_.New<HeapSnapshot>(vm_, stringTable, option, false, entryIdMap_);
    RawHeapDump rawHeapDump(vm_, stream, snapshot, entryIdMap_);
    rawHeapDump.BinaryDump(dumpOption);
    chunk_.Delete<StringHashMap>(stringTable);
    chunk_.Delete<HeapSnapshot>(snapshot);
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

void RawHeapDump::BinaryDump(const DumpSnapShotOption &dumpOption)
{
    ClearVisitMark();
    DumpVersion();
    DumpRootTable();
    DumpStringTable();
    DumpObjectTable(dumpOption);
    DumpObjectMemory();
    DumpSectionIndex();
    WriteBinBlock();
    EndVisitMark();
}

void RawHeapDump::VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area)
{
    if (area == VisitObjectArea::RAW_DATA || area == VisitObjectArea::NATIVE_POINTER) {
        return;
    }
    auto hclass = reinterpret_cast<TaggedObject *>(root->GetClass());
    if (MarkObject(hclass)) {
        bfsQueue_.emplace(hclass);
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto value = slot.GetTaggedValue();
        if (!value.IsHeapObject()) {
            continue;
        }
        auto obj = value.GetWeakReferentUnChecked();
        if (MarkObject(obj)) {
            bfsQueue_.emplace(obj);
        }
    }
}

void RawHeapDump::DumpVersion()
{
    WriteChunk(const_cast<char *>(versionID), VERSION_ID_SIZE);
    LOG_ECMA(INFO) << "rawheap dump, version " << std::string(versionID);
}

void RawHeapDump::DumpRootTable()
{
    HeapRootVisitor rootVisitor;
    rootVisitor.VisitHeapRoots(vm_->GetJSThread(), *this);
    SharedModuleManager::GetInstance()->Iterate(*this);
    Runtime::GetInstance()->IterateCachedStringRoot(*this);

    secIndexVec_.push_back(fileOffset_);
    uint32_t rootObjCnt = roots_.size();
    uint32_t rootTableHeader[2] = {rootObjCnt, sizeof(TaggedObject *)};
    WriteChunk(reinterpret_cast<char *>(rootTableHeader), sizeof(rootTableHeader));
    for (auto &root : roots_) {
        uint64_t addr = reinterpret_cast<uint64_t>(root);
        WriteU64(addr);
        ProcessMarkObjectsFromRoot(root);
    }
    secIndexVec_.push_back(sizeof(TaggedObject *) * rootObjCnt + sizeof(rootTableHeader));
    LOG_ECMA(INFO) << "rawheap dump, root count " << rootObjCnt;
}

void RawHeapDump::DumpObjectTable(const DumpSnapShotOption &dumpOption)
{
    secIndexVec_.push_back(fileOffset_);
    objCnt_ += readOnlyObjects_.size();
    uint32_t header[2] = {objCnt_, sizeof(AddrTableItem)};
    WriteChunk(reinterpret_cast<char *>(header), sizeof(header));

    uint32_t offset = header[0] * header[1];
    auto objTableDump = [&offset, &dumpOption, this](void *addr) {
        auto obj = reinterpret_cast<TaggedObject *>(addr);
        AddrTableItem table;
        table.addr = reinterpret_cast<uint64_t>(obj);
        table.id = dumpOption.isDumpOOM ?
            entryIdMap_->GetNextId() : entryIdMap_->FindOrInsertNodeId(reinterpret_cast<JSTaggedType>(addr));
        table.objSize = obj->GetClass()->SizeFromJSHClass(obj);
        table.offset = offset;
        WriteChunk(reinterpret_cast<char *>(&table), sizeof(AddrTableItem));
        if (obj->GetClass()->IsString()) {
            offset += sizeof(JSHClass *);
        } else {
            offset += table.objSize;
        }
    };
    IterateMarkedObjects(objTableDump);
    LOG_ECMA(INFO) << "rawheap dump, object total count " << objCnt_;
}

void RawHeapDump::DumpObjectMemory()
{
    uint32_t startOffset = static_cast<uint32_t>(fileOffset_);
    auto objMemDump = [this](void *addr) {
        auto obj = reinterpret_cast<TaggedObject *>(addr);
        size_t size = obj->GetClass()->SizeFromJSHClass(obj);
        if (obj->GetClass()->IsString()) {
            size = sizeof(JSHClass *);
        }
        WriteChunk(reinterpret_cast<char *>(obj), size);
    };
    IterateMarkedObjects(objMemDump);
    // 2: means headers
    secIndexVec_.push_back(fileOffset_ - startOffset + objCnt_ * sizeof(AddrTableItem) + sizeof(uint32_t) * 2);
    LOG_ECMA(INFO) << "rawheap dump, object memory " << fileOffset_ - startOffset;
}

void RawHeapDump::DumpStringTable()
{
    for (auto obj : readOnlyObjects_) {
        UpdateStringTable(obj);
    }
    WriteBinBlock();
    secIndexVec_.push_back(fileOffset_);
    auto size = HeapSnapshotJSONSerializer::DumpStringTable(snapshot_->GetEcmaStringTable(), stream_, strIdMapObjVec_);
    fileOffset_ += size;
    secIndexVec_.push_back(size);
    auto capcity = snapshot_->GetEcmaStringTable()->GetCapcity();
    LOG_ECMA(INFO) << "rawheap dump, string table capcity " << capcity << ", size " << size;
}

void RawHeapDump::DumpSectionIndex()
{
    secIndexVec_.push_back(secIndexVec_.size());
    secIndexVec_.push_back(sizeof(uint32_t));
    WriteChunk(reinterpret_cast<char *>(secIndexVec_.data()), secIndexVec_.size() * sizeof(uint32_t));
    LOG_ECMA(INFO) << "rawheap dump, section count " << secIndexVec_.size();
}

void RawHeapDump::WriteU64(uint64_t number)
{
    if (PER_GROUP_MEM_SIZE - bufIndex_ < sizeof(uint64_t)) {
        stream_->WriteBinBlock(buffer_, bufIndex_);
        bufIndex_ = 0;
    }
    *reinterpret_cast<uint64_t *>(buffer_ + bufIndex_) = number;
    bufIndex_ += sizeof(uint64_t);
    fileOffset_ += sizeof(uint64_t);
}

void RawHeapDump::WriteChunk(char *data, size_t size)
{
    while (size > 0) {
        MaybeWriteBuffer();
        size_t remainderSize = PER_GROUP_MEM_SIZE - bufIndex_;
        size_t writeSize = size < remainderSize ? size : remainderSize;
        if (writeSize <= 0) {
            break;
        }
        if (memcpy_s(buffer_ + bufIndex_, remainderSize, data, writeSize) != 0) {
            LOG_ECMA(ERROR) << "rawheap dump, WriteChunk failed, write size " <<
                               writeSize << ", remainder size " << remainderSize;
            break;
        }
        data += writeSize;
        size -= writeSize;
        bufIndex_ += writeSize;
        fileOffset_ += writeSize;
    }
}

void RawHeapDump::MaybeWriteBuffer()
{
    if (UNLIKELY(bufIndex_ == PER_GROUP_MEM_SIZE)) {
        WriteBinBlock();
    }
}

void RawHeapDump::WriteBinBlock()
{
    if (bufIndex_ <= 0) {
        return;
    }
    if (!stream_->WriteBinBlock(buffer_, bufIndex_)) {
        LOG_ECMA(ERROR) << "rawheap dump, WriteBinBlock failed, write size " << bufIndex_;
    }
    bufIndex_ = 0;
}


void RawHeapDump::UpdateStringTable(TaggedObject *object)
{
    StringId strId = snapshot_->GenerateStringId(object);
    if (strId == 1) {  // 1 : invalid str id
        return;
    }
    auto vec = strIdMapObjVec_.find(strId);
    if (vec != strIdMapObjVec_.end()) {
        vec->second.push_back(reinterpret_cast<uint64_t>(object));
    } else {
        CVector<uint64_t> objVec;
        objVec.push_back(reinterpret_cast<uint64_t>(object));
        strIdMapObjVec_.emplace(strId, objVec);
    }
}

void RawHeapDump::HandleRootValue(JSTaggedValue value)
{
    if (!value.IsHeapObject()) {
        return;
    }
    TaggedObject *root = value.GetWeakReferentUnChecked();
    roots_.insert(root);
}

void RawHeapDump::IterateMarkedObjects(const std::function<void(void *)> &visitor)
{
    auto cb = [&visitor](Region *region) {
        region->IterateAllMarkedBits(visitor);
    };
    vm_->GetHeap()->EnumerateRegions(cb);
    SharedHeap::GetInstance()->EnumerateOldSpaceRegions(cb);

    for (auto obj : readOnlyObjects_) {
        visitor(obj);
    }
}

void RawHeapDump::ProcessMarkObjectsFromRoot(TaggedObject *root)
{
    if (!MarkObject(root)) {
        return;
    }
    bfsQueue_.emplace(root);
    while (!bfsQueue_.empty()) {
        auto object = bfsQueue_.front();
        bfsQueue_.pop();
        if (object->GetClass()->IsString()) {
            MarkObject(reinterpret_cast<TaggedObject*>(object->GetClass()));
            continue;
        }
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, object->GetClass(), *this);
    }
}

bool RawHeapDump::MarkObject(TaggedObject *object)
{
    Region *region = Region::ObjectAddressToRange(object);
    if (region->InReadOnlySpace() || region->InSharedReadOnlySpace()) {
        readOnlyObjects_.insert(object);
        return false;
    }
    if (!region->NonAtomicMark(object)) {
        return false;
    }
    UpdateStringTable(object);
    ++objCnt_;
    return true;
}

void RawHeapDump::ClearVisitMark()
{
    auto cb = [](Region *region) {
        region->ClearMarkGCBitset();
    };
    vm_->GetHeap()->EnumerateRegions(cb);
    SharedHeap::GetInstance()->EnumerateOldSpaceRegions(cb);
}

void RawHeapDump::EndVisitMark()
{
    auto cb = [](Region *region) {
        if (region->InAppSpawnSpace() || region->InSharedAppSpawnSpace()) {
            return;
        }
        region->ClearMarkGCBitset();
    };
    vm_->GetHeap()->EnumerateRegions(cb);
    SharedHeap::GetInstance()->EnumerateOldSpaceRegions(cb);
}
}  // namespace panda::ecmascript
