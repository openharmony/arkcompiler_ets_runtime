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

#ifndef ECMASCRIPT_JSPANDAFILE_CLASS_INFO_EXTRACTOR_H
#define ECMASCRIPT_JSPANDAFILE_CLASS_INFO_EXTRACTOR_H
#include <vector>
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value_internals.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
// ClassInfoExtractor will analyze and extract the contents from class literal to keys, properties and elements(both
// non-static and static), later generate the complete hclass (both prototype and constructor) based on keys.
// Attention: keys accessor stores the property key and properties accessor stores the property value, but elements
// accessor stores the key-value pair abuttally.
using EntityId = panda_file::File::EntityId;
enum class FieldType {
    NONE = 0,
    NUMBER = (1 << 0),
    STRING = (1 << 1),
    BOOLEAN = (1 << 2),
    TS_TYPE_REF = (1 << 3),
    BIG_INT = (1 << 4),
    GENERIC = (1 << 5),
    NULL_TYPE = (1 << 6),
    UNDEFINED = (1 << 7),
};
class ClassInfoExtractor : public TaggedObject {
public:
    static constexpr uint8_t NON_STATIC_RESERVED_LENGTH = 1;
    static constexpr uint8_t STATIC_RESERVED_LENGTH = 3;
    static constexpr uint8_t SENDABLE_STATIC_RESERVED_LENGTH = 4; // add elements for sendable class

    static constexpr uint8_t CONSTRUCTOR_INDEX = 0;
    static constexpr uint8_t LENGTH_INDEX = 0;
    static constexpr uint8_t NAME_INDEX = 1;
    static constexpr uint8_t PROTOTYPE_INDEX = 2;
    static constexpr uint8_t SENDABLE_ELEMENTS_INDEX = 3;

    struct ExtractContentsDetail {
        uint32_t extractBegin;
        uint32_t extractEnd;
        uint8_t fillStartLoc;
        MethodLiteral *methodLiteral;
    };

    CAST_CHECK(ClassInfoExtractor, IsClassInfoExtractor);

    static void BuildClassInfoExtractorFromLiteral(JSThread *thread, JSHandle<ClassInfoExtractor> &extractor,
                                                   const JSHandle<TaggedArray> &literal,
                                                   uint32_t length,
                                                   ClassKind kind = ClassKind::NON_SENDABLE,
                                                   uint32_t implementLength = 0);

    static JSHandle<JSHClass> CreatePrototypeHClass(JSThread *thread,
                                                    JSHandle<TaggedArray> &keys,
                                                    JSHandle<TaggedArray> &properties);

    static JSHandle<JSHClass> CreateConstructorHClass(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                                      JSHandle<TaggedArray> &keys,
                                                      JSHandle<TaggedArray> &properties);
    static JSHandle<JSHClass> CreateSendableHClass(JSThread *thread, JSHandle<TaggedArray> &keys,
                                                   JSHandle<TaggedArray> &properties, bool isProtoClass,
                                                   uint32_t extraLength = 0);
    static void CorrectConstructorHClass(JSThread *thread,
                                         JSHandle<TaggedArray> &properties,
                                         JSHClass *constructorHClass);

    static constexpr size_t PROTOTYPE_HCLASS_OFFSET = TaggedObjectSize();
    ACCESSORS(NonStaticKeys, PROTOTYPE_HCLASS_OFFSET, NON_STATIC_PROPERTIES_OFFSET)
    ACCESSORS(NonStaticProperties, NON_STATIC_PROPERTIES_OFFSET, NON_STATIC_ELEMENTS_OFFSET)
    ACCESSORS(NonStaticElements, NON_STATIC_ELEMENTS_OFFSET, CONSTRUCTOR_HCLASS_OFFSET)
    ACCESSORS(StaticKeys, CONSTRUCTOR_HCLASS_OFFSET, STATIC_PROPERTIES_OFFSET)
    ACCESSORS(StaticProperties, STATIC_PROPERTIES_OFFSET, STATIC_ELEMENTS_OFFSET)
    ACCESSORS(StaticElements, STATIC_ELEMENTS_OFFSET, CONSTRUCTOR_METHOD_OFFSET)
    ACCESSORS(ConstructorMethod, CONSTRUCTOR_METHOD_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t NON_STATIC_BITS = 1;
    static constexpr size_t STATIC_BITS = 1;
    FIRST_BIT_FIELD(BitField, NonStaticWithElements, bool, NON_STATIC_BITS)
    NEXT_BIT_FIELD(BitField, StaticWithElements, bool, STATIC_BITS, NonStaticWithElements)

    DECL_VISIT_OBJECT(PROTOTYPE_HCLASS_OFFSET, BIT_FIELD_OFFSET)

    DECL_DUMP()

private:
    static bool ExtractAndReturnWhetherWithElements(JSThread *thread, const JSHandle<TaggedArray> &literal,
                                                    const ExtractContentsDetail &detail,
                                                    JSHandle<TaggedArray> &keys, JSHandle<TaggedArray> &properties,
                                                    JSHandle<TaggedArray> &elements,
                                                    const JSPandaFile *jsPandaFile);
};

enum class ClassPropertyType : uint8_t { NON_STATIC = 0, STATIC };

class ClassHelper {
public:
    static JSHandle<JSFunction> DefineClassFromExtractor(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                                         JSHandle<ClassInfoExtractor> &extractor,
                                                         const JSHandle<JSTaggedValue> &lexenv);

