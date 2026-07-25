/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/serializer/serialize_data.h"

#include <array>
#include <vector>

#include "ecmascript/base/config.h"
#include "securec.h"

namespace panda::ecmascript {
// Flat buffer layout index constants for the size group array.
static constexpr int BUFFER_SIZE_INDEX = 0;
static constexpr int BUFFER_CAPACITY_INDEX = 1;
static constexpr int REGULAR_SPACE_SIZE_INDEX = 2;
static constexpr int PIN_SPACE_SIZE_INDEX = 3;
static constexpr int OLD_SPACE_SIZE_INDEX = 4;
static constexpr int HUGE_SPACE_SIZE_INDEX = 5;
static constexpr int NONMOVABLE_SPACE_SIZE_INDEX = 6;
static constexpr int MACHINECODE_SPACE_SIZE_INDEX = 7;
static constexpr int SHARED_OLD_SPACE_SIZE_INDEX = 8;
static constexpr int SHARED_NONMOVABLE_SPACE_SIZE_INDEX = 9;
static constexpr int INCOMPLETE_DATA_INDEX = 10;
static constexpr int GROUP_SIZE = 11;
static constexpr int CMC_GC_REGION_SIZE = 2;

static size_t GetAlignUpPadding(const uint8_t *curPtr, void *originAddr, const size_t alignment)
{
    const auto originPtr = static_cast<uint8_t *>(originAddr);
    return AlignUp(curPtr - originPtr, alignment) - (curPtr - originPtr);
}

static bool WriteAndAdvance(uint8_t *&ptr, uint8_t *buffer, size_t totalSize, const void *src, size_t bytes)
{
    if (memcpy_s(ptr, totalSize - (ptr - buffer), src, bytes) != EOK) {
        return false;
    }
    ptr += bytes;
    return true;
}

static bool PadAlignment(uint8_t *&ptr, uint8_t *buffer, size_t alignment)
{
    size_t padding = GetAlignUpPadding(ptr, buffer, alignment);
    if (padding == 0) {
        return true;
    }
    if (memset_s(ptr, padding, 0, padding) != EOK) {
        return false;
    }
    ptr += padding;
    return true;
}

static bool ReadAndAdvance(const uint8_t *&ptr, void *dst, size_t bytes)
{
    if (memcpy_s(dst, bytes, ptr, bytes) != EOK) {
        return false;
    }
    ptr += bytes;
    return true;
}

uint8_t* SerializeData::Pack(std::unique_ptr<SerializeData> &data, size_t &outSize)
{
    if (data == nullptr) {
        outSize = 0;
        return nullptr;
    }

    size_t bufferSize_ = CalculateFlatBufferSize(data.get());
    if (bufferSize_ <= 0) {
        outSize = 0;
        return nullptr;
    }
    void *rawBuffer = malloc(bufferSize_);
    if (rawBuffer == nullptr) {
        outSize = 0;
        return nullptr;
    }
    uint8_t *buffer = static_cast<uint8_t *>(rawBuffer);

    uint8_t *ptr = buffer;
    bool ok = WriteFlatHeader(ptr, buffer, bufferSize_, data.get()) &&
              WriteFlatSizeGroup(ptr, buffer, bufferSize_, data.get()) &&
              WriteFlatRegionSizes(ptr, buffer, bufferSize_, data.get()) &&
              WriteFlatBufferData(ptr, buffer, bufferSize_, data.get());
    if (!ok) {
        free(buffer);
        outSize = 0;
        return nullptr;
    }

    // Reset dataIndex_ on the source object so its destructor does not release
    // any serialization root that may have been registered in the source process.
    // In cross-process scenarios the data is rebuilt via Unpack() in a different
    // process where the original serialization root index would be meaningless,
    // so the unpacked copy must not attempt RemoveSerializationRoot either.
    // For FileSerializer (the intended producer for flat buffers) dataIndex_ is
    // always RESERVED_INDEX, so this assignment is a no-op.
    data->dataIndex_ = RESERVED_INDEX;

    outSize = bufferSize_;
    return buffer;
}

size_t SerializeData::CalculateFlatBufferSize(const SerializeData *data)
{
    size_t totalSize = sizeof(data->dataIndex_);
    totalSize = AlignUp(totalSize, sizeof(uint64_t)) + sizeof(data->sizeLimit_);
    totalSize = AlignUp(totalSize, sizeof(size_t));
    totalSize += GROUP_SIZE * sizeof(size_t);

    if (g_isEnableCMCGC) {
        totalSize += CMC_GC_REGION_SIZE * sizeof(size_t);
        totalSize += data->regularRemainSizeVector_.size() * sizeof(size_t);
        totalSize += data->pinRemainSizeVector_.size() * sizeof(size_t);
    } else {
        totalSize += SERIALIZE_SPACE_NUM * sizeof(size_t);
        for (const auto& vec : data->regionRemainSizeVectors_) {
            totalSize += vec.size() * sizeof(size_t);
        }
    }
    totalSize += data->bufferSize_;
    return totalSize;
}

bool SerializeData::WriteFlatHeader(uint8_t *&ptr, uint8_t *buffer, size_t totalSize, const SerializeData *data)
{
    if (!WriteAndAdvance(ptr, buffer, totalSize, &data->dataIndex_, sizeof(data->dataIndex_))) {
        return false;
    }
    if (!PadAlignment(ptr, buffer, sizeof(uint64_t))) {
        return false;
    }
    if (!WriteAndAdvance(ptr, buffer, totalSize, &data->sizeLimit_, sizeof(data->sizeLimit_))) {
        return false;
    }
    return PadAlignment(ptr, buffer, sizeof(size_t));
}

bool SerializeData::WriteFlatSizeGroup(uint8_t *&ptr, uint8_t *buffer, size_t totalSize, const SerializeData *data)
{
    size_t sizeGroup[GROUP_SIZE] = {
        data->bufferSize_,
        data->bufferCapacity_,
        data->regularSpaceSize_,
        data->pinSpaceSize_,
        data->oldSpaceSize_,
        data->hugeSpaceSize_,
        data->nonMovableSpaceSize_,
        data->machineCodeSpaceSize_,
        data->sharedOldSpaceSize_,
        data->sharedNonMovableSpaceSize_,
        static_cast<size_t>(data->incompleteData_)
    };
    size_t groupBytes = GROUP_SIZE * sizeof(size_t);
    return WriteAndAdvance(ptr, buffer, totalSize, sizeGroup, groupBytes);
}

bool SerializeData::WriteFlatRegionSizes(uint8_t *&ptr, uint8_t *buffer, size_t totalSize, const SerializeData *data)
{
    if (g_isEnableCMCGC) {
        auto writeSizeVec = [&](const std::vector<size_t> &vec) -> bool {
            size_t vecSize = vec.size();
            if (!WriteAndAdvance(ptr, buffer, totalSize, &vecSize, sizeof(vecSize))) {
                return false;
            }
            if (vecSize == 0) {
                return true;
            }
            return WriteAndAdvance(ptr, buffer, totalSize, vec.data(), vecSize * sizeof(size_t));
        };
        return writeSizeVec(data->regularRemainSizeVector_) && writeSizeVec(data->pinRemainSizeVector_);
    }

    std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes;
    for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
        vecSizes[i] = data->regionRemainSizeVectors_[i].size();
    }
    uint32_t vecBytes = SERIALIZE_SPACE_NUM * sizeof(size_t);
    if (!WriteAndAdvance(ptr, buffer, totalSize, vecSizes.data(), vecBytes)) {
        return false;
    }
    for (const auto& vec : data->regionRemainSizeVectors_) {
        if (vec.empty()) {
            continue;
        }
        uint32_t curVecBytes = vec.size() * sizeof(size_t);
        if (!WriteAndAdvance(ptr, buffer, totalSize, vec.data(), curVecBytes)) {
            return false;
        }
    }
    return true;
}

