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

#include "ecmascript/global_env.h"
#include "ecmascript/subtyping_operator-inl.h"
#include "ecmascript/vtable.h"

namespace panda::ecmascript {
bool SubtypingOperator::CheckBaseClass(const JSThread *thread, const JSHandle<TSClassType> &classType)
{
    // the base class type does not need to check rules 2 and 4, because Object has no local properties
    TSObjectType *it = TSObjectType::Cast(classType->GetInstanceType().GetTaggedObject());
    TSObjLayoutInfo *itLayout = TSObjLayoutInfo::Cast(it->GetObjLayoutInfo().GetTaggedObject());
    TSObjectType *pt = TSObjectType::Cast(classType->GetPrototypeType().GetTaggedObject());
    TSObjLayoutInfo *ptLayout = TSObjLayoutInfo::Cast(pt->GetObjLayoutInfo().GetTaggedObject());

    // itLayout  -> local, ptLayout  -> vtable
    if (!SubtypingCondition(thread, itLayout, ptLayout, ConditionType::NO_OVERLAP_SUB_LOCAL_SUB_VTABLE )) {
        return false;
    }

    JSHandle<panda::ecmascript::GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHClass *oProtoHClass = JSHClass::Cast(env->GetObjectFunctionPrototypeClass()->GetTaggedObject());

    if (!SubtypingCondition(thread, ptLayout, oProtoHClass, ConditionType::SUB_VTABLE_CONTAIN_SUP_VTABLE) ||
        !SubtypingCondition(thread, itLayout, oProtoHClass, ConditionType::NO_OVERLAP_SUP_VTABLE_SUB_LOCAL)) {
        return false;
    }

    return true;
}

bool SubtypingOperator::CheckSubtyping(const JSThread *thread, const JSHandle<TSClassType> &classType)
{
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    JSHandle<TSClassType> eClassType = tsManager->GetExtendedClassType(classType);
    JSHClass *eIhc = JSHClass::Cast(tsManager->GetInstanceTSHClass(eClassType).GetTaggedObject());
    WeakVector *eSupers = WeakVector::Cast(eIhc->GetSupers().GetTaggedObject());

    // if the supers of extended IHClass is empty, means the chain is broken
    // will be overwritten by IsTSChainInfoFilled() in PR Part2.
    if (eSupers->Empty() || eIhc->GetLevel() + 1 == MAX_LEVEL) {
        return false;
    }

    TSObjectType *it = TSObjectType::Cast(classType->GetInstanceType().GetTaggedObject());
    TSObjLayoutInfo *itLayout = TSObjLayoutInfo::Cast(it->GetObjLayoutInfo().GetTaggedObject());
    TSObjectType *pt = TSObjectType::Cast(classType->GetPrototypeType().GetTaggedObject());
    TSObjLayoutInfo *ptLayout = TSObjLayoutInfo::Cast(pt->GetObjLayoutInfo().GetTaggedObject());

    TSObjectType *eIt = TSObjectType::Cast(eClassType->GetInstanceType().GetTaggedObject());
    TSObjLayoutInfo *eItLayout = TSObjLayoutInfo::Cast(eIt->GetObjLayoutInfo().GetTaggedObject());
    VTable *eVtable = VTable::Cast(eIhc->GetVTable().GetTaggedObject());

    // itLayout  -> local, eItLayout -> extended local
    // ptLayout  -> vtable, eVtable   -> extended vtable
    if (!SubtypingCondition(thread, itLayout, ptLayout, ConditionType::NO_OVERLAP_SUB_LOCAL_SUB_VTABLE ) ||
        !SubtypingCondition(thread, itLayout, eItLayout, ConditionType::SUB_LOCAL_CONTAIN_SUP_LOCAL) ||
        !SubtypingCondition(thread, ptLayout, eVtable, ConditionType::SUB_VTABLE_CONTAIN_SUP_VTABLE) ||
        !SubtypingCondition(thread, ptLayout, eItLayout, ConditionType::NO_OVERLAP_SUP_LOCAL_SUB_VTABLE) ||
        !SubtypingCondition(thread, itLayout, eVtable, ConditionType::NO_OVERLAP_SUP_VTABLE_SUB_LOCAL)) {
        return false;
    }

    return true;
}

void SubtypingOperator::MergeClassField(const JSThread *thread, const JSHandle<TSClassType> &classType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();

    JSHandle<TSClassType> eClassType = tsManager->GetExtendedClassType(classType);
    JSHandle<TSObjectType> field(thread, classType->GetInstanceType());
    TSObjectType *eField = TSObjectType::Cast(eClassType->GetInstanceType());

    JSHandle<TSObjLayoutInfo> layout(thread, field->GetObjLayoutInfo());
    JSHandle<TSObjLayoutInfo> eLayout(thread, eField->GetObjLayoutInfo());

    uint32_t numSelfTypes = layout->GetNumOfProperties();
    uint32_t numExtendTypes = eLayout->GetNumOfProperties();
    uint32_t numTypes = numTypes = numSelfTypes + numExtendTypes;
    JSHandle<TSObjLayoutInfo> newLayout = factory->CreateTSObjLayoutInfo(numTypes);

    for (uint32_t index = 0; index < numExtendTypes; index++) {
        JSTaggedValue key = eLayout->GetKey(index);
        JSTaggedValue type = eLayout->GetTypeId(index);
        newLayout->AddKeyAndType(thread, key, type);
    }

    for (uint32_t index = 0; index < numSelfTypes; index++) {
        JSTaggedValue key = layout->GetKey(index);
        JSTaggedValue type = layout->GetTypeId(index);
        if (eLayout->Find(key)) {
            continue;
        }
        newLayout->AddKeyAndType(thread, key, type);
    }

    field->SetObjLayoutInfo(thread, newLayout);
    classType->SetHasLinked(true);
}

void SubtypingOperator::FillTSInheritInfo(JSThread *thread, const JSHandle<TSClassType> &classType,
                                          const JSHandle<JSHClass> &ihcHandle)
{
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    JSObject *prototype = JSObject::Cast(ihcHandle->GetProto());
    JSHandle<JSHClass> phcHandle(thread, prototype->GetJSHClass());
    JSHandle<JSHClass> eIhcHandle(thread, JSTaggedValue::Undefined());

    if (classType->IsBaseClassType()) {
        GenVTable(thread, ihcHandle, phcHandle, eIhcHandle);
        ihcHandle->SetLevel(0);
        AddSuper(thread, ihcHandle, eIhcHandle);
    } else {
        // get extended instance TSHclass
        JSHandle<TSClassType> eClassType = tsManager->GetExtendedClassType(classType);
        JSTaggedValue eIhc = tsManager->GetInstanceTSHClass(eClassType);
        eIhcHandle = JSHandle<JSHClass>(thread, eIhc);

        uint8_t eIhcLevel = eIhcHandle->GetLevel();
        JSHandle<JSObject> ePrototype(thread, eIhcHandle->GetProto());

        GenVTable(thread, ihcHandle, phcHandle, eIhcHandle);
        ihcHandle->SetLevel(eIhcLevel + 1);
        AddSuper(thread, ihcHandle, eIhcHandle);
    }
}

void SubtypingOperator::GenVTable(const JSThread *thread, const JSHandle<JSHClass> &ihcHandle,
                                  const JSHandle<JSHClass> &phcHandle, const JSHandle<JSHClass> &eIhcHandle)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> owner(thread, ihcHandle->GetProto());
    uint32_t propNumber = phcHandle->NumberOfProps();
    JSMutableHandle<VTable> eVTable(thread, JSTaggedValue::Undefined());
    uint32_t ePropNumber = 0;
    if (!eIhcHandle.GetTaggedValue().IsUndefined()) {
        eVTable.Update(eIhcHandle->GetVTable());
        ePropNumber = eVTable->GetNumberOfTuples();
    }

