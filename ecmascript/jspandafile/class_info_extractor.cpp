/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/lexical_env.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript {
void ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(JSThread *thread, JSHandle<ClassInfoExtractor> &extractor,
                                                            const JSHandle<TaggedArray> &literal,
                                                            uint32_t length, ClassKind kind,
                                                            uint32_t implementLength)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    ASSERT(length <= literal->GetLength());
    // non static properties number is hidden in the last index of Literal buffer
    uint32_t nonStaticNum = 0;
    // The effective data length of taggedarray(valueLength) is equal to length - implementationLength -1
    uint32_t valueLength = length - implementLength - 1;
    if (length != 0) {
        nonStaticNum = static_cast<uint32_t>(literal->Get(thread, valueLength).GetInt());
    }

    // Reserve sufficient length to prevent frequent creation.
    JSHandle<TaggedArray> nonStaticKeys;
    JSHandle<TaggedArray> nonStaticProperties;
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
    Method *method = Method::Cast(extractor->GetConstructorMethod(thread).GetTaggedObject());
    MethodLiteral *methodLiteral = method->GetMethodLiteral(thread);
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile(thread);
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

    uint32_t staticNum = length == 0 ? 0 : (valueLength) / 2 - nonStaticNum;

    // Reserve sufficient length to prevent frequent creation.
    JSHandle<TaggedArray> staticKeys;
    JSHandle<TaggedArray> staticProperties;
    if (kind == ClassKind::SENDABLE) {
        staticKeys = factory->NewSOldSpaceTaggedArray(staticNum + SENDABLE_STATIC_RESERVED_LENGTH);
        staticProperties = factory->NewSOldSpaceTaggedArray(staticNum + SENDABLE_STATIC_RESERVED_LENGTH);
    } else {
        staticKeys = factory->NewOldSpaceTaggedArray(staticNum + STATIC_RESERVED_LENGTH);
        staticProperties = factory->NewOldSpaceTaggedArray(staticNum + STATIC_RESERVED_LENGTH);
    }

    staticKeys->Set(thread, LENGTH_INDEX, globalConst->GetLengthString());
    staticKeys->Set(thread, NAME_INDEX, globalConst->GetNameString());
    staticKeys->Set(thread, PROTOTYPE_INDEX, globalConst->GetPrototypeString());
    if (kind == ClassKind::SENDABLE) {
        staticKeys->Set(thread, SENDABLE_ELEMENTS_INDEX, globalConst->GetSendableElementsSymbol());
    }

    JSHandle<TaggedArray> staticElements = factory->EmptyArray();

    if (staticNum) {
        ExtractContentsDetail staticDetail {};

        if (kind == ClassKind::SENDABLE) {
            staticDetail = {
                nonStaticNum * 2, valueLength, SENDABLE_STATIC_RESERVED_LENGTH, methodLiteral };
        } else {
            staticDetail = { nonStaticNum * 2, valueLength, STATIC_RESERVED_LENGTH, methodLiteral };
        }

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
    if (kind == ClassKind::SENDABLE) {
        staticProperties->Set(thread, SENDABLE_ELEMENTS_INDEX, JSTaggedValue::Undefined());
    }

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
        firstValue.Update(literal->Get(thread, index));
        secondValue.Update(literal->Get(thread, index + 1));
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(firstValue), "Key is not a property key");

        if (LIKELY(firstValue->IsString())) {
            if (isStaticFlag && !keysHasNameFlag && JSTaggedValue::SameValue(thread, firstValue, nameString)) {
                properties->Set(thread, NAME_INDEX, secondValue);
                keysHasNameFlag = true;
                continue;
            }

            // front-end can do better: write index in class literal directly.
            uint32_t elementIndex = 0;
            if (JSTaggedValue::StringToElementIndex(thread, firstValue.GetTaggedValue(), &elementIndex)) {
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
            key.Update(keys->Get(thread, index));
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);  // non-enumerable

            if (UNLIKELY(properties->Get(thread, index).IsAccessor())) {
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
            properties->Get(thread, NAME_INDEX).IsString()) {
            const GlobalEnvConstants *globalConst = thread->GlobalConstants();
            return JSHandle<JSHClass>(globalConst->GetHandledClassConstructorClass());
        }
    }
    JSHandle<JSHClass> hclass;
    if (LIKELY(length <= PropertyAttributes::MAX_FAST_PROPS_CAPACITY)) {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(length, MemSpaceType::OLD_SPACE, GrowMode::KEEP);
        for (uint32_t index = 0; index < length; ++index) {
            key.Update(keys->Get(thread, index));
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes;
            switch (index) {
                case LENGTH_INDEX:
                    attributes = PropertyAttributes::Default(false, false, true);
                    break;
                case NAME_INDEX:
                    if (LIKELY(properties->Get(thread, NAME_INDEX).IsString())) {
                        attributes = PropertyAttributes::Default(false, false, true);
                    } else {
                        ASSERT(properties->Get(thread, NAME_INDEX).IsFunctionTemplate());
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

            if (UNLIKELY(properties->Get(thread, index).IsAccessor())) {
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
    if (constructorHClass->IsDictionaryMode()) {
        return;
    }
    JSHandle<LayoutInfo> layout(thread, constructorHClass->GetLayout(thread));
    for (uint32_t index = 0; index < ClassInfoExtractor::STATIC_RESERVED_LENGTH; ++index) {
        if (index == NAME_INDEX) {
            if (UNLIKELY(properties->Get(thread, NAME_INDEX).IsFunctionTemplate())) {
                PropertyAttributes attr = layout->GetAttr(thread, index);
                attr.SetWritable(true);
                layout->SetNormalAttr(thread, index, attr);
            }
            if (UNLIKELY(properties->Get(thread, index).IsAccessor())) {
                PropertyAttributes attr = layout->GetAttr(thread, index);
                attr.SetIsAccessor(true);
                layout->SetNormalAttr(thread, index, attr);
            }
        } else {
            if (UNLIKELY(properties->Get(thread, index).IsAccessor())) {
                PropertyAttributes attr = layout->GetAttr(thread, index);
                attr.SetIsAccessor(true);
                layout->SetNormalAttr(thread, index, attr);
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
            key.Update(keys->Get(thread, index));
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
            if (UNLIKELY(properties->Get(thread, index).IsAccessor())) {
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
    JSHandle<TaggedArray> staticKeys(thread, extractor->GetStaticKeys(thread));
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties(thread));

    JSHandle<TaggedArray> nonStaticKeys(thread, extractor->GetNonStaticKeys(thread));
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties(thread));
    JSHandle<JSHClass> prototypeHClass = ClassInfoExtractor::CreatePrototypeHClass(thread, nonStaticKeys,
                                                                                   nonStaticProperties);

    JSHandle<JSObject> prototype = factory->NewOldSpaceJSObject(prototypeHClass);
    JSHandle<Method> method(thread, Method::Cast(extractor->GetConstructorMethod(thread).GetTaggedObject()));
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
            propValue.Update(nonStaticProperties->Get(thread, index));
            if (propValue->IsFunctionTemplate()) {
                auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
                propValue.Update(CreateJSFunctionFromTemplate(thread, literalFunc, prototype, lexenv));
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
        JSHandle<TaggedArray> nonStaticElements(thread, extractor->GetNonStaticElements(thread));
        ClassHelper::HandleElementsProperties(thread, prototype, lexenv, nonStaticElements);
    }

    // static
    uint32_t staticLength = staticProperties->GetLength();

    if (LIKELY(!constructorHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < staticLength; ++index) {
            propValue.Update(staticProperties->Get(thread, index));
            if (propValue->IsFunctionTemplate()) {
                auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
                propValue.Update(
                    CreateJSFunctionFromTemplate(thread, literalFunc, JSHandle<JSObject>(constructor), lexenv));
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
        JSHandle<TaggedArray> staticElements(thread, extractor->GetStaticElements(thread));
        ClassHelper::HandleElementsProperties(thread, JSHandle<JSObject>(constructor), lexenv, staticElements);
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

JSHandle<JSFunction> ClassHelper::DefineClassWithIHClass(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                                         JSHandle<ClassInfoExtractor> &extractor,
                                                         const JSHandle<JSTaggedValue> &lexenv,
                                                         const JSHandle<JSTaggedValue> &prototypeOrHClassVal,
                                                         const JSHandle<JSTaggedValue> &constructorHClassVal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> staticKeys(thread, extractor->GetStaticKeys(thread));
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties(thread));
    JSHandle<JSHClass> constructorHClass;
    // When constructorHClassVal is undefined, it means that AOT has not generated the corresponding hclass (chc),
    // then chc will be created through the interpreter.
    if (constructorHClassVal->IsUndefined()) {
        constructorHClass = ClassInfoExtractor::CreateConstructorHClass(thread, base, staticKeys, staticProperties);
    } else {
        constructorHClass = JSHandle<JSHClass>(constructorHClassVal);
        ClassInfoExtractor::CorrectConstructorHClass(thread, staticProperties, *constructorHClass);
    }

    JSHandle<TaggedArray> nonStaticKeys(thread, extractor->GetNonStaticKeys(thread));
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties(thread));
    JSHandle<JSObject> prototype;
    JSHandle<JSTaggedValue> prototypeOrHClass = prototypeOrHClassVal;
    // When prototypeOrHClassVal is undefined, it means that AOT has not generated the corresponding hclass or
    // prototype, then prototype will be created through the interpreter.
    if (prototypeOrHClassVal->IsUndefined()) {
        JSHandle<JSHClass> prototypeHClass = ClassInfoExtractor::CreatePrototypeHClass(thread, nonStaticKeys,
                                                                                       nonStaticProperties);
        prototype = factory->NewOldSpaceJSObject(prototypeHClass);
        prototypeOrHClass = JSHandle<JSTaggedValue>(prototype);
    } else if (prototypeOrHClassVal->IsJSHClass()) {
        JSHandle<JSHClass> ihclass(prototypeOrHClassVal);
        prototype = JSHandle<JSObject>(thread, ihclass->GetProto(thread));
    } else {
        prototype = JSHandle<JSObject>(prototypeOrHClassVal);
    }

    JSHandle<Method> method(thread, Method::Cast(extractor->GetConstructorMethod(thread).GetTaggedObject()));
    JSHandle<JSFunction> constructor = factory->NewJSFunctionByHClass(method, constructorHClass,
        MemSpaceType::NON_MOVABLE);

    // non-static
    nonStaticProperties->Set(thread, 0, constructor);

    uint32_t nonStaticLength = nonStaticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());

    if (LIKELY(!prototype->GetJSHClass()->IsDictionaryMode())) {
        for (uint32_t index = 0; index < nonStaticLength; ++index) {
            propValue.Update(nonStaticProperties->Get(thread, index));
            if (propValue->IsFunctionTemplate()) {
                auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
                propValue.Update(CreateJSFunctionFromTemplate(thread, literalFunc, prototype, lexenv));
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
        JSHandle<TaggedArray> nonStaticElements(thread, extractor->GetNonStaticElements(thread));
        ClassHelper::HandleElementsProperties(thread, prototype, lexenv, nonStaticElements);
    }

    // static
    uint32_t staticLength = staticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    int correntIndex = 0;
    if (LIKELY(!constructorHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < staticLength; ++index) {
            propValue.Update(staticProperties->Get(thread, index));
            if (propValue->IsFunctionTemplate()) {
                auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
                propValue.Update(
                    CreateJSFunctionFromTemplate(thread, literalFunc, JSHandle<JSObject>(constructor), lexenv));
            }
            bool needCorrentIndex = index >= ClassInfoExtractor::STATIC_RESERVED_LENGTH;
            if (needCorrentIndex) {
                key.Update(staticKeys->Get(thread, index));
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
        JSHandle<TaggedArray> staticElements(thread, extractor->GetStaticElements(thread));
        ClassHelper::HandleElementsProperties(thread, JSHandle<JSObject>(constructor), lexenv, staticElements);
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

JSHandle<JSFunction> ClassHelper::CreateJSFunctionFromTemplate(JSThread *thread,
                                                               const JSHandle<FunctionTemplate> &funcTemp,
                                                               const JSHandle<JSObject> &homeObject,
                                                               const JSHandle<JSTaggedValue> &lexenv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> propFunc = factory->CreateJSFunctionFromTemplate(funcTemp);
    if (thread->IsMultiContextTriggered()) {
        // Disable the profileTypeInfo sharing in the multi-context scenario.
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        propFunc->SetRawProfileTypeInfo(thread, globalConst->GetEmptyProfileTypeInfoCell());
    } else {
        JSFunction::UpdateProfileTypeInfoCell(thread, funcTemp, propFunc);
    }
    propFunc->SetHomeObject(thread, homeObject);
    propFunc->SetLexicalEnv(thread, lexenv);
    return propFunc;
}

JSHandle<NameDictionary> ClassHelper::BuildDictionaryProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                                                JSHandle<TaggedArray> &keys,
                                                                JSHandle<TaggedArray> &properties,
                                                                ClassPropertyType type,
                                                                const JSHandle<JSTaggedValue> &lexenv)
{
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
                    if (LIKELY(properties->Get(thread, ClassInfoExtractor::NAME_INDEX).IsString())) {
                        attributes = PropertyAttributes::Default(false, false, true);
                    } else {
                        ASSERT(properties->Get(thread, ClassInfoExtractor::NAME_INDEX).IsFunctionTemplate());
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
        propKey.Update(keys->Get(thread, index));
        propValue.Update(properties->Get(thread, index));
        if (propValue->IsFunctionTemplate()) {
            auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
            propValue.Update(CreateJSFunctionFromTemplate(thread, literalFunc, object, lexenv));
        }
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attributes);
        dict.Update(newDict);
    }
    return dict;
}

bool ClassHelper::MatchFieldType(SharedFieldType fieldType, JSTaggedValue value)
{
    // all sendable types can be set to undefined
    if (value.IsUndefined()) {
        return true;
    }
    uint32_t sharedFieldType = static_cast<uint32_t>(fieldType);
    if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::NUMBER)) != 0 && value.IsNumber()) {
        return true;
    } else if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::BOOLEAN)) != 0 && value.IsBoolean()) {
        return true;
    } else if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::STRING)) != 0 &&
        (value.IsString() || value.IsNull())) {
        return true;
    } else if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::BIG_INT)) != 0 && value.IsBigInt()) {
        return true;
    } else if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::SENDABLE)) != 0 &&
        (value.IsJSShared() || value.IsNull())) {
        return true;
    } else if ((sharedFieldType == static_cast<uint32_t>(SharedFieldType::NONE) ||
        (sharedFieldType & static_cast<uint32_t>(SharedFieldType::GENERIC)) != 0) &&
        (value.IsJSShared() || !value.IsHeapObject())) {
        // (none || generic) && (jsShared || !heapObject)
        return true;
    } else if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::NULL_TYPE)) != 0 && value.IsNull()) {
        return true;
    } else if ((sharedFieldType & static_cast<uint32_t>(SharedFieldType::UNDEFINED)) != 0 && value.IsUndefined()) {
        return true;
    }
    std::stringstream oss;
    value.DumpTaggedValueType(oss);
    LOG_ECMA(ERROR) << "Sendable obj Match field type fail. expected type: " <<
        StaticFieldTypeToString(sharedFieldType) << ", actual type: " << oss.str();
    return false;
}

