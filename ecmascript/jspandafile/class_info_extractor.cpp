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

#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
void ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(JSThread *thread, JSHandle<ClassInfoExtractor> &extractor,
                                                            const JSHandle<TaggedArray> &literal,
                                                            ClassKind kind)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t literalBufferLength = literal->GetLength();
    // non static properties number is hidden in the last index of Literal buffer
    uint32_t nonStaticNum = 0;
    if (literalBufferLength != 0) {
        nonStaticNum = static_cast<uint32_t>(literal->Get(thread, literalBufferLength - 1).GetInt());
    }

    // Reserve sufficient length to prevent frequent creation.
    JSHandle<TaggedArray> nonStaticKeys;
    JSHandle<TaggedArray> nonStaticProperties;
        factory->NewOldSpaceTaggedArray(nonStaticNum + NON_STATIC_RESERVED_LENGTH);
    if (kind == ClassKind::SENDABLE) {
        nonStaticKeys = factory->NewSOldSpaceTaggedArray(nonStaticNum + NON_STATIC_RESERVED_LENGTH);
        nonStaticProperties =
            factory->NewSOldSpaceTaggedArray(nonStaticNum + NON_STATIC_RESERVED_LENGTH);
    } else {
        nonStaticKeys = factory->NewOldSpaceTaggedArray(nonStaticNum + NON_STATIC_RESERVED_LENGTH);
        nonStaticProperties =
            factory->NewOldSpaceTaggedArray(nonStaticNum + NON_STATIC_RESERVED_LENGTH);
    }

    nonStaticKeys->Set(thread, CONSTRUCTOR_INDEX, globalConst->GetConstructorString());
    Method *method = Method::Cast(extractor->GetConstructorMethod().GetTaggedObject());
    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    EntityId methodId = method->GetMethodId();
    if (nonStaticNum) {
        ExtractContentsDetail nonStaticDetail {0, nonStaticNum * 2, NON_STATIC_RESERVED_LENGTH, nullptr};

        JSHandle<TaggedArray> nonStaticElements = factory->EmptyArray();
        if (UNLIKELY(ExtractAndReturnWhetherWithElements(thread, literal, nonStaticDetail, nonStaticKeys,
                                                         nonStaticProperties, nonStaticElements, jsPandaFile))) {
            extractor->SetNonStaticWithElements(true);
            extractor->SetNonStaticElements(thread, nonStaticElements);
        }
    }

    extractor->SetNonStaticKeys(thread, nonStaticKeys);
    extractor->SetNonStaticProperties(thread, nonStaticProperties);

    uint32_t staticNum = literalBufferLength == 0 ? 0 : (literalBufferLength - 1) / 2 - nonStaticNum;

    // Reserve sufficient length to prevent frequent creation.
    JSHandle<TaggedArray> staticKeys;
    JSHandle<TaggedArray> staticProperties;
    if (kind == ClassKind::SENDABLE) {
        staticKeys = factory->NewSOldSpaceTaggedArray(staticNum + STATIC_RESERVED_LENGTH);
        staticProperties = factory->NewSOldSpaceTaggedArray(staticNum + STATIC_RESERVED_LENGTH);
    } else {
        staticKeys = factory->NewOldSpaceTaggedArray(staticNum + STATIC_RESERVED_LENGTH);
        staticProperties = factory->NewOldSpaceTaggedArray(staticNum + STATIC_RESERVED_LENGTH);
    }

    staticKeys->Set(thread, LENGTH_INDEX, globalConst->GetLengthString());
    staticKeys->Set(thread, NAME_INDEX, globalConst->GetNameString());
    staticKeys->Set(thread, PROTOTYPE_INDEX, globalConst->GetPrototypeString());

    JSHandle<TaggedArray> staticElements = factory->EmptyArray();

    if (staticNum) {
        ExtractContentsDetail staticDetail {
            nonStaticNum * 2,
            literalBufferLength - 1,
            STATIC_RESERVED_LENGTH,
            methodLiteral
        };
        if (UNLIKELY(ExtractAndReturnWhetherWithElements(thread, literal, staticDetail, staticKeys,
                                                         staticProperties, staticElements, jsPandaFile))) {
            extractor->SetStaticWithElements(true);
            extractor->SetStaticElements(thread, staticElements);
        }
    } else {
        // without static properties, set class name
        std::string clsName = MethodLiteral::ParseFunctionName(jsPandaFile, methodId);
        JSHandle<EcmaString> clsNameHandle = factory->NewFromStdString(clsName);
        staticProperties->Set(thread, NAME_INDEX, clsNameHandle);
    }

    // set prototype internal accessor
    JSHandle<JSTaggedValue> prototypeAccessor = globalConst->GetHandledFunctionPrototypeAccessor();
    staticProperties->Set(thread, PROTOTYPE_INDEX, prototypeAccessor);

    extractor->SetStaticKeys(thread, staticKeys);
    extractor->SetStaticProperties(thread, staticProperties);
}

