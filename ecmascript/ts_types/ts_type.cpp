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
#include "ecmascript/ts_types/ts_type.h"

#include "ecmascript/js_function.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
JSHClass *TSObjectType::GetOrCreateHClass(JSThread *thread, JSHandle<TSObjectType> objectType, TSObjectTypeKind kind)
{
    JSTaggedValue mayBeHClass = objectType->GetHClass();
    if (mayBeHClass.IsJSHClass()) {
        return JSHClass::Cast(mayBeHClass.GetTaggedObject());
    }
    JSHandle<TSObjLayoutInfo> propTypeInfo(thread, objectType->GetObjLayoutInfo());
    JSHClass *hclass = nullptr;

    switch (kind) {
        case TSObjectTypeKind::INSTANCE: {
            hclass = objectType->CreateHClassByProps(thread, propTypeInfo);
            break;
        }
        case TSObjectTypeKind::PROTOTYPE: {
            hclass = objectType->CreatePrototypeHClassByProps(thread, propTypeInfo);
            break;
        }
        default:
            UNREACHABLE();
    }

    objectType->SetHClass(thread, JSTaggedValue(hclass));
    return hclass;
}

JSHClass *TSObjectType::CreateHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t numOfProps = propType->GetNumOfProperties();
    JSHandle<JSHClass> hclass;
    if (LIKELY(numOfProps <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)) {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(numOfProps);
        for (uint32_t index = 0; index < numOfProps; ++index) {
            JSTaggedValue tsPropKey = propType->GetKey(index);
            key.Update(tsPropKey);
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default();
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::MIXED);
            attributes.SetOffset(index);
            layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
        }
        hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, numOfProps);
        hclass->SetLayout(thread, layout);
        hclass->SetNumberOfProps(numOfProps);
    } else {
        // dictionary mode
        hclass = factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_OBJECT, 0);  // without in-obj
        hclass->SetIsDictionaryMode(true);
        hclass->SetNumberOfProps(0);
    }

    hclass->SetTS(true);

    return *hclass;
}

JSHClass *TSObjectType::CreatePrototypeHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    uint32_t numOfProps = propType->GetNumOfProperties();
    JSHandle<JSHClass> hclass;
    if (LIKELY(numOfProps <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)) {
        TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
        JSHandle<JSTaggedValue> ctor = globalConst->GetHandledConstructorString();
        CVector<std::pair<JSHandle<JSTaggedValue>, GlobalTSTypeRef>> sortedPrototype {{ctor, GlobalTSTypeRef()}};
        CVector<std::pair<JSHandle<JSTaggedValue>, GlobalTSTypeRef>> signatureVec {};
        for (uint32_t index = 0; index < numOfProps; ++index) {
            JSHandle<JSTaggedValue> key(thread, propType->GetKey(index));
            auto value = GlobalTSTypeRef(propType->GetTypeId(index).GetInt());
            // Usually, abstract methods in abstract class have no specific implementation,
            // and method signatures will be added after class scope.
            // Strategy: ignore abstract method, and rearrange the order of method signature to be at the end.
            bool isSame = JSTaggedValue::SameValue(key, ctor);
            bool isAbs = tsManager->IsAbstractMethod(value);
            if (!isSame && !isAbs) {
                bool isSign = tsManager->IsMethodSignature(value);
                if (LIKELY(!isSign)) {
                    sortedPrototype.emplace_back(std::make_pair(key, value));
                } else {
                    signatureVec.emplace_back(std::make_pair(key, value));
                }
            }
        }

        if (!signatureVec.empty()) {
            sortedPrototype.insert(sortedPrototype.end(), signatureVec.begin(), signatureVec.end());
        }

        uint32_t keysLen = sortedPrototype.size();
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(keysLen);

        for (uint32_t index = 0; index < keysLen; ++index) {
            key.Update(sortedPrototype[index].first);
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);
            if (tsManager->IsGetterSetterFunc(sortedPrototype[index].second)) {
                attributes.SetIsAccessor(true);
            }
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::MIXED);
            attributes.SetOffset(index);
            layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
        }
        hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, keysLen);
        hclass->SetLayout(thread, layout);
        hclass->SetNumberOfProps(keysLen);
    } else {
        // dictionary mode
        hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, 0);  // without in-obj
        hclass->SetIsDictionaryMode(true);
        hclass->SetNumberOfProps(0);
    }

    hclass->SetTS(true);
    hclass->SetClassPrototype(true);
    hclass->SetIsPrototype(true);

    return *hclass;
}

