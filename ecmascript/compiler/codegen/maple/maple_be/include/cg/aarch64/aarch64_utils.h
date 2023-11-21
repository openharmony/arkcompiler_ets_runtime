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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_UTILS_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_UTILS_H

#include "aarch64_cg.h"
#include "aarch64_operand.h"
#include "aarch64_cgfunc.h"

namespace maplebe {

/**
 * Get or create new memory operand for load instruction loadIns for which
 * machine opcode will be replaced with newLoadMop.
 *
 * @param loadIns load instruction
 * @param newLoadMop new opcode for load instruction
 * @return memory operand for new load machine opcode
 *         or nullptr if memory operand can't be obtained
 */
MemOperand *GetOrCreateMemOperandForNewMOP(CGFunc &cgFunc, const Insn &loadIns, MOperator newLoadMop);
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_UTILS_H
