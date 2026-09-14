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

#ifndef ECMASCRIPT_COMPILER_CONSTANT_H
#define ECMASCRIPT_COMPILER_CONSTANT_H

#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "macros.h"
#include <cstdint>
namespace panda::ecmascript::kungfu {

class Constant {
public:
    Constant() : type(Invalid) {};
    Constant(bool boolValue) : type(ConstantType::I1), boolValue(boolValue) {}
    Constant(int int32Value)
        : type(ConstantType::I32), int32Value(int32Value) {}
    Constant(int64_t int64Value)
        : type(ConstantType::I64), int64Value(int64Value) {}
    Constant(double doubleValue)
        : type(ConstantType::F64), doubleValue(doubleValue) {}

    bool IsBool() const
    {
        return type == ConstantType::I1;
    }

    bool IsInt32() const
    {
        return type == ConstantType::I32;
    }

    bool IsInt64() const
    {
        return type == ConstantType::I64;
    }

    bool IsInt() const
    {
        return type == ConstantType::I1 || type == ConstantType::I32 ||
            type == ConstantType::I64;
    }

    bool IsDouble() const
    {
        return type == ConstantType::F64;
    }

    bool IsValid() const
    {
        return type != Invalid;
    }

    bool GetBool() const
    {
        ASSERT(IsBool());
        return boolValue;
    }

    int GetInt32() const
    {
        ASSERT(IsInt32());
        return int32Value;
    }

    int64_t GetInt64() const
    {
        ASSERT(IsInt64());
        return int64Value;
    }

    int64_t GetInt() const
    {
        ASSERT(IsInt());
        if (type == ConstantType::I64) {
            return int64Value;
        } else if (type == ConstantType::I1) {
            return boolValue;
        }
        return int32Value;
    }

    uint64_t GetUint() const
    {
        if (type == ConstantType::I64) {
            return int64Value;
        } else if (type == ConstantType::I1) {
            return boolValue;
        }
        return static_cast<uint64_t>(int32Value);
    }

    double GetDouble() const
    {
        if (type == ConstantType::F64) {
            return doubleValue;
        }
        return GetInt();
    }

    Constant operator + (const Constant &rhs) const
    {
        if (IsInt() && rhs.IsInt()) {
            auto lval = GetInt();
            auto rval = rhs.GetInt();
            if (lval > INT32_MAX - rval) {
                return Constant(lval + rval);
            } else {
                return Constant(static_cast<int>(lval + rval));
            }
        }
        return Constant(GetDouble() + rhs.GetDouble());
    }

    Constant operator - (const Constant &rhs) const
    {
        if (IsInt() && rhs.IsInt()) {
            auto lval = GetInt();
            auto rval = rhs.GetInt();
            if (lval < INT32_MIN + rval) {
                return Constant(lval - rval);
            } else {
                return Constant(static_cast<int>(lval - rval));
            }
        }
        return Constant(GetDouble() - rhs.GetDouble());
    }

    Constant operator * (const Constant &rhs) const
    {
        if (IsInt()) {
            auto lval = GetInt();
            auto rval = rhs.GetInt();
            if (lval > 0 && rval > 0 && lval > INT32_MAX / rval) {
                return Constant(lval * rval);
            } else if (lval < 0 && rval < 0 && lval < INT32_MAX / rval) {
                return Constant(lval * rval);
            } else if (lval > 0 && rval < 0 && lval < INT32_MIN / rval) {
                return Constant(lval * rval);
            } else if (lval < 0 && rval > 0 && lval > INT32_MIN / rval) {
                return Constant(lval * rval);
            }
            return Constant(static_cast<int>(lval * rval));
        }
        return Constant(GetDouble() * rhs.GetDouble());
    }

    Constant CheckDiv(const Constant &rhs) const
    {
        if (IsDouble() && rhs.IsDouble() && std::fabs(rhs.GetDouble()) > 1e-6) {
            return Constant(GetDouble() / rhs.GetDouble());
        } else if (IsInt() && rhs.IsInt() && rhs.GetInt() != 0) {
            return Constant(GetInt() / rhs.GetInt());
        }
        return Constant();
    }

    Constant operator | (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() | rhs.GetInt());
        } else {
            return Constant();
        }
    }

    Constant operator & (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() & rhs.GetInt());
        } else {
            return Constant();
        }
    }

    Constant operator ^ (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() ^ rhs.GetInt());
        } else {
            return Constant();
        }
    }

    Constant CheckMod(const Constant &rhs) const
    {
        if (IsInt() && rhs.GetInt() != 0) {
            return Constant(GetInt() % rhs.GetInt());
        } else {
            return Constant();
        }
    }

    Constant operator ~ () const
    {
        if (IsBool()) {
            return Constant(!GetBool());
        } else if (IsInt32()) {
            return Constant(~GetInt32());
        } else {
            return Constant();
        }
    }

    Constant operator == (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() == rhs.GetInt());
        } else {
            return Constant(GetDouble() == rhs.GetDouble());
        }
    }

    Constant operator != (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() != rhs.GetInt());
        } else {
            return Constant(GetDouble() != rhs.GetDouble());
        }
    }

    Constant operator < (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() < rhs.GetInt());
        } else {
            return Constant(GetDouble() < rhs.GetDouble());
        }
    }

    Constant operator <= (const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetInt() <= rhs.GetInt());
        } else {
            return Constant(GetDouble() <= rhs.GetDouble());
        }
    }

    Constant ULE(const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetUint() <= rhs.GetUint());
        } else {
            return Constant(GetDouble() <= rhs.GetDouble());
        }
    }

    Constant ULT(const Constant &rhs) const
    {
        if (IsInt()) {
            return Constant(GetUint() < rhs.GetUint());
        } else {
            return Constant(GetDouble() < rhs.GetDouble());
        }
    }

private:
    enum ConstantType {
        I1,
        I32,
        I64,
        F64,
        Invalid,
    };
    ConstantType type;
    union {
        bool boolValue;
        int int32Value;
        int64_t int64Value;
        double doubleValue;
    };
};

Constant GetConstant(GateAccessor acc, GateRef gate);

} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H
