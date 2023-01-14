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

#include "ecmascript/compiler/circuit_optimizer.h"

namespace panda::ecmascript::kungfu {
ValueLattice::ValueLattice() : value_(0), status_(LatticeStatus::TOP)
{
}

ValueLattice::ValueLattice(LatticeStatus status) : value_(0), status_(status)
{
}

ValueLattice::ValueLattice(uint64_t value) : value_(value), status_(LatticeStatus::MID)
{
}

bool ValueLattice::IsTop() const
{
    return GetStatus() == LatticeStatus::TOP;
}

bool ValueLattice::IsMid() const
{
    return GetStatus() == LatticeStatus::MID;
}

bool ValueLattice::IsBot() const
{
    return GetStatus() == LatticeStatus::BOT;
}

LatticeStatus ValueLattice::GetStatus() const
{
    return status_;
}

std::optional<uint64_t> ValueLattice::GetValue() const
{
    if (IsTop() || IsBot()) {
        return std::nullopt;
    }
    return value_;
}

ValueLattice ValueLattice::Meet(const ValueLattice &other)
{
    if (this->IsTop()) {
        return other;
    }
    if (other.IsTop()) {
        return *this;
    }
    if (this->IsBot() || other.IsBot()) {
        return ValueLattice(LatticeStatus::BOT);
    }
    // both are single
    if (this->GetValue().value() != other.GetValue().value()) {
        return ValueLattice(LatticeStatus::BOT);
    }
    return *this;
}

bool ValueLattice::operator==(const ValueLattice &other) const
{
    if (this->IsTop() && other.IsTop()) {
        return true;
    }
    if (this->IsBot() && other.IsBot()) {
        return true;
    }
    if (this->IsMid() && other.IsMid()) {
        return this->GetValue().value() == other.GetValue().value();
    }
    return false;
}

bool ValueLattice::operator!=(const ValueLattice &other) const
{
    return !(*this == other);
}

bool ValueLattice::operator<(const ValueLattice &other) const
{
    if (this->IsMid() && other.IsTop()) {
        return true;
    }
    if (this->IsBot() && other.IsMid()) {
        return true;
    }
    if (this->IsBot() && other.IsTop()) {
        return true;
    }
    return false;
}

bool ValueLattice::operator>(const ValueLattice &other) const
{
    return !(*this < other);
}

bool ValueLattice::operator<=(const ValueLattice &other) const
{
    return (*this < other) || (*this == other);
}

bool ValueLattice::operator>=(const ValueLattice &other) const
{
    return (*this > other) || (*this == other);
}

ValueLattice ValueLattice::Implies(const ValueLattice &other) const
{
    if (!this->IsTop()) {
        return other;
    }
    return ValueLattice(LatticeStatus::TOP);
}

void ValueLattice::Print(std::ostream &os) const
{
    if (IsTop()) {
        os << "TOP";
    } else {
        if (IsBot()) {
            os << "BOT";
        } else {
            os << GetValue().value();
        }
    }
}

ReachabilityLattice::ReachabilityLattice() : reachable_(false)
{
}

ReachabilityLattice::ReachabilityLattice(bool reachable) : reachable_(reachable)
{
}

bool ReachabilityLattice::IsReachable() const
{
    return reachable_;
}

bool ReachabilityLattice::IsUnreachable() const
{
    return !reachable_;
}

bool ReachabilityLattice::operator==(const ReachabilityLattice &other) const
{
    return this->IsReachable() == other.IsReachable();
}

bool ReachabilityLattice::operator!=(const ReachabilityLattice &other) const
{
    return !(*this == other);
}

ReachabilityLattice ReachabilityLattice::operator+(const ReachabilityLattice &other) const
{
    return ReachabilityLattice(this->IsReachable() || other.IsReachable());
}

ReachabilityLattice ReachabilityLattice::operator*(const ReachabilityLattice &other) const
{
    return ReachabilityLattice(this->IsReachable() && other.IsReachable());
}

ValueLattice ReachabilityLattice::Implies(const ValueLattice &other) const
{
    if (this->IsReachable()) {
        return other;
    }
    return ValueLattice(LatticeStatus::TOP);
}

void ReachabilityLattice::Print(std::ostream &os) const
{
    if (this->IsReachable()) {
        os << "reachable";
    } else {
        os << "unreachable";
    }
}

void LatticeUpdateRule::Initialize(Circuit *circuit,
                                   std::function<ValueLattice &(GateRef)> valueLattice,
                                   std::function<ReachabilityLattice &(GateRef)> reachabilityLattice)
{
    circuit_ = circuit;
    acc_ = GateAccessor(circuit);
    valueLatticeMap_ = std::move(valueLattice);
    reachabilityLatticeMap_ = std::move(reachabilityLattice);
}

bool LatticeUpdateRule::UpdateValueLattice(GateRef gate, const ValueLattice &valueLattice)
{
    if (valueLatticeMap_(gate) != valueLattice) {
        valueLatticeMap_(gate) = valueLattice;
        return true;
    }
    return false;
}

bool LatticeUpdateRule::UpdateReachabilityLattice(GateRef gate, const ReachabilityLattice &reachabilityLattice)
{
    if (reachabilityLatticeMap_(gate) != reachabilityLattice) {
        reachabilityLatticeMap_(gate) = reachabilityLattice;
        return true;
    }
    return false;
}

uint64_t LatticeUpdateRuleSCCP::RunBoolArithmetic(bool valueA, bool valueB, OpCode op)
{
    switch (op) {
        case OpCode::ADD:
            return (valueA + valueB);
        case OpCode::SUB:
            return (valueA - valueB);
        case OpCode::MUL:
            return (valueA * valueB);
        case OpCode::AND:
            return (valueA & valueB);
        case OpCode::XOR:
            return (valueA ^ valueB);
        case OpCode::OR:
            return (valueA | valueB);
        default:
            LOG_COMPILER(ERROR) << "unexpected op!";
            return 0;
    }
    return 0;
}

template<class T>
uint64_t LatticeUpdateRuleSCCP::RunFixedPointArithmetic(T valueA, T valueB, OpCode op)
{
    static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");
    using make_signed_t = typename std::make_signed<T>::type;
    switch (op) {
        case OpCode::ADD:
            return (valueA + valueB);
        case OpCode::SUB:
            return (valueA - valueB);
        case OpCode::MUL:
            return (valueA * valueB);
        case OpCode::SDIV:
            return (static_cast<make_signed_t>(valueA) / static_cast<make_signed_t>(valueB));
        case OpCode::UDIV:
            return (valueA / valueB);
        case OpCode::SMOD:
            return (static_cast<make_signed_t>(valueA) % static_cast<make_signed_t>(valueB));
        case OpCode::UMOD:
            return (valueA % valueB);
        case OpCode::AND:
            return (valueA & valueB);
        case OpCode::XOR:
            return (valueA ^ valueB);
        case OpCode::OR:
            return (valueA | valueB);
        case OpCode::LSL:
            return (valueA << valueB);
        case OpCode::LSR:
            return (valueA >> valueB);
        case OpCode::ASR:
            return (static_cast<make_signed_t>(valueA) >> static_cast<make_signed_t>(valueB));
        default:
            LOG_COMPILER(ERROR) << "unexpected op!";
            return 0;
    }
    return 0;
}

template<class T>
double LatticeUpdateRuleSCCP::RunFloatingPointArithmetic(T valueA, T valueB, OpCode op)
{
    switch (op) {
        case OpCode::ADD:
            return (valueA + valueB);
        case OpCode::SUB:
            return (valueA - valueB);
        case OpCode::MUL:
            return (valueA * valueB);
        case OpCode::FDIV:
            return (valueA / valueB);
        case OpCode::FMOD:
            return fmod(valueA, valueB);
        default:
            LOG_COMPILER(ERROR) << "unexpected op!";
            return 0;
    }
    return 0;
}

uint64_t LatticeUpdateRuleSCCP::RunBasicArithmetic(ValueLattice operandA, ValueLattice operandB,
                                                   OpCode op, MachineType machineType)
{
    auto valueA = operandA.GetValue().value();
    auto valueB = operandB.GetValue().value();
    if (machineType == MachineType::I1) {
        return static_cast<bool>(RunBoolArithmetic(static_cast<bool>(valueA),
                                                   static_cast<bool>(valueB), op));
    } else if (machineType == MachineType::I8) {
        return static_cast<uint8_t>(RunFixedPointArithmetic(static_cast<uint8_t>(valueA),
                                                            static_cast<uint8_t>(valueB), op));
    } else if (machineType == MachineType::I16) {
        return static_cast<uint16_t>(RunFixedPointArithmetic(static_cast<uint16_t>(valueA),
                                                             static_cast<uint16_t>(valueB), op));
    } else if (machineType == MachineType::I32) {
        return static_cast<uint32_t>(RunFixedPointArithmetic(static_cast<uint32_t>(valueA),
                                                             static_cast<uint32_t>(valueB), op));
    } else if (machineType == MachineType::I64) {
        return RunFixedPointArithmetic(static_cast<uint64_t>(valueA), static_cast<uint64_t>(valueB), op);
    } else if (machineType == MachineType::F32) {
        float valueA_ = base::bit_cast<float>(static_cast<uint32_t>(valueA));
        float valueB_ = base::bit_cast<float>(static_cast<uint32_t>(valueB));
        return base::bit_cast<uint64_t>(RunFloatingPointArithmetic(valueA_, valueB_, op));
    } else if (machineType == MachineType::F64) {
        double valueA_ = base::bit_cast<double>(static_cast<uint64_t>(valueA));
        double valueB_ = base::bit_cast<double>(static_cast<uint64_t>(valueB));
        return base::bit_cast<uint64_t>(RunFloatingPointArithmetic(valueA_, valueB_, op));
    } else {
        LOG_COMPILER(ERROR) << "unexpected machineType!";
    }
    return 0;
}

uint64_t LatticeUpdateRuleSCCP::RunFCompareArithmetic(ValueLattice operandA, ValueLattice operandB,
                                                      FCmpCondition cond, MachineType machineType)
{
    auto valueA = operandA.GetValue().value();
    auto valueB = operandB.GetValue().value();
    if (machineType == MachineType::F32) {
        float valueA_ = base::bit_cast<float>(static_cast<uint32_t>(valueA));
        float valueB_ = base::bit_cast<float>(static_cast<uint32_t>(valueB));
        return base::bit_cast<uint64_t>(RunFloatingPointCompare(valueA_, valueB_, cond));
    } else if (machineType == MachineType::F64) {
        double valueA_ = base::bit_cast<double>(static_cast<uint64_t>(valueA));
        double valueB_ = base::bit_cast<double>(static_cast<uint64_t>(valueB));
        return base::bit_cast<uint64_t>(RunFloatingPointCompare(valueA_, valueB_, cond));
    } else {
        LOG_COMPILER(ERROR) << "unexpected machineType!";
    }
    return 0;
}

uint64_t LatticeUpdateRuleSCCP::RunICompareArithmetic(ValueLattice operandA, ValueLattice operandB,
                                                      ICmpCondition cond, MachineType machineType)
{
    auto valueA = operandA.GetValue().value();
    auto valueB = operandB.GetValue().value();
    if (machineType == MachineType::I1) {
        return static_cast<bool>(RunBoolCompare(static_cast<bool>(valueA),
                                                static_cast<bool>(valueB), cond));
    } else if (machineType == MachineType::I8) {
        return static_cast<uint8_t>(RunFixedPointCompare(static_cast<uint8_t>(valueA),
                                                         static_cast<uint8_t>(valueB), cond));
    } else if (machineType == MachineType::I16) {
        return static_cast<uint16_t>(RunFixedPointCompare(static_cast<uint16_t>(valueA),
                                                          static_cast<uint16_t>(valueB), cond));
    } else if (machineType == MachineType::I32) {
        return static_cast<uint32_t>(RunFixedPointCompare(static_cast<uint32_t>(valueA),
                                                          static_cast<uint32_t>(valueB), cond));
    } else if (machineType == MachineType::I64) {
        return RunFixedPointCompare(static_cast<uint64_t>(valueA), static_cast<uint64_t>(valueB), cond);
    } else {
        LOG_COMPILER(ERROR) << "unexpected machineType!";
    }
    return 0;
}

uint64_t LatticeUpdateRuleSCCP::RunBoolCompare(bool valueA, bool valueB, ICmpCondition cond)
{
    switch (cond) {
        case ICmpCondition::EQ:
            return (valueA == valueB ? 1 : 0);
        case ICmpCondition::NE:
            return (valueA != valueB ? 1 : 0);
        default:
            LOG_COMPILER(ERROR) << "unexpected cond!";
            return 0;
    }
}

template<class T>
uint64_t LatticeUpdateRuleSCCP::RunFixedPointCompare(T valueA, T valueB, ICmpCondition cond)
{
    static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");
    using make_signed_t = typename std::make_signed<T>::type;
    switch (cond) {
        case ICmpCondition::SLT:
            return (static_cast<make_signed_t>(valueA) < static_cast<make_signed_t>(valueB));
        case ICmpCondition::SLE:
            return (static_cast<make_signed_t>(valueA) <= static_cast<make_signed_t>(valueB));
        case ICmpCondition::SGT:
            return (static_cast<make_signed_t>(valueA) > static_cast<make_signed_t>(valueB));
        case ICmpCondition::SGE:
            return (static_cast<make_signed_t>(valueA) >= static_cast<make_signed_t>(valueB));
        case ICmpCondition::ULT:
            return (valueA < valueB);
        case ICmpCondition::ULE:
            return (valueA <= valueB);
        case ICmpCondition::UGT:
            return (valueA > valueB);
        case ICmpCondition::UGE:
            return (valueA >= valueB);
        case ICmpCondition::NE:
            return (valueA == valueB ? 1 : 0);
        case ICmpCondition::EQ:
            return (valueA != valueB ? 1 : 0);
        default:
            UNREACHABLE();
    }
}

template<class T>
uint64_t LatticeUpdateRuleSCCP::RunFloatingPointCompare(T valueA, T valueB, FCmpCondition cond)
{
    switch (cond) {
        case FCmpCondition::OLT:
            return (valueA < valueB);
        case FCmpCondition::OLE:
            return (valueA <= valueB);
        case FCmpCondition::OGT:
            return (valueA > valueB);
        case FCmpCondition::OGE:
            return (valueA >= valueB);
        case FCmpCondition::ONE:
            return (valueA != valueB ? 1 : 0);
        case FCmpCondition::OEQ:
            return (valueA == valueB ? 1 : 0);
        default:
            LOG_COMPILER(ERROR) << "unexpected cond!";
            return 0;
    }
    return 0;
}

bool LatticeUpdateRuleSCCP::Run(GateRef gate)
{
    const std::map<OpCode, std::function<bool(void)>> functionTable = {
        {OpCode::CIRCUIT_ROOT, [&]() -> bool { return RunCircuitRoot(gate); }},
        {OpCode::STATE_ENTRY, [&]() -> bool { return RunStateEntry(gate); }},
        {OpCode::DEPEND_ENTRY, [&]() -> bool { return RunDependEntry(gate); }},
        {OpCode::RETURN_LIST, [&]() -> bool { return RunReturnList(gate); }},
        {OpCode::ARG_LIST, [&]() -> bool { return RunArgList(gate); }},
        {OpCode::RETURN, [&]() -> bool { return RunReturn(gate); }},
        {OpCode::RETURN_VOID, [&]() -> bool { return RunReturnVoid(gate); }},
        {OpCode::THROW, [&]() -> bool { return RunThrow(gate); }},
        {OpCode::ORDINARY_BLOCK, [&]() -> bool { return RunOrdinaryBlock(gate); }},
        {OpCode::IF_BRANCH, [&]() -> bool { return RunIfBranch(gate); }},
        {OpCode::SWITCH_BRANCH, [&]() -> bool { return RunSwitchBranch(gate); }},
        {OpCode::IF_TRUE, [&]() -> bool { return RunIfTrue(gate); }},
        {OpCode::IF_FALSE, [&]() -> bool { return RunIfFalse(gate); }},
        {OpCode::SWITCH_CASE, [&]() -> bool { return RunSwitchCase(gate); }},
        {OpCode::DEFAULT_CASE, [&]() -> bool { return RunDefaultCase(gate); }},
        {OpCode::MERGE, [&]() -> bool { return RunMerge(gate); }},
        {OpCode::LOOP_BEGIN, [&]() -> bool { return RunLoopBegin(gate); }},
        {OpCode::LOOP_BACK, [&]() -> bool { return RunLoopBack(gate); }},
        {OpCode::VALUE_SELECTOR, [&]() -> bool { return RunValueSelector(gate); }},
        {OpCode::DEPEND_SELECTOR, [&]() -> bool { return RunDependSelector(gate); }},
        {OpCode::DEPEND_RELAY, [&]() -> bool { return RunDependRelay(gate); }},
        {OpCode::DEPEND_AND, [&]() -> bool { return RunDependAnd(gate); }},
        {OpCode::JS_BYTECODE, [&]() -> bool { return RunJSBytecode(gate); }},
        {OpCode::IF_SUCCESS, [&]() -> bool { return RunIfSuccess(gate); }},
        {OpCode::IF_EXCEPTION, [&]() -> bool { return RunIfException(gate); }},
        {OpCode::GET_EXCEPTION, [&]() -> bool { return RunGetException(gate); }},
        {OpCode::RUNTIME_CALL, [&]() -> bool { return RunRuntimeCall(gate); }},
        {OpCode::NOGC_RUNTIME_CALL, [&]() -> bool { return RunNoGCRuntimeCall(gate); }},
        {OpCode::BYTECODE_CALL, [&]() -> bool { return RunBytecodeCall(gate); }},
        {OpCode::BUILTINS_CALL, [&]() -> bool { return RunBuiltinsCall(gate); }},
        {OpCode::BUILTINS_CALL_WITH_ARGV, [&]() -> bool { return RunBuiltinsCallWithArgv(gate); }},
        {OpCode::DEBUGGER_BYTECODE_CALL, [&]() -> bool { return RunDebuggerBytecodeCall(gate); }},
        {OpCode::CALL, [&]() -> bool { return RunCall(gate); }},
        {OpCode::RUNTIME_CALL_WITH_ARGV, [&]() -> bool { return RunRuntimeCallWithArgv(gate); }},
        {OpCode::ALLOCA, [&]() -> bool { return RunAlloca(gate); }},
        {OpCode::ARG, [&]() -> bool { return RunArg(gate); }},
        {OpCode::CONST_DATA, [&]() -> bool { return RunConstData(gate); }},
        {OpCode::RELOCATABLE_DATA, [&]() -> bool { return RunRelocatableData(gate); }},
        {OpCode::CONSTANT, [&]() -> bool { return RunConstant(gate); }},
        {OpCode::ZEXT, [&]() -> bool { return RunZExtToIntOrArch(gate); }},
        {OpCode::SEXT, [&]() -> bool { return RunSExtToIntOrArch(gate); }},
        {OpCode::TRUNC, [&]() -> bool { return RunTruncToInt(gate); }},
        {OpCode::REV, [&]() -> bool { return RunRev(gate); }},
        {OpCode::ADD, [&]() -> bool { return RunAdd(gate); }},
        {OpCode::SUB, [&]() -> bool { return RunSub(gate); }},
        {OpCode::MUL, [&]() -> bool { return RunMul(gate); }},
        {OpCode::EXP, [&]() -> bool { return RunExp(gate); }},
        {OpCode::SDIV, [&]() -> bool { return RunSDiv(gate); }},
        {OpCode::SMOD, [&]() -> bool { return RunSMod(gate); }},
        {OpCode::UDIV, [&]() -> bool { return RunUDiv(gate); }},
        {OpCode::UMOD, [&]() -> bool { return RunUMod(gate); }},
        {OpCode::FDIV, [&]() -> bool { return RunFDiv(gate); }},
        {OpCode::FMOD, [&]() -> bool { return RunFMod(gate); }},
        {OpCode::AND, [&]() -> bool { return RunAnd(gate); }},
        {OpCode::XOR, [&]() -> bool { return RunXor(gate); }},
        {OpCode::OR, [&]() -> bool { return RunOr(gate); }},
        {OpCode::LSL, [&]() -> bool { return RunLSL(gate); }},
        {OpCode::LSR, [&]() -> bool { return RunLSR(gate); }},
        {OpCode::ASR, [&]() -> bool { return RunASR(gate); }},
        {OpCode::ICMP, [&]() -> bool { return RunIcmp(gate); }},
        {OpCode::FCMP, [&]() -> bool { return RunFcmp(gate); }},
        {OpCode::LOAD, [&]() -> bool { return RunLoad(gate); }},
        {OpCode::STORE, [&]() -> bool { return RunStore(gate); }},
        {OpCode::TAGGED_TO_INT64, [&]() -> bool { return RunTaggedToInt64(gate); }},
        {OpCode::INT64_TO_TAGGED, [&]() -> bool { return RunInt64ToTagged(gate); }},
        {OpCode::SIGNED_INT_TO_FLOAT, [&]() -> bool { return RunSignedIntToFloat(gate); }},
        {OpCode::UNSIGNED_INT_TO_FLOAT, [&]() -> bool { return RunUnsignedIntToFloat(gate); }},
        {OpCode::FLOAT_TO_SIGNED_INT, [&]() -> bool { return RunFloatToSignedInt(gate); }},
        {OpCode::UNSIGNED_FLOAT_TO_INT, [&]() -> bool { return RunUnsignedFloatToInt(gate); }},
        {OpCode::BITCAST, [&]() -> bool { return RunBitCast(gate); }},
    };
    return functionTable.at(acc_.GetOpCode(gate))();
}

bool LatticeUpdateRuleSCCP::RunCircuitRoot([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunStateEntry(GateRef gate)
{
    return UpdateReachabilityLattice(gate, ReachabilityLattice(true));
}

bool LatticeUpdateRuleSCCP::RunDependEntry(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunFrameStateEntry([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunReturnList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunThrowList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunConstantList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunAllocaList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunArgList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunReturn(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunReturnVoid(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunThrow(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunOrdinaryBlock(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunIfBranch(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunSwitchBranch(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunIfTrue(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    const bool predicateMayBeTrue =
        valueLatticeMap_(acc_.GetIn(previousState, 1)) <= ValueLattice(1);
    return UpdateReachabilityLattice(gate,
                                     reachabilityLatticeMap_(previousState)
                                         * ReachabilityLattice(predicateMayBeTrue));
}

bool LatticeUpdateRuleSCCP::RunIfFalse(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    const bool predicateMayBeFalse =
        valueLatticeMap_(acc_.GetIn(previousState, 1)) <= ValueLattice(0);
    return UpdateReachabilityLattice(gate,
                                     reachabilityLatticeMap_(previousState)
                                         * ReachabilityLattice(predicateMayBeFalse));
}

bool LatticeUpdateRuleSCCP::RunSwitchCase(GateRef gate)
{
    const bool valueMayMatch =
        valueLatticeMap_(acc_.GetIn(acc_.GetIn(gate, 0), 1))
            <= ValueLattice(acc_.TryGetValue(gate));
    return UpdateReachabilityLattice(gate,
                                     reachabilityLatticeMap_(acc_.GetIn(gate, 0)) * ReachabilityLattice(valueMayMatch));
}

bool LatticeUpdateRuleSCCP::RunDefaultCase(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunMerge(GateRef gate)
{
    ReachabilityLattice reachable;
    for (const auto &input : acc_.ConstIns(gate)) {
        reachable = reachable + reachabilityLatticeMap_(input);
    }
    return UpdateReachabilityLattice(gate, reachable);
}

bool LatticeUpdateRuleSCCP::RunLoopBegin(GateRef gate)
{
    ReachabilityLattice reachable;
    for (const auto &input : acc_.ConstIns(gate)) {
        reachable = reachable + reachabilityLatticeMap_(input);
    }
    return UpdateReachabilityLattice(gate, reachable);
}

bool LatticeUpdateRuleSCCP::RunLoopBack(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunValueSelector(GateRef gate)
{
    const auto relatedState = acc_.GetIn(gate, 0);
    ValueLattice value;
    size_t cnt = 0;
    for (const auto &input : acc_.ConstIns(gate)) {
        if (cnt > 0) {
            value = value.Meet(reachabilityLatticeMap_(
                acc_.GetIn(relatedState, cnt - 1)).Implies(valueLatticeMap_(input)));
        }
        cnt++;
    }
    return UpdateValueLattice(gate, value);
}

bool LatticeUpdateRuleSCCP::RunDependSelector(GateRef gate)
{
    const auto relatedState = acc_.GetIn(gate, 0);
    ValueLattice value;
    size_t cnt = 0;
    for (const auto &input : acc_.ConstIns(gate)) {
        if (cnt > 0) {
            value = value.Meet(reachabilityLatticeMap_(
                acc_.GetIn(relatedState, cnt - 1)).Implies(valueLatticeMap_(input)));
        }
        cnt++;
    }
    if (!value.IsTop()) {
        value = ValueLattice(LatticeStatus::BOT);
    }
    return UpdateValueLattice(gate, value);
}

bool LatticeUpdateRuleSCCP::RunDependRelay(GateRef gate)
{
    const auto relatedState = acc_.GetIn(gate, 0);
    ValueLattice value = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (!value.IsTop()) {
        value = ValueLattice(LatticeStatus::BOT);
    }
    return UpdateValueLattice(gate, reachabilityLatticeMap_(relatedState).Implies(value));
}

bool LatticeUpdateRuleSCCP::RunDependAnd(GateRef gate)
{
    ValueLattice value = ValueLattice(LatticeStatus::BOT);
    for (const auto &input : acc_.ConstIns(gate)) {
        if (valueLatticeMap_(input).IsTop()) {
            value = ValueLattice(LatticeStatus::TOP);
        }
    }
    return UpdateValueLattice(gate, value);
}

bool LatticeUpdateRuleSCCP::RunJSBytecode(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState))
        || UpdateValueLattice(gate, reachabilityLatticeMap_(gate).Implies(ValueLattice(LatticeStatus::BOT)));
}

bool LatticeUpdateRuleSCCP::RunIfSuccess(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunIfException(GateRef gate)
{
    const auto previousState = acc_.GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunGetException(GateRef gate)
{
    return UpdateValueLattice(gate, valueLatticeMap_(acc_.GetIn(gate, 0)).Implies(
        ValueLattice(LatticeStatus::BOT)));
}

bool LatticeUpdateRuleSCCP::RunRuntimeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunNoGCRuntimeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunBytecodeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunDebuggerBytecodeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunBuiltinsCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunBuiltinsCallWithArgv(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunRuntimeCallWithArgv(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunAlloca(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunArg(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunMutableData(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunConstData(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunRelocatableData(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunConstant(GateRef gate)
{
    const auto constantValue = ValueLattice(acc_.GetConstantValue(gate));
    return UpdateValueLattice(gate, constantValue);
}

bool LatticeUpdateRuleSCCP::RunZExtToIntOrArch(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunSExtToIntOrArch(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunTruncToInt(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunRev(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    if (operandA.IsMid()) {
        return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value()));
    }
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunAdd(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::ADD, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunSub(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::SUB, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunMul(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if ((operandA.IsMid() && operandA.GetValue().value() == 0) ||
        (operandB.IsMid() && operandB.GetValue().value() == 0)) {
        return UpdateValueLattice(gate, ValueLattice(0));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::MUL, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunExp(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto fastPow = [](uint64_t a, uint64_t b) {
        uint64_t result = 1;
        uint64_t power = a;
        while (b) {
            if (b & 1) {
                result = result * power;
            }
            power = power * power;
            b >>= 1;
        }
        return result;
    };
    return UpdateValueLattice(gate, ValueLattice(fastPow(operandA.GetValue().value(), operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSDiv(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::SDIV, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunSMod(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::SMOD, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunUDiv(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::UDIV, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunUMod(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::UMOD, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunFDiv(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::FDIV, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunFMod(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::FMOD, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunAnd(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::AND, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunXor(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::XOR, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunOr(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::OR, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunLSL(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::LSL, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunLSR(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::LSR, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunASR(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunBasicArithmetic(operandA, operandB, OpCode::ASR, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunIcmp(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    ICmpCondition cond = acc_.GetICmpCondition(gate);
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunICompareArithmetic(operandA, operandB, cond, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunFcmp(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(acc_.GetIn(gate, 1));
    FCmpCondition cond = acc_.GetFCmpCondition(gate);
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto machineType = acc_.GetMachineType(gate);
    auto value = RunFCompareArithmetic(operandA, operandB, cond, machineType);
    return UpdateValueLattice(gate, ValueLattice(value));
}

bool LatticeUpdateRuleSCCP::RunLoad(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunStore(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunTaggedToInt64(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunInt64ToTagged(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunSignedIntToFloat(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunUnsignedIntToFloat(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunFloatToSignedInt(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunUnsignedFloatToInt(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunBitCast(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(acc_.GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

void SubgraphRewriteRule::Initialize(Circuit *circuit)
{
    circuit_ = circuit;
    acc_ = GateAccessor(circuit);
}

bool SubgraphRewriteRuleCP::Run(GateRef gate)
{
    const std::map<OpCode, std::function<bool(void)>> functionTable = {
        {OpCode::ADD, [&]() -> bool { return RunAdd(gate); }},
        {OpCode::SUB, [&]() -> bool { return RunSub(gate); }},
    };
    if (!functionTable.count(acc_.GetOpCode(gate))) {
        return false;
    }
    return functionTable.at(acc_.GetOpCode(gate))();
}

bool SubgraphRewriteRuleCP::RunAdd(GateRef gate)
{
    GateAccessor acc(circuit_);
    const auto &operandA = acc_.GetIn(gate, 0);
    const auto &operandB = acc_.GetIn(gate, 1);
    if (acc_.GetOpCode(operandA) == OpCode::CONSTANT &&
        acc_.GetOpCode(operandB) == OpCode::CONSTANT) {
        acc_.DeleteIn(gate, 0);
        acc_.DeleteIn(gate, 1);
        const auto valueA = acc_.GetConstantValue(operandA);
        const auto valueB = acc_.GetConstantValue(operandB);
        acc_.SetMetaData(gate, circuit_->Constant(valueA + valueB));
        return true;
    }
    return false;
}

bool SubgraphRewriteRuleCP::RunSub(GateRef gate)
{
    GateAccessor acc(circuit_);
    const auto &operandA = acc_.GetIn(gate, 0);
    const auto &operandB = acc_.GetIn(gate, 1);
    if (acc_.GetOpCode(operandA) == OpCode::CONSTANT &&
        acc_.GetOpCode(operandB) == OpCode::CONSTANT) {
        acc_.DeleteIn(gate, 0);
        acc_.DeleteIn(gate, 1);
        const auto valueA = acc_.GetConstantValue(operandA);
        const auto valueB = acc_.GetConstantValue(operandB);
        acc_.SetMetaData(gate, circuit_->Constant(valueA - valueB));
        return true;
    }
    return false;
}

LatticeEquationsSystemSolverFramework::LatticeEquationsSystemSolverFramework(LatticeUpdateRule *latticeUpdateRule)
    : circuit_(nullptr), acc_(nullptr), latticeUpdateRule_(latticeUpdateRule)
{
}

bool LatticeEquationsSystemSolverFramework::Run(Circuit *circuit, bool enableLogging)
{
    circuit_ = circuit;
    acc_ = GateAccessor(circuit);
    auto valueLatticeMapFunction = [&](GateRef gate) -> ValueLattice & {
        return valueLatticesMap_[gate];
    };
    auto reachabilityLatticeMapFunction = [&](GateRef gate) -> ReachabilityLattice & {
        return reachabilityLatticesMap_[gate];
    };
    latticeUpdateRule_->Initialize(circuit_, valueLatticeMapFunction, reachabilityLatticeMapFunction);
    std::deque<GateRef> workList;
    std::set<GateRef> workSet;
    std::vector<GateRef> gates;
    circuit_->GetAllGates(gates);
    for (auto gate : gates) {
        workList.push_back(gate);
        workSet.insert(gate);
    }
    while (!workList.empty()) {
        const auto gate = workList.front();
        workList.pop_front();
        workSet.erase(gate);
        if (latticeUpdateRule_->Run(gate) || acc_.GetMetaData(gate)->IsCFGMerge()) {
            for (const auto &output : acc_.ConstUses(gate)) {
                if (!workSet.count(output)) {
                    workList.push_back(output);  // work queue
                    workSet.insert(output);
                }
            }
        }
    }
    if (enableLogging) {
        circuit_->GetAllGates(gates);
        for (auto gate : gates) {
            if (valueLatticesMap_.count(gate)) {
                if (valueLatticesMap_.at(gate).IsTop()) {
                    std::cerr << "[Top]";
                } else if (valueLatticesMap_.at(gate).IsBot()) {
                    std::cerr << "[Bot]";
                } else {
                    std::cerr << "[" << valueLatticesMap_.at(gate).GetValue().value() << "]";
                }
            }
            if (reachabilityLatticesMap_.count(gate)) {
                if (reachabilityLatticesMap_.at(gate).IsReachable()) {
                    std::cerr << "[Reachable]";
                } else {
                    std::cerr << "[Unreachable]";
                }
            }
            std::cerr << " ";
            acc_.Print(gate);
        }
    }
    return true;
}

const ValueLattice &LatticeEquationsSystemSolverFramework::GetValueLattice(GateRef gate) const
{
    return valueLatticesMap_.at(gate);
}

const ReachabilityLattice &LatticeEquationsSystemSolverFramework::GetReachabilityLattice(GateRef gate) const
{
    return reachabilityLatticesMap_.at(gate);
}

SubGraphRewriteFramework::SubGraphRewriteFramework(SubgraphRewriteRule *subgraphRewriteRule)
    : circuit_(nullptr), acc_(nullptr), subgraphRewriteRule_(subgraphRewriteRule)
{
}

bool SubGraphRewriteFramework::Run(Circuit *circuit, bool enableLogging)
{
    circuit_ = circuit;
    acc_ = GateAccessor(circuit);
    subgraphRewriteRule_->Initialize(circuit_);
    std::deque<GateRef> workList;
    std::set<GateRef> workSet;
    std::vector<GateRef> gates;
    circuit_->GetAllGates(gates);
    for (auto gate : gates) {
        workList.push_back(gate);
        workSet.insert(gate);
    }
    while (!workList.empty()) {
        const auto gate = workList.front();
        workList.pop_front();
        workSet.erase(gate);
        if (subgraphRewriteRule_->Run(gate)) {
            for (const auto &output : acc_.ConstUses(gate)) {
                if (!workSet.count(output)) {
                    workList.push_front(output);  // work stack
                    workSet.insert(output);
                }
            }
        }
    }
    if (enableLogging) {
        circuit_->GetAllGates(gates);
        for (auto gate : gates) {
            acc_.Print(gate);
        }
    }
    return true;
}

PartitionNode::PartitionNode() : gate_(Circuit::NullGate())
{
}

PartitionNode::PartitionNode(GateRef gate) : gate_(gate)
{
}

std::shared_ptr<PartitionNode> PartitionNode::GetPrev() const
{
    return prev_.lock();
}

std::shared_ptr<PartitionNode> PartitionNode::GetNext() const
{
    return next_.lock();
}

std::shared_ptr<Partition> PartitionNode::GetBelong() const
{
    return belong_.lock();
}

GateRef PartitionNode::GetGate() const
{
    return gate_;
}

void PartitionNode::SetPrev(std::shared_ptr<PartitionNode> prev)
{
    prev_ = prev;
}

void PartitionNode::SetNext(std::shared_ptr<PartitionNode> next)
{
    next_ = next;
}

void PartitionNode::SetBelong(std::shared_ptr<Partition> belong)
{
    belong_ = belong;
}

bool PartitionNode::ExistUseByIndex(uint32_t index) const
{
    return indexToUses_.count(index) > 0;
}

void PartitionNode::SetUseByIndex(uint32_t index, std::shared_ptr<PartitionNode> node)
{
    if (!ExistUseByIndex(index)) {
        indexToUses_.emplace(index, std::vector<std::shared_ptr<PartitionNode>>(0));
    }
    indexToUses_[index].emplace_back(node);
}

void PartitionNode::GetUsesVector(std::vector<std::pair<uint32_t,
                                  std::vector<std::shared_ptr<PartitionNode>>>> &uses) const
{
    for (const auto &p : indexToUses_) {
        uses.emplace_back(p);
    }
}

Partition::Partition() : isTouched_(false), onWorkList_(false), size_(0)
{
}

std::shared_ptr<PartitionNode> Partition::GetHead() const
{
    return head_.lock();
}

void Partition::SetHead(std::shared_ptr<PartitionNode> head)
{
    head_ = head;
}

void Partition::SetTouched()
{
    isTouched_ = true;
}

void Partition::SetNotTouched()
{
    isTouched_ = false;
}

void Partition::SetOnWorkList()
{
    onWorkList_ = true;
}

void Partition::SetNotOnWorkList()
{
    onWorkList_ = false;
}

bool Partition::IsTouched() const
{
    return isTouched_;
}

bool Partition::IsOnWorkList() const
{
    return onWorkList_;
}

uint32_t Partition::GetSize() const
{
    return size_;
}

void Partition::SizeUp()
{
    ++size_;
}

void Partition::SizeDown()
{
    --size_;
}

void Partition::AddTouchedNode(std::shared_ptr<PartitionNode> node)
{
    touched_.emplace_back(node);
}

void Partition::CleanTouchedNode()
{
    touched_.clear();
}

size_t Partition::GetTouchedSize() const
{
    return touched_.size();
}

void Partition::Insert(std::shared_ptr<PartitionNode> node)
{
    if (this->GetHead() != nullptr) {
        this->GetHead()->SetPrev(node);
    }
    node->SetPrev(nullptr);
    node->SetNext(this->GetHead());
    this->SetHead(node);
    this->SizeUp();
}

void Partition::Delete(std::shared_ptr<PartitionNode> node)
{
    if (node->GetPrev() != nullptr) {
        node->GetPrev()->SetNext(node->GetNext());
    } else {
        this->SetHead(node->GetNext());
    }
    if (node->GetNext() != nullptr) {
        node->GetNext()->SetPrev(node->GetPrev());
    }
    node->SetPrev(nullptr);
    node->SetNext(nullptr);
    this->SizeDown();
}

std::shared_ptr<Partition> Partition::SplitByTouched()
{
    for (auto node : touched_) {
        this->Delete(node);
    }
    auto newPartition = std::make_shared<Partition>(Partition());
    for (auto node : touched_) {
        newPartition->Insert(node);
        node->SetBelong(newPartition);
    }
    return newPartition;
}

void Partition::MergeUses(std::map<uint32_t, std::vector<std::shared_ptr<PartitionNode>>> &indexToUses) const
{
    std::vector<std::pair<uint32_t, std::vector<std::shared_ptr<PartitionNode>>>> uses;
    for (auto defNode = this->GetHead(); defNode != nullptr; defNode = defNode->GetNext()) {
        uses.clear();
        defNode->GetUsesVector(uses);
        for (const auto &use : uses) {
            auto index = use.first;
            const auto &useNodes = use.second;
            if (indexToUses.count(index) == 0) {
                indexToUses.emplace(index, std::vector<std::shared_ptr<PartitionNode>>(0));
            }
            for (auto useNode : useNodes) {
                indexToUses[index].emplace_back(useNode);
            }
        }
    }
}

GlobalValueNumbering::GlobalValueNumbering(Circuit *circuit, bool enableLog)
    : acc_(GateAccessor(circuit)), enableLog_(enableLog)
{
}

void GlobalValueNumbering::GetPartitionNodes(std::vector<std::shared_ptr<PartitionNode>> &pNodes)
{
    std::vector<GateRef> gates;
    std::map<GateRef, std::shared_ptr<PartitionNode>> gateToNode;
    acc_.GetAllGates(gates);
    for (auto gate : gates) {
        auto node = std::make_shared<PartitionNode>(PartitionNode(gate));
        pNodes.emplace_back(node);
        gateToNode[gate] = node;
    }
    for (auto gate : gates) {
        size_t count = acc_.GetNumIns(gate);
        auto node = gateToNode[gate];
        for (size_t i = 0; i < count; ++i) {
            GateRef r = acc_.GetIn(gate, i);
            auto defNode = gateToNode[r];
            defNode->SetUseByIndex(i, node);
        }
    }
}

void GlobalValueNumbering::SplitByOpCode(const std::vector<std::shared_ptr<PartitionNode>> &nodes,
                                         std::vector<std::shared_ptr<Partition>> &partitions)
{
    std::map<std::tuple<OpCode, BitField, MachineType, uint32_t>, std::shared_ptr<Partition>> opToPartition;
    for (auto node : nodes) {
        auto op = OpCode(acc_.GetOpCode(node->GetGate()));
        auto bit = acc_.TryGetValue(node->GetGate());
        auto mt = acc_.GetMachineType(node->GetGate());
        auto gt = acc_.GetGateType(node->GetGate()).Value();
        auto tp = std::make_tuple(op, bit, mt, gt);
        if (opToPartition.count(tp) == 0) {
            auto p = std::make_shared<Partition>(Partition());
            opToPartition[tp] = p;
            partitions.emplace_back(p);
        }
        auto p = opToPartition[tp];
        node->SetBelong(p);
        p->Insert(node);
    }
}

void GlobalValueNumbering::TrySplit(std::queue<std::shared_ptr<Partition>> &workList,
                                    std::vector<std::shared_ptr<Partition>> &partitions)
{
    auto curPartition = workList.front();
    workList.pop();
    curPartition->SetNotOnWorkList();
    std::vector<std::shared_ptr<Partition>> touchedPartition;
    std::map<uint32_t, std::vector<std::shared_ptr<PartitionNode>>> indexToUses;
    curPartition->MergeUses(indexToUses);
    for (const auto &use : indexToUses) {
        const auto &useNodes = use.second;
        for (auto useNode : useNodes) {
            if (!useNode->GetBelong()->IsTouched()) {
                useNode->GetBelong()->SetTouched();
                touchedPartition.emplace_back(useNode->GetBelong());
            }
            useNode->GetBelong()->AddTouchedNode(useNode);
        }
        for (auto partition : touchedPartition) {
            if (partition->GetSize() != static_cast<uint32_t>(partition->GetTouchedSize())) {
                auto newPartition = partition->SplitByTouched();
                if (partition->IsOnWorkList() || partition->GetSize() > newPartition->GetSize()) {
                    workList.push(newPartition);
                    newPartition->SetOnWorkList();
                } else {
                    workList.push(partition);
                    partition->SetOnWorkList();
                }
                partitions.emplace_back(newPartition);
            }
            partition->CleanTouchedNode();
        }
        for (auto partition : touchedPartition) {
            partition->SetNotTouched();
        }
        touchedPartition.clear();
    }
}

void GlobalValueNumbering::EliminateRedundantGates(const std::vector<std::shared_ptr<Partition>> &partitions)
{
    if (IsLogEnabled()) {
        Print(partitions);
    }
    for (auto partition : partitions) {
        std::map<uint32_t, std::vector<std::shared_ptr<PartitionNode>>> indexToUses;
        partition->MergeUses(indexToUses);
        auto kingNode = partition->GetHead();
        for (const auto &uses : indexToUses) {
            auto index = uses.first;
            const auto &useNodes = uses.second;
            for (auto useNode : useNodes) {
                acc_.ReplaceIn(useNode->GetGate(), index, kingNode->GetGate());
            }
        }
    }
    for (auto partition : partitions) {
        auto kingNode = partition->GetHead();
        for (auto node = kingNode->GetNext(); node != nullptr; node = node->GetNext()) {
            acc_.DeleteGate(node->GetGate());
        }
    }
}

void GlobalValueNumbering::Run()
{
    std::vector<std::shared_ptr<PartitionNode>> pNodes;
    GetPartitionNodes(pNodes);
    std::vector<std::shared_ptr<Partition>> partitions;
    SplitByOpCode(pNodes, partitions);
    std::queue<std::shared_ptr<Partition>> workList;
    for (auto p : partitions) {
        workList.push(p);
        p->SetOnWorkList();
    }
    while (!workList.empty()) {
        TrySplit(workList, partitions);
    }
    EliminateRedundantGates(partitions);
}

void GlobalValueNumbering::Print(const std::vector<std::shared_ptr<Partition>> &partitions)
{
    for (auto partition : partitions) {
        auto kingNode = partition->GetHead();
        std::string log = "[global-value-numbering] replace [";
        bool noGateReplaced = true;
        for (auto node = kingNode->GetNext(); node != nullptr; node = node->GetNext()) {
            if (noGateReplaced) {
                noGateReplaced = false;
            } else {
                log += ", ";
            }
            log += std::to_string(acc_.GetId(node->GetGate()));
        }
        if (noGateReplaced) {
            continue;
        }
        log += "] with " + GateMetaData::Str(acc_.GetOpCode(kingNode->GetGate())) + " " +
                std::to_string(acc_.GetId(kingNode->GetGate()));
        LOG_COMPILER(INFO) << log;
    }
}
}  // namespace panda::ecmascript::kungfu
