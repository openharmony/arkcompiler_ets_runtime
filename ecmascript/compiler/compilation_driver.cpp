/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/compilation_driver.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
CompilationDriver::CompilationDriver(PGOProfilerLoader &profilerLoader, BytecodeInfoCollector* collector)
    : vm_(collector->GetVM()),
      jsPandaFile_(collector->GetJSPandaFile()),
      pfLoader_(profilerLoader),
      bytecodeInfo_(collector->GetBytecodeInfo())
{
    vm_->GetTSManager()->SetCompilationDriver(this);
}

CompilationDriver::~CompilationDriver()
{
    vm_->GetTSManager()->SetCompilationDriver(nullptr);
}

void CompilationDriver::TopologicalSortForRecords()
{
    const auto &importRecordsInfos = bytecodeInfo_.GetImportRecordsInfos();
    std::queue<CString> recordList;
    std::unordered_map<CString, uint32_t> recordInDegree;
    std::unordered_map<CString, std::vector<CString>> exportRecords;
    std::vector<CString> &tpOrder = bytecodeInfo_.GetRecordNames();
    for (auto &record : tpOrder) {
        auto iter = importRecordsInfos.find(record);
        if (iter == importRecordsInfos.end()) {
            recordInDegree.emplace(record, 0);
            recordList.emplace(record);
        } else {
            recordInDegree.emplace(record, iter->second.GetImportRecordSize());
        }
    }
    tpOrder.clear();

    for (auto iter = importRecordsInfos.begin(); iter != importRecordsInfos.end(); iter++) {
        const auto &importRecords = iter->second.GetImportRecords();
        for (const auto &import : importRecords) {
            if (exportRecords.find(import) != exportRecords.end()) {
                exportRecords[import].emplace_back(iter->first);
            } else {
                exportRecords.emplace(import, std::vector<CString>{iter->first});
            }
        }
    }

    while (!recordList.empty()) {
        auto curRecord = recordList.front();
        tpOrder.emplace_back(curRecord);
        recordList.pop();
        auto iter = exportRecords.find(curRecord);
        if (iter != exportRecords.end()) {
            for (const auto &ele : iter->second) {
                if (recordInDegree[ele] > 0 && --recordInDegree[ele] == 0) {
                    recordList.emplace(ele);
                }
            }
        }
    }

    if (UNLIKELY(tpOrder.size() != recordInDegree.size())) {
        LOG_COMPILER(INFO) << "There are circular references in records";
        for (auto &it : recordInDegree) {
            if (it.second != 0) {
                tpOrder.emplace_back(it.first);
            }
        }
        ASSERT(tpOrder.size() == recordInDegree.size());
    }
    auto &mainMethods = bytecodeInfo_.GetMainMethodIndexes();
    auto sortId = 0;
    for (auto &it : tpOrder) {
        mainMethods.emplace_back(jsPandaFile_->GetMainMethodIndex(it));
        sortedRecords_.emplace(it, sortId++);
    }
    ASSERT(tpOrder.size() == mainMethods.size());
}

void CompilationDriver::UpdatePGO()
{
    std::unordered_set<EntityId> newMethodIds;
    auto dfs = [this, &newMethodIds] (const CString &recordName,
        const std::unordered_set<EntityId> &oldIds) -> std::unordered_set<EntityId> & {
            newMethodIds.clear();
            if (!jsPandaFile_->HasTSTypes(recordName)) {
                return newMethodIds;
            }
            uint32_t mainMethodOffset = jsPandaFile_->GetMainMethodIndex(recordName);
            SearchForCompilation(recordName, oldIds, newMethodIds, mainMethodOffset, false);
            return newMethodIds;
        };
    pfLoader_.Update(dfs);
}

void CompilationDriver::InitializeCompileQueue()
{
    TopologicalSortForRecords();
    auto &mainMethodIndexes = bytecodeInfo_.GetMainMethodIndexes();
    for (auto mainMethodIndex : mainMethodIndexes) {
        compileQueue_.push_back(mainMethodIndex);
    }
}

bool CompilationDriver::FilterMethod(const CString &recordName, const MethodLiteral *methodLiteral,
                                     const MethodPcInfo &methodPCInfo) const
{
    if (methodPCInfo.methodsSize > bytecodeInfo_.GetMaxMethodSize() ||
        !pfLoader_.Match(recordName, methodLiteral->GetMethodId())) {
        return true;
    }
    return false;
}
} // namespace panda::ecmascript::kungfu
