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

#ifndef ECMASCRIPT_ARKSTEED_REGISTER_MERGE_STATE_H
#define ECMASCRIPT_ARKSTEED_REGISTER_MERGE_STATE_H

#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_vertex.h"

namespace panda::ecmascript::arksteed {

// =============================================================================
// RegisterMergeInfo - Stores merge information for a register at a merge point
// =============================================================================
struct RegisterMergeInfo {
    // Get operands array pointer
    InstructionOperand *Operands()
    {
        return reinterpret_cast<InstructionOperand *>(this + 1);
    }

    InstructionOperand &Operand(size_t i)
    {
        return Operands()[i];
    }

    ValueVertex *vertex;
};

// =============================================================================
// RegisterState - Simple class with pointer and flags
// =============================================================================
class RegisterState {
public:
    RegisterState() : pointer_(0), isInitialized_(false), isMerge_(false) {}

    bool IsInitialized() const
    {
        return isInitialized_;
    }
    bool IsMerge() const
    {
        return isMerge_;
    }

    void SetValue(ValueVertex *value)
    {
        pointer_ = reinterpret_cast<uintptr_t>(value);
        isInitialized_ = true;
        isMerge_ = false;
    }

    void SetMerge(RegisterMergeInfo *mergeInfo)
    {
        pointer_ = reinterpret_cast<uintptr_t>(mergeInfo);
        isInitialized_ = true;
        isMerge_ = true;
    }

    bool LoadMergeState(ValueVertex **vertex, RegisterMergeInfo **mergeInfo)
    {
        ASSERT(isInitialized_);
        if (isMerge_) {
            *mergeInfo = reinterpret_cast<RegisterMergeInfo *>(pointer_);
            *vertex = (*mergeInfo)->vertex;
            return true;
        }
        *mergeInfo = nullptr;
        *vertex = reinterpret_cast<ValueVertex *>(pointer_);
        return false;
    }

private:
    uintptr_t pointer_;
    bool isInitialized_;
    bool isMerge_;
};

// =============================================================================
// RegisterMergeState - Stores register state at a merge point
// =============================================================================
class RegisterMergeState {
public:
    static constexpr int ALLOCATABLE_GENERAL_REGISTER_COUNT = GetAllocatableGeneralRegisters().Count();
    static constexpr int ALLOCATABLE_DOUBLE_REGISTER_COUNT = GetAllocatableDoubleRegisters().Count();

    bool IsInitialized() const
    {
        return values_[0].IsInitialized();
    }

    template <typename Function>
    void ForEachGeneralRegister(Function &&f)
    {
        RegisterState *currentValue = &values_[0];
        for (ArkSteedRegister reg : GetAllocatableGeneralRegisters()) {
            f(reg, *currentValue);
            ++currentValue;
        }
    }

    template <typename Function>
    void ForEachDoubleRegister(Function &&f)
    {
        RegisterState *currentValue = &doubleValues_[0];
        for (ArkSteedDoubleRegister reg : GetAllocatableDoubleRegisters()) {
            f(reg, *currentValue);
            ++currentValue;
        }
    }

private:
    RegisterState values_[ALLOCATABLE_GENERAL_REGISTER_COUNT];
    RegisterState doubleValues_[ALLOCATABLE_DOUBLE_REGISTER_COUNT];
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_REGISTER_MERGE_STATE_H
