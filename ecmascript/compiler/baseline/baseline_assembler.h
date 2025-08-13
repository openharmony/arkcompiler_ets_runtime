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

#ifndef ECMASCRIPT_BASELINE_BASELINE_ASSEMBLER_H
#define ECMASCRIPT_BASELINE_BASELINE_ASSEMBLER_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/compiler/assembler/macro_assembler.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::kungfu {
enum class SpecialRegister : uint8_t {
    NEW_TARGET,
    CALL_SIZE, // jumpSizeAfterCall
    ENV,
    ACC_REGISTER,
    THIS_OBJECT, // this
    FUNC, // callTarget
};

using BaselineParameter = std::variant<int8_t, int16_t, int32_t, int64_t, BaselineSpecialParameter, VirtualRegister>;

class StackOffsetDescriptor {
public:
    StackOffsetDescriptor(uint64_t inputCallField) : callField(inputCallField)
    {
        numVregs = Method::GetNumVregsWithCallField(callField);
        haveNewTarget = Method::HaveNewTargetWithCallField(callField);
        // Add stack slot info as required
    }
    ~StackOffsetDescriptor() = default;

    int32_t GetVregOffset(VirtualRegister virtualReg)
    {
        VRegIDType vregId = virtualReg.GetId();
        return (vregId) * FRAME_SLOT_SIZE;
    }

    int32_t GetSpecialRegisterOffset(SpecialRegister reg)
    {
        switch (reg) {
            case SpecialRegister::NEW_TARGET:
                return ((3 + numVregs) * FRAME_SLOT_SIZE);  // +3: contains numVregs, undefined, callTarget, newTarget
            case SpecialRegister::CALL_SIZE:
                return -(5 * FRAME_SLOT_SIZE); // -5: contains frametype, rbp, pc, sp, callsize(jumpSizeAfterCall)
            case SpecialRegister::ENV:
                return -(6 * FRAME_SLOT_SIZE); // -6: contains frametype, rbp, pc, sp, callsize, env,
            case SpecialRegister::ACC_REGISTER:
                return -(7 * FRAME_SLOT_SIZE); // -7: contains frametype, rbp, pc, sp, callsize, env, acc,
            case SpecialRegister::THIS_OBJECT:
                return -(8 * FRAME_SLOT_SIZE); // -8: contains frametype, rbp, pc, sp, callsize, env, acc, thisObj
            case SpecialRegister::FUNC:
                // -9: contains frametype, rbp, pc, sp, callsize, env, acc, thisObj, call-target
                return -(9 * FRAME_SLOT_SIZE);
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                return 0;
        }
    }
private:
    uint64_t callField = 0;
    uint32_t numVregs = 0;
    bool haveNewTarget = false;
    static constexpr int32_t FRAME_SLOT_SIZE = 8;
};

class BaselineAssembler {
public:
    explicit BaselineAssembler(const std::string &tripleStr);
    uint8_t *GetBuffer() const
    {
        return macroAssembler->GetBegin();
    }

    size_t GetBufferSize() const
    {
        return macroAssembler->GetBufferCurrentSize();
    }

    void SetStackOffsetDescriptor(const StackOffsetDescriptor &inputStackOffsetDesc)
    {
        stackOffsetDescriptor = inputStackOffsetDesc;
    }

    MacroAssembler &GetMacroAssembler()
    {
        ASSERT(macroAssembler != nullptr);
        return *macroAssembler;
    }

    virtual ~BaselineAssembler()
    {
        if (macroAssembler != nullptr) {
            delete macroAssembler;
            macroAssembler = nullptr;
        }
    }
    void Move(VirtualRegister interpreterDestReg, Immediate value);
    void Move(SpecialRegister destReg, Immediate value);
    void Move(SpecialRegister destReg, SpecialRegister srcReg);
    void Move(VirtualRegister interpreterDestReg, VirtualRegister interpreterSrcReg);
    void Move(SpecialRegister destReg, VirtualRegister interpreterSrcReg);
    void Move(VirtualRegister interpreterDestReg, SpecialRegister srcReg);
    void Cmp(SpecialRegister reg, Immediate value);

    void Bind(JumpLabel &label)
    {
        macroAssembler->Bind(label);
    }

    void Jz(JumpLabel &label)
    {
        macroAssembler->Jz(label);
    }

    void Jnz(JumpLabel &label)
    {
        macroAssembler->Jnz(label);
    }

    void Jump(JumpLabel &label)
    {
        macroAssembler->Jump(label);
    }

    void SaveResultIntoAcc();
    void CallBuiltin(Address funcAddress,
                     const std::vector<BaselineParameter> &parameters);

private:
    MacroAssembler *macroAssembler;
    StackOffsetDescriptor stackOffsetDescriptor;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_BASELINE_BASELINE_ASSEMBLER_H
