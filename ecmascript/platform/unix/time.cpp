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
#include <cmath>

namespace panda::ecmascript {
static constexpr int MS_PER_SECOND = 1000;

int64_t GetLocalOffsetFromOS(int64_t timeMs, bool isLocal)
{
    if (!isLocal) {
        return 0;
    }
    timeMs /= MS_PER_SECOND;
    time_t tv = timeMs;
    tm localTime {};
    // localtime_r is only suitable for linux.
    tm *t = localtime_r(&tv, &localTime);
    // tm_gmtoff includes any daylight savings offset.
    if (t == nullptr) {
        return 0;
    }

    return t->tm_gmtoff * MS_PER_SECOND;
}

int64_t GetUTCTimestamp(int year, int month, int day, int hour, int minute, int second, int millisecond)
{
    tm localTime = {
        .tm_sec = second + (millisecond / MS_PER_SECOND),
        .tm_min = minute,
        .tm_hour = hour,
        .tm_mday = day,
        .tm_mon = month,
        .tm_year = year,
        .tm_isdst = -1 // -1: let the system decide whether to use DST.
    };
    return (mktime(&localTime) * MS_PER_SECOND) + (millisecond % MS_PER_SECOND);
}

bool IsDst(int64_t timeMs)
{
    timeMs /= MS_PER_SECOND;
    time_t tv = timeMs;
    struct tm tm {
    };
    struct tm *t = nullptr;
    t = localtime_r(&tv, &tm);
    return (t != nullptr) && t->tm_isdst;
}
}  // namespace panda::ecmascript
