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

#ifndef ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_H
#define ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_H

#include "ecmascript/arksteed/arksteed_bytecode_context.h"
#include "ecmascript/arksteed/arksteed_bytecode_iterator.h"
#include "ecmascript/arksteed/arksteed_compiler.h"
#include "ecmascript/arksteed/arksteed_framestate.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_helper.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
using namespace panda::ecmascript::kungfu;

class BytecodeAnalysis;
class ValueVertex;
class ControlVertex;

class ArkSteedGraphBuilder : public ArkSteedHelper<ArkSteedGraphBuilder> {
public:
    ArkSteedGraphBuilder(JSThread *compilerThread, uintptr_t glueAddr, Graph *graph, JitCompilationEnv *env)
        : ArkSteedHelper<ArkSteedGraphBuilder>(graph, this),
          compilerThread_(compilerThread),
          glueAddr_(glueAddr),
          env_(env),
          bytecodeContext_(graph->GetChunk()),
          mergeStates_(graph->GetChunk()),
          jumpTargets_(graph->GetChunk()),
          currentFrameState_(nullptr),
          bytecodeAnalysis_(nullptr)
    {}

    ~ArkSteedGraphBuilder() = default;

    bool Build();

    VRegIDType NumLocalVRegs() const
    {
        return numLocal_;
    }
    VRegIDType NumParamVRegs() const
    {
        return numParams_;
    }

    ValueVertex *GetActualArgc()
    {
        return NewVertexNoInput<ActualArgcVertex>();
    }
    ValueVertex *GetGlue()
    {
        return glue_;
    }
    ValueVertex *GetGlobalEnv() const
    {
        // to do: optimize
        ASSERT(currentFrameState_ != nullptr);
        ValueVertex *lexicalEnv = currentFrameState_->GetEnv();
        int32_t globalEnvOffset = static_cast<int32_t>(GlobalEnv::HEADER_SIZE);
        return const_cast<ArkSteedGraphBuilder *>(this)->NewVertex<LoadTaggedFieldVertex>({lexicalEnv},
                                                                                          globalEnvOffset);
    }

    ValueVertex *GetSharedConstPool()
    {
        ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
        int32_t methodOffset = static_cast<int32_t>(JSFunctionBase::METHOD_OFFSET);
        ValueVertex *method = NewVertex<LoadTaggedFieldVertex>({jsFunc}, methodOffset);
        int32_t constpoolOffset = static_cast<int32_t>(Method::CONSTANT_POOL_OFFSET);
        return NewVertex<LoadTaggedFieldVertex>({method}, constpoolOffset);
    }

    ValueVertex *GetModuleFromFunction()
    {
        ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
        int32_t moduleOffset = static_cast<int32_t>(JSFunction::ECMA_MODULE_OFFSET);
        return NewVertex<LoadTaggedFieldVertex>({jsFunc}, moduleOffset);
    }

    ValueVertex *GetStringFromConstPool(ValueVertex *stringId)
    {
        ValueVertex *glue = GetGlue();
        ValueVertex *constpool = GetSharedConstPool();
        return NewCommonStubCall({glue, constpool, stringId}, CommonStubCSigns::GetStringFromConstPool);
    }

    ValueVertex *GetObjectFromConstPool(ValueVertex *index)
    {
        ValueVertex *glue = GetGlue();
        ValueVertex *constpool = GetSharedConstPool();
        ValueVertex *module = GetModuleFromFunction();
        return NewCommonStubCall({glue, constpool, index, module}, CommonStubCSigns::GetObjectFromConstPool);
    }

    ValueVertex *GetMethodFromConstPool(ValueVertex *index)
    {
        ValueVertex *constpool = GetSharedConstPool();
        return NewVertex<CallRuntimeVertex>({constpool, index}, RTSTUB_ID(GetMethodFromCache));
    }

