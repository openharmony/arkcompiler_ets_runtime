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

#include "ecmascript/dfx/vmstat/function_call_timer.h"

#include <iomanip>

namespace panda::ecmascript {
void FunctionCallTimer::StartCount(size_t id, bool isAot)
{
    PandaRuntimeTimer *callerTimer = currentTimer_;
    auto calleeTimer = &callTimer_[id];
    if (callerTimer != nullptr) {
        calleeTimer->SetParent(callerTimer);
    }
    currentTimer_ = calleeTimer;
    FunctionCallStat *calleeStat = nullptr;
    if (isAot) {
        calleeStat = &aotCallStat_[id];
    } else {
        calleeStat = &intCallStat_[id];
    }
    calleeTimer->Start(calleeStat, callerTimer);
}

void FunctionCallTimer::StopCount(const JSThread *thread, Method *method)
{
    size_t id = method->GetMethodId().GetOffset();
    auto callee = &callTimer_[id];
    if (callee != currentTimer_) {
        LOG_ECMA(INFO) << "EndCallTimer and StartCallTimer have different functions. Current function: "
                       << GetFullName(thread, method) << "has been skipped";
        return;
    }

    PandaRuntimeTimer *callerTimer = callee->Stop();
    currentTimer_ = callerTimer;
}

CString FunctionCallTimer::GetFullName(const JSThread *thread, Method *method)
{
    CString funcName(method->GetMethodName(thread));
    CString recordName = method->GetRecordNameStr(thread);
    CString fullName = funcName + "@" + recordName;
    return fullName;
}

void FunctionCallTimer::InitialStatAndTimer(const JSThread *thread, Method *method, size_t methodId, bool isAot)
{
    if (isAot) {
        auto iter = aotCallStat_.find(methodId);
        if (iter == aotCallStat_.end()) {
            CString funcName = GetFullName(thread, method);
            FunctionCallStat stat(funcName, isAot);
            aotCallStat_[methodId] = stat;
        }
    } else {
        auto iter = intCallStat_.find(methodId);
        if (iter == intCallStat_.end()) {
            CString funcName = GetFullName(thread, method);
            FunctionCallStat stat(funcName, isAot);
            intCallStat_[methodId] = stat;
        }
    }

    PandaRuntimeTimer timer;
    callTimer_[methodId] = timer;
}

void FunctionCallTimer::PrintAllStats()
{
    LOG_ECMA(INFO) << "function call stat:";
    static constexpr int nameRightAdjustment = 45;
    static constexpr int numberRightAdjustment = 15;
    LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << "JS && TS Function Name"
        << std::setw(numberRightAdjustment) << "Type"
        << std::setw(numberRightAdjustment) << "Time(ns)" << std::setw(numberRightAdjustment) << "Count"
        << std::setw(numberRightAdjustment) << "MaxTime(ns)"
        << std::setw(numberRightAdjustment) << "AvgTime(ns)";
    LOG_ECMA(INFO) << "============================================================="
                   << "=============================================================";

    CVector<FunctionCallStat> callStatVec;
    for (auto &stat : aotCallStat_) {
        callStatVec.emplace_back(stat.second);
    }
    for (auto &stat : intCallStat_) {
        callStatVec.emplace_back(stat.second);
    }
    // Sort by TotalTime
    std::sort(callStatVec.begin(), callStatVec.end(),
        [](const FunctionCallStat &a, const FunctionCallStat &b) -> bool {
            return a.TotalTime() > b.TotalTime();
    });

    for (auto &stat : callStatVec) {
        if (stat.TotalCount() != 0) {
            CString type = stat.IsAot() ? "Aot" : "Interpreter";
            LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << stat.Name()
                << std::setw(numberRightAdjustment) << type
                << std::setw(numberRightAdjustment) << stat.TotalTime()
                << std::setw(numberRightAdjustment) << stat.TotalCount()
                << std::setw(numberRightAdjustment) << stat.MaxTime()
                << std::setw(numberRightAdjustment) << stat.TotalTime() / stat.TotalCount();
        }
    }
}

void FunctionCallTimer::ResetStat()
{
    for (auto &stat : aotCallStat_) {
        stat.second.Reset();
    }

    for (auto &stat : intCallStat_) {
        stat.second.Reset();
    }
}
}