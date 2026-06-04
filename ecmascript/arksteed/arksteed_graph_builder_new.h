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

#include "ecmascript/arksteed/arksteed_bytecode_analysis_new.h"
#include "ecmascript/arksteed/arksteed_bytecode_preprocessor_new.h"
#include "ecmascript/arksteed/arksteed_framestate_new.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/compiler/common_stub_csigns.h"

namespace panda::ecmascript::arksteed {
using CallSignature = kungfu::CallSignature;
using CommonStubCSigns = kungfu::CommonStubCSigns;
using RuntimeStubCSigns = kungfu::RuntimeStubCSigns;

class GraphBuilderNew {
public:
    GraphBuilderNew(Graph *destGraph,
                    uintptr_t glueAddr,
                    BytecodePreprocessorNew *preproc,
                    BytecodeAnalysisNew *analysis);

    bool Run();

    Chunk *GetChunk() const
    {
        return preproc_->GetChunk();
    }

    uint32_t GetNumLocalVRegs() const
    {
        return preproc_->GetNumLocalVRegs();
    }
    uint32_t GetNumParamVRegs() const
    {
        return preproc_->GetNumParamVRegs();
    }
    uint32_t GetNumVRegs() const
    {
        return preproc_->GetNumVRegs();
    }

private:
    using BasicBlockInfo = BytecodePreprocessorNew::BasicBlockInfo;
    using BytecodeInfo = BytecodePreprocessorNew::BytecodeInfo;
    using CommonStubID = CommonStubCSigns::ID;
    using RuntimeStubID = RuntimeStubCSigns::ID;

    struct CatchBlockInputData;
    struct BytecodeVisitor;

    void DebugLog();
    void InitializeBasicBlocks();
    void InitializeGlobalsAndParameters();
    void FinalizeStartBasicBlock(BCFrameState &frameState);
    void ProcessBasicBlock(BCFrameState &frameState, uint32_t rpoIndex);
    void ProcessCatchBlockHead(BCFrameState &frameState, uint32_t rpoIndex);
    void VisitBytecode(uint32_t rpoIndex, BCFrameState *frameState, const BytecodeInfo *bcInfo);
    void FinalizeBasicBlockRelations();

    void InitFrameState(BCFrameState &framestate, uint32_t rpoIndex);
    void InitFrameStateForLoopHeader(BCFrameState &framestate, uint32_t rpoIndex);
    void InitFrameStateForCatchBlockHeader(BCFrameState &framestate, uint32_t rpoIndex);

    void WriteBackFrameStateToLoopHeader(BCFrameState &current, uint32_t rpoIndex);
    void MergeFrameState(BCFrameState &dest, uint32_t rpoIndex, uint32_t predecessorIndex);

    uint32_t AppendCatchBlockInputs(const BCFrameState &current, uint32_t catchBlockIndex);

    template <class VertexT, class... Args>
    VertexT *NewVertex(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args);

    template <class VertexT, class... Args>
    VertexT *NewVertex(BB *owner, const ChunkVector<ValueVertex *> &inputs, Args &&...args);

    template <class VertexT, class... Args>
    VertexT *NewVertexNoInput(BB *owner, Args &&...args);

    PhiVertex *NewPhiVertex(BB *owner, uint32_t numPredecessors, VRegIDType vreg);

    template <class VertexT, class... Args>
    VertexT *NewControlVertex(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args);

    template <class VertexT>
    VertexT *FinalizeNonControlVertex(BB *owner, VertexT *vertex);

    template <class VertexT>
    VertexT *FinalizeControlVertex(BB *owner, VertexT *vertex);

    ValueVertex *GetGlue() const;
    ValueVertex *GetUndefinedValue() const;

    Graph *graph_;
    uintptr_t glueAddr_;
    BytecodePreprocessorNew *preproc_;
    BytecodeAnalysisNew *analysis_;

    // Constants frequently used
    ValueVertex *glue_ = nullptr;
    ValueVertex *undefinedValue_ = nullptr;

    ChunkVector<BB *> blocks_;
    ChunkVector<CondensedBCFrameState> frameStates_;
    ChunkVector<CatchBlockInputData *> catchBlockInputs_;
};
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_NEW_H