bool ClassInfoExtractor::ExtractAndReturnWhetherWithElements(JSThread *thread, const JSHandle<TaggedArray> &literal,
                                                             const ExtractContentsDetail &detail,
                                                             JSHandle<TaggedArray> &keys,
                                                             JSHandle<TaggedArray> &properties,
                                                             JSHandle<TaggedArray> &elements,
                                                             const JSPandaFile *jsPandaFile)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    ASSERT(keys->GetLength() == properties->GetLength() && elements->GetLength() == 0);

    uint32_t pos = detail.fillStartLoc;
    bool withElementsFlag = false;
    bool isStaticFlag = (detail.methodLiteral != nullptr);
    bool keysHasNameFlag = false;

    JSHandle<JSTaggedValue> nameString = globalConst->GetHandledNameString();
    JSMutableHandle<JSTaggedValue> firstValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> secondValue(thread, JSTaggedValue::Undefined());
    for (uint32_t index = detail.extractBegin; index < detail.extractEnd; index += 2) {  // 2: key-value pair
        firstValue.Update(literal->Get(index));
        secondValue.Update(literal->Get(index + 1));
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(firstValue), "Key is not a property key");

        if (LIKELY(firstValue->IsString())) {
            if (isStaticFlag && !keysHasNameFlag && JSTaggedValue::SameValue(firstValue, nameString)) {
                properties->Set(thread, NAME_INDEX, secondValue);
                keysHasNameFlag = true;
                continue;
            }

            // front-end can do better: write index in class literal directly.
            uint32_t elementIndex = 0;
            if (JSTaggedValue::StringToElementIndex(firstValue.GetTaggedValue(), &elementIndex)) {
                ASSERT(elementIndex < JSObject::MAX_ELEMENT_INDEX);
                uint32_t elementsLength = elements->GetLength();
                elements =
                    TaggedArray::SetCapacityInOldSpace(thread, elements, elementsLength + 2); // 2: key-value pair
                elements->Set(thread, elementsLength, firstValue);
                elements->Set(thread, elementsLength + 1, secondValue);
                withElementsFlag = true;
                continue;
            }
        }

        keys->Set(thread, pos, firstValue);
        properties->Set(thread, pos, secondValue);
        pos++;
    }

    if (isStaticFlag) {
        if (LIKELY(!keysHasNameFlag)) {
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            EntityId methodId = detail.methodLiteral->GetMethodId();
            std::string clsName = MethodLiteral::ParseFunctionName(jsPandaFile, methodId);
            JSHandle<EcmaString> clsNameHandle = factory->NewFromStdString(clsName);
            properties->Set(thread, NAME_INDEX, clsNameHandle);
        } else {
            // class has static name property, reserved length bigger 1 than actual, need trim
            uint32_t trimOneLength = keys->GetLength() - 1;
            keys->Trim(thread, trimOneLength);
            properties->Trim(thread, trimOneLength);
        }
    }

    if (UNLIKELY(withElementsFlag)) {
        ASSERT(pos + elements->GetLength() / 2 == properties->GetLength());  // 2: half
        keys->Trim(thread, pos);
        properties->Trim(thread, pos);
    }

    return withElementsFlag;
}

JSHandle<JSHClass> ClassInfoExtractor::CreatePrototypeHClass(JSThread *thread,
                                                             JSHandle<TaggedArray> &keys,
                                                             JSHandle<TaggedArray> &properties)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t length = keys->GetLength();
    JSHandle<JSHClass> hclass;
    if (LIKELY(length <= PropertyAttributes::MAX_FAST_PROPS_CAPACITY)) {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(length, MemSpaceType::OLD_SPACE, GrowMode::KEEP);
        for (uint32_t index = 0; index < length; ++index) {
            key.Update(keys->Get(index));
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);  // non-enumerable

            if (UNLIKELY(properties->Get(index).IsAccessor())) {
                attributes.SetIsAccessor(true);
            }

            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::TAGGED);
            attributes.SetOffset(index);
            layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
        }

        hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, length);
        // Not need set proto here
        hclass->SetLayout(thread, layout);
        hclass->SetNumberOfProps(length);
    } else {
        // dictionary mode
        hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, 0);  // without in-obj
        hclass->SetIsDictionaryMode(true);
        hclass->SetNumberOfProps(0);
    }

    hclass->SetClassPrototype(true);
    hclass->SetIsPrototype(true);
    return hclass;
}

JSHandle<JSHClass> ClassInfoExtractor::CreateConstructorHClass(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                                               JSHandle<TaggedArray> &keys,
                                                               JSHandle<TaggedArray> &properties)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t length = keys->GetLength();
    if (!thread->GetEcmaVM()->IsEnablePGOProfiler()) {
        // The class constructor of AOT is not shared, and PGO collect cannot be shared.
        if (length == ClassInfoExtractor::STATIC_RESERVED_LENGTH && base->IsHole() &&
            properties->Get(NAME_INDEX).IsString()) {
            const GlobalEnvConstants *globalConst = thread->GlobalConstants();
            return JSHandle<JSHClass>(globalConst->GetHandledClassConstructorClass());
        }
    }
    JSHandle<JSHClass> hclass;
    if (LIKELY(length <= PropertyAttributes::MAX_FAST_PROPS_CAPACITY)) {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(length, MemSpaceType::OLD_SPACE, GrowMode::KEEP);
        for (uint32_t index = 0; index < length; ++index) {
            key.Update(keys->Get(index));
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes;
            switch (index) {
                case LENGTH_INDEX:
                    attributes = PropertyAttributes::Default(false, false, true);
                    break;
                case NAME_INDEX:
                    if (LIKELY(properties->Get(NAME_INDEX).IsString())) {
                        attributes = PropertyAttributes::Default(false, false, true);
                    } else {
                        ASSERT(properties->Get(NAME_INDEX).IsJSFunction());
                        attributes = PropertyAttributes::Default(true, false, true);
                    }
                    break;
                case PROTOTYPE_INDEX:
                    attributes = PropertyAttributes::DefaultAccessor(false, false, false);
                    break;
                default:
                    attributes = PropertyAttributes::Default(true, false, true);
                    break;
            }

            if (UNLIKELY(properties->Get(index).IsAccessor())) {
                attributes.SetIsAccessor(true);
            }

            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::TAGGED);
            attributes.SetOffset(index);
            layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
        }

        hclass = factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, length);
        // Not need set proto here
        hclass->SetLayout(thread, layout);
        hclass->SetNumberOfProps(length);
    } else {
        // dictionary mode
        hclass = factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, 0);  // without in-obj
        hclass->SetIsDictionaryMode(true);
        hclass->SetNumberOfProps(0);
    }

    hclass->SetClassConstructor(true);
    hclass->SetConstructor(true);

    return hclass;
}

