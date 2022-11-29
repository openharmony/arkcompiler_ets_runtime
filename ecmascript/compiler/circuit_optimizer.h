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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_OPTIMIZER_H_
#define ECMASCRIPT_COMPILER_CIRCUIT_OPTIMIZER_H_

#include <queue>

#include "ecmascript/base/bit_helper.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
enum class LatticeStatus {
    TOP,
    MID,
    BOT,
};

class ValueLattice {
public:
    explicit ValueLattice();
    explicit ValueLattice(LatticeStatus status);
    explicit ValueLattice(uint64_t value);
    ~ValueLattice() = default;
    [[nodiscard]] bool IsTop() const;
    [[nodiscard]] bool IsMid() const;
    [[nodiscard]] bool IsBot() const;
    [[nodiscard]] LatticeStatus GetStatus() const;
    [[nodiscard]] std::optional<uint64_t> GetValue() const;
    ValueLattice Meet(const ValueLattice &other);
    bool operator==(const ValueLattice &other) const;
    bool operator!=(const ValueLattice &other) const;
    bool operator<(const ValueLattice &other) const;
    bool operator>(const ValueLattice &other) const;
    bool operator<=(const ValueLattice &other) const;
    bool operator>=(const ValueLattice &other) const;
    [[nodiscard]] ValueLattice Implies(const ValueLattice &other) const;
    void Print(std::ostream &os) const;

private:
    uint64_t value_;
    LatticeStatus status_;
};

class ReachabilityLattice {
public:
    explicit ReachabilityLattice();
    explicit ReachabilityLattice(bool reachable);
    ~ReachabilityLattice() = default;
    [[nodiscard]] bool IsReachable() const;
    [[nodiscard]] bool IsUnreachable() const;
    bool operator==(const ReachabilityLattice &other) const;
    bool operator!=(const ReachabilityLattice &other) const;
    ReachabilityLattice operator+(const ReachabilityLattice &other) const;
    ReachabilityLattice operator*(const ReachabilityLattice &other) const;
    [[nodiscard]] ValueLattice Implies(const ValueLattice &other) const;
    void Print(std::ostream &os) const;

private:
    bool reachable_;
};

class LatticeUpdateRule {
public:
    void Initialize(Circuit *circuit,
                    std::function<ValueLattice &(GateRef)> valueLattice,
                    std::function<ReachabilityLattice &(GateRef)> reachabilityLattice);
    bool UpdateValueLattice(GateRef gate, const ValueLattice &valueLattice);
    bool UpdateReachabilityLattice(GateRef gate, const ReachabilityLattice &reachabilityLattice);
    virtual bool Run(GateRef gate) = 0;
    LatticeUpdateRule() : circuit_(nullptr), acc_(circuit_) {}
    virtual ~LatticeUpdateRule() {}

protected:
    std::function<ValueLattice &(GateRef)> valueLatticeMap_;
    std::function<ReachabilityLattice &(GateRef)> reachabilityLatticeMap_;
    Circuit *circuit_;
    GateAccessor acc_;
};

