/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/aot_snapshot/snapshot_constantpool_data.h"

#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript::kungfu {
uint64_t BaseSnapshotInfo::GetItemKey(uint32_t constantPoolId, uint32_t constantPoolIdx)
{
    uint64_t result = constantPoolId;
    result = result << CONSTPOOL_MASK;
    result |= constantPoolIdx;
    return result;
}

bool BaseSnapshotInfo::TryGetABCId(ApEntityId &abcId)
{
    return pfDecoder_->GetABCIdByJSPandaFile(jsPandaFile_, abcId);
}

JSHandle<JSTaggedValue> BaseSnapshotInfo::TryGetIHClass(ProfileType rootType, ProfileType childType,
    const ItemData &data, const JSHandle<TaggedArray> &properties, const SnapshotGlobalData &globalData) const
{
    JSHandle<JSTaggedValue> ihc = TryGetHClass(rootType, childType);
    if (ihc->IsUndefined()) {
        PGOTypeLocation loc(jsPandaFile_, data.methodOffset_, data.bcIndex_);
        ihc = TryGetHClassByPGOTypeLocation(loc);
        if (ihc->IsUndefined()) {
            ihc = TryGetHClassFromCached(properties, globalData);
        }
    }
    return ihc;
}

JSHandle<JSTaggedValue> BaseSnapshotInfo::TryGetHClass(ProfileType rootType, ProfileType childType) const
{
    PGOTypeManager* ptManager = thread_->GetEcmaVM()->GetPTManager();
    JSTaggedValue hclass = ptManager->QueryHClass(rootType, childType);
    return JSHandle<JSTaggedValue>(thread_, hclass);
}

JSHandle<JSTaggedValue> BaseSnapshotInfo::TryGetHClassByPGOTypeLocation(PGOTypeLocation loc) const
{
    PGOTypeManager* ptManager = thread_->GetEcmaVM()->GetPTManager();
    ProfileType  pt = ptManager->GetRootIdByLocation(loc);
    return TryGetHClass(pt, pt);
}

JSHandle<JSTaggedValue> BaseSnapshotInfo::TryGetHClassFromCached(const JSHandle<TaggedArray> &properties,
                                                                 const SnapshotGlobalData &globalData) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<JSTaggedValue> maybeCache(thread_, globalData.GetObjectLiteralHClassCache(thread_));
    if (maybeCache->IsTaggedArray()) {
        size_t length = properties->GetLength();
        uint32_t propsLen = 0;
        for (size_t i = 0; i < length; i += 2) { // 2: skip a pair of key and value
            if (properties->Get(thread_, i).IsHole()) {
                break;
            }
            propsLen++;
        }

        if (!ObjectFactory::CanObjectLiteralHClassCache(propsLen)) {
            return thread_->GlobalConstants()->GetHandledUndefined();
        }

        JSHandle<TaggedArray> hclassCacheArr = JSHandle<TaggedArray>::Cast(maybeCache);
        JSTaggedValue maybeHClass = hclassCacheArr->Get(thread_, propsLen);
        if (!maybeHClass.IsJSHClass()) {
            return thread_->GlobalConstants()->GetHandledUndefined();
        }
        JSHClass *newClass = JSHClass::Cast(maybeHClass.GetTaggedObject());
        JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());

        for (size_t fieldOffset = 0; fieldOffset < propsLen; fieldOffset++) {
            key.Update(properties->Get(thread_, fieldOffset * 2)); // 2 : pair of key and value
            ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
            PropertyAttributes attributes = PropertyAttributes::Default();
            auto value = properties->Get(thread_, fieldOffset * 2 + 1);
            if (value.IsAccessor()) {  // 2: Meaning to double
                attributes.SetIsAccessor(true);
            }
            attributes.SetIsInlinedProps(true);
            attributes.SetRepresentation(Representation::TAGGED);
            attributes.SetOffset(fieldOffset);

            auto metadata = JSTaggedValue(attributes.GetPropertyMetaData());
            auto rep = PropertyAttributes::TranslateToRep(value);
            newClass = newClass->FindTransitions(thread_, key.GetTaggedValue(), metadata, rep);
            if (newClass == nullptr) {
                return thread_->GlobalConstants()->GetHandledUndefined();
            }
        }

        auto result = JSHandle<JSHClass>(thread_, newClass);
        if (JSObject::CheckPropertiesForRep(thread_, properties, propsLen, result)) {
            return JSHandle<JSTaggedValue>(result);
        }
    }
    return thread_->GlobalConstants()->GetHandledUndefined();
}

