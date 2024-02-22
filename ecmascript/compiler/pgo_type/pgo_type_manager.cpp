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

#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "index_accessor.h"

namespace panda::ecmascript::kungfu {
void PGOTypeManager::Iterate(const RootVisitor &v)
{
    for (auto &iter : hcData_) {
        for (auto &hclassIter : iter.second) {
            v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(hclassIter.second))));
        }
    }
    aotSnapshot_.Iterate(v);
}

int32_t PGOTypeManager::GetConstantPoolIDByMethodOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(),
                                            panda_file::File::EntityId(methodOffset));
    return static_cast<int32_t>(indexAccessor.GetHeaderIndex());
}

void PGOTypeManager::InitAOTSnapshot(uint32_t compileFilesCount)
{
    aotSnapshot_.InitSnapshot(compileFilesCount);
    GenHClassInfo();
    GenArrayInfo();
    GenConstantIndexInfo();
}

void PGOTypeManager::GenHClassInfo()
{
    uint32_t count = 0;
    for (auto &data : hcData_) {
        count += data.second.size();
    }
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> hclassInfo = factory->NewTaggedArray(count);
    uint32_t pos = 0;
    for (auto &x : hcData_) {
        ProfileType rootType = x.first;
        for (auto &y : x.second) {
            ProfileType childType = y.first;
            JSTaggedType hclass = y.second;
            ProfileTyper key = std::make_pair(rootType, childType);
            profileTyperToHClassIndex_.emplace(key, pos);
            hclassInfo->Set(thread_, pos++, JSTaggedValue(hclass));
        }
    }
    aotSnapshot_.StoreHClassInfo(hclassInfo);
}

void PGOTypeManager::GenArrayInfo()
{
    uint32_t count = arrayData_.size();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> arrayInfo = factory->NewTaggedArray(count);
    for (uint32_t pos = 0; pos < count; pos++) {
        arrayInfo->Set(thread_, pos, JSTaggedValue(arrayData_[pos]));
    }
    aotSnapshot_.StoreArrayInfo(arrayInfo);
}

void PGOTypeManager::GenConstantIndexInfo()
{
    uint32_t count = constantIndexData_.size();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> constantIndexInfo = factory->NewTaggedArray(count);
    for (uint32_t pos = 0; pos < count; pos++) {
        constantIndexInfo->Set(thread_, pos, JSTaggedValue(constantIndexData_[pos]));
    }
    aotSnapshot_.StoreConstantIndexInfo(constantIndexInfo);
}

void PGOTypeManager::RecordHClass(ProfileType rootType, ProfileType childType, JSTaggedType hclass)
{
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
        ASSERT(hclass == hclassIter->second);
        return;
    }
    hclassMap.emplace(childType, hclass);
}

void PGOTypeManager::RecordElements(panda_file::File::EntityId id, JSTaggedValue elements)
{
    JSHandle<TaggedArray> elementsHandle(thread_, elements);
    arrayData_.emplace_back(elementsHandle.GetTaggedType());
    idElmsIdxMap_.emplace(id, arrayData_.size() - 1);
}

void PGOTypeManager::UpdateRecordedElements(panda_file::File::EntityId id, JSTaggedValue elements, ElementsKind kind)
{
    JSHandle<TaggedArray> elementsHandle(thread_, elements);
    auto iter = idElmsIdxMap_.find(id);
    if (iter == idElmsIdxMap_.end()) {
        LOG_COMPILER(FATAL) << "this branch is unreachable - Should have recorded elements";
        UNREACHABLE();
    }
    auto arrayDataIndex = iter->second;
    arrayData_[arrayDataIndex] = elementsHandle.GetTaggedType();
    JSHandle<TaggedArray> arrayInfo(thread_, aotSnapshot_.GetArrayInfo());
    arrayInfo->Set(thread_, arrayDataIndex, JSTaggedValue(elements));
    idElmsKindMap_.emplace(id, kind);
}

void PGOTypeManager::RecordConstantIndex(uint32_t bcAbsoluteOffset, uint32_t index)
{
    constantIndexData_.emplace_back(bcAbsoluteOffset);
    constantIndexData_.emplace_back(index);
}

int PGOTypeManager::GetElementsIndexByEntityId(panda_file::File::EntityId id)
{
    auto iter = idElmsIdxMap_.find(id);
    auto endIter = idElmsIdxMap_.end();
    if (iter == endIter) {
        return -1;
    }
    return iter->second;
}

ElementsKind PGOTypeManager::GetElementsKindByEntityId(panda_file::File::EntityId id)
{
    auto iter = idElmsKindMap_.find(id);
    auto endIter = idElmsKindMap_.end();
    if (iter == endIter) {
        return ElementsKind::HOLE_TAGGED;
    }
    return iter->second;
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

JSTaggedValue PGOTypeManager::QueryHClass(ProfileType rootType, ProfileType childType) const
{
    JSTaggedValue result = JSTaggedValue::Undefined();
    auto iter = hcData_.find(rootType);
    if (iter != hcData_.end()) {
        auto hclassMap = iter->second;
        auto hclassIter = hclassMap.find(childType);
        if (hclassIter != hclassMap.end()) {
            result = JSTaggedValue(hclassIter->second);
        }
    }
    return result;
}

void PGOTypeManager::SetCurConstantPool(const JSPandaFile *jsPandaFile, uint32_t methodOffset)
{
    curCPID_ = GetConstantPoolIDByMethodOffset(jsPandaFile, methodOffset);
    curCP_ = thread_->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, curCPID_);
}
}  // namespace panda::ecmascript
