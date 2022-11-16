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

enum class PropertyType : uint8_t {
    NORMAL = 0,
    STATIC,
    OTHERS,
};

enum class BuiltinTypeId : uint8_t {  // keep the same with enum BuiltinType in ets_frontend
    FUNCTION = 1,
    RANGE_ERROR,
    ERROR,
    OBJECT,
    SYNTAX_ERROR,
    TYPE_ERROR,
    REFERENCE_ERROR,
    URI_ERROR,
    SYMBOL,
    EVAL_ERROR,
    NUMBER,
    PARSE_FLOAT,
    DATE,
    BOOLEAN,
    BIG_INT,
    PARSE_INT,
    WEAK_MAP,
    REG_EXP,
    SET,
    MAP,
    WEAK_REF,
    WEAK_SET,
    FINALIZATION_REGISTRY,
    ARRAY,
    UINT8_CLAMPED_ARRAY,
    UINT8_ARRAY,
    TYPED_ARRAY,
    INT8_ARRAY,
    UINT16_ARRAY,
    UINT32_ARRAY,
    INT16_ARRAY,
    INT32_ARRAY,
    FLOAT32_ARRAY,
    FLOAT64_ARRAY,
    BIG_INT64_ARRAY,
    BIG_UINT64_ARRAY,
    SHARED_ARRAY_BUFFER,
    DATA_VIEW,
    STRING,
    ARRAY_BUFFER,
    EVAL,
    IS_FINITE,
    ARK_PRIVATE,
    PRINT,
    DECODE_URI,
    DECODE_URI_COMPONENT,
    IS_NAN,
    ENCODE_URI,
    JS_NAN,
    GLOBAL_THIS,
    ENCODE_URI_COMPONENT,
    JS_INFINITY,
    MATH,
    JSON,
    ATOMICS,
    UNDEFINED,
    REFLECT,
    PROMISE,
    PROXY,
    GENERATOR_FUNCTION,
    INTL,
};

class TSModuleTable : public TaggedArray {
public:

    static constexpr int AMI_PATH_OFFSET = 1;
    static constexpr int SORT_ID_OFFSET = 2;
    static constexpr int TYPE_TABLE_OFFSET = 3;
    static constexpr int ELEMENTS_LENGTH = 3;
    static constexpr int NUMBER_OF_TABLES_INDEX = 0;
    static constexpr int INCREASE_CAPACITY_RATE = 2;
    static constexpr int DEFAULT_NUMBER_OF_TABLES = 4; // primitive table, builtins table, infer table and runtime table
    // first +1 means reserve a table from pandafile, second +1 menas the NUMBER_OF_TABLES_INDEX
    static constexpr int DEFAULT_TABLE_CAPACITY =  (DEFAULT_NUMBER_OF_TABLES + 1) * ELEMENTS_LENGTH + 1;
    static constexpr int PRIMITIVE_TABLE_ID = 0;
    static constexpr int BUILTINS_TABLE_ID = 1;
    static constexpr int INFER_TABLE_ID = 2;
    static constexpr int RUNTIME_TABLE_ID = 3;
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

    static void AddRuntimeTypeTable(JSThread *thread);

    static void FillLayoutTypes(JSThread *thread, JSHandle<TSObjLayoutInfo> &layOut,
        std::vector<JSHandle<JSTaggedValue>> &prop, std::vector<GlobalTSTypeRef> &propType);

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

    void InitTypeTables(const JSPandaFile *jsPandaFile, const CString &recordName);

    void AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    void LinkTSTypeTable(JSHandle<TSTypeTable> table);

    void LinkInRange(const JSPandaFile *jsPandaFile, JSHandle<TSModuleTable> moduleTable, int start, int end);

    void FillInterfaceMethodName(JSMutableHandle<JSTaggedValue> type);

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

