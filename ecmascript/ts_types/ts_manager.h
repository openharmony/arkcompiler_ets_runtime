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

#include "ecmascript/compiler/aot_snapshot/aot_snapshot.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/compilation_driver.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/ts_types/builtin_type_id.h"
#include "ecmascript/ts_types/global_ts_type_ref.h"
#include "ecmascript/ts_types/global_type_info.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"

namespace panda::ecmascript {
using ProfileType = pgo::ProfileType;
using SnapshotGlobalData = kungfu::SnapshotGlobalData;
enum class PropertyType : uint8_t {
    NORMAL = 0,
    STATIC,
    OTHERS,
};

struct ModuleInfo {
    const JSPandaFile *jsPandaFile {nullptr}; // there may be serval merged abc files.
    const CString recordName {""}; // distinguish different files which are all merged to one abc file.
    uint32_t index {0}; // bytecode "stmoudlevar index", exportVar index is unique in the same record.
    bool operator < (const ModuleInfo &moduleInfo) const
    {
        if (index < moduleInfo.index) {
            return true;
        }
        if (index == moduleInfo.index && recordName < moduleInfo.recordName) {
            return true;
        }
        if (index == moduleInfo.index && recordName == moduleInfo.recordName &&
            jsPandaFile < moduleInfo.jsPandaFile) {
            return true;
        }
        return false;
    }
};

class TSModuleTable : public TaggedArray {
public:
    // Each TSTypeTable occupies three positions
    static constexpr int ELEMENTS_LENGTH = 4;
    static constexpr int MODULE_REQUEST_OFFSET = 1;
    static constexpr int SORT_ID_OFFSET = 2;
    static constexpr int TYPE_TABLE_OFFSET = 3;
    static constexpr int ABC_ID_OFFSET = 4;
    // Reserve a position which is used to store the number of TSTypeTables and a TSTypeTable storage space
    static constexpr int INITIAL_CAPACITY = ELEMENTS_LENGTH + 1;
    static constexpr int NUMBER_OF_TABLES_INDEX = 0;
    static constexpr int INCREASE_CAPACITY_RATE = 2;
    static constexpr int NOT_FOUND = -1;

    static TSModuleTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSModuleTable *>(object);
    }

    JSHandle<EcmaString> GetModuleRequestByModuleId(JSThread *thread, int entry) const;

    JSHandle<EcmaString> GetAbcRequestByModuleId(JSThread *thread, int entry) const;

    int GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath, JSHandle<EcmaString> abcPath) const;

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

    static uint32_t GetAbcRequestOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + ABC_ID_OFFSET;
    }
};

class PUBLIC_API TSManager {
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

