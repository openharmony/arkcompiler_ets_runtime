/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/stackmap/ark_stackmap_builder.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript::kungfu {
class LLVMStackMapParser {
public:
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr);
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
                                      uintptr_t hostCodeSectionAddr,
                                      uintptr_t hostCodeSectionOffset);
    void PUBLIC_API Print() const
    {
        if (IsLogEnabled()) {
            llvmStackMap_.Print();
        }
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    LLVMStackMapParser(LLVMStackMapInfo &stackMapInfo, bool enableLog = false)
        : stackMapInfo(stackMapInfo),
          enableLog_(enableLog)
    {
        fun2RecordNum_.clear();
        dataInfo_ = nullptr;
        funAddr_.clear();
        fun2FpDelta_.clear();
        llvmStackMap_.head.stackmapversion = 0;
        llvmStackMap_.head.reserved0 = 0;
        llvmStackMap_.head.reserved1 = 0;
    }
    ~LLVMStackMapParser()
    {
        fun2RecordNum_.clear();
        dataInfo_ = nullptr;
        funAddr_.clear();
        fun2FpDelta_.clear();
    }

private:
    static const size_t GC_PAIR_SIZE = 2;
    void FilterCallSiteInfo(LLVMStackMapType::CallSiteInfo &info);
    void CalcCallSite();
    struct LLVMStackMap llvmStackMap_;
    // use reference here to avoid extra copy
    LLVMStackMapInfo &stackMapInfo;
    std::vector<std::pair<uintptr_t, uint64_t>> fun2RecordNum_;
    std::unique_ptr<DataInfo> dataInfo_;
    bool enableLog_ {false};
    std::set<uintptr_t> funAddr_;
    std::vector<LLVMStackMapType::Func2FpDelta> fun2FpDelta_;
    std::map<uint32_t, std::vector<LLVMStackMapType::Func2FpDelta>> module2fun2FpDelta_;
    std::map<uint32_t, std::set<uintptr_t>> module2funAddr_;
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LLVM_STACKMAP_PARSER_H
