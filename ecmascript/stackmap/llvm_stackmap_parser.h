/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_LLVM_STACKMAP_PARSER_H
#define ECMASCRIPT_LLVM_STACKMAP_PARSER_H

#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ecmascript/common.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/stackmap/llvm_stackmap_type.h"

namespace panda::ecmascript::kungfu {
class LLVMStackMapParser {
public:
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr);
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr);
    void PUBLIC_API Print() const
    {
        if (IsLogEnabled()) {
            llvmStackMap_.Print();
        }
    }
    const CallSiteInfo *GetCallSiteInfoByPc(uintptr_t funcAddr) const;
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    void PUBLIC_API CalculateFuncFpDelta(Func2FpDelta info, uint32_t moduleIndex);

    explicit LLVMStackMapParser(bool enableLog = false)
    {
        pc2CallSiteInfoVec_.clear();
        fun2RecordNum_.clear();
        dataInfo_ = nullptr;
        enableLog_ = enableLog;
        funAddr_.clear();
        fun2FpDelta_.clear();
        pc2DeoptVec_.clear();
    }
    ~LLVMStackMapParser()
    {
        pc2CallSiteInfoVec_.clear();
        fun2RecordNum_.clear();
        dataInfo_ = nullptr;
        funAddr_.clear();
        fun2FpDelta_.clear();
        pc2DeoptVec_.clear();
    }
    std::vector<Pc2CallSiteInfo> PUBLIC_API GetPc2StackMapVec() const
    {
        return pc2CallSiteInfoVec_;
    }
    std::vector<Pc2Deopt> GetPc2Deopt() const
    {
        return pc2DeoptVec_;
    }
private:
    void CalcCallSite();
    struct LLVMStackMap llvmStackMap_;
    std::vector<Pc2CallSiteInfo> pc2CallSiteInfoVec_;
    std::vector<std::pair<uintptr_t, uint64_t>> fun2RecordNum_;
    [[maybe_unused]] std::unique_ptr<DataInfo> dataInfo_;
    bool enableLog_ {false};
    std::set<uintptr_t> funAddr_;
    std::vector<Func2FpDelta> fun2FpDelta_;
    std::vector<Pc2Deopt> pc2DeoptVec_;
    std::map<uint32_t, std::vector<Func2FpDelta>> module2fun2FpDelta_;
    std::map<uint32_t, std::set<uintptr_t>> module2funAddr_;
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LLVM_STACKMAP_PARSER_H
