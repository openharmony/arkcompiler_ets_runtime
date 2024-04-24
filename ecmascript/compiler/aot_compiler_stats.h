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

#ifndef ECMASCRIPT_AOT_COMPILER_STATS_H
#define ECMASCRIPT_AOT_COMPILER_STATS_H

#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"

#include <ctime>
#include <chrono>
#include <string>

namespace panda::ecmascript {
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<uint64_t, std::nano>;

class AotCompilerStats {
public:
    AotCompilerStats() {};
    ~AotCompilerStats() = default;

    void SetBundleName(std::string bundleName)
    {
        bundleName_ = bundleName;
    }

    std::string GetBundleName() const
    {
        return bundleName_;
    }

    void SetPgoPath(std::string pgoFilePath)
    {
        pgoFilePath_ = pgoFilePath;
    }

    std::string GetPgoPath() const
    {
        return pgoFilePath_;
    }

    void SetAotFilePath(std::string aotFilePath)
    {
        aotFilePath_ = aotFilePath;
    }

    std::string GetAotFilePath() const
    {
        return aotFilePath_;
    }

    void StartCompiler()
    {
        start_ = Clock::now();
    }

    void EndCompiler()
    {
        end_ = Clock::now();
    }

    float GetTotalTime() const
    {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
        return (float) duration.count() / SECOND_TIME;
    }

    void SetIsLiteCg(bool litecg)
    {
        isLiteCg_ = litecg;
    }

    bool IsLiteCg() const
    {
        return isLiteCg_;
    }

    void SetCompilerMethodCount(uint32_t count)
    {
        compilerMethodCount_ += count;
    }

    uint32_t GetCompilerMethodCount() const
    {
        return compilerMethodCount_;
    }

    void SetPgoFileLegal(bool legal)
    {
        pgoFileLegal_ = legal;
    }

    bool GetPgoFileLegal() const
    {
        return pgoFileLegal_;
    }

    void PrintCompilerStatsLog();

private:
    void SendSysEvent() const;
    bool IsLongTimeCompiler() const
    {
        return GetTotalTime() > longTime_;
    }

    std::string bundleName_ = "";
    std::string pgoFilePath_ = "";
    std::string aotFilePath_ = "";
    Clock::time_point start_;
    Clock::time_point end_;
    bool isLiteCg_ = false;
    uint32_t compilerMethodCount_ = 0;
    bool pgoFileLegal_ = true;
    float longTime_ = 120;
    static constexpr uint32_t SECOND_TIME = 1000000;
};
}

#endif // ECMASCRIPT_AOT_COMPILER_STATS_H