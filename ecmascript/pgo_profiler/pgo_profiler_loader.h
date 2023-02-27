/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_PROFILE_LOADER_H
#define ECMASCRIPT_PGO_PROFILE_LOADER_H

#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
class PGOProfilerLoader {
public:
    PGOProfilerLoader() = default;
    PGOProfilerLoader(const std::string &inPath, uint32_t hotnessThreshold)
        : inPath_(inPath), hotnessThreshold_(hotnessThreshold) {}

    virtual ~PGOProfilerLoader() = default;

    NO_COPY_SEMANTIC(PGOProfilerLoader);
    NO_MOVE_SEMANTIC(PGOProfilerLoader);

    bool PUBLIC_API Match(const CString &recordName, EntityId methodId);

    bool PUBLIC_API LoadAndVerify(uint32_t checksum);
    bool PUBLIC_API LoadFull();
    void PUBLIC_API Clear();

    bool PUBLIC_API SaveAPTextFile(const std::string &outPath);

    template <typename Callback>
    void Update(Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->Update(callback);
    }

    template <typename Callback>
    void Update(const CString &recordName, Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->Update(recordName, callback);
    }

    bool IsLoaded() const
    {
        return isLoaded_;
    }

private:
    bool Load();
    bool Verify(uint32_t checksum);

    bool LoadAPBinaryFile();
    void UnLoadAPBinaryFile();

    bool isLoaded_ {false};
    bool isVerifySuccess_ {false};
    std::string inPath_;
    uint32_t hotnessThreshold_ {0};
    PGOProfilerHeader *header_ {nullptr};
    PGOPandaFileInfos pandaFileInfos_;
    std::unique_ptr<PGORecordDetailInfos> recordDetailInfos_;
    std::unique_ptr<PGORecordSimpleInfos> recordSimpleInfos_;
    MemMap fileMapAddr_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILE_LOADER_H
