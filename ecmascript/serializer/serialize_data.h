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

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/dyn_chunk.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"

namespace panda::ecmascript {
enum class EncodeFlag : uint8_t {
    // 0x00~0x03 represent new object to different space:
    // 0x00: old space
    // 0x01: non movable space
    // 0x02: machine code space
    // 0x03: huge space
    NEW_OBJECT = 0x00,
    REFERENCE = 0x04,
    WEAK,
    PRIMITIVE,
    MULTI_RAW_DATA,
    ROOT_OBJECT,
    OBJECT_PROTO,
    ARRAY_BUFFER,
    TRANSFER_ARRAY_BUFFER,
    METHOD,
    NATIVE_BINDING_OBJECT,
    JS_ERROR,
    JS_REG_EXP,
    LAST
};

enum class SerializedObjectSpace : uint8_t {
    OLD_SPACE = 0,
    NON_MOVABLE_SPACE,
    MACHINE_CODE_SPACE,
    HUGE_SPACE
};

enum class SerializeType : uint8_t {
    VALUE_SERIALIZE,
    PGO_SERIALIZE
};

class SerializeData {
public:
    explicit SerializeData(JSThread *thread) : chunk_(thread->GetNativeAreaAllocator()), data_(&chunk_) {}
    ~SerializeData()
    {
        regionRemainSizeVector_.clear();
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

    void WriteUint8(uint8_t data)
    {
        data_.EmitChar(data);
    }

    uint8_t ReadUint8()
    {
        ASSERT(position_ < Size());
        return data_.GetU8(position_++);
    }

    void WriteEncodeFlag(EncodeFlag flag)
    {
        data_.EmitChar(static_cast<uint8_t>(flag));
    }

    void WriteUint32(uint32_t data)
    {
        data_.EmitU32(data);
    }

    uint32_t ReadUint32()
    {
        ASSERT(position_ < Size());
        uint32_t value = data_.GetU32(position_);
        position_ += sizeof(uint32_t);
        return value;
    }

    void WriteRawData(uint8_t *data, size_t length)
    {
        data_.Emit(data, length);
    }

    void WriteJSTaggedValue(JSTaggedValue value)
    {
        data_.EmitU64(value.GetRawData());
    }

    void WriteJSTaggedType(JSTaggedType value)
    {
        data_.EmitU64(value);
    }

    JSTaggedType ReadJSTaggedType()
    {
        ASSERT(position_ < Size());
        JSTaggedType value = data_.GetU64(position_);
        position_ += sizeof(JSTaggedType);
        return value;
    }

    void ReadRawData(uintptr_t addr, size_t len)
    {
        ASSERT(position_ + len <= Size());
        if (memcpy_s(reinterpret_cast<void *>(addr), len, data_.GetBegin() + position_, len) != EOK) {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
        position_ += len;
    }

    uint8_t* Data() const
    {
        return data_.GetBegin();
    }

    size_t Size() const
    {
        return data_.GetSize();
    }

    size_t GetPosition() const
    {
        return position_;
    }

    void SetIncompleteData(bool incomplete)
    {
        incompleteData_ = incomplete;
    }

    bool IsIncompleteData() const
    {
        return incompleteData_;
    }

    const std::vector<size_t>& GetRegionRemainSizeVector() const
    {
        return regionRemainSizeVector_;
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

    void CalculateSerializedObjectSize(SerializedObjectSpace space, size_t objectSize)
    {
        switch (space) {
            case SerializedObjectSpace::OLD_SPACE:
                AlignSpaceObjectSize(oldSpaceSize_, objectSize);
                break;
            case SerializedObjectSpace::NON_MOVABLE_SPACE:
                AlignSpaceObjectSize(nonMovableSpaceSize_, objectSize);
                break;
            case SerializedObjectSpace::MACHINE_CODE_SPACE:
                AlignSpaceObjectSize(machineCodeSpaceSize_, objectSize);
                break;
            default:
                break;
        }
    }

    void AlignSpaceObjectSize(size_t &spaceSize, size_t objectSize)
    {
        size_t alignRegionSize = AlignUpRegionAvailableSize(spaceSize);
        if (UNLIKELY(spaceSize + objectSize > alignRegionSize)) {
            regionRemainSizeVector_.push_back(alignRegionSize - spaceSize);
            spaceSize = alignRegionSize;
        }
        spaceSize += objectSize;
        ASSERT(spaceSize <= SnapshotEnv::MAX_UINT_32);
    }

    void ResetPosition()
    {
        position_ = 0;
    }

private:
    Chunk chunk_;
    DynChunk data_;
    size_t oldSpaceSize_ {0};
    size_t nonMovableSpaceSize_ {0};
    size_t machineCodeSpaceSize_ {0};
    size_t position_ {0};
    bool incompleteData_ {false};
    std::vector<size_t> regionRemainSizeVector_;
};
}

#endif  // ECMASCRIPT_SERIALIZER_SERIALIZE_DATA_H