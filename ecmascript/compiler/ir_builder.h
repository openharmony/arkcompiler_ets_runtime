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

#ifndef ECMASCRIPT_COMPILER_IR_BUILDER_H
#define ECMASCRIPT_COMPILER_IR_BUILDER_H

#include "ecmascript/compiler/lcr_gate_meta_data.h"

namespace panda::ecmascript::kungfu {
using OperandsVector = std::set<int>;
enum class MachineRep {
    K_NONE,
    K_BIT,
    K_WORD8,
    K_WORD16,
    K_WORD32,
    K_WORD64,
    // FP representations must be last, and in order of increasing size.
    K_FLOAT32,
    K_FLOAT64,
    K_SIMD128,
    K_PTR_1, // Tagged Pointer
    K_META,
};

enum class CallInputs : size_t {
    DEPEND = 0,
    TARGET,
    GLUE,
    FIRST_PARAMETER
};

enum class CallInfoKind : bool {
    HAS_FRAME_STATE = true,
    NO_FRAME_STATE = false
};

#define OPCODES(V)                                                                        \
    V(Call, (GateRef gate, const std::vector<GateRef> &inList, OpCode op))                \
    V(RuntimeCall, (GateRef gate, const std::vector<GateRef> &inList))                    \
    V(RuntimeCallWithArgv, (GateRef gate, const std::vector<GateRef> &inList))            \
    V(ASMCallBarrier, (GateRef gate, const std::vector<GateRef> &inList))                 \
    V(NoGcRuntimeCall, (GateRef gate, const std::vector<GateRef> &inList))                \
    V(BytecodeCall, (GateRef gate, const std::vector<GateRef> &inList))                   \
    V(Alloca, (GateRef gate))                                                             \
    V(Block, (int id, const OperandsVector &predecessors))                                \
    V(Goto, (int block, int bbout))                                                       \
    V(Parameter, (GateRef gate))                                                          \
    V(Constant, (GateRef gate, std::bitset<64> value))                                    \
    V(ConstString, (GateRef gate, const ChunkVector<char> &str))                          \
    V(RelocatableData, (GateRef gate, uint64_t value))                                    \
    V(ZExtInt, (GateRef gate, GateRef e1))                                                \
    V(SExtInt, (GateRef gate, GateRef e1))                                                \
    V(FPExt, (GateRef gate, GateRef e1))                                                  \
    V(FPTrunc, (GateRef gate, GateRef e1))                                                \
    V(Load, (GateRef gate, GateRef base))                                                 \
    V(Store, (GateRef gate, GateRef base, GateRef value))                                 \
    V(IntRev, (GateRef gate, GateRef e1))                                                 \
    V(Add, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Sub, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Mul, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(FloatDiv, (GateRef gate, GateRef e1, GateRef e2))                                   \
    V(IntDiv, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(UDiv, (GateRef gate, GateRef e1, GateRef e2))                                       \
    V(IntOr, (GateRef gate, GateRef e1, GateRef e2))                                      \
    V(IntAnd, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(IntXor, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(IntLsr, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(IntAsr, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(Int32LessThanOrEqual, (GateRef gate, GateRef e1, GateRef e2))                       \
    V(Cmp, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Branch, (GateRef gate, GateRef cmp, GateRef btrue, GateRef bfalse))                 \
    V(Switch, (GateRef gate, GateRef input, const std::vector<GateRef> &outList))         \
    V(SwitchCase, (GateRef gate, GateRef switchBranch, GateRef out))                      \
    V(Phi, (GateRef gate, const std::vector<GateRef> &srcGates))                          \
    V(Return, (GateRef gate, GateRef popCount, const std::vector<GateRef> &operands))     \
    V(ReturnVoid, (GateRef gate))                                                         \
    V(CastIntXToIntY, (GateRef gate, GateRef e1))                                         \
    V(ChangeInt32ToDouble, (GateRef gate, GateRef e1))                                    \
    V(ChangeUInt32ToDouble, (GateRef gate, GateRef e1))                                   \
    V(ChangeDoubleToInt32, (GateRef gate, GateRef e1))                                    \
    V(BitCast, (GateRef gate, GateRef e1))                                                \
    V(IntLsl, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(Mod, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(ChangeTaggedPointerToInt64, (GateRef gate, GateRef e1))                             \
    V(ChangeInt64ToTagged, (GateRef gate, GateRef e1))                                    \
    V(DeoptCheck, (GateRef gate))                                                         \
    V(HeapConstant, (GateRef gate, GateRef heapConstant))                                 \
    V(TruncFloatToInt, (GateRef gate, GateRef e1))                                        \
    V(AddWithOverflow, (GateRef gate, GateRef e1, GateRef e2))                            \
    V(SubWithOverflow, (GateRef gate, GateRef e1, GateRef e2))                            \
    V(MulWithOverflow, (GateRef gate, GateRef e1, GateRef e2))                            \
    V(ExtractValue, (GateRef gate, GateRef e1, GateRef e2))                               \
    V(Sqrt, (GateRef gate, GateRef e1))                                                   \
    V(Exp, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Abs, (GateRef gate, GateRef e1))                                                    \
    V(Min, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Max, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Clz32, (GateRef gate, GateRef e1))                                                  \
    V(DoubleTrunc, (GateRef gate, GateRef e1))                                            \
    V(Ceil, (GateRef gate, GateRef e1))                                                   \
    V(Floor, (GateRef gate, GateRef e1))                                                  \
    V(ReadSp, (GateRef gate))                                                             \
    V(InitVreg, (GateRef gate))                                                           \
    V(BitRev, (GateRef gate, GateRef e1))                                                 \
    V(FinishAllocate, (GateRef gate, GateRef e1))

bool IsAddIntergerType(MachineType machineType);
bool IsMulIntergerType(MachineType machineType);
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_IR_BUILDER_H
