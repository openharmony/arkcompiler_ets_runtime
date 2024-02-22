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
#ifndef ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_MANAGER_H
#define ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_MANAGER_H

#include "ecmascript/compiler/aot_snapshot/aot_snapshot.h"
#include "ecmascript/compiler/pgo_type/pgo_type_location.h"

namespace panda::ecmascript::kungfu {
class PGOTypeManager {
public:
    PGOTypeManager(EcmaVM *vm)
        : thread_(vm->GetJSThread()), aotSnapshot_(vm) {}
    ~PGOTypeManager() = default;

    static int32_t GetConstantPoolIDByMethodOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset);

    void Iterate(const RootVisitor &v);

    // common
    JSThread* GetJSThread()
    {
        return thread_;
    }

    void PUBLIC_API SetCurConstantPool(const JSPandaFile *jsPandaFile, uint32_t methodOffset);

    JSHandle<JSTaggedValue> GetCurConstantPool() const
    {
        return JSHandle<JSTaggedValue>(uintptr_t(&curCP_));
    }

    // snapshot
    void PUBLIC_API InitAOTSnapshot(uint32_t compileFilesCount);
    void GenArrayInfo();
    void GenConstantIndexInfo();

    AOTSnapshot& GetAOTSnapshot()
    {
        return aotSnapshot_;
    }

    CVector<JSTaggedType>& GetArrayInfo()
    {
        return arrayData_;
    }

    // array
    void PUBLIC_API RecordElements(panda_file::File::EntityId id, JSTaggedValue elements);

    void UpdateRecordedElements(panda_file::File::EntityId id, JSTaggedValue elements, ElementsKind kind);

    void RecordConstantIndex(uint32_t bcAbsoluteOffset, uint32_t index);

    int PUBLIC_API GetElementsIndexByEntityId(panda_file::File::EntityId id);

    ElementsKind GetElementsKindByEntityId(panda_file::File::EntityId id);

    // hclass
    void RecordHClass(ProfileType rootType, ProfileType childType, JSTaggedType hclass);

    uint32_t PUBLIC_API GetHClassIndexByProfileType(ProfileTyper type) const;

    JSTaggedValue PUBLIC_API QueryHClass(ProfileType rootType, ProfileType childType) const;
    ElementsKind QueryElementKind(ProfileType rootType);

    inline ProfileType GetRootIdByLocation(const PGOTypeLocation &loc)
    {
        auto it = locToRootIdMap_.find(loc);
        if (it != locToRootIdMap_.end()) {
            return it->second;
        }
        return ProfileType::PROFILE_TYPE_NONE;
    }

    inline ElementsKind GetElementsKindByLocation(PGOTypeLocation loc)
    {
        auto it = locToElmsKindMap_.find(loc);
        if (it != locToElmsKindMap_.end()) {
            return it->second;
        }
        return ElementsKind::GENERIC;
    }

    inline void RecordLocationToRootType(const PGOTypeLocation &loc, ProfileType rootType)
    {
        locToRootIdMap_.emplace(loc, rootType);
    }

    inline void RecordLocationToElementsKind(PGOTypeLocation loc, ElementsKind kind)
    {
        locToElmsKindMap_.emplace(loc, kind);
    }

private:
    // snapshot
    void GenHClassInfo();

    // opt to std::unordered_map
    using TransIdToHClass = std::map<ProfileType, JSTaggedType>;
    using RootIdToTransMap = std::map<ProfileType, TransIdToHClass>;

    JSThread *thread_;
    RootIdToTransMap hcData_;
    CVector<JSTaggedType> arrayData_ {};
    CVector<uint32_t> constantIndexData_ {};
    CUnorderedMap<PGOTypeLocation, ProfileType, HashPGOTypeLocation> locToRootIdMap_ {};
    CUnorderedMap<PGOTypeLocation, ElementsKind, HashPGOTypeLocation> locToElmsKindMap_ {};
    CMap<ProfileTyper, uint32_t> profileTyperToHClassIndex_ {};
    std::map<panda_file::File::EntityId, uint32_t> idElmsIdxMap_ {};
    std::map<panda_file::File::EntityId, ElementsKind> idElmsKindMap_ {};
    AOTSnapshot aotSnapshot_;

    // When the passmanager iterates each method, the curCP_ and curCPID_ should be updated,
    // so that subsequent passes (type_infer, ts_hcr_lowering) can obtain the correct constpool.
    JSTaggedValue curCP_ {JSTaggedValue::Hole()};
    int32_t curCPID_ {0};
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_MANAGER_H
