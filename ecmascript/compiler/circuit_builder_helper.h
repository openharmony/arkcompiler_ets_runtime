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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_HELPER_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_HELPER_H

#include "ecmascript/compiler/circuit_builder.h"

namespace panda::ecmascript {
    class JSRuntimeOptions;
} // namespace panda::ecmascript

namespace panda::ecmascript::kungfu {

class CompilationConfig {
public:
    explicit PUBLIC_API CompilationConfig(const std::string &triple, const JSRuntimeOptions *options = nullptr);
    ~CompilationConfig() = default;

    inline bool Is32Bit() const
    {
        return triple_ == Triple::TRIPLE_ARM32;
    }

    inline bool IsAArch64() const
    {
        return triple_ == Triple::TRIPLE_AARCH64;
    }

    inline bool IsAmd64() const
    {
        return triple_ == Triple::TRIPLE_AMD64;
    }

    inline bool Is64Bit() const
    {
        return IsAArch64() || IsAmd64();
    }

    Triple GetTriple() const
    {
        return triple_;
    }

    std::string GetTripleStr() const
    {
        return tripleStr_;
    }

    bool IsTraceBC() const
    {
        return isTraceBc_;
    }

    bool IsProfiling() const
    {
        return profiling_;
    }

    bool IsStressDeopt() const
    {
        return stressDeopt_;
    }

    bool IsVerifyVTbale() const
    {
        return verifyVTable_;
    }

