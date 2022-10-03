/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#include "ecmascript/calleeReg.h"
#include "libpandabase/macros.h"
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
#endif
}

int CalleeReg::FindCallRegOrder(const DwarfRegType reg) const
{
    auto it = reg2Location_.find(static_cast<DwarfReg>(reg));
    if (it != reg2Location_.end()) {
        return it->second;
    } else {
        UNREACHABLE();
    }
}

int CalleeReg::FindCallRegOrder(const DwarfReg reg) const
{
    auto order = FindCallRegOrder(static_cast<DwarfRegType>(reg));
    return order;
}

int CalleeReg::GetCallRegNum() const
{
    return reg2Location_.size();
}
} // namespace panda::ecmascript::kungfu