    ValueVertex *GetValueFromTaggedArray(ValueVertex *array, uint32_t index)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        return NewVertex<LoadTaggedFieldVertex>({array}, offset);
    }

    ValueVertex *SetValueToTaggedArray(ValueVertex *array, uint32_t index, ValueVertex *value)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        return NewVertex<StoreTaggedFieldVertex>({array, value}, offset);
    }

    ValueVertex *GetTaggedArrayFromValueIn(uint32_t inputSize, uint32_t startIndex = 0)
    {
        ASSERT(startIndex + inputSize <= GetInputSize());
        ValueVertex *taggedLength = NewTaggedVertexFromRawInt32(static_cast<int>(inputSize));
        ValueVertex *taggedArray = NewVertex<CallRuntimeVertex>({taggedLength}, RTSTUB_ID(NewTaggedArray));

        for (uint32_t idx = 0; idx < inputSize; ++idx) {
            ValueVertex *arg = LoadRegister(startIndex + idx);
            SetValueToTaggedArray(taggedArray, idx, arg);
        }
        return taggedArray;
    }

    ValueVertex *GetTaggedLength(uint32_t inputSize)
    {
        return NewTaggedVertexFromRawInt32(static_cast<int>(inputSize));
    }

    ValueVertex *GetLexicalEnv(ValueVertex *jsFunc)
    {
        int32_t lexEnvOffset = static_cast<int32_t>(JSFunction::LEXICAL_ENV_OFFSET);
        return NewVertex<LoadTaggedFieldVertex>({jsFunc}, lexEnvOffset);
    }

    ValueVertex *LoadRegister(int index) const
    {
        auto info = iterator_.GetCurrentBytecodeInfo();
        auto &input = info.inputs[index];
        ASSERT(std::holds_alternative<VirtualRegister>(input));
        return currentFrameState_->Get(std::get<VirtualRegister>(input));
    }

    ValueVertex *NewTaggedVertexFromRawInt32(int number)
    {
        return GetTaggedConstant(JSTaggedValue(number).GetRawData());
    }

    ICSlotIdType GetICSlotId(int index) const
    {
        auto info = iterator_.GetCurrentBytecodeInfo();
        auto &input = info.inputs[index];
        ASSERT(std::holds_alternative<ICSlotId>(input));
        return std::get<ICSlotId>(input).GetId();
    }

    uint16_t GetConstDataId(int index) const
    {
        auto info = iterator_.GetCurrentBytecodeInfo();
        auto &input = info.inputs[index];
        ASSERT(std::holds_alternative<ConstDataId>(input));
        return std::get<ConstDataId>(input).GetId();
    }

    ImmValueType GetImmediate(int index) const
    {
        auto info = iterator_.GetCurrentBytecodeInfo();
        auto &input = info.inputs[index];
        ASSERT(std::holds_alternative<Immediate>(input));
        return std::get<Immediate>(input).GetValue();
    }

    uint32_t GetInputSize() const
    {
        auto info = iterator_.GetCurrentBytecodeInfo();
        return info.inputs.size();
    }

    InterpreterFrameState *CurrentFrameState()
    {
        return currentFrameState_;
    }
    const InterpreterFrameState *CurrentFrameState() const
    {
        return currentFrameState_;
    }

    //==========================================================================
    //                              Block Completion
    //==========================================================================

    template <typename ControlVertexT, typename... Args>
    BB *FinishBlock(std::initializer_list<ValueVertex *> controlInputs, Args &&...args)
    {
        ControlVertexT *control = NewControlVertex<ControlVertexT>(controlInputs, std::forward<Args>(args)...);
        BB *block = CurrentBlock();
        GetGraph()->Add(block);
        SetCurrentBlock(nullptr);
        LOG_COMPILER(DEBUG) << "Block #" << block->GetId() << " finished";
        return block;
    }