void BaseSnapshotInfo::Record(ItemData &data)
{
    ItemKey key = GetItemKey(data.constantPoolId_, data.constantPoolIdx_);
    info_.emplace(key, data);
}

void BaseSnapshotInfo::CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                                          JSHandle<ConstantPool> snapshotConstantPool,
                                          const std::set<uint32_t> &skippedMethods,
                                          JSHandle<JSTaggedValue> ihc, JSHandle<JSTaggedValue> chc)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSMutableHandle<JSTaggedValue> valueHandle(thread_, JSTaggedValue::Undefined());
    uint32_t len = array->GetLength();
    std::vector<int> methodOffsetVec;
    for (uint32_t i = 0; i < len; i++) {
        valueHandle.Update(array->Get(thread_, i));
        uint32_t methodOffset = 0;
        if (valueHandle->IsJSFunction()) {
            methodOffset = JSHandle<JSFunction>(valueHandle)->GetCallTarget(thread_)->GetMethodId().GetOffset();
        } else if (valueHandle->IsFunctionTemplate()) {
            auto method = Method::Cast(JSHandle<FunctionTemplate>(valueHandle)->GetMethod(thread_));
            methodOffset = method->GetMethodId().GetOffset();
        }
        if (methodOffset != 0) {
            if (skippedMethods.find(methodOffset) != skippedMethods.end()) {
                methodOffsetVec.emplace_back(AOTLiteralInfo::NO_FUNC_ENTRY_VALUE);
            } else {
                methodOffsetVec.emplace_back(methodOffset);
            }
        }
    }

    uint32_t methodSize = methodOffsetVec.size();
    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory->NewAOTLiteralInfo(methodSize);
    for (uint32_t i = 0; i < methodSize; ++i) {
        auto methodOffset = methodOffsetVec[i];
        aotLiteralInfo->SetObjectToCache(thread_, i, JSTaggedValue(methodOffset));
    }

    if (!ihc->IsUndefined()) {
        aotLiteralInfo->SetIhc(thread_, ihc.GetTaggedValue());
    }

    if (!chc->IsUndefined()) {
        aotLiteralInfo->SetChc(thread_, chc.GetTaggedValue());
    }

    snapshotConstantPool->SetObjectToCache(thread_, constantPoolIndex, aotLiteralInfo.GetTaggedValue());
}

bool BaseSnapshotInfo::CheckAOTPropertiesForRep(const JSThread *thread,
                                                const JSHandle<TaggedArray> &properties,
                                                const JSHandle<JSHClass> &hclass)
{
    auto layout = LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject());
    for (size_t i = 0; i < properties->GetLength(); i++) {
        auto attr = layout->GetAttr(thread, i);
        auto value = JSObject::ConvertValueWithRep(attr, properties->Get(thread, i));
        // If value.first is false, indicating that value cannot be converted to the expected value of
        // representation. For example, the representation is INT, but the value type is string.
        if (!value.first) {
            return false;
        }
    }
    return true;
}

