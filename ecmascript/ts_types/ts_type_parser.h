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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_PARSER_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_PARSER_H

#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
class TSTypeParser {
public:
    static constexpr size_t TYPE_KIND_INDEX_IN_LITERAL = 0;
    static constexpr size_t BUILDIN_TYPE_OFFSET = 20;
    static constexpr size_t USER_DEFINED_TYPE_OFFSET = 100;

    explicit TSTypeParser(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                          CVector<JSHandle<EcmaString>> &recordImportModules)
        : vm_(vm), thread_(vm_->GetJSThread()), factory_(vm_->GetFactory()), jsPandaFile_(jsPandaFile),
          recordImportModules_(recordImportModules) {}
    ~TSTypeParser() = default;

    JSHandle<JSTaggedValue> ParseType(JSHandle<TaggedArray> &literal);

    void SetTypeGT(JSHandle<JSTaggedValue> type, uint32_t moduleId, uint32_t localId)
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(moduleId, localId);
        JSHandle<TSType>(type)->SetGT(gt);
    }

    inline GlobalTSTypeRef CreateGT(uint32_t typeId)
    {
        return CreateGT(vm_, jsPandaFile_, typeId);
    }

    inline static GlobalTSTypeRef CreateGT(EcmaVM *vm, const JSPandaFile *jsPandaFile, uint32_t typeId)
    {
        if (typeId <= BUILDIN_TYPE_OFFSET) {
            return GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, typeId);
        }

        if (typeId <= USER_DEFINED_TYPE_OFFSET) {
            return GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, typeId - BUILDIN_TYPE_OFFSET);
        }

        TSManager *tsManager = vm->GetTSManager();
        panda_file::File::EntityId offset(typeId);
        return tsManager->GetGTFromOffset(jsPandaFile, offset);
    }

    inline CVector<JSHandle<EcmaString>> GetImportModules() const
    {
        return recordImportModules_;
    }

private:

    inline static uint32_t GetLocalIdInTypeTable(uint32_t typeId)
    {
        return typeId - USER_DEFINED_TYPE_OFFSET;
    }

    inline static GlobalTSTypeRef GetPrimitiveGT(uint32_t typeId)
    {
        return GlobalTSTypeRef(typeId);
    }

    inline static GlobalTSTypeRef GetBuiltinsGT(uint32_t typeId)
    {
        return GlobalTSTypeRef(typeId - BUILDIN_TYPE_OFFSET);  // not implement yet.
    }

    JSHandle<TSClassType> ParseClassType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSClassInstanceType> ParseClassInstanceType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSInterfaceType> ParseInterfaceType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSImportType> ParseImportType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSUnionType> ParseUnionType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSFunctionType> ParseFunctionType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSArrayType> ParseArrayType(const JSHandle<TaggedArray> &literal);

    JSHandle<TSObjectType> ParseObjectType(const JSHandle<TaggedArray> &literal);

    void FillPropertyTypes(JSHandle<TSObjLayoutInfo> &layOut, const JSHandle<TaggedArray> &literal,
                           uint32_t startIndex, uint32_t lastIndex, uint32_t &index, bool isField);

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    CVector<JSHandle<EcmaString>> recordImportModules_ {};
};
}  // panda::ecmascript
#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_PARSER_H
