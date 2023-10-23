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

#include "ecmascript/compiler/lexical_env_specialization.h"
#include "ecmascript/compiler/scheduler.h"

namespace panda::ecmascript::kungfu {
void LexicalEnvSpecialization::Initialize()
{
    dependChains_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
    SpecializeInlinedGetEnv();
}

GateRef LexicalEnvSpecialization::VisitDependEntry(GateRef gate)
{
    auto empty = new (chunk_) DependChains(chunk_);
    return UpdateDependChain(gate, empty);
}

void LexicalEnvSpecialization::SpecializeInlinedGetEnv()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        if (acc_.GetOpCode(gate) == OpCode::GET_ENV &&
            acc_.GetOpCode(acc_.GetValueIn(gate, 0)) != OpCode::ARG) {
            GateRef func = acc_.GetValueIn(gate, 0);
            if (acc_.GetOpCode(func) == OpCode::JS_BYTECODE) {
                TryReplaceGetEnv(gate, func);
            }
        }
    }
}

void LexicalEnvSpecialization::TryReplaceGetEnv(GateRef gate, GateRef func)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(func);
    switch (ecmaOpcode) {
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            GateRef replacement = acc_.GetValueIn(func, acc_.GetNumValueIn(func) - 1); // 1: last value in
            ReplaceGetEnv(gate, replacement);
            acc_.DeleteGate(gate);
            break;
        }
        default:
            break;
    }
}

void LexicalEnvSpecialization::ReplaceGetEnv(GateRef gate, GateRef env)
{
    auto uses = acc_.Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        ASSERT(acc_.IsValueIn(useIt));
        useIt = acc_.ReplaceIn(useIt, env);
    }
}

GateRef LexicalEnvSpecialization::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::JS_BYTECODE: {
            EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
            if (ecmaOpcode == EcmaOpcode::LDLEXVAR_IMM4_IMM4 ||
                ecmaOpcode == EcmaOpcode::LDLEXVAR_IMM8_IMM8 ||
                ecmaOpcode == EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16) {
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

GateRef LexicalEnvSpecialization::VisitOther(GateRef gate)
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

GateRef LexicalEnvSpecialization::VisitDependSelector(GateRef gate)
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

GateRef LexicalEnvSpecialization::UpdateDependChain(GateRef gate, DependChains* dependChain)
{
    ASSERT(dependChain != nullptr);
    auto oldDependChain = GetDependChain(gate);
    if (dependChain->Equals(oldDependChain)) {
        return Circuit::NullGate();
    }
    dependChains_[acc_.GetId(gate)] = dependChain;
    return gate;
}

GateRef LexicalEnvSpecialization::TrySpecializeLdLexVar(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    auto stlexvarGate = dependChain->LookupStLexvarNode(this, gate);
    if (stlexvarGate != Circuit::NullGate()) {
        return acc_.GetValueIn(stlexvarGate, 3); // 3: stlexvar value in
    }

    // update gate, for others elimination
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

bool LexicalEnvSpecialization::SearchStLexVar(GateRef gate, GateRef ldLexVar, GateRef &result)
{
    if (HasNotDomIllegalOp(gate)) {
        result = ldLexVar;
        return false;
    }

    if (acc_.GetOpCode(gate) != OpCode::JS_BYTECODE) {
        return false;
    }

    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16: {
            if (CheckStLexVar(gate, ldLexVar)) {
                result = gate;
                specializeId_.emplace_back(acc_.GetId(ldLexVar));
                return true;
            }
            return false;
        }
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            result = ldLexVar;
            return false;
        default:
            return false;;
    }
}

bool LexicalEnvSpecialization::CheckStLexVar(GateRef gate, GateRef ldldLexVar)
{
    int32_t ldLevel = acc_.TryGetValue(acc_.GetValueIn(ldldLexVar, 0));
    int32_t ldSlot = acc_.TryGetValue(acc_.GetValueIn(ldldLexVar, 1)); // 1: slot

    int32_t stLevel = acc_.TryGetValue(acc_.GetValueIn(gate, 0));
    int32_t stSlot = acc_.TryGetValue(acc_.GetValueIn(gate, 1)); // 1: slot
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

bool LexicalEnvSpecialization::caclulateDistanceToTarget(GateRef startEnv, GateRef targetEnv, int32_t &dis)
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
        curEnv = acc_.GetValueIn(curEnv, acc_.GetNumValueIn(curEnv) - 1); // 1: env value in
    }
    return false;
}

void LexicalEnvSpecialization::HasNotdomStLexVarOrCall(GateRef gate, GateRef next)
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

void LexicalEnvSpecialization::LookUpNotDomStLexVarOrCall(GateRef current, GateRef next)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(current);
    switch (ecmaOpcode) {
        case EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            if (current != next) {
                auto iter = notdomStlexvar_.find(next);
                if (iter == notdomStlexvar_.end()) {
                    notdomStlexvar_[next] = current;
                }
            }
            break;
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            if (current != next) {
                auto iter = notDomCall_.find(next);
                if (iter == notDomCall_.end()) {
                    notDomCall_[next] = current;
                }
            }
            break;
        default:
            break;
    }
}

bool LexicalEnvSpecialization::HasNotDomIllegalOp(GateRef gate)
{
    if (HasNotDomStLexvar(gate)) {
        return true;
    }

    if (HasNotDomCall(gate)) {
        return true;
    }

    return false;
}

bool LexicalEnvSpecialization::HasNotDomStLexvar(GateRef gate)
{
    auto iter = notdomStlexvar_.find(gate);
    if (iter != notdomStlexvar_.end()) {
        return true;
    }
    return false;
}

bool LexicalEnvSpecialization::HasNotDomCall(GateRef gate)
{
    auto iter = notDomCall_.find(gate);
    if (iter != notDomCall_.end()) {
        return true;
    }
    return false;
}

void LexicalEnvSpecialization::PrintSpecializeId()
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
}