void ClassInfoExtractor::CorrectConstructorHClass(JSThread *thread,
                                                  JSHandle<TaggedArray> &properties,
                                                  JSHClass *constructorHClass)
{
    if (LIKELY(!constructorHClass->IsDictionaryMode())) {
        JSHandle<LayoutInfo> layout(thread, constructorHClass->GetLayout());
        for (uint32_t index = 0; index < ClassInfoExtractor::STATIC_RESERVED_LENGTH; ++index) {
            switch (index) {
                case NAME_INDEX:
                    if (UNLIKELY(properties->Get(NAME_INDEX).IsJSFunction())) {
                        PropertyAttributes attr = layout->GetAttr(index);
                        attr.SetWritable(true);
                        layout->SetNormalAttr(thread, index, attr);
                    }
                    if (UNLIKELY(properties->Get(index).IsAccessor())) {
                        PropertyAttributes attr = layout->GetAttr(index);
                        attr.SetIsAccessor(true);
                        layout->SetNormalAttr(thread, index, attr);
                    }
                    break;
                default:
                    if (UNLIKELY(properties->Get(index).IsAccessor())) {
                        PropertyAttributes attr = layout->GetAttr(index);
                        attr.SetIsAccessor(true);
                        layout->SetNormalAttr(thread, index, attr);
                    }
                    break;
            }
        }
    }
}

JSHandle<JSHClass> ClassInfoExtractor::CreateSendableHClass(JSThread *thread, JSHandle<TaggedArray> &keys,
                                                            JSHandle<TaggedArray> &properties, bool isProtoClass,
                                                            uint32_t extraLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t length = keys->GetLength();
    JSHandle<JSHClass> hclass;
    uint32_t maxInline = isProtoClass ? JSSharedObject::MAX_INLINE : JSSharedFunction::MAX_INLINE;
    if (LIKELY(length + extraLength <= maxInline)) {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateSLayoutInfo(length + extraLength);
        for (uint32_t index = 0; index < length; ++index) {
            key.Update(keys->Get(index));
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
            if (UNLIKELY(properties->Get(index).IsAccessor())) {
                attributes.SetIsAccessor(true);
            }
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::TAGGED);
            attributes.SetOffset(index);
            layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
        }
        hclass = isProtoClass ? factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, length) :
            factory->NewSEcmaHClass(JSSharedFunction::SIZE, JSType::JS_SHARED_FUNCTION, length + extraLength);
        hclass->SetLayout(thread, layout);
        hclass->SetNumberOfProps(length);
    } else {
        // dictionary mode
        hclass = isProtoClass ? factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, 0) :
            factory->NewSEcmaHClass(JSSharedFunction::SIZE, JSType::JS_SHARED_FUNCTION, 0);
        hclass->SetIsDictionaryMode(true);
        hclass->SetNumberOfProps(0);
    }
    if (isProtoClass) {
        hclass->SetClassPrototype(true);
        hclass->SetIsPrototype(true);
    } else {
        hclass->SetClassConstructor(true);
        hclass->SetConstructor(true);
    }
    return hclass;
}

JSHandle<JSFunction> ClassHelper::DefineClassFromExtractor(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                                           JSHandle<ClassInfoExtractor> &extractor,
                                                           const JSHandle<JSTaggedValue> &lexenv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> staticKeys(thread, extractor->GetStaticKeys());
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties());

    JSHandle<TaggedArray> nonStaticKeys(thread, extractor->GetNonStaticKeys());
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties());
    JSHandle<JSHClass> prototypeHClass = ClassInfoExtractor::CreatePrototypeHClass(thread, nonStaticKeys,
                                                                                   nonStaticProperties);

    JSHandle<JSObject> prototype = factory->NewOldSpaceJSObject(prototypeHClass);
    JSHandle<Method> method(thread, Method::Cast(extractor->GetConstructorMethod().GetTaggedObject()));
    JSHandle<JSHClass> constructorHClass = ClassInfoExtractor::CreateConstructorHClass(thread, base, staticKeys,
                                                                                       staticProperties);
    // Allocate to non-movable space for PGO
    JSHandle<JSFunction> constructor = factory->NewJSFunctionByHClass(method, constructorHClass,
        MemSpaceType::NON_MOVABLE);

    // non-static
    nonStaticProperties->Set(thread, 0, constructor);

    uint32_t nonStaticLength = nonStaticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());

    if (LIKELY(!prototypeHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < nonStaticLength; ++index) {
            propValue.Update(nonStaticProperties->Get(index));
            if (propValue->IsJSFunction()) {
                JSHandle<JSFunction> propFunc = factory->CloneJSFunction(JSHandle<JSFunction>::Cast(propValue));
                propFunc->SetHomeObject(thread, prototype);
                propFunc->SetLexicalEnv(thread, lexenv);
                propValue.Update(propFunc);
            }
            prototype->SetPropertyInlinedProps(thread, index, propValue.GetTaggedValue());
        }
    } else {
        JSHandle<NameDictionary> dict = BuildDictionaryProperties(thread, prototype, nonStaticKeys, nonStaticProperties,
                                                                  ClassPropertyType::NON_STATIC, lexenv);
        prototype->SetProperties(thread, dict);
    }

    // non-static elements
    if (UNLIKELY(extractor->GetNonStaticWithElements())) {
        JSHandle<TaggedArray> nonStaticElements(thread, extractor->GetNonStaticElements());
        ClassHelper::HandleElementsProperties(thread, prototype, nonStaticElements);
    }

    // static
    uint32_t staticLength = staticProperties->GetLength();

    if (LIKELY(!constructorHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < staticLength; ++index) {
            propValue.Update(staticProperties->Get(index));
            if (propValue->IsJSFunction()) {
                JSHandle<JSFunction> propFunc = factory->CloneJSFunction(JSHandle<JSFunction>::Cast(propValue));
                propFunc->SetHomeObject(thread, constructor);
                propFunc->SetLexicalEnv(thread, lexenv);
                propValue.Update(propFunc);
            }
            JSHandle<JSObject>::Cast(constructor)->SetPropertyInlinedProps(thread, index, propValue.GetTaggedValue());
        }
    } else {
        JSHandle<NameDictionary> dict = BuildDictionaryProperties(thread, JSHandle<JSObject>(constructor), staticKeys,
                                                                  staticProperties, ClassPropertyType::STATIC, lexenv);
        constructor->SetProperties(thread, dict);
    }

    // static elements
    if (UNLIKELY(extractor->GetStaticWithElements())) {
        JSHandle<TaggedArray> staticElements(thread, extractor->GetStaticElements());
        ClassHelper::HandleElementsProperties(thread, JSHandle<JSObject>(constructor), staticElements);
    }

    PropertyDescriptor ctorDesc(thread, JSHandle<JSTaggedValue>(constructor), true, false, true);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(prototype),
                                         globalConst->GetHandledConstructorString(), ctorDesc);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSFunction, thread);
    constructor->SetHomeObject(thread, prototype);
    constructor->SetProtoOrHClass(thread, prototype);
    if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
        thread->GetEcmaVM()->GetPGOProfiler()->ProfileDefineClass(constructor.GetTaggedType());
    }
    return constructor;
}

