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
#include "ecmascript/deoptimizer/calleeReg.h"
#include <iostream>

namespace panda::ecmascript::kungfu {
CalleeReg::CalleeReg()
{
#if defined(PANDA_TARGET_AMD64)
    reg2Location_ = {
        {DwarfReg::RBX, 0},
        {DwarfReg::R15, 1},
        {DwarfReg::R14, 2},
        {DwarfReg::R13, 3},
        {DwarfReg::R12, 4},
    };
#elif defined(PANDA_TARGET_ARM64)
    reg2Location_ = {
        {DwarfReg::D8, 0},
        {DwarfReg::D9, 1},
        {DwarfReg::D10, 2},
        {DwarfReg::D11, 3},
        {DwarfReg::D12, 4},
        {DwarfReg::D13, 5},
        {DwarfReg::D14, 6},
        {DwarfReg::D15, 7},

        {DwarfReg::X19, 8},
        {DwarfReg::X20, 9},
        {DwarfReg::X21, 10},
        {DwarfReg::X22, 11},
        {DwarfReg::X23, 12},
        {DwarfReg::X24, 13},
        {DwarfReg::X25, 14},
        {DwarfReg::X26, 15},
        {DwarfReg::X27, 16},
        {DwarfReg::X28, 17},
    };
#endif
}

int CalleeReg::FindCallRegOrder(const LLVMStackMapType::DwarfRegType reg) const
{
    auto it = reg2Location_.find(static_cast<DwarfReg>(reg));
    if (it != reg2Location_.end()) {
        return it->second;
    } else {
        LOG_FULL(FATAL) << "reg:" << std::dec << reg;
        UNREACHABLE();
    }
}

int CalleeReg::FindCallRegOrder(const DwarfReg reg) const
{
    auto order = FindCallRegOrder(static_cast<LLVMStackMapType::DwarfRegType>(reg));
    return order;
}

int CalleeReg::GetCallRegNum() const
{
    return reg2Location_.size();
}
} // namespace panda::ecmascript::kungfu
