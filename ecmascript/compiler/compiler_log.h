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

#ifndef ECMASCRIPT_COMPILER_LOG_H
#define ECMASCRIPT_COMPILER_LOG_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/compiler/argument_accessor.h"

namespace panda::ecmascript::kungfu {
class AotMethodLogList;
class MethodLogList;

class CompilerLog {
public:
    explicit CompilerLog(const std::string &logOpt);
    CompilerLog() = default;
    ~CompilerLog() = default;

    bool AllMethod() const
    {
        return allMethod_;
    }

    bool CertainMethod() const
    {
        return cerMethod_;
    }

    bool NoneMethod() const
    {
        return noneMethod_;
    }

    bool OutputCIR() const
    {
        return outputCIR_;
    }

    bool OutputLLIR() const
    {
        return outputLLIR_;
    }

    bool OutputASM() const
    {
        return outputASM_;
    }

    bool OutputType() const
    {
        return outputType_;
    }

    bool GetEnableCompilerLogTime() const
    {
        return compilerLogTime_;
    }

    void SetEnableCompilerLogTime(bool compilerLogTime)
    {
        compilerLogTime_ = compilerLogTime;
    }

    bool GetEnableMethodLog() const
    {
        return enableMethodLog_;
    }

    void SetEnableMethodLog(bool enableMethodLog)
    {
        enableMethodLog_ = enableMethodLog;
    }

    bool EnableMethodCIRLog() const
    {
        return GetEnableMethodLog() && OutputCIR();
    }

    bool EnableMethodASMLog() const
    {
        return GetEnableMethodLog() && OutputASM();
    }

    void SetMethodLog(const std::string &fileName, const std::string &methodName, AotMethodLogList *logList);
    void SetStubLog(const std::string& stubName, MethodLogList* logList);
    void Print() const;
    void AddMethodTime(const std::string& name, uint32_t id, double time);
    void AddPassTime(const std::string& name, double time);
    int GetIndex();

    std::map<std::string, int> nameIndex_;

private:
    static constexpr int RECORD_LENS = 64;
    static constexpr int PASS_LENS = 32;
    static constexpr int METHOD_LENS = 16;
    static constexpr int OFFSET_LENS = 8;
    static constexpr int PERCENT_LENS = 4;
    static constexpr int TIME_LENS = 8;
    static constexpr int MILLION_TIME = 1000;
    static constexpr int HUNDRED_TIME = 100;

    void PrintPassTime() const;
    void PrintMethodTime() const;
    void PrintTime() const;

    int idx_ {0};
    bool allMethod_ {false};
    bool cerMethod_ {false};
    bool noneMethod_ {false};
    bool outputCIR_ {false};
    bool outputLLIR_ {false};
    bool outputASM_ {false};
    bool outputType_ {false};
    bool compilerLogTime_ {true};
    bool enableMethodLog_ {false};
    std::map<std::string, double> timePassMap_ {};
    std::map<std::pair<uint32_t, std::string>, double> timeMethodMap_ {};
};

class MethodLogList {
public:
    explicit MethodLogList(const std::string &logMethods) : methods_(logMethods) {}
    ~MethodLogList() = default;
    bool IncludesMethod(const std::string &methodName) const;
private:
    std::string methods_ {};
};

class AotMethodLogList : public MethodLogList {
public:
    static const char fileSplitSign = ':';
    static const char methodSplitSign = ',';

    explicit AotMethodLogList(const std::string &logMethods) : MethodLogList(logMethods)
    {
        ParseFileMethodsName(logMethods);
    }
    ~AotMethodLogList() = default;

    bool IncludesMethod(const std::string &fileName, const std::string &methodName) const;

private:
    std::vector<std::string> spiltString(const std::string &str, const char ch);
    void ParseFileMethodsName(const std::string &logMethods);
    std::map<std::string, std::vector<std::string>> fileMethods_ {};
};

class TimeScope : public ClockScope {
public:
    TimeScope(std::string name, std::string methodName, uint32_t methodOffset, CompilerLog* log);
    TimeScope(std::string name, CompilerLog* log);
    ~TimeScope();

private:
    static constexpr int PASS_LENS = 32;
    static constexpr int METHOD_LENS = 16;
    static constexpr int OFFSET_LENS = 8;
    static constexpr int TIME_LENS = 8;
    static constexpr int MILLION_TIME = 1000;

    std::string name_ {""};
    double startTime_ {0};
    double timeUsed_ {0};
    std::string methodName_ {""};
    uint32_t methodOffset_ {0};
    CompilerLog *log_ {nullptr};

    const std::string GetShortName(const std::string& methodName);
};

class PGOTypeLogList {
public:
    explicit PGOTypeLogList(Circuit *circuit) : acc_(circuit) {}
    ~PGOTypeLogList() = default;
    void CollectGateTypeLogInfo(GateRef gate, bool isBinOp);
    void PrintPGOTypeLog();
private:
    GateAccessor acc_;
    std::string log_ {};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LOG_H
