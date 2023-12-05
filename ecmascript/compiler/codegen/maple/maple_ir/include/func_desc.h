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

#ifndef MAPLE_IR_INCLUDE_FUNC_DESC_H
#define MAPLE_IR_INCLUDE_FUNC_DESC_H
#include "mpl_logging.h"
namespace maple {

enum class FI {
    kUnknown = 0,
    kPure,   // means this function will not modify any global memory.
    kConst,  // means this function will not read/modify any global memory.
};

static std::string kFIStr[] = {"kUnknown", "kPure", "kConst"};

enum class RI {
    kUnknown = 0,  // for ptr value, don't know anything.
    kNoAlias,      // for ptr value, no alias with any other ptr when this method is returned. As in malloc.
    kAliasParam0,  // for ptr value, it may alias with first param. As in memcpy.
    kAliasParam1,
    kAliasParam2,
    kAliasParam3,
    kAliasParam4,
    kAliasParam5,
};

static std::string kRIStr[] = {"kUnknown",     "kNoAlias",     "kAliasParam0", "kAliasParam1",
                               "kAliasParam2", "kAliasParam3", "kAliasParam4", "kAliasParam5"};

enum class PI {
    kUnknown = 0,      // for ptr param, may read/write every level memory.
    kReadWriteMemory,  // for ptr param, only read & write the memory it points to.
    kWriteMemoryOnly,  // for ptr param, only write the memory it points to.
    kReadMemoryOnly,   // for ptr param, only read the memory it points to.
    kReadSelfOnly,     // for ptr param, only read the ptr itself, do not dereference.
    kUnused,           // this param is not used in this function.
};

static std::string kPIStr[] = {"kUnknown",        "kReadWriteMemory", "kWriteMemoryOnly",
                               "kReadMemoryOnly", "kReadSelfOnly",    "kUnused"};

// most function has less than 6 parameters.
const size_t kMaxParamCount = 6;
struct FuncDesc {
    FI funcInfo {};
    RI returnInfo {};
    PI paramInfo[kMaxParamCount] {};
    bool configed = false;

    void InitToBest()
    {
        funcInfo = FI::kConst;
        returnInfo = RI::kNoAlias;
        for (size_t idx = 0; idx < kMaxParamCount; ++idx) {
            paramInfo[idx] = PI::kUnused;
        }
    }

    bool Equals(const FuncDesc &desc) const
    {
        if (funcInfo != desc.funcInfo) {
            return false;
        }
        if (returnInfo != desc.returnInfo) {
            return false;
        }
        for (size_t idx = 0; idx < kMaxParamCount; ++idx) {
            if (paramInfo[idx] != desc.paramInfo[idx]) {
                return false;
            }
        }
        return true;
    }

    bool IsConfiged() const
    {
        return configed;
    }

    void SetConfiged()
    {
        configed = true;
    }

    bool IsConst() const
    {
        return funcInfo == FI::kConst;
    }

    bool IsPure() const
    {
        return funcInfo == FI::kPure;
    }

    bool IsReturnNoAlias() const
    {
        return returnInfo == RI::kNoAlias;
    }
    bool IsReturnAlias() const
    {
        return returnInfo >= RI::kAliasParam0;
    }

    size_t EnumToIndex(const RI &ri) const
    {
        switch (ri) {
            case RI::kAliasParam0:
                return 0;
            case RI::kAliasParam1:
                return 1;
            case RI::kAliasParam2:
                return 2; // 2 is kAliasParam2
            case RI::kAliasParam3:
                return 3; // 3 is kAliasParam3
            case RI::kAliasParam4:
                return 4; // 4 is kAliasParam4
            case RI::kAliasParam5:
                return 5; // 5 is kAliasParam5
            default: {
                CHECK_FATAL(false, "Impossible.");
            }
        }
    }

    size_t ReturnParamX() const
    {
        CHECK_FATAL(returnInfo >= RI::kAliasParam0, "Impossible.");
        return EnumToIndex(returnInfo);
    }

    PI GetParamInfo(size_t index) const
    {
        return paramInfo[index];
    }

    bool IsArgReadSelfOnly(size_t index) const
    {
        if (index >= kMaxParamCount) {
            return false;
        }
        return paramInfo[index] == PI::kReadSelfOnly;
    }

    bool IsArgReadMemoryOnly(size_t index) const
    {
        if (index >= kMaxParamCount) {
            return false;
        }
        return paramInfo[index] == PI::kReadMemoryOnly;
    }

    bool IsArgWriteMemoryOnly(size_t index) const
    {
        if (index >= kMaxParamCount) {
            return false;
        }
        return paramInfo[index] == PI::kWriteMemoryOnly;
    }

    bool IsArgUnused(size_t index) const
    {
        if (index >= kMaxParamCount) {
            return false;
        }
        return paramInfo[index] == PI::kUnused;
    }

    void SetFuncInfo(const FI fi)
    {
        funcInfo = fi;
    }

    void SetFuncInfoNoBetterThan(const FI fi)
    {
        auto oldValue = static_cast<size_t>(funcInfo);
        auto newValue = static_cast<size_t>(fi);
        if (newValue < oldValue) {
            SetFuncInfo(static_cast<FI>(newValue));
        }
    }

    void SetReturnInfo(const RI ri)
    {
        returnInfo = ri;
    }

    void SetParamInfo(const size_t idx, const PI pi)
    {
        if (idx >= kMaxParamCount) {
            return;
        }
        paramInfo[idx] = pi;
    }

    void SetParamInfoNoBetterThan(const size_t idx, const PI pi)
    {
        size_t oldValue = static_cast<size_t>(paramInfo[idx]);
        size_t newValue = static_cast<size_t>(pi);
        if (newValue < oldValue) {
            SetParamInfo(idx, static_cast<PI>(newValue));
        }
    }

    void Dump(size_t numParam = kMaxParamCount)
    {
        auto dumpCount = numParam > kMaxParamCount ? kMaxParamCount : numParam;
        LogInfo::MapleLogger() << kFIStr[static_cast<size_t>(funcInfo)] << " "
                               << kRIStr[static_cast<size_t>(returnInfo)];
        for (size_t i = 0; i < dumpCount; ++i) {
            LogInfo::MapleLogger() << " " << kPIStr[static_cast<size_t>(paramInfo[i])];
        }
        LogInfo::MapleLogger() << "\n";
    }
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_FUNC_DESC_H