CString ClassHelper::StaticFieldTypeToString(uint32_t fieldType)
{
    switch (fieldType) {
        case static_cast<uint32_t>(SharedFieldType::NONE):
            return "[None]";
        case static_cast<uint32_t>(SharedFieldType::NUMBER):
            return "[Number]";
        case static_cast<uint32_t>(SharedFieldType::STRING):
            return "[String]";
        case static_cast<uint32_t>(SharedFieldType::BOOLEAN):
            return "[Boolean]";
        case static_cast<uint32_t>(SharedFieldType::SENDABLE):
            return "[Sendable Object]";
        case static_cast<uint32_t>(SharedFieldType::BIG_INT):
            return "[BigInt]";
        case static_cast<uint32_t>(SharedFieldType::GENERIC):
            return "[Generic]";
        case static_cast<uint32_t>(SharedFieldType::NULL_TYPE):
            return "[Null]";
        case static_cast<uint32_t>(SharedFieldType::UNDEFINED):
            return "[Undefined]";
        default: {
            CString ret = "unknown type ";
            return ret.append(std::to_string(fieldType));
        }
    }
}

void ClassHelper::HandleElementsProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                           const JSHandle<JSTaggedValue> &lexenv, JSHandle<TaggedArray> &elements)
{
    JSMutableHandle<JSTaggedValue> elementsKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> elementsValue(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < elements->GetLength(); index += 2) {  // 2: key-value pair
        elementsKey.Update(elements->Get(thread, index));
        elementsValue.Update(elements->Get(thread, index + 1));
        if (elementsValue->IsFunctionTemplate()) {
            auto literalFunc = JSHandle<FunctionTemplate>::Cast(elementsValue);
            elementsValue.Update(CreateJSFunctionFromTemplate(thread, literalFunc, object, lexenv));
        }

        // class property attribute is not default, will transition to dictionary directly.
        JSObject::DefinePropertyByLiteral(thread, object, elementsKey, elementsValue, true);
    }
}

