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

#include "ecmascript/platform/time.h"

#include <ctime>

namespace panda::ecmascript {
static constexpr uint16_t THOUSAND = 1000;
static constexpr int SEC_PER_MINUTE = 60;

int64_t GetLocalOffsetFromOS(int64_t timeMs, bool isLocal)
{
    if (!isLocal) {
        return 0;
    }
    timeMs /= THOUSAND;
    time_t tv = timeMs;
    struct tm tm {
    };
    // localtime_r is only suitable for linux.
    struct tm *t = localtime_r(&tv, &tm);
    // tm_gmtoff includes any daylight savings offset.
    if (t == nullptr) {
        return 0;
    }

    return t->tm_gmtoff / SEC_PER_MINUTE;
}

bool IsDst(int64_t timeMs)
{
    timeMs /= THOUSAND;
    time_t tv = timeMs;
    struct tm tm {
    };
    struct tm *t = nullptr;
    t = localtime_r(&tv, &tm);
    return (t != nullptr) && t->tm_isdst;
}
}  // namespace panda::ecmascript
