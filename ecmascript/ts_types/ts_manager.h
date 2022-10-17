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
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/ts_types/global_ts_type_ref.h"

namespace panda::ecmascript {
enum class MTableIdx : uint8_t {
    PRIMITIVE = 0,
    BUILTIN,
    INFERRED_UNTION,
    NUM_OF_DEFAULT_TABLES,
};

enum class CacheKind: uint8_t {
    CONST_DATA_INDEX = 0,
    HCLASS,
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

    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, JSTaggedValue propertyName) const
    {
        return GetPropType(gateType, JSHandle<EcmaString>(vm_->GetJSThread(), propertyName));
    }

    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, JSHandle<EcmaString> propertyName) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetPropType(gt, propertyName);
    }

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName) const;

    // use for object
    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, const uint64_t key) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetPropType(gt, key);
    }

    inline GlobalTSTypeRef PUBLIC_API CreateClassInstanceType(kungfu::GateType gateType)
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return CreateClassInstanceType(gt);
    }

    GlobalTSTypeRef PUBLIC_API CreateClassInstanceType(GlobalTSTypeRef gt);

    GlobalTSTypeRef PUBLIC_API GetClassType(GlobalTSTypeRef classInstanceGT) const;

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, const uint64_t key) const;

    uint32_t PUBLIC_API GetUnionTypeLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeVec);

    uint32_t PUBLIC_API GetFunctionTypeLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetFuncThisGT(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetFuncReturnValueTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetArrayParameterTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(GlobalTSTypeRef gt) const;

    bool PUBLIC_API AssertTypes() const
    {
        return assertTypes_;
    }

    bool PUBLIC_API PrintAnyTypes() const
    {
        return printAnyTypes_;
    }

    bool IsBuiltinsDTSEnabled() const
    {
        return vm_->GetJSOptions().WasSetBuiltinsDTS();
    }

    CString GetBuiltinsDTS() const
    {
        std::string fileName = vm_->GetJSOptions().GetBuiltinsDTS();
        return CString(fileName);
    }

    const std::map<GlobalTSTypeRef, uint32_t> &GetClassTypeIhcIndexMap() const
    {
        return classTypeIhcIndexMap_;
    }

    void GenerateStaticHClass(JSHandle<TSTypeTable> tsTypeTable, const JSPandaFile *jsPandaFile);

    JSHandle<JSTaggedValue> GetTSType(const GlobalTSTypeRef &gt) const;

    std::string PUBLIC_API GetTypeStr(kungfu::GateType gateType) const;

    void PUBLIC_API CreateConstantPoolInfos(size_t size);

    void PUBLIC_API CollectConstantPoolInfo(const JSPandaFile* jsPandaFile);

    JSHandle<TaggedArray> PUBLIC_API GetConstantPoolInfos() const
    {
        return JSHandle<TaggedArray>(uintptr_t(&constantPoolInfos_));
    }

    void PUBLIC_API SetConstantPoolInfos(JSTaggedValue constantPoolInfos)
    {
        constantPoolInfos_ = constantPoolInfos;
    }

    void PUBLIC_API SortConstantPoolInfos();

    JSHandle<ConstantPool> PUBLIC_API RestoreConstantPool(const JSPandaFile* pf, uint32_t constantPool);

    void ClearCaches()
    {
        constIndexCache_.clear();
        hclassCache_.clear();
    }

    void AddStringIndex(uint32_t index)
    {
        if (constIndexCache_.find({ConstPoolType::STRING, index}) != constIndexCache_.end()) {
            return;
        }
        constIndexCache_.insert({ConstPoolType::STRING, index});
    }

    void AddMethodIndex(uint32_t index)
    {
        if (constIndexCache_.find({ConstPoolType::METHOD, index}) != constIndexCache_.end()) {
            return;
        }
        constIndexCache_.insert({ConstPoolType::METHOD, index});
    }

    void AddClassLiteraIndex(uint32_t index)
    {
        if (constIndexCache_.find({ConstPoolType::CLASS_LITERAL, index}) != constIndexCache_.end()) {
            return;
        }
        constIndexCache_.insert({ConstPoolType::CLASS_LITERAL, index});
    }

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    JSThread *GetThread() const
    {
        return thread_;
    }

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
        GlobalTSTypeRef gt = gateType.GetGTRef();                                      \
        return GetTypeKind(gt) == (TSTYPEKIND);                                        \
    }

    IS_TSTYPEKIND_METHOD_LIST(IS_TSTYPEKIND)
