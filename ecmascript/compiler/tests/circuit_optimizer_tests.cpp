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

#include <random>

#include "ecmascript/compiler/circuit_optimizer.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class CircuitOptimizerTests : public testing::Test {
};

using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateRef;

HWTEST_F_L0(CircuitOptimizerTests, TestLatticeEquationsSystemSolverFramework)
{
    // construct a circuit
    Circuit circuit;
    GateAccessor acc(&circuit);
    auto n = circuit.NewGate(OpCode(OpCode::ARG),
                             MachineType::I64,
                             0,
                             {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                             GateType::NJSValue());
    auto constantA = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     1,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantB = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     2,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantC = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     1,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantD = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     0,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto loopBegin = circuit.NewGate(OpCode(OpCode::LOOP_BEGIN),
                                     0,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY)), Circuit::NullGate()},
                                     GateType::Empty());
    auto selectorA = circuit.NewGate(OpCode(OpCode::VALUE_SELECTOR),
                                     MachineType::I64,
                                     2,
                                     {loopBegin, constantA, Circuit::NullGate()},
                                     GateType::NJSValue());
    auto selectorB = circuit.NewGate(OpCode(OpCode::VALUE_SELECTOR),
                                     MachineType::I64,
                                     2,
                                     {loopBegin, n, Circuit::NullGate()},
                                     GateType::NJSValue());
    auto newX = circuit.NewGate(OpCode(OpCode::SUB),
                                MachineType::I64,
                                0,
                                {constantB, selectorA},
                                GateType::NJSValue());
    acc.NewIn(selectorA, 2, newX);
    acc.NewIn(selectorB,
              2,
              circuit.NewGate(OpCode(OpCode::SUB),
                              MachineType::I64,
                              0,
                              {selectorB, constantC},
                              GateType::NJSValue()));
    auto predicate = circuit.NewGate(OpCode(OpCode::ICMP),
                                     static_cast<uint64_t>(ecmascript::kungfu::ICmpCondition::NE),
                                     {selectorB, constantD},
                                     GateType::NJSValue());
    auto ifBranch = circuit.NewGate(OpCode(OpCode::IF_BRANCH),
                                    0,
                                    {loopBegin, predicate},
                                    GateType::Empty());
    auto ifTrue = circuit.NewGate(OpCode(OpCode::IF_TRUE),
                                  0,
                                  {ifBranch},
                                  GateType::Empty());
    auto ifFalse = circuit.NewGate(OpCode(OpCode::IF_FALSE),
                                   0,
                                   {ifBranch},
                                   GateType::Empty());
    auto loopBack = circuit.NewGate(OpCode(OpCode::LOOP_BACK),
                                    0,
                                    {ifTrue},
                                    GateType::Empty());
    acc.NewIn(loopBegin, 1, loopBack);
    auto ret = circuit.NewGate(OpCode(OpCode::RETURN),
                               0,
                               {ifFalse,
                                Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                newX,
                                Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                               GateType::Empty());
    // verify the circuit
    {
        auto verifyResult = ecmascript::kungfu::Verifier::Run(&circuit);
        EXPECT_EQ(verifyResult, true);
    }
    {
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 1)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_TRUE(solver.GetValueLattice(acc.GetIn(ret, 2)).GetValue() == 1);
    }
    {
        // modify the initial value of x to 2
        acc.SetBitField(constantA, 2);
    }
    {
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is not constant)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_TRUE(solver.GetValueLattice(acc.GetIn(ret, 2)).IsBot());
    }
    {
        // set the initial value of n to fixed value 0 (instead of function argument)
        acc.SetBitField(n, 0);
        acc.SetOpCode(n, OpCode(OpCode::CONSTANT));
        acc.ReplaceIn(n, 0, Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST)));
    }
    {
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 0)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_TRUE(solver.GetValueLattice(acc.GetIn(ret, 2)).GetValue() == 0);
    }
}

