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
public:
    ConstantFolding(Circuit *circuit, RPOVisitor* visitor, CompilationConfig *cmpCfg, bool enableLog,
                    const std::string& name, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          enableLog_(enableLog), methodName_(name) {}
    ~ConstantFolding() = default;
    void Print() const;
    GateRef VisitGate(GateRef gate);
private:
    GateRef VisitADD(GateRef gate);
    GateRef VisitMUL(GateRef gate);
    GateRef VisitZEXT(GateRef gate);
    GateRef VisitSUB(GateRef gate);
    GateRef VisitSMOD(GateRef gate);
    GateRef VisitUMOD(GateRef gate);
    GateRef Int32Constant(int32_t val);
    GateRef Int64Constant(size_t val);
    RangeInfo GetRange(GateRef gate) const;
    bool IsInt32Type(GateRef gate) const;

    bool IsConstant(GateRef gate) const
    {
        return acc_.GetOpCode(gate) == OpCode::CONSTANT;
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