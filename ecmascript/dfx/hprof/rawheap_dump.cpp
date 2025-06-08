/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/rawheap_dump.h"

namespace panda::ecmascript {
void RootMarker::VisitRoot([[maybe_unused]]Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        Mark(slot.GetTaggedType());
    }
}

void RootMarker::VisitRangeRoot(Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        VisitRoot(type, slot);
    }
}

void ObjectMarker::VisitObjectRangeImpl([[maybe_unused]]BaseObject *root, uintptr_t start,
                                        uintptr_t endAddr, [[maybe_unused]]VisitObjectArea area)
{
    ObjectSlot end(endAddr);
    for (ObjectSlot slot(start); slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject() && !value.IsWeak() && Mark(slot.GetTaggedType())) {
            Push(slot.GetTaggedType());
        }
    }
}

void ObjectMarker::ProcessMarkObjectsFromRoot(JSTaggedType root)
{
    if (!Mark(root)) {
        return;
    }

    Push(root);
    while (!IsEmpty()) {
        JSTaggedValue value(PopUnchecked());
        TaggedObject *object = value.GetTaggedObject();
        JSHClass *hclass = object->GetClass();
        if (Mark(reinterpret_cast<JSTaggedType>(hclass))) {
            Push(reinterpret_cast<JSTaggedType>(hclass));
        }
        if (hclass->IsString()) {
            continue;
        }
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, hclass, *this);
    }
}

RawHeapDump::RawHeapDump(Stream *stream, HeapSnapshot *snapshot,
                         EntryIdMap *entryIdMap, const DumpSnapShotOption &dumpOption)
    : writer_(stream), snapshot_(snapshot), entryIdMap_(entryIdMap)
{
    isOOM_ = dumpOption.isDumpOOM;
    startTime_ = std::chrono::steady_clock::now();
}

RawHeapDump::~RawHeapDump()
{
    writer_.EndOfWriteBinBlock();
    secIndexVec_.clear();
    auto endTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime_).count();
    LOG_ECMA(INFO) << "rawheap dump success, cost " << duration << "s, " << "file size " << fileOffset_;
}

void RawHeapDump::DumpVersion(const std::string &version)
{
    char versionId[8];  // 8: means the size of rawheap version
    if (strcpy_s(versionId, sizeof(versionId), version.c_str()) != 0) {
        LOG_ECMA(ERROR) << "rawheap dump, version id strcpy_s failed!";
        return;
    }
    WriteChunk(versionId, sizeof(versionId));

    char timeStamp[8];  // 8: means the size of current timestamp
    *reinterpret_cast<uint64_t *>(timeStamp) = std::chrono::system_clock::now().time_since_epoch().count();
    WriteChunk(timeStamp, sizeof(timeStamp));
    LOG_ECMA(INFO) << "rawheap dump, version " << version;
}

void RawHeapDump::DumpSectionIndex()
{
    AddSectionRecord(secIndexVec_.size());
    AddSectionRecord(sizeof(uint32_t));
    WriteChunk(reinterpret_cast<char *>(secIndexVec_.data()), secIndexVec_.size() * sizeof(uint32_t));
    LOG_ECMA(INFO) << "rawheap dump, section count " << secIndexVec_.size();
}

NodeId RawHeapDump::GenerateNodeId(JSTaggedType addr)
{
    return isOOM_ ? entryIdMap_->GetNextId() : entryIdMap_->FindOrInsertNodeId(addr);
}

void RawHeapDump::WriteChunk(char *data, size_t size)
{
    writer_.WriteBinBlock(data, size);
    fileOffset_ += size;
}

void RawHeapDump::WriteU64(uint64_t value)
{
    char data[sizeof(uint64_t)];
    *reinterpret_cast<uint64_t *>(data) = value;
    WriteChunk(data, sizeof(data));
}

void RawHeapDump::WriteU32(uint32_t value)
{
    char data[sizeof(uint32_t)];
    *reinterpret_cast<uint32_t *>(data) = value;
    WriteChunk(data, sizeof(uint32_t));
}