JSHandle<JSFunction> SendableClassDefiner::DefineSendableClassFromExtractor(JSThread *thread,
    JSHandle<ClassInfoExtractor> &extractor, const JSHandle<TaggedArray> &staticFieldArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> staticKeys(thread, extractor->GetStaticKeys(thread));
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties(thread));
    SendableClassDefiner::FilterDuplicatedKeys(thread, staticKeys, staticProperties);

    JSHandle<TaggedArray> nonStaticKeys(thread, extractor->GetNonStaticKeys(thread));
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties(thread));
    SendableClassDefiner::FilterDuplicatedKeys(thread, nonStaticKeys, nonStaticProperties);
    JSHandle<JSHClass> prototypeHClass = ClassInfoExtractor::CreateSendableHClass(thread, nonStaticKeys,
                                                                                  nonStaticProperties, true);
    JSHandle<JSObject> prototype = factory->NewSharedOldSpaceJSObject(prototypeHClass);
    uint32_t length = staticFieldArray->GetLength();
    uint32_t staticFields =  length / 2; // 2: key-type
    JSHandle<JSHClass> constructorHClass =
        ClassInfoExtractor::CreateSendableHClass(thread, staticKeys, staticProperties, false, staticFields);
    JSHandle<Method> method(thread, Method::Cast(extractor->GetConstructorMethod(thread).GetTaggedObject()));
    /*
    * Method::SetFunctionKind can't be called here, because method will set kind when set inheritance relationship,
    * so there is a multi-threading problem with multi-threads define sendable DERIVED class at the same time.
    * Scenario:
    *    A thread: define DERIVED class X [X's kind = DEFAULT --> BASE CLASS --> DERIVED CLASS], new X()
    *    B thread: define DERIVED class X [X's kind = DEFAULT --> BASE CLASS --> DERIVED CLASS], new X()
    * Issue:
    *     When A thread new DERIVED class X, X's kind maybe set to BASE CLASS at B thread,
    *     and A thread will throw error when call super().
    */

    JSHandle<NumberDictionary> elementsDic = NumberDictionary::CreateInSharedHeap(thread);
    bool hasElement = false;
    if (!constructorHClass->IsDictionaryMode() && staticFields > 0) {
        auto layout = JSHandle<LayoutInfo>(thread, constructorHClass->GetLayout(thread));
        AddFieldTypeToHClass(
            thread, staticFieldArray, length, layout, constructorHClass, elementsDic, hasElement, ~0U);
    }

    JSHandle<JSFunction> constructor = factory->NewSFunctionByHClass(method, constructorHClass);

    // non-static
    nonStaticProperties->Set(thread, 0, constructor);

    uint32_t nonStaticLength = nonStaticProperties->GetLength();
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());

    if (LIKELY(!prototypeHClass->IsDictionaryMode())) {
        for (uint32_t index = 0; index < nonStaticLength; ++index) {
            propValue.Update(nonStaticProperties->Get(thread, index));
            // constructor don't need to clone
            if (propValue->IsFunctionTemplate() && index != ClassInfoExtractor::CONSTRUCTOR_INDEX) {
                auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
                propValue.Update(
                    CreateSFunctionFromTemplate(thread, literalFunc, prototype, constructor));
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
            propValue.Update(staticProperties->Get(thread, index));
            if (propValue->IsFunctionTemplate()) {
                auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
                propValue.Update(CreateSFunctionFromTemplate(
                    thread, literalFunc, JSHandle<JSObject>(constructor), constructor));
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
            AddFieldTypeToDict(thread, staticFieldArray, length, nameDict, elementsDic, hasElement,
                               PropertyAttributes::Default(true, true, false));
        }
        constructor->SetProperties(thread, nameDict);
    }
    if (hasElement) {
        constructor->SetElements(thread, elementsDic);
        constructorHClass->SetIsDictionaryElement(true);
    }

    // static elements
    if (UNLIKELY(extractor->GetStaticWithElements())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Concurrent class don't support static members with numerical key",
            JSHandle<JSFunction>(thread, JSTaggedValue::Exception()));
    }
    prototype->GetJSHClass()->SetExtensible(false);
    constructor->SetHomeObject(thread, prototype);
    constructor->SetProtoOrHClass(thread, prototype);
    JSHandle<SFunctionEnv> sFunctionEnv = factory->NewSFunctionEnv();
    sFunctionEnv->SetConstructor(thread, constructor.GetTaggedValue());
    constructor->SetLexicalEnv(thread, sFunctionEnv);
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
        key.Update(keys->Get(thread, right));
        value.Update(properties->Get(thread, right));
        int entry = dict->FindEntry(thread, key.GetTaggedValue());
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
        auto existIndex = static_cast<uint32_t>(dict->GetValue(thread, entry).GetNumber());
        existValue.Update(properties->Get(thread, existIndex));
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
    uint32_t length = keys->GetLength();
    ASSERT(keys->GetLength() == properties->GetLength());

    JSMutableHandle<NameDictionary> dict(
        thread, NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(length)));
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < length; index++) {
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        if (UNLIKELY(properties->Get(thread, index).IsAccessor())) {
            attributes.SetIsAccessor(true);
        }
        propKey.Update(keys->Get(thread, index));
        propValue.Update(properties->Get(thread, index));
        // constructor don't need to clone
        if (index == ClassInfoExtractor::CONSTRUCTOR_INDEX && type == ClassPropertyType::NON_STATIC) {
            JSHandle<NameDictionary> newDict =
                NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attributes);
            dict.Update(newDict);
            continue;
        }
        if (propValue->IsFunctionTemplate()) {
            auto literalFunc = JSHandle<FunctionTemplate>::Cast(propValue);
            propValue.Update(CreateSFunctionFromTemplate(thread, literalFunc, object, ctor));
        } else if (propValue->IsAccessorData()) {
            UpdateAccessorFunction(thread, propValue, JSHandle<JSTaggedValue>(object), ctor);
        }
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attributes);
        dict.Update(newDict);
    }
    return dict;
}

