/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SERIALIZER_SERIALIZE_DATA_H
#define ECMASCRIPT_SERIALIZER_SERIALIZE_DATA_H

#include <limits>

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/dyn_chunk.h"
#include "ecmascript/runtime.h"
#include "ecmascript/shared_mm/shared_mm.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"

namespace panda::ecmascript {
constexpr size_t INITIAL_CAPACITY = 64;
constexpr int CAPACITY_INCREASE_RATE = 2;
constexpr uint32_t RESERVED_INDEX = 0;
static constexpr int SERIALIZE_SPACE_NUM = 7;

typedef void* (*DetachFunc)(void *enginePointer, void *objPointer, void *hint, void *detachData);
typedef Local<JSValueRef> (*AttachFunc)(void *enginePointer, void *buffer, void *hint, void *attachData);
typedef void (*DetachFinalizer)(void *detachedObject, void *finalizerHint);

struct NativeBindingDetachInfo {
    DetachFinalizer detachedFinalizer = nullptr;
    void *detachedObject = nullptr;
    void *detachedHint = nullptr;

    NativeBindingDetachInfo(void *df, void *dObj, void *hint)
        : detachedFinalizer(reinterpret_cast<DetachFinalizer>(df)), detachedObject(dObj), detachedHint(hint)
    {
    }
};

enum class EncodeFlag : uint8_t {
    // 0x00~0x06 represent new object to different space:
    // 0x00: old space
    // 0x01: non movable space
    // 0x02: machine code space
    // 0x03: huge space
    // 0x04: shared old space
    // 0x05: shared non movable space
    // 0x06: shared huge space
    NEW_OBJECT = 0x00,
    REFERENCE = 0x07,
    WEAK,
    PRIMITIVE,
    MULTI_RAW_DATA,
    ROOT_OBJECT,
    OBJECT_PROTO,
    ARRAY_BUFFER,
    TRANSFER_ARRAY_BUFFER,
    SHARED_ARRAY_BUFFER,
    SENDABLE_ARRAY_BUFFER,
    NATIVE_BINDING_OBJECT,
    JS_ERROR,
    JS_REG_EXP,
    SHARED_OBJECT,
    LAST
};

enum class SerializedObjectSpace : uint8_t {
    OLD_SPACE = 0,
    NON_MOVABLE_SPACE,
    MACHINE_CODE_SPACE,
    HUGE_SPACE,
    SHARED_OLD_SPACE,
    SHARED_NON_MOVABLE_SPACE,
    SHARED_HUGE_SPACE
};

enum class SerializeType : uint8_t {
    VALUE_SERIALIZE,
    PGO_SERIALIZE
};

class SerializeData {
public:
    explicit SerializeData(JSThread *thread) : thread_(thread) {}
    ~SerializeData()
    {
        // decrease sharedArrayBuffer reference
        if (sharedArrayBufferSet_.size() > 0) {
            DecreaseSharedArrayBufferReference();
        }
        for (const auto &info: nativeBindingDetachInfos_) {
            auto finalizer = reinterpret_cast<DetachFinalizer>(info->detachedFinalizer);
            if (finalizer != nullptr) {
                finalizer(info->detachedObject, info->detachedHint);
            }
            delete info;
        }
        nativeBindingDetachInfos_.clear();
        free(buffer_);
        if (!incompleteData_ && dataIndex_ != RESERVED_INDEX) {
            Runtime::GetInstance()->RemoveSerializationRoot(thread_, dataIndex_);
        }
    }
    NO_COPY_SEMANTIC(SerializeData);
    NO_MOVE_SEMANTIC(SerializeData);

    static uint8_t EncodeNewObject(SerializedObjectSpace space)
    {
        return static_cast<uint8_t>(space) | static_cast<uint8_t>(EncodeFlag::NEW_OBJECT);
    }

    static SerializedObjectSpace DecodeSpace(uint8_t type)
    {
        ASSERT(type < static_cast<uint8_t>(EncodeFlag::REFERENCE));
        return static_cast<SerializedObjectSpace>(type);
    }

    static size_t AlignUpRegionAvailableSize(size_t size)
    {
        if (size == 0) {
            return Region::GetRegionAvailableSize();
        }
        size_t regionAvailableSize = Region::GetRegionAvailableSize();
        return ((size - 1) / regionAvailableSize + 1) * regionAvailableSize; // 1: align up
    }

    bool ExpandBuffer(size_t requestedSize)
    {
        size_t newCapacity = bufferCapacity_ * CAPACITY_INCREASE_RATE;
        newCapacity = std::max(newCapacity, requestedSize);
        if (newCapacity > sizeLimit_) {
            return false;
        }
        uint8_t *newBuffer = reinterpret_cast<uint8_t *>(malloc(newCapacity));
        if (newBuffer == nullptr) {
            return false;
        }
        if (memcpy_s(newBuffer, newCapacity, buffer_, bufferSize_) != EOK) {
            LOG_FULL(ERROR) << "Failed to memcpy_s Data";
            free(newBuffer);
            return false;
        }
        free(buffer_);
        buffer_ = newBuffer;
        bufferCapacity_ = newCapacity;
        return true;
    }

