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
#include <memory>

#include "ecmascript/common.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pool_template.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class PGOProfilerHeader;

class PGOStringPool {
public:
    class Entry : public PGOFileDataInterface {
    public:
        explicit Entry(CString name) : name_(std::move(name)) {}
        Entry() = default;
        ApEntityId GetEntryId() const
        {
            return entryId_;
        }

        void SetEntryId(ApEntityId entryId)
        {
            entryId_ = entryId;
        }

        const CString &GetData() const
        {
            return name_;
        }

        uint32_t ProcessToBinary([[maybe_unused]] PGOContext &context, std::fstream &stream) override
        {
            stream << name_ << '\0';
            return 1;
        }

        uint32_t ParseFromBinary([[maybe_unused]] PGOContext &context, void **buffer,
                                 [[maybe_unused]] PGOProfilerHeader const *header) override
        {
            name_ = base::ReadBuffer(buffer);
            return 1;
        }

        bool ProcessToText(std::ofstream &stream) override
        {
            stream << name_;
            return true;
        }

    private:
        ApEntityId entryId_ {0};
        CString name_;
    };
    using PoolType = PoolTemplate<Entry, CString>;

    PGOStringPool(const std::string &poolName, uint32_t reservedCount)
    {
        pool_ = std::make_shared<PoolType>(poolName, reservedCount);
    };
    ~PGOStringPool()
    {
        Clear();
    }

    bool TryAdd(const CString &value, ApEntityId &entryId)
    {
        return pool_->TryAdd(value, entryId);
    }

    bool TryAddSafe(const CString& value, ApEntityId& entryId)
    {
        WriteLockHolder lock(poolLock_);
        return TryAdd(value, entryId);
    }

    bool GetEntryId(const CString &value, ApEntityId &entryId) const
    {
        return pool_->GetEntryId(value, entryId);
    }

    bool GetEntryIdSafe(const CString& value, ApEntityId& entryId)
    {
        ReadLockHolder lock(poolLock_);
        return GetEntryId(value, entryId);
    }

    bool GetPandaFileDescSafe(ApEntityId abcId, CString& desc)
    {
        ReadLockHolder lock(poolLock_);
        const auto* entry = GetEntry(abcId);
        if (entry == nullptr) {
            return false;
        }
        desc = entry->GetData();
        return true;
    }

    const Entry *GetEntry(ApEntityId entryId) const
    {
        return pool_->GetEntry(entryId);
    }

    bool GetEntryIdByNormalizedName(const CString &value, ApEntityId &entryId) const
    {
        return pool_->GetEntryIdByNormalizedName(value, entryId);
    }

    void Clear()
    {
        pool_->Clear();
    }

    void ClearSafe()
    {
        WriteLockHolder lock(poolLock_);
        Clear();
    }

    std::shared_ptr<PoolType> &GetPool()
    {
        return pool_;
    }

protected:
    NO_COPY_SEMANTIC(PGOStringPool);
    NO_MOVE_SEMANTIC(PGOStringPool);
    std::shared_ptr<PoolType> pool_;
    RWLock poolLock_;
};

class PGORecordPool : public PGOFileSectionInterface {
public:
    PGORecordPool() = default;
    bool Support(PGOProfilerHeader const *header) const override
    {
        return header->SupportRecordPool();
    }

    SectionInfo *GetSection(PGOProfilerHeader const *header) const override
    {
        return header->GetRecordPoolSection();
    }

    uint32_t ProcessToBinary(PGOContext &context, std::fstream &stream) override
    {
        LOG_ECMA(DEBUG) << "[PGORecordPool] ProcessToBinary, count: " << pool_.size();
        SectionInfo secInfo;
        secInfo.number_ = pool_.size();
        secInfo.offset_ = sizeof(SectionInfo);
        auto secInfoPos = stream.tellp();
        stream.seekp(secInfo.offset_, std::ofstream::cur);
        for (auto &entry : pool_) {
            ProfileTypeRef profileTypeRef(context, entry.first);
            stream.write(reinterpret_cast<const char *>(&(profileTypeRef)), sizeof(ApEntityId));
            stream.write(entry.second.c_str(), entry.second.length() + 1);
        }
        secInfo.size_ = static_cast<uint32_t>(stream.tellp()) - static_cast<uint32_t>(secInfoPos);
        auto tail = stream.tellp();
        stream.seekp(secInfoPos, std::ofstream::beg);
        stream.write(reinterpret_cast<const char *>(&(secInfo)), sizeof(SectionInfo));
        stream.seekp(tail, std::ofstream::beg);
        return 1;
    }

