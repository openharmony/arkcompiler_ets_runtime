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

#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info-inl.h"
namespace panda::ecmascript::kungfu {
void PGOTypeManager::Iterate(RootVisitor &v)
{
    for (auto &iter : hcData_) {
        for (auto &hclassIter : iter.second) {
            v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(hclassIter.second))));
        }
    }
    aotSnapshot_.Iterate(v);
    for (auto &iter : hclassInfoLocal_) {
        if (iter.GetRawData() != 0) {
            v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(iter))));
        }
    }
}

uint32_t PGOTypeManager::GetConstantPoolIDByMethodOffset(const uint32_t methodOffset) const
{
    ASSERT(curJSPandaFile_!=nullptr);
    panda_file::IndexAccessor indexAccessor(*curJSPandaFile_->GetPandaFile(),
                                            panda_file::File::EntityId(methodOffset));
    return static_cast<uint32_t>(indexAccessor.GetHeaderIndex());
}

JSTaggedValue PGOTypeManager::GetConstantPoolByMethodOffset(const uint32_t methodOffset) const
{
    uint32_t cpId = GetConstantPoolIDByMethodOffset(methodOffset);
    return thread_->GetEcmaVM()->FindConstpool(curJSPandaFile_, cpId);
}

JSTaggedValue PGOTypeManager::GetStringFromConstantPool(const uint32_t methodOffset, const uint16_t cpIdx) const
{
    JSTaggedValue cp = GetConstantPoolByMethodOffset(methodOffset);
    return ConstantPool::GetStringFromCache(thread_, cp, cpIdx);
}

void PGOTypeManager::InitAOTSnapshot(uint32_t compileFilesCount)
{
    aotSnapshot_.InitSnapshot(compileFilesCount);
    GenHClassInfo();
    GenSymbolInfo();
    GenArrayInfo();
    GenConstantIndexInfo();
    GenProtoTransitionInfo();
}