    static JSHandle<JSFunction> DefineClassWithIHClass(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                                       JSHandle<ClassInfoExtractor> &extractor,
                                                       const JSHandle<JSTaggedValue> &lexenv,
                                                       const JSHandle<JSTaggedValue> &prototypeOrHClassVal,
                                                       const JSHandle<JSTaggedValue> &constructorHClassVal);

    static bool PUBLIC_API MatchFieldType(SharedFieldType fieldType, JSTaggedValue value);
    static CString StaticFieldTypeToString(uint32_t fieldType);
private:
    static JSHandle<NameDictionary> BuildDictionaryProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                                              JSHandle<TaggedArray> &keys,
                                                              JSHandle<TaggedArray> &properties, ClassPropertyType type,
                                                              const JSHandle<JSTaggedValue> &lexenv);

    static JSHandle<JSFunction> CreateJSFunctionFromTemplate(JSThread *thread,
                                                             const JSHandle<FunctionTemplate> &funcTemp,
                                                             const JSHandle<JSObject> &homeObject,
                                                             const JSHandle<JSTaggedValue> &lexenv);

    static void HandleElementsProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                         const JSHandle<JSTaggedValue> &lexenv, JSHandle<TaggedArray> &elements);
};

class SendableClassDefiner : public ClassHelper {
public:
    static JSHandle<JSFunction> DefineSendableClassFromExtractor(JSThread *thread,
                                                                 JSHandle<ClassInfoExtractor> &extractor,
                                                                 const JSHandle<TaggedArray> &fieldTypeArray);

    static void DefineSendableInstanceHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
                                             uint32_t length,
                                             const JSHandle<JSFunction> &ctor, const JSHandle<JSTaggedValue> &base);

    static JSHandle<TaggedArray> ExtractStaticFieldTypeArray(JSThread *thread,
                                                             const JSHandle<TaggedArray> &fieldTypeArray);

    static void FilterDuplicatedKeys(JSThread *thread, const JSHandle<TaggedArray> &keys,
                                     const JSHandle<TaggedArray> &properties);

    static SharedFieldType FromTaggedValue(JSTaggedValue value)
    {
        if (value.IsNull()) {
            return SharedFieldType::NONE;
        } else if (value.IsNumber()) {
            return SharedFieldType::NUMBER;
        } else if (value.IsString()) {
            return SharedFieldType::STRING;
        } else if (value.IsBoolean()) {
            return SharedFieldType::BOOLEAN;
        } else if (value.IsJSSharedObject()) {
            return SharedFieldType::SENDABLE;
        } else {
            return SharedFieldType::NONE;
        }
    }

    static void PUBLIC_API AddFieldTypeToHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
                                                uint32_t length, const JSHandle<NameDictionary> &nameDict,
                                                const JSHandle<JSHClass> &hclass,
                                                JSHandle<NumberDictionary> &elementsDic, bool &hasElement);

    static void PUBLIC_API AddFieldTypeToHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
        uint32_t length, const JSHandle<LayoutInfo> &layout, const JSHandle<JSHClass> &hclass,
        JSHandle<NumberDictionary> &elementsDic, bool &hasElement, size_t start,
        std::vector<JSHandle<JSTaggedValue>> &&propertyList = std::vector<JSHandle<JSTaggedValue>>());
private:
    static SharedFieldType FromFieldType(FieldType type)
    {
        return SharedFieldType(static_cast<uint32_t>(type));
    }

    static JSHandle<NameDictionary> BuildSendableDictionaryProperties(JSThread *thread,
                                                                      const JSHandle<JSObject> &object,
                                                                      JSHandle<TaggedArray> &keys,
                                                                      JSHandle<TaggedArray> &properties,
                                                                      ClassPropertyType type,
                                                                      const JSHandle<JSFunction> &ctor);

    static JSHandle<JSFunction> CreateSFunctionFromTemplate(JSThread *thread,
                                                            const JSHandle<FunctionTemplate> &funcTemp,
                                                            const JSHandle<JSObject> &homeObject,
                                                            const JSHandle<JSFunction> &ctor);

    static void UpdateAccessorFunction(JSThread *thread, const JSMutableHandle<JSTaggedValue> &value,
                                       const JSHandle<JSTaggedValue> &homeObject, const JSHandle<JSFunction> &ctor);

    static void AddFieldTypeToDict(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
                                   uint32_t length, JSMutableHandle<NameDictionary> &dict,
                                   JSHandle<NumberDictionary> &elementsDic, bool &hasElement,
                                   PropertyAttributes attributes = PropertyAttributes::Default(true, true, false));

    static bool TryUpdateExistValue(JSThread *thread, JSMutableHandle<JSTaggedValue> &existValue,
                                    JSMutableHandle<JSTaggedValue> &value);

    static void TryUpdateValue(JSThread *thread, JSMutableHandle<JSTaggedValue> &value);

    static void UpdateValueToAccessor(JSThread *thread, JSMutableHandle<JSTaggedValue> &value,
                                      JSHandle<AccessorData> &accessor);
    static std::pair<uint32_t, uint32_t> GetSizeAndMaxInlineByType(JSType type);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_CLASS_INFO_EXTRACTOR_H