JSHandle<JSFunction> ClassHelper::DefineClassWithIHClass(JSThread *thread,
                                                         JSHandle<ClassInfoExtractor> &extractor,
                                                         const JSHandle<JSTaggedValue> &lexenv,
                                                         const JSHandle<JSTaggedValue> &prototypeOrHClass,
                                                         const JSHandle<JSHClass> &constructorHClass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> staticKeys(thread, extractor->GetStaticKeys());
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties());
    ClassInfoExtractor::CorrectConstructorHClass(thread,
                                                 staticProperties, *constructorHClass);
    JSHandle<TaggedArray> nonStaticKeys(thread, extractor->GetNonStaticKeys());
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties());
    JSHandle<JSObject> prototype;
    if (prototypeOrHClass->IsJSHClass()) {
        JSHandle<JSHClass> ihclass(prototypeOrHClass);
        prototype = JSHandle<JSObject>(thread, ihclass->GetProto());
    } else {
        prototype = JSHandle<JSObject>(prototypeOrHClass);
    }

    JSHandle<Method> method(thread, Method::Cast(extractor->GetConstructorMethod().GetTaggedObject()));
    JSHandle<JSFunction> constructor = factory->NewJSFunctionByHClass(method, constructorHClass,
        MemSpaceType::NON_MOVABLE);

    // non-static
    nonStaticProperties->Set(thread, 0, constructor);

    uint32_t nonStaticLength = nonStaticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());

    if (LIKELY(!prototype->GetJSHClass()->IsDictionaryMode())) {
        for (uint32_t index = 0; index < nonStaticLength; ++index) {
            propValue.Update(nonStaticProperties->Get(index));
            if (propValue->IsJSFunction()) {
                JSHandle<JSFunction> propFunc = factory->CloneJSFunction(JSHandle<JSFunction>::Cast(propValue));
                propFunc->SetHomeObject(thread, prototype);
                propFunc->SetLexicalEnv(thread, lexenv);
                propValue.Update(propFunc);
            }
            prototype->SetPropertyInlinedProps(thread, index, propValue.GetTaggedValue());
        }
    } else {
        JSHandle<NameDictionary> dict = BuildDictionaryProperties(thread, prototype, nonStaticKeys, nonStaticProperties,
                                                                  ClassPropertyType::NON_STATIC, lexenv);
        prototype->SetProperties(thread, dict);
    }

    // non-static elements
    if (UNLIKELY(extractor->GetNonStaticWithElements())) {
        JSHandle<TaggedArray> nonStaticElements(thread, extractor->GetNonStaticElements());
        ClassHelper::HandleElementsProperties(thread, prototype, nonStaticElements);
    }

    // static
    uint32_t staticLength = staticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    int correntIndex = 0;
    if (LIKELY(!constructorHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < staticLength; ++index) {
            propValue.Update(staticProperties->Get(index));
            if (propValue->IsJSFunction()) {
                JSHandle<JSFunction> propFunc = factory->CloneJSFunction(JSHandle<JSFunction>::Cast(propValue));
                propFunc->SetHomeObject(thread, constructor);
                propFunc->SetLexicalEnv(thread, lexenv);
                propValue.Update(propFunc);
            }
            bool needCorrentIndex = index >= ClassInfoExtractor::STATIC_RESERVED_LENGTH;
            if (needCorrentIndex) {
                key.Update(staticKeys->Get(index));
                correntIndex = JSHClass::FindPropertyEntry(thread, *constructorHClass, key.GetTaggedValue());
            }
            JSHandle<JSObject>::Cast(constructor)->SetPropertyInlinedProps(thread,
                needCorrentIndex ? static_cast<uint32_t>(correntIndex) : index, propValue.GetTaggedValue());
        }
    } else {
        JSHandle<NameDictionary> dict = BuildDictionaryProperties(thread, JSHandle<JSObject>(constructor), staticKeys,
                                                                  staticProperties, ClassPropertyType::STATIC, lexenv);
        constructor->SetProperties(thread, dict);
    }

    // static elements
    if (UNLIKELY(extractor->GetStaticWithElements())) {
        JSHandle<TaggedArray> staticElements(thread, extractor->GetStaticElements());
        ClassHelper::HandleElementsProperties(thread, JSHandle<JSObject>(constructor), staticElements);
    }

    PropertyDescriptor ctorDesc(thread, JSHandle<JSTaggedValue>(constructor), true, false, true);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(prototype),
                                         globalConst->GetHandledConstructorString(), ctorDesc);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSFunction, thread);
    constructor->SetHomeObject(thread, prototype);
    constructor->SetProtoOrHClass(thread, prototypeOrHClass);

    if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
        thread->GetEcmaVM()->GetPGOProfiler()->ProfileDefineClass(constructor.GetTaggedType());
    }
    return constructor;
}

