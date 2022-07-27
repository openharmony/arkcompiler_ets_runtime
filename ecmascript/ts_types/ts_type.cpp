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
#include "ts_type.h"

#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
JSHClass *TSObjectType::GetOrCreateHClass(JSThread *thread, JSHandle<TSObjectType> objectType)
{
    JSTaggedValue mayBeHClass = objectType->GetHClass();
    if (mayBeHClass.IsJSHClass()) {
        return JSHClass::Cast(mayBeHClass.GetTaggedObject());
    }
    JSHandle<TSObjLayoutInfo> propTypeInfo(thread, objectType->GetObjLayoutInfo());
    JSHClass *hclass = objectType->CreateHClassByProps(thread, propTypeInfo);
    objectType->SetHClass(thread, JSTaggedValue(hclass));

    return hclass;
}

JSHClass *TSObjectType::CreateHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t numOfProps = propType->NumberOfElements();
    if (numOfProps > PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES) {
        LOG_ECMA(ERROR) << "TSobject type has too many keys and cannot create hclass";
        UNREACHABLE();
    }

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
    JSHandle<JSHClass> hclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, numOfProps);
    hclass->SetLayout(thread, layout);
    hclass->SetNumberOfProps(numOfProps);
    hclass->SetAOT(true);

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
        classTypeGT = mutableClassType->GetExtensionGT();
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

GlobalTSTypeRef TSObjectType::GetPropTypeGT(JSHandle<TSObjectType> objectType, JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSObjLayoutInfo *layout = TSObjLayoutInfo::Cast(objectType->GetObjLayoutInfo().GetTaggedObject());
    for (uint32_t i = 0; i < layout->NumberOfElements(); ++i) {
        EcmaString* propKey = EcmaString::Cast(layout->GetKey(i).GetTaggedObject());
        if (!EcmaString::StringsAreEqual(propKey, *propName)) {
            continue;
        }

        uint32_t gtRawData = layout->GetTypeId(i).GetInt();
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
} // namespace panda::ecmascript