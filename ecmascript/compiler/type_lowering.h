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

#ifndef ECMASCRIPT_COMPILER_TYPE_LOWERING_H
#define ECMASCRIPT_COMPILER_TYPE_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class TypeLowering {
public:
    TypeLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg, TSManager *tsManager,
                 bool enableLog)
        : bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY))), tsManager_(tsManager),
          enableLog_(enableLog) {}
    ~TypeLowering() = default;

    void RunTypeLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    void Lower(GateRef gate);

    void ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow = false);
    void ReplaceHirToSubCfg(GateRef hir, GateRef outir,
                            const std::vector<GateRef> &successControl);

    GateRef LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel = false);

    void LowerTypeNewObjDynRange(GateRef gate, GateRef glue);
    void LowerTypeAdd2Dyn(GateRef gate, GateRef glue);

    template<OpCode::Op Op>
    GateRef FastAddAndSub(GateRef left, GateRef right);
    template<OpCode::Op Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    GateRef DoubleBuildTaggedWithNoGC(GateRef gate);
    GateRef ChangeInt32ToFloat64(GateRef gate);

    BytecodeCircuitBuilder *bcBuilder_;
    Circuit *circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_LOWERING_H