JSHandle<NameDictionary> ClassHelper::BuildDictionaryProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                                                JSHandle<TaggedArray> &keys,
                                                                JSHandle<TaggedArray> &properties,
                                                                ClassPropertyType type,
                                                                const JSHandle<JSTaggedValue> &lexenv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t length = keys->GetLength();
    ASSERT(length > PropertyAttributes::MAX_FAST_PROPS_CAPACITY);
    ASSERT(keys->GetLength() == properties->GetLength());

    JSMutableHandle<NameDictionary> dict(
        thread, NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(length)));
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < length; index++) {
        PropertyAttributes attributes;
        if (type == ClassPropertyType::STATIC) {
            switch (index) {
                case ClassInfoExtractor::LENGTH_INDEX:
                    attributes = PropertyAttributes::Default(false, false, true);
                    break;
                case ClassInfoExtractor::NAME_INDEX:
                    if (LIKELY(properties->Get(ClassInfoExtractor::NAME_INDEX).IsString())) {
                        attributes = PropertyAttributes::Default(false, false, true);
                    } else {
                        ASSERT(properties->Get(ClassInfoExtractor::NAME_INDEX).IsJSFunction());
                        attributes = PropertyAttributes::Default(true, false, true);
                    }
                    break;
                case ClassInfoExtractor::PROTOTYPE_INDEX:
                    attributes = PropertyAttributes::DefaultAccessor(false, false, false);
                    break;
                default:
                    attributes = PropertyAttributes::Default(true, false, true);
                    break;
            }
        } else {
            attributes = PropertyAttributes::Default(true, false, true);  // non-enumerable
        }
        propKey.Update(keys->Get(index));
        propValue.Update(properties->Get(index));
        if (propValue->IsJSFunction()) {
            JSHandle<JSFunction> propFunc = factory->CloneJSFunction(JSHandle<JSFunction>::Cast(propValue));
            propFunc->SetHomeObject(thread, object);
            propFunc->SetLexicalEnv(thread, lexenv);
            propValue.Update(propFunc);
        }
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attributes);
        dict.Update(newDict);
    }
    return dict;
}

bool ClassHelper::MatchTrackType(TrackType trackType, JSTaggedValue value)
{
    bool checkRet = false;
    switch (trackType) {
        case TrackType::NUMBER: {
            checkRet = value.IsNumber();
            break;
        }
        case TrackType::INT: {
            checkRet = value.IsInt();
            break;
        }
        case TrackType::DOUBLE: {
            checkRet = value.IsDouble();
            break;
        }
        case TrackType::BOOLEAN: {
            checkRet = value.IsBoolean();
            break;
        }
        case TrackType::STRING: {
            checkRet = value.IsString() || value.IsNull();
            break;
        }
        case TrackType::SENDABLE: {
            checkRet = value.IsJSShared() || value.IsNull();
            break;
        }
        case TrackType::NONE: {
            checkRet = true;
            break;
        }
        case TrackType::TAGGED:
        default:
            break;
    }
    return checkRet;
}

void ClassHelper::HandleElementsProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                           JSHandle<TaggedArray> &elements)
{
    JSMutableHandle<JSTaggedValue> elementsKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> elementsValue(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < elements->GetLength(); index += 2) {  // 2: key-value pair
        elementsKey.Update(elements->Get(index));
        elementsValue.Update(elements->Get(index + 1));
        // class property attribute is not default, will transition to dictionary directly.
        JSObject::DefinePropertyByLiteral(thread, object, elementsKey, elementsValue, true);

        if (elementsValue->IsJSFunction()) {
            JSHandle<JSFunction> elementsFunc = JSHandle<JSFunction>::Cast(elementsValue);
            elementsFunc->SetHomeObject(thread, object);
        }
    }
}

