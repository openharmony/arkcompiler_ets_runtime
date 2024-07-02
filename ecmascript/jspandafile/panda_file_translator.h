/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H
#define ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/bytecode_inst/old_instruction.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"

#include "libpandabase/utils/bit_field.h"
#include "libpandafile/code_data_accessor-inl.h"

namespace panda::ecmascript {
class Program;

class PUBLIC_API PandaFileTranslator {
public:
    enum FixInstructionIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

    PandaFileTranslator() = default;
    ~PandaFileTranslator() = default;
    NO_COPY_SEMANTIC(PandaFileTranslator);
    NO_MOVE_SEMANTIC(PandaFileTranslator);
    static JSHandle<Program> GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                             std::string_view entryPoint);
    static void TranslateClasses(const JSThread *thread, JSPandaFile *jsPandaFile, const CString &methodName);
    static void TranslateClass(const JSThread *thread, JSPandaFile *jsPandaFile, const CString &methodName,
        size_t methodIdx, size_t classIdx);

private:
    static JSHandle<Program> GenerateProgramInternal(EcmaVM *vm, MethodLiteral *mainMethodLiteral,
                                                     JSHandle<ConstantPool> constpool, const JSPandaFile *jsPandaFile);
    static void TranslateBytecode(JSPandaFile *jsPandaFile, uint32_t insSz, const uint8_t *insArr,
        const MethodLiteral *methodLiteral, const CString &methodName = JSPandaFile::ENTRY_FUNCTION_NAME);
    static void FixInstructionId32(const OldBytecodeInst &inst, uint32_t index, uint32_t fixOrder = 0);
    static void FixOpcode(MethodLiteral *method, const OldBytecodeInst &inst);
    static void UpdateICOffset(MethodLiteral *method, uint8_t *pc);
    static std::pair<JSHandle<ConstantPool>, JSHandle<ConstantPool>> ParseConstPool(
        EcmaVM *vm, const JSPandaFile *jsPandaFile);
    static void ParseFuncAndLiteralConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile, const CString &entryPoint,
        JSHandle<ConstantPool> sconstpool, JSHandle<ConstantPool> constpool);
    static JSHandle<ConstantPool> AllocateConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile);
    static JSHandle<ConstantPool> AllocateSharedConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile);

    friend class PatchLoader;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H
