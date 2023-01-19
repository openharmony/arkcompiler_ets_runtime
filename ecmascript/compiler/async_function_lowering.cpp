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

#include "ecmascript/compiler/async_function_lowering.h"

namespace panda::ecmascript::kungfu {
void AsyncFunctionLowering::ProcessAll()
{
    ProcessJumpTable();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After async function lowering "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

void AsyncFunctionLowering::ProcessJumpTable()
{
    GateRef newTarget = argAccessor_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    GateRef isEqual = builder_.Equal(newTarget, builder_.Undefined());
    GateRef stateEntryState = *accessor_.ConstUses(stateEntry_).begin();
    GateRef ifBranchCondition = builder_.Branch(stateEntry_, isEqual);
    GateRef ifTrueCondition = builder_.IfTrue(ifBranchCondition);
    GateRef ifFalseCondition = builder_.IfFalse(ifBranchCondition);
    accessor_.ReplaceStateIn(stateEntryState, ifTrueCondition);

    GateRef contextOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET);
    GateRef val = builder_.PtrAdd(newTarget, contextOffset);
    GateRef dependStart = builder_.DependRelay(ifFalseCondition, dependEntry_);
    GateRef contextGate = circuit_->NewGate(circuit_->Load(), MachineType::I64, {dependStart, val},
                                            GateType::TaggedPointer());
    GateRef bcOffset = builder_.IntPtr(GeneratorContext::GENERATOR_BC_OFFSET_OFFSET);
    val = builder_.PtrAdd(contextGate, bcOffset);
    GateRef restoreOffsetGate = circuit_->NewGate(circuit_->Load(), MachineType::I32, {contextGate, val},
                                                  GateType::NJSValue());
    GateRef firstState = Circuit::NullGate();
    const auto &suspendAndResumeGates = bcBuilder_->GetAsyncRelatedGates();
    for (const auto &gate : suspendAndResumeGates) {
        EcmaOpcode ecmaOpcode = accessor_.GetByteCodeOpcode(gate);
        if (ecmaOpcode == EcmaOpcode::RESUMEGENERATOR) {
            RebuildGeneratorCfg(gate, restoreOffsetGate, ifFalseCondition, newTarget, firstState);
        }
    }
}

void AsyncFunctionLowering::RebuildGeneratorCfg(GateRef resumeGate, GateRef restoreOffsetGate, GateRef ifFalseCondition,
                                                GateRef newTarget, GateRef &firstState)
{
    GateRef stateGate = accessor_.GetState(resumeGate);
    GateRef suspendGate = stateGate;
    if (accessor_.GetOpCode(suspendGate) == OpCode::IF_SUCCESS) {
        suspendGate = accessor_.GetState(suspendGate);
    }
    GateRef firstRestoreRegGate = GetFirstRestoreRegister(resumeGate);
    GateRef offsetConstantGate = accessor_.GetValueIn(suspendGate);
    offsetConstantGate = builder_.TruncInt64ToInt32(offsetConstantGate);
    auto stateInGate = accessor_.GetState(resumeGate);
    bool flag = true;
    GateRef prevLoopBeginGate = Circuit::NullGate();
    GateRef loopBeginStateIn = Circuit::NullGate();
    GateRef prevBcOffsetPhiGate = Circuit::NullGate();
    while (true) {
        auto opcode = accessor_.GetOpCode(stateInGate);
        if (opcode == OpCode::STATE_ENTRY) {
            GateRef condition = builder_.Equal(offsetConstantGate, restoreOffsetGate);
            GateRef ifBranch = circuit_->NewGate(circuit_->IfBranch(), { ifFalseCondition, condition });
            GateRef ifTrue = circuit_->NewGate(circuit_->IfTrue(), {ifBranch});
            GateRef ifFalse = circuit_->NewGate(circuit_->IfFalse(), {ifBranch});
            GateRef ifTrueDepend = builder_.DependRelay(ifTrue, restoreOffsetGate);
            GateRef ifFalseDepend = builder_.DependRelay(ifFalse, restoreOffsetGate);
            if (flag) {
                accessor_.ReplaceStateIn(resumeGate, ifTrue);
                accessor_.ReplaceValueIn(resumeGate, newTarget);
                accessor_.ReplaceDependIn(firstRestoreRegGate, ifTrueDepend);
                circuit_->NewGate(circuit_->Return(), MachineType::NOVALUE,
                    { stateGate, suspendGate, suspendGate, circuit_->GetReturnRoot() },
                    GateType::AnyType());
            } else {
                loopBeginStateIn = ifTrue;
            }
            accessor_.ReplaceStateIn(ifBranch, ifFalseCondition);
            if (firstState != Circuit::NullGate()) {
                accessor_.ReplaceStateIn(firstState, ifFalse);
            } else {
                auto constant = builder_.UndefineConstant();
                circuit_->NewGate(circuit_->Return(), MachineType::NOVALUE,
                    { ifFalse, ifFalseDepend, constant, circuit_->GetReturnRoot() },
                    GateType::AnyType());
            }
            firstState = ifBranch;
        }

        if (opcode == OpCode::LOOP_BEGIN) {
            // This constant gate must be created by the NewGate method to distinguish whether the while
            // loop needs to modify the phi node or not.
            GateRef emptyOffsetGate = circuit_->GetConstantGate(MachineType::I32, static_cast<uint64_t>(-1),
                                                                GateType::NJSValue());
            // 2: valuesIn
            GateRef bcOffsetPhiGate = circuit_->NewGate(circuit_->ValueSelector(2), MachineType::I32,
                                                        {stateInGate, restoreOffsetGate, emptyOffsetGate},
                                                        GateType::NJSValue());

            GateRef condition = builder_.Equal(offsetConstantGate, bcOffsetPhiGate);
            GateRef ifBranch = circuit_->NewGate(circuit_->IfBranch(), {stateInGate, condition});
            GateRef ifTrue = circuit_->NewGate(circuit_->IfTrue(), {ifBranch});
            GateRef ifFalse = circuit_->NewGate(circuit_->IfFalse(), {ifBranch});

            GateRef resumeStateGate = accessor_.GetState(resumeGate);
            if (accessor_.GetOpCode(resumeStateGate) != OpCode::IF_TRUE) {
                accessor_.ReplaceStateIn(resumeGate, ifTrue);
                accessor_.ReplaceValueIn(resumeGate, newTarget);
                accessor_.ReplaceDependIn(firstRestoreRegGate, bcOffsetPhiGate);
                circuit_->NewGate(circuit_->Return(), MachineType::NOVALUE,
                    { stateGate, suspendGate, suspendGate, circuit_->GetReturnRoot() },
                    GateType::AnyType());
            } else {
                // Handling multi-layer for loops
                UpdateValueSelector(prevLoopBeginGate, ifTrue, prevBcOffsetPhiGate);
                accessor_.ReplaceValueIn(prevBcOffsetPhiGate, bcOffsetPhiGate);
            }
            accessor_.ReplaceStateIn(ifBranch, stateInGate);

            // Find the node with LOOP_BEGIN as State input and modify its
            // state input to the newly created IF_FALSE node.
            auto uses = accessor_.Uses(stateInGate);
            for (auto useIt = uses.begin(); useIt != uses.end();) {
                if (accessor_.GetMetaData(*useIt)->IsState() && *useIt != ifBranch) {
                    useIt = accessor_.ReplaceIn(useIt, ifFalse);
                } else {
                    useIt++;
                }
            }

            prevLoopBeginGate = stateInGate;
            prevBcOffsetPhiGate = bcOffsetPhiGate;
            stateInGate = accessor_.GetState(stateInGate);
            flag = false;
            continue;
        }
        if (loopBeginStateIn != Circuit::NullGate()) {
            UpdateValueSelector(prevLoopBeginGate, loopBeginStateIn, prevBcOffsetPhiGate);
            break;
        }
        if (accessor_.GetOpCode(stateInGate) == OpCode::STATE_ENTRY) {
            break;
        }
        stateInGate = accessor_.GetState(stateInGate);
    }
}

void AsyncFunctionLowering::UpdateValueSelector(GateRef prevLoopBeginGate,
                                                GateRef controlStateGate,
                                                GateRef prevBcOffsetPhiGate)
{
    GateRef loopBeginFirstState = accessor_.GetState(prevLoopBeginGate);
    // 2: statesIn
    GateRef newGate = circuit_->NewGate(circuit_->Merge(2),
                                        {controlStateGate, loopBeginFirstState});
    GateRef emptyOffsetGate = circuit_->GetConstantGate(MachineType::I32,
                                                        static_cast<uint64_t>(-1), // -1: distinguish bcoffset
                                                        GateType::NJSValue());
    GateRef restoreOffset = accessor_.GetValueIn(prevBcOffsetPhiGate);
    // this value selector is compatible with await in the loop body
    GateRef valueSelector = circuit_->NewGate(circuit_->ValueSelector(2), MachineType::I32, // 2: num of valueIn
                                              {newGate, restoreOffset, emptyOffsetGate},
                                              GateType::NJSValue());
    accessor_.ReplaceValueIn(prevBcOffsetPhiGate, valueSelector);
    accessor_.ReplaceStateIn(prevLoopBeginGate, newGate);
    auto loopBeginUses = accessor_.Uses(prevLoopBeginGate);
    for (auto use : loopBeginUses) {
        if (accessor_.GetOpCode(use) == OpCode::VALUE_SELECTOR && use != prevBcOffsetPhiGate) {
            auto machineType = accessor_.GetMachineType(use);
            auto gateType = accessor_.GetGateType(use);
            auto undefinedGate =
                accessor_.GetConstantGate(machineType, JSTaggedValue::VALUE_UNDEFINED, gateType);
            auto firstValueGate = accessor_.GetValueIn(use, 0);
            auto newValueSelector = circuit_->NewGate(circuit_->ValueSelector(2), machineType, // 2: valuesIn
                                                      {newGate, undefinedGate, firstValueGate},
                                                      gateType);
            accessor_.ReplaceValueIn(use, newValueSelector);
        }
    }
}

bool AsyncFunctionLowering::IsAsyncRelated() const
{
    return  bcBuilder_->GetAsyncRelatedGates().size() > 0;
}

GateRef AsyncFunctionLowering::GetFirstRestoreRegister(GateRef gate) const
{
    GateRef firstRestoreGate = gate;
    GateRef curRestoreGate = accessor_.GetDep(gate);
    while (accessor_.GetOpCode(curRestoreGate) == OpCode::RESTORE_REGISTER) {
        firstRestoreGate = curRestoreGate;
        curRestoreGate = accessor_.GetDep(curRestoreGate);
    }
    return firstRestoreGate;
}
}  // panda::ecmascript::kungfu