void RawHeapDump::WriteU16(uint16_t value)
{
    char data[sizeof(uint16_t)];
    *reinterpret_cast<uint16_t *>(data) = value;
    WriteChunk(data, sizeof(uint16_t));
}

void RawHeapDump::WriteHeader(uint32_t offset, uint32_t size)
{
    uint32_t header[2] = {offset, size};
    WriteChunk(reinterpret_cast<char *>(header), sizeof(header));
}

void RawHeapDump::WritePadding()
{
    uint32_t padding = (8 - fileOffset_ % 8) % 8;
    if (padding > 0) {
        char pad[8] = {0};
        WriteChunk(pad, padding);
    }
}

void RawHeapDump::AddSectionRecord(uint32_t value)
{
    secIndexVec_.push_back(value);
}

void RawHeapDump::AddSectionOffset()
{
    secIndexVec_.push_back(fileOffset_);
    preOffset_ = fileOffset_;
}

void RawHeapDump::AddSectionBlockSize()
{
    secIndexVec_.push_back(fileOffset_ - preOffset_);
}

StringId RawHeapDump::GenerateStringId(TaggedObject *object)
{
    return snapshot_->GenerateStringId(object);
}

const StringHashMap *RawHeapDump::GetEcmaStringTable()
{
    return snapshot_->GetEcmaStringTable();
}

RawHeapDumpV1::RawHeapDumpV1(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot,
                             EntryIdMap *entryIdMap, const DumpSnapShotOption &dumpOption)
    : RawHeapDump(stream, snapshot, entryIdMap, dumpOption), vm_(vm)
{
}

RawHeapDumpV1::~RawHeapDumpV1()
{
    strIdMapObjVec_.clear();
}

/*
 *  |--4 bytes--|--4 bytes--|
 *  |-----------------------|
 *  |       versionId       |
 *  |-----------------------|
 *  |       timestamp       |
 *  |-----------------------|
 *  |  rootCnt  |  rootUnit |
 *  |-----------------------|
 *  |                       |
 *  |                       |
 *  |-----------------------|
 *  | stringCnt | 0(unused) |
 *  |-----------------------|
 *  |                       |
 *  |                       |
 *  |-----------------------|
 *  |objTotalCnt| tableUnit |
 *  |-----------------------|
 *  |                       |
 *  |                       |
 *  |-----------------------|
 *  | rootOffset|  rootSize |
 *  |-----------------------|
 *  | strOffset |  strSize  |
 *  |-----------------------|
 *  | objOffset |  objSize  |
 *  |-----------------------|
*/
void RawHeapDumpV1::BinaryDump()
{
    DumpVersion(std::string(RAWHEAP_VERSION));

    RootMarker rootMarker;
    HeapRootVisitor rootVisitor;
    rootVisitor.VisitHeapRoots(vm_->GetJSThread(), rootMarker);
    SharedModuleManager::GetInstance()->Iterate(rootMarker);
    Runtime::GetInstance()->IterateCachedStringRoot(rootMarker);
    DumpRootTable(rootMarker);

    ObjectMarker objectMarker;
    rootMarker.IterateMarked(std::bind(&ObjectMarker::ProcessMarkObjectsFromRoot,
                                       &objectMarker, std::placeholders::_1));
    DumpStringTable(objectMarker);
    DumpObjectTable(objectMarker);
    DumpObjectMemory(objectMarker);
    DumpSectionIndex();
}

void RawHeapDumpV1::DumpRootTable(HeapMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(JSTaggedType));
    auto cb = [this](JSTaggedType addr) {
        WriteU64(addr);
    };
    marker.IterateMarked(cb);
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, root count " << marker.Count();
}

void RawHeapDumpV1::DumpStringTable(HeapMarker &marker)
{
    UpdateStringTable(marker);
    auto strTable = GetEcmaStringTable();
    AddSectionOffset();
    WriteHeader(strTable->GetCapcity(), 0);
    for (auto key : strTable->GetOrderedKeyStorage()) {
        auto [strId, str] = strTable->GetStringAndIdPair(key);
        auto objVec = strIdMapObjVec_[strId];
        WriteHeader(str->size(), objVec.size());
        WriteChunk(reinterpret_cast<char *>(objVec.data()), objVec.size() * sizeof(JSTaggedType));
        WriteChunk(const_cast<char *>(str->c_str()), str->size() + 1);
    }

    WritePadding();
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, string table capcity " << strTable->GetCapcity();
}

