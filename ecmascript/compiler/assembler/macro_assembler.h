/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_MACRO_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_MACRO_ASSEMBLER_H

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/native_area_allocator.h"

namespace panda::ecmascript::kungfu {

enum class BaselineSpecialParameter : uint8_t {
    GLUE,
    SP,
    ACC,
    PROFILE_TYPE_INFO,
    HOTNESS_COUNTER,
    ENV,
    PC,
};

class StackSlotOperand {
public:
    enum class BaseRegister {
        FRAME_REGISTER,
        STACK_REGISTER,
    };
    StackSlotOperand(BaseRegister baseReg, int32_t stackOffset)
        : baseRegister(baseReg), offset(stackOffset) {}

    ~StackSlotOperand() = default;

    BaseRegister GetBaseRegister() const
    {
        return baseRegister;
    }

    int32_t GetOffset() const
    {
        return offset;
    }

    bool IsFrameBase() const
    {
        return baseRegister == BaseRegister::FRAME_REGISTER;
    }
private:
    BaseRegister baseRegister;
    int32_t offset = 0;
};


using MacroParameter = std::variant<int8_t, int16_t, int32_t, int64_t, BaselineSpecialParameter, StackSlotOperand>;
using JumpLabel = panda::ecmascript::Label;

class MacroAssembler {
public:
    MacroAssembler() : allocator(NativeAreaAllocator()), chunk(&allocator) {}
    virtual ~MacroAssembler() = default;
    virtual uint8_t *GetBegin() const = 0;
    virtual RelocMap &GetRelocInfo() = 0;
    virtual size_t GetBufferCurrentSize() const = 0;
    virtual void Move(const StackSlotOperand &dstStackSlot, Immediate value) = 0;
    virtual void Move(const StackSlotOperand &dstStackSlot,
                      const StackSlotOperand &srcStackSlot) = 0;
    virtual void Cmp(const StackSlotOperand &stackSlot, Immediate value) = 0;
    virtual void Bind(JumpLabel &label) = 0;
    virtual void Jz(JumpLabel &label) = 0;
    virtual void Jnz(JumpLabel &label) = 0;
    virtual void Jump(JumpLabel &label) = 0;
    virtual void SaveReturnRegister(const StackSlotOperand &dstStackSlot) = 0;
    virtual void CallBuiltin(Address funcAddress,
                             const std::vector<MacroParameter> &parameters) = 0;

protected:
    NativeAreaAllocator allocator;
    Chunk chunk;
    static constexpr int32_t FUNCTION_OFFSET_FROM_SP = -72; // 72: includes 9 slots
};

}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MACRO_ASSEMBLER_H
