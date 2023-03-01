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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_GENERATOR_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_GENERATOR_H

#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
class TSTypeTableGenerator {
public:
    explicit TSTypeTableGenerator(TSManager *tsManager)
        : tsManager_(tsManager), vm_(tsManager->GetEcmaVM()),
          thread_(vm_->GetJSThread()), factory_(vm_->GetFactory()) {}
    ~TSTypeTableGenerator() = default;

    void GenerateDefaultTSTypeTables();

    JSHandle<TSTypeTable> GetOrGenerateTSTypeTable(const JSPandaFile *jsPandaFile,
                                                   const CString &recordName,
                                                   uint32_t moduleId);

    uint32_t TryGetModuleId(const CString &recordName) const;

    uint32_t TryGetLocalId(const JSHandle<TSTypeTable> &table) const;

private:
    inline JSHandle<TSModuleTable> GetTSModuleTable() const
    {
        return tsManager_->GetTSModuleTable();
    }

    int GetNextModuleId() const
    {
        JSHandle<TSModuleTable> table = GetTSModuleTable();
        return table->GetNumberOfTSTypeTables();
    }

    JSHandle<TSTypeTable> AddTypeTable(const JSHandle<EcmaString> &recordNameStr, uint32_t numTypes = 0);

    void GenerateBuiltinsTypeTable();

    void InitRuntimeTypeTable();

    void FillLayoutTypes(const JSHandle<TSObjLayoutInfo> &layout,
                         const std::vector<JSHandle<JSTaggedValue>> &prop,
                         const std::vector<GlobalTSTypeRef> &propType);

    TSManager *tsManager_ {nullptr};
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
};
}  // panda::ecmascript
#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_GENERATOR_H