bool SerializeData::WriteFlatBufferData(uint8_t *&ptr, uint8_t *buffer, size_t totalSize, const SerializeData *data)
{
    if (data->bufferSize_ == 0) {
        return true;
    }
    if (data->buffer_ == nullptr) {
        return false;
    }
    return memcpy_s(ptr, totalSize - (ptr - buffer), data->buffer_, data->bufferSize_) == EOK;
}

std::unique_ptr<SerializeData> SerializeData::Unpack(JSThread *thread, const uint8_t *recorder)
{
    if (recorder == nullptr) {
        return nullptr;
    }
    std::unique_ptr<SerializeData> data = std::make_unique<SerializeData>(thread);
    const uint8_t *ptr = recorder;

    if (!data->ReadFlatHeader(ptr, recorder) ||
        !data->ReadFlatSizeGroup(ptr) ||
        data->incompleteData_ ||
        !data->ReadFlatRegionSizes(ptr) ||
        !data->ReadFlatBufferData(ptr)) {
        return nullptr;
    }
    return data;
}

bool SerializeData::ReadFlatHeader(const uint8_t *&ptr, const uint8_t *base)
{
    uint32_t packedDataIndex = RESERVED_INDEX;
    if (!ReadAndAdvance(ptr, &packedDataIndex, sizeof(packedDataIndex))) {
        return false;
    }
    // Cross-process safety: discard any source-process serialization root
    // index. The current process has no entry for it, and BaseDeserializer
    // relies on dataIndex_ == 0 to leave sharedObjChunk_ null. FileSerializer
    // already produces RESERVED_INDEX so this is a no-op for the intended
    // producer; the override only protects against unexpected producers.
    (void)packedDataIndex;
    dataIndex_ = RESERVED_INDEX;

    ptr += GetAlignUpPadding(ptr, const_cast<uint8_t *>(base), sizeof(uint64_t));
    if (!ReadAndAdvance(ptr, &sizeLimit_, sizeof(sizeLimit_))) {
        return false;
    }
    ptr += GetAlignUpPadding(ptr, const_cast<uint8_t *>(base), sizeof(size_t));
    return true;
}

