/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include <iomanip>

#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
using EcmaOpcode = kungfu::EcmaOpcode;

void OptCodeProfiler::PrintAndReset()
{
#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
    std::vector<std::pair<EcmaOpcode, Value>> profVec;
    for (auto it = profMap_.begin(); it != profMap_.end(); it++) {
        profVec.emplace_back(std::make_pair(it->first, it->second));
        it->second.ResetStat();
    }
    std::sort(profVec.begin(), profVec.end(),
              [](std::pair<EcmaOpcode, Value> &x, std::pair<EcmaOpcode, Value> &y) -> bool {
                  return x.second.Count() > y.second.Count();
              });

    LOG_ECMA(INFO) << "Runtime Statistics of optimized code path:";
    static constexpr int nameRightAdjustment = 46;
    static constexpr int numberRightAdjustment = 15;
    static constexpr int hundred = 100;
    LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << "Bytecode"
                   << std::setw(numberRightAdjustment) << "bcIndex"
                   << std::setw(numberRightAdjustment) << "Count"
                   << std::setw(numberRightAdjustment) << "TypedPathCount"
                   << std::setw(numberRightAdjustment) << "SlowPathCount"
                   << std::setw(numberRightAdjustment + 1) << "TypedPathRate";
    LOG_ECMA(INFO) << "============================================================"
                   << "=========================================================";

    uint64_t totalCount = 0;
    uint64_t totalTypedPathCount = 0;
    uint64_t totalSlowPathCount = 0;

    for (auto it = profVec.begin(); it != profVec.end(); it++) {
        Value val = it->second;
        if (val.Count() == 0) {
            break;
        }

        LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << kungfu::GetEcmaOpcodeStr(it->first)
                       << std::setw(numberRightAdjustment) << "NA"
                       << std::setw(numberRightAdjustment) << val.Count()
                       << std::setw(numberRightAdjustment) << val.TypedPathCount()
                       << std::setw(numberRightAdjustment) << val.SlowPathCount()
                       << std::setw(numberRightAdjustment) << val.TypedPathCount() * hundred / val.Count() << "%";

        totalCount += val.Count();
        totalTypedPathCount += val.TypedPathCount();
        totalSlowPathCount += val.SlowPathCount();
    }

    if (totalCount != 0) {
        LOG_ECMA(INFO) << "------------------------------------------------------------"
                       << "---------------------------------------------------------";
        LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << "Total"
                       << std::setw(numberRightAdjustment) << "NA"
                       << std::setw(numberRightAdjustment) << totalCount
                       << std::setw(numberRightAdjustment) << totalTypedPathCount
                       << std::setw(numberRightAdjustment) << totalSlowPathCount
                       << std::setw(numberRightAdjustment) << totalTypedPathCount * hundred / totalCount << "%";
    }

    FilterMethodToPrint();
    ResetMethodInfo();
#endif
}

