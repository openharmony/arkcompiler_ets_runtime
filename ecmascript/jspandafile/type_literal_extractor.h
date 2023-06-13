/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JSPANDAFILE_TYPE_LITERAL_EXTRACTOR_H
#define ECMASCRIPT_JSPANDAFILE_TYPE_LITERAL_EXTRACTOR_H

#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
class TypeLiteralExtractor {
using TypeLiteralValue = std::variant<uint32_t, CString>;

public:
    explicit TypeLiteralExtractor(const JSPandaFile *jsPandaFile, const uint32_t typeOffset);
    ~TypeLiteralExtractor() = default;
    NO_COPY_SEMANTIC(TypeLiteralExtractor);
    NO_MOVE_SEMANTIC(TypeLiteralExtractor);

    inline bool IsVaildTypeLiteral() const
    {
        return !array_.empty();
    }

    inline TSTypeKind GetTypeKind() const
    {
        return kind_;
    }

    inline bool IsGenerics() const
    {
        return isGenerics_;
    }

    inline uint32_t GetIntValue(const uint32_t index) const
    {
        ASSERT(index < array_.size());
        auto t = array_[index];
        ASSERT(std::holds_alternative<uint32_t>(t));
        return std::get<uint32_t>(t);
    }

    inline const CString &GetStringValue(const uint32_t index) const
    {
        ASSERT(index < array_.size());
        const auto &t = array_[index];
        ASSERT(std::holds_alternative<CString>(t));
        return std::get<CString>(t);
    }

    inline uint32_t GetTypeOffset() const
    {
        return typeOffset_;
    }

    template <class Callback>
    void EnumerateElements(const uint32_t numIndex, const Callback &callback)
    {
        ASSERT(numIndex < array_.size());
        uint32_t length = std::get<uint32_t>(array_[numIndex]);
        ASSERT(numIndex + length < array_.size());
        for (uint32_t i = 1; i <= length; i++) {
            uint32_t value = GetIntValue(numIndex + i);
            callback(value);
        }
    }

    template <class Callback>
    void EnumerateTypesWithIntKey(const uint32_t numIndex, const Callback &callback)
    {
        ASSERT(numIndex < array_.size());
        const uint32_t gap = 2;
        uint32_t length = std::get<uint32_t>(array_[numIndex]);
        ASSERT(numIndex + length * gap < array_.size());
        for (uint32_t i = 0; i < length; i++) {
            uint32_t keyIndex = numIndex + i * gap + KEY_OFFSET;
            uint32_t valueIndex = numIndex + i * gap + VALUE_OFFSET;
            uint32_t key = GetIntValue(keyIndex);
            uint32_t value = GetIntValue(valueIndex);
            callback(key, value);
        }
    }

    template <class Callback>
    void EnumerateProperties(const uint32_t numIndex, const uint32_t gap, const Callback &callback)
    {
        ASSERT(numIndex < array_.size());
        ASSERT(gap >= VALUE_OFFSET);
        uint32_t length = std::get<uint32_t>(array_[numIndex]);
        ASSERT(numIndex + length * gap < array_.size());
        for (uint32_t i = 0; i < length; i++) {
            uint32_t keyIndex = numIndex + i * gap + KEY_OFFSET;
            uint32_t valueIndex = numIndex + i * gap + VALUE_OFFSET;
            const CString &key = GetStringValue(keyIndex);
            uint32_t value = GetIntValue(valueIndex);
            callback(key, value);
        }
    }

    void Print() const;

private:
    static constexpr uint32_t KEY_OFFSET = 1;
    static constexpr uint32_t VALUE_OFFSET = 2;

    inline bool IsVaildKind(const uint32_t kindValue) const
    {
        return (static_cast<uint32_t>(TSTypeKind::TYPEKIND_FIRST) <= kindValue) &&
               (kindValue <= static_cast<uint32_t>(TSTypeKind::TYPEKIND_LAST));
    }

    void ProcessTypeLiteral(const JSPandaFile *jsPandaFile, const uint32_t typeOffset);
    std::string PrintTypeKind(TSTypeKind typeKind) const;

    std::vector<TypeLiteralValue> array_;
    uint32_t typeOffset_ { 0 };
    TSTypeKind kind_ { TSTypeKind::UNKNOWN };
    bool isGenerics_ { false };
};