void RawHeapDumpV1::DumpObjectTable(HeapMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(AddrTableItem));
    uint32_t memOffset = marker.Count() * sizeof(AddrTableItem);
    auto cb = [this, &memOffset](JSTaggedType addr) {
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(addr);
        AddrTableItem table = { addr, GenerateNodeId(addr), obj->GetSize(), memOffset };
        if (obj->GetClass()->IsString()) {
            memOffset += sizeof(JSHClass *);
        } else {
            memOffset += table.objSize;
        }
        WriteChunk(reinterpret_cast<char *>(&table), sizeof(AddrTableItem));
    };
    marker.IterateMarked(cb);
    LOG_ECMA(INFO) << "rawheap dump, objects total count " << marker.Count();
}

void RawHeapDumpV1::DumpObjectMemory(HeapMarker &marker)
{
    uint32_t memSize = 0;
    auto cb = [this, &memSize](JSTaggedType addr) {
        auto obj = reinterpret_cast<TaggedObject *>(addr);
        size_t size = obj->GetSize();
        if (obj->GetClass()->IsString()) {
            size = sizeof(JSHClass *);
        }
        memSize += size;
        WriteChunk(reinterpret_cast<char *>(addr), size);
    };
    marker.IterateMarked(cb);
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, objects memory size " << memSize;
}

void RawHeapDumpV1::UpdateStringTable(HeapMarker &marker)
{
    auto cb = [this](JSTaggedType addr) {
        JSTaggedValue value(addr);
        StringId strId = GenerateStringId(value.GetTaggedObject());
        if (strId == 1) {  // 1 : invalid str id
            return;
        }
        auto vec = strIdMapObjVec_.find(strId);
        if (vec != strIdMapObjVec_.end()) {
            vec->second.push_back(addr);
        } else {
            CVector<uint64_t> objVec;
            objVec.push_back(addr);
            strIdMapObjVec_.emplace(strId, objVec);
        }
    };
    marker.IterateMarked(cb);
}

RawHeapDumpV2::RawHeapDumpV2(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot,
                             EntryIdMap *entryIdMap, const DumpSnapShotOption &dumpOption)
    : RawHeapDump(stream, snapshot, entryIdMap, dumpOption), vm_(vm)
{
}

RawHeapDumpV2::~RawHeapDumpV2()
{
    regionIdMap_.clear();
}

void RawHeapDumpV2::BinaryDump()
{
    DumpVersion(std::string(RAWHEAP_VERSION_V2));

    RootMarker rootMarker;
    HeapRootVisitor rootVisitor;
    rootVisitor.VisitHeapRoots(vm_->GetJSThread(), rootMarker);
    SharedModuleManager::GetInstance()->Iterate(rootMarker);
    Runtime::GetInstance()->IterateCachedStringRoot(rootMarker);
    DumpRootTable(rootMarker);

    ObjectMarker objectMarker;
    rootMarker.IterateMarked(std::bind(&ObjectMarker::ProcessMarkObjectsFromRoot,
                                       &objectMarker, std::placeholders::_1));
    DumpStringTable(objectMarker);
    DumpObjectTable(objectMarker);
    DumpObjectMemory(objectMarker);
    DumpSectionIndex();
}

void RawHeapDumpV2::DumpRootTable(HeapMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(uint32_t));
    auto cb = [this](JSTaggedType addr) {
        WriteU32(GenerateSyntheticAddr(addr));
    };
    marker.IterateMarked(cb);
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, root count " << marker.Count();
}