private:
    ValueVertex *NewCallStubWithIC(const CommonStubCSigns::ID stubId, const std::vector<ValueVertex *> &args);
    void LowerCallStubWithIC(const CommonStubCSigns::ID stubId, const std::vector<ValueVertex *> &args);
    void LowerCallStubWithICPreserveAcc(const CommonStubCSigns::ID stubId, const std::vector<ValueVertex *> &args);
    ValueVertex *NewCommonStubCall(std::initializer_list<ValueVertex *> args, const CommonStubCSigns::ID stubId);
    void ValidateCommonStubCallArgs(const CommonStubCSigns::ID stubId, const std::vector<ValueVertex *> &args) const;

    void MergeCurrentFrameStateTo(BB *predecessor, uint32_t destIndex);
    void StartNewBlockWithMergeState(uint32_t index);
    void TrySplitCriticalEdge(uint32_t index, uint32_t predIndex);

    class ArkSteedSubGraphBuilder {
    public:
        class Label {
        public:
            BBRef ref_;
        };
        ArkSteedSubGraphBuilder(ArkSteedGraphBuilder *builder, int variableCount) : builder_(builder) {}

    private:
        [[maybe_unused]] ArkSteedGraphBuilder *builder_;
    };

    enum class BranchType { TRUE_BRANCH, FALSE_BRANCH };
    static inline BranchType ReverseBranchType(BranchType jumpType)
    {
        switch (jumpType) {
            case BranchType::TRUE_BRANCH:
                return BranchType::FALSE_BRANCH;
            case BranchType::FALSE_BRANCH:
                return BranchType::TRUE_BRANCH;
        }
    }

    class BranchBuilder {
    public:
        enum Mode {
            JUMP_BYTECODE_TARGET,
            JUMP_LABEL_TARGET,
        };

        struct BytecodeJumpTarget {
            BytecodeJumpTarget(uint32_t jumpTargetBcIndex, uint32_t fallthroughBcIndex)
                : jumpTargetBcIndex(jumpTargetBcIndex), fallthroughBcIndex(fallthroughBcIndex)
            {}
            uint32_t jumpTargetBcIndex;
            uint32_t fallthroughBcIndex;
        };

        struct JumpLabel {
            explicit JumpLabel(ArkSteedSubGraphBuilder::Label *jumpLabel) : jumpLabel(jumpLabel), fallthroughTarget{}
            {}
            ArkSteedSubGraphBuilder::Label *jumpLabel;
            BBRef fallthroughTarget;
        };

        union Data {
            Data(uint32_t jumpTargetBcIndex, uint32_t fallthroughBcIndex)
                : bytecodeTarget(jumpTargetBcIndex, fallthroughBcIndex)
            {}
            explicit Data(ArkSteedSubGraphBuilder::Label *jumpLabel) : labelTarget(jumpLabel) {}
            BytecodeJumpTarget bytecodeTarget;
            JumpLabel labelTarget;
        };

        BranchBuilder(ArkSteedGraphBuilder *builder, BranchType jumpType)
            : builder_(builder),
              subBuilder_(nullptr),
              jumpType_(jumpType),
              data_(builder_->iterator_.GetJumpTargetBcIndex(), builder_->iterator_.NextIndex())
        {}

        // Creates a branch builder for subgraph label.
        BranchBuilder(ArkSteedGraphBuilder *builder, ArkSteedSubGraphBuilder *subBuilder, BranchType jumpType,
                      ArkSteedSubGraphBuilder::Label *jumpLabel)
            : builder_(builder), subBuilder_(subBuilder), jumpType_(jumpType), data_(jumpLabel)
        {}

        Mode GetMode() const
        {
            return subBuilder_ == nullptr ? JUMP_BYTECODE_TARGET : JUMP_LABEL_TARGET;
        }

        BranchType GetCurrentBranchType() const
        {
            return jumpType_;
        }

        void SwapTargets()
        {
            jumpType_ = ReverseBranchType(jumpType_);
        }

        BBRef *JumpTarget();
        BBRef *FallThrough();
        BBRef *TrueTarget();
        BBRef *FalseTarget();

        template <typename ControlVertexT, typename... Args>
        void Build(std::initializer_list<ValueVertex *> inputs, Args &&...args);

    private:
        ArkSteedGraphBuilder *builder_;
        ArkSteedSubGraphBuilder *subBuilder_;
        BranchType jumpType_;
        Data data_;

        void StartFallthroughBlock(BB *predecessor);
    };

    BranchBuilder CreateBranchBuilder(BranchType jumpType)
    {
        return BranchBuilder(this, jumpType);
    }

    void BuildBranchIfTrue(BranchBuilder &builder, ValueVertex *vertex);

    void InitializeGraph();

    void InitializeGlobalsAndParameters();

    void InitializeCurrentFrameState();

    void BuildMergeStates();

    void BuildBody();

    void ValidateAfterBuilding();

    uint32_t PredecessorCount(uint32_t index) const;
    const LivenessBitSet *GetInLivenessFor(uint32_t index) const;

    void ProcessBytecode();

    // Lower method declarations - matching slowpath_lowering.cpp
    void LowerLoadString();
    void LowerLoadBigInt();
    void LowerLoadConst(const BytecodeInfo &info, kungfu::EcmaOpcode opcode);
    void LowerMoveValues(const BytecodeInfo &info);
    void LowerCreateEmptyObject();
    void LowerCreateEmptyArray();
    void LowerCreateObjectWithBuffer();
    void LowerCreateArrayWithBuffer();
    void LowerCreateObjectWithExcludedKeys();
    void LowerCreateRegExpWithLiteral();
    void LowerNewObjRange();
    void LowerAdd2();
    void LowerSub2();
    void LowerMul2();
    void LowerDiv2();
    void LowerMod2();
    void LowerExp();
    void LowerNeg();
    void LowerInc();
    void LowerDec();
    void LowerShl2();
    void LowerShr2();
    void LowerAshr2();
    void LowerAnd2();
    void LowerOr2();
    void LowerXor2();
    void LowerNot();
    void LowerEq();
    void LowerNotEq();
    void LowerLess();
    void LowerLessEq();
    void LowerGreater();
    void LowerGreaterEq();
    void LowerStrictEq();
    void LowerStrictNotEq();
    void LowerTypeOf();
    void LowerToNumber();
    void LowerToNumeric();
    void LowerIsIn();
    void LowerInstanceOf();
    void LowerTestIn();
    void LowerJumpConstant();
    void LowerJumpIfTrue();
    void LowerJumpIfFalse();
    void LowerCallArg0();
    void LowerCallArg1();
    void LowerCallArgs2();
    void LowerCallArgs3();
    void LowerCallRange();
    void LowerReturn(kungfu::EcmaOpcode opcode);
    void LowerThrow();
    void LowerLoadObjByName();
    void LowerStoreObjByName();
    void LowerLoadObjByValue();
    void LowerStoreObjByValue();
    void LowerLdObjByIndex();
    void LowerStObjByIndex();
    void LowerGetIterator();
    void LowerGetPropIterator();
    void LowerCloseIterator();
    void LowerGetNextPropName();
    void LowerGetTemplateObject();
    void LowerStoreArraySpread();
    void LowerCallThis0();
    void LowerCallThis1();
    void LowerCallThis2();
    void LowerCallThis3();
    void LowerCallThisRange();
    void LowerCallSpread();
    void LowerGetUnmappedArgs();
    void LowerCreateIterResultObj();
    void LowerTryLdGlobalByName();
    void LowerStGlobalVar();
    void LowerNewObjApply();
    void LowerThrowConstAssignment();
    void LowerThrowNotExists();
    void LowerThrowPatternNonCoercible();
    void LowerThrowIfNotObject();
    void LowerThrowUndefinedIfHole();
    void LowerThrowUndefinedIfHoleWithName();
    void LowerThrowIfSuperNotCorrectCall();
    void LowerThrowDeleteSuperProperty();
    void LowerLdSymbol();
    void LowerLdGlobal();
    void LowerDelObjProp();
    void LowerDefineMethod();
    void LowerStModuleVar();
    void LowerSetObjectWithProto();
    void LowerDynamicImport();
    void LowerLdExternalModuleVar();
    void LowerGetModuleNamespace();
    void LowerSuperCallThisRange();
    void LowerSuperCallArrowRange();
    void LowerSuperCallSpread();
    void LowerSuperCallForwardAllArgs();
    void LowerIsTrueOrFalse(bool isTrue);
    void LowerCopyDataProperties();
    void LowerStOwnByValue();
    void LowerStOwnByIndex();
    void LowerStOwnByName();
    void LowerNewLexicalEnv();
    void LowerNewLexicalEnvWithName();
    void LowerPopLexicalEnv();
    void LowerLdSuperByValue();
    void LowerStSuperByValue();
    void LowerTryStGlobalByName();
    void LowerStConstToGlobalRecord(bool isConst);
    void LowerStOwnByValueWithNameSet();
    void LowerStOwnByNameWithNameSet();
    void LowerLdGlobalVar();
    void LowerDefineGetterSetterByValue();
    void LowerLdThisByValue();
    void LowerStThisByValue();
    void LowerLdSuperByName();
    void LowerStSuperByName();
    void LowerLdLexVar();
    void LowerStLexVar();
    void LowerDefineClassWithBuffer();
    void LowerDefineFunc();
    void LowerCopyRestArgs();
    void LowerLdPatchVar();
    void LowerStPatchVar();
    void LowerLdLocalModuleVar();
    void LowerLdThisByName();
    void LowerStThisByName();
    void LowerLdPrivateProperty();
    void LowerStPrivateProperty();
    void LowerNotifyConcurrentResult();
    void LowerDefinePropertyByName();
    void LowerDefineFieldByName();
    void LowerDefineFieldByValue();
    void LowerDefineFieldByIndex();
    void LowerToPropertyKey();
    void LowerCreatePrivateProperty();
    void LowerDefinePrivateProperty();

    void BuildThrow(kungfu::RuntimeStubCSigns::ID id, ValueVertex *input);

    void ValidateNewBytecodePreprocessor();

    bool HasTryCatch() const
    {
        return bytecodeContext_.HasTryCatch();
    }

    const ExceptionInfo &GetExceptionInfo() const
    {
        return bytecodeContext_.GetExceptionInfo();
    }

    // to do: To be removed
    std::string VRegDisplayString(VirtualRegister vreg) const
    {
        return arksteed::VRegDisplayString(vreg, numLocal_, numParams_);
    }

    [[maybe_unused]] JSThread *compilerThread_;
    uintptr_t glueAddr_{0};
    JitCompilationEnv *env_;
    ValueVertex *glue_{nullptr};
    ArkSteedCompilationOptions options_;
    // to do: Huge object. Consider referencing instead of copying
    BytecodeContext bytecodeContext_;
    BytecodeIterator iterator_;
    ChunkVector<MergePointFrameState *> mergeStates_;
    ChunkVector<BBRef> jumpTargets_;
    BB *startBlock_{nullptr};

    MethodLiteral *method_{nullptr};

    InterpreterFrameState *currentFrameState_{nullptr};
    BytecodeAnalysis *bytecodeAnalysis_{nullptr};

    VRegIDType numLocal_;
    VRegIDType numParams_;
    // to do: Never changes. Can be removed.
    size_t entryPoint_ = 0;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_BUILDER_H
