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

#ifndef MAPLE_UTIL_INCLUDE_MPL_TIMER_H
#define MAPLE_UTIL_INCLUDE_MPL_TIMER_H

#include <chrono>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

namespace maple {
    class MPLTimer {
    public:
        MPLTimer() = default;
        ~MPLTimer()= default;

        void Start()
        {
            startTime = std::chrono::system_clock::now();
        }

        void Stop()
        {
            endTime = std::chrono::system_clock::now();
        }

        long Elapsed() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
        }

        long ElapsedMilliseconds() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        }

        long ElapsedMicroseconds() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        }

    private:
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
    };

class MPLTimerManager {
public:
    enum class Unit {
        kMicroSeconds, // us
        kMilliSeconds, // ms
        kSeconds,      // s
    };

    // time data
    struct Timer {
        void Start() noexcept
        {
            startTime = std::chrono::system_clock::now();
            ++count;
        }

        void Stop() noexcept
        {
            useTime += (std::chrono::system_clock::now() - startTime);
        }

        template <class T = std::chrono::milliseconds>
        long Elapsed() const noexcept
        {
            return std::chrono::duration_cast<T>(useTime).count();
        }

        std::chrono::system_clock::time_point startTime;
        std::chrono::nanoseconds useTime;
        uint32_t count = 0; // run count
    };

    MPLTimerManager() = default;
    virtual ~MPLTimerManager() = default;

    void Clear()
    {
        allTimer.clear();
    }

    Timer &GetTimerFormKey(const std::string &key)
    {
        return allTimer[key];
    }

    template <Unit unit = Unit::kMilliSeconds>
    std::string ConvertAllTimer2Str() const
    {
        std::ostringstream os;
        for (auto &[key, timer] : allTimer) {
            os << "\t" << key << ": ";
            if constexpr (unit == Unit::kMicroSeconds) {
                os << timer.Elapsed<std::chrono::microseconds>() << "us";
            } else if constexpr (unit == Unit::kMilliSeconds) {
                os << timer.Elapsed<std::chrono::milliseconds>() << "ms";
            } else {
                static_assert(unit == Unit::kSeconds, "unknown units");
                os << timer.Elapsed<std::chrono::seconds>() << "s";
            }
            os << ", count: " << timer.count << std::endl;
        }
        return os.str();
    }

private:
    std::map<std::string, Timer> allTimer;
};

class MPLTimerRegister {
public:
    MPLTimerRegister(MPLTimerManager &timerM, const std::string &key)
    {
        timer = &timerM.GetTimerFormKey(key);
        timer->Start();
    }

    ~MPLTimerRegister()
    {
        Stop();
    }

    void Stop() noexcept
    {
        if (timer != nullptr) {
            timer->Stop();
            timer = nullptr;
        }
    }

private:
    MPLTimerManager::Timer *timer = nullptr;
};
} // namespace maple
#endif // MAPLE_UTIL_INCLUDE_MPL_TIMER_H