void RawHeapDumpV2::DumpStringTable(HeapMarker &marker)
{
    UpdateStringTable(marker);
    auto strTable = GetEcmaStringTable();
    AddSectionOffset();
    WriteHeader(strTable->GetCapcity(), 0);
    for (auto key : strTable->GetOrderedKeyStorage()) {
        auto [strId, str] = strTable->GetStringAndIdPair(key);
        auto objVec = strIdMapObjVec_[strId];
        WriteHeader(str->size(), objVec.size());
        WriteChunk(reinterpret_cast<char *>(objVec.data()), objVec.size() * sizeof(uint32_t));
        WriteChunk(const_cast<char *>(str->c_str()), str->size() + 1);
    }

    WritePadding();
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, string table capcity " << strTable->GetCapcity();
}

void RawHeapDumpV2::DumpObjectTable(HeapMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(AddrTableItem));
    uint32_t memOffset = marker.Count() * sizeof(AddrTableItem);
    auto cb = [this, &memOffset](JSTaggedType addr) {
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(addr);
        AddrTableItem table = {
            GenerateSyntheticAddr(addr),
            obj->GetSize(),
            GenerateNodeId(addr),
            obj->GetClass()->IsJSNativePointer() ? JSNativePointer::Cast(obj)->GetBindingSize() : 0,
            static_cast<uint32_t>(obj->GetClass()->GetObjectType())
        };
        WriteChunk(reinterpret_cast<char *>(&table), sizeof(AddrTableItem));
    };
    marker.IterateMarked(cb);
    LOG_ECMA(INFO) << "rawheap dump, objects total count " << marker.Count();
}

void RawHeapDumpV2::DumpObjectMemory(HeapMarker &marker)
{
    uint32_t memSize = 0;
    auto cb = [this, &memSize](JSTaggedType addr) {
        TaggedObject *object = reinterpret_cast<TaggedObject *>(addr);
        WriteU32(GenerateSyntheticAddr(reinterpret_cast<JSTaggedType>(object->GetClass())));
        if (object->GetClass()->IsString()) {
            return;
        }
        ObjectSlot end(static_cast<uintptr_t>(addr + object->GetSize()));
        ObjectSlot slot(static_cast<uintptr_t>(addr + sizeof(JSTaggedType)));
        for (; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject() && !value.IsWeak()) {
                WriteU32(GenerateSyntheticAddr(value.GetRawData()));
                memSize += sizeof(uint32_t);
            } else {
                WriteU16(0);
                memSize += sizeof(uint16_t);
            }
        }
    };
    marker.IterateMarked(cb);
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, objects memory size " << memSize;
}

void RawHeapDumpV2::UpdateStringTable(HeapMarker &marker)
{
    auto cb = [this](JSTaggedType addr) {
        uint32_t syntheticAddr = GenerateSyntheticAddr(addr);
        StringId strId = GenerateStringId(reinterpret_cast<TaggedObject *>(addr));
        if (strId == 1) {  // 1 : invalid str id
            return;
        }
        auto vec = strIdMapObjVec_.find(strId);
        if (vec != strIdMapObjVec_.end()) {
            vec->second.push_back(syntheticAddr);
        } else {
            CVector<uint32_t> objVec;
            objVec.push_back(syntheticAddr);
            strIdMapObjVec_.emplace(strId, objVec);
        }
    };
    marker.IterateMarked(cb);
}

uint32_t RawHeapDumpV2::GenerateRegionId(JSTaggedType addr)
{
    Region *region = Region::ObjectAddressToRange(addr);
    auto [it, inserted] = regionIdMap_.try_emplace(region, regionId_);
    if (inserted) {
        ++regionId_;
    }
    return it->second;
}

/* ------------------------------
 * |  regionId  | indexInRegion |
 * ------------------------------  ==> uint32_t synthetic addr
 * |  uint16_t  |   uint16_t    |
 * ------------------------------
 */
uint32_t RawHeapDumpV2::GenerateSyntheticAddr(JSTaggedType addr)
{
#ifdef OHOS_UNIT_TEST
    return static_cast<uint32_t>(addr);
#else
    uint16_t syntheticAddr[2];
    syntheticAddr[0] = static_cast<uint16_t>(GenerateRegionId(addr));
    syntheticAddr[1] = static_cast<uint16_t>((addr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
    return *reinterpret_cast<uint32_t *>(syntheticAddr);
#endif
}
} // namespace panda::ecmascript
