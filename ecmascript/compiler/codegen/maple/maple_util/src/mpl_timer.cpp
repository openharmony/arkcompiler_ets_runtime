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

#include "mpl_timer.h"
#include <chrono>

namespace maple {
MPLTimer::MPLTimer() = default;

MPLTimer::~MPLTimer() = default;

void MPLTimer::Start()
{
    startTime = std::chrono::system_clock::now();
}

void MPLTimer::Stop()
{
    endTime = std::chrono::system_clock::now();
}

long MPLTimer::Elapsed() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
}

long MPLTimer::ElapsedMilliseconds() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
}

long MPLTimer::ElapsedMicroseconds() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
}
}  // namespace maple
