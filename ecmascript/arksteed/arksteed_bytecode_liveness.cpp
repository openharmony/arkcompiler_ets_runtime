/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_bytecode_liveness.h"

namespace panda::ecmascript::arksteed {
std::string LivenessBitSet::Dump() const
{
    std::ostringstream out;
    out << '[';

    bool first = true;
    for (VRegIDType i = 0; i < NumLocalVRegs(); i++) {
        if (!TestLocal(i)) {
            continue;
        }
        first ? (void)(first = false) : (void)(out << ", ");
        out << 'v' << i;
    }

    bool allParamsAreLive = true;
    for (VRegIDType i = 0; allParamsAreLive && i < NumParamVRegs(); i++) {
        if (!TestParam(i)) {
            allParamsAreLive = false;
        }
    }
    if (allParamsAreLive && NumParamVRegs() >= 2) {  // 2: at least two parameters to use range notation
        first ? (void)(first = false) : (void)(out << ", ");
        out << "a0-a" << (NumParamVRegs() - 1);
    } else {
        for (VRegIDType i = 0; i < NumParamVRegs(); i++) {
            if (!TestParam(i)) {
                continue;
            }
            first ? (void)(first = false) : (void)(out << ", ");
            out << 'a' << i;
        }
    }

    if (Test(VRegOfEnv(NumLocalVRegs(), NumParamVRegs()).GetId())) {
        first ? (void)(first = false) : (void)(out << ", ");
        out << "env";
    }
    if (TestAcc()) {
        first ? (void)(first = false) : (void)(out << ", ");
        out << "acc";
    }

    out << ']';
    return out.str();
}
}  // namespace panda::ecmascript::arksteed
