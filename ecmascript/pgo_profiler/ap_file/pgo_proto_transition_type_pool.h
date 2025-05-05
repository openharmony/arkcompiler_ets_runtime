/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_PROTO_TRANSITION_TYPE_POOL_H
#define ECMASCRIPT_PGO_PROTO_TRANSITION_TYPE_POOL_H

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
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class PGOProtoTransitionPool : public PGOFileSectionInterface {
public:
    PGOProtoTransitionPool() = default;
    bool Support(PGOProfilerHeader const *header) const override
    {
        return header->SupportProtoTransitionPool();
    }

    SectionInfo *GetSection(PGOProfilerHeader const *header) const override
    {
        return header->GetProtoTransitionPoolSection();
    }

    uint32_t ProcessToBinary(PGOContext &context, std::fstream &stream) override
    {
        LOG_ECMA(DEBUG) << "[PGOProtoTransitionPool] ProcessToBinary, count: " << pool_.size();
        SectionInfo secInfo;
        secInfo.number_ = pool_.size();
        secInfo.offset_ = sizeof(SectionInfo);
        auto secInfoPos = stream.tellp();
        stream.seekp(secInfo.offset_, std::ofstream::cur);
        for (auto &entry : pool_) {
            PGOProtoTransitionTypeRef profileTypeRef;
            profileTypeRef.ConvertFrom(context, entry.second);
            stream.write(reinterpret_cast<const char *>(&(profileTypeRef)), sizeof(PGOProtoTransitionTypeRef));
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
            PGOProtoTransitionTypeRef typeRef(
                base::ReadBuffer<PGOProtoTransitionTypeRef>(buffer, sizeof(PGOProtoTransitionTypeRef)));
            PGOProtoTransitionType type;
            type.ConvertFrom(context, typeRef);
            ProfileType key = type.GetIhcType();
            pool_.try_emplace(key, type);
        }
        return 1;
    }

    bool ProcessToText(std::ofstream &stream) override
    {
        bool isFirst = true;
        for (auto &entry : pool_) {
            if (isFirst) {
                stream << DumpUtils::NEW_LINE;
                stream << "ProtoTransitionPool";
                stream << DumpUtils::BLOCK_START;
                isFirst = false;
            }
            stream << DumpUtils::NEW_LINE;
            stream << entry.second.GetTypeString();
            stream << DumpUtils::SPACE;
        }
        if (!isFirst) {
            stream << (DumpUtils::SPACE + DumpUtils::NEW_LINE);
        }
        return true;
    }

    void Merge(const PGOProtoTransitionPool &protoTraPool)
    {
        for (const auto &entry : protoTraPool.pool_) {
            Add(entry.second);
        }
    }

    void Add(PGOProtoTransitionType type)
    {
        if (type.IsNone()) {
            return;
        }
        ProfileType key = type.GetIhcType();
        auto iter = pool_.find(key);
        if (iter == pool_.end()) {
            auto result = pool_.try_emplace(key, type);
            if (result.second) {
                LOG_ECMA(DEBUG) << "Add proto transition type: " << type.GetTypeString();
            }
            return;
        }
        if (iter->second.IsNone()) {
            return;
        }
        auto combineType = iter->second.CombineType(type);
        pool_[key] = combineType;
    }

    template <typename Callback>
    bool Iterate(Callback callback) const
    {
        for (auto entry : pool_) {
            if (entry.second.IsNone()) {
                continue;
            }
            callback(entry.second);
        }
        return true;
    }

    void Clear()
    {
        pool_.clear();
    }

private:
    std::unordered_map<ProfileType, PGOProtoTransitionType, HashProfileType> pool_;
};
}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROTO_TRANSITION_TYPE_POOL_H
