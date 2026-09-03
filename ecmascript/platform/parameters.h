/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PLATFORM_PARAMETERS_H
#define ECMASCRIPT_PLATFORM_PARAMETERS_H

#include "ecmascript/mem/mem_common.h"

#include <cstdint>

namespace panda::ecmascript {
    uint64_t GetImportDuration(uint64_t defaultTime);

    size_t GetPoolSize(size_t defaultSize);

    bool IsEnableCMCGC(bool defaultValue);

    // Read after fork: when true this process stops backing hole-initialised
    // objects with the shared template. The template itself is always created
    // before the fork, so the decision can be made per application process.
    bool IsDisableHoleMemory(bool defaultValue);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_PARAMETERS_H
