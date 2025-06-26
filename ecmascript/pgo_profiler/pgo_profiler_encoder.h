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

#ifndef ECMASCRIPT_PGO_PROFILER_ENCODER_H
#define ECMASCRIPT_PGO_PROFILER_ENCODER_H

#include <atomic>
#include <memory>
#include <utility>

#include "libpandabase/macros.h"

#include "ecmascript/pgo_profiler/pgo_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"

namespace panda::ecmascript::pgo {
class PGOProfilerDecoder;
class PGOProfilerEncoder {
public:
    static constexpr size_t MIN_DISK_SPACE = 300_MB;
    static constexpr size_t MAX_AP_FILE_SIZE = 100_MB;
    static constexpr size_t CONVERT_FACTOR = 1_MB;

    enum ApGenMode { OVERWRITE, MERGE };

    PGOProfilerEncoder(const std::string& path, ApGenMode mode): path_(path), mode_(mode) {}
    ~PGOProfilerEncoder() = default;

    NO_COPY_SEMANTIC(PGOProfilerEncoder);
    NO_MOVE_SEMANTIC(PGOProfilerEncoder);

    void SetApGenMode(ApGenMode mode);
    ApGenMode GetApGenMode() const;
    bool PUBLIC_API Save(const std::shared_ptr<PGOInfo> pgoInfo);

private:
    bool InternalSave(const std::shared_ptr<PGOInfo> info);
    bool SaveAndRename(const std::shared_ptr<PGOInfo> info);
    std::string GetDirectoryPath(const std::string& path) const;
    bool WriteProfilerFile(const std::shared_ptr<PGOInfo> info, const std::string& tmpOutPath);
    bool ValidateAndRename(const std::string& tmpOutPath);
    void AddChecksum(std::fstream& fileStream);

    std::string path_;
    ApGenMode mode_ {OVERWRITE};
    Mutex mutex_;
};
} // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_PROFILER_ENCODER_H