bool TSUnionType::IsEqual(JSHandle<TSUnionType> unionB)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(unionB->GetComponents().IsTaggedArray());

    TaggedArray *unionArrayA = TaggedArray::Cast(TSUnionType::GetComponents().GetTaggedObject());
    TaggedArray *unionArrayB = TaggedArray::Cast(unionB->GetComponents().GetTaggedObject());
    uint32_t unionALength = unionArrayA->GetLength();
    uint32_t unionBLength = unionArrayB->GetLength();
    if (unionALength != unionBLength) {
        return false;
    }
    for (uint32_t unionAIndex = 0; unionAIndex < unionALength; unionAIndex++) {
        int argUnionA = unionArrayA->Get(unionAIndex).GetNumber();
        bool findArgTag = false;
        for (uint32_t unionBIndex = 0; unionBIndex < unionBLength; unionBIndex++) {
            int argUnionB = unionArrayB->Get(unionBIndex).GetNumber();
            if (argUnionA == argUnionB) {
                findArgTag = true;
                break;
            }
        }
        if (!findArgTag) {
            return false;
        }
    }
    return true;
}

GlobalTSTypeRef TSClassType::GetPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                           JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    JSMutableHandle<TSClassType> mutableClassType(thread, classType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutableConstructorType(thread, mutableClassType->GetConstructorType());
    GlobalTSTypeRef propTypeGT = GlobalTSTypeRef::Default();

    while (propTypeGT.IsDefault()) {  // not find
        propTypeGT = TSObjectType::GetPropTypeGT(mutableConstructorType, propName);
        GlobalTSTypeRef classTypeGT = mutableClassType->GetExtensionGT();
        if (classTypeGT.IsDefault()) {  // end of prototype chain
            break;
        }

        JSTaggedValue tmpType = tsManager->GetTSType(classTypeGT).GetTaggedValue();
        if (tmpType.IsUndefined()) {
            return GlobalTSTypeRef::Default();
        }
        mutableClassType.Update(tmpType);
        mutableConstructorType.Update(mutableClassType->GetConstructorType());
    }
    return propTypeGT;
}

GlobalTSTypeRef TSClassType::GetSuperPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                                JSHandle<EcmaString> propName, PropertyType propType)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    JSMutableHandle<TSClassType> mutableClassType(thread, classType.GetTaggedValue());
    GlobalTSTypeRef propTypeGT = GlobalTSTypeRef::Default();
    GlobalTSTypeRef notExistPropGt = kungfu::GateType::UndefinedType().GetGTRef();
    GlobalTSTypeRef superClassTypeGT = mutableClassType->GetExtensionGT();
    if (superClassTypeGT.IsDefault()) {  // end of prototype chain
        return notExistPropGt;
    }
    ASSERT(propType != PropertyType::OTHERS);
    bool isStatic = propType == PropertyType::STATIC;
    mutableClassType.Update(tsManager->GetTSType(superClassTypeGT).GetTaggedValue());
    JSMutableHandle<TSObjectType> mutablePropTypes(thread, isStatic ?
        mutableClassType->GetConstructorType() : mutableClassType->GetPrototypeType());
    while (propTypeGT.IsDefault()) {
        propTypeGT = TSObjectType::GetPropTypeGT(mutablePropTypes, propName);
        GlobalTSTypeRef classTypeGT = mutableClassType->GetExtensionGT();
        if (classTypeGT.IsDefault()) {  // end of prototype chain
            break;
        }
        JSTaggedValue tmpType = tsManager->GetTSType(classTypeGT).GetTaggedValue();
        if (tmpType.IsUndefined()) { // this is for builtin.d.abc
            return GlobalTSTypeRef::Default();
        }
        mutableClassType.Update(tmpType);
        mutablePropTypes.Update(isStatic ?
            mutableClassType->GetConstructorType() : mutableClassType->GetPrototypeType());
    }
    return propTypeGT.IsDefault() ? notExistPropGt : propTypeGT;
}

GlobalTSTypeRef TSClassType::GetNonStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                                    JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();

    JSHandle<TSObjectType> instanceType(thread, classType->GetInstanceType());

    GlobalTSTypeRef propTypeGT = TSObjectType::GetPropTypeGT(instanceType, propName);
    if (!propTypeGT.IsDefault()) {
        return propTypeGT;
    }

    // search on prototype chain
    JSMutableHandle<TSClassType> mutableClassType(thread, classType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutablePrototypeType(thread, classType->GetPrototypeType());
    while (propTypeGT.IsDefault()) {  // not find
        propTypeGT = TSObjectType::GetPropTypeGT(mutablePrototypeType, propName);
        GlobalTSTypeRef classTypeGT = mutableClassType->GetExtensionGT();
        if (classTypeGT.IsDefault()) {  // end of prototype chain
            break;
        }

        JSTaggedValue tmpType = tsManager->GetTSType(classTypeGT).GetTaggedValue();
        if (tmpType.IsUndefined()) {
            return GlobalTSTypeRef::Default();
        }
        mutableClassType.Update(tmpType);
        mutablePrototypeType.Update(mutableClassType->GetPrototypeType());
    }
    return propTypeGT;
}