bool BaseSnapshotInfo::CheckAOTIhcPropertiesForRep(JSThread *thread, const JSHandle<JSTaggedValue> &ihc,
                                                   const JSHandle<ClassInfoExtractor> &extractor)
{
    if (ihc->IsUndefinedOrNull()) {
        return false;
    }
    JSHandle<JSObject> prototype;
    if (ihc->IsJSHClass()) {
        JSHandle<JSHClass> ihclass(ihc);
        prototype = JSHandle<JSObject>(thread, ihclass->GetProto(thread));
    } else {
        prototype = JSHandle<JSObject>(ihc);
    }

    ASSERT(!prototype->GetJSHClass()->IsDictionaryMode());
    JSHandle<TaggedArray> nonStaticProperties(thread, extractor->GetNonStaticProperties(thread));
    JSHandle<JSHClass> protohclass(thread, prototype->GetJSHClass());
    return CheckAOTPropertiesForRep(thread, nonStaticProperties, protohclass);
}

bool BaseSnapshotInfo::CheckAOTChcPropertiesForRep(JSThread *thread, const JSHandle<JSTaggedValue> &chc,
                                                   const JSHandle<ClassInfoExtractor> &extractor)
{
    if (chc->IsUndefinedOrNull()) {
        return false;
    }
    JSHandle<JSHClass> chclass(thread, JSHClass::Cast(chc->GetTaggedObject()));
    ASSERT(!chclass->IsDictionaryMode());
    JSHandle<TaggedArray> staticProperties(thread, extractor->GetStaticProperties(thread));
    return CheckAOTPropertiesForRep(thread, staticProperties, chclass);
}

void StringSnapshotInfo::StoreDataToGlobalData(SnapshotGlobalData &globalData, const std::set<uint32_t>&)
{
    for (auto item : info_) {
        const ItemData &data = item.second;
        uint32_t snapshotCpArrIdx = globalData.GetCpArrIdxByConstanPoolId(data.constantPoolId_);
        JSHandle<TaggedArray> snapshotCpArr(thread_, globalData.GetCurSnapshotCpArray());
        JSHandle<ConstantPool> snapshotCp(thread_, snapshotCpArr->Get(thread_, snapshotCpArrIdx));
        // Lazy ConstantPool String Loading
        snapshotCp->SetObjectToCache(thread_, data.constantPoolIdx_, JSTaggedValue::Hole());
    }

    if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm_->AddAOTSnapShotStats("String", info_.size());
    }
}

void MethodSnapshotInfo::StoreDataToGlobalData(SnapshotGlobalData &globalData,
                                               const std::set<uint32_t> &skippedMethods)
{
    ApEntityId abcId = INVALID_INDEX;
    bool hasAbcId = TryGetABCId(abcId);
    ObjectFactory *factory = vm_->GetFactory();
    for (auto item : info_) {
        const ItemData &data = item.second;
        JSHandle<ConstantPool> cp(thread_,
            thread_->GetEcmaVM()->FindConstpool(jsPandaFile_, data.constantPoolId_));
        uint32_t methodOffset = cp->GetEntityId(data.constantPoolIdx_).GetOffset();

        uint32_t snapshotCpArrIdx = globalData.GetCpArrIdxByConstanPoolId(data.constantPoolId_);
        JSHandle<TaggedArray> snapshotCpArr(thread_, globalData.GetCurSnapshotCpArray());
        JSHandle<ConstantPool> snapshotCp(thread_, snapshotCpArr->Get(thread_, snapshotCpArrIdx));

        JSHandle<JSTaggedValue> ihc = thread_->GlobalConstants()->GetHandledUndefined();
        if (hasAbcId) {
            ProfileType pt(abcId, methodOffset, ProfileType::Kind::ClassId, true);
            ihc = TryGetHClass(pt, pt);
        }
        JSHandle<AOTLiteralInfo> aotLiteralInfo = factory->NewAOTLiteralInfo(1); // 1: only one method
        int initValue = static_cast<int>(AOTLiteralInfo::NO_FUNC_ENTRY_VALUE);
        aotLiteralInfo->SetObjectToCache(thread_, 0, JSTaggedValue(initValue));
        aotLiteralInfo->SetLiteralType(JSTaggedValue(AOTLiteralInfo::METHOD_LITERAL_TYPE));
        if (!ihc->IsUndefined()) {
            aotLiteralInfo->SetIhc(thread_, ihc.GetTaggedValue());
            if (skippedMethods.find(methodOffset) == skippedMethods.end()) {
                aotLiteralInfo->SetObjectToCache(thread_, 0, JSTaggedValue(methodOffset));
                globalData.RecordReviseData(
                    ReviseData::ItemData {globalData.GetCurDataIdx(), snapshotCpArrIdx, data.constantPoolIdx_});
            }
            snapshotCp->SetObjectToCache(thread_, data.constantPoolIdx_, aotLiteralInfo.GetTaggedValue());
        } else if (skippedMethods.find(methodOffset) == skippedMethods.end()) {
            // For MethodSnaphotInfo which does not have ihc info, we insert JSTaggedValue(methodOffset) as revervation.
            // For the purpose of reducing ai size.
            globalData.RecordReviseData(
                ReviseData::ItemData {globalData.GetCurDataIdx(), snapshotCpArrIdx, data.constantPoolIdx_});
            snapshotCp->SetObjectToCache(thread_, data.constantPoolIdx_, JSTaggedValue(methodOffset));
        } else {
            snapshotCp->SetObjectToCache(thread_, data.constantPoolIdx_, JSTaggedValue::Hole());
        }
    }
    if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm_->AddAOTSnapShotStats("Method", info_.size());
    }
}

