/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/js_runtime_options.h"

namespace panda::ecmascript::kungfu {

CompilationConfig::CompilationConfig(const std::string &triple, const JSRuntimeOptions *options)
    : tripleStr_(triple), triple_(GetTripleFromString(triple))
{
    if (options != nullptr) {
        isTraceBc_ = options->IsTraceBC();
        profiling_ = options->GetOptCodeProfiler();
        stressDeopt_ = options->GetStressDeopt();
        verifyVTable_ = options->GetVerifyVTable();
        typedOpProfiling_ = options->GetTypedOpProfiler();
    }
}

Environment::Environment(size_t arguments, CircuitBuilder *builder)
    : circuit_(builder->GetCircuit()), circuitBuilder_(builder), arguments_(arguments)
{
    circuitBuilder_->SetEnvironment(this);
    SetCompilationConfig(circuitBuilder_->GetCompilationConfig());
    for (size_t i = 0; i < arguments; i++) {
        arguments_[i] = circuitBuilder_->Arguments(i);
    }
    entry_ = Label(NewLabel(this, circuit_->GetStateRoot()));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto depend_entry = circuit_->GetDependRoot();
    currentLabel_->SetDepend(depend_entry);
}

Environment::Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder)
    : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    SetCompilationConfig(circuitBuilder_->GetCompilationConfig());
    GateAccessor acc(circuit);
    entry_ = Label(NewLabel(this, acc.GetIn(hir, 0)));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto dependEntry = acc.GetDep(hir);
    currentLabel_->SetDepend(dependEntry);
    for (size_t i = 2; i < acc.GetNumIns(hir); i++) {
        inputList_.emplace_back(acc.GetIn(hir, i));
    }
}

Environment::Environment(GateRef stateEntry, GateRef dependEntry,
    const std::initializer_list<GateRef>& args, Circuit *circuit, CircuitBuilder *builder)
    : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    SetCompilationConfig(circuitBuilder_->GetCompilationConfig());
    entry_ = Label(NewLabel(this, stateEntry));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    currentLabel_->SetDepend(dependEntry);
    for (auto in : args) {
        inputList_.emplace_back(in);
    }
}

Environment::~Environment()
{
    circuitBuilder_->SetEnvironment(nullptr);
    for (auto label : rawLabels_) {
        delete label;
    }
}

Label::Label(Environment *env)
{
    ASSERT(env != nullptr);
    impl_ = env->NewLabel(env);
}

Label::Label(CircuitBuilder *cirBuilder)
{
    ASSERT(cirBuilder != nullptr);
    auto env = cirBuilder->GetCurrentEnvironment();
    ASSERT(env != nullptr);
    impl_ = env->NewLabel(env);
}

void Label::LabelImpl::Seal()
{
    for (auto &[variable, gate] : incompletePhis_) {
        variable->AddPhiOperand(gate);
    }
    isSealed_ = true;
}

void Label::LabelImpl::WriteVariable(Variable *var, GateRef value)
{
    valueMap_[var] = value;
}

GateRef Label::LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        GateAccessor acc(env_->GetCircuit());
        if (!acc.IsNop(result)) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef Label::LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    MachineType machineType = CircuitBuilder::GetMachineTypeFromVariableType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;
        if (machineType == MachineType::NOVALUE) {
            val = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR,
                predeControl_, {}, valueCounts, var->Type());
        } else {
            val = env_->GetBuilder()->Selector(OpCode::VALUE_SELECTOR,
                machineType, predeControl_, {}, valueCounts, var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (machineType == MachineType::NOVALUE) {
            val = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR,
                predeControl_, {}, this->predecessors_.size(), var->Type());
        } else {
            val = env_->GetBuilder()->Selector(OpCode::VALUE_SELECTOR, machineType,
                predeControl_, {}, this->predecessors_.size(), var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        WriteVariable(var, val);
        val = var->AddPhiOperand(val);
    }
    WriteVariable(var, val);
    return val;
}

void Label::LabelImpl::Bind()
{
    ASSERT(!predecessors_.empty());
    if (IsLoopHead()) {
        // 2 means input number of depend selector gate
        loopDepend_ = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR, predeControl_, {}, 2);
        GateAccessor(env_->GetCircuit()).NewIn(loopDepend_, 1, predecessors_[0]->GetDepend());
        depend_ = loopDepend_;
    }
    if (IsNeedSeal()) {
        Seal();
        MergeAllControl();
        MergeAllDepend();
    }
}

void Label::LabelImpl::MergeAllControl()
{
    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        return;
    }

    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        ASSERT(otherPredeControls_.size() == 1);
        GateAccessor(env_->GetCircuit()).NewIn(predeControl_, 1, otherPredeControls_[0]);
        return;
    }

    // merge all control of predecessors_
    std::vector<GateRef> inGates(predecessors_.size());
    size_t i = 0;
    ASSERT(predeControl_ != -1);
    ASSERT((otherPredeControls_.size() + 1) == predecessors_.size());
    inGates[i++] = predeControl_;
    for (auto in : otherPredeControls_) {
        inGates[i++] = in;
    }

    GateRef merge = env_->GetBuilder()->Merge(inGates);
    predeControl_ = merge;
    control_ = merge;
}

void Label::LabelImpl::MergeAllDepend()
{
    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (IsControlCase()) {
            // Add depend_relay to current label
            depend_ = env_->GetBuilder()->DependRelay(predeControl_, depend_);
        }
        return;
    }
    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        // Add loop depend to in of depend_seclector
        ASSERT(loopDepend_ != -1);
        // 2 mean 3rd input gate for loopDepend_(depend_selector)
        GateAccessor(env_->GetCircuit()).NewIn(loopDepend_, 2, predecessors_[1]->GetDepend());
        return;
    }

    //  Merge all depends to depend_seclector
    std::vector<GateRef> dependsList;
    for (auto prede : this->GetPredecessors()) {
        dependsList.push_back(prede->GetDepend());
    }
    depend_ = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR,
        predeControl_, dependsList, dependsList.size());
}

void Label::LabelImpl::AppendPredecessor(Label::LabelImpl *predecessor)
{
    if (predecessor != nullptr) {
        predecessors_.push_back(predecessor);
    }
}

bool Label::LabelImpl::IsNeedSeal() const
{
    auto stateCount = GateAccessor(env_->GetCircuit()).GetStateCount(predeControl_);
    return predecessors_.size() >= stateCount;
}

bool Label::LabelImpl::IsLoopHead() const
{
    return GateAccessor(env_->GetCircuit()).IsLoopHead(predeControl_);
}

bool Label::LabelImpl::IsControlCase() const
{
    return GateAccessor(env_->GetCircuit()).IsControlCase(predeControl_);
}
}