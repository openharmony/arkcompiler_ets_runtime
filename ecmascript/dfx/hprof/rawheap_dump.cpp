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

#include "common_components/heap/heap.h"
#include "ecmascript/base/config.h"
#include "ecmascript/dfx/hprof/rawheap_dump.h"
#include "ecmascript/dfx/hprof/rawheap_translate/common.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript {
void ObjectMarker::VisitRoot([[maybe_unused]]Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        MarkObject(slot.GetTaggedType());
    }
}

void ObjectMarker::VisitRangeRoot(Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        VisitRoot(type, slot);
    }
}

void ObjectMarker::VisitBaseAndDerivedRoot(Root type, ObjectSlot base, ObjectSlot derived, uintptr_t baseOldObject)
{
}

void ObjectMarker::VisitObjectRangeImpl([[maybe_unused]]BaseObject *root, uintptr_t start,
                                        uintptr_t endAddr, [[maybe_unused]]VisitObjectArea area)
{
    ObjectSlot end(endAddr);
    for (ObjectSlot slot(start); slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.GetRawData() != 0 && value.IsHeapObject() && !value.IsWeak()) {
            MarkObject(slot.GetTaggedType());
        }
    }
}

void ObjectMarker::ProcessMarkObjectsFromRoot()
{
    while (!bfsQueue_.empty()) {
        JSTaggedType addr = bfsQueue_.front();
        bfsQueue_.pop();

        JSTaggedValue value(addr);
        TaggedObject *object = value.GetTaggedObject();
        JSHClass *hclass = object->GetClass();
        MarkObject(reinterpret_cast<JSTaggedType>(hclass));
        if (hclass->IsString()) {
            continue;
        }
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, hclass, *this);
    }
}

void ObjectMarker::IterateMarkedObjects(const std::function<void(JSTaggedType)> &visitor)
{
    for (auto addr : markedObjects_) {
        visitor(addr);
    }
}

void ObjectMarker::MarkObject(JSTaggedType addr)
{
    if (Mark(addr)) {
        markedObjects_.push_back(addr);
        bfsQueue_.push(addr);
    }
}

RawHeapDump::RawHeapDump(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot,
                         EntryIdMap *entryIdMap, const DumpSnapShotOption &dumpOption)
    : vm_(vm), writer_(stream), snapshot_(snapshot), entryIdMap_(entryIdMap)
{
    isOOM_ = dumpOption.isDumpOOM;
    isJSLeakWatcher_ = dumpOption.isJSLeakWatcher;
    startTime_ = std::chrono::steady_clock::now();
}

RawHeapDump::~RawHeapDump()
{
    writer_.EndOfWriteBinBlock();
    secIndexVec_.clear();
    auto endTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime_).count();
    LOG_ECMA(INFO) << "rawheap dump success, cost " << duration << "s, " << "file size " << GetRawHeapFileOffset();
}

void RawHeapDump::MarkRootForDump(ObjectMarker &marker)
{
    if (g_isEnableCMCGC) {
        common::RefFieldVisitor visitor = [&marker](common::RefField<>& refField) {
            BaseObject *oldObj = refField.GetTargetObject();
            marker.MarkObject(reinterpret_cast<JSTaggedType>(oldObj));
        };
        common::VisitRoots(visitor);
    } else {
        HeapRootVisitor rootVisitor;
        rootVisitor.VisitHeapRoots(vm_->GetAssociatedJSThread(), marker);
        SharedModuleManager::GetInstance()->Iterate(marker);
        Runtime::GetInstance()->IterateCachedStringRoot(marker);
    }
}

