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

#ifndef ECMASCRIPT_DFX_HPROF_RAWHEAP_DUMP_H
#define ECMASCRIPT_DFX_HPROF_RAWHEAP_DUMP_H

#include <functional>

#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "ecmascript/dfx/hprof/heap_marker.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"


namespace panda::ecmascript {
class RootMarker : public HeapMarker, public RootVisitor {
public:
    RootMarker() = default;
    ~RootMarker() = default;

    void VisitRoot(Root type, ObjectSlot slot) override;
    void VisitRangeRoot(Root type, ObjectSlot start, ObjectSlot end) override;
    void VisitBaseAndDerivedRoot(Root type, ObjectSlot base, ObjectSlot derived, uintptr_t baseOldObject) override {}
};

class ObjectMarker : public HeapMarker, public BaseObjectVisitor<ObjectMarker> {
public:
    ObjectMarker() = default;
    ~ObjectMarker() = default;

    void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t endAddr, VisitObjectArea area) override;
    void ProcessMarkObjectsFromRoot(JSTaggedType root);

private:
    bool IsEmpty()
    {
        return bfsQueue_.empty();
    }

    JSTaggedType PopUnchecked()
    {
        JSTaggedType addr = bfsQueue_.front();
        bfsQueue_.pop();
        return addr;
    }

    void Push(JSTaggedType addr)
    {
        bfsQueue_.push(addr);
    }

    CQueue<JSTaggedType> bfsQueue_ {};
};

class RawHeapDump {
public:
    RawHeapDump(Stream *stream, HeapSnapshot *snapshot,
                EntryIdMap* entryIdMap, const DumpSnapShotOption &dumpOption);
    virtual ~RawHeapDump();

    virtual void BinaryDump() = 0;

protected:
    void DumpVersion(const std::string &version);
    void DumpSectionIndex();
    NodeId GenerateNodeId(JSTaggedType addr);
    void WriteChunk(char *data, size_t size);
    void WriteU64(uint64_t value);
    void WriteHeader(uint32_t offset, uint32_t size);
    void WritePadding();
    void AddSectionRecord(uint32_t value);
    void AddSectionOffset();
    void AddSectionBlockSize();
    StringId GenerateStringId(TaggedObject *object);
    const StringHashMap *GetEcmaStringTable();

private:
    StreamWriter writer_;
    HeapSnapshot *snapshot_ {nullptr};
    EntryIdMap *entryIdMap_ {nullptr};
    CVector<uint32_t> secIndexVec_ {};
    uint32_t fileOffset_ {0};
    uint32_t preOffset_ {0};
    bool isOOM_ {false};
    std::chrono::time_point<std::chrono::steady_clock> startTime_;
};

class RawHeapDumpV1 : public RawHeapDump {
public:
    RawHeapDumpV1(const EcmaVM *vm, Stream *stream, HeapSnapshot *snapshot,
                  EntryIdMap* entryIdMap, const DumpSnapShotOption &dumpOption);
    ~RawHeapDumpV1();

    void BinaryDump() override;

private:
    struct AddrTableItem {
        uint64_t addr;
        uint64_t id;
        uint32_t objSize;
        uint32_t offset; // offset to the file
    };

    void DumpRootTable(HeapMarker &marker);
    void DumpStringTable(HeapMarker &marker);
    void DumpObjectTable(HeapMarker &marker);
    void DumpObjectMemory(HeapMarker &marker);
    void UpdateStringTable(HeapMarker &marker);

    constexpr static const char *const RAWHEAP_VERSION = "1.0.0";
    const EcmaVM *vm_ {nullptr};
    CUnorderedMap<uint64_t, CVector<uint64_t>> strIdMapObjVec_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_DUMP_H
