/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_PROFILER_AP_FILE_POOL_TEMPLATE_H
#define ECMASCRIPT_PGO_PROFILER_AP_FILE_POOL_TEMPLATE_H

#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <utility>

#include "ecmascript/common.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"

namespace panda::ecmascript::pgo {
template <typename Entry, typename V>
class PoolTemplate : public PGOFileSectionInterface {
public:
    using IsReservedCb = std::function<bool(const V &value)>;
    using GetReservedIdCb = std::function<ApEntityId(const V &value)>;

    using SupportCb = std::function<bool(PGOProfilerHeader const *header)>;
    using GetSectionCb = std::function<SectionInfo *(PGOProfilerHeader const *header)>;

    PoolTemplate(std::string poolName, uint32_t reservedCount)
        : poolName_(std::move(poolName)), RESERVED_COUNT(reservedCount) {};

    ~PoolTemplate() override
    {
        Clear();
    }

    bool TryAdd(const V &value, ApEntityId &entryId)
    {
        auto it = valueToId_.find(value);
        if (it != valueToId_.end()) {
            entryId = it->second;
            return true;
        }

        entryId = ApEntityId(IsReserved(value) ? (++reservedUsed_, GetReservedId(value))
                                               : RESERVED_COUNT + pool_.size() - reservedUsed_);

        auto result = pool_.emplace(entryId, value);
        valueToId_.emplace(value, entryId);
        auto &entry = result.first->second;
        entry.SetEntryId(entryId);
        return true;
    }

    bool GetEntryId(const V &value, ApEntityId &entryId) const
    {
        for (const auto &entry : pool_) {
            if (entry.second.GetData() == value) {
                entryId = entry.first;
                return true;
            }
        }
        return false;
    }

    const Entry *GetEntry(ApEntityId id) const
    {
        auto iter = pool_.find(id);
        if (iter == pool_.end()) {
            return nullptr;
        }
        return &(iter->second);
    }

    bool GetEntryIdByNormalizedName(const V &value, ApEntityId &entryId) const
    {
        for (const auto &entry : pool_) {
            if (JSPandaFile::GetNormalizedFileDesc(entry.second.GetData()) == value) {
                entryId = entry.first;
                return true;
            }
        }
        return false;
    }

    void Clear()
    {
        pool_.clear();
        valueToId_.clear();
        reservedUsed_ = 0;
    }

    bool Empty() const
    {
        return pool_.empty();
    }

    void Merge(const PoolTemplate &pool, const std::function<void(ApEntityId, ApEntityId)> &callback)
    {
        for (const auto &entry : pool.pool_) {
            ApEntityId newId(0);
            TryAdd(entry.second.GetData(), newId);
            if (callback != nullptr) {
                callback(entry.first, newId);
            }
        }
    }

    uint32_t ProcessToBinary([[maybe_unused]] PGOContext &context, std::fstream &stream) override
    {
        LOG_ECMA(DEBUG) << "ProcessToBinary. name: " << poolName_ << ", count: " << pool_.size();
        SectionInfo secInfo;
        secInfo.number_ = pool_.size();
        secInfo.offset_ = sizeof(SectionInfo);
        auto secInfoPos = stream.tellp();
        stream.seekp(secInfo.offset_, std::ofstream::cur);
        for (auto &entry : pool_) {
            stream.write(reinterpret_cast<const char *>(&(entry.first)), sizeof(ApEntityId));
            entry.second.ProcessToBinary(context, stream);
        }
        secInfo.size_ = static_cast<uint32_t>(stream.tellp()) - static_cast<uint32_t>(secInfoPos);
        auto tail = stream.tellp();
        stream.seekp(secInfoPos, std::ofstream::beg);
        stream.write(reinterpret_cast<const char *>(&(secInfo)), sizeof(SectionInfo));
        stream.seekp(tail, std::ofstream::beg);
        return 1;
    }

    bool ProcessToText(std::ofstream &stream) override
    {
        bool isFirst = true;
        for (auto &entry : pool_) {
            if (isFirst) {
                stream << DumpUtils::NEW_LINE;
                stream << poolName_;
                stream << DumpUtils::BLOCK_START;
                isFirst = false;
            }
            stream << DumpUtils::NEW_LINE;
            stream << std::to_string(entry.first);
            stream << DumpUtils::SPACE;
            stream << DumpUtils::ARRAY_START;
            entry.second.ProcessToText(stream);
            stream << DumpUtils::ARRAY_END;
        }
        if (!isFirst) {
            stream << (DumpUtils::SPACE + DumpUtils::NEW_LINE);
        }
        return true;
    }

    void ProcessToJson(std::vector<ProfileType::StringMap> &abcPoolArray)
    {
        for (auto &entry : pool_) {
            ProfileType::StringMap abcPool;
            abcPool.insert(std::make_pair(DumpJsonUtils::ABC_ID, std::to_string(entry.first)));
            abcPool.insert(std::make_pair(DumpJsonUtils::ABC_FILE, entry.second.GetData()));
            abcPoolArray.push_back(abcPool);
        }
    }
    uint32_t ParseFromBinary([[maybe_unused]] PGOContext &context, void **buffer,
                             PGOProfilerHeader const *header) override
    {
        auto secInfo = base::ReadBuffer<SectionInfo>(buffer);
        for (uint32_t i = 0; i < secInfo.number_; i++) {
            auto entryId = base::ReadBuffer<ApEntityId>(buffer, sizeof(ApEntityId));
            auto result = pool_.try_emplace(entryId);
            result.first->second.SetEntryId(entryId);
            result.first->second.ParseFromBinary(context, buffer, header);
        }
        return 1;
    }

    void SetIsReservedCb(const IsReservedCb &isReservedCb)
    {
        isReservedCb_ = isReservedCb;
    }

    void SetGetReservedIdCb(const GetReservedIdCb &getReservedIdCb)
    {
        getReservedIdCb_ = getReservedIdCb;
    }

    void SetSupportCb(const SupportCb &supportCb)
    {
        supportCb_ = supportCb;
    }

    void SetGetSectionCb(const GetSectionCb &getSectionCb)
    {
        getSectionCb_ = getSectionCb;
    }

    std::unordered_map<ApEntityId, Entry> &GetPool()
    {
        return pool_;
    }

    std::unordered_map<V, ApEntityId> &GetValueToId()
    {
        return valueToId_;
    }

private:
    NO_COPY_SEMANTIC(PoolTemplate);
    NO_MOVE_SEMANTIC(PoolTemplate);

    bool Support(PGOProfilerHeader const *header) const override
    {
        return supportCb_(header);
    }

    SectionInfo *GetSection(PGOProfilerHeader const *header) const override
    {
        return getSectionCb_(header);
    }

    bool IsReserved(const V &value)
    {
        return isReservedCb_(value);
    }

    ApEntityId GetReservedId(const V &value)
    {
        return getReservedIdCb_(value);
    }

    const std::string poolName_;
    const uint32_t RESERVED_COUNT {};
    uint32_t reservedUsed_ {0};

    IsReservedCb isReservedCb_;
    GetReservedIdCb getReservedIdCb_;
    SupportCb supportCb_;
    GetSectionCb getSectionCb_;
    std::unordered_map<ApEntityId, Entry> pool_;
    std::unordered_map<V, ApEntityId> valueToId_;
};
}  // namespace panda::ecmascript::pgo

namespace std {
using panda::ecmascript::pgo::ProfileType;
template<>
struct hash<ProfileType> {
    size_t operator()(const ProfileType& type) const noexcept
    {
        return type.GetRaw();
    }
};
} // namespace std
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_POOL_TEMPLATE_H