GlobalTSTypeRef TSClassInstanceType::GetPropTypeGT(JSThread *thread, JSHandle<TSClassInstanceType> classInstanceType,
                                                   JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    GlobalTSTypeRef classTypeGT = classInstanceType->GetClassGT();
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(classTypeGT);

    if (type->IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }

    ASSERT(type->IsTSClassType());
    JSHandle<TSClassType> classType(type);
    GlobalTSTypeRef propTypeGT = TSClassType::GetNonStaticPropTypeGT(thread, classType, propName);
    return propTypeGT;
}

GlobalTSTypeRef TSObjectType::GetPropTypeGT(JSHandle<TSObjectType> objectType, JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSObjLayoutInfo *layout = TSObjLayoutInfo::Cast(objectType->GetObjLayoutInfo().GetTaggedObject());
    uint32_t numOfProps = layout->GetNumOfProperties();
    for (uint32_t i = 0; i < numOfProps; ++i) {
        EcmaString* propKey = EcmaString::Cast(layout->GetKey(i).GetTaggedObject());
        if (!EcmaStringAccessor::StringsAreEqual(propKey, *propName)) {
            continue;
        }

        uint32_t gtRawData = static_cast<uint32_t>(layout->GetTypeId(i).GetInt());
        return GlobalTSTypeRef(gtRawData);
    }

    return GlobalTSTypeRef::Default();
}

GlobalTSTypeRef TSFunctionType::GetParameterTypeGT(int index) const
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray* functionParametersArray = TaggedArray::Cast(GetParameterTypes().GetTaggedObject());
    JSTaggedValue parameterType = functionParametersArray->Get(index);
    ASSERT(parameterType.IsInt());
    uint32_t parameterGTRawData = parameterType.GetInt();
    return GlobalTSTypeRef(parameterGTRawData);
}

GlobalTSTypeRef TSIteratorInstanceType::GetPropTypeGT(JSThread *thread,
    JSHandle<TSIteratorInstanceType> iteratorInstanceType, JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    GlobalTSTypeRef kindGt = iteratorInstanceType->GetKindGT();
    GlobalTSTypeRef elementGt = iteratorInstanceType->GetElementGT();

    JSHandle<JSTaggedValue> tsType = tsManager->GetTSType(kindGt);
    JSHandle<TSObjectType> objType(tsType);
    GlobalTSTypeRef propGt = TSObjectType::GetPropTypeGT(objType, propName);
    if (tsManager->IsTSIterator(kindGt)) {
        GlobalTSTypeRef iteratorFunctionInstance =
            tsManager->GetOrCreateTSIteratorInstanceType(static_cast<TSRuntimeType>(propGt.GetLocalId()), elementGt);
        return iteratorFunctionInstance;
    }

    if (tsManager->IsTSIteratorResult(kindGt)) {
        if (propGt.IsDefault()) {
#ifndef NDEBUG
            JSHandle<JSTaggedValue> valueString = thread->GlobalConstants()->GetHandledValueString();
            ASSERT(EcmaStringAccessor::StringsAreEqual(*propName, EcmaString::Cast(
                valueString.GetTaggedValue().GetTaggedObject())));
#endif
            propGt = elementGt;
        }
        return propGt;
    }
    return GlobalTSTypeRef::Default();
}

GlobalTSTypeRef TSInterfaceType::GetPropTypeGT(JSThread *thread, JSHandle<TSInterfaceType> interfaceType,
                                               JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();

    JSMutableHandle<TSInterfaceType> mutableInterfaceType(thread, interfaceType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutableFieldsType(thread, mutableInterfaceType->GetFields());
    GlobalTSTypeRef propTypeGT = GlobalTSTypeRef::Default();
    propTypeGT = TSObjectType::GetPropTypeGT(mutableFieldsType, propName);

    TaggedArray* extendsArray = TaggedArray::Cast(mutableInterfaceType->GetExtends().GetTaggedObject());
    uint32_t extendsLength = extendsArray->GetLength();

    for (uint32_t index = 0; index < extendsLength; index++) {
        if (!propTypeGT.IsDefault()) {
            return propTypeGT;
        }

        JSTaggedValue extendsValue = extendsArray->Get(index);
        ASSERT(extendsValue.IsInt());
        uint32_t gtRawData = static_cast<uint32_t>(extendsValue.GetInt());
        GlobalTSTypeRef extendsGT = GlobalTSTypeRef(gtRawData);
        JSHandle<JSTaggedValue> extendsType = tsManager->GetTSType(extendsGT);
        if (extendsType->IsUndefined()) {
            return GlobalTSTypeRef::Default();
        }
        ASSERT(extendsType->IsTSType());

        if (extendsType->IsTSClassType()) {
            JSHandle<TSClassType> innerClassType(extendsType);
            propTypeGT = TSClassType::GetNonStaticPropTypeGT(thread, innerClassType, propName);
        } else if (extendsType->IsTSInterfaceType()) {
            JSHandle<TSInterfaceType> extendsInterfaceType(extendsType);
            propTypeGT = TSInterfaceType::GetPropTypeGT(thread, extendsInterfaceType, propName);
        }
    }

    return propTypeGT;
}
} // namespace panda::ecmascript
