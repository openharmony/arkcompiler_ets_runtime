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

#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/ts_types/global_ts_type_ref.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/compilation_driver.h"

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

/* Since AOT allows loading lib_ark_builtins.d.ts optionally, the localId of the GlobalTSTypeRef
 * (abbreviated as GT) of one builtin object will be different in different cases.
 *
 * In case where AOT does not load lib_ark_builtins.d.ts, builtin objects will be assigned localIds
 * according to the following enum order. For example, the GT of FUNCTION will be (1, 21), where 1
 * is the index of builtin TSTypeTable. Note that in this case, it is prohibited to get TSType from
 * builtin TSTypeTable.
 *
 * In case where AOT has loaded lib_ark_builtins.d.ts, builtin objects will be assigned localIds in
 * the order in which they appear in bytecodes. To identify types of builtin objects, the following
 * enum is required as a parameter to the TSManager::GetBuiltinOffset to help to get the GT of some
 * builtin object.
 */
enum class BuiltinTypeId : uint8_t {  // keep the same with enum BuiltinType in ets_frontend
    NUM_INDEX_IN_SUMMARY = 0,
    BUILTIN_OFFSET = 20,
    FUNCTION,
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
    NUM_OF_BUILTIN_TYPES = INTL - BUILTIN_OFFSET,
    TYPED_ARRAY_FIRST = UINT8_CLAMPED_ARRAY,
    TYPED_ARRAY_LAST = BIG_UINT64_ARRAY,
};

struct LocalModuleInfo {
    const JSPandaFile *jsPandaFile {nullptr}; // there may be serval merged abc files.
    const CString recordName {""}; // distinguish different files which are all merged to a abc file.
    uint32_t index {0}; // bytecode "ldlocalmodulevar index", importVar index is unique in the same recordName.
    bool operator < (const LocalModuleInfo &localModuleInfo) const
    {
        return index < localModuleInfo.index;
    }
};

class TSModuleTable : public TaggedArray {
public:
    // Each TSTypeTable occupies three positions
    static constexpr int ELEMENTS_LENGTH = 3;
    static constexpr int MODULE_REQUEST_OFFSET = 1;
    static constexpr int SORT_ID_OFFSET = 2;
    static constexpr int TYPE_TABLE_OFFSET = 3;
    // Reserve a position which is used to store the number of TSTypeTables and a TSTypeTable storage space
    static constexpr int INITIAL_CAPACITY = ELEMENTS_LENGTH + 1;
    static constexpr int NUMBER_OF_TABLES_INDEX = 0;
    static constexpr int INCREASE_CAPACITY_RATE = 2;
    // primitive table, builtins table, infer table and runtime table
    static constexpr int DEFAULT_NUMBER_OF_TABLES = 4;
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

    JSHandle<EcmaString> GetModuleRequestByModuleId(JSThread *thread, int entry) const;

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

    static int GetModuleRequestOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + MODULE_REQUEST_OFFSET;
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

    JSHandle<TSTypeTable> GetTSTypeTable(int entry) const;

    void SetTSTypeTable(const JSHandle<TSTypeTable> &table, int tableId) const;

    void GenerateBuiltinSummary();

    inline uint32_t GetBuiltinOffset(uint32_t index) const
    {
        if (index == static_cast<uint32_t>(BuiltinTypeId::NUM_INDEX_IN_SUMMARY)) {
            return builtinOffsets_[index];
        }
        return builtinOffsets_[index - static_cast<uint32_t>(BuiltinTypeId::BUILTIN_OFFSET)];
    }

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

    void PUBLIC_API SetFuncMethodOffset(GlobalTSTypeRef gt, uint32_t methodOffset);

    uint32_t PUBLIC_API GetFuncMethodOffset(GlobalTSTypeRef gt) const;

    bool PUBLIC_API IsGetterSetterFunc(GlobalTSTypeRef gt) const;

    bool IsAbstractMethod(GlobalTSTypeRef gt) const;

    bool IsMethodSignature(GlobalTSTypeRef gt) const;

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

    void GenerateTSHClass(JSHandle<TSClassType> classType);

    void GenerateTSHClasses();

    JSHandle<JSTaggedValue> GetTSType(const GlobalTSTypeRef &gt) const;

    std::string PUBLIC_API GetTypeStr(kungfu::GateType gateType) const;

    int PUBLIC_API GetHClassIndexByInstanceGateType(const kungfu::GateType &gateType);

    int PUBLIC_API GetHClassIndexByClassGateType(const kungfu::GateType &gateType);