    uint32_t ParseFromBinary(PGOContext &context, void **buffer,
                             [[maybe_unused]] PGOProfilerHeader const *header) override
    {
        auto secInfo = base::ReadBuffer<SectionInfo>(buffer);
        for (uint32_t i = 0; i < secInfo.number_; i++) {
            ProfileTypeRef profileTypeRef(base::ReadBuffer<ApEntityId>(buffer, sizeof(ApEntityId)));
            ProfileType profileType(context, profileTypeRef);
            pool_.try_emplace(profileType, base::ReadBuffer(buffer));
        }
        return 1;
    }

    bool ProcessToText(std::ofstream &stream) override
    {
        bool isFirst = true;
        for (auto &entry : pool_) {
            if (isFirst) {
                stream << DumpUtils::NEW_LINE;
                stream << "RecordPool";
                stream << DumpUtils::BLOCK_START;
                isFirst = false;
            }
            stream << DumpUtils::NEW_LINE;
            stream << entry.first.GetTypeString();
            stream << DumpUtils::SPACE;
            stream << DumpUtils::ARRAY_START;
            stream << entry.second;
            stream << DumpUtils::ARRAY_END;
        }
        if (!isFirst) {
            stream << (DumpUtils::SPACE + DumpUtils::NEW_LINE);
        }
        return true;
    }

    void Merge(const PGORecordPool &recordPool)
    {
        for (const auto &entry : recordPool.pool_) {
            Add(ProfileType(entry.first), entry.second);
        }
    }

    void Add(ProfileType profileType, const CString &recordName)
    {
        ASSERT(profileType.GetKind() == ProfileType::Kind::RecordClassId);
        auto result = pool_.try_emplace(profileType, recordName);
        if (result.second) {
            LOG_ECMA(DEBUG) << "Add Record. profile: " << profileType.GetTypeString() << ", recordName: " << recordName;
        }
    }

    const char *GetName(ProfileType profileType) const
    {
        auto iter = pool_.find(profileType);
        if (iter == pool_.end()) {
            return "";
        }
        return iter->second.c_str();
    }

    void Clear()
    {
        pool_.clear();
    }

private:
    std::unordered_map<ProfileType, CString, HashProfileType> pool_;
};

class PGOAbcFilePool : public PGOStringPool {
public:
    enum class ReservedType : uint8_t { EMPTY_ABC_FILE_ID = 0, END };
    static constexpr uint32_t RESERVED_COUNT = 64;
    static_assert(static_cast<uint32_t>(ReservedType::END) < RESERVED_COUNT);

    static bool IsReserved(const CString &value)
    {
        return value.empty();
    }

    static ApEntityId GetReservedId([[maybe_unused]] const CString &value)
    {
        ASSERT(value.empty());
        return ApEntityId(static_cast<uint32_t>(ReservedType::EMPTY_ABC_FILE_ID));
    }

    static bool Support(PGOProfilerHeader const *header)
    {
        return header->SupportProfileTypeWithAbcId();
    }

    static SectionInfo *GetSection(PGOProfilerHeader const *header)
    {
        return header->GetAbcFilePoolSection();
    }

    PGOAbcFilePool() : PGOStringPool("AbcFilePool", RESERVED_COUNT)
    {
        pool_->SetIsReservedCb(IsReserved);
        pool_->SetGetReservedIdCb(GetReservedId);
        pool_->SetGetSectionCb(GetSection);
        pool_->SetSupportCb(Support);
    }

    void Merge(const PGOContext &context, PGOAbcFilePool &pool)
    {
        if (pool.GetPool() == nullptr) {
            return;
        }
        pool_->Merge(*pool.GetPool(), [&](ApEntityId oldEntryId, ApEntityId newEntryId) {
            context.AddAbcIdRemap(oldEntryId, newEntryId);
        });
    }

    void Copy(const std::shared_ptr<PGOAbcFilePool> &pool)
    {
        for (auto &entry : pool->GetPool()->GetPool()) {
            auto emplacedEntry = GetPool()->GetPool().try_emplace(entry.first, entry.second.GetData());
            emplacedEntry.first->second.SetEntryId(entry.first);
            GetPool()->GetValueToId().try_emplace(entry.second.GetData(), entry.first);
        }
    }

    void CopySafe(const std::shared_ptr<PGOAbcFilePool>& pool)
    {
        WriteLockHolder lock(poolLock_);
        Copy(pool);
    }
};
}  // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_RECORD_POOL_H