void RawHeapDump::MarkHeapObjectForDump(ObjectMarker &marker)
{
    marker.ProcessMarkObjectsFromRoot();
    LOG_ECMA(INFO) << "rawheap dump, marked objects count " << marker.Count();
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

/*
* mixed hash in NodeId, for example:
* |----------------- uint64_t -----------------|
* |-----high-32-bits-----|----lower-32-bits----|
* |--------------------------------------------|
* |         hash         | nodeId & 0xFFFFFFFF |
* |--------------------------------------------|
*/
NodeId RawHeapDump::GenerateNodeId(JSTaggedType addr)
{
    NodeId nodeId = isOOM_ ? entryIdMap_->GetNextId() : entryIdMap_->FindOrInsertNodeId(addr);
    if (!isJSLeakWatcher_) {
        return nodeId;
    }

    JSTaggedValue value {addr};
    int32_t hash = value.IsJSObject() ? JSObject::Cast(value)->GetHash(vm_->GetJSThread()) : 0;
    return (static_cast<uint64_t>(hash) << 32) | (nodeId & 0xFFFFFFFF);  // 32: 32-bits means a half of uint64_t
}

void RawHeapDump::WriteChunk(char *data, size_t size)
{
    writer_.WriteBinBlock(data, size);
}

void RawHeapDump::WriteU64(uint64_t value)
{
    writer_.WriteUInt64(value);
}

void RawHeapDump::WriteU32(uint32_t value)
{
    writer_.WriteUInt32(value);
}

void RawHeapDump::WriteU16(uint16_t value)
{
    writer_.WriteUInt16(value);
}

void RawHeapDump::WriteU8(uint8_t value)
{
    writer_.WriteUInt8(value);
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
    secIndexVec_.push_back(GetRawHeapFileOffset());
    preOffset_ = GetRawHeapFileOffset();
}

void RawHeapDump::AddSectionBlockSize()
{
    secIndexVec_.push_back(GetRawHeapFileOffset() - preOffset_);
}

StringId RawHeapDump::GenerateStringId(TaggedObject *object)
{
    JSTaggedValue entry(object);
    JSThread *thread = vm_->GetAssociatedJSThread();

    if (entry.IsOnlyJSObject()) {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        bool isCallGetter = false;
        JSHandle<JSTaggedValue> contructorKey = globalConst->GetHandledConstructorString();
        JSTaggedValue objConstructor = ObjectFastOperator::GetPropertyByName(thread, entry,
                                                                             contructorKey.GetTaggedValue(), true,
                                                                             &isCallGetter);
        auto it = objectStrIds_.find(objConstructor.GetRawData());
        if (it != objectStrIds_.end()) {
            return it->second;
        }
        StringId strId = snapshot_->GenerateStringId(object);
        objectStrIds_.emplace(objConstructor.GetRawData(), strId);
        return strId;
    }

    if (entry.IsJSFunction()) {
        JSFunctionBase *func = JSFunctionBase::Cast(object);
        Method *method = Method::Cast(func->GetMethod(thread).GetTaggedObject());
        auto it = functionStrIds_.find(method);
        if (it != functionStrIds_.end()) {
            return it->second;
        }
        StringId strId = snapshot_->GenerateStringId(object);
        functionStrIds_.emplace(method, strId);
        return strId;
    }

    return 1;  // 1 : invalid id
}

const StringHashMap *RawHeapDump::GetEcmaStringTable()
{
    return snapshot_->GetEcmaStringTable();
}

RawHeapDumpV1::RawHeapDumpV1(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot,
                             EntryIdMap *entryIdMap, const DumpSnapShotOption &dumpOption)
    : RawHeapDump(vm, stream, snapshot, entryIdMap, dumpOption)
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

    ObjectMarker marker;
    MarkRootForDump(marker);
    DumpRootTable(marker);

    MarkHeapObjectForDump(marker);
    DumpStringTable(marker);
    DumpObjectTable(marker);
    DumpObjectMemory(marker);
    DumpSectionIndex();
}

void RawHeapDumpV1::DumpRootTable(ObjectMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(JSTaggedType));
    marker.IterateMarkedObjects([this](JSTaggedType addr) {
        WriteU64(addr);
    });
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, root count " << marker.Count();
}

void RawHeapDumpV1::DumpStringTable(ObjectMarker &marker)
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

void RawHeapDumpV1::DumpObjectTable(ObjectMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(AddrTableItem));
    uint32_t memOffset = marker.Count() * sizeof(AddrTableItem);
    marker.IterateMarkedObjects([this, &memOffset](JSTaggedType addr) {
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(addr);
        AddrTableItem table = { addr, GenerateNodeId(addr), obj->GetSize(), memOffset };
        if (obj->GetClass()->IsString()) {
            memOffset += sizeof(JSHClass *);
        } else {
            memOffset += table.objSize;
        }
        WriteChunk(reinterpret_cast<char *>(&table), sizeof(AddrTableItem));
    });
    LOG_ECMA(INFO) << "rawheap dump, objects total count " << marker.Count();
}

void RawHeapDumpV1::DumpObjectMemory(ObjectMarker &marker)
{
    uint32_t memSize = 0;
    marker.IterateMarkedObjects([this, &memSize](JSTaggedType addr) {
        auto obj = reinterpret_cast<TaggedObject *>(addr);
        size_t size = obj->GetSize();
        memSize += size;
        if (obj->GetClass()->IsString()) {
            size = sizeof(JSHClass *);
        }
        if (g_isEnableCMCGC) {
            WriteU64(reinterpret_cast<JSTaggedType>(obj->GetClass()));
            WriteChunk(reinterpret_cast<char *>(addr + sizeof(JSTaggedType)), size - sizeof(JSTaggedType));
        } else {
            WriteChunk(reinterpret_cast<char *>(addr), size);
        }
    });
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, objects memory size " << memSize;
}

void RawHeapDumpV1::UpdateStringTable(ObjectMarker &marker)
{
    uint32_t strCnt = 0;
    marker.IterateMarkedObjects([&strCnt, this](JSTaggedType addr) {
        JSTaggedValue value(addr);
        StringId strId = GenerateStringId(value.GetTaggedObject());
        if (strId == 1) {  // 1 : invalid str id
            return;
        }
        ++strCnt;
        auto vec = strIdMapObjVec_.find(strId);
        if (vec != strIdMapObjVec_.end()) {
            vec->second.push_back(addr);
        } else {
            CVector<uint64_t> objVec;
            objVec.push_back(addr);
            strIdMapObjVec_.emplace(strId, objVec);
        }
    });
    LOG_ECMA(INFO) << "rawheap dump, update string table count " << strCnt;
}