JSHandle<JSFunction> SendableClassDefiner::CreateSFunctionFromTemplate(JSThread *thread,
                                                                       const JSHandle<FunctionTemplate> &funcTemp,
                                                                       const JSHandle<JSObject> &homeObject,
                                                                       const JSHandle<JSFunction> &ctor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> propFunc = factory->CreateSFunctionFromTemplate(funcTemp);
    propFunc->SetHomeObject(thread, homeObject);
    JSHandle<SFunctionEnv> sFunctionEnv = factory->NewSFunctionEnv();
    sFunctionEnv->SetConstructor(thread, ctor.GetTaggedValue());
    propFunc->SetLexicalEnv(thread, sFunctionEnv);
    ASSERT(!propFunc->GetClass()->IsExtensible());
    return propFunc;
}

void SendableClassDefiner::AddFieldTypeToDict(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
                                              uint32_t length, JSMutableHandle<NameDictionary> &dict,
                                              JSHandle<NumberDictionary> &elementsDic, bool &hasElement,
                                              PropertyAttributes attributes)
{
    ASSERT(length <= fieldTypeArray->GetLength());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    JSHandle<JSTaggedValue> value = globalConst->GetHandledUndefined();

    JSMutableHandle<NumberDictionary> elementsDicUpdate(thread, elementsDic);
    JSHandle<JSTaggedValue> undefinedVal = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> eleIndexKey(thread, JSTaggedValue::Undefined());

    for (uint32_t i = 0; i < length; i += 2) { // 2: key-value pair;
        key.Update(fieldTypeArray->Get(thread, i));
        ASSERT(key->IsString());
        SharedFieldType type = FromFieldType(FieldType(fieldTypeArray->Get(thread, i + 1).GetInt()));
        attributes.SetSharedFieldType(type);
        attributes.SetBoxType(PropertyBoxType::UNDEFINED);
        int64_t eleIndex = ObjectFastOperator::TryToElementsIndex(thread, key.GetTaggedValue());
        if (eleIndex >= 0) {
            eleIndexKey.Update(JSTaggedValue(eleIndex));
            JSHandle<NumberDictionary> newElementsDic = NumberDictionary::Put(
                thread, elementsDic, eleIndexKey, undefinedVal, attributes);
            elementsDicUpdate.Update(newElementsDic);
            if (!hasElement) {
                hasElement = true;
            }
        } else {
            JSHandle<NameDictionary> newDict = NameDictionary::Put(thread, dict, key, value, attributes);
            dict.Update(newDict);
        }
    }
}

