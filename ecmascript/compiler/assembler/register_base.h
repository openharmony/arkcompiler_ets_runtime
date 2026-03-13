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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_REGISTER_BASE_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_REGISTER_BASE_H

#include <cstdint>

namespace panda::ecmascript {

// Base type for CPU Registers using CRTP pattern
template <typename SubType, int afterLastRegister>
class RegisterBase {
public:
    static constexpr int8_t CODE_INVALID_REG = -1;
    static constexpr int8_t NUM_REGISTERS = afterLastRegister;

    static constexpr SubType InvalidReg()
    {
        return SubType{CODE_INVALID_REG};
    }

    static constexpr SubType FromCode(int8_t code)
    {
        return SubType{code};
    }

    constexpr bool IsValid() const
    {
        return reg_code_ != CODE_INVALID_REG;
    }

    constexpr int8_t Code() const
    {
        return reg_code_;
    }

    inline constexpr bool operator==(const RegisterBase<SubType, afterLastRegister>& other) const
    {
        return reg_code_ == other.reg_code_;
    }

    inline constexpr bool operator!=(const RegisterBase<SubType, afterLastRegister>& other) const
    {
        return reg_code_ != other.reg_code_;
    }

protected:
    explicit constexpr RegisterBase(int8_t code) : reg_code_(code) {}
    int8_t reg_code_;
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_REGISTER_BASE_H
