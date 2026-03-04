/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
static constexpr int MS_PER_SECOND = 1000;
static constexpr int NS_PER_SECOND = 1000000;

int64_t GetCurrentTimestamp()
{
    struct timespec ts {};
    // clock_gettime with CLOCK_REALTIME is async-signal-safe
    clock_gettime(CLOCK_REALTIME, &ts);
    return (static_cast<int64_t>(ts.tv_sec) * MS_PER_SECOND) +
            (ts.tv_nsec / NS_PER_SECOND);
}
} // namespace panda::ecmascript
