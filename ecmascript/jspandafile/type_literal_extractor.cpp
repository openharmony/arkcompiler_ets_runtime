/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/type_literal_extractor.h"

#include "libpandafile/literal_data_accessor-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
using LiteralTag = panda_file::LiteralTag;
using LiteralValue = panda_file::LiteralDataAccessor::LiteralValue;
using StringData = panda_file::StringData;
using EntityId = panda_file::File::EntityId;
using LiteralDataAccessor = panda_file::LiteralDataAccessor;

static constexpr const char *TYPE_ANNO_RECORD_NAME = "L_ESTypeAnnotation;";

static bool IsLegalOffset(uint32_t offset)
{
    return offset != 0U;
}

static uint32_t GetMethodAnnoOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset, const char *annoName)
{
    const panda_file::File &pf = *jsPandaFile->GetPandaFile();
    EntityId methodId(methodOffset);
    panda_file::MethodDataAccessor mda(pf, methodId);

    uint32_t annoOffset = 0;
    mda.EnumerateAnnotations([&jsPandaFile, &annoName, &pf, &annoOffset](EntityId annotationId) {
        panda_file::AnnotationDataAccessor ada(pf, annotationId);
        auto *annotationName = reinterpret_cast<const char *>(jsPandaFile->GetStringData(ada.GetClassId()).data);
        ASSERT(annotationName != nullptr);
        if (::strcmp(TYPE_ANNO_RECORD_NAME, annotationName) != 0) {
            return;
        }
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elemName = reinterpret_cast<const char *>(jsPandaFile->GetStringData(adae.GetNameId()).data);
            ASSERT(elemName != nullptr);

            if (::strcmp(annoName, elemName) != 0) {
                continue;
            }

            panda_file::ScalarValue sv = adae.GetScalarValue();
            annoOffset = sv.GetValue();
        }
    });
    return annoOffset;
}

TypeLiteralExtractor::TypeLiteralExtractor(const JSPandaFile *jsPandaFile, const uint32_t typeOffset)
    : typeOffset_(typeOffset)
{
    ProcessTypeLiteral(jsPandaFile, typeOffset);
}