void OptCodeProfiler::FilterMethodToPrint()
{
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER
    std::vector<CString> methods;
    auto &profMap = JitWarmupProfiler::GetInstance()->profMap_;
    for (auto it = profMap.begin(); it != profMap.end();) {
        if (it->second == false) {
            methods.push_back(it->first);
            profMap.erase(it++);
        } else {
            it++;
        }
    }
    for (auto &methodName : methods) {
        if (methodName.find("func_main_") != methodName.npos) {
            continue;
        }
        LOG_ECMA(ERROR) << methodName << " has not been fully jit warmed up.";
    }
#endif
    std::vector<std::pair<uint64_t, Name>> profVec;
    for (auto it = methodIdToName_.begin(); it != methodIdToName_.end(); it++) {
        profVec.emplace_back(std::make_pair(it->first, it->second));
    }
    std::sort(profVec.begin(), profVec.end(),
              [](std::pair<uint64_t, Name> &x, std::pair<uint64_t, Name> &y) -> bool {
                  return x.second.Count() > y.second.Count();
              });

    auto itr = profVec.begin();
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER
    while (itr != profVec.end()) {
#else
    for (int i = 0; i < printMehodCount_ && itr != profVec.end(); i++) {
#endif
        PrintMethodRecord(itr->first, itr->second.GetName());
        itr++;
    }
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER
    if (profMap.size() != 0) {
        for (auto it = profMap.begin(); it != profMap.end(); it++) {
            if (it->first.find("func_main_") != it->first.npos) {
                continue;
            }
            LOG_ECMA(ERROR) << "There exists compiled function " << it->first
                            << ", but it has not been jit executed, please "
                               "warm up strongly.";
        }
    }
#endif
}

void OptCodeProfiler::PrintMethodRecord(Key key, std::string methodName)
{
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER
    CString methodInfo = abcNames_[key.GetAbcId()] + ":" + CString(methodName);
    auto &profMap = JitWarmupProfiler::GetInstance()->profMap_;
    if (profMap.find(methodInfo) != profMap.end()) {
        profMap.erase(methodInfo);
    }
#endif
    LOG_ECMA(INFO) << "==== methodId: " << key.GetMethodId()
                   << ", methodName: " << methodName.c_str()
                   << ", abcName: " << abcNames_[key.GetAbcId()] << " ====";

    static constexpr int nameRightAdjustment = 46;
    static constexpr int numberRightAdjustment = 15;
    static constexpr int hundred = 100;
    BcRecord& bcRecord = methodIdToRecord_[key.Value()];
    for (auto it = bcRecord.begin(); it != bcRecord.end(); it++) {
        Record record = it->second;
#if ECMASCRIPT_ENABLE_JIT_WARMUP_PROFILER == 0
        if (record.Count() == 0) {
            break;
        }
#endif

        LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << kungfu::GetEcmaOpcodeStr(record.GetOpCode())
                       << std::setw(numberRightAdjustment) << it->first
                       << std::setw(numberRightAdjustment) << record.Count()
                       << std::setw(numberRightAdjustment) << record.GetFast()
                       << std::setw(numberRightAdjustment) << record.GetSlow()
                       << std::setw(numberRightAdjustment) << record.GetFast() * hundred / record.Count() << "%";
    }
}

void OptCodeProfiler::Update(JSHandle<JSTaggedValue> &func, int bcIndex, EcmaOpcode opcode, Mode mode)
{
    auto it = profMap_.find(opcode);
    if (it != profMap_.end()) {
        (mode == Mode::TYPED_PATH) ? (it->second.typedPathValue++) : (it->second.slowPathValue++);
    }

    if (func->IsUndefined()) {
        return;
    }

    // methodId & methodName
    auto funcPoint = JSFunction::Cast(func->GetTaggedObject());
    auto method = funcPoint->GetMethod();
    if (!method.IsMethod()) {
        return;
    }
    auto methodPoint = Method::Cast(method);
    auto methodId = methodPoint->GetMethodId().GetOffset();
    auto methodName = ConvertToStdString(methodPoint->GetRecordNameStr()) + "." + methodPoint->GetMethodName();

    const auto *pf = methodPoint->GetJSPandaFile();
    ASSERT(pf != nullptr);
    auto pfName = pf->GetJSPandaFileDesc();
    auto itr = std::find(abcNames_.begin(), abcNames_.end(), pfName);
    uint32_t index = 0;
    if (itr != abcNames_.end()) {
        index = static_cast<uint32_t>(std::distance(abcNames_.begin(), itr));
    } else {
        index = abcNames_.size();
        abcNames_.emplace_back(pfName);
    }

    Key key(index, methodId);
    // deal methodIdToName
    auto result = methodIdToName_.find(key.Value());
    if (result != methodIdToName_.end()) {
        result->second.Inc();
    } else {
        methodIdToName_.emplace(key.Value(), Name(methodName));
    }

    // deal methodIdToRecord_
    auto result2 = methodIdToRecord_.find(key.Value());
    if (result2 == methodIdToRecord_.end()) {
        BcRecord bcRecord;
        bcRecord.emplace(bcIndex, Record(opcode));
        methodIdToRecord_.emplace(key.Value(), bcRecord);
    }
    result2 = methodIdToRecord_.find(key.Value());

    auto result3 = result2->second.find(bcIndex);
    if (result3 != result2->second.end()) {
        (mode == Mode::TYPED_PATH) ? (result3->second.IncFast()) : (result3->second.IncSlow());
    } else {
        auto record = Record(opcode);
        (mode == Mode::TYPED_PATH) ? (record.IncFast()) : (record.IncSlow());
        result2->second.emplace(bcIndex, record);
    }
}

OptCodeProfiler::~OptCodeProfiler()
{
    PrintAndReset();
}
} // namespace panda::ecmascript