bool SerializeData::ReadFlatSizeGroup(const uint8_t *&ptr)
{
    size_t sizeGroup[GROUP_SIZE];
    if (!ReadAndAdvance(ptr, sizeGroup, GROUP_SIZE * sizeof(size_t))) {
        return false;
    }
    bufferSize_ = sizeGroup[BUFFER_SIZE_INDEX];
    bufferCapacity_ = sizeGroup[BUFFER_CAPACITY_INDEX];
    regularSpaceSize_ = sizeGroup[REGULAR_SPACE_SIZE_INDEX];
    pinSpaceSize_ = sizeGroup[PIN_SPACE_SIZE_INDEX];
    oldSpaceSize_ = sizeGroup[OLD_SPACE_SIZE_INDEX];
    hugeSpaceSize_ = sizeGroup[HUGE_SPACE_SIZE_INDEX];
    nonMovableSpaceSize_ = sizeGroup[NONMOVABLE_SPACE_SIZE_INDEX];
    machineCodeSpaceSize_ = sizeGroup[MACHINECODE_SPACE_SIZE_INDEX];
    sharedOldSpaceSize_ = sizeGroup[SHARED_OLD_SPACE_SIZE_INDEX];
    sharedNonMovableSpaceSize_ = sizeGroup[SHARED_NONMOVABLE_SPACE_SIZE_INDEX];
    incompleteData_ = sizeGroup[INCOMPLETE_DATA_INDEX] != 0;
    return true;
}

bool SerializeData::ReadFlatRegionSizes(const uint8_t *&ptr)
{
    if (g_isEnableCMCGC) {
        auto readOneVec = [&](std::vector<size_t> &vec) -> bool {
            size_t vecSize = 0;
            if (!ReadAndAdvance(ptr, &vecSize, sizeof(vecSize))) {
                return false;
            }
            if (vecSize == 0) {
                vec.clear();
                return true;
            }
            vec.resize(vecSize);
            return ReadAndAdvance(ptr, vec.data(), vecSize * sizeof(size_t));
        };
        return readOneVec(regularRemainSizeVector_) && readOneVec(pinRemainSizeVector_);
    }

    std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes {};
    uint32_t vecBytes = SERIALIZE_SPACE_NUM * sizeof(size_t);
    if (!ReadAndAdvance(ptr, vecSizes.data(), vecBytes)) {
        return false;
    }
    for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
        auto& vec = regionRemainSizeVectors_[i];
        const size_t curVectorSize = vecSizes[i];
        if (curVectorSize == 0) {
            vec.clear();
            continue;
        }
        vec.resize(curVectorSize);
        if (!ReadAndAdvance(ptr, vec.data(), curVectorSize * sizeof(size_t))) {
            return false;
        }
    }
    return true;
}

bool SerializeData::ReadFlatBufferData(const uint8_t *&ptr)
{
    if (bufferSize_ <= 0) {
        buffer_ = nullptr;
        return true;
    }
    void *rawBuffer = malloc(bufferSize_);
    if (rawBuffer == nullptr) {
        return false;
    }
    buffer_ = static_cast<uint8_t *>(rawBuffer);
    if (memcpy_s(buffer_, bufferSize_, ptr, bufferSize_) != EOK) {
        free(buffer_);
        return false;
    }
    return true;
}
}  // namespace panda::ecmascript
