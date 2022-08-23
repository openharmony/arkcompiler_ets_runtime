/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TS_TYPES_TS_MANAGER_H
#define ECMASCRIPT_TS_TYPES_TS_MANAGER_H

#include "ecmascript/mem/c_string.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/ts_types/global_ts_type_ref.h"

namespace panda::ecmascript {
enum class MTableIdx : uint8_t {
    PRIMITIVE = 0,
    BUILTIN,
    INFERRED_UNTION,
    NUM_OF_DEFAULT_TABLES,
};

class TSModuleTable : public TaggedArray {
public:

    static constexpr int AMI_PATH_OFFSET = 1;
    static constexpr int SORT_ID_OFFSET = 2;
    static constexpr int TYPE_TABLE_OFFSET = 3;
    static constexpr int ELEMENTS_LENGTH = 3;
    static constexpr int NUMBER_OF_TABLES_INDEX = 0;
    static constexpr int INCREASE_CAPACITY_RATE = 2;
    static constexpr int DEFAULT_NUMBER_OF_TABLES = 3;  // primitive table, builtins table and infer table
    // first +1 means reserve a table from pandafile, second +1 menas the NUMBER_OF_TABLES_INDEX
    static constexpr int DEFAULT_TABLE_CAPACITY =  (DEFAULT_NUMBER_OF_TABLES + 1) * ELEMENTS_LENGTH + 1;
    static constexpr int PRIMITIVE_TABLE_ID = 0;
    static constexpr int BUILTINS_TABLE_ID = 1;
    static constexpr int INFER_TABLE_ID = 2;
    static constexpr int NOT_FOUND = -1;

    static TSModuleTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSModuleTable *>(object);
    }

    static void Initialize(JSThread *thread, JSHandle<TSModuleTable> mTable);

    static JSHandle<TSModuleTable> AddTypeTable(JSThread *thread, JSHandle<TSModuleTable> table,
                                                JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    JSHandle<EcmaString> GetAmiPathByModuleId(JSThread *thread, int entry) const;

    JSHandle<TSTypeTable> GetTSTypeTable(JSThread *thread, int entry) const;

    int GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const;

    inline int GetNumberOfTSTypeTables() const
    {
        return Get(NUMBER_OF_TABLES_INDEX).GetInt();
    }

    inline void SetNumberOfTSTypeTables(JSThread *thread, int num)
    {
        Set(thread, NUMBER_OF_TABLES_INDEX, JSTaggedValue(num));
    }

    static uint32_t GetTSTypeTableOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + TYPE_TABLE_OFFSET;
    }

    static JSHandle<TSTypeTable> GenerateBuiltinsTypeTable(JSThread *thread);

private:

    static int GetAmiPathOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + AMI_PATH_OFFSET;
    }

    static int GetSortIdOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + SORT_ID_OFFSET;
    }
};

class TSManager {
public:
    explicit TSManager(EcmaVM *vm);
    ~TSManager() = default;

    void Initialize();

    void PUBLIC_API DecodeTSTypes(const JSPandaFile *jsPandaFile);

    void AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    void Link();

    void Dump();

    void Iterate(const RootVisitor &v);

    JSHandle<TSModuleTable> GetTSModuleTable() const
    {
        return JSHandle<TSModuleTable>(reinterpret_cast<uintptr_t>(&globalModuleTable_));
    }

    CVector<JSTaggedType> GetConstStringTable() const
    {
        return constantStringTable_;
    }

    void ClearConstStringTable()
    {
        constantStringTable_.clear();
    }

    void SetTSModuleTable(JSHandle<TSModuleTable> table)
    {
        globalModuleTable_ = table.GetTaggedValue();
    }

    int GetNextModuleId() const
    {
        JSHandle<TSModuleTable> table = GetTSModuleTable();
        return table->GetNumberOfTSTypeTables();
    }

    JSHandle<EcmaString> GenerateAmiPath(JSHandle<EcmaString> cur, JSHandle<EcmaString> rel) const;

    JSHandle<EcmaString> GenerateImportVar(JSHandle<EcmaString> import) const;

    JSHandle<EcmaString> GenerateImportRelativePath(JSHandle<EcmaString> importRel) const;

    GlobalTSTypeRef PUBLIC_API CreateGT(const panda_file::File &pf, const uint32_t localId) const;