class LatticeUpdateRuleSCCP : public LatticeUpdateRule {
public:
    ~LatticeUpdateRuleSCCP() override {}
    uint64_t RunBoolArithmetic(bool valueA, bool valueB, OpCode op);
    template<class T>
    uint64_t RunFixedPointArithmetic(T valueA, T valueB, OpCode op);
    template<class T>
    double RunFloatingPointArithmetic(T valueA, T valueB, OpCode op);
    uint64_t RunBasicArithmetic(ValueLattice operandA, ValueLattice operandB, OpCode op, MachineType machineType);
    uint64_t RunFCompareArithmetic(ValueLattice operandA, ValueLattice operandB,
                                   FCmpCondition cond, MachineType machineType);
    uint64_t RunICompareArithmetic(ValueLattice operandA, ValueLattice operandB,
                                   ICmpCondition cond, MachineType machineType);
    uint64_t RunBoolCompare(bool valueA, bool valueB, ICmpCondition cond);
    template<class T>
    uint64_t RunFixedPointCompare(T valueA, T valueB, ICmpCondition cond);
    template<class T>
    uint64_t RunFloatingPointCompare(T valueA, T valueB, FCmpCondition cond);
    bool Run(GateRef gate) override;
    bool RunCircuitRoot(GateRef gate);
    bool RunStateEntry(GateRef gate);
    bool RunDependEntry(GateRef gate);
    bool RunFrameStateEntry(GateRef gate);
    bool RunReturnList(GateRef gate);
    bool RunThrowList(GateRef gate);
    bool RunConstantList(GateRef gate);
    bool RunAllocaList(GateRef gate);
    bool RunArgList(GateRef gate);
    bool RunReturn(GateRef gate);
    bool RunReturnVoid(GateRef gate);
    bool RunThrow(GateRef gate);
    bool RunOrdinaryBlock(GateRef gate);
    bool RunIfBranch(GateRef gate);
    bool RunSwitchBranch(GateRef gate);
    bool RunIfTrue(GateRef gate);
    bool RunIfFalse(GateRef gate);
    bool RunSwitchCase(GateRef gate);
    bool RunDefaultCase(GateRef gate);
    bool RunMerge(GateRef gate);
    bool RunLoopBegin(GateRef gate);
    bool RunLoopBack(GateRef gate);
    bool RunValueSelector(GateRef gate);
    bool RunDependSelector(GateRef gate);
    bool RunDependRelay(GateRef gate);
    bool RunDependAnd(GateRef gate);
    bool RunJSBytecode(GateRef gate);
    bool RunIfSuccess(GateRef gate);
    bool RunIfException(GateRef gate);
    bool RunGetException(GateRef gate);
    bool RunRuntimeCall(GateRef gate);
    bool RunNoGCRuntimeCall(GateRef gate);
    bool RunBytecodeCall(GateRef gate);
    bool RunDebuggerBytecodeCall(GateRef gate);
    bool RunBuiltinsCall(GateRef gate);
    bool RunBuiltinsCallWithArgv(GateRef gate);
    bool RunCall(GateRef gate);
    bool RunRuntimeCallWithArgv(GateRef gate);
    bool RunAlloca(GateRef gate);
    bool RunArg(GateRef gate);
    bool RunMutableData(GateRef gate);
    bool RunConstData(GateRef gate);
    bool RunRelocatableData(GateRef gate);
    bool RunConstant(GateRef gate);
    bool RunZExtToIntOrArch(GateRef gate);
    bool RunSExtToIntOrArch(GateRef gate);
    bool RunTruncToInt(GateRef gate);
    bool RunRev(GateRef gate);
    bool RunAdd(GateRef gate);
    bool RunSub(GateRef gate);
    bool RunMul(GateRef gate);
    bool RunExp(GateRef gate);
    bool RunSDiv(GateRef gate);
    bool RunSMod(GateRef gate);
    bool RunUDiv(GateRef gate);
    bool RunUMod(GateRef gate);
    bool RunFDiv(GateRef gate);
    bool RunFMod(GateRef gate);
    bool RunAnd(GateRef gate);
    bool RunXor(GateRef gate);
    bool RunOr(GateRef gate);
    bool RunLSL(GateRef gate);
    bool RunLSR(GateRef gate);
    bool RunASR(GateRef gate);
    bool RunIcmp(GateRef gate);
    bool RunFcmp(GateRef gate);
    bool RunLoad(GateRef gate);
    bool RunStore(GateRef gate);
    bool RunTaggedToInt64(GateRef gate);
    bool RunInt64ToTagged(GateRef gate);
    bool RunSignedIntToFloat(GateRef gate);
    bool RunUnsignedIntToFloat(GateRef gate);
    bool RunFloatToSignedInt(GateRef gate);
    bool RunUnsignedFloatToInt(GateRef gate);
    bool RunBitCast(GateRef gate);
};

class SubgraphRewriteRule {
public:
    SubgraphRewriteRule() : circuit_(nullptr), acc_(circuit_)
    {
    }
    void Initialize(Circuit *circuit);
    virtual bool Run(GateRef gate) = 0;

protected:
    Circuit *circuit_ = nullptr;
    GateAccessor acc_;
};

class SubgraphRewriteRuleCP : public SubgraphRewriteRule {
public:
    bool Run(GateRef gate) override;
    bool RunAdd(GateRef gate);
    bool RunSub(GateRef gate);
};