RawHeapDumpV2::RawHeapDumpV2(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot,
                             EntryIdMap *entryIdMap, const DumpSnapShotOption &dumpOption)
    : RawHeapDump(vm, stream, snapshot, entryIdMap, dumpOption)
{
}

RawHeapDumpV2::~RawHeapDumpV2()
{
    regionIdMap_.clear();
}

void RawHeapDumpV2::BinaryDump()
{
    DumpVersion(std::string(RAWHEAP_VERSION_V2));

    ObjectMarker marker;
    MarkRootForDump(marker);
    DumpRootTable(marker);

    MarkHeapObjectForDump(marker);
    DumpStringTable(marker);
    DumpObjectTable(marker);
    DumpObjectMemory(marker);
    DumpSectionIndex();
}

void RawHeapDumpV2::DumpRootTable(ObjectMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(uint32_t));
    marker.IterateMarkedObjects([this](JSTaggedType addr) {
        WriteU32(GenerateSyntheticAddr(addr));
    });
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, root count " << marker.Count();
}

void RawHeapDumpV2::DumpStringTable(ObjectMarker &marker)
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

void RawHeapDumpV2::DumpObjectTable(ObjectMarker &marker)
{
    AddSectionOffset();
    WriteHeader(marker.Count(), sizeof(AddrTableItem));
    uint32_t memOffset = marker.Count() * sizeof(AddrTableItem);
    marker.IterateMarkedObjects([this, &memOffset](JSTaggedType addr) {
        TaggedObject *obj = reinterpret_cast<TaggedObject *>(addr);
        AddrTableItem table = {
            GenerateSyntheticAddr(addr),
            obj->GetSize(),
            GenerateNodeId(addr),
            obj->GetClass()->IsJSNativePointer() ? JSNativePointer::Cast(obj)->GetBindingSize() : 0,
            static_cast<uint32_t>(obj->GetClass()->GetObjectType())
        };
        WriteChunk(reinterpret_cast<char *>(&table), sizeof(AddrTableItem));
    });
    LOG_ECMA(INFO) << "rawheap dump, objects total count " << marker.Count();
}

void RawHeapDumpV2::DumpObjectMemory(ObjectMarker &marker)
{
    uint32_t memSize = 0;
    marker.IterateMarkedObjects([this, &memSize](JSTaggedType addr) {
        TaggedObject *object = reinterpret_cast<TaggedObject *>(addr);
        memSize += object->GetSize();

        WriteU32(GenerateSyntheticAddr(reinterpret_cast<JSTaggedType>(object->GetClass())));
        if (object->GetClass()->IsString()) {
            return;
        }
        ObjectSlot end(static_cast<uintptr_t>(addr + object->GetSize()));
        ObjectSlot slot(static_cast<uintptr_t>(addr + sizeof(JSTaggedType)));
        for (; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.GetRawData() != 0 && value.IsHeapObject() && !value.IsWeak()) {
                WriteU32(GenerateSyntheticAddr(value.GetRawData()));
            } else {
                WriteU8(rawheap_translate::ZERO_VALUE);
            }
        }
    });
    AddSectionBlockSize();
    LOG_ECMA(INFO) << "rawheap dump, objects memory size " << memSize;
}

void RawHeapDumpV2::UpdateStringTable(ObjectMarker &marker)
{
    uint32_t strCnt = 0;
    marker.IterateMarkedObjects([&strCnt, this](JSTaggedType addr) {
        uint32_t syntheticAddr = GenerateSyntheticAddr(addr);
        StringId strId = GenerateStringId(reinterpret_cast<TaggedObject *>(addr));
        if (strId == 1) {  // 1 : invalid str id
            return;
        }
        ++strCnt;
        auto vec = strIdMapObjVec_.find(strId);
        if (vec != strIdMapObjVec_.end()) {
            vec->second.push_back(syntheticAddr);
        } else {
            CVector<uint32_t> objVec;
            objVec.push_back(syntheticAddr);
            strIdMapObjVec_.emplace(strId, objVec);
        }
    });
    LOG_ECMA(INFO) << "rawheap dump, update string table count " << strCnt;
}

uint32_t RawHeapDumpV2::GenerateRegionId(JSTaggedType addr)
{
    Region *region = Region::ObjectAddressToRange(addr);
    auto [it, inserted] = regionIdMap_.try_emplace(region, regionId_);
    if (inserted) {
        regionId_ += sizeof(JSTaggedType);
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
    if (g_isEnableCMCGC) {
        return static_cast<uint32_t>(addr);
    } else {
        uint16_t syntheticAddr[2];
        syntheticAddr[0] = static_cast<uint16_t>(GenerateRegionId(addr));
        syntheticAddr[1] = static_cast<uint16_t>((addr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
        return *reinterpret_cast<uint32_t *>(syntheticAddr);
    }
#endif
}
} // namespace panda::ecmascript
