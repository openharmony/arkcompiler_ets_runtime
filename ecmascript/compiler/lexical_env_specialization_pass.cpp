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

#include "ecmascript/compiler/lexical_env_specialization_pass.h"

namespace panda::ecmascript::kungfu {
void LexicalEnvSpecializationPass::Initialize()
{
    dependChains_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
}

GateRef LexicalEnvSpecializationPass::VisitDependEntry(GateRef gate)
{
    auto empty = new (chunk_) DependChains(chunk_);
    return UpdateDependChain(gate, empty);
}

GateRef LexicalEnvSpecializationPass::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::JS_BYTECODE: {
            EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
            if (Bytecodes::IsLdLexVarOp(ecmaOpcode)) {
                return TrySpecializeLdLexVar(gate);
            }
            return VisitOther(gate);
        }
        case OpCode::DEPEND_SELECTOR:
            return VisitDependSelector(gate);
        default:
            if (acc_.GetDependCount(gate) == 1) { // 1: depend in is 1
                return VisitOther(gate);
            }
    }
    return Circuit::NullGate();
}

GateRef LexicalEnvSpecializationPass::VisitOther(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) >= 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

GateRef LexicalEnvSpecializationPass::VisitDependSelector(GateRef gate)
{
    auto state = acc_.GetState(gate);
    if (acc_.IsLoopHead(state)) {
        // use loop head as depend chain
        return VisitOther(gate);
    }

    auto dependCount = acc_.GetDependCount(gate);
    for (size_t i = 0; i < dependCount; ++i) {
        auto depend = acc_.GetDep(gate, i);
        auto dependChain = GetDependChain(depend);
        if (dependChain == nullptr) {
            return Circuit::NullGate();
        }
    }

    // all depend done.
    auto depend = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depend);
    DependChains* copy = new (chunk_) DependChains(chunk_);
    copy->CopyFrom(dependChain);
    for (size_t i = 1; i < dependCount; ++i) { // 1: second in
        auto dependIn = acc_.GetDep(gate, i);
        auto tempChain = GetDependChain(dependIn);
        copy->Merge(tempChain);
    }
    HasNotdomStLexVarOrCall(gate, copy->GetHeadGate());
    return UpdateDependChain(gate, copy);
}

GateRef LexicalEnvSpecializationPass::UpdateDependChain(GateRef gate, DependChains* dependChain)
{
    ASSERT(dependChain != nullptr);
    auto oldDependChain = GetDependChain(gate);
    if (dependChain->Equals(oldDependChain)) {
        return Circuit::NullGate();
    }
    dependChains_[acc_.GetId(gate)] = dependChain;
    return gate;
}

GateRef LexicalEnvSpecializationPass::TrySpecializeLdLexVar(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    auto stlexvarGate = LookupStLexvarNode(dependChain, gate);
    if (stlexvarGate != Circuit::NullGate()) {
        return acc_.GetValueIn(stlexvarGate, 3); // 3: stlexvar value in
    }

    // update gate, for others elimination
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

bool LexicalEnvSpecializationPass::SearchStLexVar(GateRef gate, GateRef ldLexVar, GateRef &result)
{
    if (HasNotDomIllegalOp(gate)) {
        result = ldLexVar;
        return false;
    }

    if (acc_.GetOpCode(gate) != OpCode::JS_BYTECODE) {
        return false;
    }

    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    if (Bytecodes::IsStLexVarOp(ecmaOpcode)) {
        if (CheckStLexVar(gate, ldLexVar)) {
            result = gate;
            specializeId_.emplace_back(acc_.GetId(ldLexVar));
            return true;
        }
        return false;
    }

    if (Bytecodes::IsCallOrAccessorOp(ecmaOpcode)) {
        result = ldLexVar;
        return false;
    }

    return false;
}

bool LexicalEnvSpecializationPass::CheckStLexVar(GateRef gate, GateRef ldldLexVar)
{
    int32_t ldLevel = static_cast<int32_t>(acc_.TryGetValue(acc_.GetValueIn(ldldLexVar, 0)));
    int32_t ldSlot = static_cast<int32_t>(acc_.TryGetValue(acc_.GetValueIn(ldldLexVar, 1))); // 1: slot

    int32_t stLevel = static_cast<int32_t>(acc_.TryGetValue(acc_.GetValueIn(gate, 0)));
    int32_t stSlot = static_cast<int32_t>(acc_.TryGetValue(acc_.GetValueIn(gate, 1))); // 1: slot
    if (stSlot != ldSlot) {
        return false;
    }
    int32_t depth = 0;
    GateRef ldEnv = acc_.GetValueIn(ldldLexVar, 2);
    GateRef stEnv = acc_.GetValueIn(gate, 2);
    if (caclulateDistanceToTarget(ldEnv, stEnv, depth)) {
        return (ldLevel == stLevel + depth);
    }

    depth = 0;
    if (caclulateDistanceToTarget(stEnv, ldEnv, depth)) {
        return (ldLevel + depth == stLevel);
    }
    return false;
}

bool LexicalEnvSpecializationPass::caclulateDistanceToTarget(GateRef startEnv, GateRef targetEnv, int32_t &dis)
{
    GateRef curEnv = startEnv;
    while (true) {
        if (curEnv == targetEnv) {
            return true;
        }
        if (acc_.GetOpCode(curEnv) == OpCode::GET_ENV) {
            return false;
        }
        if (acc_.GetOpCode(curEnv) != OpCode::JS_BYTECODE) {
            return false;
        }
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(curEnv);
        if (ecmaOpcode != EcmaOpcode::POPLEXENV) {
            ++dis;
        } else {
            --dis;
        }
        ASSERT(acc_.GetNumValueIn(curEnv) > 0);
        curEnv = acc_.GetValueIn(curEnv, acc_.GetNumValueIn(curEnv) - 1); // 1: env value in
    }
    return false;
}

void LexicalEnvSpecializationPass::HasNotdomStLexVarOrCall(GateRef gate, GateRef next)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::DEPEND_SELECTOR);
    ChunkVector<GateRef> vec(chunk_);
    ChunkVector<GateRef> visited(chunk_);
    vec.emplace_back(gate);
    while (!vec.empty()) {
        GateRef current = vec.back();
        visited.emplace_back(current);
        vec.pop_back();
        if (current != next) {
            if (acc_.GetOpCode(current) == OpCode::JS_BYTECODE) {
                LookUpNotDomStLexVarOrCall(current, next);
            }
            for (size_t i = 0; i < acc_.GetDependCount(current); i++) {
                GateRef dependIn = acc_.GetDep(current, i);
                if (!std::count(visited.begin(), visited.end(), dependIn)) {
                    vec.emplace_back(dependIn);
                }
            }
        }
    }
}

