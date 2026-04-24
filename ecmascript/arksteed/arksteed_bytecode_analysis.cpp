/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_bytecode_analysis.h"

#include "ecmascript/arksteed/arksteed_bytecode_iterator.h"
#include "ecmascript/compiler/bytecodes.h"

namespace panda::ecmascript::arksteed {

void BytecodeAnalysis::PushLoop(uint32_t loopHeader, uint32_t loopEnd)
{
    loopEndIndexQueue_.push_back(loopEnd);
    loopInfo_.emplace_back(chunk_, loopHeader, loopEnd, numLocal_, numParams_);
    loopStack_.push_back(static_cast<int>(loopInfo_.size() - 1));
}

void BytecodeAnalysis::AnalyzeLivenessAndAssignments(BytecodeContext *bytecodeContext,
                                                     const ArkSteedCompilationOptions *options)
{
    SetOptions(options);
    const auto &bytecodes = GetBytecodes();
    const auto &jumpLoop = GetJumpLoop();
    const auto &pcOffsets = GetPcOffsets();

    ASSERT(bytecodes.size() > 0);
    BytecodeIterator iterator(bytecodeContext);
    bytecodeCount_ = bytecodes.size();
    loopStack_.clear();
    loopEndIndexQueue_.clear();
    liveness_.clear();
    loopStack_.push_back(-1);

    std::generate_n(std::back_inserter(liveness_), bytecodeCount_, [this] {
        return LivenessInfo(chunk_, numLocal_, numParams_);
    });
    LivenessBitSet nexBytecodeLiveIn(chunk_, numLocal_, numParams_);

    // 1. analysis loop
    for (iterator.GotoEnd(); !iterator.Done(); --iterator) {
        auto &bytecodeInfo = bytecodes[iterator.Index()];
        uint32_t index = iterator.Index();
        if (jumpLoop[index]) {
            uint32_t loopHeader = iterator.GetJumpTargetBcIndex();
            uint32_t loopEnd = index;
            PushLoop(loopHeader, loopEnd);
        }
        int inLoop = loopStack_.size() > 1 && (!jumpLoop[index] || iterator.GetJumpTargetBcIndex() == index);
        if (inLoop) {
            for (size_t i = 1; i < loopStack_.size(); i++) {
                LoopInfo &curLoop = loopInfo_[loopStack_[i]];
                UpdateAssignments(curLoop, iterator);
            }
            LoopInfo &curInnermostLoop = loopInfo_[loopStack_.back()];
            if (curInnermostLoop.HeaderIndex() == index) {
                loopStack_.pop_back();
            }
        }
        UpdateLiveness(bytecodes[index], liveness_[index], nexBytecodeLiveIn, iterator);
    }

    ASSERT(loopStack_.size() == 1);
    ASSERT(loopStack_.back() == -1);

    for (uint32_t loopEndIndex : loopEndIndexQueue_) {
        iterator.Goto(loopEndIndex);
        uint32_t headerIndex = iterator.GetJumpTargetBcIndex();
        uint32_t endIndex = iterator.Index();
        LivenessInfo &headerLiveness = liveness_[headerIndex];
        LivenessInfo &endLiveness = liveness_[endIndex];

        // Live variable set unchanged, continue propagation
        // The back edge propagates loop_header.in info back to loop_end.out
        if (!endLiveness.GetLiveOut().UnionWithChanged(headerLiveness.GetLiveIn())) {
            continue;
        }
        endLiveness.GetLiveIn().CopyFrom(endLiveness.GetLiveOut());
        nexBytecodeLiveIn.CopyFrom(endLiveness.GetLiveIn());

        --iterator;
        for (; iterator.Index() != headerIndex; --iterator) {
            uint32_t index = iterator.Index();
            UpdateLiveness(bytecodes[index], liveness_[index], nexBytecodeLiveIn, iterator);
        }
        UpdateOutLiveness(bytecodes[iterator.Index()], headerLiveness, nexBytecodeLiveIn, iterator);
    }

    // 3. Print liveness results
    if (IsLogEnabled()) {
        DumpLiveness();
    }
}

void BytecodeAnalysis::UpdateAssignments(LoopInfo &dest, const BytecodeIterator &iterator)
{
    const BytecodeInfo &info = iterator.GetCurrentBytecodeInfo();
    if (info.AccOut()) {
        dest.AddDef(VRegOfAcc(numLocal_, numParams_));
    }
    for (VRegIDType out : info.vregOut) {
        dest.AddDef(VirtualRegister{out});
    }
}

void BytecodeAnalysis::UpdateLiveness(const BytecodeInfo &bytecode, LivenessInfo &liveness,
                                      LivenessBitSet &nextBytecodeLiveIn, BytecodeIterator iterator)
{
    UpdateOutLiveness(bytecode, liveness, nextBytecodeLiveIn, iterator);
    UpdateInLiveness(bytecode, liveness);
    nextBytecodeLiveIn.CopyFrom(liveness.GetLiveIn());
}

// in[i] = use[i] ∪ (out[i] \ def[i])
void BytecodeAnalysis::UpdateInLiveness(const BytecodeInfo &bytecode, LivenessInfo &liveness)
{
    // Initially let in[i] <- out[i]
    liveness.GetLiveIn().CopyFrom(liveness.GetLiveOut());
    // def[i], variable kill
    if (bytecode.AccOut()) {
        liveness.GetLiveIn().ClearAcc();
    }
    for (const auto &out : bytecode.vregOut) {
        liveness.GetLiveIn().Clear(out);
    }

    // use[i], variable use
    if (bytecode.AccIn()) {
        liveness.GetLiveIn().SetAcc();
    }
    for (size_t i = 0; i < bytecode.inputs.size(); i++) {
        auto in = bytecode.inputs[i];
        if (std::holds_alternative<VirtualRegister>(in)) {
            auto vreg = std::get<VirtualRegister>(in).GetId();
            liveness.GetLiveIn().Set(vreg);
        }
    }
}

/*
 out[i] = ⋃ in[next]                    // fall-through
          ⋃ in[jump_target]             // forward jump
          ⋃ in[handler]                 // exception handler
*/
void BytecodeAnalysis::UpdateOutLiveness(const BytecodeInfo &bytecode, LivenessInfo &liveness,
                                         LivenessBitSet &nexBytecodeLiveIn, BytecodeIterator iterator)
{
    const auto &jumpLoop = GetJumpLoop();
    const auto &exceptionInfo = GetExceptionInfo();

    uint32_t currentIndex = iterator.Index();
    if (!bytecode.IsJumpImm() && !bytecode.IsReturn() && !bytecode.IsThrow()) {
        liveness.GetLiveOut().Union(liveness_[currentIndex + 1].GetLiveIn());
    }

    // forwardJump
    if (bytecode.IsJump() && !jumpLoop[currentIndex]) {
        int targetIndex = iterator.GetJumpTargetBcIndex();
        liveness.GetLiveOut().Union(liveness_[targetIndex].GetLiveIn());
    }

    // Exception handling
    // If bytecode has external side effects (may throw exceptions), consider exception handler liveness
    // In Ark, the throw instruction's accumulator carries the exception value to the handler
    if (bytecode.NoSideEffects() || exceptionInfo.empty()) {
        return;
    }
    
    // Binary search for the try block containing the current bytecode
    // Find the last ExceptionItem where startBcIndex <= currentIndex
    auto it = std::upper_bound(
        exceptionInfo.begin(),
        exceptionInfo.end(),
        currentIndex,
        [](int value, const ExceptionItem &item) { return value < static_cast<int>(item.startBcIndex); });

    if (it == exceptionInfo.begin()) {
        return;
    }
    --it;
    const ExceptionItem &exItem = *it;

    // Check if currentIndex is within [startBcIndex, endBcIndex)
    if (currentIndex < static_cast<uint32_t>(exItem.startBcIndex) ||
        currentIndex >= static_cast<uint32_t>(exItem.endBcIndex)) {
        return;
    }
    
    // 1. First record whether the accumulator was originally live
    bool wasAccumulatorLive = liveness.GetLiveOut().TestAcc();

    // 2. For each catch block, union its in-liveness into out-liveness
    for (uint32_t catchBcIndex : exItem.catchBcIndices) {
        if (catchBcIndex < liveness_.size()) {
            liveness.GetLiveOut().Union(liveness_[catchBcIndex].GetLiveIn());
        }
    }

    // 3. If only the catch makes the accumulator live, clear it
    // This is because the accumulator is overwritten by the exception value when entering the handler
    if (!wasAccumulatorLive) {
        liveness.GetLiveOut().ClearAcc();
    }
}

void BytecodeAnalysis::DumpLiveness() const
{
    LOG_COMPILER(INFO) << "========== Liveness Analysis Result ==========";
    LOG_COMPILER(INFO) << "numLocal = " << numLocal_ << ", numParams = " << numParams_;

    const auto &bytecodes = GetBytecodes();
    std::ostringstream out;
    auto appendVReg = [this, &out](VirtualRegister reg, bool *first) {
        if (!*first) {
            out << ", ";
        }
        *first = false;
        if (reg.GetId() < numLocal_) {
            out << 'v' << reg.GetId();
        } else if (reg.GetId() < numLocal_ + numParams_) {
            out << 'a' << reg.GetId() - numLocal_;
        } else if (reg == VRegOfEnv(numLocal_, numParams_)) {
            out << "env";
        } else if (reg == VRegOfAcc(numLocal_, numParams_)) {
            out << "acc";
        } else {
            UNREACHABLE();
        }
    };
    for (size_t i = 0; i < bytecodes.size(); i++) {
        out << "BC[" << i << "]: " << GetEcmaOpcodeStr(bytecodes[i].GetMetaData().GetOpcode()) << '\n';
        out << "  LiveIn: {";

        bool first = true;
        for (VRegIDType j = 0; j < NumVRegs(numLocal_, numParams_); j++) {
            if (liveness_[i].GetLiveIn().Test(j)) {
                appendVReg(VirtualRegister{j}, &first);
            }
        }
        out << "}\n  LiveOut: {";
        first = true;
        for (VRegIDType j = 0; j < NumVRegs(numLocal_, numParams_); j++) {
            if (liveness_[i].GetLiveOut().Test(j)) {
                appendVReg(VirtualRegister{j}, &first);
            }
        }
        out << "}\n";
    }
    LOG_COMPILER(INFO) << out.str() << "==============================================";
}

}  // namespace panda::ecmascript::arksteed
