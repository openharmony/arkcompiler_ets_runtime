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

#ifndef ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H
#define ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/constant.h"
#include <cstdint>
#include <unordered_map>

namespace panda::ecmascript::kungfu {
class ConstantFolding;
using ConstantFoldingHandleType = GateRef(ConstantFolding::*)(GateRef gate);

class ConstantFolding : public PassVisitor {
public:
    ConstantFolding(Circuit *circuit, RPOVisitor* visitor, CompilationConfig *cmpCfg, bool enableLog,
                    const std::string& name, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          enableLog_(enableLog), methodName_(name) {}
    ~ConstantFolding() = default;
    void Print() const;
    void Initialize() override;
    GateRef VisitGate(GateRef gate) override;
private:
    void InitializeOpHandlers();
    // Binary op
    GateRef VisitADD(GateRef gate);
    GateRef VisitSUB(GateRef gate);
    GateRef VisitMUL(GateRef gate);
    GateRef VisitSDIV(GateRef gate);
    GateRef VisitSMOD(GateRef gate);
    GateRef VisitUMOD(GateRef gate);
    GateRef VisitFDIV(GateRef gate);
    GateRef VisitAND(GateRef gate);
    GateRef VisitXOR(GateRef gate);
    GateRef VisitOR(GateRef gate);
    GateRef VisitMIN(GateRef gate);
    GateRef VisitMAX(GateRef gate);
    GateRef VisitICMP(GateRef gate);

    // Unary op
    GateRef VisitZEXT(GateRef gate);
    GateRef VisitREV(GateRef gate);
    GateRef VisitABS(GateRef gate);
    GateRef VisitSQRT(GateRef gate);
    GateRef VisitCEIL(GateRef gate);
    GateRef VisitFLOOR(GateRef gate);

    GateRef BinaryCalculate(GateRef gate, std::function<Constant(Constant, Constant)> op);
    GateRef UnaryCalculate(GateRef gate, std::function<Constant(Constant)> op);

    inline GateRef BoolConstant(Constant val)
    {
        AddFoldingCount();
        GateRef result = builder_.Boolean(val.GetInt());
        return result;
    }

    inline GateRef Int32Constant(Constant val)
    {
        AddFoldingCount();
        GateRef result = builder_.Int32(val.GetInt());
        return result;
    }

    inline GateRef Int64Constant(Constant val)
    {
        AddFoldingCount();
        GateRef result = builder_.Int64(val.GetInt());
        return result;
    }

    inline GateRef DoubleConstant(Constant val)
    {
        AddFoldingCount();
        GateRef result = builder_.Double(val.GetDouble());
        return result;
    }

    inline GateRef BuildConstant(GateRef gate, Constant val)
    {
        if (IsBoolType(gate)) {
            return BoolConstant(val);
        } else if (IsInt32Type(gate)) {
            return Int32Constant(val);
        } else if (IsInt64Type(gate)) {
            return Int64Constant(val);
        } else if (IsF64Type(gate)) {
            return DoubleConstant(val);
        }
        return Circuit::NullGate();
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
    std::unordered_map<OpCode, ConstantFoldingHandleType> opHandlers_;
};

}   // namespace panda::ecmascript::kungfu
#endif  //ECMASCRIPT_COMPILER_CONSTANT_FOLDING_H