JSHandle<JSFunction> SendableClassDefiner::DefineSendableClassFromExtractor(JSThread *thread,
    JSHandle<ClassInfoExtractor> &extractor, const JSHandle<TaggedArray> &staticFieldArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> staticKeys(thread, extractor->GetStaticKeys());
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties());
    SendableClassDefiner::FilterDuplicatedKeys(thread, staticKeys, staticProperties);

    JSHandle<TaggedArray> nonStaticKeys(thread, extractor->GetNonStaticKeys());
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties());
    SendableClassDefiner::FilterDuplicatedKeys(thread, nonStaticKeys, nonStaticProperties);
    JSHandle<JSHClass> prototypeHClass = ClassInfoExtractor::CreateSendableHClass(thread, nonStaticKeys,
                                                                                  nonStaticProperties, true);
    JSHandle<JSObject> prototype = factory->NewSharedOldSpaceJSObject(prototypeHClass);
    uint32_t length = staticFieldArray->GetLength();
    uint32_t staticFields =  length / 2; // 2: key-type
    JSHandle<JSHClass> constructorHClass =
        ClassInfoExtractor::CreateSendableHClass(thread, staticKeys, staticProperties, false, staticFields);
    // todo(lukai) method from constantpool should allocate to sharedspace in future.
    JSHandle<Method> method(thread, Method::Cast(extractor->GetConstructorMethod().GetTaggedObject()));
    method->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
    if (!constructorHClass->IsDictionaryMode() && staticFields > 0) {
        auto layout = JSHandle<LayoutInfo>(thread, constructorHClass->GetLayout());
        AddFieldTypeToHClass(thread, staticFieldArray, layout, constructorHClass);
    }

    JSHandle<JSFunction> constructor = factory->NewSFunctionByHClass(method, constructorHClass);

    // non-static
    nonStaticProperties->Set(thread, 0, constructor);

    uint32_t nonStaticLength = nonStaticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());

    if (LIKELY(!prototypeHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < nonStaticLength; ++index) {
            propValue.Update(nonStaticProperties->Get(index));
            // constructor don't need to clone
            if (propValue->IsJSFunction() && index != ClassInfoExtractor::CONSTRUCTOR_INDEX) {
                JSHandle<JSFunction> propFunc = factory->CloneSFunction(JSHandle<JSFunction>::Cast(propValue));
                propFunc->SetHomeObject(thread, prototype);
                propFunc->SetLexicalEnv(thread, constructor);
                ASSERT(!propFunc->GetClass()->IsExtensible());
                propValue.Update(propFunc);
            } else if (propValue->IsAccessorData()) {
                UpdateAccessorFunction(thread, propValue, JSHandle<JSTaggedValue>(prototype), constructor);
            }
            prototype->SetPropertyInlinedProps(thread, index, propValue.GetTaggedValue());
        }
    } else {
        JSHandle<NameDictionary> dict = BuildSendableDictionaryProperties(thread, prototype, nonStaticKeys,
            nonStaticProperties, ClassPropertyType::NON_STATIC, constructor);
        prototype->SetProperties(thread, dict);
    }

    // non-static elements
    if (UNLIKELY(extractor->GetNonStaticWithElements())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Concurrent class don't support members with numerical key",
            JSHandle<JSFunction>(thread, JSTaggedValue::Exception()));
    }

    // static
    uint32_t staticLength = staticProperties->GetLength();
    if (LIKELY(!constructorHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < staticLength; ++index) {
            propValue.Update(staticProperties->Get(index));
            if (propValue->IsJSFunction()) {
                JSHandle<JSFunction> propFunc = factory->CloneSFunction(JSHandle<JSFunction>::Cast(propValue));
                propFunc->SetHomeObject(thread, constructor);
                propFunc->SetLexicalEnv(thread, constructor);
                ASSERT(!propFunc->GetClass()->IsExtensible());
                propValue.Update(propFunc);
            } else if (propValue->IsAccessorData()) {
                UpdateAccessorFunction(thread, propValue, JSHandle<JSTaggedValue>(constructor), constructor);
            }
            JSHandle<JSObject>::Cast(constructor)->SetPropertyInlinedProps(thread, index, propValue.GetTaggedValue());
        }
    } else {
        JSHandle<NameDictionary> dict =
            BuildSendableDictionaryProperties(thread, JSHandle<JSObject>(constructor), staticKeys,
                                              staticProperties, ClassPropertyType::STATIC, constructor);
        JSMutableHandle<NameDictionary> nameDict(thread, dict);
        if (staticFields > 0) {
            AddFieldTypeToDict(thread, staticFieldArray, nameDict,
                               PropertyAttributes::Default(true, true, false));
        }
        constructor->SetProperties(thread, nameDict);
    }

    // static elements
    if (UNLIKELY(extractor->GetStaticWithElements())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Concurrent class don't support static members with numerical key",
            JSHandle<JSFunction>(thread, JSTaggedValue::Exception()));
    }
    prototype->GetJSHClass()->SetExtensible(false);
    constructor->SetHomeObject(thread, prototype);
    constructor->SetProtoOrHClass(thread, prototype);
    constructor->SetLexicalEnv(thread, constructor);
    return constructor;
}

// Process duplicated key due to getter/setter.
void SendableClassDefiner::FilterDuplicatedKeys(JSThread *thread, const JSHandle<TaggedArray> &keys,
                                                const JSHandle<TaggedArray> &properties)
{
    auto attr = PropertyAttributes::Default();
    uint32_t length = keys->GetLength();
    uint32_t left = 0;
    uint32_t right = 0;
    JSMutableHandle<NameDictionary> dict(
        thread, NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(length)));
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> existValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> index(thread, JSTaggedValue::Undefined());
    for (; right < length; right++) {
        key.Update(keys->Get(right));
        value.Update(properties->Get(right));
        int entry = dict->FindEntry(key.GetTaggedValue());
        if (entry == -1) {
            TryUpdateValue(thread, value);
            index.Update(JSTaggedValue(left));
            JSHandle<NameDictionary> newDict =
                NameDictionary::PutIfAbsent(thread, dict, key, index, attr);
            dict.Update(newDict);
            if (left < right) {
                keys->Set(thread, left, key);
            }
            properties->Set(thread, left, value);
            left++;
            continue;
        }
        auto existIndex = static_cast<uint32_t>(dict->GetValue(entry).GetNumber());
        existValue.Update(properties->Get(existIndex));
        bool needUpdateValue = TryUpdateExistValue(thread, existValue, value);
        if (needUpdateValue) {
            properties->Set(thread, existIndex, value);
        }
    }
    if (left < right) {
        keys->Trim(thread, left);
        properties->Trim(thread, left);
    }
}

