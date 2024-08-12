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
#ifndef ECMASCRIPT_LITECG_STACKMAP_TYPE_H
#define ECMASCRIPT_LITECG_STACKMAP_TYPE_H

#include <cstdint>
#include <map>
#include <vector>
#include "ecmascript/stackmap/cg_stackmap.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript::kungfu {
class LiteCGStackMapType {
public:
    using Pc2CallSiteInfo = std::unordered_map<uint64_t, std::vector<uint8_t>>;
    using Pc2Deopt = std::map<uint64_t, std::vector<uint8_t>>;
};

class LiteCGStackMapInfo : public CGStackMapInfo {
public:
    LiteCGStackMapInfo() : CGStackMapInfo() {}
    ~LiteCGStackMapInfo() = default;

    const std::vector<LiteCGStackMapType::Pc2CallSiteInfo> &GetCallSiteInfoVec() const
    {
        return pc2CallSiteInfoVec_;
    }

    const std::vector<LiteCGStackMapType::Pc2Deopt> &GetDeoptInfoVec() const
    {
        return pc2DeoptVec_;
    }

    void AppendCallSiteInfo(const LiteCGStackMapType::Pc2CallSiteInfo &callSiteInfo)
    {
        pc2CallSiteInfoVec_.push_back(callSiteInfo);
    }

    void AppendDeoptInfo(const LiteCGStackMapType::Pc2Deopt &deoptInfo)
    {
        pc2DeoptVec_.push_back(deoptInfo);
    }

    CGStackMapKind GetStackMapKind() const override
    {
        return kLiteCGStackMapInfo;
    }

    void ConvertToLLVMStackMapInfo(
        std::vector<LLVMStackMapType::Pc2CallSiteInfo> &pc2StackMapsVec,
        std::vector<LLVMStackMapType::Pc2Deopt> &pc2DeoptInfoVec, Triple triple) const;
private:
    std::vector<LiteCGStackMapType::Pc2CallSiteInfo> pc2CallSiteInfoVec_;
    std::vector<LiteCGStackMapType::Pc2Deopt> pc2DeoptVec_;
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LITECG_STACKMAP_TYPE_H