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

#ifndef ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_NEW_H
#define ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_NEW_H

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_bc_frame_state.h"
#include "ecmascript/arksteed/arksteed_bytecode_analysis.h"
#include "ecmascript/arksteed/arksteed_bytecode_preprocessor.h"
#include "ecmascript/arksteed/arksteed_pgo_context.h"
#include "ecmascript/compiler/common_stub_csigns.h"

namespace panda::ecmascript::arksteed {
class CompileInfoFacts;
class Graph;

class GraphBuilder {
public:
    GraphBuilder(JSThread *compilerThread,
                 Graph *destGraph,
                 uintptr_t glueAddr,
                 BytecodePreprocessor *preproc,
                 BytecodeAnalysis *analysis);

    bool Run();

private:
    using BasicBlockInfo = BytecodePreprocessor::BasicBlockInfo;
    using CommonStubID = kungfu::CommonStubCSigns::ID;
    using RuntimeStubID = kungfu::RuntimeStubCSigns::ID;

    struct CatchBlockInputData;
    struct BytecodeVisitor;

    VRegIDType LexicalEnvIndex() const
    {
        return VRegOfLexicalEnv(numLocal_, numParams_);
    }
    VRegIDType AccIndex() const
    {
        return VRegOfAcc(numLocal_, numParams_);
    }

    JSRuntimeOptions *GetOptions() const
    {
        return &preproc_->GetEnv()->GetJSOptions();
    }

    bool IsExceptionLazyDeoptEnabled() const
    {
        return GetOptions()->IsEnableJitLazyDeopt();
    }

    void DebugLog();
    void InitializeStartBlock(SharedBCFrameState frameState);
    void ProcessDeadBasicBlock(uint32_t rpoIndex);
    void ProcessBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex);
    void ProcessCatchBlockHead(SharedBCFrameState frameState, uint32_t rpoIndex);
    void VisitBytecodesOfBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex);

    void InitFrameState(SharedBCFrameState framestate, uint32_t rpoIndex);
    void InitFrameStateForLoopHeader(SharedBCFrameState framestate, uint32_t rpoIndex);
    void InitFrameStateForCatchBlockHeader(SharedBCFrameState framestate, uint32_t rpoIndex);

    void InitCompileInfoFacts(uint32_t rpoIndex);
    void InitCompileInfoFactsForCatchBlock(uint32_t rpoIndex);
    void WriteBackFrameStateToLoopHeader(SharedBCFrameState current, uint32_t rpoIndex);
    void MergeFrameState(SharedBCFrameState dest, uint32_t rpoIndex, uint32_t predRpoIndex,
                         uint32_t actualPredIndex, uint32_t actualNumPreds);

    PhiVertex *NewPhiVertex(BB *owner, uint32_t numPredecessors, VRegIDType vreg);
    template <class InputRange = std::initializer_list<ValueVertex *>>
    PhiVertex *NewPhiVertexWith(BB *owner, const InputRange &inputs, VRegIDType vreg);

    // VertexT should be neither control vertex nor Phi
    template <class VertexT, class InputRange = std::initializer_list<ValueVertex *>, class... Args>
    VertexT *NewVertex(BB *owner, const InputRange &inputs, Args &&...args);

    template <class VertexT, class InputRange = std::initializer_list<ValueVertex *>, class... Args>
    VertexT *NewVertex(CompileInfoFacts *compileInfoFacts, BB *owner, const InputRange &inputs, Args &&...args);

    JumpVertex *FinishBlockWithJump(BB *owner, BB *target);
    JumpLoopVertex *FinishBlockWithJumpLoop(BB *owner, BB *target);

    [[deprecated("Use FinishBlockWithBranch<BranchVertexT>() instead")]]
    ControlVertex *FinishBlockWithBranch(BB *owner, ValueVertex *input, BB *targetIfTrue, BB *targetIfFalse);

    template <class BranchVertexT, class... Args>
    BranchVertexT *FinishBlockWithBranch(BB *owner, std::initializer_list<ValueVertex *> input,
                                         BB *targetIfTrue, BB *targetIfFalse, Args &&...args);

    // VertexT should be control vertex
    template <class VertexT, class... Args>
    VertexT *FinishBlockWith(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args);

    BB *NewBlock();
    BB *ActivateNonCatchBlock(uint32_t rpoIndex);
    BB *ActivateCatchBlock(CatchBlockInputData **inputData, uint32_t rpoIndex);
    LoadTaggedFieldVertex *ActivateGlobalEnv();

    Graph *graph_;
    JSThread *compilerThread_;
    uintptr_t glueAddr_;
    BytecodePreprocessor *preproc_;
    BytecodeAnalysis *analysis_;

    ArkSteedPGOContext pgoContext_;

    // Frequently used fields. Cached for performance.
    uint32_t numLocal_;   // Equivalent to preproc_->GetNumLocalRegs()
    uint32_t numParams_;  // Equivalent to preproc_->GetNumParamRegs()
    Chunk *chunk_;        // Equivalent to preproc_->GetChunk()

    // Constants frequently used
    ValueVertex *glue_ = nullptr;
    ValueVertex *undefinedValue_ = nullptr;
    InitialValueVertex *initialLexicalEnv_ = nullptr;
    LoadTaggedFieldVertex *lazyGlobalEnv_ = nullptr;

    ChunkVector<BB *> blocks_;
    ChunkVector<CondensedBCFrameState> frameStates_;
    ChunkVector<CompileInfoFacts *> compileInfoFacts_;
    ChunkVector<CatchBlockInputData *> catchBlockInputs_;
};
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_NEW_H
