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
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
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
                                           JSHandle<JSTaggedValue> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
    JSMutableHandle<TSClassType> mutableClassType(thread, classType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutableConstructorType(thread, mutableClassType->GetConstructorType());
    GlobalTSTypeRef propTypeGT = GlobalTSTypeRef::Default();

    while (propTypeGT.IsDefault()) {  // not find
        propTypeGT = TSObjectType::GetPropTypeGT(thread, mutableConstructorType, propName);
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
                                                JSHandle<JSTaggedValue> propName, PropertyType propType)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
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
        propTypeGT = TSObjectType::GetPropTypeGT(thread, mutablePropTypes, propName);
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
                                                    JSHandle<JSTaggedValue> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
    JSHandle<TSObjectType> instanceType(thread, classType->GetInstanceType());
    GlobalTSTypeRef propTypeGT = TSObjectType::GetPropTypeGT(thread, instanceType, propName);
    if (!propTypeGT.IsDefault()) {
        return propTypeGT;
    }

    // search on prototype chain
    JSMutableHandle<TSClassType> mutableClassType(thread, classType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutablePrototypeType(thread, classType->GetPrototypeType());
    while (propTypeGT.IsDefault()) {  // not find
        propTypeGT = TSObjectType::GetPropTypeGT(thread, mutablePrototypeType, propName);
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

void TSClassType::UpdateNonStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                            JSHandle<JSTaggedValue> propName, GlobalTSTypeRef newGT)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<TSObjectType> instanceType(thread, classType->GetInstanceType());
    TSObjectType::UpdatePropTypeGT(thread, instanceType, propName, newGT);
}

void TSClassType::UpdateStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                         JSHandle<JSTaggedValue> propName, GlobalTSTypeRef newGT)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
    JSMutableHandle<TSClassType> mutableClassType(thread, classType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutableConstructorType(thread, mutableClassType->GetConstructorType());
    bool hasUpdate = false;
    while (!hasUpdate) {
        hasUpdate = TSObjectType::UpdatePropTypeGT(thread, mutableConstructorType, propName, newGT);
        GlobalTSTypeRef classTypeGT = mutableClassType->GetExtensionGT();
        if (classTypeGT.IsDefault()) {
            break;
        }

        JSTaggedValue tmpType = tsManager->GetTSType(classTypeGT).GetTaggedValue();
        if (tmpType.IsUndefined()) {
            break;
        }
        mutableClassType.Update(tmpType);
        mutableConstructorType.Update(mutableClassType->GetConstructorType());
    }
}

GlobalTSTypeRef TSClassInstanceType::GetPropTypeGT(JSThread *thread, JSHandle<TSClassInstanceType> classInstanceType,
                                                   JSHandle<JSTaggedValue> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
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

GlobalTSTypeRef TSClassInstanceType::GetIndexSignType(JSThread *thread,
                                                      const JSHandle<TSClassInstanceType> &classInstanceType,
                                                      const uint32_t typeId)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
    GlobalTSTypeRef classTypeGT = classInstanceType->GetClassGT();
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(classTypeGT);
    // Es2abc no longer obeys that what is stored in classinstance types must be classtype,
    // Retains the follwoing branch until they change it.
    if (!type->IsTSClassType()) {
        return GlobalTSTypeRef::Default();
    }
    JSHandle<TSClassType> classType(type);
    if (classType->GetIndexSigns().IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }

    JSHandle<TSObjLayoutInfo> indexSignInfo(thread, classType->GetIndexSigns());
    JSTaggedValue valueType = indexSignInfo->TryGetTypeByIndexSign(typeId);
    if (valueType.IsInt()) {
        return GlobalTSTypeRef(static_cast<uint32_t>(valueType.GetInt()));
    }
    return GlobalTSTypeRef::Default();
}

GlobalTSTypeRef TSObjectType::GetPropTypeGT(JSThread *thread, JSHandle<TSObjectType> objectType,
                                            JSHandle<JSTaggedValue> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<TSObjLayoutInfo> layout(thread, objectType->GetObjLayoutInfo().GetTaggedObject());
    uint32_t numOfProps = layout->GetNumOfProperties();
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < numOfProps; ++i) {
        propKey.Update(layout->GetKey(i));
        if (!JSTaggedValue::Equal(thread, propName, propKey)) {
            continue;
        }
        uint32_t gtRawData = static_cast<uint32_t>(layout->GetTypeId(i).GetInt());
        return GlobalTSTypeRef(gtRawData);
    }

    return GlobalTSTypeRef::Default();
}

bool TSObjectType::UpdatePropTypeGT(JSThread *thread, JSHandle<TSObjectType> objectType,
                                    JSHandle<JSTaggedValue> propName, GlobalTSTypeRef newGT)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<TSObjLayoutInfo> layout(thread, objectType->GetObjLayoutInfo().GetTaggedObject());
    int propIdx = layout->GetElementIndexByKey(propName.GetTaggedValue());
    if (!TSObjLayoutInfo::IsValidIndex(propIdx)) {
        return false;
    }
    uint32_t gtRawData = static_cast<uint32_t>(layout->GetTypeId(propIdx).GetInt());
    if (GlobalTSTypeRef(gtRawData).IsDefault()) {
        layout->SetTypeId(thread, propIdx, JSTaggedValue(newGT.GetType()));
        return true;
    }
    return false;
}

