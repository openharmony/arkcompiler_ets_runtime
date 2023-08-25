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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_RECORD_POOL_H
#define ECMASCRIPT_PGO_PROFILER_PGO_RECORD_POOL_H

#include <cstdint>
#include <fstream>
#include <unordered_map>

#include "ecmascript/common.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/pgo_file_info.h"
#include "macros.h"

namespace panda::ecmascript {

class PGOProfilerHeader;
class PGORecord {
public:
    explicit PGORecord(CString name) : name_(std::move(name)) {}
    PGORecordId GetId() const
    {
        return id_;
    }

    void SetId(PGORecordId id)
    {
        id_ = id;
    }

    const CString &GetRecordName() const
    {
        return name_;
    }

private:
    PGORecordId id_ {};
    CString name_;
};

class PGORecordPool : public PGOFileDataInterface {
public:
    PGORecordPool() = default;
    ~PGORecordPool() override
    {
        Clear();
    }

    PGORecord *TryAdd(const CString &recordName)
    {
        for (auto &record : recordIdMapping_) {
            if (record.second.GetRecordName() == recordName) {
                return &(record.second);
            }
        }
        PGORecordId recordId(recordIdMapping_.size());

        auto result = recordIdMapping_.emplace(recordId, recordName);
        auto &record = result.first->second;
        record.SetId(recordId);
        return &(result.first->second);
    }

    bool GetRecordId(const CString &recordName, PGORecordId &recordId) const
    {
        for (auto [id, record] : recordIdMapping_) {
            if (record.GetRecordName() == recordName) {
                recordId = id;
                return true;
            }
        }
        return false;
    }

    const PGORecord *GetRecord(PGORecordId id) const
    {
        auto iter = recordIdMapping_.find(id);
        if (iter == recordIdMapping_.end()) {
            return nullptr;
        }
        return &(iter->second);
    }

    void Clear()
    {
        recordIdMapping_.clear();
    }

    void Merge(const PGORecordPool &recordPool, std::map<PGORecordId, PGORecordId> &idMapping)
    {
        for (auto [id, record] : recordPool.recordIdMapping_) {
            auto *newRecord = TryAdd(record.GetRecordName());
            idMapping.emplace(id, newRecord->GetId());
        }
    }

    uint32_t ProcessToBinary(std::fstream &stream) override
    {
        SectionInfo secInfo;
        secInfo.number_ = recordIdMapping_.size();
        secInfo.offset_ = sizeof(SectionInfo);
        auto secInfoPos = stream.tellp();
        stream.seekp(secInfo.offset_, std::ofstream::cur);
        for (const auto &record : recordIdMapping_) {
            stream.write(reinterpret_cast<const char *>(&(record.first)), sizeof(PGORecordId));
            stream << record.second.GetRecordName() << '\0';
        }
        secInfo.size_ = static_cast<uint32_t>(stream.tellp()) - secInfoPos;
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
        for (auto [id, record] : recordIdMapping_) {
            if (isFirst) {
                profilerString += PGODumpUtils::NEW_LINE;
                profilerString += "RecordPool";
                profilerString += PGODumpUtils::BLOCK_START;
                isFirst = false;
            }
            profilerString += PGODumpUtils::NEW_LINE;
            profilerString += std::to_string(id.GetOffset());
            profilerString += PGODumpUtils::ELEMENT_SEPARATOR;
            profilerString += record.GetRecordName();
        }
        if (!isFirst) {
            profilerString += (PGODumpUtils::SPACE + PGODumpUtils::NEW_LINE);
            stream << profilerString;
        }
        return true;
    }

    uint32_t ParseFromBinary(void **buffer, [[maybe_unused]] PGOProfilerHeader *const header) override
    {
        auto *startBuffer = *buffer;
        auto secInfo = base::ReadBuffer<SectionInfo>(buffer);
        for (uint32_t i = 0; i < secInfo.number_; i++) {
            auto recordId = base::ReadBuffer<PGORecordId>(buffer, sizeof(PGORecordId));
            auto *recordName = base::ReadBuffer(buffer);
            auto result = recordIdMapping_.emplace(recordId, recordName);
            result.first->second.SetId(recordId);
        }
        return reinterpret_cast<uintptr_t>(*buffer) - reinterpret_cast<uintptr_t>(startBuffer);
    }

private:
    NO_COPY_SEMANTIC(PGORecordPool);
    NO_MOVE_SEMANTIC(PGORecordPool);
    std::unordered_map<PGORecordId, PGORecord> recordIdMapping_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILER_PGO_RECORD_POOL_H