    bool AllocateBuffer(size_t bytes)
    {
        // Get internal heap size
        if (sizeLimit_ == 0) {
            uint64_t heapSize = thread_->GetEcmaVM()->GetJSOptions().GetSerializerBufferSizeLimit();
            sizeLimit_ = heapSize;
        }
        size_t oldSize = bufferSize_;
        size_t newSize = oldSize + bytes;
        if (newSize > sizeLimit_) {
            return false;
        }
        if (bufferCapacity_ == 0) {
            if (bytes < INITIAL_CAPACITY) {
                buffer_ = reinterpret_cast<uint8_t *>(malloc(INITIAL_CAPACITY));
                if (buffer_ != nullptr) {
                    bufferCapacity_ = INITIAL_CAPACITY;
                    return true;
                } else {
                    return false;
                }
            } else {
                buffer_ = reinterpret_cast<uint8_t *>(malloc(bytes));
                if (buffer_ != nullptr) {
                    bufferCapacity_ = bytes;
                    return true;
                } else {
                    return false;
                }
            }
        }
        if (newSize > bufferCapacity_) {
            if (!ExpandBuffer(newSize)) {
                return false;
            }
        }
        return true;
    }

    ssize_t RawDataEmit(const void *data, size_t length)
    {
        return RawDataEmit(data, length, bufferSize_);
    }

    ssize_t RawDataEmit(const void *data, size_t length, size_t offset)
    {
        if (length <= 0) {
            return -1;
        }
        if ((offset + length) > bufferCapacity_) {
            if (!AllocateBuffer(length)) {
                return -1;
            }
        }
        if (memcpy_s(buffer_ + offset, bufferCapacity_ - offset, data, length) != EOK) {
            LOG_FULL(ERROR) << "Failed to memcpy_s Data";
            return -1;
        }
        if (UNLIKELY(offset > std::numeric_limits<ssize_t>::max())) {
            return -1;
        }
        ssize_t res = static_cast<ssize_t>(offset);
        if (bufferSize_ == offset) {
            bufferSize_ += length;
        }
        return res;
    }

    void EmitChar(uint8_t c)
    {
        RawDataEmit(&c, U8_SIZE);
    }

    ssize_t EmitU64(uint64_t c)
    {
        return RawDataEmit(reinterpret_cast<uint8_t *>(&c), U64_SIZE);
    }

    ssize_t EmitU64(uint64_t c, size_t offset)
    {
        return RawDataEmit(reinterpret_cast<uint8_t *>(&c), U64_SIZE, offset);
    }

    void WriteUint8(uint8_t data)
    {
        RawDataEmit(&data, 1);
    }

    uint8_t ReadUint8(size_t &position)
    {
        ASSERT(position < Size());
        return *(buffer_ + (position++));
    }

    void WriteEncodeFlag(EncodeFlag flag)
    {
        EmitChar(static_cast<uint8_t>(flag));
    }

    void WriteUint32(uint32_t data)
    {
        RawDataEmit(reinterpret_cast<uint8_t *>(&data), U32_SIZE);
    }

    uint32_t ReadUint32(size_t &position)
    {
        ASSERT(position < Size());
        uint32_t value = *reinterpret_cast<uint32_t *>(buffer_ + position);
        position += sizeof(uint32_t);
        return value;
    }

    void WriteRawData(uint8_t *data, size_t length)
    {
        RawDataEmit(data, length);
    }

    void WriteJSTaggedValue(JSTaggedValue value)
    {
        EmitU64(value.GetRawData());
    }

    ssize_t WriteJSTaggedType(JSTaggedType value)
    {
        return EmitU64(value);
    }

    JSTaggedType ReadJSTaggedType(size_t &position)
    {
        ASSERT(position < Size());
        JSTaggedType value = *reinterpret_cast<uint64_t *>(buffer_ + position);
        position += sizeof(JSTaggedType);
        return value;
    }

