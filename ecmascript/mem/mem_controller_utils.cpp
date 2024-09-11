/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/mem_controller_utils.h"

#include "ecmascript/mem/mem_common.h"

namespace panda::ecmascript {

double MemControllerUtils::CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer)
{
    return CalculateAverageSpeed(buffer, MakeBytesAndDuration(0, 0), 0);
}

double MemControllerUtils::CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer,
    const BytesAndDuration &initial, const double timeMs)
{
    BytesAndDuration sum = buffer.Sum(
        [timeMs](BytesAndDuration a, BytesAndDuration b) {
            if (timeMs != 0 && a.second >= timeMs) {
                return a;
            }
            return std::make_pair(a.first + b.first, a.second + b.second);
        },
        initial);
    uint64_t bytes = sum.first;
    double durations = sum.second;
    if (fabs(durations) <= 1e-6) {
        return 0;
    }
    double speed = bytes / durations;
    const int maxSpeed = static_cast<int>(1_GB);
    const int minSpeed = 1;
    if (speed >= maxSpeed) {
        return maxSpeed;
    }
    if (speed <= minSpeed) {
        return minSpeed;
    }
    return speed;
}

}