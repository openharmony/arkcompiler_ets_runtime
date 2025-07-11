/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_PROCESSOR_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_PROCESSOR_H

#include <iostream>
#include <fstream>
#include <sstream>

#include "common_components/serialize/serialize_utils.h"
#include "ecmascript/serializer/serialize_data.h"
#include "ecmascript/snapshot/mem/encode_bit.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/object_xray.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
class EcmaVM;
class JSPandaFile;
class AOTFileManager;

enum class SnapshotType {
    VM_ROOT,
    BUILTINS,
    AI
};

struct SnapshotRegionHeadInfo {
    size_t regionIndex_ {0};
    size_t aliveObjectSize_ {0};

    static constexpr size_t RegionHeadInfoSize()
    {
        return sizeof(SnapshotRegionHeadInfo);
    }
};

using ObjectEncode = std::pair<uint64_t, ecmascript::EncodeBit>;

class SnapshotProcessor final {
public:
    explicit SnapshotProcessor(EcmaVM *vm)
        : vm_(vm), sHeap_(SharedHeap::GetInstance()) {}
    ~SnapshotProcessor();

    struct AllocResult {
        uintptr_t address;
        size_t offset;
        size_t regionIndex;
    };

    void Initialize();
    void StopAllocate();
    void WriteObjectToFile(std::fstream &write);
    std::vector<size_t> StatisticsObjectSize();
    void ProcessObjectQueue(CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, ObjectEncode> *data);
    void SerializeObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                         std::unordered_map<uint64_t, ObjectEncode> *data);
    void Relocate(SnapshotType type, const JSPandaFile *jsPandaFile,
                  uint64_t rootObjSize);
    void RelocateSpaceObject(std::vector<std::pair<uintptr_t, size_t>> &regions, SnapshotType type,
        MethodLiteral* methods, size_t methodNums, size_t rootObjSize);
    void RelocateSpaceObject(Space* space, SnapshotType type, MethodLiteral* methods,
                             size_t methodNums, size_t rootObjSize);
    void SerializePandaFileMethod();
    AllocResult GetNewObj(size_t objectSize, TaggedObject *objectHeader);
    uintptr_t GetNewObjAddress(size_t objectSize, TaggedObject *objectHeader);
    EncodeBit EncodeTaggedObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                 std::unordered_map<uint64_t, ObjectEncode> *data);
    EncodeBit GetObjectEncode(JSTaggedValue object, CQueue<TaggedObject *> *queue,
                              std::unordered_map<uint64_t, ObjectEncode> *data);
    void EncodeTaggedObjectRange(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                 std::unordered_map<uint64_t, ObjectEncode> *data);
    void DeserializeObjectExcludeString(uintptr_t regularObjBegin, size_t regularObjSize, size_t pinnedObjSize,
                                        size_t largeObjSize);
    void DeserializeObjectExcludeString(uintptr_t oldSpaceBegin, size_t oldSpaceObjSize, size_t nonMovableObjSize,
                                        size_t machineCodeObjSize, size_t snapshotObjSize, size_t hugeSpaceObjSize);
    void DeserializeString(uintptr_t stringBegin, uintptr_t stringEnd);

    // ONLY used in UT to get the deserialize value result
    JSTaggedValue GetDeserializeResultForUT() const
    {
        return root_;
    }

    void AddRootObjectToAOTFileManager(SnapshotType type, const CString &fileName);

    void SetProgramSerializeStart()
    {
        programSerialize_ = true;
    }

    void SetBuiltinsSerializeStart()
    {
        builtinsSerialize_ = true;
    }

    void SetBuiltinsDeserializeStart()
    {
        builtinsDeserialize_ = true;
    }

    const CVector<uintptr_t> GetStringVector() const
    {
        return stringVector_;
    }

    size_t GetNativeTableSize() const;

