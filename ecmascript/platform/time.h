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

#ifndef ECMASCRIPT_PLATFORM_TIME_H
#define ECMASCRIPT_PLATFORM_TIME_H

#include <cstdint>

namespace panda::ecmascript {
// Get the time offset relative to the GMT time zone (in second) through the timestamp
int64_t GetLocalOffsetFromOS(int64_t timeMs, bool isLocal);
// Note that the parameter "year" in this file is the difference from 1900 to the actual year.
// Get the timestamp (in millisecond) through the explicit time
int64_t GetUTCTimestamp(int year, int month, int day, int hour, int minute, int second, int millisecond);

bool IsDst(int64_t timeMs);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_TIME_H