    JSHandle<TSClassType> GetExtendClassType(JSHandle<TSClassType> classType) const;

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, const uint64_t key) const;

    uint32_t PUBLIC_API GetUnionTypeLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeVec);

    uint32_t PUBLIC_API GetFunctionTypeLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetOrCreateTSIteratorInstanceType(TSRuntimeType runtimeType, GlobalTSTypeRef elementGt);

    GlobalTSTypeRef PUBLIC_API GetIteratorInstanceElementGt(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetIteratorInstanceElementGt(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetGTRef());
        return GetIteratorInstanceElementGt(gt);
    }

    bool PUBLIC_API IsStaticFunc(GlobalTSTypeRef gt) const;
    
    GlobalTSTypeRef PUBLIC_API GetSuperPropType(GlobalTSTypeRef gt,
                                                JSHandle<EcmaString> propertyName,
                                                PropertyType propType) const;
    
    inline GlobalTSTypeRef PUBLIC_API GetSuperPropType(GlobalTSTypeRef gt,
                                                       JSTaggedValue propertyName,
                                                       PropertyType propType) const
    {
        return GetSuperPropType(gt, JSHandle<EcmaString>(vm_->GetJSThread(), propertyName), propType);
    }

    GlobalTSTypeRef PUBLIC_API GetSuperPropType(GlobalTSTypeRef gt,
                                                const uint64_t key,
                                                PropertyType propType) const;

    GlobalTSTypeRef PUBLIC_API GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetFuncThisGT(GlobalTSTypeRef gt) const;

    bool PUBLIC_API IsGetterSetterFunc(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetFuncReturnValueTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const;

    std::string PUBLIC_API GetFuncName(kungfu::GateType gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetArrayParameterTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(GlobalTSTypeRef gt) const;

    JSHandle<TSTypeTable> GetRuntimeTypeTable() const;

    void SetRuntimeTypeTable(JSHandle<TSTypeTable> inferTable);

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

    int GetHClassIndexByClassGT(const kungfu::GateType &gateType) const
    {
        if (!IsClassTypeKind(gateType)) {
            return -1;
        }
        GlobalTSTypeRef classGT = gateType.GetGTRef();
        auto it = classTypeIhcIndexMap_.find(classGT);
        if (it == classTypeIhcIndexMap_.end()) {
            return -1;
        } else {
            return it->second;
        }
    }

    bool IsUserDefinedClassTypeKind(const kungfu::GateType &gateType) const
    {
        if (!IsClassTypeKind(gateType)) {
            return false;
        }

        GlobalTSTypeRef gt = gateType.GetGTRef();
        uint32_t m = gt.GetModuleId();
        return m != TSModuleTable::BUILTINS_TABLE_ID;
    }

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    JSThread *GetThread() const
    {
        return thread_;
    }

#define IS_TSTYPEKIND_METHOD_LIST(V)                    \
    V(Primitive, TSTypeKind::PRIMITIVE)                 \
    V(Class, TSTypeKind::CLASS)                         \
    V(ClassInstance, TSTypeKind::CLASS_INSTANCE)        \
    V(Function, TSTypeKind::FUNCTION)                   \
    V(Union, TSTypeKind::UNION)                         \
    V(Array, TSTypeKind::ARRAY)                         \
    V(Object, TSTypeKind::OBJECT)                       \
    V(Import, TSTypeKind::IMPORT)                       \
    V(Interface, TSTypeKind::INTERFACE_KIND)            \
    V(IteratorInstance, TSTypeKind::ITERATOR_INSTANCE)  \

#define IS_TSTYPEKIND(NAME, TSTYPEKIND)                                                \
    bool inline PUBLIC_API Is##NAME##TypeKind(const kungfu::GateType &gateType) const  \
    {                                                                                  \
        GlobalTSTypeRef gt = gateType.GetGTRef();                                      \
        return GetTypeKind(gt) == (TSTYPEKIND);                                        \
    }

    IS_TSTYPEKIND_METHOD_LIST(IS_TSTYPEKIND)
#undef IS_TSTYPEKIND

    bool PUBLIC_API IsTypedArrayType(kungfu::GateType gateType) const;

    bool PUBLIC_API IsFloat32ArrayType(kungfu::GateType gateType) const;

    GlobalTSTypeRef ConvertPrimitiveToBuiltin(kungfu::GateType gateType)
    {
        const GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.Value());
        uint32_t l = gt.GetLocalId();
        switch (l) {
            case static_cast<uint32_t>(TSPrimitiveType::SYMBOL):
                return GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, static_cast<int>(BuiltinTypeId::SYMBOL));
            case static_cast<uint32_t>(TSPrimitiveType::INT):
            case static_cast<uint32_t>(TSPrimitiveType::DOUBLE):
            case static_cast<uint32_t>(TSPrimitiveType::NUMBER):
                return GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, static_cast<int>(BuiltinTypeId::NUMBER));
            case static_cast<uint32_t>(TSPrimitiveType::BOOLEAN):
                return GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, static_cast<int>(BuiltinTypeId::BOOLEAN));
            case static_cast<uint32_t>(TSPrimitiveType::STRING):
                return GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, static_cast<int>(BuiltinTypeId::STRING));
            default:
                return gt;
        }
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

    bool IsTSIterator(GlobalTSTypeRef gt) const
    {
        uint32_t m = gt.GetModuleId();
        uint32_t l = gt.GetLocalId();
        return (m == static_cast<uint32_t>(TSModuleTable::RUNTIME_TABLE_ID)) &&
               (l == static_cast<uint32_t>(TSRuntimeType::ITERATOR));
    }

    bool IsTSIteratorResult(GlobalTSTypeRef gt) const
    {
        uint32_t m = gt.GetModuleId();
        uint32_t l = gt.GetLocalId();
        return (m == static_cast<uint32_t>(TSModuleTable::RUNTIME_TABLE_ID)) &&
               (l == static_cast<uint32_t>(TSRuntimeType::ITERATOR_RESULT));
    }

    int PUBLIC_API GetHClassIndex(const kungfu::GateType &gateType);

    JSTaggedValue PUBLIC_API GetHClassFromCache(uint32_t index);

    // not consider [[prototype]] properties and accessor, -1: not find
    int PUBLIC_API GetPropertyOffset(JSTaggedValue hclass, JSTaggedValue key);

    enum class SnapshotInfoType : uint8_t {
        STRING = 0,
        METHOD,
        SKIPPED_METHOD,
        CLASS_LITERAL,
        OBJECT_LITERAL,
        ARRAY_LITERAL,
    };

    class SnapshotRecordInfo {
    public:
        void Iterate(const RootVisitor &v);

        void AddHClass(JSTaggedType hclass)
        {
            hclassCache_.emplace_back(hclass);
        }

        uint32_t GetHClassCacheSize() const
        {
            return hclassCache_.size();
        }

        CVector<JSTaggedType>& GetHClassCache()
        {
            return hclassCache_;
        }

        void AddRecordLiteralIndex(uint32_t index)
        {
            recordLiteralIndex_.emplace_back(index);
        }

        const std::vector<uint32_t>& GetRecordLiteralIndex()
        {
            return recordLiteralIndex_;
        }

        void AddMethodIndex(uint32_t index)
        {
            recordMethodIndex_.emplace_back(index);
        }

        const std::vector<uint32_t>& GetRecordMethodIndex()
        {
            return recordMethodIndex_;
        }

        void AddSkippedMethodID(uint32_t methodID)
        {
            if (skippedMethodIDs_.find(methodID) == skippedMethodIDs_.end()) {
                skippedMethodIDs_.insert(methodID);
            }
        }

        bool IsSkippedMethod(uint32_t methodID) const
        {
            if (skippedMethodIDs_.find(methodID) == skippedMethodIDs_.end()) {
                return false;
            }
            return true;
        }

        bool isProcessed(uint32_t constantPoolIndex)
        {
            if (recordConstantPoolIndex_.find(constantPoolIndex) == recordConstantPoolIndex_.end()) {
                recordConstantPoolIndex_.insert(constantPoolIndex);
                return false;
            }
            return true;
        }

    private:
        std::vector<uint32_t> recordLiteralIndex_ {};
        std::vector<uint32_t> recordMethodIndex_ {};
        std::set<uint32_t> skippedMethodIDs_ {};
        std::set<uint32_t> recordConstantPoolIndex_ {};

        // store hclass of each abc which produced from static type info
        CVector<JSTaggedType> hclassCache_ {};
    };

    void PUBLIC_API GenerateSnapshotConstantPool(JSTaggedValue constantPool);

    void PUBLIC_API AddIndexOrSkippedMethodID(SnapshotInfoType type, uint32_t index, const CString &recordName = "");

    bool PUBLIC_API IsSkippedMethod(uint32_t methodId) const
    {
        return snapshotRecordInfo_.IsSkippedMethod(methodId);
    }

    void PUBLIC_API ResolveSnapshotConstantPool(const std::map<uint32_t, uint32_t> &methodToEntryIndexMap);

    JSHandle<JSTaggedValue> PUBLIC_API GetSnapshotConstantPool() const
    {
        return JSHandle<JSTaggedValue>(uintptr_t(&snapshotConstantPool_));
    }

    bool PUBLIC_API IsBuiltin(kungfu::GateType funcType) const;

    bool PUBLIC_API IsBuiltinMath(kungfu::GateType funcType) const;

