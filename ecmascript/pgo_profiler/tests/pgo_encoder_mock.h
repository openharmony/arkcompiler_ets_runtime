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

#ifndef ECMASCRIPT_PGO_PROFILER_TESTS_PGO_ENCODER_MOCK_H
#define ECMASCRIPT_PGO_PROFILER_TESTS_PGO_ENCODER_MOCK_H

#include <memory>

#include "ecmascript/base/file_header.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"

namespace panda::ecmascript::pgo {
class PGOProfilerEncoderMock : public PGOProfilerEncoder {
public:
    PGOProfilerEncoderMock(const std::string &outDir, uint32_t hotnessThreshold, ApGenMode mode)
        : PGOProfilerEncoder(outDir, hotnessThreshold, mode)
    {
    }

    void SetVersion(base::FileHeaderBase::VersionType version)
    {
        header_->SetVersion(version);
    }
};
}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_TESTS_PGO_ENCODER_MOCK_H