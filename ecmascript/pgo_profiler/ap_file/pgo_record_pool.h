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
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pool_template.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
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

        uint32_t ProcessToBinary(std::fstream &stream) override
        {
            stream << name_ << '\0';
            return 1;
        }

        uint32_t ParseFromBinary(void **buffer, [[maybe_unused]] PGOProfilerHeader const *header) override
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

    bool GetEntryId(const CString &value, ApEntityId &entryId) const
    {
        return pool_->GetEntryId(value, entryId);
    }

    const Entry *GetEntry(ApEntityId entryId) const
    {
        return pool_->GetEntry(entryId);
    }

    void Clear()
    {
        pool_->Clear();
    }

    std::shared_ptr<PoolType> &GetPool()
    {
        return pool_;
    }

protected:
    NO_COPY_SEMANTIC(PGOStringPool);
    NO_MOVE_SEMANTIC(PGOStringPool);
    std::shared_ptr<PoolType> pool_;
};

class PGORecordPool : public PGOStringPool {
public:
    enum class ReservedType : uint8_t { EMPTY_RECORD_ID = 0, END };
    static constexpr uint32_t RESERVED_COUNT = 64;
    static_assert(static_cast<uint32_t>(ReservedType::END) < RESERVED_COUNT);

    static bool IsReserved(const CString &value)
    {
        return value.empty();
    }

    static ApEntityId GetReservedId([[maybe_unused]] const CString &value)
    {
        ASSERT(value.empty());
        return ApEntityId(static_cast<uint32_t>(ReservedType::EMPTY_RECORD_ID));
    }

    static bool Support(PGOProfilerHeader const *header)
    {
        return header->SupportRecordPool();
    }

    static SectionInfo *GetSection(PGOProfilerHeader const *header)
    {
        return header->GetRecordPoolSection();
    }

    PGORecordPool() : PGOStringPool("RecordPool", RESERVED_COUNT)
    {
        pool_->SetIsReservedCb(IsReserved);
        pool_->SetGetReservedIdCb(GetReservedId);
        pool_->SetGetSectionCb(GetSection);
        pool_->SetSupportCb(Support);
    }

    void Merge(const PGORecordPool &recordPool, std::map<ApEntityId, ApEntityId> &idMapping)
    {
        pool_->Merge(*recordPool.pool_,
                     [&](ApEntityId oldId, ApEntityId newId) { idMapping.try_emplace(oldId, newId); });
    }
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
};

}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_PGO_RECORD_POOL_H
