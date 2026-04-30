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

#include "ecmascript/arksteed/arksteed_regalloc_types.h"

namespace panda::ecmascript::arksteed {
const char *ToString(MachineRepresentation r)
{
    switch (r) {
        case MachineRepresentation::None:
            return "None";
        case MachineRepresentation::Bit:
            return "Bit";
        case MachineRepresentation::Word8:
            return "Word8";
        case MachineRepresentation::Word16:
            return "Word16";
        case MachineRepresentation::Word32:
            return "Word32";
        case MachineRepresentation::Word64:
            return "Word64";
        case MachineRepresentation::Tagged:
            return "Tagged";
        case MachineRepresentation::Float64:
            return "Float64";
        default:
            return "<INVALID>";
    }
}

const char *ToString(InstructionOperandKind kind)
{
    switch (kind) {
        case InstructionOperandKind::INVALID:
            return "INVALID";
        case InstructionOperandKind::CONSTANT:
            return "CONSTANT";
        case InstructionOperandKind::UNALLOCATED:
            return "UNALLOCATED";
        case InstructionOperandKind::ALLOCATED:
            return "ALLOCATED";
        default:
            return "<INVALID>";
    }
}

std::string InstructionOperand::Description() const
{
    switch (GetKind()) {
        case InstructionOperandKind::CONSTANT:
            return ConstantOperand::Cast(this)->Description();
        case InstructionOperandKind::UNALLOCATED:
            return UnallocatedState::Cast(this)->Description();
        case InstructionOperandKind::ALLOCATED:
            return AllocatedState::Cast(this)->Description();
        default:
            return "<INVALID>";
    }
}

std::string UnallocatedState::Description() const
{
    std::string res = "Unallocated(v" + std::to_string(GetVirtualRegister()) + ")::";
    if (IsFixedSlotPolicy()) {
        res += "FixedSlot(" + std::to_string(GetFixedSlotIndex()) + ')';
        return res;
    }
#define USED_AT_STR                                   \
    std::string                                       \
    {                                                 \
        IsUsedAtStart() ? "UsedAtStart" : "UsedAtEnd" \
    }
    switch (GetExtendedPolicy()) {
        case ExtendedPolicy::FIXED_REGISTER:
            res += "FixedRegister(" + std::to_string(GetFixedRegisterIndex()) + ')';
            return res;
        case ExtendedPolicy::FIXED_FP_REGISTER:
            res += "FixedFPRegister(" + std::to_string(GetFixedRegisterIndex()) + ')';
            return res;
        case ExtendedPolicy::MUST_HAVE_REGISTER:
            res += "MustHaveRegister(" + USED_AT_STR + ')';
            return res;
        case ExtendedPolicy::MUST_HAVE_SLOT:
            res += "MustHaveSlot(" + USED_AT_STR + ')';
            return res;
        case ExtendedPolicy::REGISTER_OR_SLOT:
            res += "RegisterOrSlot(" + USED_AT_STR + ')';
            return res;
        case ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT:
            res += "RegisterOrSlotOrConstant(" + USED_AT_STR + ')';
            return res;
        case ExtendedPolicy::SAME_AS_INPUT:
            res += "SameAsInput(" + std::to_string(GetInputIndex()) + ')';
            return res;
        case ExtendedPolicy::NONE:
            return res + "None";
        default:
            return res + "<INVALID>";
    }
#undef USED_AT_STR
}

std::string ConstantOperand::Description() const
{
    return "Constant(v" + std::to_string(GetVirtualRegister()) + ')';
}

std::string AllocatedState::Description() const
{
    std::string res = "Allocated::";
    if (GetLocationKind() == REGISTER) {
        res += "Register(";
    } else {
        res += "StackSlot(";
    }
    res += ToString(GetRepresentation());
    if (IsRegister()) {
        res += "res r " + std::to_string(GetRegisterCode()) + ')';
    } else if (IsDoubleRegister()) {
        res += "res fr " + std::to_string(GetRegisterCode()) + ')';
    } else {
        res += "res " + std::to_string(GetIndex()) + ')';
    }
    return res;
}
}  // namespace panda::ecmascript::arksteed