HWTEST_F_L0(CircuitOptimizerTests, TestSubgraphRewriteFramework)
{
    Circuit circuit;
    GateAccessor acc(&circuit);
    const uint64_t numOfConstants = 100;
    const uint64_t numOfUses = 10;
    std::random_device randomDevice;
    std::mt19937_64 rng(randomDevice());
    std::multimap<uint64_t, GateRef> constantsSet;
    for (uint64_t iter = 0; iter < numOfUses; iter++) {
        for (uint64_t idx = 0; idx < numOfConstants; idx++) {
            constantsSet.insert(
                std::make_pair(rng(),
                               circuit.GetConstantGate(MachineType::I64,
                                                       idx,
                                                       GateType::NJSValue())));
        }
    }
    while (constantsSet.size() > 1) {
        const auto elementA = constantsSet.begin();
        const auto operandA = elementA->second;
        constantsSet.erase(elementA);
        const auto elementB = constantsSet.begin();
        const auto operandB = elementB->second;
        constantsSet.erase(elementB);
        constantsSet.insert(
            std::make_pair(rng(),
                           circuit.NewGate(OpCode(OpCode::ADD),
                                           MachineType::I64,
                                           0,
                                           {operandA,
                                            operandB},
                                           GateType::NJSValue())));
    }
    auto ret = circuit.NewGate(OpCode(OpCode::RETURN),
                               0,
                               {Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY)),
                                Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                constantsSet.begin()->second,
                                Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                               GateType::Empty());
    ecmascript::kungfu::SubgraphRewriteRuleCP rule;
    ecmascript::kungfu::SubGraphRewriteFramework rewriter(&rule);
    rewriter.Run(&circuit);
    auto returnValue = acc.GetIn(ret, 2);
    EXPECT_TRUE(acc.GetOpCode(returnValue) == OpCode(OpCode::CONSTANT));
    EXPECT_TRUE(acc.GetBitField(returnValue) == (numOfUses) * (numOfConstants) * (numOfConstants - 1) / 2);
}

HWTEST_F_L0(CircuitOptimizerTests, TestLatticeUpdateRuleSCCP)
{
    Circuit circuit;
    GateAccessor acc(&circuit);
    auto constantA = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I32,
                                     -8848,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantB = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I32,
                                     4,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto newX = circuit.NewGate(OpCode(OpCode::SDIV),
                                MachineType::I32,
                                0,
                                {constantA, constantB},
                                GateType::NJSValue());
    auto ret = circuit.NewGate(OpCode(OpCode::RETURN),
                               0,
                               {Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY)),
                                Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                newX,
                                Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                               GateType::Empty());
    {
        int32_t x = -8848;
        int32_t y = 4;
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant -2212)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_EQ(solver.GetValueLattice(acc.GetIn(ret, 2)).GetValue(),
                  ecmascript::base::bit_cast<uint32_t>(x / y));
    }
    {
        acc.SetOpCode(newX, OpCode(OpCode::UDIV));
        uint32_t x = -8848;
        uint32_t y = 4;
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 1073739612)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_EQ(solver.GetValueLattice(acc.GetIn(ret, 2)).GetValue(), x / y);
    }
    {
        // modify the initial type of constantA to int8_t
        acc.SetMachineType(constantA, MachineType::I8);
        // modify the initial value of constantA to 200
        acc.SetBitField(constantA, 200);
        // modify the initial type of constantB to int8_t
        acc.SetMachineType(constantB, MachineType::I8);
        // modify the initial value of constantB to 200
        acc.SetBitField(constantB, 200);
        acc.SetMachineType(newX, MachineType::I8);
        acc.SetOpCode(newX, OpCode(OpCode::ADD));
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 144)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_EQ(solver.GetValueLattice(acc.GetIn(ret, 2)).GetValue(), 144);
    }
    {
        float x = 9.6;
        float y = 6.9;
        // modify the initial type of constantA to float
        acc.SetMachineType(constantA, MachineType::F32);
        // modify the initial value of constantA to 9.6
        acc.SetBitField(constantA, ecmascript::base::bit_cast<uint32_t>(x));
        // modify the initial type of constantB to float
        acc.SetMachineType(constantB, MachineType::F32);
        // modify the initial value of constantB to 6.9
        acc.SetBitField(constantB, ecmascript::base::bit_cast<uint32_t>(y));
        acc.SetMachineType(newX, MachineType::F32);
        acc.SetOpCode(newX, OpCode(OpCode::FMOD));
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 2.7)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_EQ(ecmascript::base::bit_cast<double>(solver.GetValueLattice(acc.GetIn(ret, 2)).GetValue().value()),
                  fmod(x, y));
    }
}

