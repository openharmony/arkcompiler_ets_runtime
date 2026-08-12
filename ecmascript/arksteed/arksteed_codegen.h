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

#ifndef ECMASCRIPT_ARKSTEED_CODEGEN_H
#define ECMASCRIPT_ARKSTEED_CODEGEN_H

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_deferred_code.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

#include <utility>
#include <vector>

namespace panda::ecmascript::arksteed {

class ArkSteedSafepointTableBuilder;
class DeoptLiteralTableBuilder;
class DeoptTranslationBuilder;
class GapMoveResolver;

class ArkSteedCodeGenerator {
public:
    ArkSteedCodeGenerator(ArkSteedAssembler *assembler, Graph *graph,
                          ArkSteedSafepointTableBuilder *safepointBuilder = nullptr,
                          DeoptTranslationBuilder *translationBuilder = nullptr,
                          DeoptLiteralTableBuilder *deoptLiteralTableBuilder = nullptr,
                          bool withColors = false)
        : assembler_(assembler),
          graph_(graph),
          safepointBuilder_(safepointBuilder),
          translationBuilder_(translationBuilder),
          deoptLiteralTableBuilder_(deoptLiteralTableBuilder),
          eagerDeoptTargetsById_(graph->GetChunk()),
          blockColorAssignment_(graph->GetChunk()),
          deferredCode_(graph->GetChunk()),
          withColors_(withColors)
    {}

    void Generate();

private:
    static int32_t ComputeFrameSize(Graph *graph);

    void ProcessValueVertex(ValueVertex *valueVertex);
    void ProcessNonControlVertex(NonControlVertex *vertex);
    void ProcessControlVertex(ControlVertex *vertex);
    void DeconstructPhisInSuccessor(BB *successor, uint32_t predecessorId);
    void CollectPhiMoves(GapMoveResolver *generalResolver, GapMoveResolver *doubleResolver,
                         BB *successor, int predecessorId,
                         ArkSteedRegList *registersSetByPhis, ArkDoubleRegList *doubleRegistersSetByPhis,
                         ChunkVector<std::pair<AllocatedState, ValueVertex *>> *constantMoves);
    void CollectRegisterStateMoves(GapMoveResolver *generalResolver, GapMoveResolver *doubleResolver,
                                   BB *successor, int predecessorId,
                                   const ArkSteedRegList &registersSetByPhis,
                                   const ArkDoubleRegList &doubleRegistersSetByPhis,
                                   ChunkVector<std::pair<AllocatedState, ValueVertex *>> *constantMoves);
    void LoadConstantToRegister(const ValueVertex *constVertex, ArkSteedRegister reg);
    void ExecuteConstantMove(const AllocatedState &dest, ValueVertex *constVertex,
                             const ArkSteedRegister *scratchGPR = nullptr,
                             const ArkSteedDoubleRegister *scratchFPR = nullptr);
    void ExecuteGapMove(const InstructionOperand &dest, const InstructionOperand &src,
                        const ArkSteedRegister *scratchGPR = nullptr,
                        const ArkSteedDoubleRegister *scratchFPR = nullptr);
    void StoreStubStackArgument(const Vertex *callVertex, int paramIdx, ArkSteedAssembler::MemoryOperand destMem);

    struct EagerDeoptTarget {
        Label label;
        DeoptId deoptId;

        explicit EagerDeoptTarget(DeoptId translationId) : deoptId(translationId) {}
    };

    Label *RecordEagerDeoptTarget(const EagerDeoptimizableMixin *vertex, kungfu::DeoptType type);
    void BranchToEagerDeoptTarget(Condition condition, const EagerDeoptimizableMixin *vertex,
                                  kungfu::DeoptType type);
    void EmitEagerDeoptExit(const EagerDeoptimizableMixin *vertex, kungfu::DeoptType type);
    void EmitQueuedEagerDeoptExits();
    void EmitEagerDeoptStackOverflow();

    int PrepareCommonStubStackArguments(const Vertex *callVertex, int argCount);
    int PrepareRuntimeStubStackArguments(const Vertex *callVertex, int argCount, int runtimeId);
    void LoadSteedExpectedArgc(ArkSteedRegister target, ArkSteedRegister expectedArgc);
    void ComputeSteedCallSlotCount(CallVertex *call, ArkSteedRegister slotCount);
    void PrepareArkSteedCall(CallVertex *call, ArkSteedRegister target, ArkSteedRegister scratch);
    void FreeArkSteedCallFrame(CallVertex *call);
    void EmitCallArkSteed(CallVertex *call, ArkSteedRegister target, ArkSteedRegister scratch, Label *exit);
    void EmitCallGeneric(CallVertex *call, ArkSteedRegister scratch);
    void EmitReturnWithPendingException();
    void EmitReturnIfPendingException();
    int PrepareTrampolineArguments(CallVertex *call, ArkSteedRegister scratch);

    template <class VertexT>
    void VisitNonControlVertex(VertexT *vertex);

    template <class VertexT>
    void VisitControlVertex(VertexT *vertex);

    ArkSteedSafepointTableBuilder *GetSafepointBuilder() const
    {
        return safepointBuilder_;
    }

    // -------------------------------------------------------------------------
    // Comment recording helpers
    // -------------------------------------------------------------------------

    void RecordComment(const char *msg);
    void RecordBlockComment(BB *block);
    void RecordVertexComment(Vertex *vertex);
    void AppendVertexSuccessorInfo(std::ostringstream *ss, Vertex *vertex);
    void RecordGapMoveComment(const InstructionOperand &src, const InstructionOperand &dest, PhiVertex *phi);
    void RecordSpillComment();
    void EmitDeferredCode();

    int ComputeDeferredBlocks();
    void ReorderDeferredBlocks(int deferredCount);
    bool IsNextBlockInLayout(BB *target) const;

    bool AllPredecessorsDeferred(BB *block) const;
    bool AllSuccessorsDeferred(BB *block);

    void ComputeBlockColors();
    void BuildBlockAdjacencyList(std::vector<std::vector<int>> *adjacentBlocks);
    void AssignBlockColors(const std::vector<std::vector<int>> &adjacentBlocks);
    int GetBlockColorIndex(int blockId) const;

    ArkSteedAssembler *assembler_;
    Graph *graph_;
    ArkSteedSafepointTableBuilder *safepointBuilder_;
    DeoptTranslationBuilder *translationBuilder_;
    DeoptLiteralTableBuilder *deoptLiteralTableBuilder_;
    ChunkVector<EagerDeoptTarget *> eagerDeoptTargetsById_;
    int currentBlockColorIndex_ = 0;
    BB *currentLayoutNextBlock_ = nullptr;

    // Block color assignment for CFG coloring (only computed when comments enabled)
    ChunkVector<int> blockColorAssignment_;
    ArkSteedDeferredCodeList deferredCode_;
    bool blockColorsComputed_ = false;
    bool withColors_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_CODEGEN_H