void ClassLiteralSnapshotInfo::StoreDataToGlobalData(SnapshotGlobalData &globalData,
                                                     const std::set<uint32_t> &skippedMethods)
{
    ApEntityId abcId = INVALID_INDEX;
    bool hasAbcId = TryGetABCId(abcId);
    for (auto item : info_) {
        const ItemData &data = item.second;
        JSHandle<ConstantPool> cp = GetUnsharedConstpool(data);
        auto literalObj = ConstantPool::GetClassLiteralFromCache(thread_, cp, data.constantPoolIdx_, data.recordName_);
        JSHandle<ClassLiteral> classLiteral(thread_, literalObj);
        JSHandle<TaggedArray> arrayHandle(thread_, classLiteral->GetArray(thread_));

        uint32_t snapshotCpArrIdx = globalData.GetCpArrIdxByConstanPoolId(data.constantPoolId_);
        JSHandle<TaggedArray> snapshotCpArr(thread_, globalData.GetCurSnapshotCpArray());
        JSHandle<ConstantPool> snapshotCp(thread_, snapshotCpArr->Get(thread_, snapshotCpArrIdx));

        uint32_t methodId = cp->GetEntityId(data.ctorMethodOffset_).GetOffset();
        JSHandle<JSTaggedValue> undefinedHandle = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> ihc = undefinedHandle;
        JSHandle<JSTaggedValue> chc = undefinedHandle;
        if (hasAbcId) {
            ProfileType pt(abcId, methodId, ProfileType::Kind::ClassId, true);
            ProfileType ctorPt(abcId, methodId, ProfileType::Kind::ConstructorId, true);
            ihc = TryGetHClass(pt, pt);
            chc = TryGetHClass(ctorPt, ctorPt);

            auto method = ConstantPool::GetMethodFromCache(thread_, cp.GetTaggedValue(), data.ctorMethodOffset_);
            auto *factory = thread_->GetEcmaVM()->GetFactory();
            auto extractor = factory->NewClassInfoExtractor(JSHandle<JSTaggedValue>(thread_, method));
            ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(thread_, extractor, arrayHandle,
                arrayHandle->GetLength());
            if (!CheckAOTIhcPropertiesForRep(thread_, ihc, extractor)) {
                ihc = undefinedHandle;
            }
            if (!CheckAOTChcPropertiesForRep(thread_, chc, extractor)) {
                chc = undefinedHandle;
            }
        }

        CollectLiteralInfo(arrayHandle, data.constantPoolIdx_, snapshotCp, skippedMethods, ihc, chc);
        globalData.RecordReviseData(
            ReviseData::ItemData {globalData.GetCurDataIdx(), snapshotCpArrIdx, data.constantPoolIdx_});
    }
    if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm_->AddAOTSnapShotStats("ClassLiteral", info_.size());
    }
}