private:
    class SerializeObjectVisitor final : public BaseObjectVisitor<SerializeObjectVisitor> {
    public:
        explicit SerializeObjectVisitor(SnapshotProcessor *processor, uintptr_t snapshotObj,
            CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, ObjectEncode> *data);
        ~SerializeObjectVisitor() override = default;

        void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr,
                                  uintptr_t endAddr, VisitObjectArea area) override;
    private:
        SnapshotProcessor *processor_ {nullptr};
        uintptr_t snapshotObj_ {-1};
        CQueue<TaggedObject *> *queue_ {nullptr};
        std::unordered_map<uint64_t, ObjectEncode> *data_{nullptr};
    };

    class DeserializeFieldVisitor final : public BaseObjectVisitor<DeserializeFieldVisitor> {
    public:
        explicit DeserializeFieldVisitor(SnapshotProcessor *processor);
        ~DeserializeFieldVisitor() override = default;

        void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr,
                                  VisitObjectArea area) override;
    private:
        SnapshotProcessor *processor_ {nullptr};
    };
    size_t GetMarkGCBitSetSize() const
    {
        return GCBitset::SizeOfGCBitset(DEFAULT_REGION_SIZE -
            AlignUp(sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION)));
    }

    bool VisitObjectBodyWithRep(TaggedObject *root, ObjectSlot slot, uintptr_t obj, int index, VisitObjectArea area);
    void SetObjectEncodeField(uintptr_t obj, size_t offset, uint64_t value);

    EncodeBit SerializeObjectHeader(TaggedObject *objectHeader, size_t objectType, CQueue<TaggedObject *> *queue,
                                    std::unordered_map<uint64_t, ObjectEncode> *data);
    uint64_t SerializeTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                                  std::unordered_map<uint64_t, ObjectEncode> *data);
    void DeserializeField(TaggedObject *objectHeader);
    void DeserializeTaggedField(uint64_t *value, TaggedObject *root);
    void DeserializeNativePointer(uint64_t *value);
    void DeserializeClassWord(TaggedObject *object);
    void DeserializePandaMethod(uintptr_t begin, uintptr_t end, MethodLiteral *methods,
                                size_t &methodNums, size_t &others);
    void DeserializeSpaceObject(uintptr_t beginAddr, size_t objSize, SerializedObjectSpace spaceType);
    void DeserializeSpaceObject(uintptr_t beginAddr, Space* space, size_t spaceObjSize);
    void DeserializeHugeSpaceObject(uintptr_t beginAddr, HugeObjectSpace* space, size_t hugeSpaceObjSize);
    void HandleRootObject(SnapshotType type, uintptr_t rootObjectAddr, size_t objType, size_t &constSpecialIndex);

    EncodeBit NativePointerToEncodeBit(void *nativePointer);
    size_t SearchNativeMethodIndex(void *nativePointer);
    uintptr_t TaggedObjectEncodeBitToAddr(EncodeBit taggedBit);
    void WriteSpaceObjectToFile(Space* space, std::fstream &write);
    void WriteHugeObjectToFile(HugeObjectSpace* space, std::fstream &writer);
    size_t StatisticsSpaceObjectSize(Space* space);
    size_t StatisticsHugeObjectSize(HugeObjectSpace* space);
    uintptr_t AllocateObjectToLocalSpace(Space *space, size_t objectSize);
    SnapshotRegionHeadInfo GenerateRegionHeadInfo(Region *region);
    void ResetRegionUnusedRange(Region *region);

    EcmaVM *vm_ {nullptr};
    SharedHeap* sHeap_ {nullptr};
    class RegionProxy {
    public:
        RegionProxy() = default;
        ~RegionProxy() = default;

        bool IsEmpty() const
        {
            return begin_ == 0;
        }

        AllocResult Allocate(size_t size)
        {
            if (top_ + size <= end_) {
                uintptr_t addr = top_;
                top_ += size;
                return {addr, addr - begin_, regionIndex_};
            }
            return {0, 0, -1};
        }

        void Reset(uintptr_t begin, uintptr_t end, size_t regionIndex)
        {
            begin_ = begin;
            top_ = begin;
            end_ = end;
            regionIndex_ = regionIndex;
        }

        uintptr_t GetBegin() const
        {
            return begin_;
        }

        uintptr_t GetTop() const
        {
            return top_;
        }

        uintptr_t GetEnd() const
        {
            return end_;
        }

        size_t GetRegionIndex() const
        {
            return regionIndex_;
        }
    private:
        uintptr_t begin_ {0};
        uintptr_t top_ {0};
        uintptr_t end_ {0};
        size_t regionIndex_ {-1};
    };

    class AllocateProxy {
    public:
        AllocateProxy(SerializedObjectSpace spaceType) : spaceType_(spaceType) {}
        ~AllocateProxy()
        {
            for (RegionProxy &proxy : regions_) {
                ASSERT(!proxy.IsEmpty());
                void *ptr = reinterpret_cast<void*>(proxy.GetBegin());
                free(ptr);
            }
        }

        void Initialize(size_t commonRegionSize);

        AllocResult Allocate(size_t size, uintptr_t &regionIndex);

        void StopAllocate(SharedHeap *sHeap);

        size_t GetAllocatedSize() const
        {
            return allocatedSize_;
        }

        void WriteToFile(std::fstream &writer);
    private:
        void AllocateNewRegion(size_t size, uintptr_t &regionIndex);
        std::vector<RegionProxy> regions_ {};
        RegionProxy currentRegion_ {};
        SerializedObjectSpace spaceType_ {SerializedObjectSpace::OTHER};
        size_t allocatedSize_ {0};
        size_t commonRegionSize_ {0};
    };
    AllocateProxy regularObjAllocator_ {SerializedObjectSpace::REGULAR_SPACE};
    AllocateProxy pinnedObjAllocator_ {SerializedObjectSpace::PIN_SPACE};
    AllocateProxy largeObjAllocator_ {SerializedObjectSpace::LARGE_SPACE};
    size_t commonRegionSize_ {0};
    LocalSpace *oldLocalSpace_ {nullptr};
    LocalSpace *nonMovableLocalSpace_ {nullptr};
    LocalSpace *machineCodeLocalSpace_ {nullptr};
    SnapshotSpace *snapshotLocalSpace_ {nullptr};
    HugeObjectSpace *hugeObjectLocalSpace_ {nullptr};
    bool programSerialize_ {false};
    bool builtinsSerialize_ {false};
    bool builtinsDeserialize_ {false};
    CVector<uintptr_t> pandaMethod_;
    CVector<uintptr_t> stringVector_;
    /**
     * In deserialize, RuntimeLock for string table may cause a SharedGC, making strings just created invalid,
     * so use handle to protect.
    */
    CVector<JSHandle<EcmaString>> deserializeStringVector_;
    std::unordered_map<size_t, uintptr_t> regionIndexMap_;
    size_t regionIndex_ {0};
    bool isRootObjRelocate_ {false};
    JSTaggedValue root_ {JSTaggedValue::Hole()};
    std::vector<std::pair<uintptr_t, size_t>> regularRegions_ {};
    std::vector<std::pair<uintptr_t, size_t>> pinnedRegions_ {};
    std::vector<std::pair<uintptr_t, size_t>> largeRegions_ {};

    NO_COPY_SEMANTIC(SnapshotProcessor);
    NO_MOVE_SEMANTIC(SnapshotProcessor);
};

class SnapshotHelper {
public:
    // when snapshot serialize, huge obj size is writed to region snapshotData_ high 32 bits
    static inline uint64_t EncodeHugeObjectSize(uint64_t objSize)
    {
        return objSize << Constants::UINT_32_BITS_COUNT;
    }

    // get huge object size which is saved in region snapshotData_ high 32 bits
    static inline size_t GetHugeObjectSize(uint64_t snapshotData)
    {
        return snapshotData >> Constants::UINT_32_BITS_COUNT;
    }

    // get huge object region index which is saved in region snapshotMark_ low 32 bits
    static inline size_t GetHugeObjectRegionIndex(uint64_t snapshotData)
    {
        return snapshotData & Constants::MAX_UINT_32;
    }
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_PROCESSOR_H
