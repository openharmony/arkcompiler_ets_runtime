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

#ifndef ECMASCRIPT_PGO_PROFILER_TESTS_PGO_CONTEXT_MOCK_H
#define ECMASCRIPT_PGO_PROFILER_TESTS_PGO_CONTEXT_MOCK_H

#include <memory>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/base/file_header.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"

namespace panda::ecmascript::pgo {
class PGOContextMock : public PGOContext {
public:
    explicit PGOContextMock(base::FileHeaderBase::VersionType version)
    {
        profileTypePool_ = std::make_shared<PGOProfileTypePool>();
        PGOProfilerHeader::Build(&header_, sizeof(PGOProfilerHeader));
        header_->SetVersion(version);
    }
    ~PGOContextMock() override
    {
        delete header_;
        header_ = nullptr;
    };

    std::shared_ptr<PGOProfileTypePool> GetProfileTypePool() const override
    {
        return profileTypePool_;
    }
    uint32_t GetHotnessThreshold() const override
    {
        return threshold_;
    }
    PGOProfilerHeader *GetHeader() const override
    {
        return header_;
    }
    bool SupportElementsKind() const override
    {
        ASSERT(header_ != nullptr);
        return header_->SupportElementsKind();
    }

    bool SupportElementsTrackInfo() const override
    {
        ASSERT(header_ != nullptr);
        return header_->SupportElementsTrackInfo();
    }

    void ResetAbcIdRemap() const override
    {
        abcIdRemap_.clear();
    }

    void AddAbcIdRemap(ApEntityId oldId, ApEntityId newId) const override
    {
        abcIdRemap_[oldId] = newId;
    }

    const std::map<ApEntityId, ApEntityId> &GetAbcIdRemap() const override
    {
        return abcIdRemap_;
    }

private:
    NO_COPY_SEMANTIC(PGOContextMock);
    NO_MOVE_SEMANTIC(PGOContextMock);
    std::shared_ptr<PGOProfileTypePool> profileTypePool_;
    PGOProfilerHeader *header_ {};
    uint32_t threshold_ {};
    mutable std::map<ApEntityId, ApEntityId> abcIdRemap_;
};
} // namespace panda::ecmascript::pgo
#endif