void ObjectLiteralSnapshotInfo::StoreDataToGlobalData(SnapshotGlobalData &globalData,
                                                      const std::set<uint32_t> &skippedMethods)
{
    ApEntityId abcId = INVALID_INDEX;
    bool hasAbcId = TryGetABCId(abcId);
    for (auto item : info_) {
        const ItemData &data = item.second;
        JSHandle<ConstantPool> cp = GetUnsharedConstpool(data);
        panda_file::File::EntityId id = cp->GetEntityId(data.constantPoolIdx_);
        JSMutableHandle<TaggedArray> elements(thread_, JSTaggedValue::Undefined());
        JSMutableHandle<TaggedArray> properties(thread_, JSTaggedValue::Undefined());
        LiteralDataExtractor::ExtractObjectDatas(thread_, jsPandaFile_, id, elements,
                                                 properties, cp, data.recordName_);

        uint32_t snapshotCpArrIdx = globalData.GetCpArrIdxByConstanPoolId(data.constantPoolId_);
        JSHandle<TaggedArray> snapshotCpArr(thread_, globalData.GetCurSnapshotCpArray());
        JSHandle<ConstantPool> snapshotCp(thread_, snapshotCpArr->Get(thread_, snapshotCpArrIdx));

        JSHandle<JSTaggedValue> ihc = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> chc = thread_->GlobalConstants()->GetHandledUndefined();
        if (hasAbcId) {
            ProfileType pt(abcId, id.GetOffset(), ProfileType::Kind::ObjectLiteralId, true);
            ProfileType ctorPt(abcId, id.GetOffset(), ProfileType::Kind::ConstructorId, true);
            chc = TryGetHClass(ctorPt, ctorPt);
            ihc = TryGetIHClass(pt, pt, data, properties, globalData);
        }

        CollectLiteralInfo(properties, data.constantPoolIdx_, snapshotCp, skippedMethods, ihc, chc);
        globalData.RecordReviseData(
            ReviseData::ItemData {globalData.GetCurDataIdx(), snapshotCpArrIdx, data.constantPoolIdx_});
    }
    if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm_->AddAOTSnapShotStats("ObjectLiteral", info_.size());
    }
}

void ArrayLiteralSnapshotInfo::StoreDataToGlobalData(SnapshotGlobalData &globalData,
                                                     const std::set<uint32_t> &skippedMethods)
{
    for (auto item : info_) {
        const ItemData &data = item.second;
        JSHandle<ConstantPool> cp = GetUnsharedConstpool(data);
        panda_file::File::EntityId id = cp->GetEntityId(data.constantPoolIdx_);
        JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(
            thread_, jsPandaFile_, id, cp, data.recordName_);
        uint32_t snapshotCpArrIdx = globalData.GetCpArrIdxByConstanPoolId(data.constantPoolId_);
        JSHandle<TaggedArray> snapshotCpArr(thread_, globalData.GetCurSnapshotCpArray());
        JSHandle<ConstantPool> snapshotCp(thread_, snapshotCpArr->Get(thread_, snapshotCpArrIdx));
        JSHandle<JSTaggedValue> ihc = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> chc = thread_->GlobalConstants()->GetHandledUndefined();
        CollectLiteralInfo(literal, data.constantPoolIdx_, snapshotCp, skippedMethods, ihc, chc);
        globalData.RecordReviseData(
            ReviseData::ItemData {globalData.GetCurDataIdx(), snapshotCpArrIdx, data.constantPoolIdx_});
    }
    if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm_->AddAOTSnapShotStats("ArrayLiteral", info_.size());
    }
}

