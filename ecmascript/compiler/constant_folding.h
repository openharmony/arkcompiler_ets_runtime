/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H
#define ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/number_gate_info.h"

namespace panda::ecmascript::kungfu {
class ConstantFolding : public PassVisitor {

#define BINARY_OP_LIST(V)                       \
    V(ADD,          VALID)                      \
    V(SUB,          VALID)                      \
    V(MUL,          VALID)                      \
    V(EXP,          INVALID)                    \
    V(SDIV,         INVALID)                    \
    V(SMOD,         VALID)                      \
    V(UDIV,         INVALID)                    \
    V(UMOD,         VALID)                      \
    V(FDIV,         INVALID)                    \
    V(FMOD,         INVALID)                    \
    V(AND,          VALID)                      \
    V(XOR,          VALID)                      \
    V(OR,           VALID)                      \
    V(LSL,          INVALID)                    \
    V(LSR,          INVALID)                    \
    V(ASR,          INVALID)                    \
    V(MIN,          VALID)                      \
    V(MAX,          VALID)

#define UNARY_OP_LIST(V)                        \
    V(ZEXT,                    VALID)           \
    V(SEXT,                    INVALID)         \
    V(DOUBLE_TRUNC,            INVALID)         \
    V(TRUNC,                   INVALID)         \
    V(FEXT,                    INVALID)         \
    V(FTRUNC,                  INVALID)         \
    V(REV,                     VALID)           \
    V(TAGGED_TO_INT64,         INVALID)         \
    V(INT64_TO_TAGGED,         INVALID)         \
    V(SIGNED_INT_TO_FLOAT,     INVALID)         \
    V(UNSIGNED_INT_TO_FLOAT,   INVALID)         \
    V(FLOAT_TO_SIGNED_INT,     INVALID)         \
    V(UNSIGNED_FLOAT_TO_INT,   INVALID)         \
    V(TRUNC_FLOAT_TO_INT64,    INVALID)         \
    V(TRUNC_FLOAT_TO_INT32,    INVALID)         \
    V(BITCAST,                 INVALID)         \
    V(ABS,                     VALID)           \
    V(SQRT,                    VALID)           \
    V(CLZ32,                   INVALID)         \
    V(CEIL,                    VALID)           \
    V(FLOOR,                   VALID)

#define GEN_SWITCH_CASE(op, flag) GEN_SWITCH_CASE_##flag(op)
#define GEN_SWITCH_CASE_VALID(op)                                     \
    case OpCode::op: {                                                \
        return Visit##op(gate);                                       \
    }
#define GEN_SWITCH_CASE_INVALID(op)

  public:
    ConstantFolding(Circuit *circuit, RPOVisitor* visitor, CompilationConfig *cmpCfg, bool enableLog,
                    const std::string& name, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          enableLog_(enableLog), methodName_(name) {}
    ~ConstantFolding() = default;
    void Print() const;
    GateRef VisitGate(GateRef gate);
private:

#define DECLARE_VISIT_OP(name, valid)           \
    GateRef Visit##name(GateRef gate);

    BINARY_OP_LIST(DECLARE_VISIT_OP)                                      
    UNARY_OP_LIST(DECLARE_VISIT_OP)
#undef DECLARE_VISIT_BINARY_OP

    GateRef UnaryInt32Calculate(GateRef gate, std::function<GateRef(int)> op);
    GateRef UnaryF64Calculate(GateRef gate, std::function<GateRef(double)> op);
    GateRef BinaryInt32Calculate(GateRef gate, std::function<GateRef(int, int)> op);
    GateRef BinaryF64Calculate(GateRef gate, std::function<GateRef(double, double)> op);

    inline GateRef BoolConstant(bool val)
    {
        AddFoldingCount();
        GateRef result = builder_.Boolean(val);
        return result;
    }

    inline GateRef Int32Constant(int32_t val)
    {
        AddFoldingCount();
        GateRef result = builder_.Int32(val);
        return result;
    }

    inline GateRef Int64Constant(size_t val)
    {
        AddFoldingCount();
        GateRef result = builder_.Int64(val);
        return result;
    }

    inline GateRef DoubleConstant(double val)
    {
        AddFoldingCount();
        GateRef result = builder_.Double(val);
        return result;
    }

    inline bool IsBoolType(GateRef gate) const
    {
        return acc_.GetMachineType(gate) == MachineType::I1;
    }

    inline bool IsInt32Type(GateRef gate) const
    {
        return acc_.GetMachineType(gate) == MachineType::I32;
    }

    inline bool IsInt64Type(GateRef gate) const
    {
        return acc_.GetMachineType(gate) == MachineType::I64;
    }

    inline bool IsF64Type(GateRef gate) const
    {
        return acc_.GetMachineType(gate) == MachineType::F64;
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    std::string GetMethodName() const
    {
        return methodName_;
    }

    void AddFoldingCount()
    {
        ++foldingCount_;
    }

    int32_t GetFoldingCount() const
    {
        return foldingCount_;
    }

    Circuit* circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    bool enableLog_{false};
    std::string methodName_;
    int32_t foldingCount_{0};
};

}   // namespace panda::ecmascript::kungfu
#endif  //ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H