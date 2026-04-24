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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_H

#include "ecmascript/arksteed/arksteed_bytecode_context.h"
#include "ecmascript/arksteed/arksteed_bytecode_iterator.h"
#include "ecmascript/arksteed/arksteed_bytecode_liveness.h"
#include "ecmascript/arksteed/arksteed_compiler.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

using namespace panda::ecmascript::kungfu;

class BytecodeAnalysis {
public:
    BytecodeAnalysis(Chunk *chunk, BytecodeContext &bytecodeContext, VRegIDType numLocal, VRegIDType numParams)
        : chunk_(chunk),
          bytecodeContext_(bytecodeContext),
          numLocal_(numLocal),
          numParams_(numParams),
          loopInfo_(chunk),
          loopStack_(chunk),
          loopEndIndexQueue_(chunk),
          liveness_(chunk)
    {}
    ~BytecodeAnalysis() = default;

    const ChunkVector<BytecodeInfo> &GetBytecodes() const
    {
        return bytecodeContext_.GetInfoData();
    }

    const std::vector<bool> &GetJumpLoop() const
    {
        return bytecodeContext_.GetJumpLoop();
    }

    const ChunkVector<const uint8_t *> &GetPcOffsets() const
    {
        return bytecodeContext_.GetPcOffsets();
    }

    const ChunkVector<uint32_t> &GetBcIndex() const
    {
        return bytecodeContext_.GetBcIndex();
    }

    const ExceptionInfo &GetExceptionInfo() const
    {
        return bytecodeContext_.GetExceptionInfo();
    }
    const ChunkVector<LoopInfo> &GetLoopInfo() const
    {
        return loopInfo_;
    }
    // to do: To be refactored
    const LoopInfo &GetLoopInfo(uint32_t bcIndex) const
    {
        auto it = std::find_if(loopInfo_.begin(), loopInfo_.end(), [bcIndex](const LoopInfo &info) {
            return info.HeaderIndex() == bcIndex;
        });
        ASSERT(loopInfo_.end() != it);
        return *it;
    }

    const LivenessInfo &GetLivenessInfo(uint32_t bcIndex) const
    {
        return liveness_[bcIndex];
    }
    const LivenessBitSet &GetInLiveness(uint32_t bcIndex) const
    {
        return liveness_[bcIndex].GetLiveIn();
    }
    const LivenessBitSet &GetOutLiveness(uint32_t bcIndex) const
    {
        return liveness_[bcIndex].GetLiveOut();
    }

    bool IsLogEnabled() const
    {
        return options_->enableLog;
    }

    void SetOptions(const ArkSteedCompilationOptions *options)
    {
        options_ = options;
    }

    void AnalyzeLivenessAndAssignments(BytecodeContext *bytecodeContext, const ArkSteedCompilationOptions *options);

private:
    void PushLoop(uint32_t loopHeader, uint32_t loopEnd);
    void DumpLiveness() const;
    void UpdateAssignments(LoopInfo &dest, const BytecodeIterator &iterator);
    void UpdateLiveness(const BytecodeInfo &bytecode, LivenessInfo &liveness, LivenessBitSet &nexBytecodeLiveIn,
                        BytecodeIterator iterator);
    void UpdateInLiveness(const BytecodeInfo &bytecode, LivenessInfo &liveness);
    void UpdateOutLiveness(const BytecodeInfo &bytecode, LivenessInfo &liveness, LivenessBitSet &nexBytecodeLiveIn,
                           BytecodeIterator iterator);

    Chunk *chunk_;
    BytecodeContext &bytecodeContext_;
    VRegIDType numLocal_;
    VRegIDType numParams_;
    int bytecodeCount_ = -1;
    ChunkVector<LoopInfo> loopInfo_;
    ChunkVector<int> loopStack_;
    ChunkVector<int> loopEndIndexQueue_;
    ChunkVector<LivenessInfo> liveness_;
    const ArkSteedCompilationOptions *options_ = nullptr;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_H
