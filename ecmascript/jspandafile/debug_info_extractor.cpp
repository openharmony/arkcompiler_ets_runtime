/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/jspandafile/debug_info_extractor.h"

#include "libpandafile/debug_data_accessor-inl.h"
#include "libpandafile/line_number_program.h"

namespace panda::ecmascript {
using panda::panda_file::ClassDataAccessor;
using panda::panda_file::DebugInfoDataAccessor;
using panda::panda_file::File;
using panda::panda_file::LineNumberProgramItem;
using panda::panda_file::LineProgramState;
using panda::panda_file::LineNumberProgramProcessor;
using panda::panda_file::MethodDataAccessor;
using panda::panda_file::ProtoDataAccessor;

static const char *GetStringFromConstantPool(const panda_file::File &pf, uint32_t offset)
{
    return reinterpret_cast<const char *>(pf.GetStringData(panda_file::File::EntityId(offset)).data);
}

class LineNumberProgramHandler {
public:
    explicit LineNumberProgramHandler(LineProgramState *state) : state_(state) {}
    ~LineNumberProgramHandler() = default;

    NO_COPY_SEMANTIC(LineNumberProgramHandler);
    NO_MOVE_SEMANTIC(LineNumberProgramHandler);

    LineProgramState *GetState() const
    {
        return state_;
    }

    void ProcessBegin()
    {
        lnt_.push_back({state_->GetAddress(), static_cast<int32_t>(state_->GetLine())});
    }

    void ProcessEnd()
    {
        // When process ends, update any variableInfo
        // with end_offset = 0, set it to the state address.
        for (auto iter = lvt_.begin(); iter != lvt_.end(); iter++) {
            if (iter->endOffset == 0) {
                iter->endOffset = state_->GetAddress();
            }
        }
    }

    bool HandleAdvanceLine(int32_t lineDiff) const
    {
        state_->AdvanceLine(lineDiff);
        return true;
    }

    bool HandleAdvancePc(uint32_t pcDiff) const
    {
        state_->AdvancePc(pcDiff);
        return true;
    }

    bool HandleSetFile(uint32_t sourceFileId) const
    {
        state_->SetFile(sourceFileId);
        return true;
    }

    bool HandleSetSourceCode(uint32_t sourceCodeId) const
    {
        state_->SetSourceCode(sourceCodeId);
        return true;
    }

    bool HandleSetPrologueEnd() const
    {
        return true;
    }

    bool HandleSetEpilogueBegin() const
    {
        return true;
    }

    bool HandleStartLocal(int32_t regNumber, uint32_t nameId, [[maybe_unused]] uint32_t typeId)
    {
        // start_offset is the current state address, end_offset will temporarily be 0 here,
        // then being updated inside the HandleEndLocal method.
        uint32_t startOffset = state_->GetAddress();
        uint32_t endOffset = 0;
        const char *name = GetStringFromConstantPool(state_->GetPandaFile(), nameId);
        lvt_.push_back({name, regNumber, startOffset, endOffset});
        return true;
    }

    bool HandleStartLocalExtended(int32_t regNumber, uint32_t nameId, [[maybe_unused]] uint32_t typeId,
                                  [[maybe_unused]] uint32_t typeSignatureId)
    {
        uint32_t startOffset = state_->GetAddress();
        uint32_t endOffset = 0;
        const char *name = GetStringFromConstantPool(state_->GetPandaFile(), nameId);
        lvt_.push_back({name, regNumber, startOffset, endOffset});
        return true;
    }

    bool HandleEndLocal([[maybe_unused]] int32_t regNumber)
    {
        for (auto iter = lvt_.rbegin(); iter != lvt_.rend(); iter++) {
            // reversely finds the variable and updates its end_offset to be state address
            if (iter->regNumber == regNumber && iter->endOffset == 0) {
                iter->endOffset = state_->GetAddress();
                break;
            }
        }
        return true;
    }

    bool HandleSetColumn(int32_t columnNumber)
    {
        state_->SetColumn(columnNumber);
        cnt_.push_back({state_->GetAddress(), static_cast<int32_t>(state_->GetColumn())});
        return true;
    }

    bool HandleSpecialOpcode(uint32_t pcOffset, int32_t lineOffset)
    {
        state_->AdvancePc(pcOffset);
        state_->AdvanceLine(lineOffset);
        lnt_.push_back({state_->GetAddress(), static_cast<int32_t>(state_->GetLine())});
        return true;
    }

    LineNumberTable GetLineNumberTable() const
    {
        return lnt_;
    }

    LocalVariableTable GetLocalVariableTable() const
    {
        return lvt_;
    }

    ColumnNumberTable GetColumnNumberTable() const
    {
        return cnt_;
    }

    const uint8_t *GetFile() const
    {
        return state_->GetFile();
    }

    const uint8_t *GetSourceCode() const
    {
        return state_->GetSourceCode();
    }

private:
    using Opcode = LineNumberProgramItem::Opcode;

    LineProgramState *state_;
    LineNumberTable lnt_;
    LocalVariableTable lvt_;
    ColumnNumberTable cnt_;
};

const LineNumberTable &DebugInfoExtractor::GetLineNumberTable(const panda_file::File::EntityId methodId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    static const LineNumberTable EMPTY_LINE_TABLE {};

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        if (!ExtractorMethodDebugInfo(methodId)) {
            return EMPTY_LINE_TABLE;
        }
        return methods_[methodId.GetOffset()].lineNumberTable;
    }
    ASSERT(iter != methods_.end());
    return iter->second.lineNumberTable;
}