#undef IS_TSTYPEKIND

    static constexpr int BUILTIN_ARRAY_ID = 24;
    static constexpr int BUILTIN_TYPED_ARRAY_FIRST_ID = 25;
    static constexpr int BUILTIN_TYPED_ARRAY_LAST_ID = 36;

    bool IsTypedArrayType(kungfu::GateType gateType) const
    {
        if (!IsClassInstanceTypeKind(gateType)) {
            return false;
        }

        const GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.Value());
        GlobalTSTypeRef classGT = GetClassType(gt);
        uint32_t m = classGT.GetModuleId();
        uint32_t l = classGT.GetLocalId();
        return (m == TSModuleTable::BUILTINS_TABLE_ID) &&
               (l >= BUILTIN_TYPED_ARRAY_FIRST_ID) && (l <= BUILTIN_TYPED_ARRAY_LAST_ID);
    }

    void AddElementToLiteralOffsetGTMap(const JSPandaFile *jsPandaFile, panda_file::File::EntityId offset,
                                        GlobalTSTypeRef gt)
    {
        auto key = std::make_pair(jsPandaFile, offset);
        literalOffsetGTMap_.emplace(key, gt);
    }

    GlobalTSTypeRef GetGTFromOffset(const JSPandaFile *jsPandaFile, panda_file::File::EntityId offset) const
    {
        auto key = std::make_pair(jsPandaFile, offset);
        return literalOffsetGTMap_.at(key);
    }

private:
    // constantpoolInfos
    static constexpr uint32_t CONSTANTPOOL_INFO_DATA_OFFSET = 2;
    static constexpr uint32_t CONSTANTPOOL_INFOS_ITEM_SIZE = 2;
    static constexpr uint32_t NUM_OF_ORIGINAL_CONSTANTPOOL_DATA_INDEX = 0;
    static constexpr uint32_t ORIGINAL_CONSTANTPOOL_DATA_SIZE = 2;
    static constexpr uint32_t NUM_OF_HCLASS_INDEX = 1;

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

    JSTaggedValue GenerateConstantPoolInfo(const JSPandaFile* jsPandaFile);

    int BinarySearch(uint32_t target, uint32_t itemSize, bool findLeftBound = true);

    uint32_t ComputeSizeOfConstantPoolInfo() const
    {
        return CONSTANTPOOL_INFO_DATA_OFFSET + ORIGINAL_CONSTANTPOOL_DATA_SIZE *
               GetConstDataCacheSize() + GetHClassCacheSize();
    }

    uint32_t GetHClassCacheSize() const
    {
        return hclassCache_.size();
    }

    uint32_t GetConstDataCacheSize() const
    {
        return constIndexCache_.size();
    }

    void AddHClassInCompilePhase(GlobalTSTypeRef gt, JSTaggedValue hclass, uint32_t constantPoolLen)
    {
        hclassCache_.emplace_back(hclass.GetRawData());
        classTypeIhcIndexMap_[gt] = constantPoolLen + hclassCache_.size() - 1;
    }

    template <class Callback>
    void IterateConstDataCaches(const Callback &cb)
    {
        for (auto item: constIndexCache_) {
            cb(std::get<0>(item), std::get<1>(item));
        }
    }

    template <class Callback>
    void IterateHClassCaches(const Callback &cb)
    {
        for (JSTaggedType item: hclassCache_) {
            cb(item);
        }
    }

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    JSTaggedValue constantPoolInfos_ {JSTaggedValue::Hole()};
    size_t constantPoolInfosSize_ {0};
    // record the mapping relationship between classType and instance hclass index in the constant pool
    std::map<GlobalTSTypeRef, uint32_t> classTypeIhcIndexMap_ {};
    bool assertTypes_ {false};
    bool printAnyTypes_ {false};
    friend class EcmaVM;

    // recode the index of String in each constpool
    std::set<std::tuple<ConstPoolType, int>>constIndexCache_ {};
    // store hclass of each abc which produced from static type info
    CVector<JSTaggedType> hclassCache_ {};

    std::map<std::pair<const JSPandaFile *, panda_file::File::EntityId>, GlobalTSTypeRef> literalOffsetGTMap_ {};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_MANAGER_H
