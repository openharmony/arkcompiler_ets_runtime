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

#include "x64_live.h"
#include "x64_cg.h"

namespace maplebe {
static const std::set<regno_t> intParamRegSet = {RDI, RSI, RDX, RCX, R8, R9};

bool X64LiveAnalysis::CleanupBBIgnoreReg(regno_t reg)
{
    if (intParamRegSet.find(reg) != intParamRegSet.end()) {
        return true;
    }
    return false;
}
} /* namespace maplebe */
