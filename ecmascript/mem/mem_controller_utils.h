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

#ifndef ECMASCRIPT_MEM_MEM_CONTROLLER_UTILS_H
#define ECMASCRIPT_MEM_MEM_CONTROLLER_UTILS_H

#include <chrono>
#include <cmath>
#include <limits>

#include "ecmascript/base/gc_ring_buffer.h"

using BytesAndDuration = std::pair<uint64_t, double>;
inline BytesAndDuration MakeBytesAndDuration(uint64_t bytes, double duration)
{
    return std::make_pair(bytes, duration);
}

namespace panda::ecmascript {
class MemControllerUtils {
public:
    static double GetSystemTimeInMs()
    {
        double currentTime =
            std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        return currentTime * MILLISECOND_PER_SECOND;
    }
    static constexpr int LENGTH = 10;
    static double CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer);
    static double CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer,
                                        const BytesAndDuration &initial, const double timeMs);
private:
    static constexpr int MILLISECOND_PER_SECOND = 1000;
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_MEM_MEM_CONTROLLER_UTILS_H