    GlobalTSTypeRef PUBLIC_API GetImportTypeTargetGT(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, JSHandle<EcmaString> propertyName) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetPropType(gt, propertyName);
    }

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName) const;

    // use for object
    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, const uint64_t key) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetPropType(gt, key);
    }

    inline GlobalTSTypeRef PUBLIC_API CreateClassInstanceType(kungfu::GateType gateType)
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return CreateClassInstanceType(gt);
    }

    GlobalTSTypeRef PUBLIC_API CreateClassInstanceType(GlobalTSTypeRef gt);

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, const uint64_t key) const;

    uint32_t PUBLIC_API GetUnionTypeLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeVec);

    int PUBLIC_API GetFunctionTypLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const;

    inline GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetFuncReturnValueTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetArrayParameterTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(GlobalTSTypeRef gt) const;

    size_t PUBLIC_API AddConstString(JSTaggedValue string);

    bool PUBLIC_API IsTypeVerifyEnabled() const;

    bool IsBuiltinsDTSEnabled() const
    {
        return vm_->GetJSOptions().WasSetBuiltinsDTS();
    }

    CString GetBuiltinsDTS() const
    {
        std::string fileName = vm_->GetJSOptions().GetBuiltinsDTS();
        return CString(fileName);
    }

    // add string to constantstringtable and get its index
    size_t PUBLIC_API GetStringIdx(JSHandle<JSTaggedValue> constPool, const uint16_t id);

    /*
     * Before using this method for type infer, you need to wait until all the
     * string objects of the panda_file are collected, otherwise an error will
     * be generated, and it will be optimized later.
     */
    JSHandle<EcmaString> PUBLIC_API GetStringById(size_t index) const
    {
        ASSERT(index < constantStringTable_.size());
        return JSHandle<EcmaString>(reinterpret_cast<uintptr_t>(&(constantStringTable_.at(index))));
    }

    std::string PUBLIC_API GetStdStringById(size_t index) const;

    CVector<JSTaggedType> GetStaticHClassTable() const
    {
        return staticHClassTable_;
    }

    void AddStaticHClassInCompilePhase(GlobalTSTypeRef gt, JSTaggedValue hclass)
    {
        staticHClassTable_.emplace_back(hclass.GetRawData());
        gtHClassIndexMap_[gt] = staticHClassTable_.size() - 1;
    }

    void AddStaticHClassInRuntimePhase(JSTaggedValue hclass)
    {
        staticHClassTable_.emplace_back(hclass.GetRawData());
    }

    std::map<GlobalTSTypeRef, uint32_t> GetGtHClassIndexMap()
    {
        return gtHClassIndexMap_;
    }

    void GenerateStaticHClass(JSHandle<TSTypeTable> tsTypeTable);

    JSHandle<JSTaggedValue> GetTSType(const GlobalTSTypeRef &gt) const;

    std::string PUBLIC_API GetTypeStr(kungfu::GateType gateType) const;

#define IS_TSTYPEKIND_METHOD_LIST(V)              \
    V(Primitive, TSTypeKind::PRIMITIVE)           \
    V(Class, TSTypeKind::CLASS)                   \
    V(ClassInstance, TSTypeKind::CLASS_INSTANCE)  \
    V(Function, TSTypeKind::FUNCTION)             \
    V(Union, TSTypeKind::UNION)                   \
    V(Array, TSTypeKind::ARRAY)                   \
    V(Object, TSTypeKind::OBJECT)                 \
    V(ImportT, TSTypeKind::IMPORT)                \
    V(Interface, TSTypeKind::INTERFACE_KIND)

#define IS_TSTYPEKIND(NAME, TSTYPEKIND)                                                \
    bool inline PUBLIC_API Is##NAME##TypeKind(const kungfu::GateType &gateType) const  \
    {                                                                                  \
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());                      \
        ASSERT(gt.GetFlag() == 0);                                                     \
        return GetTypeKind(gt) == (TSTYPEKIND);                                        \
    }

    IS_TSTYPEKIND_METHOD_LIST(IS_TSTYPEKIND)
#undef IS_TSTYPEKIND

private:

    NO_COPY_SEMANTIC(TSManager);
    NO_MOVE_SEMANTIC(TSManager);

    void LinkTSTypeTable(JSHandle<TSTypeTable> table);

    void RecursivelyResolveTargetType(JSMutableHandle<TSImportType>& importType);

    int GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const;

    GlobalTSTypeRef PUBLIC_API AddUnionToInferTable(JSHandle<TSUnionType> unionType);

    GlobalTSTypeRef FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const;

    JSHandle<TSTypeTable> GetInferTypeTable() const;

    void SetInferTypeTable(JSHandle<TSTypeTable> inferTable);

    TSTypeKind PUBLIC_API GetTypeKind(const GlobalTSTypeRef &gt) const;

    std::string GetPrimitiveStr(const GlobalTSTypeRef &gt) const;

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    CVector<JSTaggedType> constantStringTable_ {};
    CVector<JSTaggedType> staticHClassTable_ {};  // store hclass which produced from static type info
    std::map<GlobalTSTypeRef, uint32_t> gtHClassIndexMap_ {};  // record gt and static hclass index mapping relation
    friend class EcmaVM;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_MANAGER_H
