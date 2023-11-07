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
#ifndef ECMASCRIPT_COMPILER_AOT_SNAPSHOT_SNAPSHOT_GLOBAL_DATA_H
#define ECMASCRIPT_COMPILER_AOT_SNAPSHOT_SNAPSHOT_GLOBAL_DATA_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/compiler/aot_snapshot/aot_snapshot_constants.h"

namespace panda::ecmascript::kungfu {
class SnapshotGlobalData;
/*
 * The information that needs to be revised before saving the 'ai' file is recorded in SnapshotReviseData.
 * Currently, the revised information includes the entry index of each method in the 'an' file.
 */
#define REVISE_DATA_TYPE_LIST(V) \
    V(METHOD, Method)            \
    V(LITERAL, Literal)

class BaseReviseData {
public:
    struct ItemData {
        uint32_t dataIdx_;
        uint32_t cpArrayIdx_;
        int32_t constpoolIdx_;
    };

    BaseReviseData() = default;
    virtual ~BaseReviseData() = default;

    void Record(ItemData data)
    {
        data_.emplace_back(data);
    }

    virtual void Resolve(JSThread *thread, const SnapshotGlobalData *globalData,
                         const CMap<std::pair<std::string, uint32_t>, uint32_t> &methodToEntryIndexMap) = 0;

protected:
    JSHandle<ConstantPool> GetConstantPoolFromSnapshotData(JSThread *thread, const SnapshotGlobalData *globalData,
                                                           uint32_t dataIdx, uint32_t cpArrayIdx);

    using ReviseData = std::vector<ItemData>;
    ReviseData data_ {};
};

#define DEFINE_REVISE_CLASS(V, name)                                                                 \
    class name##ReviseData final : public BaseReviseData {                                           \
    public:                                                                                          \
        virtual void Resolve(JSThread *thread, const SnapshotGlobalData *globalData,                 \
            const CMap<std::pair<std::string, uint32_t>, uint32_t> &methodToEntryIndexMap) override; \
    };

    REVISE_DATA_TYPE_LIST(DEFINE_REVISE_CLASS)
#undef DEFINE_REVISE_CLASS

class SnapshotReviseInfo {
public:
    enum class Type {
#define DEFINE_TYPE(type, ...) type,
    REVISE_DATA_TYPE_LIST(DEFINE_TYPE)
#undef DEFINE_TYPE
    };

    SnapshotReviseInfo()
    {
#define ADD_REVISE_DATA(V, name)                                   \
    reviseData_.emplace_back(std::make_unique<name##ReviseData>());
    REVISE_DATA_TYPE_LIST(ADD_REVISE_DATA)
#undef ADD_REVISE_DATA
    }
    ~SnapshotReviseInfo() = default;

    void Record(Type type, BaseReviseData::ItemData data)
    {
        size_t reviseDataIdx = static_cast<size_t>(type);
        reviseData_.at(reviseDataIdx)->Record(data);
    }

    void ResolveData(JSThread *thread, const SnapshotGlobalData *globalData,
                     const CMap<std::pair<std::string, uint32_t>, uint32_t> &methodToEntryIndexMap)
    {
        for (auto &data : reviseData_) {
            data->Resolve(thread, globalData, methodToEntryIndexMap);
        }
    }
private:
    CVector<std::unique_ptr<BaseReviseData>> reviseData_ {};
};
#undef REVISE_DATA_TYPE_LIST

class SnapshotGlobalData {
public:
    static constexpr uint32_t CP_ARRAY_OFFSET = 1;

    SnapshotGlobalData() = default;
    ~SnapshotGlobalData() = default;

    void Iterate(const RootVisitor &v)
    {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&data_)));
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&curSnapshotCpArray_)));
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&hclassInfo_)));
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&arrayInfo_)));
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&constantIndexInfo_)));
    }

    void SetData(JSTaggedValue data)
    {
        data_ = data;
    }

    JSTaggedValue GetData() const
    {
        return data_;
    }

    uint32_t GetCurDataIdx() const
    {
        return curDataIdx_;
    }

    JSTaggedValue GetCurSnapshotCpArray() const
    {
        return curSnapshotCpArray_;
    }

    void AddSnapshotCpArrayToData(JSThread *thread, CString fileName, JSHandle<TaggedArray> snapshotCpArray);

    CString GetFileNameByDataIdx(uint32_t dataIdx) const;

    void RecordReviseData(SnapshotReviseInfo::Type type, BaseReviseData::ItemData data)
    {
        reviseInfo_.Record(type, data);
    }

    void ResolveSnapshotData(JSThread *thread,
                             const CMap<std::pair<std::string, uint32_t>, uint32_t> &methodToEntryIndexMap)
    {
        reviseInfo_.ResolveData(thread, this, methodToEntryIndexMap);
    }

    void RecordCpArrIdx(int32_t constantPoolId, uint32_t cpArrIdx)
    {
        dataIdxToCpArrIdxMap_[curDataIdx_][constantPoolId] = cpArrIdx;
    }

    uint32_t GetCpArrIdxByConstanPoolId(int32_t constantPoolId)
    {
        return GetCpIdToCpArrIdxMap().at(constantPoolId);
    }

    const CUnorderedMap<int32_t, uint32_t>& GetCpIdToCpArrIdxMap()
    {
        return dataIdxToCpArrIdxMap_.at(curDataIdx_);
    }

    JSTaggedValue GetHClassInfo()
    {
        return hclassInfo_;
    }

    JSTaggedValue GetArrayInfo()
    {
        return arrayInfo_;
    }

    JSTaggedValue GetConstantIndexInfo()
    {
        return constantIndexInfo_;
    }

    void StoreHClassInfo(JSHandle<TaggedArray> info)
    {
        hclassInfo_ = info.GetTaggedValue();
    }

    void StoreArrayInfo(JSHandle<TaggedArray> info)
    {
        arrayInfo_ = info.GetTaggedValue();
    }

    void StoreConstantIndexInfo(JSHandle<TaggedArray> info)
    {
        constantIndexInfo_ = info.GetTaggedValue();
    }

private:
    using CpIdToCpArrIdxMap = CUnorderedMap<int32_t, uint32_t>;

    bool isFirstData_ {true};
    uint32_t curDataIdx_ {0};
    JSTaggedValue data_ {JSTaggedValue::Hole()};
    JSTaggedValue curSnapshotCpArray_ {JSTaggedValue::Hole()};
    CUnorderedMap<uint32_t, CpIdToCpArrIdxMap> dataIdxToCpArrIdxMap_;
    CUnorderedMap<uint32_t, CString> dataIdxToFileNameMap_ {};

    SnapshotReviseInfo reviseInfo_;
    JSTaggedValue hclassInfo_ {JSTaggedValue::Hole()};
    JSTaggedValue arrayInfo_ {JSTaggedValue::Hole()};
    JSTaggedValue constantIndexInfo_ {JSTaggedValue::Hole()};
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_AOT_SNAPSHOT_SNAPSHOT_GLOBAL_DATA_H