    void ReadRawData(uintptr_t addr, size_t len, size_t &position)
    {
        ASSERT(position + len <= Size());
        if (memcpy_s(reinterpret_cast<void *>(addr), len, buffer_ + position, len) != EOK) {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
        position += len;
    }

    uint8_t* Data() const
    {
        return buffer_;
    }

    size_t Size() const
    {
        return bufferSize_;
    }

    void SetIncompleteData(bool incomplete)
    {
        incompleteData_ = incomplete;
    }

    bool IsIncompleteData() const
    {
        return incompleteData_;
    }

    const std::array<std::vector<size_t>, SERIALIZE_SPACE_NUM>& GetRegionRemainSizeVectors() const
    {
        return regionRemainSizeVectors_;
    }

    size_t GetOldSpaceSize() const
    {
        return oldSpaceSize_;
    }

    size_t GetNonMovableSpaceSize() const
    {
        return nonMovableSpaceSize_;
    }

    size_t GetMachineCodeSpaceSize() const
    {
        return machineCodeSpaceSize_;
    }

    size_t GetSharedOldSpaceSize() const
    {
        return sharedOldSpaceSize_;
    }

    size_t GetSharedNonMovableSpaceSize() const
    {
        return sharedNonMovableSpaceSize_;
    }

    void CalculateSerializedObjectSize(SerializedObjectSpace space, size_t objectSize)
    {
        switch (space) {
            case SerializedObjectSpace::OLD_SPACE:
                AlignSpaceObjectSize(oldSpaceSize_, objectSize, SerializedObjectSpace::OLD_SPACE);
                break;
            case SerializedObjectSpace::NON_MOVABLE_SPACE:
                AlignSpaceObjectSize(nonMovableSpaceSize_, objectSize, SerializedObjectSpace::NON_MOVABLE_SPACE);
                break;
            case SerializedObjectSpace::MACHINE_CODE_SPACE:
                AlignSpaceObjectSize(machineCodeSpaceSize_, objectSize, SerializedObjectSpace::MACHINE_CODE_SPACE);
                break;
            case SerializedObjectSpace::SHARED_OLD_SPACE:
                AlignSpaceObjectSize(sharedOldSpaceSize_, objectSize, SerializedObjectSpace::SHARED_OLD_SPACE);
                break;
            case SerializedObjectSpace::SHARED_NON_MOVABLE_SPACE:
                AlignSpaceObjectSize(sharedNonMovableSpaceSize_, objectSize,
                                     SerializedObjectSpace::SHARED_NON_MOVABLE_SPACE);
                break;
            default:
                break;
        }
    }

    void AlignSpaceObjectSize(size_t &spaceSize, size_t objectSize, SerializedObjectSpace spaceType)
    {
        size_t alignRegionSize = AlignUpRegionAvailableSize(spaceSize);
        if (UNLIKELY(spaceSize + objectSize > alignRegionSize)) {
            regionRemainSizeVectors_.at(static_cast<uint8_t>(spaceType)).push_back(alignRegionSize - spaceSize);
            spaceSize = alignRegionSize;
        }
        spaceSize += objectSize;
        ASSERT(spaceSize <= SnapshotEnv::MAX_UINT_32);
    }

    void DecreaseSharedArrayBufferReference()
    {
        auto manager = JSSharedMemoryManager::GetInstance();
        for (auto iter = sharedArrayBufferSet_.begin(); iter != sharedArrayBufferSet_.end(); iter++) {
            JSSharedMemoryManager::RemoveSharedMemory(thread_->GetEnv(), reinterpret_cast<void *>(*iter), manager);
        }
        sharedArrayBufferSet_.clear();
    }

    void insertSharedArrayBuffer(uintptr_t ptr)
    {
        sharedArrayBufferSet_.insert(ptr);
    }

    void SetDataIndex(uint32_t dataIndex)
    {
        dataIndex_ = dataIndex;
    }

    uint32_t GetDataIndex() const
    {
        return dataIndex_;
    }

    void AddNativeBindingDetachInfo(panda::JSNApi::NativeBindingInfo *info, void *dObj)
    {
        auto *detachInfo = new NativeBindingDetachInfo(info->detachedFinalizer, dObj, info->detachedHint);
        nativeBindingDetachInfos_.insert(detachInfo);
    }

private:
    static constexpr size_t U8_SIZE = 1;
    static constexpr size_t U16_SIZE = 2;
    static constexpr size_t U32_SIZE = 4;
    static constexpr size_t U64_SIZE = 8;
    JSThread *thread_;
    uint32_t dataIndex_ {RESERVED_INDEX};
    uint8_t *buffer_ {nullptr};
    uint64_t sizeLimit_ {0};
    size_t bufferSize_ {0};
    size_t bufferCapacity_ {0};
    size_t oldSpaceSize_ {0};
    size_t nonMovableSpaceSize_ {0};
    size_t machineCodeSpaceSize_ {0};
    size_t sharedOldSpaceSize_ {0};
    size_t sharedNonMovableSpaceSize_ {0};
    bool incompleteData_ {false};
    std::array<std::vector<size_t>, SERIALIZE_SPACE_NUM> regionRemainSizeVectors_ {};
    std::set<uintptr_t> sharedArrayBufferSet_ {};
    std::set<NativeBindingDetachInfo *> nativeBindingDetachInfos_ {};
};
}

#endif  // ECMASCRIPT_SERIALIZER_SERIALIZE_DATA_H