    inline GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, JSTaggedValue propertyName) const
    {
        return GetPropType(gt, JSHandle<JSTaggedValue>(thread_, propertyName));
    }

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, const uint64_t key) const;

    GlobalTSTypeRef PUBLIC_API GetIndexSignType(GlobalTSTypeRef objType, kungfu::GateType indexType) const;

    inline GlobalTSTypeRef PUBLIC_API CreateClassInstanceType(kungfu::GateType gateType)
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return CreateClassInstanceType(gt);
    }

    GlobalTSTypeRef PUBLIC_API CreateClassInstanceType(GlobalTSTypeRef gt);

    inline GlobalTSTypeRef PUBLIC_API GetClassType(kungfu::GateType gateType)
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetClassType(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetClassType(GlobalTSTypeRef classInstanceGT) const;

    JSHandle<TSClassType> GetExtendedClassType(JSHandle<TSClassType> classType) const;

    TSClassType* GetExtendedClassType(const TSClassType *classType) const;

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

    bool PUBLIC_API IsHotnessFunc(GlobalTSTypeRef gt) const;

    void PUBLIC_API SetHotnessFunc(GlobalTSTypeRef gt, bool isHotness) const;

    bool PUBLIC_API GetSuperGateType(kungfu::GateType &gateType) const;

    GlobalTSTypeRef PUBLIC_API GetSuperPropType(GlobalTSTypeRef gt,
                                                JSHandle<JSTaggedValue> propertyName,
                                                PropertyType propType) const;

    inline GlobalTSTypeRef PUBLIC_API GetSuperPropType(GlobalTSTypeRef gt,
                                                       JSTaggedValue propertyName,
                                                       PropertyType propType) const
    {
        return GetSuperPropType(gt, JSHandle<JSTaggedValue>(vm_->GetJSThread(), propertyName),
                                propType);
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
    bool CanFastCall(GlobalTSTypeRef gt) const;
    bool IsNoGC(GlobalTSTypeRef gt) const;
    bool MethodOffsetIsVaild(GlobalTSTypeRef gt) const;
    bool FastCallFlagIsVaild(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return GetFuncReturnValueTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const;

    std::string PUBLIC_API GetFuncName(kungfu::GateType gt) const;

    int PUBLIC_API GetMethodIndex(GlobalTSTypeRef gt) const;

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

    double PUBLIC_API GetTypeThreshold() const
    {
        return typeThreshold_;
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

    void AddInstanceTSHClass(GlobalTSTypeRef gt, JSHandle<JSHClass> &ihclass);

    void AddConstructorTSHClass(GlobalTSTypeRef gt, JSHandle<JSHClass> &constructorHClass);

    JSTaggedValue GetInstanceTSHClass(const JSHandle<TSClassType> &classType) const;

    bool HasTSHClass(const JSHandle<TSClassType> &classType) const;

    bool HasTSHClass(const TSClassType *classType) const;

    JSHandle<JSTaggedValue> GetTSType(const GlobalTSTypeRef &gt) const;

    std::string PUBLIC_API GetTypeStr(kungfu::GateType gateType) const;

    int PUBLIC_API GetHClassIndexByObjectType(const kungfu::GateType &gateType);

    int PUBLIC_API GetHClassIndexByInstanceGateType(const kungfu::GateType &gateType);

    int PUBLIC_API GetConstructorHClassIndexByClassGateType(const kungfu::GateType &gateType);

    int PUBLIC_API GetHClassIndexByClassGateType(const kungfu::GateType &gateType);

    JSTaggedValue GetTSHClass(const kungfu::GateType &gateType) const;

    JSTaggedValue PUBLIC_API GetAOTHClassInfoByIndex(uint32_t index);

    GlobalTSTypeRef PUBLIC_API CreateArrayType();

    GlobalTSTypeRef PUBLIC_API CreateNamespaceType();

    bool AddNamespacePropType(kungfu::GateType objType, JSTaggedValue name, kungfu::GateType valueType);

    inline bool IsUserDefinedClassTypeKind(const kungfu::GateType &gateType) const
    {
        GlobalTSTypeRef gt = gateType.GetGTRef();
        return IsUserDefinedClassTypeKind(gt);
    }

    inline bool IsUserDefinedClassTypeKind(const GlobalTSTypeRef &gt) const
    {
        return IsClassTypeKind(gt) && (!gt.IsBuiltinModule());
    }

    inline void StoreNamespaceType(const uint32_t methodOffset, const kungfu::GateType type)
    {
        methodOffsetToType_.insert(std::make_pair(methodOffset, type));
    }

    inline kungfu::GateType GetNamespaceObjType(const uint32_t methodOffset) const
    {
        ASSERT(HasInferredNamespaceType(methodOffset));
        return methodOffsetToType_.at(methodOffset);
    }

    inline bool HasInferredNamespaceType(const uint32_t methodOffset) const
    {
        return methodOffsetToType_.find(methodOffset) != methodOffsetToType_.end();
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
    V(Interface, TSTypeKind::INTERFACE)                 \
    V(IteratorInstance, TSTypeKind::ITERATOR_INSTANCE)  \
    V(Namespace, TSTypeKind::NAMESPACE)

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

    bool PUBLIC_API IsBuiltinClassType(BuiltinTypeId id, kungfu::GateType gateType) const;

    bool PUBLIC_API IsBuiltinInstanceType(BuiltinTypeId id, kungfu::GateType gateType) const;

    bool PUBLIC_API IsTypedArrayType(kungfu::GateType gateType) const;

    bool PUBLIC_API IsValidTypedArrayType(kungfu::GateType gateType) const;

    inline bool PUBLIC_API IsBuiltinObjectType(kungfu::GateType gateType) const
    {
        return gateType.GetGTRef().IsBuiltinModule() && IsClassTypeKind(gateType);
    }

    bool PUBLIC_API IsIntTypedArrayType(kungfu::GateType gateType) const;

    bool PUBLIC_API IsDoubleTypedArrayType(kungfu::GateType gateType) const;

    BuiltinTypeId PUBLIC_API GetTypedArrayBuiltinId(kungfu::GateType gateType) const;

    static const std::vector<BuiltinTypeId> &GetValidTypedArrayIds();

    inline void AddElementToIdGTMap(const GlobalTypeID &id, GlobalTSTypeRef gt,
                                    const CString &recordName = "", bool isImportType = false)
    {
        auto it = idGTMap_.find(id);
        if (it != idGTMap_.end()) {
            it->second = gt;
        } else {
            idGTMap_.emplace(id, gt);
        }
        if (!isImportType && !id.IsPGOType()) {
            auto value = std::make_tuple(id.GetJSPandaFile()->GetNormalizedFileDesc(), recordName, id.GetTypeId());
            gtLiteralOffsetMap_.emplace(gt, value);
        }
    }

    inline bool HasCreatedGT(const GlobalTypeID &id) const
    {
        return idGTMap_.find(id) != idGTMap_.end();
    }

    inline GlobalTSTypeRef GetGTByGlobalTypeID(const GlobalTypeID &id) const
    {
        return idGTMap_.at(id);
    }

    inline bool HasOffsetFromGT(GlobalTSTypeRef gt) const
    {
        return gtLiteralOffsetMap_.find(gt) != gtLiteralOffsetMap_.end();
    }

    inline std::tuple<CString, CString, uint32_t> GetOffsetFromGt(GlobalTSTypeRef gt) const
    {
        return gtLiteralOffsetMap_.at(gt);
    }

    inline void AddTypeToModuleVarGtMap(const JSPandaFile *jsPandaFile, const CString &recordName,
                                        uint32_t index, GlobalTSTypeRef gt)
    {
        ModuleInfo key = {jsPandaFile, recordName, index};
        if (moduleVarGtMap_.find(key) == moduleVarGtMap_.end()) {
            moduleVarGtMap_.emplace(key, gt);
        } else {
            moduleVarGtMap_[key] = gt;
        }
    }

    inline bool HasExportGT(const JSPandaFile *jsPandaFile, const CString &recordName,
                            uint32_t index) const
    {
        ModuleInfo key = {jsPandaFile, recordName, index};
        return moduleVarGtMap_.find(key) != moduleVarGtMap_.end();
    }

    inline GlobalTSTypeRef GetGTFromModuleMap(const JSPandaFile *jsPandaFile, const CString &recordName,
                                              uint32_t index) const
    {
        ModuleInfo key = {jsPandaFile, recordName, index};
        return moduleVarGtMap_.at(key);
    }

    inline void AddResolvedExportTable(const JSPandaFile *jsPandaFile, const CString &recordName,
                                       JSTaggedValue exportTable)
    {
        auto key = std::make_pair(jsPandaFile, recordName);
        if (resolvedExportTable_.find(key) == resolvedExportTable_.end()) {
            resolvedExportTable_.emplace(key, exportTable);
        } else {
            resolvedExportTable_[key] = exportTable;
        }
    }

    inline bool HasResolvedExportTable(const JSPandaFile *jsPandaFile, const CString &recordName) const
    {
        auto key = std::make_pair(jsPandaFile, recordName);
        return resolvedExportTable_.find(key) != resolvedExportTable_.end();
    }

    inline JSTaggedValue GetResolvedExportTable(const JSPandaFile *jsPandaFile, const CString &recordName) const
    {
        auto key = std::make_pair(jsPandaFile, recordName);
        return resolvedExportTable_.at(key);
    }

    bool IsTSIterator(GlobalTSTypeRef gt) const
    {
        uint32_t l = gt.GetLocalId();
        return gt.IsRuntimeModule() && (l == static_cast<uint32_t>(TSRuntimeType::ITERATOR));
    }

    bool IsTSIteratorResult(GlobalTSTypeRef gt) const
    {
        uint32_t l = gt.GetLocalId();
        return gt.IsRuntimeModule() && (l == static_cast<uint32_t>(TSRuntimeType::ITERATOR_RESULT));
    }

    void PUBLIC_API SetCurConstantPool(const JSPandaFile *jsPandaFile, uint32_t methodOffset);

    int32_t PUBLIC_API GetConstantPoolIDByMethodOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset);

    uint32_t GetConstantPoolId(uint32_t methodId) const;
    JSTaggedValue GetConstantPool(uint32_t methodId) const;

    JSHandle<JSTaggedValue> PUBLIC_API GetConstantPool() const
    {
        return JSHandle<JSTaggedValue>(uintptr_t(&curCP_));
    }

    JSTaggedValue PUBLIC_API GetStringFromConstantPool(uint32_t methodId, const uint16_t stringId) const;

    inline std::string PUBLIC_API GetStdStringFromConstantPool(uint32_t methodId, const uint16_t stringId) const
    {
        JSTaggedValue str = GetStringFromConstantPool(methodId, stringId);
        return EcmaStringAccessor(str).ToStdString(StringConvertedUsage::LOGICOPERATION);
    }

    bool PUBLIC_API IsBuiltinObjectMethod(BuiltinTypeId id, kungfu::GateType funcType) const;
    bool PUBLIC_API IsBuiltinConstructor(BuiltinTypeId id, GlobalTSTypeRef ctorType) const;

    inline const JSPandaFile *GetBuiltinPandaFile() const
    {
        return builtinPandaFile_;
    }

    inline const CString &GetBuiltinRecordName() const
    {
        return builtinsRecordName_;
    }

    inline kungfu::BytecodeInfoCollector *GetBytecodeInfoCollector() const
    {
        return bcInfoCollector_;
    }

    inline void SetBytecodeInfoCollector(kungfu::BytecodeInfoCollector *bcInfoCollector)
    {
        bcInfoCollector_ = bcInfoCollector;
    }

    class IHClassData {
    public:
        explicit IHClassData(JSTaggedType ihc) : ihc_(ihc) {}

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

    void SetCompilationDriver(kungfu::CompilationDriver *cmpDriver)
    {
        cmpDriver_ = cmpDriver;
    }

    kungfu::CompilationDriver *GetCompilationDriver() const
    {
        return cmpDriver_;
    }

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

    inline void CollectGT(GlobalTSTypeRef gt)
    {
        if (IsClassTypeKind(gt)) {
            collectedGT_.insert(gt);
        }
    }

    inline std::set<GlobalTSTypeRef>& GetCollectedGT()
    {
        return collectedGT_;
    }

    inline void InsertLiteralGTMap(TypeLocation &loc, GlobalTSTypeRef gt)
    {
        literalGTMap_[loc] = gt;
    }

    inline GlobalTSTypeRef GetLiteralGT(TypeLocation &loc)
    {
        auto it = literalGTMap_.find(loc);
        if (it != literalGTMap_.end()) {
            return it->second;
        }
        return GlobalTSTypeRef::Default();
    }

    inline void InsertPtToGtMap(ProfileType pgoType, const kungfu::GateType &gateType)
    {
        ptToGtMap_.emplace(pgoType, gateType);
    }

    inline const kungfu::GateType GetGateTypeByPt(ProfileType pgoType)
    {
        if (pgoType.IsBuiltinsType()) {
            return GetBuiltinsGateTypeByPt(pgoType);
        }
        auto it = ptToGtMap_.find(pgoType);
        if (it != ptToGtMap_.end()) {
            return it->second;
        }
        return kungfu::GateType::AnyType();
    }

    void PrintNumOfTypes() const;

    void PrintTypeInfo(const JSPandaFile *jsPandaFile) const;

    kungfu::GateType TryNarrowUnionType(kungfu::GateType gateType);

    JSHandle<TaggedArray> GetExportTableFromLiteral(const JSPandaFile *jsPandaFile, const CString &recordName);

    int GetHClassIndex(GlobalTSTypeRef classGT, bool isConstructor = false);

    uint32_t GetPGOGTCountByRecordName(const CString &recordName) const
    {
        if (bcInfoCollector_ == nullptr) {
            return 0;
        }
        const kungfu::PGOBCInfo *bcInfo = bcInfoCollector_->GetPGOBCInfo();
        return bcInfo->GetPGOExtendGTCount(recordName);
    }

    inline std::string GetBuiltinsName(GlobalTSTypeRef builtinGT) const
    {
        uint32_t index = GetBuiltinIndex(builtinGT);
        return GetBuiltinsName(index);
    }

    TSTypeKind PUBLIC_API GetTypeKind(const GlobalTSTypeRef &gt) const;

#define TSTYPETABLE_ACCESSOR_LIST(V)       \
    V(Builtin, ModuleTableIdx::BUILTIN)    \
    V(Inferred, ModuleTableIdx::INFERRED)  \
    V(Runtime, ModuleTableIdx::RUNTIME)    \
    V(Generics, ModuleTableIdx::GENERICS)

#define TSTYPETABLE_ACCESSOR(NAME, MODULEID)                                             \
    inline GlobalTSTypeRef AddTSTypeTo##NAME##Table(const JSHandle<TSType> &type) const  \
    {                                                                                    \
        return AddTSTypeToTypeTable(type, static_cast<uint32_t>(MODULEID));              \
    }                                                                                    \
                                                                                         \
    inline JSHandle<TSTypeTable> Get##NAME##Table() const                                \
    {                                                                                    \
        return GetTSTypeTable(static_cast<uint32_t>(MODULEID));                          \
    }                                                                                    \
                                                                                         \
    inline void Set##NAME##Table(const JSHandle<TSTypeTable> &table)                     \
    {                                                                                    \
        SetTSTypeTable(table, static_cast<uint32_t>(MODULEID));                          \
    }

    TSTYPETABLE_ACCESSOR_LIST(TSTYPETABLE_ACCESSOR)
#undef TSTYPETABLE_ACCESSOR

private:
    NO_COPY_SEMANTIC(TSManager);
    NO_MOVE_SEMANTIC(TSManager);

    GlobalTSTypeRef AddTSTypeToTypeTable(const JSHandle<TSType> &type, int tableId) const;

    JSHandle<TaggedArray> GenerateExportTableFromLiteral(const JSPandaFile *jsPandaFile, const CString &recordName);

    GlobalTSTypeRef FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const;

    GlobalTSTypeRef FindIteratorInstanceInInferTable(GlobalTSTypeRef kindGt, GlobalTSTypeRef elementGt) const;

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, JSHandle<JSTaggedValue> propertyName) const;

    std::string GetClassTypeStr(GlobalTSTypeRef gt) const;

    std::string GetClassInstanceTypeStr(GlobalTSTypeRef gt) const;

    std::string GetFunctionTypeStr(GlobalTSTypeRef gt) const;

    std::string GetArrayTypeStr(GlobalTSTypeRef gt) const;

    std::string GetPrimitiveStr(const GlobalTSTypeRef &gt) const;

    uint32_t RecordIhcToVecAndIndexMap(IHClassData &ihcData);

    uint32_t GetBuiltinIndex(GlobalTSTypeRef builtinGT) const;

    std::string GetBuiltinsName(uint32_t index) const;

    inline void SetBuiltinPandaFile(JSPandaFile *jsPandaFile)
    {
        builtinPandaFile_ = jsPandaFile;
    }

    inline void SetBuiltinRecordName(CString &builtinsRecordName)
    {
        builtinsRecordName_ = builtinsRecordName;
    }

    // for jsarray
    void TryGetElmsKind(panda_file::File::EntityId id, JSHandle<JSTaggedValue> &ekd);

    const kungfu::GateType GetBuiltinsGateTypeByPt(ProfileType pgoType);

    BuiltinTypeId GetBuiltinTypeIdByJSType(JSType jsType);

    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    // kungfu::PGOTypeManager ptManager_;
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    CMap<ProfileType, const kungfu::GateType> ptToGtMap_ {};
    std::map<GlobalTSTypeRef, IHClassData> gtIhcMap_ {};
    std::map<GlobalTSTypeRef, IHClassData> gtConstructorhcMap_ {};
    std::unordered_map<TypeLocation, GlobalTSTypeRef, HashTypeLocation> literalGTMap_ {};
    std::map<JSType, GlobalTSTypeRef> pgoBuiltinGTCache_ {};
    bool assertTypes_ {false};
    double typeThreshold_ {-1};

    // when the passmanager iterates each method, the curCP_ and curCPID_ should be updated
    // so that subsequent passes (type_infer, type_bytecode_lowering) can obtain the correct constpool.
    JSTaggedValue curCP_ {JSTaggedValue::Hole()};
    int32_t curCPID_ {0};
    const JSPandaFile *curJSPandaFile_ {nullptr};

    std::unordered_map<GlobalTypeID, GlobalTSTypeRef, HashGlobalTypeID> idGTMap_ {};
    std::map<GlobalTSTypeRef, std::tuple<CString, CString, uint32_t>> gtLiteralOffsetMap_ {};
    std::vector<uint32_t> builtinOffsets_ {};
    JSPandaFile *builtinPandaFile_ {nullptr};
    CString builtinsRecordName_ {""};
    std::map<ModuleInfo, GlobalTSTypeRef> moduleVarGtMap_{};
    kungfu::CompilationDriver *cmpDriver_ {nullptr};
    std::set<GlobalTSTypeRef> collectedGT_ {};  // use for storing types that need to generate hclasses

    friend class EcmaVM;
    friend class TSTypeAccessor;

    std::map<std::pair<const JSPandaFile *, uint32_t>, std::string> literalOffsetClassNameMap_ {};
    kungfu::BytecodeInfoCollector *bcInfoCollector_ {nullptr};
    // use for collect the literal of export type table.
    CMap<std::pair<const JSPandaFile *, CString>, JSTaggedValue> resolvedExportTable_ {};
    CMap<uint32_t, kungfu::GateType> methodOffsetToType_ {};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_MANAGER_H
