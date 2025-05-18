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

#include "ecmascript/compiler/aot_snapshot/aot_snapshot.h"

#include "ecmascript/compiler/aot_snapshot/aot_snapshot_constants.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript::kungfu {
void AOTSnapshot::InitSnapshot(uint32_t compileFilesCount)
{
    JSHandle<TaggedArray> data = factory_->NewTaggedArray(compileFilesCount *
                                                          AOTSnapshotConstants::SNAPSHOT_DATA_ITEM_SIZE);
    snapshotData_.SetData(data.GetTaggedValue());
}

JSHandle<ConstantPool> AOTSnapshot::NewSnapshotConstantPool(uint32_t cacheSize)
{
    JSHandle<ConstantPool> cp = factory_->NewConstantPool(cacheSize);

    ASSERT(!snapshotData_.GetHClassInfo().IsHole());
    cp->SetAotSymbolInfo(vm_->GetJSThread(), snapshotData_.GetSymbolInfo());
    cp->SetAotHClassInfo(vm_->GetJSThread(), snapshotData_.GetHClassInfo());
    cp->SetAotArrayInfo(vm_->GetJSThread(), snapshotData_.GetArrayInfo());
    cp->SetConstantIndexInfo(vm_->GetJSThread(), snapshotData_.GetConstantIndexInfo());
    cp->SetProtoTransTableInfo(vm_->GetJSThread(), snapshotData_.GetProtoTransTableInfo());
    return cp;
}

void AOTSnapshot::GenerateSnapshotConstantPools(const CMap<int32_t, JSTaggedValue> &allConstantPools,
                                                const CString &fileName, uint32_t fileIndex)
{
    JSHandle<TaggedArray> snapshotCpArr = factory_->NewTaggedArray(allConstantPools.size() *
                                                                   AOTSnapshotConstants::SNAPSHOT_CP_ARRAY_ITEM_SIZE);
    snapshotData_.AddSnapshotCpArrayToData(thread_, fileName, fileIndex, snapshotCpArr);

    JSMutableHandle<ConstantPool> cp(thread_, thread_->GlobalConstants()->GetUndefined());
    uint32_t pos = 0;
    for (auto &iter : allConstantPools) {
        int32_t cpId = iter.first;
        cp.Update(iter.second);
        // cachedSize should not have extra data included
        uint32_t cacheSize = cp->GetCacheLength() - ConstantPool::EXTEND_DATA_NUM;
        if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
            LOG_COMPILER(INFO) << "[aot-snapshot] constantPoolID: " << cpId;
            LOG_COMPILER(INFO) << "[aot-snapshot] cacheSize: " << cacheSize;
        }

        JSHandle<ConstantPool> newCp = NewSnapshotConstantPool(cacheSize);
        snapshotCpArr->Set(thread_, pos++, JSTaggedValue(cpId));
        snapshotData_.RecordCpArrIdx(cpId, pos);
        snapshotCpArr->Set(thread_, pos++, newCp.GetTaggedValue());
    }
}

void AOTSnapshot::StoreConstantPoolInfo(BytecodeInfoCollector *bcInfoCollector)
{
    const JSPandaFile *jsPandaFile = bcInfoCollector->GetJSPandaFile();
    auto constpoolsOptional = vm_->FindConstpools(jsPandaFile);
    if (!constpoolsOptional.has_value()) {
        LOG_COMPILER(ERROR) << "StoreConstantPoolInfo failed. no constant pools found for"
                            << jsPandaFile->GetJSPandaFileDesc();
        return;
    }
    const CMap<int32_t, JSTaggedValue> &allConstantPools = constpoolsOptional.value();
    pgo::ApEntityId fileId = INVALID_INDEX;
    if (!pgo::PGOProfilerManager::GetInstance()->GetPandaFileId(jsPandaFile->GetJSPandaFileDesc(), fileId)) {
        LOG_COMPILER(ERROR) << "StoreConstantPoolInfo failed. no file id found for "
                            << jsPandaFile->GetJSPandaFileDesc();
        return;
    }
    GenerateSnapshotConstantPools(allConstantPools, jsPandaFile->GetNormalizedFileDesc(), fileId);
    bcInfoCollector->StoreDataToGlobalData(snapshotData_);
}
}  // namespace panda::ecmascript