void SendableClassDefiner::AddFieldTypeToHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
                                                uint32_t length, const JSHandle<LayoutInfo> &layout,
                                                const JSHandle<JSHClass> &hclass,
                                                JSHandle<NumberDictionary> &elementsDic,
                                                bool &hasElement, size_t start,
                                                std::vector<JSHandle<JSTaggedValue>> &&propertyList)
{
    ASSERT(length <= fieldTypeArray->GetLength());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    uint32_t index = static_cast<uint32_t>(layout->NumberOfElements());

    JSMutableHandle<NumberDictionary> elementsDicUpdate(thread, elementsDic);
    JSHandle<JSTaggedValue> undefinedVal = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> eleIndexKey(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; i += 2) { // 2: key-value pair;
        PropertyAttributes attributes = PropertyAttributes::Default(true, true, false);
        key.Update(fieldTypeArray->Get(thread, i));
        ASSERT(key->IsString());
        SharedFieldType type = FromFieldType(FieldType(fieldTypeArray->Get(thread, i + 1).GetInt()));
        int entry = layout->FindElementWithCache(thread, *hclass, key.GetTaggedValue(), index);
        if (entry != -1) {
            attributes = layout->GetAttr(thread, entry);
            attributes.SetSharedFieldType(type);
            layout->SetNormalAttr(thread, entry, attributes);
            if (start != ~0U && propertyList.size() > 0) {
                propertyList[start + (static_cast<uint32_t>(entry) << 1)] = propertyList[start + i];
                propertyList[start + (static_cast<uint32_t>(entry) << 1) + 1] = propertyList[start + i + 1];
            }
        } else {
            int64_t eleIndex = ObjectFastOperator::TryToElementsIndex(thread, key.GetTaggedValue());
            if (eleIndex < 0 && start != ~0U && propertyList.size() > 0) {
                propertyList[start + (index << 1)] = propertyList[start + i];
                propertyList[start + (index << 1) + 1] = propertyList[start + i + 1];
            }
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::TAGGED);
            attributes.SetSharedFieldType(type);
            attributes.SetOffset(index);
            if (eleIndex >= 0) {
                if (propertyList.size() > 0) {
                    undefinedVal = propertyList[start + i + 1];
                }
                eleIndexKey.Update(JSTaggedValue(eleIndex));
                JSHandle<NumberDictionary> newElementsDic = NumberDictionary::Put(
                    thread, elementsDic, eleIndexKey, undefinedVal, attributes);
                elementsDicUpdate.Update(newElementsDic);
                if (!hasElement) {
                    hasElement = true;
                }
            } else {
                layout->AddKey(thread, index++, key.GetTaggedValue(), attributes);
            }
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

void SendableClassDefiner::AddFieldTypeToHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
                                                uint32_t length, const JSHandle<NameDictionary> &nameDict,
                                                const JSHandle<JSHClass> &hclass,
                                                JSHandle<NumberDictionary> &elementsDic,
                                                bool &hasElement)
{
    JSMutableHandle<NameDictionary> dict(thread, nameDict);
    AddFieldTypeToDict(thread, fieldTypeArray, length, dict, elementsDic, hasElement);
    hclass->SetLayout(thread, dict);
    hclass->SetNumberOfProps(0);
    hclass->SetIsDictionaryMode(true);
}

void SendableClassDefiner::DefineSendableInstanceHClass(JSThread *thread, const JSHandle<TaggedArray> &fieldTypeArray,
    uint32_t length, const JSHandle<JSFunction> &ctor, const JSHandle<JSTaggedValue> &base)
{
    ASSERT(ctor->GetClass()->IsJSSharedFunction());
    JSHandle<JSObject> clsPrototype(thread, JSHandle<JSFunction>(ctor)->GetFunctionPrototype(thread));
    ASSERT(clsPrototype->GetClass()->IsJSSharedObject());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ASSERT(length <= fieldTypeArray->GetLength());
    uint32_t fieldNum = length / 2; // 2: key-value pair;
    JSHandle<JSHClass> iHClass;
    JSHandle<NumberDictionary> elementsDic = NumberDictionary::CreateInSharedHeap(thread);
    bool hasElement = false;
    if (base->IsHole() || base->IsNull()) {
        if (fieldNum == 0) {
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, fieldNum);
        } else if (LIKELY(fieldNum <= JSSharedObject::MAX_INLINE)) {
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, fieldNum);
            JSHandle<LayoutInfo> layout = factory->CreateSLayoutInfo(fieldNum);
            AddFieldTypeToHClass(thread, fieldTypeArray, length, layout, iHClass, elementsDic, hasElement, ~0U);
        } else {
            iHClass = factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, 0);
            JSHandle<NameDictionary> dict =
                NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(fieldNum));
            AddFieldTypeToHClass(thread, fieldTypeArray, length, dict, iHClass, elementsDic, hasElement);
        }
    } else {
        ASSERT(base->IsJSSharedFunction());
        JSHandle<JSFunction> baseCtor = JSHandle<JSFunction>::Cast(base);
        JSHandle<JSHClass> baseIHClass(thread, baseCtor->GetProtoOrHClass(thread));
        ASSERT(baseIHClass->IsJSShared());
        JSType baseType = baseIHClass->GetObjectType();
        const auto [baseSize, baseMaxInlineSize] = GetSizeAndMaxInlineByType(baseType);
        if (LIKELY(!baseIHClass->IsDictionaryMode())) {
            auto baseLength = baseIHClass->NumberOfProps();
            JSHandle<LayoutInfo> baseLayout(thread, baseIHClass->GetLayout(thread));
            auto newLength = baseLength + fieldNum;
            if (newLength == 0) {
                iHClass = factory->NewSEcmaHClass(baseSize, baseType, 0);
            } else if (LIKELY(newLength <= baseMaxInlineSize)) {
                iHClass = factory->NewSEcmaHClass(baseSize, baseType, newLength);
                JSHandle<LayoutInfo> layout = factory->CopyAndReSortSLayoutInfo(baseLayout, baseLength, newLength);
                AddFieldTypeToHClass(thread, fieldTypeArray, length, layout, iHClass, elementsDic, hasElement, ~0U);
            } else {
                iHClass = factory->NewSEcmaHClass(baseSize, baseType, 0);
                JSHandle<NameDictionary> dict =
                    NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(newLength));
                auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
                JSHandle<JSTaggedValue> value = globalConst->GetHandledUndefined();
                JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
                for (uint32_t i = 0; i < baseLength; i++) {
                    key.Update(baseLayout->GetKey(thread, i));
                    PropertyAttributes attr = baseLayout->GetAttr(thread, i);
                    attr.SetIsInlinedProps(false);
                    attr.SetBoxType(PropertyBoxType::UNDEFINED);
                    dict = NameDictionary::Put(thread, dict, key, value, attr);
                }
                AddFieldTypeToHClass(thread, fieldTypeArray, length, dict, iHClass, elementsDic, hasElement);
            }
        } else {
            JSHandle<NameDictionary> baseDict(thread, baseIHClass->GetLayout(thread));
            auto baseLength = baseDict->EntriesCount();
            auto newLength = fieldNum + static_cast<uint32_t>(baseLength);
            JSHandle<NameDictionary> dict =
                NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(newLength));
            baseDict->Rehash(thread, *dict);
            dict->SetNextEnumerationIndex(thread, baseDict->GetNextEnumerationIndex());
            iHClass = factory->NewSEcmaHClass(baseSize, baseType, 0);
            AddFieldTypeToHClass(thread, fieldTypeArray, length, dict, iHClass, elementsDic, hasElement);
        }
    }
    if (hasElement) {
        JSHandle<JSObject>::Cast(ctor)->SetPropertyInlinedProps(
            thread, ClassInfoExtractor::SENDABLE_ELEMENTS_INDEX, elementsDic.GetTaggedValue());
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
    ASSERT(arrayLength > 0);
    auto instanceFieldNums = static_cast<uint32_t>(fieldTypeArray->Get(thread, arrayLength - 1).GetInt());
    uint32_t staticFieldBegin = instanceFieldNums * 2; // 2: key-type
    if (staticFieldBegin >= arrayLength) {
        LOG_ECMA(ERROR) << "ExtractStaticFieldTypeArray Failed, staticFieldBegin:" << staticFieldBegin
                        << " should be less than totalLength:" << arrayLength;
        return factory->EmptyArray();
    }
    uint32_t staticFieldLength = arrayLength - staticFieldBegin - 1;
    JSHandle<TaggedArray> staticFieldArray = factory->NewTaggedArray(staticFieldLength);
    for (uint32_t i = 0; i < staticFieldLength; i += 2) {  // 2: key-type
        staticFieldArray->Set(thread, i, fieldTypeArray->Get(thread, staticFieldBegin + i));
        staticFieldArray->Set(thread, i + 1, fieldTypeArray->Get(thread, staticFieldBegin + i + 1));
    }
    return staticFieldArray;
}

