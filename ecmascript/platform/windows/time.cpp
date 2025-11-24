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

#include <timezoneapi.h>
#include <ctime>
#include <windows.h>

namespace panda::ecmascript {
static constexpr int MS_PER_SECOND = 1000;
static constexpr int MS_PER_MINUTE = 60000;

int64_t GetLocalOffsetFromOS([[maybe_unused]] int64_t timeMs, bool isLocal)
{
    if (!isLocal) {
        return 0;
    }
    TIME_ZONE_INFORMATION tmp;
    GetTimeZoneInformation(&tmp);
    int64_t res = -tmp.Bias;
    return res * MS_PER_MINUTE;
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
    struct tm nowtm;
    localtime_s(&nowtm, &tv);

    int month = nowtm.tm_mon + 1;
    int day = nowtm.tm_mday;
    int hour = nowtm.tm_hour;

    TIME_ZONE_INFORMATION tzi;
    GetTimeZoneInformation(&tzi);

    SYSTEMTIME stDSTStart = tzi.DaylightDate;
    SYSTEMTIME stDSTEnd = tzi.StandardDate;

    if (month > stDSTStart.wMonth && month < stDSTEnd.wMonth) {
        return true;
    } else if (month == stDSTStart.wMonth) {
        if (day > stDSTStart.wDay || (day == stDSTStart.wDay && hour >= tzi.DaylightBias)) {
            return true;
        }
    } else if (month == stDSTEnd.wMonth) {
        if (day < stDSTEnd.wDay || (day == stDSTEnd.wDay && hour < tzi.DaylightBias)) {
            return true;
        }
    }

    return false;
}
}  // namespace panda::ecmascript