private:
    NO_COPY_SEMANTIC(TSManager);
    NO_MOVE_SEMANTIC(TSManager);

    void RecursivelyResolveTargetType(JSHandle<TSImportType> importType);

    void RecursivelyMergeClassField(JSHandle<TSClassType> classType, JSHandle<TSClassType> extendClassType);

    bool IsDuplicatedKey(JSHandle<TSObjLayoutInfo> extendLayout, JSTaggedValue key);

    GlobalTSTypeRef GetExportGTByName(JSHandle<EcmaString> target,
                                      JSHandle<TaggedArray> &exportTable) const;

    GlobalTSTypeRef PUBLIC_API AddTSTypeToInferTable(JSHandle<TSType> type) const;

    GlobalTSTypeRef FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const;

    GlobalTSTypeRef FindIteratorInstanceInInferTable(GlobalTSTypeRef kindGt, GlobalTSTypeRef elementGt) const;

    JSHandle<TSTypeTable> GetInferTypeTable() const;

    void SetInferTypeTable(JSHandle<TSTypeTable> inferTable) const;

    TSTypeKind PUBLIC_API GetTypeKind(const GlobalTSTypeRef &gt) const;

    std::string GetPrimitiveStr(const GlobalTSTypeRef &gt) const;

    void AddHClassInCompilePhase(GlobalTSTypeRef gt, JSTaggedValue hclass, uint32_t constantPoolLen)
    {
        CVector<JSTaggedType> &hclassCache_ = snapshotRecordInfo_.GetHClassCache();
        hclassCache_.emplace_back(hclass.GetRawData());
        classTypeIhcIndexMap_[gt] = constantPoolLen + hclassCache_.size() - 1;
    }

    void CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                            JSHandle<ConstantPool> snapshotConstantPool);

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    // record the mapping relationship between classType and instance hclass index in the constant pool
    std::map<GlobalTSTypeRef, uint32_t> classTypeIhcIndexMap_ {};
    bool assertTypes_ {false};
    bool printAnyTypes_ {false};
    friend class EcmaVM;

    // For snapshot
    JSTaggedValue snapshotConstantPool_ {JSTaggedValue::Hole()};
    SnapshotRecordInfo snapshotRecordInfo_ {};

    std::map<std::pair<const JSPandaFile *, panda_file::File::EntityId>, GlobalTSTypeRef> literalOffsetGTMap_ {};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_MANAGER_H
