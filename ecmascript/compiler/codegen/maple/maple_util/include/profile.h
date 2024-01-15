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

#ifndef MAPLE_UTIL_INCLUDE_PROFILE_H
#define MAPLE_UTIL_INCLUDE_PROFILE_H
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "profile_type.h"
#include "mpl_logging.h"
#include "types_def.h"
#include "option.h"

namespace maple {
struct IRProfileDesc {
    uint32 counterStart = 0;
    uint32 counterEnd = 0;
    uint64 funcHash = 0;
    IRProfileDesc() = default;
    IRProfileDesc(uint64 hash, uint32 start, uint32 end) : counterStart(start), counterEnd(end), funcHash(hash) {}
    ~IRProfileDesc() = default;
};

class Profile {
public:
    struct FuncItem {
        uint32 callTimes;
        uint8 type;
    };
    // function ir profile info
    struct BBInfo {
        uint64 funcHash = 0;
        uint32 totalCounter = 0;
        std::vector<uint32> counter;
        BBInfo() = default;
        BBInfo(uint64 hash, uint32 num, std::vector<uint32> &&counter)
            : funcHash(hash), totalCounter(num), counter(counter)
        {
        }
        BBInfo(uint64 hash, uint32 num, const std::initializer_list<uint32> &iList)
            : funcHash(hash), totalCounter(num), counter(iList)
        {
        }
        ~BBInfo() = default;
    };

    static const uint8 stringEnd;
    bool CheckFuncHot(const std::string &className) const;
    bool CheckMethodHot(const std::string &className) const;
    bool CheckMethodSigHot(const std::string &methodSigStr) const;
    bool CheckFieldHot(const std::string &className) const;
    bool CheckClassHot(const std::string &className) const;
    bool CheckLiteralHot(const std::string &literal) const;
    bool CheckReflectionStrHot(const std::string &str, uint8 &layoutType) const;
    // default get all kind profile
    bool DeCompress(const std::string &fileName, const std::string &dexName, ProfileType type = kAll);
    const std::unordered_map<std::string, FuncItem> &GetFunctionProf() const;
    bool GetFunctionBBProf(const std::string &funcName, BBInfo &result);
    size_t GetLiteralProfileSize() const;
    bool CheckProfValid() const;
    bool CheckDexValid(uint32 idx) const;
    void SetProfileMode();
    void Dump() const;
    void DumpFuncIRProfUseInfo() const;
    void SetFuncStatus(const std::string &funcName, bool succ);
    Profile();
    ~Profile() = default;

    bool IsValid() const
    {
        return valid;
    }

private:
    bool valid = false;
    bool profileMode = false;
    bool isCoreSo = false;
    bool isAppProfile = false;
    static bool debug;
    static uint32 hotFuncCountThreshold;
    static bool initialized;
    std::vector<std::string> strMap;
    std::string dexName;
    std::string appPackageName;
    std::unordered_set<std::string> classMeta;
    std::unordered_set<std::string> methodMeta;
    std::unordered_set<std::string> fieldMeta;
    std::unordered_set<std::string> methodSigMeta;
    std::unordered_set<std::string> literal;
    std::unordered_map<std::string, uint8> reflectionStrData;
    std::unordered_map<std::string, Profile::FuncItem> funcProfData;
    std::unordered_set<std::string> &GetMeta(uint8 type);
    std::unordered_map<std::string, BBInfo> funcBBProfData;
    std::unordered_map<std::string, bool> funcBBProfUseInfo;
    std::unordered_map<std::string, IRProfileDesc> funcDesc;
    std::vector<uint32> counterTab;
    static const std::string preClassHot[];
    bool CheckProfileHeader(const Header &header) const;
    std::string GetProfileNameByType(uint8 type) const;
    std::string GetFunctionName(uint32 classIdx, uint32 methodIdx, uint32 sigIdx) const;
    std::string GetMethodSigStr(uint32 methodIdx, uint32 sigIdx) const;
    void ParseMethodSignature(const char *data, int fileNum, std::unordered_set<std::string> &metaData) const;
    void ParseMeta(const char *data, int fileNum, std::unordered_set<std::string> &metaData) const;
    void ParseReflectionStr(const char *data, int fileNum);
    void ParseFunc(const char *data, int fileNum);
    void ParseLiteral(const char *data, const char *end);
    void ParseIRFuncDesc(const char *data, int fileNum);
    void ParseCounterTab(const char *data, int fileNum);
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_PROFILE_H
