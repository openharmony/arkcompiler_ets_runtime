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

#ifndef ECMASCRIPT_ARKSTEED_VREG_H
#define ECMASCRIPT_ARKSTEED_VREG_H

#include "ecmascript/compiler/bytecodes.h"

namespace panda::ecmascript::arksteed {
using VirtualRegister = kungfu::VirtualRegister;
using VRegIDType = kungfu::VRegIDType;

/**
 * Virtual register convention:
 *   - Locals: falls in index range [0, nL)
 *   - Params: falls in index range [nL, nL + nP)
 *       - CallTarget: index = nL,
 *       - NewTarget:  index = nL + 1
 *       - ThisObject: index = nL + 2
 *       - User-provided args: falls in index range [nL + 3, nL + nP)
 *   - Extra:
 *       - Env: index = nL + nP
 *       - Acc: index = nL + nP + 1
 * where nL = number of local virtual registers, nP = number of parameters.
 *
 * Unless specially specified, expressions like "number of virtual registers", "NumVRegs", etc.
 * refer to the **total** number, i.e. nL + nP + 2.
 */

enum FixedParamVRegIndex : VRegIDType {
    CALL_TARGET_PARAM_INDEX,
    NEW_TARGET_PARAM_INDEX,
    THIS_OBJECT_PARAM_INDEX,
    FIXED_PARAM_VREG_COUNT,
};

enum ExtraVRegIndex : VRegIDType {
    ENV_EXTRA_INDEX,
    ACC_EXTRA_INDEX,
    EXTRA_VREG_COUNT,
};

inline VirtualRegister VRegOfLocal(VRegIDType localIndex)
{
    return VirtualRegister{localIndex};
}
inline VirtualRegister VRegOfParam(VRegIDType numLocal, VRegIDType paramIndex)
{
    return VirtualRegister{numLocal + paramIndex};
}
inline VirtualRegister VRegOfEnv(VRegIDType numLocal, VRegIDType numParams)
{
    return VirtualRegister{numLocal + numParams + ENV_EXTRA_INDEX};
}
inline VirtualRegister VRegOfAcc(VRegIDType numLocal, VRegIDType numParams)
{
    return VirtualRegister{numLocal + numParams + ACC_EXTRA_INDEX};
}
constexpr VRegIDType NumVRegs(VRegIDType numLocal, VRegIDType numParams)
{
    return numLocal + numParams + EXTRA_VREG_COUNT;
}

static std::string VRegDisplayStringImpl(VirtualRegister vreg, VRegIDType numLocal, VRegIDType numParams)
{
    if (vreg.GetId() < numLocal) {
        return "v" + std::to_string(vreg.GetId());
    }
    if (vreg.GetId() < numLocal + numParams) {
        return "a" + std::to_string(vreg.GetId() - numLocal);
    }
    if (vreg == VRegOfEnv(numLocal, numParams)) {
        return "env";
    }
    if (vreg == VRegOfAcc(numLocal, numParams)) {
        return "acc";
    }
    return "<invalid>";
}

inline std::string VRegDisplayString(VirtualRegister vreg, VRegIDType numLocal, VRegIDType numParams)
{
    return VRegDisplayStringImpl(vreg, numLocal, numParams);
}
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_VREG_H