uint32_t PGOTypeManager::GetSymbolCountFromHClassData()
{
    uint32_t count = 0;
    for (auto& root: hcData_) {
        for (auto& child: root.second) {
            if (!JSTaggedValue(child.second).IsJSHClass()) {
                continue;
            }
            JSHClass* hclass = JSHClass::Cast(JSTaggedValue(child.second).GetTaggedObject());
            if (!hclass->HasTransitions(thread_)) {
                LayoutInfo* layoutInfo = LayoutInfo::GetLayoutInfoFromHClass(thread_, hclass);
                uint32_t len = hclass->NumberOfProps();
                for (uint32_t i = 0; i < len; i++) {
                    JSTaggedValue key = layoutInfo->GetKey(thread_, i);
                    if (key.IsSymbol() && JSSymbol::Cast(key)->HasId()) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

void PGOTypeManager::GenSymbolInfo()
{
    uint32_t count = GetSymbolCountFromHClassData();
    ObjectFactory* factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> symbolInfo = factory->NewTaggedArray(count * 2); // 2: symbolId, symbol
    uint32_t pos = 0;

    for (auto& root: hcData_) {
        ProfileType rootType = root.first;
        for (auto& child: root.second) {
            if (!JSTaggedValue(child.second).IsJSHClass()) {
                continue;
            }
            ProfileType childType = child.first;
            JSHClass* hclass = JSHClass::Cast(JSTaggedValue(child.second).GetTaggedObject());
            if (!hclass->HasTransitions(thread_)) {
                LayoutInfo* layoutInfo = LayoutInfo::GetLayoutInfoFromHClass(thread_, hclass);
                uint32_t len = hclass->NumberOfProps();
                for (uint32_t i = 0; i < len; i++) {
                    JSTaggedValue symbol = layoutInfo->GetKey(thread_, i);
                    if (symbol.IsSymbol() && JSSymbol::Cast(symbol)->HasId()) {
                        JSSymbol* symbolPtr = JSSymbol::Cast(symbol.GetTaggedObject());
                        uint64_t symbolId = symbolPtr->GetPrivateId();
                        uint64_t slotIndex = JSSymbol::GetSlotIndex(symbolId);
                        ProfileTypeTuple symbolIdKey = std::make_tuple(rootType, childType, slotIndex);
                        profileTypeToSymbolId_.emplace(symbolIdKey, symbolId);
                        symbolInfo->Set(thread_, pos++, JSTaggedValue(symbolId));
                        symbolInfo->Set(thread_, pos++, symbol);
                    }
                }
            }
        }
    }

    EcmaVM *vm = thread_->GetEcmaVM();
    if (vm->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm->AddAOTSnapShotStats("SymbolInfo", symbolInfo->GetLength());
    }
    aotSnapshot_.StoreSymbolInfo(symbolInfo);
}

bool PGOTypeManager::IsNapiIhc(ProfileType rootType, ProfileType childType)
{
    // If the rootType is NapiType and the childType is root, it must be napi object's ihc.
    return rootType.IsNapiType() && childType.IsRootType();
}

/*                  hclassInfo
 *      +-----------------------------------+----
 *      |          AOT Napi Hclass          |  zero slot
 *      +-----------------------------------+----
 *      |              ...                  |   ^
 *      |              ...                  |   |
 *      |             hcData                |  pos
 *      |              ...                  |   |
 *      |              ...                  |   v
 *      +-----------------------------------+----
 *      | Object Literal Cache (maybe hole) |  last slot
 *      +-----------------------------------+----
 */

void PGOTypeManager::GenHClassInfo()
{
    uint32_t count = 2; // For object literal hclass cache and Napi root type.
    for (auto& root: hcData_) {
        count += root.second.size();
    }

    ObjectFactory* factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> hclassInfo = factory->NewTaggedArray(count);
    uint32_t pos = 1; // 1: the first one is the napi object's ihc.
    profileTyperToHClassIndex_.clear();
    for (auto& root: hcData_) {
        ProfileType rootType = root.first;
        for (auto& child: root.second) {
            ProfileType childType = child.first;
            JSTaggedType hclass = child.second;
            ProfileTyper key = std::make_pair(rootType, childType);
            if (IsNapiIhc(rootType, childType)) {
                profileTyperToHClassIndex_.emplace(key, 0);
                hclassInfo->Set(thread_, 0, JSTaggedValue(hclass));
            } else {
                profileTyperToHClassIndex_.emplace(key, pos);
                hclassInfo->Set(thread_, pos++, JSTaggedValue(hclass));
            }
        }
    }
    // The cache of Object literal serializes to last index of AOTHClassInfo.
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> maybeCache = env->GetObjectLiteralHClassCache();
    if (!maybeCache->IsHole()) {
        // It cannot be serialized if object in global env.
        JSHandle<TaggedArray> array(maybeCache);
        auto cloneResult = factory->CopyArray(array, array->GetLength(), array->GetLength());
        hclassInfo->Set(thread_, hclassInfo->GetLength() - 1, cloneResult);
    }

    EcmaVM *vm = thread_->GetEcmaVM();
    if (vm->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm->AddAOTSnapShotStats("HClassInfo", hclassInfo->GetLength());
    }
    aotSnapshot_.StoreHClassInfo(hclassInfo);
}

void PGOTypeManager::GenProtoTransitionInfo()
{
    auto transitionTable = thread_->GetEcmaVM()->GetFunctionProtoTransitionTable();
    for (auto &protoTransType : protoTransTypes_) {
        JSTaggedValue ihc = QueryHClass(protoTransType.ihcType, protoTransType.ihcType);
        JSTaggedValue baseIhc = QueryHClass(protoTransType.baseRootType, protoTransType.baseType);
        JSTaggedValue transIhc = QueryHClass(protoTransType.transIhcType, protoTransType.transIhcType);
        JSTaggedValue transPhc = QueryHClass(protoTransType.transPhcType, protoTransType.transPhcType);
        if (ihc.IsUndefined() || baseIhc.IsUndefined() || transIhc.IsUndefined() || transPhc.IsUndefined()) {
            LOG_COMPILER(DEBUG) << "broken prototype transition info!";
            continue;
        }
        transitionTable->InsertTransitionItem(thread_,
                                              JSHandle<JSTaggedValue>(thread_, ihc),
                                              JSHandle<JSTaggedValue>(thread_, baseIhc),
                                              JSHandle<JSTaggedValue>(thread_, transIhc),
                                              JSHandle<JSTaggedValue>(thread_, transPhc));
        // Situation:
        // 1: d1.prototype = p
        // 2: d2.prototype = p
        // For this case, baseIhc is transPhc
        transitionTable->InsertTransitionItem(thread_,
                                              JSHandle<JSTaggedValue>(thread_, ihc),
                                              JSHandle<JSTaggedValue>(thread_, transPhc),
                                              JSHandle<JSTaggedValue>(thread_, transIhc),
                                              JSHandle<JSTaggedValue>(thread_, transPhc));
    }
    aotSnapshot_.StoreProtoTransTableInfo(JSHandle<JSTaggedValue>(thread_, transitionTable->GetProtoTransitionTable()));
}

void PGOTypeManager::GenArrayInfo()
{
    EcmaVM *vm = thread_->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<TaggedArray> arrayInfo = factory->EmptyArray();
    if (vm->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm->AddAOTSnapShotStats("ArrayInfo", arrayInfo->GetLength());
    }
    aotSnapshot_.StoreArrayInfo(arrayInfo);
}

void PGOTypeManager::GenConstantIndexInfo()
{
    uint32_t count = constantIndexData_.size();
    EcmaVM *vm = thread_->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<TaggedArray> constantIndexInfo = factory->NewTaggedArray(count);
    for (uint32_t pos = 0; pos < count; pos++) {
        constantIndexInfo->Set(thread_, pos, JSTaggedValue(constantIndexData_[pos]));
    }
    if (vm->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm->AddAOTSnapShotStats("ConstantIndexInfo", constantIndexInfo->GetLength());
    }
    aotSnapshot_.StoreConstantIndexInfo(constantIndexInfo);
}

void PGOTypeManager::RecordHClass(ProfileType rootType, ProfileType childType, JSTaggedType hclass, bool update)
{
    LockHolder lock(mutex_);
    auto iter = hcData_.find(rootType);
    if (iter == hcData_.end()) {
        auto map = TransIdToHClass();
        map.emplace(childType, hclass);
        hcData_.emplace(rootType, map);
        return;
    }

    auto &hclassMap = iter->second;
    auto hclassIter = hclassMap.find(childType);
    if (hclassIter != hclassMap.end()) {
        if (!update) {
            ASSERT(hclass == hclassIter->second);
            return;
        } else {
            hclassMap[childType]= hclass;
            return;
        }
    }
    hclassMap.emplace(childType, hclass);
}

void PGOTypeManager::RecordConstantIndex(uint32_t bcAbsoluteOffset, uint32_t index)
{
    constantIndexData_.emplace_back(bcAbsoluteOffset);
    constantIndexData_.emplace_back(index);
}

uint32_t PGOTypeManager::GetHClassIndexByProfileType(ProfileTyper type) const
{
    uint32_t index = -1;
    auto iter = profileTyperToHClassIndex_.find(type);
    if (iter != profileTyperToHClassIndex_.end()) {
        index = iter->second;
    }
    return index;
}

int PGOTypeManager::GetHolderHIndexByPGOObjectInfoType(pgo::PGOObjectInfo type, bool isAot)
{
    if (isAot) {
        ProfileTyper holderType = std::make_pair(type.GetHoldRootType(), type.GetHoldType());
        int holderHCIndex = static_cast<int>(GetHClassIndexByProfileType(holderType));
        ASSERT(QueryHClass(holderType.first, holderType.second).IsJSHClass());
        return holderHCIndex;
    } else {
        JSHClass *holderHClass = type.GetHolderHclass();
        int holderHCIndex = RecordAndGetHclassIndexForJIT(holderHClass);
        return holderHCIndex;
    }
}

int PGOTypeManager::GetReceiverHIndexByPGOObjectInfoType(pgo::PGOObjectInfo type, bool isAot)
{
    if (isAot) {
        ProfileTyper receiverType = std::make_pair(type.GetReceiverRootType(), type.GetReceiverType());
        int receiverHCIndex = static_cast<int>(GetHClassIndexByProfileType(receiverType));
        ASSERT(QueryHClass(receiverType.first, receiverType.second).IsJSHClass());
        return receiverHCIndex;
    } else {
        JSHClass *receiverHClass = type.GetReceiverHclass();
        int receiverHCIndex = RecordAndGetHclassIndexForJIT(receiverHClass);
        return receiverHCIndex;
    }
}

std::optional<uint64_t> PGOTypeManager::GetSymbolIdByProfileType(ProfileTypeTuple type) const
{
    auto iter = profileTypeToSymbolId_.find(type);
    if (iter != profileTypeToSymbolId_.end()) {
        return iter->second;
    }
    return std::nullopt;
}

JSTaggedValue PGOTypeManager::QueryHClass(ProfileType rootType, ProfileType childType)
{
    LockHolder lock(mutex_);
    JSTaggedValue result = JSTaggedValue::Undefined();
    auto iter = hcData_.find(rootType);
    if (iter != hcData_.end()) {
        auto hclassMap = iter->second;
        auto hclassIter = hclassMap.find(childType);
        if (hclassIter != hclassMap.end()) {
            result = JSTaggedValue(hclassIter->second);
        }
    }
    // Can return real hclass of object or prototype of it
    return result;
}

JSTaggedValue PGOTypeManager::QueryHClassByIndexForJIT(uint32_t hclassIndex)
{
    ASSERT(hclassIndex < pos_);
    return hclassInfoLocal_[hclassIndex];
}

int PGOTypeManager::RecordAndGetHclassIndexForJIT(JSHClass* hclass)
{
    // The hclass in hclassInfoLocal_ cannot currently be cleared after each
    // JIT session because it depends on ensuring the survival of hclass.
    // If a reference from machineCode to hclass is added in the future, then it can be cleared.
    auto value = JSTaggedValue::Cast(hclass);
    for (int i = 0; i < pos_; i++) {
        if (hclassInfoLocal_[i] == value) {
            return i;
        }
    }
    hclassInfoLocal_.emplace_back(value);
    return pos_++;
}

void PGOTypeManager::MergeRepresentationForProtoTransition()
{
    for (auto &protoTransType : protoTransTypes_) {
        JSTaggedValue ihc = QueryHClass(protoTransType.ihcType, protoTransType.ihcType);
        JSTaggedValue baseIhc = QueryHClass(protoTransType.baseRootType, protoTransType.baseType);
        JSTaggedValue transIhc = QueryHClass(protoTransType.transIhcType, protoTransType.transIhcType);
        JSTaggedValue transPhc = QueryHClass(protoTransType.transPhcType, protoTransType.transPhcType);
        if (ihc.IsUndefined() || baseIhc.IsUndefined() || transIhc.IsUndefined() || transPhc.IsUndefined()) {
            LOG_COMPILER(DEBUG) << "broken prototype transition info!";
            continue;
        }
        JSHClass::MergeRepresentation(thread_, JSHClass::Cast(baseIhc.GetTaggedObject()),
            JSHClass::Cast(transPhc.GetTaggedObject()));
        JSHClass::MergeRepresentation(thread_, JSHClass::Cast(ihc.GetTaggedObject()),
            JSHClass::Cast(transIhc.GetTaggedObject()));
    }
}
}  // namespace panda::ecmascript