const ColumnNumberTable &DebugInfoExtractor::GetColumnNumberTable(const panda_file::File::EntityId methodId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    static const ColumnNumberTable EMPTY_COLUMN_TABLE {};

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        if (!ExtractorMethodDebugInfo(methodId)) {
            return EMPTY_COLUMN_TABLE;
        }
        return methods_[methodId.GetOffset()].columnNumberTable;
    }
    ASSERT(iter != methods_.end());
    return iter->second.columnNumberTable;
}

const LocalVariableTable &DebugInfoExtractor::GetLocalVariableTable(const panda_file::File::EntityId methodId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    static const LocalVariableTable EMPTY_VARIABLE_TABLE {};

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        if (!ExtractorMethodDebugInfo(methodId)) {
            return EMPTY_VARIABLE_TABLE;
        }
        return methods_[methodId.GetOffset()].localVariableTable;
    }
    ASSERT(iter != methods_.end());
    return iter->second.localVariableTable;
}

const std::string &DebugInfoExtractor::GetSourceFile(const panda_file::File::EntityId methodId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    static const std::string sourceFile = "";

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        if (!ExtractorMethodDebugInfo(methodId)) {
            return sourceFile;
        }
        return methods_[methodId.GetOffset()].sourceFile;
    }
    ASSERT(iter != methods_.end());
    return iter->second.sourceFile;
}

const std::string &DebugInfoExtractor::GetSourceCode(const panda_file::File::EntityId methodId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    static const std::string sourceCode = "";

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        if (!ExtractorMethodDebugInfo(methodId)) {
            return sourceCode;
        }
        return methods_[methodId.GetOffset()].sourceCode;
    }
    ASSERT(iter != methods_.end());
    return iter->second.sourceCode;
}

bool DebugInfoExtractor::ExtractorMethodDebugInfo(const panda_file::File::EntityId methodId)
{
    if (!methodId.IsValid()) {
        return false;
    }

    uint32_t offset = methodId.GetOffset();
    if (offset >= jsPandaFile_->GetFileSize()) {
        return false;
    }

    auto &pandaFile = *jsPandaFile_->GetPandaFile();
    MethodDataAccessor mda(pandaFile, methodId);
    auto classId = mda.GetClassId();
    ASSERT(classId.IsValid() && !pandaFile.IsExternal(classId));
    ClassDataAccessor cda(pandaFile, classId);
    auto sourceFileId = cda.GetSourceFileId();
    auto debugInfoId = mda.GetDebugInfoId();
    if (!debugInfoId) {
        return false;
    }

    ExtractorMethodDebugInfo(pandaFile, sourceFileId, debugInfoId, offset);
    return true;
}

void DebugInfoExtractor::ExtractorMethodDebugInfo(const panda_file::File &pandaFile,
                                                  const std::optional<panda_file::File::EntityId> sourceFileId,
                                                  const std::optional<panda_file::File::EntityId> debugInfoId,
                                                  uint32_t offset)
{
    DebugInfoDataAccessor dda(pandaFile, debugInfoId.value());
    const uint8_t *program = dda.GetLineNumberProgram();
    LineProgramState state(pandaFile, sourceFileId.value_or(panda_file::File::EntityId(0)), dda.GetLineStart(),
                            dda.GetConstantPool());

    LineNumberProgramHandler handler(&state);
    LineNumberProgramProcessor<LineNumberProgramHandler> programProcessor(program, &handler);
    programProcessor.Process();

    const char *sourceFile = "";
    if (state.HasFile()) {
        sourceFile = reinterpret_cast<const char *>(handler.GetFile());
    }

    const char *sourceCode = "";
    if (state.HasSourceCode()) {
        sourceCode = reinterpret_cast<const char *>(handler.GetSourceCode());
    }

    MethodDebugInfo methodDebugInfo = {sourceFile, sourceCode, handler.GetLineNumberTable(),
        handler.GetColumnNumberTable(), handler.GetLocalVariableTable()};
    methods_.emplace(offset, std::move(methodDebugInfo));
}

void DebugInfoExtractor::Extract()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto &pandaFile = *jsPandaFile_->GetPandaFile();
    auto classes = jsPandaFile_->GetClasses();

    for (size_t i = 0; i < classes.Size(); i++) {
        panda_file::File::EntityId classId(classes[i]);
        if (!classId.IsValid() || jsPandaFile_->IsExternal(classId)) {
            continue;
        }

        ClassDataAccessor cda(pandaFile, classId);
        auto sourceFileId = cda.GetSourceFileId();

        cda.EnumerateMethods([&](MethodDataAccessor &mda) {
            panda_file::File::EntityId methodId = mda.GetMethodId();
            if (!methodId.IsValid()) {
                return;
            }

            uint32_t offset = methodId.GetOffset();
            if (offset >= jsPandaFile_->GetFileSize()) {
                return;
            }

            auto iter = methods_.find(offset);
            if (iter != methods_.end()) {
                return;
            }

            auto debugInfoId = mda.GetDebugInfoId();
            if (!debugInfoId) {
                return;
            }

            ExtractorMethodDebugInfo(pandaFile, sourceFileId, debugInfoId, offset);
        });
    }
}
}  // namespace panda::ecmascript