JSHandle<NameDictionary> SendableClassDefiner::BuildSendableDictionaryProperties(JSThread *thread,
    const JSHandle<JSObject> &object, JSHandle<TaggedArray> &keys, JSHandle<TaggedArray> &properties,
    ClassPropertyType type, const JSHandle<JSFunction> &ctor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t length = keys->GetLength();
    ASSERT(keys->GetLength() == properties->GetLength());

    JSMutableHandle<NameDictionary> dict(
        thread, NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(length)));
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < length; index++) {
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        propKey.Update(keys->Get(index));
        propValue.Update(properties->Get(index));
        // constructor don't need to clone
        if (index == ClassInfoExtractor::CONSTRUCTOR_INDEX && type == ClassPropertyType::NON_STATIC) {
            JSHandle<NameDictionary> newDict =
                NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attributes);
            dict.Update(newDict);
            continue;
        }
        if (propValue->IsJSFunction()) {
            JSHandle<JSFunction> propFunc = factory->CloneSFunction(JSHandle<JSFunction>::Cast(propValue));
            propFunc->SetHomeObject(thread, object);
            propFunc->SetLexicalEnv(thread, ctor);
            ASSERT(!propFunc->GetClass()->IsExtensible());
            propValue.Update(propFunc);
        } else if (propValue->IsAccessorData()) {
            UpdateAccessorFunction(thread, propValue, JSHandle<JSTaggedValue>(object), ctor);
        }
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attributes);
        dict.Update(newDict);
    }
    return dict;
}

void SendableClassDefiner::AddFieldTypeToHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
    const JSHandle<LayoutInfo> &layout, const JSHandle<JSHClass> &hclass)
{
    uint32_t length = fieldTypeArray->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    uint32_t index = static_cast<uint32_t>(layout->NumberOfElements());
    PropertyAttributes attributes = PropertyAttributes::Default(true, true, false);
    attributes.SetIsInlinedProps(true);
    attributes.SetRepresentation(Representation::TAGGED);
    for (uint32_t i = 0; i < length; i += 2) { // 2: key-value pair;
        key.Update(fieldTypeArray->Get(i));
        ASSERT(key->IsString());
        TrackType type = FromFieldType(FieldType(fieldTypeArray->Get(i + 1).GetInt()));
        int entry = layout->FindElementWithCache(thread, *hclass, key.GetTaggedValue(), index);
        if (entry != -1) {
            attributes = layout->GetAttr(entry);
            attributes.SetTrackType(type);
            layout->SetNormalAttr(thread, entry, attributes);
        } else {
            attributes.SetTrackType(type);
            attributes.SetOffset(index);
            layout->AddKey(thread, index++, key.GetTaggedValue(), attributes);
        }
    }
    hclass->SetLayout(thread, layout);
    hclass->SetNumberOfProps(index);
    auto inlinedProps = hclass->GetInlinedProperties();
    if (inlinedProps > index) {
        // resize hclass due to duplicated key.
        uint32_t duplicatedSize = (inlinedProps - index) * JSTaggedValue::TaggedTypeSize();
        hclass->SetObjectSize(hclass->GetObjectSize() - duplicatedSize);
    }
}

void SendableClassDefiner::AddFieldTypeToDict(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
    JSMutableHandle<NameDictionary> &dict, PropertyAttributes attributes)
{
    uint32_t length = fieldTypeArray->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    JSHandle<JSTaggedValue> value = globalConst->GetHandledUndefined();
    for (uint32_t i = 0; i < length; i += 2) { // 2: key-value pair;
        key.Update(fieldTypeArray->Get(i));
        ASSERT(key->IsString());
        TrackType type = FromFieldType(FieldType(fieldTypeArray->Get(i + 1).GetInt()));
        attributes.SetTrackType(type);
        attributes.SetBoxType(PropertyBoxType::UNDEFINED);
        JSHandle<NameDictionary> newDict = NameDictionary::Put(thread, dict, key, value, attributes);
        dict.Update(newDict);
    }
}

void SendableClassDefiner::AddFieldTypeToHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
    const JSHandle<NameDictionary> &nameDict, const JSHandle<JSHClass> &hclass)
{
    JSMutableHandle<NameDictionary> dict(thread, nameDict);
    AddFieldTypeToDict(thread, fieldTypeArray, dict);
    hclass->SetLayout(thread, dict);
    hclass->SetNumberOfProps(0);
    hclass->SetIsDictionaryMode(true);
}

void SendableClassDefiner::DefineSendableInstanceHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
    const JSHandle<JSFunction> &ctor, const JSHandle<JSTaggedValue> &base)
{
    ASSERT(ctor->GetClass()->IsJSSharedFunction());
    JSHandle<JSObject> clsPrototype(thread, JSHandle<JSFunction>(ctor)->GetFunctionPrototype());
    ASSERT(clsPrototype->GetClass()->IsJSSharedObject());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t length = fieldTypeArray->GetLength();
    uint32_t fieldNum = length / 2; // 2: key-value pair;
    JSHandle<JSHClass> iHClass;
    if (base->IsHole() || base->IsNull()) {
        if (fieldNum == 0) {
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, fieldNum);
        } else if (LIKELY(fieldNum <= JSSharedObject::MAX_INLINE)) {
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, fieldNum);
            JSHandle<LayoutInfo> layout = factory->CreateSLayoutInfo(fieldNum);
            AddFieldTypeToHClass(thread, fieldTypeArray, layout, iHClass);
        } else {
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, 0);
            JSHandle<NameDictionary> dict =
                NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(fieldNum));
            AddFieldTypeToHClass(thread, fieldTypeArray, dict, iHClass);
        }
    } else {
        ASSERT(base->IsJSSharedFunction());
        JSHandle<JSFunction> baseCtor = JSHandle<JSFunction>::Cast(base);
        JSHandle<JSHClass> baseIHClass(thread, baseCtor->GetProtoOrHClass());
        ASSERT(baseIHClass->IsJSSharedObject());
        if (LIKELY(!baseIHClass->IsDictionaryMode())) {
            auto baseLength = baseIHClass->NumberOfProps();
            JSHandle<LayoutInfo> baseLayout(thread, baseIHClass->GetLayout());
            auto newLength = baseLength + fieldNum;
            if (fieldNum == 0) {
                iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, fieldNum);
            } else if (LIKELY(newLength <= JSSharedObject::MAX_INLINE)) {
                iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, newLength);
                JSHandle<LayoutInfo> layout = factory->CopyAndReSortSLayoutInfo(baseLayout, baseLength, newLength);
                AddFieldTypeToHClass(thread, fieldTypeArray, layout, iHClass);
            } else {
                iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, 0);
                JSHandle<NameDictionary> dict =
                    NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(newLength));
                auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
                JSHandle<JSTaggedValue> value = globalConst->GetHandledUndefined();
                JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
                for (uint32_t i = 0; i < baseLength; i++) {
                    key.Update(baseLayout->GetKey(i));
                    PropertyAttributes attr = baseLayout->GetAttr(i);
                    attr.SetIsInlinedProps(false);
                    attr.SetBoxType(PropertyBoxType::UNDEFINED);
                    dict = NameDictionary::Put(thread, dict, key, value, attr);
                }
                AddFieldTypeToHClass(thread, fieldTypeArray, dict, iHClass);
            }
        } else {
            JSHandle<NameDictionary> baseDict(thread, baseIHClass->GetLayout());
            auto baseLength = baseDict->EntriesCount();
            auto newLength = fieldNum + baseLength;
            JSHandle<NameDictionary> dict =
                NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(newLength));
            baseDict->Rehash(thread, *dict);
            dict->SetNextEnumerationIndex(thread, baseDict->GetNextEnumerationIndex());
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, 0);
            AddFieldTypeToHClass(thread, fieldTypeArray, dict, iHClass);
        }
    }
    iHClass->SetPrototype(thread, JSHandle<JSTaggedValue>(clsPrototype));
    iHClass->SetExtensible(false);
    ctor->SetProtoOrHClass(thread, iHClass);
    ctor->GetJSHClass()->SetExtensible(false);
}

