/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_INSTRUCTION_COMBINE_H
#define ECMASCRIPT_COMPILER_INSTRUCTION_COMBINE_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class InstructionCombine : public PassVisitor {
public:
    InstructionCombine(Circuit *circuit, RPOVisitor *visitor, Chunk *chunk, bool enableLog = false)
        : PassVisitor(circuit, chunk, visitor), builder_(circuit), enableLog_(enableLog)
    {
    }

    ~InstructionCombine() = default;

    GateRef VisitGate(GateRef gate) override;

private:
    GateRef VisitADD(GateRef gate);
    GateRef VisitSUB(GateRef gate);
    GateRef VisitMUL(GateRef gate);
    GateRef VisitSDIV(GateRef gate);
    GateRef VisitFDIV(GateRef gate);
    GateRef VisitSMOD(GateRef gate);
    GateRef VisitAND(GateRef gate);
    GateRef VisitOR(GateRef gate);
    GateRef VisitXOR(GateRef gate);
    GateRef VisitLSR(GateRef gate);
    GateRef VisitASR(GateRef gate);
    GateRef VisitLSL(GateRef gate);
    GateRef VisitBranch(GateRef gate);
    GateRef VisitICMP(GateRef gate);
    GateRef VisitREV(GateRef gate);
    GateRef VisitConvert(GateRef gate);
    GateRef VisitExtractValue(GateRef gate);
    GateRef ReduceWord32And(GateRef gate);
    GateRef ReduceWord64And(GateRef gate);
    GateRef ReduceWord32Or(GateRef gate);
    GateRef ReduceWord64Or(GateRef gate);
    GateRef ReduceWord32Xor(GateRef gate);
    GateRef ReduceWord64Xor(GateRef gate);
    GateRef ReduceWord32Lsr(GateRef gate);
    GateRef ReduceWord64Lsr(GateRef gate);
    GateRef ReduceWord32Asr(GateRef gate);
    GateRef ReduceWord64Asr(GateRef gate);
    GateRef ReduceWord32Lsl(GateRef gate);
    GateRef ReduceWord64Lsl(GateRef gate);
    GateRef ReduceInt32Add(GateRef gate);
    GateRef ReduceInt64Add(GateRef gate);
    GateRef ReduceInt32Sub(GateRef gate);
    GateRef ReduceInt64Sub(GateRef gate);
    GateRef ReduceInt32Mul(GateRef gate);
    GateRef ReduceInt64Mul(GateRef gate);
    GateRef ReduceInt32Div(GateRef gate);
    GateRef ReduceInt64Div(GateRef gate);
    GateRef ReduceDoubleAdd(GateRef gate);
    GateRef ReduceDoubleSub(GateRef gate);
    GateRef ReduceDoubleMul(GateRef gate);
    GateRef ReduceDoubleDiv(GateRef gate);
    GateRef ReduceInt32Mod(GateRef gate);
    GateRef ReduceDoubleMod(GateRef gate);
    
    GateRef ReplaceOld(GateRef gate, GateRef newGate);
    CircuitBuilder builder_;
    bool enableLog_ = false;
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_VALUE_NUMBERING_H