void SendableClassDefiner::UpdateAccessorFunction(JSThread *thread, const JSMutableHandle<JSTaggedValue> &value,
    const JSHandle<JSTaggedValue> &homeObject, const JSHandle<JSFunction> &ctor)
{
    ASSERT(value->IsAccessorData());
    JSHandle<AccessorData> accessor(value);
    auto getter = accessor->GetGetter(thread);
    if (getter.IsFunctionTemplate()) {
        auto funcTemp = JSHandle<FunctionTemplate>(thread, getter);
        auto propFunc = CreateSFunctionFromTemplate(
            thread, funcTemp, JSHandle<JSObject>(homeObject), ctor);
        accessor->SetGetter(thread, propFunc);
    }
    auto setter = accessor->GetSetter(thread);
    if (setter.IsFunctionTemplate()) {
        auto funcTemp = JSHandle<FunctionTemplate>(thread, setter);
        auto propFunc = CreateSFunctionFromTemplate(
            thread, funcTemp, JSHandle<JSObject>(homeObject), ctor);
        accessor->SetSetter(thread, propFunc);
    }
}

bool SendableClassDefiner::TryUpdateExistValue(JSThread *thread, JSMutableHandle<JSTaggedValue> &existValue,
                                               JSMutableHandle<JSTaggedValue> &value)
{
    bool needUpdateValue = true;
    if (existValue->IsAccessorData()) {
        if (value->IsFunctionTemplate() && JSHandle<FunctionTemplate>(value)->IsGetterOrSetter(thread)) {
            JSHandle<AccessorData> accessor(existValue);
            UpdateValueToAccessor(thread, value, accessor);
            needUpdateValue = false;
        }
    } else {
        if (value->IsFunctionTemplate() && JSHandle<FunctionTemplate>(value)->IsGetterOrSetter(thread)) {
            JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewSAccessorData();
            UpdateValueToAccessor(thread, value, accessor);
        }
    }
    return needUpdateValue;
}

