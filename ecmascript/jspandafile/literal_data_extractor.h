/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSPANDAFILE_LITERAL_DATA_EXTRACTOR_H
#define ECMASCRIPT_JSPANDAFILE_LITERAL_DATA_EXTRACTOR_H

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "libpandafile/literal_data_accessor-inl.h"

namespace panda::ecmascript {
class LiteralDataExtractor {
public:
    LiteralDataExtractor() = default;
    virtual ~LiteralDataExtractor() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(LiteralDataExtractor);
    DEFAULT_COPY_SEMANTIC(LiteralDataExtractor);

    static constexpr uint32_t LITERALARRAY_VALUE_LOW_BOUNDARY = 100;

    // Parameter 'size_t index' is for old isa, and 'EntityId id' for new isa.
    static void ExtractObjectDatas(JSThread *thread, const JSPandaFile *jsPandaFile, size_t index,
                                   JSMutableHandle<TaggedArray> elements, JSMutableHandle<TaggedArray> properties,
                                   JSHandle<ConstantPool> constpool, const CString &entryPoint = "");
    static PUBLIC_API void ExtractObjectDatas(JSThread *thread, const JSPandaFile *jsPandaFile, EntityId id,
                                              JSMutableHandle<TaggedArray> elements,
                                              JSMutableHandle<TaggedArray> properties,
                                              JSHandle<ConstantPool> constpool, const CString &entryPoint = "",
                                              bool isLoadedAOT = false,
                                              JSHandle<AOTLiteralInfo> entryIndexes = JSHandle<AOTLiteralInfo>());

    static JSHandle<TaggedArray> GetDatasIgnoreType(JSThread *thread, const JSPandaFile *jsPandaFile, size_t index,
                                                    JSHandle<ConstantPool> constpool, const CString &entryPoint = "");
    static PUBLIC_API JSHandle<TaggedArray> GetDatasIgnoreType(JSThread *thread, const JSPandaFile *jsPandaFile,
        EntityId id, JSHandle<ConstantPool> constpool, const CString &entryPoint = "",
        bool isLoadedAOT = false, JSHandle<AOTLiteralInfo> entryIndexes = JSHandle<AOTLiteralInfo>(),
        ElementsKind *newKind = nullptr, JSHandle<JSTaggedValue> sendableEnv = JSHandle<JSTaggedValue>(),
        ClassKind classKind = ClassKind::NON_SENDABLE);
    static PUBLIC_API JSHandle<TaggedArray> GetDatasIgnoreTypeForClass(JSThread *thread, const JSPandaFile *jsPandaFile,
        EntityId id, JSHandle<ConstantPool> constpool, const CString &entryPoint = "",
        bool isLoadedAOT = false, JSHandle<AOTLiteralInfo> entryIndexes = JSHandle<AOTLiteralInfo>(),
        ElementsKind *newKind = nullptr, JSHandle<JSTaggedValue> sendableEnv = JSHandle<JSTaggedValue>(),
        ClassKind classKind = ClassKind::NON_SENDABLE);

    static JSHandle<TaggedArray> GetDatasIgnoreTypeForClass(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                            size_t index, JSHandle<ConstantPool> constpool,
                                                            const CString &entryPoint = "");

    static JSHandle<JSFunction> DefineMethodInLiteral(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                      uint32_t offset, JSHandle<ConstantPool> constpool,
                                                      FunctionKind kind, uint16_t length,
                                                      const CString &entryPoint = "",
                                                      bool isLoadedAOT = false, uint32_t entryIndex = 0,
                                                      JSHandle<JSTaggedValue> sendableEnv = JSHandle<JSTaggedValue>(),
                                                      ClassKind classKind = ClassKind::NON_SENDABLE);

    static void PUBLIC_API GetMethodOffsets(const JSPandaFile *jsPandaFile, size_t index,
                                            std::vector<uint32_t> &methodOffsets);

    static void PUBLIC_API GetMethodOffsets(const JSPandaFile *jsPandaFile, EntityId id,
                                            std::vector<uint32_t> &methodOffsets);
private:
    static JSHandle<FunctionTemplate> DefineFunctionTemplate(JSThread *thread, const JSPandaFile *jsPandaFile,
        uint32_t offset, JSHandle<ConstantPool> constpool, FunctionKind kind, uint16_t length,
        const CString &entryPoint = "", bool isLoadedAOT = false, uint32_t entryIndex = 0,
        JSHandle<JSTaggedValue> sendableEnv = JSHandle<JSTaggedValue>(), ClassKind classKind = ClassKind::NON_SENDABLE);

    static JSHandle<JSFunction> CreateJSFunctionInLiteral(EcmaVM *vm, const JSHandle<Method> &method,
                                                          FunctionKind kind, ClassKind classKind);
    static JSHandle<TaggedArray> EnumerateLiteralVals(JSThread *thread, panda_file::LiteralDataAccessor &lda,
        const JSPandaFile *jsPandaFile, size_t index, JSHandle<ConstantPool> constpool,
        const CString &entryPoint = "");
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_LITERAL_DATA_EXTRACTOR_H