    uint32_t loc = 0;
    uint32_t finalLength = propNumber + ePropNumber;
    JSHandle<VTable> vtable = factory->NewVTable(finalLength);

    for (uint32_t index = 0; index < ePropNumber; ++index) {
        VTable::Tuple tuple = eVTable->GetTuple(thread, index);
        vtable->SetByIndex(thread, loc++, tuple);
    }

    for (uint32_t index = 0; index < propNumber; ++index) {
        VTable::Tuple tuple = VTable::CreateTuple(thread, phcHandle.GetTaggedValue(), owner, index);
        JSTaggedValue nameVal = tuple.GetItem(VTable::TupleItem::NAME).GetTaggedValue();
        // When the vtable and the parent class vtable have the same name attribute,
        // the attribute of the parent class vtable should be overwritten
        int tIndex = vtable->GetTupleIndexByName(nameVal);
        if (tIndex == -1) {
            vtable->SetByIndex(thread, loc++, tuple);
        } else {
            vtable->SetByIndex(thread, tIndex, tuple);
        }
    }

    if (loc != finalLength) {
        vtable->Trim(thread, loc);
    }
    ihcHandle->SetVTable(thread, vtable);
}

void SubtypingOperator::AddSuper(const JSThread *thread, const JSHandle<JSHClass> &iHClass,
                                 const JSHandle<JSHClass> &superHClass)
{
    JSHandle<WeakVector> supersHandle(thread, iHClass->GetSupers());
    if (!superHClass.GetTaggedValue().IsUndefined()) {
        JSHandle<WeakVector> old(thread, superHClass->GetSupers());
        supersHandle = WeakVector::Copy(thread, old, old->Full());
    }

    JSHandle<JSTaggedValue> iHClassVal(iHClass);
    JSHandle<WeakVector> newSupers = WeakVector::Append(thread, supersHandle,
        iHClassVal, WeakVector::ElementType::WEAK);
    iHClass->SetSupers(thread, newSupers);
}
}  // namespace panda::ecmascript