class TypeSummaryExtractor {
public:
    explicit TypeSummaryExtractor(const JSPandaFile *jsPandaFile, const CString &recordName);
    ~TypeSummaryExtractor() = default;
    NO_COPY_SEMANTIC(TypeSummaryExtractor);
    NO_MOVE_SEMANTIC(TypeSummaryExtractor);

    template <class Callback>
    void EnumerateTypeOffsets(const uint32_t lastIndex, const Callback &callback)
    {
        ASSERT(lastIndex < typeOffsets_.size());
        for (uint32_t i = 0; i <= lastIndex; i++) {
            callback(typeOffsets_[i]);
        }
    }

    inline uint32_t GetNumOfTypes() const
    {
        return numOfTypes_;
    }

    void Print() const;

private:
    void ProcessTypeSummary(const JSPandaFile *jsPandaFile, const uint32_t summaryOffset);

    const JSPandaFile *jsPandaFile_ {nullptr};
    std::vector<uint32_t> typeOffsets_ {};
    std::vector<CString> reDirects_ {};
    uint32_t numOfTypes_ { 0 };
    uint32_t numOfRedirects_ { 0 };
};

class TypeAnnotationExtractor {
using LiteralTag = panda_file::LiteralTag;

public:
    explicit TypeAnnotationExtractor(const JSPandaFile *jsPandaFile, const uint32_t methodOffset);
    ~TypeAnnotationExtractor() = default;
    NO_COPY_SEMANTIC(TypeAnnotationExtractor);
    NO_MOVE_SEMANTIC(TypeAnnotationExtractor);

    template <class Callback>
    void EnumerateInstsAndTypes(const Callback &callback)
    {
        ASSERT(bcOffsets_.size() == typeIds_.size());
        uint32_t length = bcOffsets_.size();
        for (uint32_t i = 0; i < length; i++) {
            callback(bcOffsets_[i], typeIds_[i]);
        }
    }
    
    uint32_t GetMethodTypeOffset() const
    {
        return methodTypeOffset_;
    }

    bool IsNamespace() const
    {
        return isNamespace_;
    }

    void Print() const;

private:
    static constexpr const char *TYPE_ANNO_ELEMENT_NAME = "_TypeOfInstruction";
    static constexpr int METHOD_ANNOTATION_FUNCTION_TYPE_OFFSET = -1;
    static constexpr int METHOD_ANNOTATION_NAMESPACE = 255;
    static constexpr int METHOD_ANNOTATION_ENUM = 254;

    void ProcessTypeAnnotation(const JSPandaFile *jsPandaFile, const uint32_t methodOffset);
    void CollectTSMethodKind();
    std::string PrintTag(LiteralTag tag) const;

    uint32_t methodTypeOffset_ {0};
    std::vector<int32_t> bcOffsets_ {};
    std::vector<uint32_t> typeIds_ {};
    std::vector<LiteralTag> tags_ {};
    bool isNamespace_ {false};
};

class ExportTypeTableExtractor {
public:
    explicit ExportTypeTableExtractor(const JSPandaFile *jsPandaFile, const CString &recordName, bool isBuiltinTable);
    ~ExportTypeTableExtractor() = default;
    NO_COPY_SEMANTIC(ExportTypeTableExtractor);
    NO_MOVE_SEMANTIC(ExportTypeTableExtractor);

    inline uint32_t GetLength() const
    {
        ASSERT(exportVars_.size() == typeIds_.size());
        return exportVars_.size() + typeIds_.size();
    }

    template <class Callback>
    void EnumerateModuleTypes(const Callback &callback)
    {
        ASSERT(exportVars_.size() == typeIds_.size());
        uint32_t length = exportVars_.size();
        for (uint32_t i = 0; i < length; i++) {
            callback(exportVars_[i], typeIds_[i]);
        }
    }

    void Print() const;

private:
    static constexpr const char *DECLARED_SYMBOL_TYPES = "declaredSymbolTypes";
    static constexpr const char *EXPORTED_SYMBOL_TYPES = "exportedSymbolTypes";

    void ProcessExportTable(const JSPandaFile *jsPandaFile, const CString &recordName, bool isBuiltinTable);

    std::vector<CString> exportVars_ {};
    std::vector<uint32_t> typeIds_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_TYPE_LITERAL_EXTRACTOR_H