    JSTaggedValue PUBLIC_API GetHClassFromCache(uint32_t index);

    inline bool IsUserDefinedClassTypeKind(const kungfu::GateType &gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return IsUserDefinedClassTypeKind(gt);
    }

    inline bool IsUserDefinedClassTypeKind(const GlobalTSTypeRef &gt) const
    {
        uint32_t m = gt.GetModuleId();
        return (IsClassTypeKind(gt)) &&
               (m != TSModuleTable::BUILTINS_TABLE_ID);
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
    inline bool PUBLIC_API Is##NAME##TypeKind(const kungfu::GateType &gateType) const  \
    {                                                                                  \
        GlobalTSTypeRef gt = gateType.GetGTRef();                                      \
        return GetTypeKind(gt) == (TSTYPEKIND);                                        \
    }                                                                                  \
                                                                                       \
    inline bool PUBLIC_API Is##NAME##TypeKind(const GlobalTSTypeRef &gt) const         \
    {                                                                                  \
        return GetTypeKind(gt) == (TSTYPEKIND);                                        \
    }

    IS_TSTYPEKIND_METHOD_LIST(IS_TSTYPEKIND)
#undef IS_TSTYPEKIND

    bool PUBLIC_API IsBuiltinArrayType(kungfu::GateType gateType) const;

    bool PUBLIC_API IsTypedArrayType(kungfu::GateType gateType) const;

    bool PUBLIC_API IsFloat32ArrayType(kungfu::GateType gateType) const;

    inline void AddElementToLiteralOffsetGTMap(const JSPandaFile *jsPandaFile, uint32_t offset,
                                               const CString &recordName, GlobalTSTypeRef gt,
                                               bool isImportType = false)
    {
        auto key = std::make_pair(jsPandaFile, offset);
        literalOffsetGTMap_.emplace(key, gt);
        if (!isImportType) {
            auto value = std::make_pair(recordName, offset);
            gtLiteralOffsetMap_.emplace(gt, value);
        }
    }

    inline bool HasCreatedGT(const JSPandaFile *jsPandaFile, uint32_t offset) const
    {
        auto key = std::make_pair(jsPandaFile, offset);
        return literalOffsetGTMap_.find(key) != literalOffsetGTMap_.end();
    }

    inline GlobalTSTypeRef GetGTFromOffset(const JSPandaFile *jsPandaFile, uint32_t offset) const
    {
        auto key = std::make_pair(jsPandaFile, offset);
        return literalOffsetGTMap_.at(key);
    }

    inline bool HasOffsetFromGT(GlobalTSTypeRef gt) const
    {
        return gtLiteralOffsetMap_.find(gt) != gtLiteralOffsetMap_.end();
    }

    inline std::pair<CString, uint32_t> GetOffsetFromGt(GlobalTSTypeRef gt) const
    {
        return gtLiteralOffsetMap_.at(gt);
    }

    inline void AddTypeToLocalModuleVarGtMap(const JSPandaFile *jsPandaFile, const CString &recordName,
                                             uint32_t index, GlobalTSTypeRef gt)
    {
        LocalModuleInfo key = {jsPandaFile, recordName, index};
        if (localModuleVarGtMap_.find(key) == localModuleVarGtMap_.end()) {
            localModuleVarGtMap_.emplace(key, gt);
        } else {
            localModuleVarGtMap_[key] = gt;
        }
    }

    inline bool HasExportGT(const JSPandaFile *jsPandaFile, const CString &recordName,
                            uint32_t index)
    {
        LocalModuleInfo key = {jsPandaFile, recordName, index};
        return localModuleVarGtMap_.find(key) != localModuleVarGtMap_.end();
    }

    inline GlobalTSTypeRef GetGTFromModuleMap(const JSPandaFile *jsPandaFile, const CString &recordName,
                                              uint32_t index)
    {
        LocalModuleInfo key = {jsPandaFile, recordName, index};
        return localModuleVarGtMap_.at(key);
    }

    bool IsTSIterator(GlobalTSTypeRef gt) const
    {
        uint32_t m = gt.GetModuleId();
        uint32_t l = gt.GetLocalId();
        return (m == TSModuleTable::RUNTIME_TABLE_ID) && (l == static_cast<int>(TSRuntimeType::ITERATOR));
    }

    bool IsTSIteratorResult(GlobalTSTypeRef gt) const
    {
        uint32_t m = gt.GetModuleId();
        uint32_t l = gt.GetLocalId();
        return (m == TSModuleTable::RUNTIME_TABLE_ID) && (l == static_cast<int>(TSRuntimeType::ITERATOR_RESULT));
    }

    // not consider [[prototype]] properties and accessor, -1: not find
    int PUBLIC_API GetPropertyOffset(JSTaggedValue hclass, JSTaggedValue key);

    void PUBLIC_API SetCurConstantPool(const JSPandaFile *jsPandaFile, uint32_t methodOffset);

    JSHandle<JSTaggedValue> PUBLIC_API GetConstantPool() const
    {
        return JSHandle<JSTaggedValue>(uintptr_t(&curCP_));
    }

    bool PUBLIC_API IsBuiltin(kungfu::GateType funcType) const;

    bool PUBLIC_API IsBuiltinMath(kungfu::GateType funcType) const;

    void RecursivelyMergeClassField(JSHandle<TSClassType> classType);

    inline const JSPandaFile *GetBuiltinPandaFile() const
    {
        return builtinPandaFile_;
    }

    inline const CString &GetBuiltinRecordName() const
    {
        return builtinsRecordName_;
    }

    class IHClassData {
    public:
        IHClassData(JSTaggedType ihc) : ihc_(ihc) {}

        void Iterate(const RootVisitor &v)
        {
            v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&ihc_)));
        }

        std::unordered_map<int32_t, uint32_t>& GetCPIndexMap()
        {
            return cpIndexMap_;
        }

        JSTaggedType GetIHC() const
        {
            return ihc_;
        }

    private:
        JSTaggedType ihc_ {0};
        std::unordered_map<int32_t, uint32_t> cpIndexMap_ {};
    };

    // for snapshot
    class SnapshotData {
    public:
        enum RecordType {
            METHOD = 0,
            LITERAL,

            RECORD_TYPE_NUM,
            RECORD_TYPE_FIRST = METHOD,
            RECORD_TYPE_LAST = LITERAL,
        };

        static constexpr uint8_t SNAPSHOT_CP_LIST_ITEM_SIZE = 2;

        using RecordData = std::vector<std::pair<uint32_t, uint32_t>>;

        SnapshotData() : recordInfo_(RecordType::RECORD_TYPE_NUM, RecordData{}) {}

        void Iterate(const RootVisitor &v)
        {
            v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&snapshotCPList_)));
        }

        void SetSnapshotCPList(JSTaggedValue snapshotCPList)
        {
            snapshotCPList_ = snapshotCPList;
        }

        JSTaggedValue GetSnapshotCPList() const
        {
            return snapshotCPList_;
        }

        CVector<JSTaggedType>& GetSnapshotHCVector(int32_t cpID)
        {
            return snapshotHCs_[cpID];
        }

        void AddIndexInfoToRecordInfo(RecordType type, std::pair<uint32_t, uint32_t> indexInfo)
        {
            ASSERT(RECORD_TYPE_FIRST <= type && type <= RECORD_TYPE_LAST);
            recordInfo_[type].emplace_back(indexInfo);
        }

        const RecordData& GetRecordInfo(RecordType type)
        {
            ASSERT(RECORD_TYPE_FIRST <= type && type <= RECORD_TYPE_LAST);
            return recordInfo_[type];
        }

    private:
        JSTaggedValue snapshotCPList_ {JSTaggedValue::Hole()};

        // key: constantpoolnum,  value: store hclass which produced from static type info
        CMap<int32_t, CVector<JSTaggedType>> snapshotHCs_ {};

        // used to record the data that needs to be modified into the aot code entry index
        std::vector<RecordData> recordInfo_ {};
    };

    void SetCompilationDriver(kungfu::CompilationDriver *cmpDriver)
    {
        cmpDriver_ = cmpDriver;
    }

    kungfu::CompilationDriver *GetCompilationDriver() const
    {
        return cmpDriver_;
    }

    JSTaggedValue PUBLIC_API GetSnapshotCPList() const
    {
        return snapshotData_.GetSnapshotCPList();
    }

    void PUBLIC_API ProcessSnapshotConstantPool(kungfu::BytecodeInfoCollector *bcInfoCollector);

    void PUBLIC_API ResolveSnapshotConstantPool(const std::map<uint32_t, uint32_t> &methodToEntryIndexMap);

    void AddElementToClassNameMap(const JSPandaFile *jsPandaFile, uint32_t offset, std::string className)
    {
        literalOffsetClassNameMap_.emplace(std::make_pair(jsPandaFile, offset), className);
    }

    const std::string GetClassNameByOffset(const JSPandaFile *jsPandaFile, uint32_t typeId) const
    {
        std::pair<const JSPandaFile *, uint32_t> pair = std::make_pair(jsPandaFile, typeId);
        std::string name = "";
        if (literalOffsetClassNameMap_.find(pair) != literalOffsetClassNameMap_.end()) {
            name = literalOffsetClassNameMap_.at(pair);
        }
        return name;
    }

    inline void CollectTypeOffsets(GlobalTSTypeRef classGT)
    {
        if (IsClassTypeKind(classGT)) {
            collectedTypeOffsets_.insert(classGT);
        }
    }

    void PrintNumOfTypes() const;

