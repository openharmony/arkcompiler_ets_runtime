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
#include <memory>
#include <string>
#include <unordered_map>

#include "ecmascript/common.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pool_template.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::pgo {
class PGOProfileTypePool {
public:
    class Entry : public PGOFileDataInterface {
    public:
        explicit Entry(ProfileType type) : type_(type) {}
        Entry() = default;
        ApEntityId GetEntryId() const
        {
            return entryId_;
        }

        void SetEntryId(ApEntityId entryId)
        {
            entryId_ = entryId;
        }

        ProfileType GetProfileType() const
        {
            return type_;
        }

        const ProfileType &GetData() const
        {
            return type_;
        }

        uint32_t ProcessToBinary([[maybe_unused]] PGOContext &context, std::fstream &stream) override
        {
            auto profileType = type_.GetRaw();
            stream.write(reinterpret_cast<const char *>(&(profileType)), sizeof(ProfileType));
            return 1;
        }

        uint32_t ParseFromBinary([[maybe_unused]] PGOContext &context, void **buffer,
                                 [[maybe_unused]] PGOProfilerHeader const *header) override
        {
            type_ = base::ReadBuffer<ProfileType>(buffer, sizeof(ProfileType));
            return 1;
        }

        bool ProcessToText(std::ofstream &stream) override
        {
            stream << type_.GetTypeString();
            return true;
        }

        void Remap(PGOContext &context)
        {
            type_.Remap(context);
        }

    private:
        ApEntityId entryId_ {0};
        ProfileType type_;
    };

    using PoolType = PoolTemplate<Entry, ProfileType>;

    enum class ReservedType : uint8_t { NONE_CLASS_TYPE_REF = 0, END };
    static constexpr uint32_t RESERVED_COUNT = 64;

    static_assert(static_cast<uint32_t>(ReservedType::END) < RESERVED_COUNT);

    static bool IsReserved(const ProfileType &value)
    {
        return value.IsNone();
    }

    static ApEntityId GetReservedId([[maybe_unused]] const ProfileType &value)
    {
        ASSERT(value.IsNone());
        return ApEntityId(static_cast<uint32_t>(ReservedType::NONE_CLASS_TYPE_REF));
    }

    static bool Support(PGOProfilerHeader const *header)
    {
        return header->SupportWideProfileType();
    }

    static SectionInfo *GetSection(PGOProfilerHeader const *header)
    {
        return header->GetProfileTypeSection();
    }

    PGOProfileTypePool()
    {
        pool_ = std::make_shared<PoolType>("ProfileTypePool", RESERVED_COUNT);
        pool_->SetIsReservedCb(IsReserved);
        pool_->SetGetReservedIdCb(GetReservedId);
        pool_->SetGetSectionCb(GetSection);
        pool_->SetSupportCb(Support);
    }
    ~PGOProfileTypePool()
    {
        Clear();
    }

    bool TryAdd(const ProfileType &profileType, ApEntityId &entryId)
    {
        return pool_->TryAdd(profileType, entryId);
    }

    bool GetEntryId(const ProfileType &profileType, ApEntityId &entryId) const
    {
        return pool_->GetEntryId(profileType, entryId);
    }

    const Entry *GetEntry(ApEntityId entryId) const
    {
        return pool_->GetEntry(entryId);
    }

    void Clear()
    {
        pool_->Clear();
    }

    void Merge(const PGOProfileTypePool &profileTypePool, std::map<ApEntityId, ApEntityId> &idMapping)
    {
        pool_->Merge(*profileTypePool.pool_, [&](ApEntityId oldEntryId, ApEntityId newEntryId) {
            idMapping.try_emplace(oldEntryId, newEntryId);
        });
    }

    void Remap(PGOContext &context)
    {
        auto &valueToId = pool_->GetValueToId();
        valueToId.clear();
        for (auto &entry : pool_->GetPool()) {
            entry.second.Remap(context);
            valueToId[entry.second.GetData()] = entry.first;
        }
    }

    std::shared_ptr<PoolType> &GetPool()
    {
        return pool_;
    }

private:
    NO_COPY_SEMANTIC(PGOProfileTypePool);
    NO_MOVE_SEMANTIC(PGOProfileTypePool);

    std::shared_ptr<PoolType> pool_;
};
}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_PROFILE_TYPE_POOL_H
