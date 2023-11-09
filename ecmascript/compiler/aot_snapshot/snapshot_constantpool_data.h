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
#ifndef ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_DATA_H
#define ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_DATA_H

#include "ecmascript/compiler/aot_snapshot/snapshot_global_data.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript::kungfu {
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
    };

    BaseSnapshotInfo() = default;
    virtual ~BaseSnapshotInfo() = default;

    virtual void StoreDataToGlobalData(EcmaVM *vm, const JSPandaFile *jsPandaFile,
        SnapshotGlobalData &globalData, const std::set<uint32_t> &skippedMethods) = 0;

    void Record(ItemData &data);

protected:
    using ItemKey = uint64_t;

    static constexpr uint32_t CONSTPOOL_MASK = 32;

    static ItemKey GetItemKey(uint32_t constantPoolId, uint32_t constantPoolIdx);

    void CollectLiteralInfo(EcmaVM *vm, JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                            JSHandle<ConstantPool> snapshotConstantPool, const std::set<uint32_t> &skippedMethods,
                            JSHandle<JSTaggedValue> ihc, JSHandle<JSTaggedValue> chc);

    CUnorderedMap<ItemKey, ItemData> info_;
};

#define DEFINE_INFO_CLASS(V, name)                                                                \
    class name##Info final : public BaseSnapshotInfo {                                            \
    public:                                                                                       \
        virtual void StoreDataToGlobalData(EcmaVM *vm, const JSPandaFile *jsPandaFile,            \
            SnapshotGlobalData &globalData, const std::set<uint32_t> &skippedMethods) override;   \
    };

    DATA_TYPE_LIST(DEFINE_INFO_CLASS)
#undef DEFINE_INFO_CLASS

class SnapshotConstantPoolData {
public:
    SnapshotConstantPoolData(EcmaVM *vm, const JSPandaFile *jsPandaFile)
        : vm_(vm), jsPandaFile_(jsPandaFile)
    {
#define ADD_INFO(V, name)                               \
    infos_.emplace_back(std::make_unique<name##Info>());
    DATA_TYPE_LIST(ADD_INFO)
#undef ADD_INFO
    }
    ~SnapshotConstantPoolData() = default;

    void Record(const BytecodeInstruction &bcIns, int32_t bcIndex,
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

    EcmaVM *vm_;
    const JSPandaFile *jsPandaFile_;
    CVector<std::unique_ptr<BaseSnapshotInfo>> infos_ {};
};
#undef DATA_TYPE_LIST
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_DATA_H