private:
    NO_COPY_SEMANTIC(TSManager);
    NO_MOVE_SEMANTIC(TSManager);

    bool IsDuplicatedKey(JSHandle<TSObjLayoutInfo> extendLayout, JSTaggedValue key);

    GlobalTSTypeRef AddTSTypeToTypeTable(const JSHandle<TSType> &type, int tableId) const;

    GlobalTSTypeRef FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const;

    GlobalTSTypeRef FindIteratorInstanceInInferTable(GlobalTSTypeRef kindGt, GlobalTSTypeRef elementGt) const;

    TSTypeKind PUBLIC_API GetTypeKind(const GlobalTSTypeRef &gt) const;

    std::string GetClassTypeStr(GlobalTSTypeRef gt) const;

    std::string GetClassInstanceTypeStr(GlobalTSTypeRef gt) const;

    std::string GetFunctionTypeStr(GlobalTSTypeRef gt) const;

    std::string GetArrayTypeStr(GlobalTSTypeRef gt) const;

    std::string GetPrimitiveStr(const GlobalTSTypeRef &gt) const;

    int GetHClassIndex(GlobalTSTypeRef classGT);

    uint32_t RecordIhcToVecAndIndexMap(IHClassData &ihcData);

    uint32_t GetBuiltinIndex(GlobalTSTypeRef builtinGT) const;

    std::string GetBuiltinsName(uint32_t index) const;

    void CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                            JSHandle<ConstantPool> snapshotConstantPool,
                            kungfu::BytecodeInfoCollector *bcInfoCollector);

    inline void SetBuiltinPandaFile(JSPandaFile *jsPandaFile)
    {
        builtinPandaFile_ = jsPandaFile;
    }

    inline void SetBuiltinRecordName(CString &builtinsRecordName)
    {
        builtinsRecordName_ = builtinsRecordName;
    }

    // for snapshot
    int32_t GetOldConstantPoolIDByMethodOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset);

    void GenerateSnapshotConstantPoolList(std::map<int32_t, uint32_t> &cpListIndexMap,
                                          const CMap<int32_t, JSTaggedValue> &oldCPValues);

    void FillSnapshotConstantPoolList(const std::map<int32_t, uint32_t> &cpListIndexMap,
                                      kungfu::BytecodeInfoCollector *bcInfoCollector);

    void AddHClassToSnapshotConstantPoolList(const std::map<int32_t, uint32_t> &cpListIndexMap,
                                             kungfu::BytecodeInfoCollector *bcInfoCollector);

    JSHandle<ConstantPool> GetSnapshotConstantPool(uint32_t cpListIndex);

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    std::map<GlobalTSTypeRef, IHClassData> gtIhcMap_ {};
    bool assertTypes_ {false};
    bool printAnyTypes_ {false};

    // when the passmanager iterates each method, the curCP_ and curCPID_ should be updated
    // so that subsequent passes (type_infer, ts_type_lowering) can obtain the correct constpool.
    JSTaggedValue curCP_ {JSTaggedValue::Hole()};
    int32_t curCPID_ {0};

    // for snapshot
    SnapshotData snapshotData_ {};

    std::map<std::pair<const JSPandaFile *, uint32_t>, GlobalTSTypeRef> literalOffsetGTMap_ {};
    std::map<GlobalTSTypeRef, std::pair<CString, uint32_t>> gtLiteralOffsetMap_ {};
    std::vector<uint32_t> builtinOffsets_ {};
    JSPandaFile *builtinPandaFile_ {nullptr};
    CString builtinsRecordName_ {""};
    std::map<LocalModuleInfo, GlobalTSTypeRef> localModuleVarGtMap_{};
    kungfu::CompilationDriver *cmpDriver_ {nullptr};
    std::set<GlobalTSTypeRef> collectedTypeOffsets_ {};  // use for storing types that need to generate hclasses

    friend class EcmaVM;

    std::map<std::pair<const JSPandaFile *, uint32_t>, std::string> literalOffsetClassNameMap_ {};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_MANAGER_H
