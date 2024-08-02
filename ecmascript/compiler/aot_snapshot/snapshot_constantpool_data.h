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
#ifndef ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_DATA_H
#define ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_DATA_H

#include "ecmascript/compiler/aot_snapshot/snapshot_global_data.h"
#include "ecmascript/compiler/pgo_type/pgo_type_location.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "libpandafile/bytecode_instruction.h"

namespace panda::ecmascript::kungfu {
using ApEntityId = pgo::ApEntityId;
using PGOProfilerDecoder = pgo::PGOProfilerDecoder;

#define DATA_TYPE_LIST(V)                     \
    V(STRING, StringSnapshot)                 \
    V(METHOD, MethodSnapshot)                 \
    V(CLASS_LITERAL, ClassLiteralSnapshot)    \
    V(OBJECT_LITERAL, ObjectLiteralSnapshot)  \
    V(ARRAY_LITERAL, ArrayLiteralSnapshot)

class BaseSnapshotInfo {
public:
    struct ItemData {
        CString recordName_;
        int32_t constantPoolId_ {0};
        uint32_t constantPoolIdx_ {0};
        uint32_t methodOffset_ {0};
        uint32_t bcIndex_ {0};
        uint32_t ctorMethodOffset_ {0}; // class constructor
    };

    BaseSnapshotInfo(EcmaVM *vm,
                     const JSPandaFile *jsPandaFile,
                     const PGOProfilerDecoder *pfDecoder)
        : vm_(vm),
          thread_(vm->GetJSThread()),
          jsPandaFile_(jsPandaFile),
          pfDecoder_(pfDecoder)
    {}

    virtual ~BaseSnapshotInfo() = default;

    virtual void StoreDataToGlobalData(SnapshotGlobalData &globalData, const std::set<uint32_t> &skippedMethods) = 0;

    void Record(ItemData &data);

protected:
    using ItemKey = uint64_t;

    static constexpr uint32_t CONSTPOOL_MASK = 32;

    static ItemKey GetItemKey(uint32_t constantPoolId, uint32_t constantPoolIdx);

    bool TryGetABCId(ApEntityId &abcId);

    JSHandle<JSTaggedValue> TryGetIHClass(ProfileType rootType, ProfileType childType, const ItemData &data,
        const JSHandle<TaggedArray> &properties, const SnapshotGlobalData &globalData) const;

    JSHandle<JSTaggedValue> TryGetHClass(ProfileType rootType, ProfileType childType) const;

    JSHandle<JSTaggedValue> TryGetHClassByPGOTypeLocation(PGOTypeLocation loc) const;

    JSHandle<JSTaggedValue> TryGetHClassFromCached(const JSHandle<TaggedArray> &properties,
                                                   const SnapshotGlobalData &globalData) const;

    void CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                            JSHandle<ConstantPool> snapshotConstantPool, const std::set<uint32_t> &skippedMethods,
                            JSHandle<JSTaggedValue> ihc, JSHandle<JSTaggedValue> chc);
    JSHandle<ConstantPool> GetUnsharedConstpool(const ItemData &data);

    static bool CheckAOTPropertiesForRep(const JSHandle<TaggedArray> &properties, const JSHandle<JSHClass> &hclass);

    static bool CheckAOTIhcPropertiesForRep(JSThread *thread, const JSHandle<JSTaggedValue> &ihc,
                                            const JSHandle<ClassInfoExtractor> &extractor);

    static bool CheckAOTChcPropertiesForRep(JSThread *thread, const JSHandle<JSTaggedValue> &chc,
                                            const JSHandle<ClassInfoExtractor> &extractor);

    CUnorderedMap<ItemKey, ItemData> info_ {};
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    const PGOProfilerDecoder *pfDecoder_ {nullptr};
};

#define DEFINE_INFO_CLASS(V, name)                                          \
    class PUBLIC_API name##Info final : public BaseSnapshotInfo {           \
    public:                                                                 \
        name##Info(EcmaVM *vm,                                              \
                   const JSPandaFile *jsPandaFile,                          \
                   const PGOProfilerDecoder *pfDecoder)                     \
        : BaseSnapshotInfo(vm, jsPandaFile, pfDecoder) {}                   \
                                                                            \
        virtual void StoreDataToGlobalData(SnapshotGlobalData &globalData,  \
            const std::set<uint32_t> &skippedMethods) override;             \
    };

    DATA_TYPE_LIST(DEFINE_INFO_CLASS)
#undef DEFINE_INFO_CLASS

class SnapshotConstantPoolData {
public:
    SnapshotConstantPoolData(EcmaVM *vm, const JSPandaFile *jsPandaFile, const PGOProfilerDecoder *pfDecoder)
        : jsPandaFile_(jsPandaFile)
    {
#define ADD_INFO(V, name)                               \
    infos_.emplace_back(std::make_unique<name##Info>(vm, jsPandaFile, pfDecoder));
    DATA_TYPE_LIST(ADD_INFO)
#undef ADD_INFO
    }
    ~SnapshotConstantPoolData() = default;

    void PUBLIC_API Record(const BytecodeInstruction &bcIns, int32_t bcIndex,
                           const CString &recordName, const MethodLiteral *method);

    void StoreDataToGlobalData(SnapshotGlobalData &snapshotData, const std::set<uint32_t> &skippedMethods) const;

private:
    enum class Type {
#define DEFINE_TYPE(type, ...) type,
    DATA_TYPE_LIST(DEFINE_TYPE)
#undef DEFINE_TYPE
    };

    void RecordInfo(Type type, BaseSnapshotInfo::ItemData &itemData)
    {
        size_t infoIdx = static_cast<size_t>(type);
        infos_.at(infoIdx)->Record(itemData);
    }

    const JSPandaFile *jsPandaFile_;
    CVector<std::unique_ptr<BaseSnapshotInfo>> infos_ {};
};
#undef DATA_TYPE_LIST
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_DATA_H

