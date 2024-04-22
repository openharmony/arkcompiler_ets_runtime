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

#include "ecmascript/compiler/compiler_log.h"

namespace panda::ecmascript::kungfu {
CompilerLog::CompilerLog(const std::string &logOpt)
{
    outputCIR_ = (logOpt.find("cir") != std::string::npos) || (logOpt.find("0") != std::string::npos);
    outputLLIR_ = (logOpt.find("llir") != std::string::npos) || (logOpt.find("1") != std::string::npos);
    outputASM_ = (logOpt.find("asm") != std::string::npos) || (logOpt.find("2") != std::string::npos);
    outputType_ = (logOpt.find("type") != std::string::npos) || (logOpt.find("3") != std::string::npos);
    allMethod_ = logOpt.find("all") != std::string::npos;
    cerMethod_ = (logOpt.find("all") == std::string::npos) && (logOpt.find("cer") != std::string::npos);
    noneMethod_ = (logOpt.find("all") == std::string::npos) && (logOpt.find("cer") == std::string::npos);
}

void CompilerLog::SetMethodLog(const std::string &fileName,
                               const std::string &methodName, AotMethodLogList *logList)
{
    bool enableMethodLog = !NoneMethod();
    if (CertainMethod()) {
        enableMethodLog = logList->IncludesMethod(fileName, methodName);
    }
    SetEnableMethodLog(enableMethodLog);
}

void CompilerLog::SetStubLog(const std::string &stubName, MethodLogList *logList)
{
    bool enableMethodLog = !NoneMethod();
    if (CertainMethod()) {
        enableMethodLog = logList->IncludesMethod(stubName);
    }
    SetEnableMethodLog(enableMethodLog);
}

bool MethodLogList::IncludesMethod(const std::string &methodName) const
{
    bool empty = methodName.empty();
    bool found = methods_.find(methodName) != std::string::npos;
    return !empty && found;
}

bool AotMethodLogList::IncludesMethod(const std::string &fileName, const std::string &methodName) const
{
    if (fileMethods_.find(fileName) == fileMethods_.end()) {
        return false;
    }
    std::vector methodVector = fileMethods_.at(fileName);
    auto it = find(methodVector.begin(), methodVector.end(), methodName);
    return (it != methodVector.end());
}

std::vector<std::string> AotMethodLogList::spiltString(const std::string &str, const char ch)
{
    std::vector<std::string> vec {};
    std::istringstream sstr(str.c_str());
    std::string spilt;
    while (getline(sstr, spilt, ch)) {
        vec.emplace_back(spilt);
    }
    return vec;
}

void AotMethodLogList::ParseFileMethodsName(const std::string &logMethods)
{
    std::vector<std::string> fileVector = spiltString(logMethods, fileSplitSign);
    std::vector<std::string> itemVector;
    for (size_t index = 0; index < fileVector.size(); ++index) {
        itemVector = spiltString(fileVector[index], methodSplitSign);
        std::vector<std::string> methodVector(itemVector.begin() + 1, itemVector.end());
        fileMethods_[itemVector[0]] = methodVector;
    }
}

TimeScope::TimeScope(std::string name, std::string methodName, uint32_t methodOffset, CompilerLog* log)
    : ClockScope(), name_(std::move(name)), methodName_(std::move(methodName)), methodOffset_(methodOffset), log_(log)
{
    if (log_->GetEnableCompilerLogTime()) {
        if (log_->nameIndex_.find(name_) == log_->nameIndex_.end()) {
            log_->nameIndex_[name_] = log_->GetIndex();
        }
        startTime_ = ClockScope().GetCurTime();
    }
}

TimeScope::TimeScope(std::string name, CompilerLog* log)
    : ClockScope(), name_(std::move(name)), log_(log)
{
    if (log_->GetEnableCompilerLogTime()) {
        if (log_->nameIndex_.find(name_) == log_->nameIndex_.end()) {
            log_->nameIndex_[name_] = log_->GetIndex();
        }
        startTime_ = ClockScope().GetCurTime();
    }
}

TimeScope::~TimeScope()
{
    if (log_->GetEnableCompilerLogTime()) {
        timeUsed_ = ClockScope().GetCurTime() - startTime_;
        if (log_->CertainMethod() && log_->GetEnableMethodLog()) {
            LOG_COMPILER(INFO) << std::setw(PASS_LENS) << name_ << " " << std::setw(METHOD_LENS)
                               << GetShortName(methodName_) << " offset:" << std::setw(OFFSET_LENS) << methodOffset_
                               << " time used:" << std::setw(TIME_LENS) << timeUsed_ / MILLION_TIME << "ms";
        }
        std::string shortName = GetShortName(methodName_);
        log_->AddPassTime(name_, timeUsed_);
        log_->AddMethodTime(shortName, methodOffset_, timeUsed_);
    }
}

const std::string TimeScope::GetShortName(const std::string& methodName)
{
    std::string KeyStr = "@";
    if (methodName.find(KeyStr) != std::string::npos) {
        std::string::size_type index = methodName.find(KeyStr);
        std::string extracted = methodName.substr(0, index);
        return extracted;
    } else {
        return methodName;
    }
}

void CompilerLog::Print() const
{
    if (compilerLogTime_) {
        PrintTime();
    }
    PrintPGOMismatchedMethod();
    PrintCompiledMethod();
}

void CompilerLog::PrintPassTime() const
{
    double allPassTimeforAllMethods = 0;
    auto myMap = timePassMap_;
    auto myIndexMap = nameIndex_;
    std::multimap<int, std::string> PassTimeMap;
    for (auto it = myMap.begin(); it != myMap.end(); it++) {
        PassTimeMap.insert(make_pair(myIndexMap[it->first], it->first));
        allPassTimeforAllMethods += it->second;
    }
    for (auto [key, val] : PassTimeMap) {
        LOG_COMPILER(INFO) << std::setw(PASS_LENS) << val << " Total cost time is "<< std::setw(TIME_LENS)
                           << myMap[val] / MILLION_TIME << "ms " << "percentage:"
                           << std::fixed << std::setprecision(PERCENT_LENS)
                           << myMap[val] / allPassTimeforAllMethods * HUNDRED_TIME << "% ";
    }
}

void CompilerLog::PrintMethodTime() const
{
    double methodTotalTime = 0;
    auto myMap = timeMethodMap_;
    std::multimap<double, std::pair<uint32_t, std::string>> MethodTimeMap;
    std::map<std::pair<uint32_t, std::string>, double>::iterator it;
    for (it = myMap.begin(); it != myMap.end(); it++) {
        MethodTimeMap.insert(make_pair(it->second, it->first));
    }
    for (auto [key, val] : MethodTimeMap) {
        methodTotalTime += key;
    }
    for (auto [key, val] : MethodTimeMap) {
        LOG_COMPILER(INFO) << "method:" << std::setw(METHOD_LENS) << val.second
                           << " offset:" << std::setw(OFFSET_LENS) << val.first << " all pass cost time is "
                           << std::setw(TIME_LENS) << key / MILLION_TIME << "ms " << "percentage:" << std::fixed
                           << std::setprecision(PERCENT_LENS) << key / methodTotalTime * HUNDRED_TIME << "% ";
    }
    LOG_COMPILER(INFO) << "total compile time is " << std::setw(TIME_LENS) << methodTotalTime / MILLION_TIME << "ms ";
}

void CompilerLog::PrintTime() const
{
    LOG_COMPILER(INFO) << " ";
    PrintPassTime();
    LOG_COMPILER(INFO) << " ";
    PrintMethodTime();
}

void CompilerLog::PrintCompiledMethod() const
{
    LOG_COMPILER(INFO) << " ";
    LOG_COMPILER(INFO) << " Total number of full compiled methods is: " << compiledMethodSet_.size();
    for (auto it = compiledMethodSet_.begin(); it != compiledMethodSet_.end(); it++) {
        LOG_COMPILER(INFO) << " method: " << std::setw(METHOD_LENS) << it->first
                           << " in record: " << std::setw(RECORD_LENS) << it->second
                           << " has been full compiled ";
    }
}

void CompilerLog::PrintPGOMismatchedMethod() const
{
    if (totalPGOMethodCount_ == 0) {
        return;
    }
    LOG_COMPILER(INFO) << " ";
    LOG_COMPILER(INFO) << " Number of mismatched methods from ap file : " << mismatchPGOMethodCount_ << " / "
                       << totalPGOMethodCount_;
    for (const auto &it : mismatchPGOMethodSet_) {
        LOG_COMPILER(INFO) << " method: " << std::setw(METHOD_LENS) << it.first
                           << " in record: " << std::setw(RECORD_LENS) << it.second
                           << " has not been found in abc, and will be abandoned.";
    }
}

void CompilerLog::AddMethodTime(const std::string& name, uint32_t id, double time)
{
    auto methodInfo = std::make_pair(id, name);
    timeMethodMap_[methodInfo] += time;
}

void CompilerLog::AddPassTime(const std::string& name, double time)
{
    timePassMap_[name] += time;
}

void CompilerLog::AddCompiledMethod(const std::string& name, const CString& recordName)
{
    auto info = std::make_pair(name, recordName);
    compiledMethodSet_.insert(info);
}

void CompilerLog::RemoveCompiledMethod(const std::string& name, const CString& recordName)
{
    auto info = std::make_pair(name, recordName);
    compiledMethodSet_.erase(info);
}

int CompilerLog::GetIndex()
{
    return (idx_++);
}

void PGOTypeLogList::CollectGateTypeLogInfo(GateRef gate, bool isBinOp)
{
    log_ += "[PGOTypePrinter] ";
    log_ += "gate id: "+ std::to_string(acc_.GetId(gate)) + ", ";
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    log_ += "bytecode: " + GetEcmaOpcodeStr(ecmaOpcode);

    uint32_t pcOffset = acc_.TryGetPcOffset(gate);
    if (isBinOp) {
        const PGOSampleType *sampleType = acc_.TryGetPGOType(gate).GetPGOSampleType();
        if (sampleType->IsString()) {
            log_ += " [left type: string, right type: string]";
        } else if (sampleType->HasNumber()) {
            if (sampleType->IsInt()) {
                log_ += " [left type: int, right type: int]";
            } else {
                if (sampleType->IsIntOverFlow()) {
                    log_ += " [left type: int, right type: int]";
                } else if (sampleType->IsDouble()) {
                    log_ += " [left type: double, right type: double]";
                } else {
                    log_ += " [left type: number, right type: number]";
                }
            }
        } else {
            GateRef left = acc_.GetValueIn(gate, 0);
            GateRef right = acc_.GetValueIn(gate, 1);
            GateType leftType = acc_.GetGateType(left);
            GateType rightType = acc_.GetGateType(right);
            if (leftType.IsIntType()) {
                log_ += " [left type: int, ";
            } else if (leftType.IsDoubleType()) {
                log_ += " [left type: double, ";
            } else {
                log_ += " [left type: number, ";
            }
            if (rightType.IsIntType()) {
                log_ += "right type: int]";
            } else if (rightType.IsDoubleType()) {
                log_ += "right type: double]";
            } else {
                log_ += "right type: number]";
            }
        }
    } else {
        GateRef value = acc_.GetValueIn(gate, 0);
        GateType valueType = acc_.GetGateType(value);
        if (valueType.IsIntType()) {
            log_ += " [type: int]";
        } else if (valueType.IsDoubleType()) {
            log_ += " [type: double]";
        } else {
            log_ += " [type: number]";
        }
    }
    log_ += ", pcOffset: " + std::to_string(pcOffset);
    log_ += "\n[compiler] ";
}

void PGOTypeLogList::PrintPGOTypeLog()
{
    LOG_COMPILER(INFO) << log_;
}

void CompilerLog::SetPGOMismatchResult(uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                                       std::set<std::pair<std::string, CString>> &mismatchMethodSet)
{
    totalPGOMethodCount_ = totalMethodCount;
    mismatchPGOMethodCount_ = mismatchMethodCount;
    mismatchPGOMethodSet_ = std::move(mismatchMethodSet);
}

} // namespace panda::ecmascript::kungfu