JSHandle<TaggedArray> SendableClassDefiner::ExtractStaticFieldTypeArray(JSThread *thread,
    const JSHandle<TaggedArray> &fieldTypeArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = fieldTypeArray->GetLength();
    auto instanceFieldNums = static_cast<uint32_t>(fieldTypeArray->Get(arrayLength - 1).GetInt());
    uint32_t staticFieldBegin = instanceFieldNums * 2; // 2: key-type
    if (staticFieldBegin >= arrayLength) {
        LOG_ECMA(ERROR) << "ExtractStaticFieldTypeArray Failed, staticFieldBegin:" << staticFieldBegin
                        << " should be less than totalLength:" << arrayLength;
        return factory->EmptyArray();
    }
    uint32_t staticFieldLength = arrayLength - staticFieldBegin - 1;
    JSHandle<TaggedArray> staticFieldArray = factory->NewTaggedArray(staticFieldLength);
    for (uint32_t i = 0; i < staticFieldLength; i += 2) {  // 2: key-type
        staticFieldArray->Set(thread, i, fieldTypeArray->Get(staticFieldBegin + i));
        staticFieldArray->Set(thread, i + 1, fieldTypeArray->Get(staticFieldBegin + i + 1));
    }
    return staticFieldArray;
}

void SendableClassDefiner::UpdateAccessorFunction(JSThread *thread, const JSMutableHandle<JSTaggedValue> &value,
    const JSHandle<JSTaggedValue> &homeObject, const JSHandle<JSFunction> &ctor)
{
    ASSERT(value->IsAccessorData());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> accessor(value);
    auto getter = accessor->GetGetter();
    if (getter.IsJSFunction()) {
        JSHandle<JSFunction> func(thread, getter);
        JSHandle<JSFunction> propFunc = factory->CloneSFunction(func);
        propFunc->SetHomeObject(thread, homeObject);
        propFunc->SetLexicalEnv(thread, ctor);
        ASSERT(!propFunc->GetClass()->IsExtensible());
        accessor->SetGetter(thread, propFunc);
    }
    auto setter = accessor->GetSetter();
    if (setter.IsJSFunction()) {
        JSHandle<JSFunction> func(thread, setter);
        JSHandle<JSFunction> propFunc = factory->CloneSFunction(func);
        propFunc->SetHomeObject(thread, homeObject);
        propFunc->SetLexicalEnv(thread, ctor);
        ASSERT(!propFunc->GetClass()->IsExtensible());
        accessor->SetSetter(thread, propFunc);
    }
}

bool SendableClassDefiner::TryUpdateExistValue(JSThread *thread, JSMutableHandle<JSTaggedValue> &existValue,
                                               JSMutableHandle<JSTaggedValue> &value)
{
    bool needUpdateValue = true;
    if (existValue->IsAccessorData()) {
        if (value->IsJSFunction() && JSHandle<JSFunction>(value)->IsGetterOrSetter()) {
            JSHandle<AccessorData> accessor(existValue);
            UpdateValueToAccessor(thread, value, accessor);
            needUpdateValue = false;
        }
    } else {
        if (value->IsJSFunction() && JSHandle<JSFunction>(value)->IsGetterOrSetter()) {
            JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewSAccessorData();
            UpdateValueToAccessor(thread, value, accessor);
        }
    }
    return needUpdateValue;
}

void SendableClassDefiner::TryUpdateValue(JSThread *thread, JSMutableHandle<JSTaggedValue> &value)
{
    if (value->IsJSFunction() && JSHandle<JSFunction>(value)->IsGetterOrSetter()) {
        JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewSAccessorData();
        UpdateValueToAccessor(thread, value, accessor);
    }
}

void SendableClassDefiner::UpdateValueToAccessor(JSThread *thread, JSMutableHandle<JSTaggedValue> &value,
                                                 JSHandle<AccessorData> &accessor)
{
    ASSERT(value->IsJSFunction() && JSHandle<JSFunction>(value)->IsGetterOrSetter());
    if (JSHandle<JSFunction>(value)->IsGetter()) {
        accessor->SetGetter(thread, value);
    } else {
        accessor->SetSetter(thread, value);
    }
    value.Update(accessor);
}
}  // namespace panda::ecmascript