void LexicalEnvSpecializationPass::LookUpNotDomStLexVarOrCall(GateRef current, GateRef next)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(current);
    if (Bytecodes::IsStLexVarOp(ecmaOpcode)) {
        if (current != next) {
            auto iter = notdomStlexvar_.find(next);
            if (iter == notdomStlexvar_.end()) {
                notdomStlexvar_[next] = current;
            }
        }
    }

    if (Bytecodes::IsCallOrAccessorOp(ecmaOpcode)) {
        if (current != next) {
            auto iter = notDomCall_.find(next);
            if (iter == notDomCall_.end()) {
                notDomCall_[next] = current;
            }
        }
    }
}

bool LexicalEnvSpecializationPass::HasNotDomIllegalOp(GateRef gate)
{
    if (HasNotDomStLexvar(gate)) {
        return true;
    }

    if (HasNotDomCall(gate)) {
        return true;
    }

    return false;
}

bool LexicalEnvSpecializationPass::HasNotDomStLexvar(GateRef gate)
{
    auto iter = notdomStlexvar_.find(gate);
    if (iter != notdomStlexvar_.end()) {
        return true;
    }
    return false;
}

bool LexicalEnvSpecializationPass::HasNotDomCall(GateRef gate)
{
    auto iter = notDomCall_.find(gate);
    if (iter != notDomCall_.end()) {
        return true;
    }
    return false;
}

GateRef LexicalEnvSpecializationPass::LookupStLexvarNode(DependChains* dependChain, GateRef gate)
{
    GateRef result = Circuit::NullGate();
    for (auto iter = dependChain->begin(); iter != dependChain->end(); ++iter) {
        GateRef curGate = iter.GetCurrentGate();
        if (SearchStLexVar(curGate, gate, result)) {
            return curGate;
        } else {
            if (result == gate) {
                return Circuit::NullGate();
            }
        }
    }
    return Circuit::NullGate();
}

void LexicalEnvSpecializationPass::PrintSpecializeId()
{
    if (enableLog_) {
        LOG_COMPILER(INFO) << "\033[34m" << "================="
                           << " specialize ldlexvar gate id "
                           << "=================" << "\033[0m";
        for (auto id : specializeId_) {
            LOG_COMPILER(INFO) << "ldlexvar id: " << id;
        }
        LOG_COMPILER(INFO) << "\033[34m" << "===========================================================" << "\033[0m";
    }
}

GateRef GetEnvSpecializationPass::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    if (opcode == OpCode::GET_ENV && acc_.GetOpCode(acc_.GetValueIn(gate, 0)) != OpCode::ARG) {
        GateRef func = acc_.GetValueIn(gate, 0);
        if (acc_.GetOpCode(func) == OpCode::JS_BYTECODE) {
            return TryGetReplaceEnv(func);
        }
    }
    return Circuit::NullGate();
}


GateRef GetEnvSpecializationPass::TryGetReplaceEnv(GateRef func)
{
    ASSERT(acc_.GetNumValueIn(func) >= 1);
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(func);
    switch (ecmaOpcode) {
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            GateRef replacement = acc_.GetValueIn(func, acc_.GetNumValueIn(func) - 1); // 1: last value in
            return replacement;
        }
        default:
            return Circuit::NullGate();
    }
    return Circuit::NullGate();
}
}