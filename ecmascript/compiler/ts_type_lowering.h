/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H
#define ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
class TSTypeLowering {
public:
    TSTypeLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg,
                   TSManager *tsManager, bool enableLog)
        : bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY))), tsManager_(tsManager),
          enableLog_(enableLog) {}
    ~TSTypeLowering() = default;

    void RunTSTypeLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }
    void Lower(GateRef gate);
    void RebuildSlowpathCfg(GateRef hir, std::map<GateRef, size_t> &stateGateMap);
    void GenerateSuccessMerge(std::vector<GateRef> &successControl);
    void ReplaceHirToFastPathCfg(GateRef hir, GateRef outir, const std::vector<GateRef> &successControl);
    void LowerTypeAdd2Dyn(GateRef gate);
    void LowerTypeSub2Dyn(GateRef gate);
    void LowerTypeMul2Dyn(GateRef gate);
    void LowerTypeLess2Dyn(GateRef gate);
    void LowerTypeLessEq2Dyn(GateRef gate);
    void SpeculateNumberAdd(GateRef gate);
    void SpeculateNumberSub(GateRef gate);
    void SpeculateNumberMul(GateRef gate);
    void SpeculateNumberLess(GateRef gate);
    void SpeculateNumberLessEq(GateRef gate);
    BytecodeCircuitBuilder *bcBuilder_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_ {Gate::InvalidGateRef};
    [[maybe_unused]]TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H