JSHandle<ConstantPool> BaseSnapshotInfo::GetUnsharedConstpool(const ItemData &data)
{
    EcmaVM *vm = thread_->GetEcmaVM();
    JSTaggedValue shareCp = vm->FindConstpool(jsPandaFile_, data.constantPoolId_);
    JSHandle<ConstantPool> cp(thread_, vm->FindOrCreateUnsharedConstpool(shareCp));
    return cp;
}

void SnapshotConstantPoolData::Record(const BytecodeInstruction &bcIns, int32_t bcIndex,
                                      const CString &recordName, const MethodLiteral *method)
{
    BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
    uint32_t methodOffset = method->GetMethodId().GetOffset();
    panda_file::IndexAccessor indexAccessor(*jsPandaFile_->GetPandaFile(),
                                            panda_file::File::EntityId(methodOffset));
    uint32_t constantPoolId = static_cast<uint32_t>(indexAccessor.GetHeaderIndex());

    switch (opcode) {
        case BytecodeInstruction::Opcode::LDA_STR_ID16:
        case BytecodeInstruction::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        case BytecodeInstruction::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        case BytecodeInstruction::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        case BytecodeInstruction::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::LDTHISBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::LDTHISBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STTHISBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::STTHISBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::LDGLOBALVAR_IMM16_ID16:
        case BytecodeInstruction::Opcode::LDOBJBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::LDOBJBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::LDSUPERBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::LDSUPERBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::STGLOBALVAR_IMM16_ID16:
        case BytecodeInstruction::Opcode::LDBIGINT_ID16: {
            auto constantPoolIdx = bcIns.GetId().AsRawValue();
            BaseSnapshotInfo::ItemData itemData = {recordName, constantPoolId, constantPoolIdx, methodOffset, bcIndex};
            RecordInfo(Type::STRING, itemData);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            auto constantPoolIdx = bcIns.GetId().AsRawValue();
            BaseSnapshotInfo::ItemData itemData = {recordName, constantPoolId, constantPoolIdx, methodOffset, bcIndex};
            RecordInfo(Type::METHOD, itemData);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
            auto constantPoolIdx = bcIns.GetId().AsRawValue();
            BaseSnapshotInfo::ItemData itemData = {recordName, constantPoolId, constantPoolIdx, methodOffset, bcIndex};
            RecordInfo(Type::OBJECT_LITERAL, itemData);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
            auto constantPoolIdx = bcIns.GetId().AsRawValue();
            BaseSnapshotInfo::ItemData itemData = {recordName, constantPoolId, constantPoolIdx, methodOffset, bcIndex};
            RecordInfo(Type::ARRAY_LITERAL, itemData);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
            auto methodCPIdx = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            BaseSnapshotInfo::ItemData methodItemData = {recordName, constantPoolId,
                methodCPIdx, methodOffset, bcIndex};
            RecordInfo(Type::METHOD, methodItemData);

            auto literalCPIdx = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            BaseSnapshotInfo::ItemData literalItemData = {recordName, constantPoolId,
                literalCPIdx, methodOffset, bcIndex, methodCPIdx};
            RecordInfo(Type::CLASS_LITERAL, literalItemData);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
            auto methodCPIdx = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            BaseSnapshotInfo::ItemData methodItemData = {recordName, constantPoolId,
                methodCPIdx, methodOffset, bcIndex};
            RecordInfo(Type::METHOD, methodItemData);

            auto literalCPIdx = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            BaseSnapshotInfo::ItemData literalItemData = {recordName, constantPoolId,
                literalCPIdx, methodOffset, bcIndex, methodCPIdx};
            RecordInfo(Type::CLASS_LITERAL, literalItemData);
            break;
        }
        default:
            break;
    }
}

void SnapshotConstantPoolData::StoreDataToGlobalData(SnapshotGlobalData &snapshotData,
                                                     const std::set<uint32_t> &skippedMethods) const
{
    for (auto &info : infos_) {
        info->StoreDataToGlobalData(snapshotData, skippedMethods);
    }
}
}  // namespace panda::ecmascript
