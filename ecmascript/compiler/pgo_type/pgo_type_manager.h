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
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"

namespace panda::ecmascript::kungfu {
class PGOTypeManager {
public:
    PGOTypeManager(EcmaVM *vm)
        : thread_(vm->GetJSThread()), aotSnapshot_(vm) {}
    ~PGOTypeManager() = default;

    void Iterate(const RootVisitor &v);

    // common
    uint32_t PUBLIC_API GetConstantPoolIDByMethodOffset(const uint32_t methodOffset) const;

    JSTaggedValue PUBLIC_API GetConstantPoolByMethodOffset(const uint32_t methodOffset) const;

    JSTaggedValue PUBLIC_API GetStringFromConstantPool(const uint32_t methodOffset, const uint16_t cpIdx) const;

    inline JSThread* GetJSThread()
    {
        return thread_;
    }

    void PUBLIC_API SetCurCompilationFile(const JSPandaFile *jsPandaFile)
    {
        curJSPandaFile_ = jsPandaFile;
    }

    // snapshot
    void PUBLIC_API InitAOTSnapshot(uint32_t compileFilesCount);

    AOTSnapshot& GetAOTSnapshot()
    {
        return aotSnapshot_;
    }

    // array
    void RecordConstantIndex(uint32_t bcAbsoluteOffset, uint32_t index);

    // hclass
    void RecordHClass(ProfileType rootType, ProfileType childType, JSTaggedType hclass, bool update = false);
    int PUBLIC_API RecordAndGetHclassIndexForJIT(JSHClass* hclass);
    uint32_t PUBLIC_API GetHClassIndexByProfileType(ProfileTyper type) const;
    int PUBLIC_API GetHolderHIndexByPGOObjectInfoType(pgo::PGOObjectInfo type, bool isAot);
    int PUBLIC_API GetReceiverHIndexByPGOObjectInfoType(pgo::PGOObjectInfo type, bool isAot);

    JSTaggedValue PUBLIC_API QueryHClass(ProfileType rootType, ProfileType childType) ;
    JSTaggedValue PUBLIC_API QueryHClassByIndexForJIT(uint32_t hclassIndex) ;
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

    struct ProtoTransType {
        ProtoTransType(ProfileType ihcType,
                       ProfileType baseRootType,
                       ProfileType baseType,
                       ProfileType transIhcType,
                       ProfileType transPhcType)
            : ihcType(ihcType),
              baseRootType(baseRootType),
              baseType(baseType),
              transIhcType(transIhcType),
              transPhcType(transPhcType) {}

        ProfileType ihcType {};
        ProfileType baseRootType {};
        ProfileType baseType {};
        ProfileType transIhcType {};
        ProfileType transPhcType {};
    };

    inline void RecordProtoTransType(const ProtoTransType &type)
    {
        protoTransTypes_.emplace_back(type);
    }

    std::vector<ProfileType> FindAllTransPhcByBaseType(ProfileType baseRootType)
    {
        std::vector<ProfileType> transPhcs;
        for (auto &transType: protoTransTypes_) {
            if (baseRootType == transType.baseRootType) {
                transPhcs.emplace_back(transType.transPhcType);
            }
        }
        return transPhcs;
    }

    inline void ClearHCInfoLocal()
    {
        hclassInfoLocal_.clear();
        pos_ = 0;
    }

    // symbol
    std::optional<uint64_t> PUBLIC_API GetSymbolIdByProfileType(ProfileTypeTuple type) const;

    void DumpHClassData(std::ostream& os) const
    {
        int i = 0;
        for (const auto& root: hcData_) {
            int j = 0;
            os << "[" << i << "]" << std::endl;
            os << "RootType: " << root.first << std::endl;
            for (const auto& child: root.second) {
                os << "[" << i << "]" << "[" << j << "]" << std::endl;
                os << "ChildType: " << child.first << std::endl;
                os << "HClass: " << JSTaggedValue(child.second) << std::endl;
                j++;
            }
            i++;
        }
    }

private:
    // snapshot
    void GenHClassInfo();
    void GenSymbolInfo();
    void GenArrayInfo();
    void GenConstantIndexInfo();
    void GenProtoTransitionInfo();

    uint32_t GetSymbolCountFromHClassData();

    // opt to std::unordered_map
    using TransIdToHClass = std::map<ProfileType, JSTaggedType>;
    using RootIdToTransMap = std::map<ProfileType, TransIdToHClass>;

    JSThread *thread_;
    RootIdToTransMap hcData_;
    CVector<uint32_t> constantIndexData_ {};
    CUnorderedMap<PGOTypeLocation, ProfileType, HashPGOTypeLocation> locToRootIdMap_ {};
    CUnorderedMap<PGOTypeLocation, ElementsKind, HashPGOTypeLocation> locToElmsKindMap_ {};
    CMap<ProfileTyper, uint32_t> profileTyperToHClassIndex_ {};
    CMap<ProfileTypeTuple, uint64_t> profileTypeToSymbolId_ {};
    std::vector<ProtoTransType> protoTransTypes_;

    AOTSnapshot aotSnapshot_;

    // Since there is only one PGOTypeManager instance during compilation,
    // the currently compiled jspandafile needs to be set to satisfy multi-file compilation.
    const JSPandaFile *curJSPandaFile_ {nullptr};
    Mutex mutex_;
    CVector<JSTaggedValue> hclassInfoLocal_ {};
    // When the passmanager iterates each method, the curCP_ and curCPID_ should be updated,
    // so that subsequent passes (type_infer, ts_hcr_lowering) can obtain the correct constpool.
    JSTaggedValue curCP_ {JSTaggedValue::Hole()};
    int32_t curCPID_ {0};
    int32_t pos_ {0};
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_MANAGER_H
