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

#include "ecmascript/compiler/aot_snapshot/snapshot_global_data.h"

#include "ecmascript/compiler/aot_snapshot/aot_snapshot_constants.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::kungfu {
JSHandle<ConstantPool> ReviseData::GetConstantPoolFromSnapshotData(JSThread *thread,
                                                                   const SnapshotGlobalData *globalData,
                                                                   uint32_t dataIdx, uint32_t cpArrayIdx)
{
    JSHandle<TaggedArray> data(thread, globalData->GetData());
    auto cpArrayOffset = SnapshotGlobalData::Cast(SnapshotGlobalData::CP_TOP_ITEM::CP_ARRAY_ID);
    JSHandle<TaggedArray> cpArr(thread, data->Get(dataIdx + cpArrayOffset));
    return JSHandle<ConstantPool>(thread, cpArr->Get(cpArrayIdx));
}

void ReviseData::Resolve(JSThread *thread, const SnapshotGlobalData *globalData,
    const CMap<std::pair<std::string, uint32_t>, uint32_t> &methodToEntryIndexMap)
{
    for (auto &item: data_) {
        JSHandle<ConstantPool> newCP = GetConstantPoolFromSnapshotData(thread, globalData,
                                                                       item.dataIdx_, item.cpArrayIdx_);

        JSTaggedValue val = newCP->GetObjectFromCache(item.constpoolIdx_);
        AOTLiteralInfo *aotLiteralInfo = AOTLiteralInfo::Cast(val.GetTaggedObject());
        uint32_t aotLiteralInfoLen = aotLiteralInfo->GetCacheLength();
        std::string name = globalData->GetFileNameByDataIdx(item.dataIdx_).c_str();
        for (uint32_t i = 0; i < aotLiteralInfoLen; ++i) {
            JSTaggedValue methodOffsetVal = aotLiteralInfo->GetObjectFromCache(i);
            if (methodOffsetVal.GetInt() == -1) {
                continue;
            }
            uint32_t methodOffset = static_cast<uint32_t>(methodOffsetVal.GetInt());
            if (thread->GetEcmaVM()->GetJSOptions().IsEnableCompilerLogSnapshot()) {
                LOG_COMPILER(INFO) << "[aot-snapshot] store AOT entry index of method (offset: "
                                   << methodOffset << ") ";
            }
            AnFileInfo::FuncEntryIndexKey key = std::make_pair(name, methodOffset);
            uint32_t entryIndex = methodToEntryIndexMap.at(key);
            aotLiteralInfo->SetObjectToCache(thread, i, JSTaggedValue(entryIndex));
        }
    }
}

void SnapshotGlobalData::AddSnapshotCpArrayToData(JSThread *thread, CString fileName, uint32_t fileIndex,
                                                  JSHandle<TaggedArray> snapshotCpArray)
{
    if (isFirstData_) {
        isFirstData_ = false;
    } else {
        curDataIdx_ += AOTSnapshotConstants::SNAPSHOT_DATA_ITEM_SIZE;
    }
    // handle file info
    JSHandle<EcmaString> nameStr = thread->GetEcmaVM()->GetFactory()->NewFromStdString(fileName.c_str());
    auto fileInfo = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(Cast(CP_PANDA_INFO_ITEM::COUNT));
    fileInfo->Set(thread, Cast(CP_PANDA_INFO_ITEM::NAME_ID), nameStr);
    fileInfo->Set(thread, Cast(CP_PANDA_INFO_ITEM::INDEX_ID), JSTaggedValue(fileIndex));

    JSHandle<TaggedArray> dataHandle(thread, data_);
    dataHandle->Set(thread, curDataIdx_ + Cast(CP_TOP_ITEM::PANDA_INFO_ID), fileInfo);

    // handle constant pool
    curSnapshotCpArray_ = snapshotCpArray.GetTaggedValue();
    dataHandle->Set(thread, curDataIdx_ + Cast(CP_TOP_ITEM::CP_ARRAY_ID), curSnapshotCpArray_);
    dataIdxToFileNameMap_[curDataIdx_] = fileName;
}

CString SnapshotGlobalData::GetFileNameByDataIdx(uint32_t dataIdx) const
{
    auto it = dataIdxToFileNameMap_.find(dataIdx);
    if (it != dataIdxToFileNameMap_.end()) {
        return it->second;
    }
    LOG_COMPILER(FATAL) << "Can't find snapshot data by index '" << dataIdx << "'";
    UNREACHABLE();
}
}  // namespace panda::ecmascript