GlobalTSTypeRef TSObjectType::GetIndexSignType(JSThread *thread, const JSHandle<TSObjectType> &objectType,
                                               const uint32_t typeId)
{
    if (objectType->GetIndexSigns().IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<TSObjLayoutInfo> indexSignInfo(thread, objectType->GetIndexSigns());
    JSTaggedValue valueType = indexSignInfo->TryGetTypeByIndexSign(typeId);
    if (valueType.IsInt()) {
        return GlobalTSTypeRef(static_cast<uint32_t>(valueType.GetInt()));
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
    JSHandle<TSIteratorInstanceType> iteratorInstanceType, JSHandle<JSTaggedValue> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
    GlobalTSTypeRef kindGt = iteratorInstanceType->GetKindGT();
    GlobalTSTypeRef elementGt = iteratorInstanceType->GetElementGT();

    JSHandle<JSTaggedValue> tsType = tsManager->GetTSType(kindGt);
    JSHandle<TSObjectType> objType(tsType);
    GlobalTSTypeRef propGt = TSObjectType::GetPropTypeGT(thread, objType, propName);
    if (tsManager->IsTSIterator(kindGt)) {
        GlobalTSTypeRef iteratorFunctionInstance =
            tsManager->GetOrCreateTSIteratorInstanceType(static_cast<TSRuntimeType>(propGt.GetLocalId()), elementGt);
        return iteratorFunctionInstance;
    }

    if (tsManager->IsTSIteratorResult(kindGt)) {
        if (propGt.IsDefault()) {
#ifndef NDEBUG
            ASSERT(propName->IsString());
            JSHandle<JSTaggedValue> valueString = thread->GlobalConstants()->GetHandledValueString();
            ASSERT(EcmaStringAccessor::StringsAreEqual(*JSHandle<EcmaString>::Cast(propName), EcmaString::Cast(
                valueString.GetTaggedValue().GetTaggedObject())));
#endif
            propGt = elementGt;
        }
        return propGt;
    }
    return GlobalTSTypeRef::Default();
}

GlobalTSTypeRef TSInterfaceType::GetPropTypeGT(JSThread *thread, JSHandle<TSInterfaceType> interfaceType,
                                               JSHandle<JSTaggedValue> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();

    JSMutableHandle<TSInterfaceType> mutableInterfaceType(thread, interfaceType.GetTaggedValue());
    JSMutableHandle<TSObjectType> mutableFieldsType(thread, mutableInterfaceType->GetFields());
    GlobalTSTypeRef propTypeGT = GlobalTSTypeRef::Default();
    propTypeGT = TSObjectType::GetPropTypeGT(thread, mutableFieldsType, propName);

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

GlobalTSTypeRef TSInterfaceType::GetIndexSignType(JSThread *thread, const JSHandle<TSInterfaceType> &interfaceType,
                                                  const uint32_t typeId)
{
    if (interfaceType->GetIndexSigns().IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<TSObjLayoutInfo> indexSignInfo(thread, interfaceType->GetIndexSigns());
    JSTaggedValue valueType = indexSignInfo->TryGetTypeByIndexSign(typeId);
    if (valueType.IsInt()) {
        return GlobalTSTypeRef(static_cast<uint32_t>(valueType.GetInt()));
    }
    return GlobalTSTypeRef::Default();
}


void TSNamespaceType::AddKeyAndValue(const JSThread *thread, const JSHandle<TSNamespaceType> &namespaceType,
                                     const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TSObjLayoutInfo> propLayout(thread, namespaceType->GetPropertyType());
    if (propLayout->Find(key.GetTaggedValue())) {
        return;
    }
    JSHandle<TSObjLayoutInfo> newPropLayout = TSObjLayoutInfo::PushBack(thread, propLayout, key, value);
    namespaceType->SetPropertyType(thread, newPropLayout.GetTaggedValue());
}

GlobalTSTypeRef TSNamespaceType::GetPropTypeGT(JSThread *thread, const JSHandle<TSNamespaceType> &namespaceType,
                                               const JSHandle<JSTaggedValue> &propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (namespaceType->GetPropertyType().IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }

    JSHandle<JSTaggedValue> properties(thread, namespaceType->GetPropertyType());

    if (properties->IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }

    JSHandle<TSObjLayoutInfo> layout(thread, namespaceType->GetPropertyType().GetTaggedObject());
    uint32_t numOfProps = layout->GetNumOfProperties();
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < numOfProps; ++i) {
        propKey.Update(layout->GetKey(i));
        if (!JSTaggedValue::Equal(thread, propName, propKey)) {
            continue;
        }
        uint32_t gtRawData = static_cast<uint32_t>(layout->GetTypeId(i).GetInt());
        return GlobalTSTypeRef(gtRawData);
    }
    return GlobalTSTypeRef::Default();
}
} // namespace panda::ecmascript
