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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_CONTEXT_H
#define ECMASCRIPT_PGO_PROFILER_PGO_CONTEXT_H

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "macros.h"

namespace panda::ecmascript::pgo {
class PGORecordPool;
class PGOProfileTypePool;
class PGOProfilerHeader;

class PGOContext {
public:
    PGOContext() = default;
    virtual ~PGOContext() = default;
    virtual std::shared_ptr<PGORecordPool> GetRecordPool() const = 0;
    virtual std::shared_ptr<PGOProfileTypePool> GetProfileTypePool() const = 0;
    virtual uint32_t GetHotnessThreshold() const = 0;
    virtual PGOProfilerHeader *GetHeader() const = 0;
    virtual bool SupportElementsKind() const = 0;

private:
    NO_COPY_SEMANTIC(PGOContext);
    NO_MOVE_SEMANTIC(PGOContext);
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_PGO_CONTEXT_H