void TypeLiteralExtractor::ProcessTypeLiteral(const JSPandaFile *jsPandaFile, const uint32_t typeOffset)
{
    EntityId literalId(typeOffset);
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    bool isFirst = true;
    lda.EnumerateLiteralVals(literalId,
        [this, &jsPandaFile, &isFirst](const LiteralValue &value, const LiteralTag &tag) {
            if (isFirst) {
                uint32_t kindValue = std::get<uint32_t>(value);
                if (UNLIKELY(!IsVaildKind(kindValue))) {
                    LOG_COMPILER(FATAL) << "Load type literal failure.";
                    return;
                }
                kind_ = static_cast<TSTypeKind>(std::get<uint32_t>(value));
                isFirst = false;
                return;
            }
            switch (tag) {
                case LiteralTag::INTEGER: {
                    uint32_t valueValue = std::get<uint32_t>(value);
                    if (static_cast<int32_t>(valueValue) < 0) {
                        isGenerics_ = true;
                    }
                    array_.emplace_back(valueValue);
                    break;
                }
                case LiteralTag::LITERALARRAY: {
                    array_.emplace_back(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::BUILTINTYPEINDEX: {
                    array_.emplace_back(static_cast<uint32_t>(std::get<uint8_t>(value)));
                    break;
                }
                case LiteralTag::STRING: {
                    StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                    CString stringValue = utf::Mutf8AsCString(sd.data);
                    array_.emplace_back(stringValue);
                    break;
                }
                default: {
                    LOG_COMPILER(FATAL) << "TypeLiteral should not exist LiteralTag: " << static_cast<uint32_t>(tag);
                    break;
                }
            }
        });
}

TypeSummaryExtractor::TypeSummaryExtractor(const JSPandaFile *jsPandaFile, const CString &recordName)
{
    ProcessTypeSummary(jsPandaFile, jsPandaFile->GetTypeSummaryOffset(recordName));
}

void TypeSummaryExtractor::ProcessTypeSummary(const JSPandaFile *jsPandaFile, const uint32_t summaryOffset)
{
    EntityId summaryId(summaryOffset);
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    bool isFirstIndex = true;
    lda.EnumerateLiteralVals(summaryId,
        [this, &isFirstIndex, &jsPandaFile](const LiteralValue &value, const LiteralTag &tag) {
            switch (tag) {
                case LiteralTag::LITERALARRAY: {
                    typeOffsets_.emplace_back(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::BUILTINTYPEINDEX: {
                    typeOffsets_.emplace_back(static_cast<uint32_t>(std::get<uint8_t>(value)));
                    break;
                }
                case LiteralTag::INTEGER: {
                    if (isFirstIndex) {
                        numOfTypes_ = std::get<uint32_t>(value);
                        typeOffsets_.emplace_back(numOfTypes_);
                        isFirstIndex = false;
                    } else {
                        numOfRedirects_ = std::get<uint32_t>(value);
                    }
                    break;
                }
                case LiteralTag::STRING: {
                    StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                    CString stringValue = utf::Mutf8AsCString(sd.data);
                    reDirects_.emplace_back(stringValue);
                    break;
                }
                default: {
                    LOG_COMPILER(FATAL) << "TypeSummary should not exist LiteralTag: " << static_cast<uint32_t>(tag);
                    break;
                }
            }
        });
    ASSERT(typeOffsets_.size() == numOfTypes_ + 1);
    ASSERT(reDirects_.size() == numOfRedirects_);
}

TypeAnnotationExtractor::TypeAnnotationExtractor(const JSPandaFile *jsPandaFile, const uint32_t methodOffset)
{
    ProcessTypeAnnotation(jsPandaFile, methodOffset);
}

void TypeAnnotationExtractor::ProcessTypeAnnotation(const JSPandaFile *jsPandaFile, const uint32_t methodOffset)
{
    uint32_t annoOffset = GetMethodAnnoOffset(jsPandaFile, methodOffset, TYPE_ANNO_ELEMENT_NAME);
    if (!IsLegalOffset(annoOffset)) {
        return;
    }

    EntityId annoId(annoOffset);
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    lda.EnumerateLiteralVals(annoId, [this](const LiteralValue &value, const LiteralTag &tag) {
        switch (tag) {
            case LiteralTag::INTEGER: {
                int32_t valueValue = base::bit_cast<int32_t>(std::get<uint32_t>(value));
                bcOffsets_.emplace_back(valueValue);
                break;
            }
            case LiteralTag::LITERALARRAY: {
                typeIds_.emplace_back(std::get<uint32_t>(value));
                break;
            }
            case LiteralTag::BUILTINTYPEINDEX: {
                typeIds_.emplace_back(static_cast<uint32_t>(std::get<uint8_t>(value)));
                break;
            }
            default: {
                LOG_COMPILER(FATAL) << "TypeAnnotation should not exist LiteralTag: " << static_cast<uint32_t>(tag);
                break;
            }
        }
    });
    ASSERT(bcOffsets_.size() == typeIds_.size());
}

ExportTypeTableExtractor::ExportTypeTableExtractor(const JSPandaFile *jsPandaFile,
                                                   const CString &recordName,
                                                   bool isBuiltinTable)
{
    ProcessExportTable(jsPandaFile, recordName, isBuiltinTable);
}

void ExportTypeTableExtractor::ProcessExportTable(const JSPandaFile *jsPandaFile,
                                                  const CString &recordName,
                                                  bool isBuiltinTable)
{
    const char *name = isBuiltinTable ? DECLARED_SYMBOL_TYPES : EXPORTED_SYMBOL_TYPES;
    uint32_t methodOffset = jsPandaFile->GetMainMethodIndex(recordName);
    uint32_t annoOffset = GetMethodAnnoOffset(jsPandaFile, methodOffset, name);
    if (!IsLegalOffset(annoOffset)) {
        return;
    }

    EntityId annoId(annoOffset);
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    lda.EnumerateLiteralVals(annoId, [this, &jsPandaFile](const LiteralValue &value, const LiteralTag &tag) {
        switch (tag) {
            case LiteralTag::LITERALARRAY: {
                typeIds_.emplace_back(std::get<uint32_t>(value));
                break;
            }
            case LiteralTag::BUILTINTYPEINDEX: {
                typeIds_.emplace_back(static_cast<uint32_t>(std::get<uint8_t>(value)));
                break;
            }
            case LiteralTag::STRING: {
                StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                exportVars_.emplace_back(utf::Mutf8AsCString(sd.data));
                break;
            }
            default: {
                LOG_COMPILER(FATAL) << "TypeExportTable should not exist LiteralTag: " << static_cast<uint32_t>(tag);
                break;
            }
        }
    });
    ASSERT(exportVars_.size() == typeIds_.size());
}
}  // namespace panda::ecmascript
