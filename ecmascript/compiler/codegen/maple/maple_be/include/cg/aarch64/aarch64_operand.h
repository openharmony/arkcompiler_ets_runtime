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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H

#include <limits>
#include <string>
#include <iomanip>
#include "aarch64_isa.h"
#include "operand.h"
#include "cg.h"
#include "emit.h"
#include "common_utils.h"

namespace std {
template <> /* function-template-specialization */
class std::hash<maplebe::MemOperand> {
public:
    size_t operator()(const maplebe::MemOperand &x) const
    {
        std::size_t seed = 0;
        hash_combine<uint8_t>(seed, x.GetAddrMode());
        hash_combine<uint32_t>(seed, x.GetSize());
        maplebe::RegOperand *xb = x.GetBaseRegister();
        maplebe::RegOperand *xi = x.GetIndexRegister();
        if (xb != nullptr) {
            hash_combine<uint32_t>(seed, xb->GetRegisterNumber());
            hash_combine<uint32_t>(seed, xb->GetSize());
        }
        if (xi != nullptr) {
            hash_combine<uint32_t>(seed, xi->GetRegisterNumber());
            hash_combine<uint32_t>(seed, xi->GetSize());
        }
        return seed;
    }
};
}  // namespace std
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H */