class LatticeEquationsSystemSolverFramework {
public:
    explicit LatticeEquationsSystemSolverFramework(LatticeUpdateRule *latticeUpdateRule);
    ~LatticeEquationsSystemSolverFramework() = default;
    bool Run(Circuit *circuit, bool enableLogging = false);
    [[nodiscard]] const ValueLattice &GetValueLattice(GateRef gate) const;
    [[nodiscard]] const ReachabilityLattice &GetReachabilityLattice(GateRef gate) const;

private:
    Circuit *circuit_;
    GateAccessor acc_;
    LatticeUpdateRule *latticeUpdateRule_;
    std::map<GateRef, ValueLattice> valueLatticesMap_;
    std::map<GateRef, ReachabilityLattice> reachabilityLatticesMap_;
};

class SubGraphRewriteFramework {
public:
    explicit SubGraphRewriteFramework(SubgraphRewriteRule *subgraphRewriteRule);
    ~SubGraphRewriteFramework() = default;
    bool Run(Circuit *circuit, bool enableLogging = false);

private:
    Circuit *circuit_;
    GateAccessor acc_;
    SubgraphRewriteRule *subgraphRewriteRule_;
};

class Partition;

class PartitionNode {
public:
    PartitionNode();
    PartitionNode(GateRef gate);
    ~PartitionNode() = default;
    std::shared_ptr<PartitionNode> GetPrev() const;
    std::shared_ptr<PartitionNode> GetNext() const;
    std::shared_ptr<Partition> GetBelong() const;
    GateRef GetGate() const;
    void SetPrev(std::shared_ptr<PartitionNode> prev);
    void SetNext(std::shared_ptr<PartitionNode> next);
    void SetBelong(std::shared_ptr<Partition> belong);
    bool ExistUseByIndex(uint32_t index) const;
    void SetUseByIndex(uint32_t index, std::shared_ptr<PartitionNode> node);
    void GetUsesVector(std::vector<std::pair<uint32_t, std::vector<std::shared_ptr<PartitionNode>>>> &uses) const;
private:
    std::weak_ptr<PartitionNode> prev_;
    std::weak_ptr<PartitionNode> next_;
    std::weak_ptr<Partition> belong_;
    GateRef gate_;
    std::map<uint32_t, std::vector<std::shared_ptr<PartitionNode>>> indexToUses_;
};

class Partition {
public:
    Partition();
    ~Partition() = default;
    std::shared_ptr<PartitionNode> GetHead() const;
    void SetHead(std::shared_ptr<PartitionNode> head);
    void SetTouched();
    void SetNotTouched();
    void SetOnWorkList();
    void SetNotOnWorkList();
    bool IsTouched() const;
    bool IsOnWorkList() const;
    uint32_t GetSize() const;
    void SizeUp();
    void SizeDown();
    void AddTouchedNode(std::shared_ptr<PartitionNode> node);
    void CleanTouchedNode();
    size_t GetTouchedSize() const;
    void Insert(std::shared_ptr<PartitionNode> node);
    void Delete(std::shared_ptr<PartitionNode> node);
    std::shared_ptr<Partition> SplitByTouched();
    void MergeUses(std::map<uint32_t, std::vector<std::shared_ptr<PartitionNode>>> &indexToUses) const;
private:
    std::weak_ptr<PartitionNode> head_;
    bool isTouched_;
    bool onWorkList_;
    uint32_t size_;
    std::vector<std::shared_ptr<PartitionNode>> touched_;
};

class GlobalValueNumbering {
public:
    GlobalValueNumbering(Circuit *circuit, bool enableLog);
    ~GlobalValueNumbering() = default;
    void GetPartitionNodes(std::vector<std::shared_ptr<PartitionNode>> &nodes);
    void SplitByOpCode(const std::vector<std::shared_ptr<PartitionNode>> &nodes,
                       std::vector<std::shared_ptr<Partition>> &partitions);
    uint32_t GetMaxIndex(std::shared_ptr<Partition> partition) const;
    void TrySplit(std::queue<std::shared_ptr<Partition>> &workList,
                  std::vector<std::shared_ptr<Partition>> &partitions);
    void EliminateRedundantGates(const std::vector<std::shared_ptr<Partition>> &partitions);
    void Run();
    void Print(const std::vector<std::shared_ptr<Partition>> &partitions);
private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }
    GateAccessor acc_;
    bool enableLog_ {false};
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_OPTIMIZER_H_
