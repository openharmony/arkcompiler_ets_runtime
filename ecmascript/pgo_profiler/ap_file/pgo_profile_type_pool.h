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

#ifndef ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_PROFILE_TYPE_POOL_H
#define ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_PROFILE_TYPE_POOL_H

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>

#include "ecmascript/common.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class PGOProfileTypePool : public PGOFileDataInterface {
public:
    class Entry {
    public:
        explicit Entry(ProfileType type) : type_(type) {}
        ApEntityId GetEntryId() const
        {
            return id_;
        }

        void SetId(ApEntityId id)
        {
            id_ = id;
        }

        const ProfileType &GetProfileType() const
        {
            return type_;
        }

    private:
        ApEntityId id_ {};
        ProfileType type_;
    };

    enum class ReservedType : uint8_t {
        NONE_CLASS_TYPE_REF = 0,
        END
    };
    static constexpr uint32_t RESERVED_COUNT = 64;

    static_assert(static_cast<uint32_t>(ReservedType::END) < RESERVED_COUNT);

    PGOProfileTypePool() = default;
    ~PGOProfileTypePool() override
    {
        Clear();
    }

    bool TryAdd(const ProfileType &profileType, ApEntityId &id)
    {
        for (auto &record : pool_) {
            if (record.second.GetProfileType() == profileType) {
                id = record.second.GetEntryId();
                return true;
            }
        }

        id = ApEntityId(IsReserved(profileType) ? GetReservedId(profileType).GetOffset()
                                                      : RESERVED_COUNT + pool_.size());

        auto result = pool_.emplace(id, profileType);
        auto &record = result.first->second;
        record.SetId(id);
        return true;
    }

    bool GetRecordId(const ProfileType &profileType, ApEntityId &recordId) const
    {
        for (auto [id, record] : pool_) {
            if (record.GetProfileType() == profileType) {
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

    void Merge(const PGOProfileTypePool &recordPool, std::map<ApEntityId, ApEntityId> &idMapping)
    {
        for (auto [id, record] : recordPool.pool_) {
            ApEntityId apId;
            TryAdd(record.GetProfileType(), apId);
            idMapping.emplace(id, apId);
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
            auto profileType = record.second.GetProfileType().GetRaw();
            stream.write(reinterpret_cast<const char *>(&(profileType)), sizeof(ProfileType));
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
                profilerString += "ProfileTypePool";
                profilerString += DumpUtils::BLOCK_START;
                isFirst = false;
            }
            profilerString += DumpUtils::NEW_LINE;
            profilerString += std::to_string(id.GetOffset());
            profilerString += DumpUtils::ELEMENT_SEPARATOR;
            profilerString += record.GetProfileType().GetTypeString();
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
            auto profileType = base::ReadBuffer<ProfileType>(buffer, sizeof(ProfileType));
            auto result = pool_.emplace(recordId, profileType);
            result.first->second.SetId(recordId);
        }
        return reinterpret_cast<uintptr_t>(*buffer) - reinterpret_cast<uintptr_t>(startBuffer);
    }

private:
    NO_COPY_SEMANTIC(PGOProfileTypePool);
    NO_MOVE_SEMANTIC(PGOProfileTypePool);

    static bool IsReserved(ProfileType type)
    {
        return type.IsNone();
    }

    static ApEntityId GetReservedId([[maybe_unused]] ProfileType type)
    {
        ASSERT(type.IsNone());
        return ApEntityId(static_cast<uint32_t>(ReservedType::NONE_CLASS_TYPE_REF));
    }

    std::unordered_map<ApEntityId, Entry> pool_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_PROFILE_TYPE_POOL_H
