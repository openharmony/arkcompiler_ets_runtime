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

#ifndef ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_RECORD_POOL_H
#define ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_RECORD_POOL_H

#include <cstdint>
#include <fstream>
#include <unordered_map>

#include "ecmascript/common.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class PGOProfilerHeader;

class PGORecordPool : public PGOFileDataInterface {
public:
    class Entry {
    public:
        explicit Entry(CString name) : name_(std::move(name)) {}
        ApEntityId GetEntryId() const
        {
            return id_;
        }

        void SetId(ApEntityId id)
        {
            id_ = id;
        }

        const CString &GetRecordName() const
        {
            return name_;
        }

    private:
        ApEntityId id_ {};
        CString name_;
    };
    enum class ReservedType : uint8_t {
        EMPTY_RECORD_ID = 0,
        END
    };
    static constexpr uint32_t RESERVED_COUNT = 64;

    static_assert(static_cast<uint32_t>(ReservedType::END) < RESERVED_COUNT);

    PGORecordPool() = default;
    ~PGORecordPool() override
    {
        Clear();
    }

    bool TryAdd(const CString &recordName, ApEntityId &id)
    {
        for (auto &record : pool_) {
            if (record.second.GetRecordName() == recordName) {
                id = record.second.GetEntryId();
                return true;
            }
        }

        id = ApEntityId(IsReserved(recordName) ? GetReservedId(recordName).GetOffset()
                                               : RESERVED_COUNT + pool_.size());

        auto result = pool_.emplace(id, recordName);
        auto &record = result.first->second;
        record.SetId(id);
        return true;
    }

    bool GetRecordId(const CString &recordName, ApEntityId &recordId) const
    {
        for (auto [id, record] : pool_) {
            if (record.GetRecordName() == recordName) {
                recordId = id;
                return true;
            }
        }
        return false;
    }

    const Entry *GetRecord(ApEntityId id) const
    {
        auto iter = pool_.find(id);
        if (iter == pool_.end()) {
            return nullptr;
        }
        return &(iter->second);
    }

    void Clear()
    {
        pool_.clear();
    }

    void Merge(const PGORecordPool &recordPool, std::map<ApEntityId, ApEntityId> &idMapping)
    {
        for (auto [oldId, record] : recordPool.pool_) {
            ApEntityId newId;
            TryAdd(record.GetRecordName(), newId);
            idMapping.emplace(oldId, newId);
        }
    }

    uint32_t ProcessToBinary(std::fstream &stream) override
    {
        SectionInfo secInfo;
        secInfo.number_ = pool_.size();
        secInfo.offset_ = sizeof(SectionInfo);
        auto secInfoPos = stream.tellp();
        stream.seekp(secInfo.offset_, std::ofstream::cur);
        for (const auto &record : pool_) {
            stream.write(reinterpret_cast<const char *>(&(record.first)), sizeof(ApEntityId));
            stream << record.second.GetRecordName() << '\0';
        }
        secInfo.size_ = static_cast<uint32_t>(stream.tellp()) - static_cast<uint32_t>(secInfoPos);
        auto tail = stream.tellp();
        stream.seekp(secInfoPos, std::ofstream::beg);
        stream.write(reinterpret_cast<const char *>(&(secInfo)), sizeof(SectionInfo));
        stream.seekp(tail, std::ofstream::beg);
        return tail - secInfoPos;
    }

    bool ProcessToText(std::ofstream &stream) override
    {
        std::string profilerString;
        bool isFirst = true;
        for (auto [id, record] : pool_) {
            if (isFirst) {
                profilerString += DumpUtils::NEW_LINE;
                profilerString += "RecordPool";
                profilerString += DumpUtils::BLOCK_START;
                isFirst = false;
            }
            profilerString += DumpUtils::NEW_LINE;
            profilerString += std::to_string(id.GetOffset());
            profilerString += DumpUtils::ELEMENT_SEPARATOR;
            profilerString += record.GetRecordName();
        }
        if (!isFirst) {
            profilerString += (DumpUtils::SPACE + DumpUtils::NEW_LINE);
            stream << profilerString;
        }
        return true;
    }

    uint32_t ParseFromBinary(void **buffer, [[maybe_unused]] PGOProfilerHeader *const header) override
    {
        auto *startBuffer = *buffer;
        auto secInfo = base::ReadBuffer<SectionInfo>(buffer);
        for (uint32_t i = 0; i < secInfo.number_; i++) {
            auto recordId = base::ReadBuffer<ApEntityId>(buffer, sizeof(ApEntityId));
            auto *recordName = base::ReadBuffer(buffer);
            auto result = pool_.emplace(recordId, recordName);
            result.first->second.SetId(recordId);
        }
        return reinterpret_cast<uintptr_t>(*buffer) - reinterpret_cast<uintptr_t>(startBuffer);
    }

private:
    NO_COPY_SEMANTIC(PGORecordPool);
    NO_MOVE_SEMANTIC(PGORecordPool);

    static bool IsReserved(const CString &recordName)
    {
        return recordName.empty();
    }

    static ApEntityId GetReservedId([[maybe_unused]]const CString &recordName)
    {
        ASSERT(recordName.empty());
        return ApEntityId(static_cast<uint32_t>(ReservedType::EMPTY_RECORD_ID));
    }

    std::unordered_map<ApEntityId, Entry> pool_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_RECORD_POOL_H
