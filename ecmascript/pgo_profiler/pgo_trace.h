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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_TRACE_H
#define ECMASCRIPT_PGO_PROFILER_PGO_TRACE_H

#include <iomanip>
#include <memory>
#include <numeric>
#include <unordered_map>

#include "ecmascript/method.h"

namespace panda::ecmascript::pgo {
class PGOTrace {
public:
    static constexpr int NAME_WIDTH = 40;
    static constexpr int WIDTH = 20;
    static constexpr int COLUMN_WIDTH = NAME_WIDTH + WIDTH * 6;
    static constexpr const char* TITLE_TEXT = " PGO Trace ";
    static constexpr const float MIN_PRINT_TIME = 10.0;
    static constexpr const int MIN_PRINT_COUNT = 10;
    static constexpr const int PRECISION = 3;
    static constexpr const int TOTAL_COLUMN = 6;
    static constexpr const int PROFILE_BYTECODE_TIME_COLUMN = 2;

    static std::string Title()
    {
        int halfWidth = (COLUMN_WIDTH - std::strlen(TITLE_TEXT)) / 2 + 1;
        return std::string(halfWidth, '=') + TITLE_TEXT + std::string(halfWidth, '=');
    }

    static std::string Separator()
    {
        return std::string(Title().length(), '=');
    }

    static std::shared_ptr<PGOTrace> GetInstance()
    {
        static auto trace = std::make_shared<PGOTrace>();
        return trace;
    }

    void SetEnable(bool enable)
    {
        LOG_PGO(DEBUG) << "pgo trace is " << (enable ? "enabled" : "disabled");
        enable_ = enable;
    }

    bool IsEnable() const
    {
        return enable_;
    }

    class MethodData {
    public:
        MethodData(const JSThread *thread, JSTaggedValue value, bool hotness)
        {
            Method* method = Method::Cast(value);
            name_ = method->GetMethodName(thread);
            id_ = method->GetMethodId();
            codesize_ = method->GetCodeSize(thread);
            hotness_ = hotness;
        }

        EntityId GetId() const
        {
            return id_;
        }

        void SetProfileBytecodeTime(float time)
        {
            profileBytecodeTime_.push_back(time);
        }

        void Print() const
        {
            float totalTime = std::accumulate(profileBytecodeTime_.begin(), profileBytecodeTime_.end(), 0.0);
            float count = profileBytecodeTime_.size();
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
            if (totalTime < MIN_PRINT_TIME && count < MIN_PRINT_COUNT) {
                return;
            }
#endif
            if (count <= 0) {
                return;
            }
            float avgProfileBytecodeTime = totalTime / count;
            LOG_TRACE(INFO) << std::left << std::setw(NAME_WIDTH) << name_ << std::right << std::setw(WIDTH) << id_
                            << std::setw(WIDTH) << codesize_ << std::setw(WIDTH) << hotness_ << std::setw(WIDTH)
                            << count << std::fixed << std::setprecision(PRECISION) << std::setw(WIDTH)
                            << avgProfileBytecodeTime << std::setw(WIDTH) << totalTime;
        }

        void SetHotness(bool hotness)
        {
            hotness_ = hotness;
        }

    private:
        const char* name_;
        EntityId id_;
        uint32_t codesize_;
        std::list<float> profileBytecodeTime_;
        bool hotness_;
    };

    MethodData* GetMethodData(EntityId id)
    {
        auto iter = methods.find(id);
        if (iter == methods.end()) {
            return nullptr;
        }
        return &(iter->second);
    }

    MethodData* TryGetMethodData(const JSThread *thread, JSTaggedValue value, bool hotness = false)
    {
        MethodData method(thread, value, hotness);
        auto data = GetMethodData(method.GetId());
        if (data) {
            data->SetHotness(hotness);
            return data;
        } else {
            auto res = methods.emplace(method.GetId(), method);
            return &(res.first->second);
        }
    }

    void Print() const
    {
        LOG_TRACE(INFO) << "only print methods which (total time > 10 ms || count > 10) on ohos device";
        LOG_TRACE(INFO) << Title();
        LOG_TRACE(INFO) << std::setw(NAME_WIDTH + WIDTH * (TOTAL_COLUMN - PROFILE_BYTECODE_TIME_COLUMN)) << ""
                        << std::right << std::setw(WIDTH * PROFILE_BYTECODE_TIME_COLUMN) << "Profile Bytecode Time(ms)";
        LOG_TRACE(INFO) << std::left << std::setw(NAME_WIDTH) << "Name" << std::right << std::setw(WIDTH) << "Id"
                        << std::setw(WIDTH) << "CodeSize" << std::setw(WIDTH) << "Hotness" << std::setw(WIDTH)
                        << "Count" << std::setw(WIDTH) << "Avg(ms)" << std::setw(WIDTH) << "Total(ms)";
        for (auto& [id, method]: methods) {
            method.Print();
        }
        LOG_TRACE(INFO) << Separator();
        LOG_TRACE(INFO) << std::left << std::setw(NAME_WIDTH) << "MergeWithExistAP(ms) " << mergeWithExistProfileTime_;
        LOG_TRACE(INFO) << std::left << std::setw(NAME_WIDTH) << "SaveAndRename(ms) " << saveTime_;
        LOG_TRACE(INFO) << Separator();
    }

    void SetSaveTime(float time)
    {
        saveTime_ = time;
    }

    void SetMergeWithExistProfileTime(float time)
    {
        mergeWithExistProfileTime_ = time;
    }

private:
    std::unordered_map<EntityId, MethodData> methods;
    float mergeWithExistProfileTime_;
    float saveTime_;
    bool enable_ {false};
};
} // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_PROFILER_PGO_TRACE_H