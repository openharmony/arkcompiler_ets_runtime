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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_CONTEXT_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_CONTEXT_H

#include "ecmascript/arksteed/arksteed_bytecode_preprocessor.h"
#include "ecmascript/arksteed/arksteed_compiler.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/chunk_containers.h"
#include "libpandafile/bytecode_instruction.h"

namespace panda::ecmascript::arksteed {

struct ExceptionItem {
    uint32_t startBcIndex;
    uint32_t endBcIndex;
    std::vector<uint32_t> catchBcIndices;

    ExceptionItem(uint32_t startBcIndex, uint32_t endBcIndex, std::vector<uint32_t> catchBcIndices)
        : startBcIndex(startBcIndex), endBcIndex(endBcIndex), catchBcIndices(catchBcIndices)
    {}
};
using ExceptionInfo = ChunkVector<ExceptionItem>;

class BytecodeContext {
public:
    static constexpr uint32_t INVALID_BC_INDEX = UINT32_MAX;

    explicit BytecodeContext(Chunk *chunk)
        : chunk_(chunk),
          infoData_(chunk),
          maxLoopEndIndex_(chunk),
          jumpTargetIndex_(chunk),
          pcOffsets_(chunk),
          bcIndex_(chunk),
          exceptionInfo_(chunk),
          preprocessor_(chunk)
    {}
    ~BytecodeContext() = default;

    void Initialize(MethodLiteral *method, JitCompilationEnv *env, const ArkSteedCompilationOptions *options);

    Bytecodes &GetBytecodes()
    {
        return bytecodes_;
    }

    const Bytecodes &GetBytecodes() const
    {
        return bytecodes_;
    }

    ChunkVector<BytecodeInfo> &GetInfoData()
    {
        return infoData_;
    }

    const ChunkVector<BytecodeInfo> &GetInfoData() const
    {
        return infoData_;
    }

    std::vector<bool> &GetJumpLoop()
    {
        return jumpLoop_;
    }

    const std::vector<bool> &GetJumpLoop() const
    {
        return jumpLoop_;
    }

    std::vector<ChunkVector<uint32_t> *> &GetLoopEndIndex()
    {
        return loopEndIndex_;
    }

    const std::vector<ChunkVector<uint32_t> *> &GetLoopEndIndex() const
    {
        return loopEndIndex_;
    }

    ChunkVector<uint32_t> &GetMaxLoopEndIndex()
    {
        return maxLoopEndIndex_;
    }

    const ChunkVector<uint32_t> &GetMaxLoopEndIndex() const
    {
        return maxLoopEndIndex_;
    }

    ChunkVector<const uint8_t *> &GetPcOffsets()
    {
        return pcOffsets_;
    }

    const ChunkVector<const uint8_t *> &GetPcOffsets() const
    {
        return pcOffsets_;
    }

    ChunkVector<uint32_t> &GetBcIndex()
    {
        return bcIndex_;
    }

    const ChunkVector<uint32_t> &GetBcIndex() const
    {
        return bcIndex_;
    }

    const uint8_t *GetBytecodeStart() const
    {
        return bytecodeStart_;
    }

    uint32_t GetPcOffset(const uint8_t *pc) const
    {
        return static_cast<uint32_t>(pc - bytecodeStart_);
    }

    size_t GetBytecodeCount() const
    {
        return infoData_.size();
    }

    MethodLiteral *GetMethod() const
    {
        return method_;
    }

    const ExceptionInfo &GetExceptionInfo() const
    {
        return exceptionInfo_;
    }

    const ChunkVector<uint32_t> &GetPredecessorCount() const
    {
        return preprocessor_.GetPredecessorCount();
    }

    bool HasTryCatch() const
    {
        return hasTryCatch_;
    }

    bool IsLogEnabled() const
    {
        return options_->enableLog;
    }

    void SetOptions(const ArkSteedCompilationOptions *options)
    {
        options_ = options;
    }

    // Get the preprocessor for advanced access
    const BytecodePreprocessor &GetPreprocessor() const
    {
        return preprocessor_;
    }

    uint32_t GetJumpTargetBcIndex(uint32_t bcIndex) const
    {
        ASSERT(jumpTargetIndex_[bcIndex] != INVALID_BC_INDEX);
        return jumpTargetIndex_[bcIndex];
    }

private:
    void CollectPcOffsets();
    void CollectJumpTargets();
    void BuildBytecodeInfo(size_t envVregIdx);
    void JumpLoopAnalysis();
    bool NeedInsertJumpForLoop(uint32_t loopHeaderIndex) const;
    void RebuildBytecodeWithJumpRedirection();
    void CollectTryCatchBlockInfo(JitCompilationEnv *env);

    Bytecodes bytecodes_;
    Chunk *chunk_;
    ChunkVector<BytecodeInfo> infoData_;
    std::vector<bool> jumpLoop_;
    std::vector<ChunkVector<uint32_t> *> loopEndIndex_;
    ChunkVector<uint32_t> maxLoopEndIndex_;
    ChunkVector<uint32_t> jumpTargetIndex_;
    ChunkVector<const uint8_t *> pcOffsets_;
    ChunkVector<uint32_t> bcIndex_;
    const uint8_t *bytecodeStart_ = nullptr;
    uint32_t bytecodeSize_ = 0;
    MethodLiteral *method_ = nullptr;
    const ArkSteedCompilationOptions *options_ = nullptr;
    ExceptionInfo exceptionInfo_;
    bool hasTryCatch_ = false;

    // Preprocessor handles BB construction and CFG
    BytecodePreprocessor preprocessor_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_CONTEXT_H