void SendableClassDefiner::TryUpdateValue(JSThread *thread, JSMutableHandle<JSTaggedValue> &value)
{
    if (value->IsFunctionTemplate() && JSHandle<FunctionTemplate>(value)->IsGetterOrSetter(thread)) {
        JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewSAccessorData();
        UpdateValueToAccessor(thread, value, accessor);
    }
}

void SendableClassDefiner::UpdateValueToAccessor(JSThread *thread, JSMutableHandle<JSTaggedValue> &value,
                                                 JSHandle<AccessorData> &accessor)
{
    ASSERT(value->IsFunctionTemplate() && JSHandle<FunctionTemplate>(value)->IsGetterOrSetter(thread));
    if (JSHandle<FunctionTemplate>(value)->IsGetter(thread)) {
        accessor->SetGetter(thread, value);
    } else {
        accessor->SetSetter(thread, value);
    }
    value.Update(accessor);
}

std::pair<uint32_t, uint32_t> SendableClassDefiner::GetSizeAndMaxInlineByType(JSType type)
{
    switch (type) {
        case JSType::JS_SHARED_OBJECT:
            return { JSSharedObject::SIZE, JSSharedObject::MAX_INLINE };
        case JSType::JS_SHARED_ARRAY:
            return { JSSharedArray::SIZE, JSSharedArray::MAX_INLINE };
        case JSType::JS_SHARED_MAP:
            return { JSSharedMap::SIZE, JSSharedMap::MAX_INLINE };
        case JSType::JS_SHARED_SET:
            return { JSSharedSet::SIZE, JSSharedSet::MAX_INLINE };
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            return { JSSendableArrayBuffer::SIZE, JSSendableArrayBuffer::MAX_INLINE };
        case JSType::JS_API_BITVECTOR:
            return { JSAPIBitVector::SIZE, JSAPIBitVector::MAX_INLINE };
        default:
            if (JSType::JS_SHARED_TYPED_ARRAY_FIRST < type && type <= JSType::JS_SHARED_TYPED_ARRAY_LAST) {
                return { JSSharedTypedArray::SIZE, JSSharedTypedArray::MAX_INLINE };
            }
            LOG_ECMA(FATAL) << "this branch is unreachable, cannot get size for type: " << static_cast<uint32_t>(type);
            UNREACHABLE();
            return {};
    }
}
}  // namespace panda::ecmascript