HWTEST_F_L0(CircuitOptimizerTests, TestSmallSizeGlobalValueNumbering) {
    // construct a circuit
    Circuit circuit;
    GateAccessor acc(&circuit);
    auto constantA = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     1,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantB = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     1,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto argA = circuit.NewGate(OpCode(OpCode::ARG),
                                MachineType::I64,
                                1,
                                {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                GateType::NJSValue());
    auto argB = circuit.NewGate(OpCode(OpCode::ARG),
                                MachineType::I64,
                                2,
                                {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                GateType::NJSValue());

    auto add1 = circuit.NewGate(OpCode(OpCode::ADD),
                               MachineType::I64,
                               0,
                               {constantA, argA},
                               GateType::NJSValue());

    auto add2 = circuit.NewGate(OpCode(OpCode::ADD),
                               MachineType::I64,
                               0,
                               {constantB, argA},
                               GateType::NJSValue());

    auto add3 = circuit.NewGate(OpCode(OpCode::ADD),
                               MachineType::I64,
                               0,
                               {constantA, argB},
                               GateType::NJSValue());
    ecmascript::kungfu::GlobalValueNumbering(&circuit).Run();
    EXPECT_FALSE(acc.GetOpCode(add3).IsNop());
    EXPECT_FALSE(acc.GetOpCode(argA).IsNop());
    EXPECT_FALSE(acc.GetOpCode(argB).IsNop());
    EXPECT_TRUE(acc.GetOpCode(constantA).IsNop() || acc.GetOpCode(constantB).IsNop());
    EXPECT_FALSE(acc.GetOpCode(constantA).IsNop() && acc.GetOpCode(constantB).IsNop());
    EXPECT_TRUE(acc.GetOpCode(add1).IsNop() || acc.GetOpCode(add2).IsNop());
    EXPECT_FALSE(acc.GetOpCode(add1).IsNop() && acc.GetOpCode(add2).IsNop());
}

HWTEST_F_L0(CircuitOptimizerTests, TestMultiLevelGlobalValueNumbering) {
    Circuit circuit;
    GateAccessor acc(&circuit);
    std::random_device randomDevice;
    std::mt19937_64 rng(randomDevice());
    std::vector<GateRef> args;
    for (uint32_t i = 0; i < 5; ++i) {
        args.push_back(circuit.NewGate(OpCode(OpCode::ARG),
                                MachineType::I64,
                                i,
                                {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                GateType::NJSValue()));
    }
    std::map<GateRef, std::vector<GateRef>> addToAdds;
    std::map<GateRef, GateRef> addToAdd;
    std::map<std::pair<GateRef, GateRef>, GateRef> pairToAdd;
    std::vector<GateRef> adds;
    for (uint32_t i = 0; i < 50; ++i) {
        std::pair<GateRef, GateRef> p(args[rng() % 5], args[rng() % 5]);
        auto add = circuit.NewGate(OpCode(OpCode::ADD),
                               MachineType::I64,
                               0,
                               {p.first, p.second},
                               GateType::NJSValue());
        adds.push_back(add);
        if (pairToAdd.count(p) == 0) {
            pairToAdd[p] = add;
            addToAdds[add] = std::vector<GateRef>(0);
        }
        addToAdd[add] = pairToAdd[p];
        addToAdds[addToAdd[add]].emplace_back(add);
    }
    std::map<GateRef, std::vector<GateRef>> subToSubs;
    std::map<GateRef, GateRef> subToSub;
    std::map<std::pair<GateRef, GateRef>, GateRef> pairToSub;
    std::vector<GateRef> subs;
    for (uint32_t i = 0; i < 50; ++i) {
        std::pair<GateRef, GateRef> p(adds[rng() % 5], adds[rng() % 5]);
        auto sub = circuit.NewGate(OpCode(OpCode::SUB),
                               MachineType::I64,
                               0,
                               {p.first, p.second},
                               GateType::NJSValue());
        subs.push_back(sub);
        // remove redundant adds.
        std::pair<GateRef, GateRef> np(addToAdd[p.first], addToAdd[p.second]);
        if (pairToSub.count(np) == 0) {
            pairToSub[np] = sub;
            subToSubs[sub] = std::vector<GateRef>(0);
        }
        subToSub[sub] = pairToSub[np];
        subToSubs[subToSub[sub]].emplace_back(sub);
    }
    ecmascript::kungfu::GlobalValueNumbering(&circuit).Run();
    std::map<GateRef, GateRef> gateToKing;
    for (const auto &p : addToAdds) {
        uint32_t cnt = 0;
        GateRef kingGate;
        for (auto gate : p.second) {
            if (acc.GetOpCode(gate) != OpCode::NOP) {
                cnt++;
                kingGate = gate;
            }
        }
        EXPECT_TRUE(cnt == 1);
        for (auto gate : p.second) {
            gateToKing[gate] = kingGate;
        }
    }
    for (const auto &p : subToSubs) {
        uint32_t cnt = 0;
        GateRef kingGate;
        for (auto gate : p.second) {
            if (acc.GetOpCode(gate) != OpCode::NOP) {
                cnt++;
                kingGate = gate;
            }
        }
        EXPECT_TRUE(cnt == 1);
        for (auto gate : p.second) {
            gateToKing[gate] = kingGate;
        }
    }
    std::vector<GateRef> gates;
    acc.GetAllGates(gates);
    for (auto gate : gates) {
        if (acc.GetOpCode(gate) == OpCode::NOP) {
            continue;
        }
        std::vector<GateRef> ins;
        for (auto in : ins) {
            EXPECT_TRUE(in == gateToKing[in]);
        }
    }
}

HWTEST_F_L0(CircuitOptimizerTests, TestSmallWorldGlobalValueNumbering) {
    Circuit circuit;
    GateAccessor acc(&circuit);
    std::random_device randomDevice;
    std::mt19937_64 rng(randomDevice());
    std::vector<GateRef> args;
    for (uint32_t i = 0; i < 3; ++i) {
        args.push_back(circuit.NewGate(OpCode(OpCode::ARG),
                                MachineType::I64,
                                i,
                                {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                GateType::NJSValue()));
    }
    std::map<GateRef, std::vector<GateRef>> addToAdds;
    std::map<GateRef, GateRef> addToAdd;
    std::map<std::pair<GateRef, GateRef>, GateRef> pairToAdd;
    std::vector<GateRef> adds;
    std::vector<GateRef> toBeSelect;
    for (uint32_t i = 0; i < 10; ++i) {
        std::pair<GateRef, GateRef> p(args[rng() % 3], args[rng() % 3]);
        auto add = circuit.NewGate(OpCode(OpCode::ADD),
                               MachineType::I64,
                               0,
                               {p.first, p.second},
                               GateType::NJSValue());
        adds.emplace_back(add);
        toBeSelect.emplace_back(add);
        toBeSelect.emplace_back(add);
        if (pairToAdd.count(p) == 0) {
            pairToAdd[p] = add;
            addToAdds[add] = std::vector<GateRef>(0);
        }
        addToAdd[add] = pairToAdd[p];
        addToAdds[addToAdd[add]].emplace_back(add);
    }
    for (uint32_t i = 0; i < 1000; ++i) {
        std::pair<GateRef, GateRef> p(toBeSelect[rng() % toBeSelect.size()], toBeSelect[rng() % toBeSelect.size()]);
        auto add = circuit.NewGate(OpCode(OpCode::ADD),
                               MachineType::I64,
                               0,
                               {p.first, p.second},
                               GateType::NJSValue());
        adds.emplace_back(add);
        toBeSelect.emplace_back(add);
        toBeSelect.emplace_back(add);
        toBeSelect.emplace_back(p.first);
        toBeSelect.emplace_back(p.second);

        std::pair<GateRef, GateRef> np(addToAdd[p.first], addToAdd[p.second]);
        if (pairToAdd.count(np) == 0) {
            pairToAdd[np] = add;
            addToAdds[add] = std::vector<GateRef>(0);
        }
        addToAdd[add] = pairToAdd[np];
        addToAdds[addToAdd[add]].emplace_back(add);
    }
    ecmascript::kungfu::GlobalValueNumbering(&circuit).Run();
    std::map<GateRef, GateRef> gateToKing;
    for (const auto &p : addToAdds) {
        uint32_t cnt = 0;
        GateRef kingGate;
        for (auto gate : p.second) {
            if (acc.GetOpCode(gate) != OpCode::NOP) {
                cnt++;
                kingGate = gate;
            }
        }
        EXPECT_TRUE(cnt == 1);
        for (auto gate : p.second) {
            gateToKing[gate] = kingGate;
        }
    }
    std::vector<GateRef> gates;
    acc.GetAllGates(gates);
    for (auto gate : gates) {
        if (acc.GetOpCode(gate) == OpCode::NOP) {
            continue;
        }
        std::vector<GateRef> ins;
        for (auto in : ins) {
            EXPECT_TRUE(in == gateToKing[in]);
        }
    }
}
} // namespace panda::test