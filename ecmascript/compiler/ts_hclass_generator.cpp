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

#include "ecmascript/compiler/ts_hclass_generator.h"
#include "ecmascript/subtyping_operator.h"
#include "ecmascript/jspandafile/class_info_extractor.h"

namespace panda::ecmascript::kungfu {
using ClassInfoExtractor = panda::ecmascript::ClassInfoExtractor;
void TSHClassGenerator::GenerateTSHClasses() const
{
    const JSThread *thread = tsManager_->GetThread();
    const std::set<GlobalTSTypeRef> &collectedGT = GetCollectedGT();
    for (const auto &gt : collectedGT) {
        JSHandle<JSTaggedValue> tsType = tsManager_->GetTSType(gt);
        if (tsType->IsUndefined()) {
            continue;
        }
        ASSERT(tsType->IsTSClassType());
        JSHandle<TSClassType> classType(tsType);
        if (tsManager_->HasTSHClass(classType)) {
            continue;
        } else if (tsManager_->IsUserDefinedClassTypeKind(gt)) {
            RecursiveGenerate(classType);
        } else if (!classType->GetHasLinked()) {
            SubtypingOperator::MergeClassField(thread, classType);
        }
    }
}

void TSHClassGenerator::UpdateTSHClassFromPGO(const kungfu::GateType &type, const PGOHClassLayoutDesc &desc,
    bool enableOptTrackField) const
{
    DISALLOW_GARBAGE_COLLECTION;
    auto hclassValue = tsManager_->GetTSHClass(type);
    if (!hclassValue.IsJSHClass()) {
        return;
    }

    tsManager_->InsertPtToGtMap(desc.GetClassType(), type);
    if (!enableOptTrackField) {
        return;
    }
    if (tsManager_->IsInSkipTrackFieldSet(desc.GetClassType())) {
        return;
    }

    std::vector<JSHClass *> superHClasses;
    kungfu::GateType current = type;
    while (tsManager_->GetSuperGateType(current)) {
        auto superHClassValue = tsManager_->GetTSHClass(current);
        if (!superHClassValue.IsJSHClass()) {
            break;
        }
        superHClasses.emplace_back(JSHClass::Cast(superHClassValue.GetTaggedObject()));
    }

    auto hclass = JSHClass::Cast(hclassValue.GetTaggedObject());
    const JSThread *thread = tsManager_->GetThread();
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    int numOfProps = hclass->NumberOfProps();
    for (int i = 0; i < numOfProps; i++) {
        auto key = layoutInfo->GetKey(i);
        if (!key.IsString()) {
            continue;
        }
        auto keyString = EcmaStringAccessor(key).ToCString();
        PGOHandler newHandler;
        if (desc.FindDescWithKey(keyString, newHandler)) {
            auto attr = layoutInfo->GetAttr(i);
            if (newHandler.SetAttribute(attr)) {
                hclass->SetIsAllTaggedProp(false);
            }
            layoutInfo->SetNormalAttr(thread, i, attr);

            // Update super class representation
            for (auto superHClass : superHClasses) {
                int entry = JSHClass::FindPropertyEntry(thread, superHClass, key);
                if (entry == -1) {
                    continue;
                }
                auto superLayout = LayoutInfo::Cast(superHClass->GetLayout().GetTaggedObject());
                PropertyAttributes superAttr = superLayout->GetAttr(entry);
                if (newHandler.SetAttribute(superAttr)) {
                    superHClass->SetIsAllTaggedProp(false);
                }
                superLayout->SetNormalAttr(thread, entry, superAttr);
            }
        }
    }
}

void TSHClassGenerator::RecursiveGenerate(const JSHandle<TSClassType> &classType) const
{
    if (!classType->IsBaseClassType()) {
        JSHandle<TSClassType> extendedClassType(tsManager_->GetExtendedClassType(classType));
        if (!tsManager_->HasTSHClass(extendedClassType)) {
            RecursiveGenerate(extendedClassType);
        }
    }

    JSThread *thread = tsManager_->GetThread();
    bool passed = false;
    if (classType->IsBaseClassType()) {
        passed = SubtypingOperator::CheckBaseClass(thread, classType);
    } else {
        passed = SubtypingOperator::CheckSubtyping(thread, classType);
    }

    classType->SetHasPassedSubtypingCheck(passed);

    if (passed) {
        if (!classType->IsBaseClassType()) {
            SubtypingOperator::MergeClassField(thread, classType);
        }
        JSHandle<JSHClass> ihclass = Generate(classType);
        SubtypingOperator::FillTSInheritInfo(thread, classType, ihclass);
        return;
    }
    Generate(classType);
}

JSHandle<JSHClass> TSHClassGenerator::Generate(const JSHandle<TSClassType> &classType) const
{
    const JSThread *thread = tsManager_->GetThread();
    JSHandle<JSHClass> ihclass = CreateHClass(thread, classType, Kind::INSTANCE);
    JSHandle<JSHClass> phclass = CreateHClass(thread, classType, Kind::PROTOTYPE);
    JSHandle<JSHClass> chclass = CreateHClass(thread, classType, Kind::CONSTRUCTOR);
    JSHandle<JSObject> prototype = thread->GetEcmaVM()->GetFactory()->NewJSObject(phclass);
    ihclass->SetProto(thread, prototype);

    GlobalTSTypeRef gt = classType->GetGT();
    tsManager_->AddInstanceTSHClass(gt, ihclass);
    tsManager_->AddConstructorTSHClass(gt, chclass);
    return ihclass;
}

JSHandle<JSHClass> TSHClassGenerator::CreateHClass(const JSThread *thread, const JSHandle<TSClassType> &classType,
                                                   Kind kind) const
{
    JSHandle<JSHClass> hclass(thread->GlobalConstants()->GetHandledUndefined());
    switch (kind) {
        case Kind::INSTANCE: {
            hclass = CreateIHClass(thread, classType);
            break;
        }
        case Kind::PROTOTYPE: {
            hclass = CreatePHClass(thread, classType);
            break;
        }
        case Kind::CONSTRUCTOR: {
            hclass = CreateCHClass(thread, classType);
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    return hclass;
}

JSHandle<JSHClass> TSHClassGenerator::CreateIHClass(const JSThread *thread,
                                                    const JSHandle<TSClassType> &classType) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<TSObjectType> instanceType(thread, classType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> tsLayout(thread, instanceType->GetObjLayoutInfo());
    uint32_t numOfProps = tsLayout->GetNumOfProperties();
    JSHandle<JSHClass> hclass;
    if (LIKELY(numOfProps <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)) {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(numOfProps);
        for (uint32_t index = 0; index < numOfProps; ++index) {
            JSTaggedValue tsPropKey = tsLayout->GetKey(index);
            key.Update(tsPropKey);
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default();
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::NONE);
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

    return hclass;
}

JSHandle<JSHClass> TSHClassGenerator::CreatePHClass(const JSThread *thread,
                                                    const JSHandle<TSClassType> &classType) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<TSObjectType> prototypeType(thread, classType->GetPrototypeType());
    JSHandle<TSObjLayoutInfo> tsLayout(thread, prototypeType->GetObjLayoutInfo());
    uint32_t numOfProps = tsLayout->GetNumOfProperties();
    JSHandle<JSHClass> hclass;
    if (LIKELY(numOfProps <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)) {
        TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        JSHandle<JSTaggedValue> ctor = globalConst->GetHandledConstructorString();
        CVector<std::pair<JSHandle<JSTaggedValue>, GlobalTSTypeRef>> sortedPrototype {{ctor, GlobalTSTypeRef()}};
        CUnorderedMap<std::string, uint32_t> keysMap;
        for (uint32_t index = 0; index < numOfProps; ++index) {
            JSHandle<JSTaggedValue> key(thread, tsLayout->GetKey(index));
            auto value = GlobalTSTypeRef(tsLayout->GetTypeId(index).GetInt());
            // Usually, abstract methods in abstract class have no specific implementation,
            // and method signatures will be added after class scope.
            // Strategy: ignore abstract method, and rearrange the order of method signature to be at the end.
            bool isSame = JSTaggedValue::SameValue(key, ctor);
            bool isAbs = tsManager->IsAbstractMethod(value);
            if (!isSame && !isAbs) {
                bool isSign = tsManager->IsMethodSignature(value);
                if (isSign) {
                    continue;
                }
                std::string keyStr = EcmaStringAccessor(key.GetTaggedValue()).ToStdString();
                auto it = keysMap.find(keyStr);
                if (it != keysMap.end()) {
                    sortedPrototype[it->second] = std::make_pair(key, value);
                    continue;
                }
                keysMap[keyStr] = sortedPrototype.size();
                sortedPrototype.emplace_back(std::make_pair(key, value));
            }
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
            attributes.SetRepresentation(Representation::NONE);
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

    return hclass;
}

JSHandle<JSHClass> TSHClassGenerator::CreateCHClass(const JSThread *thread,
                                                    const JSHandle<TSClassType> &classType) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<TSObjectType> constructorType(thread, classType->GetConstructorType());
    JSHandle<TSObjLayoutInfo> tsLayout(thread, constructorType->GetObjLayoutInfo());
    uint32_t numOfProps = tsLayout->GetNumOfProperties() + ClassInfoExtractor::STATIC_RESERVED_LENGTH;
    JSHandle<JSHClass> hclass;
    uint32_t functionFirstIndex = numOfProps;
    uint32_t numNonStaticFunc = 0;
    bool hasFunction = false;
    if (LIKELY(numOfProps <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)) {
        TSManager *tsManager = thread->GetCurrentEcmaContext()->GetTSManager();
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(numOfProps);
        for (uint32_t index = 0; index < numOfProps; ++index) {
            JSTaggedValue tsPropKey;
            PropertyAttributes attributes;
            switch (index) {
                case ClassInfoExtractor::LENGTH_INDEX:
                    attributes = PropertyAttributes::Default(false, false, true);
                    tsPropKey = globalConst->GetLengthString();
                    break;
                case ClassInfoExtractor::NAME_INDEX:
                    attributes = PropertyAttributes::Default(false, false, true);
                    tsPropKey = globalConst->GetNameString();
                    break;
                case ClassInfoExtractor::PROTOTYPE_INDEX:
                    attributes = PropertyAttributes::DefaultAccessor(false, false, false);
                    tsPropKey = globalConst->GetPrototypeString();
                    break;
                default:
                    attributes = PropertyAttributes::Default(true, false, true);
                    tsPropKey = tsLayout->GetKey(index - ClassInfoExtractor::STATIC_RESERVED_LENGTH);
                    JSTaggedValue typeId = tsLayout->GetTypeId(index - ClassInfoExtractor::STATIC_RESERVED_LENGTH);
                    GlobalTSTypeRef gt(static_cast<uint32_t>(typeId.GetNumber()));
                    if (!tsManager->IsFunctionTypeKind(gt)) {
                        continue;
                    }
                    if (!hasFunction) {
                        hasFunction = true;
                        functionFirstIndex = index;
                        numNonStaticFunc = functionFirstIndex - ClassInfoExtractor::STATIC_RESERVED_LENGTH;
                    }
                    if (tsManager->IsGetterSetterFunc(gt)) {
                        attributes.SetIsAccessor(true);
                    }
                    break;
            }
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(JSHandle<JSTaggedValue>(thread, tsPropKey)),
                         "Key is not a property key");
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::NONE);
            attributes.SetOffset(index - numNonStaticFunc);
            layout->AddKey(thread, index - numNonStaticFunc, tsPropKey, attributes);
        }
        uint32_t numStaticFunc = numOfProps - functionFirstIndex;
        for (uint32_t index = ClassInfoExtractor::STATIC_RESERVED_LENGTH; index < functionFirstIndex; index++) {
            JSTaggedValue tsPropKey = tsLayout->GetKey(index - ClassInfoExtractor::STATIC_RESERVED_LENGTH);
            PropertyAttributes attributes = PropertyAttributes::Default();
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::NONE);
            attributes.SetOffset(index + numStaticFunc);
            layout->AddKey(thread, index + numStaticFunc, tsPropKey, attributes);
        }
        hclass = factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, numOfProps);
        hclass->SetLayout(thread, layout);
        hclass->SetNumberOfProps(numOfProps);
    } else {
        // dictionary mode
        hclass = factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, 0);  // without in-obj
        hclass->SetIsDictionaryMode(true);
        hclass->SetNumberOfProps(0);
    }

    hclass->SetTS(true);
    hclass->SetClassConstructor(true);
    hclass->SetConstructor(true);

    return hclass;
}
}  // namespace panda::ecmascript::kungfu
