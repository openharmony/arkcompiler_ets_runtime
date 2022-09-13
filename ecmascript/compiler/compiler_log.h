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

#ifndef ECMASCRIPT_COMPILER_LOG_H
#define ECMASCRIPT_COMPILER_LOG_H

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace panda::ecmascript::kungfu {
class CompilerLog {
public:
    CompilerLog(const std::string &logOpt, bool enableBCTrace = false)
    {
        outputCIR_ = logOpt.find("cir") != std::string::npos ||
            logOpt.find("0") != std::string::npos;
        outputLLIR_ = logOpt.find("llir") != std::string::npos ||
            logOpt.find("1") != std::string::npos;
        outputASM_ = logOpt.find("asm") != std::string::npos ||
            logOpt.find("2") != std::string::npos;
        outputType_ = logOpt.find("type") != std::string::npos ||
            logOpt.find("3") != std::string::npos;
        allMethod_ = logOpt.find("all") != std::string::npos;
        cerMethod_ = logOpt.find("all") == std::string::npos &&
            logOpt.find("cer") != std::string::npos;
        noneMethod_ = logOpt.find("all") == std::string::npos &&
            logOpt.find("cer") == std::string::npos;
        enableBCTrace_ = enableBCTrace;
    }
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

    bool IsEnableByteCodeTrace() const
    {
        return enableBCTrace_;
    }
private:
    bool allMethod_ {false};
    bool cerMethod_ {false};
    bool noneMethod_ {false};
    bool outputCIR_ {false};
    bool outputLLIR_ {false};
    bool outputASM_ {false};
    bool outputType_ {false};
    bool enableBCTrace_ {false};
};

class MethodLogList {
public:
    explicit MethodLogList(const std::string &logMethods) : methods_(logMethods) {}
    ~MethodLogList() = default;
    bool IncludesMethod(const std::string &methodName) const
    {
        bool empty = methodName.empty();
        bool found = methods_.find(methodName) != std::string::npos;
        return !empty && found;
    }
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

    bool IncludesMethod(const std::string &fileName, const std::string &methodName) const
    {
        if (fileMethods_.find(fileName) == fileMethods_.end()) {
            return false;
        }
        std::vector mehtodVector = fileMethods_.at(fileName);
        auto it = find(mehtodVector.begin(), mehtodVector.end(), methodName);
        return (it != mehtodVector.end());
    }

private:
    std::vector<std::string> spiltString(const std::string &str, const char ch)
    {
        std::vector<std::string> vec{};
        std::istringstream sstr(str.c_str());
        std::string spilt;
        while (getline(sstr, spilt, ch)) {
            vec.emplace_back(spilt);
        }
        return vec;
    }

    void ParseFileMethodsName(const std::string &logMethods)
    {
        std::vector<std::string> fileVector = spiltString(logMethods, fileSplitSign);
        std::vector<std::string> itemVector;
        for (size_t index = 0; index < fileVector.size(); ++index) {
            itemVector = spiltString(fileVector[index], methodSplitSign);
            std::vector<std::string> methodVector(itemVector.begin() + 1, itemVector.end());
            fileMethods_[itemVector[0]] = methodVector;
        }
    }

    std::map<std::string, std::vector<std::string>> fileMethods_ {};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LOG_H