    bool IsTypedOpProfiling() const
    {
        return typedOpProfiling_;
    }

private:
    inline Triple GetTripleFromString(const std::string &triple)
    {
        if (triple.compare(TARGET_X64) == 0) {
            return Triple::TRIPLE_AMD64;
        }

        if (triple.compare(TARGET_AARCH64) == 0) {
            return Triple::TRIPLE_AARCH64;
        }

        if (triple.compare(TARGET_ARM32) == 0) {
            return Triple::TRIPLE_ARM32;
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    std::string tripleStr_;
    Triple triple_;
    bool isTraceBc_ {false};
    bool profiling_ {false};
    bool stressDeopt_ {false};
    bool verifyVTable_ {false};
    bool typedOpProfiling_ {false};
};

class Label {
public:
    Label() = default;
    explicit Label(Environment *env);
    explicit Label(CircuitBuilder *cirBuilder);
    ~Label() = default;
    Label(Label const &label) = default;
    Label &operator=(Label const &label) = default;
    Label(Label &&label) = default;
    Label &operator=(Label &&label) = default;
    inline void Seal()
    {
        return impl_->Seal();
    }
    inline void WriteVariable(Variable *var, GateRef value)
    {
        impl_->WriteVariable(var, value);
    }
    inline GateRef ReadVariable(Variable *var)
    {
        return impl_->ReadVariable(var);
    }
    inline void Bind()
    {
        impl_->Bind();
    }
    inline void MergeAllControl()
    {
        impl_->MergeAllControl();
    }
    inline void MergeAllDepend()
    {
        impl_->MergeAllDepend();
    }
    inline void AppendPredecessor(const Label *predecessor)
    {
        impl_->AppendPredecessor(predecessor->GetRawLabel());
    }
    inline std::vector<Label> GetPredecessors() const
    {
        std::vector<Label> labels;
        for (auto rawlabel : impl_->GetPredecessors()) {
            labels.emplace_back(Label(rawlabel));
        }
        return labels;
    }
    inline void SetControl(GateRef control)
    {
        impl_->SetControl(control);
    }
    inline void SetPreControl(GateRef control)
    {
        impl_->SetPreControl(control);
    }
    inline void MergeControl(GateRef control)
    {
        impl_->MergeControl(control);
    }
    inline GateRef GetControl() const
    {
        return impl_->GetControl();
    }
    inline GateRef GetDepend() const
    {
        return impl_->GetDepend();
    }
    inline void SetDepend(GateRef depend)
    {
        return impl_->SetDepend(depend);
    }

private:
    class LabelImpl {
    public:
        LabelImpl(Environment *env, GateRef control)
            : env_(env), control_(control), predeControl_(-1), isSealed_(false)
        {
        }
        ~LabelImpl() = default;
        NO_MOVE_SEMANTIC(LabelImpl);
        NO_COPY_SEMANTIC(LabelImpl);
        void Seal();
        void WriteVariable(Variable *var, GateRef value);
        GateRef ReadVariable(Variable *var);
        void Bind();
        void MergeAllControl();
        void MergeAllDepend();
        void AppendPredecessor(LabelImpl *predecessor);
        std::vector<LabelImpl *> GetPredecessors() const
        {
            return predecessors_;
        }
        void SetControl(GateRef control)
        {
            control_ = control;
        }
        void SetPreControl(GateRef control)
        {
            predeControl_ = control;
        }
        void MergeControl(GateRef control)
        {
            if (predeControl_ == Circuit::NullGate()) {
                predeControl_ = control;
                control_ = predeControl_;
            } else {
                otherPredeControls_.push_back(control);
            }
        }
        GateRef GetControl() const
        {
            return control_;
        }
        void SetDepend(GateRef depend)
        {
            depend_ = depend;
        }
        GateRef GetDepend() const
        {
            return depend_;
        }

    private:
        bool IsNeedSeal() const;
        bool IsSealed() const
        {
            return isSealed_;
        }
        bool IsLoopHead() const;
        bool IsControlCase() const;
        GateRef ReadVariableRecursive(Variable *var);
        Environment *env_;
        GateRef control_;
        GateRef predeControl_ {Circuit::NullGate()};
        GateRef depend_ {Circuit::NullGate()};
        GateRef loopDepend_ {Circuit::NullGate()};
        std::vector<GateRef> otherPredeControls_;
        bool isSealed_ {false};
        std::map<Variable *, GateRef> valueMap_;
        std::vector<GateRef> phi;
        std::vector<LabelImpl *> predecessors_;
        std::map<Variable *, GateRef> incompletePhis_;
    };

    explicit Label(LabelImpl *impl) : impl_(impl) {}
    friend class Environment;
    LabelImpl *GetRawLabel() const
    {
        return impl_;
    }
    LabelImpl *impl_ {nullptr};
};

class Environment {
public:
    using LabelImpl = Label::LabelImpl;
    Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder);
    Environment(GateRef stateEntry, GateRef dependEntry, const std::initializer_list<GateRef>& args,
                Circuit *circuit, CircuitBuilder *builder);
    Environment(size_t arguments, CircuitBuilder *builder);
    ~Environment();
    Label *GetCurrentLabel() const
    {
        return currentLabel_;
    }
    void SetCurrentLabel(Label *label)
    {
        currentLabel_ = label;
    }
    CircuitBuilder *GetBuilder() const
    {
        return circuitBuilder_;
    }
    Circuit *GetCircuit() const
    {
        return circuit_;
    }
    int NextVariableId()
    {
        return nextVariableId_++;
    }
    void SetCompilationConfig(const CompilationConfig *cfg)
    {
        ccfg_ = cfg;
    }
    const CompilationConfig *GetCompilationConfig() const
    {
        return ccfg_;
    }
    inline bool Is32Bit() const
    {
        return ccfg_->Is32Bit();
    }
    inline bool IsAArch64() const
    {
        return ccfg_->IsAArch64();
    }
    inline bool IsAmd64() const
    {
        return ccfg_->IsAmd64();
    }
    inline bool IsArch64Bit() const
    {
        return ccfg_->IsAmd64() ||  ccfg_->IsAArch64();
    }
    inline bool IsAsmInterp() const
    {
        return circuit_->GetFrameType() == FrameType::ASM_INTERPRETER_FRAME;
    }
    inline bool IsBaselineBuiltin() const
    {
        return circuit_->GetFrameType() == FrameType::BASELINE_BUILTIN_FRAME;
    }
    inline bool IsArch32Bit() const
    {
        return ccfg_->Is32Bit();
    }
    inline GateRef GetArgument(size_t index) const
    {
        return arguments_.at(index);
    }
    inline Label GetLabelFromSelector(GateRef sel)
    {
        Label::LabelImpl *rawlabel = phiToLabels_[sel];
        return Label(rawlabel);
    }
    inline void AddSelectorToLabel(GateRef sel, Label label)
    {
        phiToLabels_[sel] = label.GetRawLabel();
    }
    inline LabelImpl *NewLabel(Environment *env, GateRef control = -1)
    {
        auto impl = new Label::LabelImpl(env, control);
        rawLabels_.emplace_back(impl);
        return impl;
    }
    void SubCfgEntry(Label *entry)
    {
        if (currentLabel_ != nullptr) {
            GateRef control = currentLabel_->GetControl();
            GateRef depend = currentLabel_->GetDepend();
            stack_.push(currentLabel_);
            currentLabel_ = entry;
            currentLabel_->SetControl(control);
            currentLabel_->SetDepend(depend);
        }
    }
    void SubCfgExit()
    {
        if (currentLabel_ != nullptr) {
            GateRef control = currentLabel_->GetControl();
            GateRef depend = currentLabel_->GetDepend();
            if (!stack_.empty()) {
                currentLabel_ = stack_.top();
                currentLabel_->SetControl(control);
                currentLabel_->SetDepend(depend);
                stack_.pop();
            }
        }
    }
    inline GateRef GetInput(size_t index) const
    {
        return inputList_.at(index);
    }

private:
    Label *currentLabel_ {nullptr};
    Circuit *circuit_ {nullptr};
    CircuitBuilder *circuitBuilder_ {nullptr};
    std::unordered_map<GateRef, LabelImpl *> phiToLabels_;
    std::vector<GateRef> inputList_;
    Label entry_;
    std::vector<LabelImpl *> rawLabels_;
    std::stack<Label *> stack_;
    int nextVariableId_ {0};
    std::vector<GateRef> arguments_;
    const CompilationConfig *ccfg_ {nullptr};
};

class Variable {
public:
    Variable(Environment *env, VariableType type, uint32_t id, GateRef value) : id_(id), type_(type), env_(env)
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    Variable(CircuitBuilder *cirbuilder, VariableType type, uint32_t id, GateRef value)
        : id_(id), type_(type), env_(cirbuilder->GetCurrentEnvironment())
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    ~Variable() = default;
    NO_MOVE_SEMANTIC(Variable);
    NO_COPY_SEMANTIC(Variable);
    void Bind(GateRef value)
    {
        currentValue_ = value;
    }
    GateRef Value() const
    {
        return currentValue_;
    }
    VariableType Type() const
    {
        return type_;
    }
    bool IsBound() const
    {
        return currentValue_ != 0;
    }
    Variable &operator=(const GateRef value)
    {
        env_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
        return *this;
    }
    GateRef operator*()
    {
        return env_->GetCurrentLabel()->ReadVariable(this);
    }
    GateRef ReadVariable()
    {
        return env_->GetCurrentLabel()->ReadVariable(this);
    }
    void WriteVariable(GateRef value)
    {
        env_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
    }
    GateRef AddPhiOperand(GateRef val);
    GateRef AddOperandToSelector(GateRef val, size_t idx, GateRef in);
    GateRef TryRemoveTrivialPhi(GateRef phi);
    uint32_t GetId() const
    {
        return id_;
    }

private:
    Circuit* GetCircuit() const
    {
        return env_->GetCircuit();
    }

    uint32_t id_;
    VariableType type_;
    GateRef currentValue_ {0};
    Environment *env_;
};

}

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
