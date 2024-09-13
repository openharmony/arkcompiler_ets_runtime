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

#ifndef MAPLE_IR_INCLUDE_GLOBAL_TABLES_H
#define MAPLE_IR_INCLUDE_GLOBAL_TABLES_H
#include <iostream>
#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include "mempool.h"
#include "mempool_allocator.h"
#include "types_def.h"
#include "prim_types.h"
#include "mir_module.h"
#include "namemangler.h"
#include "mir_type.h"
#include "mir_const.h"

namespace maple {

class BinaryMplImport;  // circular dependency exists, no other choice

// to facilitate the use of unordered_map
class TyIdxHash {
public:
    std::size_t operator()(const TyIdx &tyIdx) const
    {
        return std::hash<uint32> {}(tyIdx);
    }
};

// to facilitate the use of unordered_map
class GStrIdxHash {
public:
    std::size_t operator()(const GStrIdx &gStrIdx) const
    {
        return std::hash<uint32> {}(gStrIdx);
    }
};

// to facilitate the use of unordered_map
class UStrIdxHash {
public:
    std::size_t operator()(const UStrIdx &uStrIdx) const
    {
        return std::hash<uint32> {}(uStrIdx);
    }
};

class IntConstKey {
    friend class IntConstHash;
    friend class IntConstCmp;

public:
    IntConstKey(int64 v, TyIdx tyIdx) : val(v), tyIdx(tyIdx) {}
    virtual ~IntConstKey() {}

private:
    int64 val;
    TyIdx tyIdx;
};

class IntConstHash {
public:
    std::size_t operator()(const IntConstKey &key) const
    {
        return std::hash<int64> {}(key.val) ^ (std::hash<uint64> {}(static_cast<uint64>(key.tyIdx)) << 1);
    }
};

class IntConstCmp {
public:
    bool operator()(const IntConstKey &lkey, const IntConstKey &rkey) const
    {
        return lkey.val == rkey.val && lkey.tyIdx == rkey.tyIdx;
    }
};

class TypeTable {
    friend BinaryMplImport;

public:
    static MIRType *voidPtrType;

    TypeTable();
    TypeTable(const TypeTable &) = delete;
    TypeTable &operator=(const TypeTable &) = delete;
    ~TypeTable();

    void Init();
    void Reset();
    void ReleaseTypes();

    std::vector<MIRType *> &GetTypeTable()
    {
        return typeTable;
    }

    const std::vector<MIRType *> &GetTypeTable() const
    {
        return typeTable;
    }

    auto &GetTypeHashTable() const
    {
        return typeHashTable;
    }

    auto &GetPtrTypeMap() const
    {
        return ptrTypeMap;
    }

    auto &GetRefTypeMap() const
    {
        return refTypeMap;
    }

    MIRType *GetTypeFromTyIdx(TyIdx tyIdx)
    {
        return const_cast<MIRType *>(const_cast<const TypeTable *>(this)->GetTypeFromTyIdx(tyIdx));
    }
    const MIRType *GetTypeFromTyIdx(TyIdx tyIdx) const
    {
        CHECK_FATAL(tyIdx < typeTable.size(), "array index out of range");
        return typeTable.at(tyIdx);
    }

    MIRType *GetTypeFromTyIdx(uint32 index) const
    {
        CHECK_FATAL(index < typeTable.size(), "array index out of range");
        return typeTable.at(index);
    }

    PrimType GetPrimTypeFromTyIdx(const TyIdx &tyIdx) const
    {
        CHECK_FATAL(tyIdx < typeTable.size(), "array index out of range");
        return typeTable.at(tyIdx)->GetPrimType();
    }

    MIRType *GetOrCreateMIRTypeNode(MIRType &ptype);

    TyIdx GetOrCreateMIRType(MIRType *pType)
    {
        return GetOrCreateMIRTypeNode(*pType)->GetTypeIndex();
    }

    uint32 GetTypeTableSize() const
    {
        return static_cast<uint32>(typeTable.size());
    }

    // Get primtive types.
    MIRType *GetPrimType(PrimType primType) const
    {
        DEBUG_ASSERT(primType < typeTable.size(), "array index out of range");
        return typeTable.at(primType);
    }

    MIRType *GetFloat() const
    {
        DEBUG_ASSERT(PTY_f32 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_f32);
    }

    MIRType *GetDouble() const
    {
        DEBUG_ASSERT(PTY_f64 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_f64);
    }

    MIRType *GetUInt1() const
    {
        DEBUG_ASSERT(PTY_u1 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u1);
    }

    MIRType *GetUInt8() const
    {
        DEBUG_ASSERT(PTY_u8 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u8);
    }

    MIRType *GetInt8() const
    {
        DEBUG_ASSERT(PTY_i8 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_i8);
    }

    MIRType *GetUInt16() const
    {
        DEBUG_ASSERT(PTY_u16 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u16);
    }

    MIRType *GetInt16() const
    {
        DEBUG_ASSERT(PTY_i16 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_i16);
    }

    MIRType *GetInt32() const
    {
        DEBUG_ASSERT(PTY_i32 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_i32);
    }

    MIRType *GetUInt32() const
    {
        DEBUG_ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u32);
    }

    MIRType *GetInt64() const
    {
        DEBUG_ASSERT(PTY_i64 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_i64);
    }

    MIRType *GetUInt64() const
    {
        DEBUG_ASSERT(PTY_u64 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u64);
    }

    MIRType *GetPtr() const
    {
        DEBUG_ASSERT(PTY_ptr < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_ptr);
    }

#ifdef USE_ARM32_MACRO
    MIRType *GetUIntType() const
    {
        DEBUG_ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u32);
    }

    MIRType *GetPtrType() const
    {
        DEBUG_ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u32);
    }
#else
    MIRType *GetUIntType() const
    {
        DEBUG_ASSERT(PTY_u64 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u64);
    }

    MIRType *GetPtrType() const
    {
        DEBUG_ASSERT(PTY_ptr < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_ptr);
    }
#endif

#ifdef USE_32BIT_REF
    MIRType *GetCompactPtr() const
    {
        DEBUG_ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u32);
    }

#else
    MIRType *GetCompactPtr() const
    {
        DEBUG_ASSERT(PTY_u64 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_u64);
    }

#endif
    MIRType *GetRef() const
    {
        DEBUG_ASSERT(PTY_ref < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_ref);
    }

    MIRType *GetAddr64() const
    {
        DEBUG_ASSERT(PTY_a64 < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_a64);
    }

    MIRType *GetVoid() const
    {
        DEBUG_ASSERT(PTY_void < typeTable.size(), "array index out of range");
        return typeTable.at(PTY_void);
    }

    // Get or Create derived types.
    MIRType *GetOrCreatePointerType(const TyIdx &pointedTyIdx, PrimType primType = PTY_ptr,
                                    const TypeAttrs &attrs = TypeAttrs());
    MIRType *GetOrCreatePointerType(const MIRType &pointTo, PrimType primType = PTY_ptr,
                                    const TypeAttrs &attrs = TypeAttrs());
    MIRType *GetVoidPtr() const
    {
        DEBUG_ASSERT(voidPtrType != nullptr, "voidPtrType should not be null");
        return voidPtrType;
    }

    MIRType *GetOrCreateFunctionType(const TyIdx &, const std::vector<TyIdx> &, const std::vector<TypeAttrs> &,
                                     bool isVarg = false, const TypeAttrs &retAttrs = TypeAttrs());

    TyIdx lastDefaultTyIdx;

private:
    using MIRTypePtr = MIRType *;
    struct Hash {
        size_t operator()(const MIRTypePtr &ty) const
        {
            return ty->GetHashIndex();
        }
    };

    struct Equal {
        bool operator()(const MIRTypePtr &tx, const MIRTypePtr &ty) const
        {
            return tx->EqualTo(*ty);
        }
    };

    // create an entry in typeTable for the type node
    MIRType *CreateType(const MIRType &oldType)
    {
        MIRType *newType = oldType.CopyMIRTypeNode();
        newType->SetTypeIndex(TyIdx(typeTable.size()));
        typeTable.push_back(newType);
        return newType;
    }

    void PushNull()
    {
        typeTable.push_back(nullptr);
    }
    void PopBack()
    {
        typeTable.pop_back();
    }

    MIRType *CreateAndUpdateMirTypeNode(MIRType &pType);

    MIRType *CreateMirType(uint32 primTypeIdx) const;
    void PutToHashTable(MIRType *mirType);

    std::unordered_set<MIRTypePtr, Hash, Equal> typeHashTable;
    std::unordered_map<TyIdx, TyIdx, TyIdxHash> ptrTypeMap;
    std::unordered_map<TyIdx, TyIdx, TyIdxHash> refTypeMap;
    std::vector<MIRType *> typeTable;
    mutable std::shared_timed_mutex mtx;
};

class StrPtrHash {
public:
    size_t operator()(const std::string *str) const
    {
        return std::hash<std::string> {}(*str);
    }

    size_t operator()(const std::u16string *str) const
    {
        return std::hash<std::u16string> {}(*str);
    }
};

class StrPtrEqual {
public:
    bool operator()(const std::string *str1, const std::string *str2) const
    {
        return *str1 == *str2;
    }

    bool operator()(const std::u16string *str1, const std::u16string *str2) const
    {
        return *str1 == *str2;
    }
};

// T can be std::string or std::u16string
// U can be GStrIdx, UStrIdx, or U16StrIdx
template <typename T, typename U>
class StringTable {
public:
    StringTable() = default;
    StringTable(const StringTable &) = delete;
    StringTable &operator=(const StringTable &) = delete;

    ~StringTable()
    {
        ReleaseStrings();
    }

    void Init()
    {
        // initialize 0th entry of stringTable with an empty string
        T *ptr = new T;
        stringTable.push_back(ptr);
    }

    void Reset()
    {
        ReleaseStrings();
        stringTable.clear();
        Init();
    }

    void ReleaseStrings()
    {
        stringTableMap.clear();
        for (auto it : stringTable) {
            delete it;
        }
    }

    U GetStrIdxFromName(const T &str) const
    {
        auto it = stringTableMap.find(&str);
        if (it == stringTableMap.end()) {
            return U(0);
        }
        return it->second;
    }

    U GetOrCreateStrIdxFromName(const T &str)
    {
        U strIdx = GetStrIdxFromName(str);
        if (strIdx == 0u) {
            strIdx.reset(stringTable.size());
            T *newStr = new T(str);
            stringTable.push_back(newStr);
            stringTableMap[newStr] = strIdx;
        }
        return strIdx;
    }

    size_t StringTableSize() const
    {
        return stringTable.size();
    }

    const T &GetStringFromStrIdx(U strIdx) const
    {
        DEBUG_ASSERT(strIdx < stringTable.size(), "array index out of range");
        return *stringTable[strIdx];
    }

    const T &GetStringFromStrIdx(uint32 idx) const
    {
        DEBUG_ASSERT(idx < stringTable.size(), "array index out of range");
        return *stringTable[idx];
    }

private:
    std::vector<const T *> stringTable;  // index is uint32
    std::unordered_map<const T *, U, StrPtrHash, StrPtrEqual> stringTableMap;
    mutable std::shared_timed_mutex mtx;
};

class FPConstTable {
public:
    FPConstTable(const FPConstTable &p) = delete;
    FPConstTable &operator=(const FPConstTable &p) = delete;
    ~FPConstTable();

    // get the const from floatConstTable or create a new one
    MIRFloatConst *GetOrCreateFloatConst(float fval);
    // get the const from doubleConstTable or create a new one
    MIRDoubleConst *GetOrCreateDoubleConst(double fval);

    static std::unique_ptr<FPConstTable> Create()
    {
        auto p = std::unique_ptr<FPConstTable>(new FPConstTable());
        p->PostInit();
        return p;
    }

private:
    FPConstTable() : floatConstTable(), doubleConstTable() {};
    void PostInit();
    MIRFloatConst *DoGetOrCreateFloatConst(float);
    MIRDoubleConst *DoGetOrCreateDoubleConst(double);
    std::shared_timed_mutex floatMtx;
    std::shared_timed_mutex doubleMtx;
    std::unordered_map<float, MIRFloatConst *> floatConstTable;     // map float const value to the table;
    std::unordered_map<double, MIRDoubleConst *> doubleConstTable;  // map double const value to the table;
    MIRFloatConst *nanFloatConst = nullptr;
    MIRFloatConst *infFloatConst = nullptr;
    MIRFloatConst *minusInfFloatConst = nullptr;
    MIRFloatConst *minusZeroFloatConst = nullptr;
    MIRDoubleConst *nanDoubleConst = nullptr;
    MIRDoubleConst *infDoubleConst = nullptr;
    MIRDoubleConst *minusInfDoubleConst = nullptr;
    MIRDoubleConst *minusZeroDoubleConst = nullptr;
};

class IntConstTable {
public:
    IntConstTable(const IntConstTable &p) = delete;
    IntConstTable &operator=(const IntConstTable &p) = delete;
    ~IntConstTable();

    MIRIntConst *GetOrCreateIntConst(uint64 val, MIRType &type);

    static std::unique_ptr<IntConstTable> Create()
    {
        auto p = std::unique_ptr<IntConstTable>(new IntConstTable());
        return p;
    }

private:
    IntConstTable() = default;
    MIRIntConst *DoGetOrCreateIntConst(uint64 val, MIRType &type);
    std::shared_timed_mutex mtx;
    std::unordered_map<IntConstKey, MIRIntConst *, IntConstHash, IntConstCmp> intConstTable;
};

// STypeNameTable is only used to store class and interface types.
// Each module maintains its own MIRTypeNameTable.
class STypeNameTable {
public:
    STypeNameTable() = default;
    virtual ~STypeNameTable() = default;

    void Reset()
    {
        gStrIdxToTyIdxMap.clear();
    }

    const std::unordered_map<GStrIdx, TyIdx, GStrIdxHash> &GetGStridxToTyidxMap() const
    {
        return gStrIdxToTyIdxMap;
    }

    TyIdx GetTyIdxFromGStrIdx(GStrIdx idx) const
    {
        const auto it = gStrIdxToTyIdxMap.find(idx);
        if (it == gStrIdxToTyIdxMap.cend()) {
            return TyIdx(0);
        }
        return it->second;
    }

    void SetGStrIdxToTyIdx(GStrIdx gStrIdx, TyIdx tyIdx)
    {
        gStrIdxToTyIdxMap[gStrIdx] = tyIdx;
    }

private:
    std::unordered_map<GStrIdx, TyIdx, GStrIdxHash> gStrIdxToTyIdxMap;
};

class FunctionTable {
public:
    FunctionTable()
    {
        Init();
    }

    virtual ~FunctionTable() = default;

    void Init()
    {
        // puIdx 0 is reserved
        funcTable.push_back(nullptr);
    }

    void Reset()
    {
        funcTable.clear();
        Init();
    }

    std::vector<MIRFunction *> &GetFuncTable()
    {
        return funcTable;
    }

    MIRFunction *GetFunctionFromPuidx(PUIdx pIdx) const
    {
        CHECK_FATAL(pIdx < funcTable.size(), "Invalid puIdx");
        return funcTable.at(pIdx);
    }

    void SetFunctionItem(uint32 pIdx, MIRFunction *func)
    {
        CHECK_FATAL(pIdx < funcTable.size(), "Invalid puIdx");
        funcTable[pIdx] = func;
    }

private:
    std::vector<MIRFunction *> funcTable;  // index is PUIdx
};

class GSymbolTable {
public:
    GSymbolTable();
    GSymbolTable(const GSymbolTable &) = delete;
    GSymbolTable &operator=(const GSymbolTable &) = delete;
    ~GSymbolTable();

    void Init();
    void Reset();
    void ReleaseSymbols();

    MIRModule *GetModule()
    {
        return module;
    }

    void SetModule(MIRModule *m)
    {
        module = m;
    }

    bool IsValidIdx(size_t idx) const
    {
        return idx < symbolTable.size();
    }

    MIRSymbol *GetSymbolFromStidx(uint32 idx, bool checkFirst = false) const
    {
        if (checkFirst && idx >= symbolTable.size()) {
            return nullptr;
        }
        DEBUG_ASSERT(IsValidIdx(idx), "symbol table index out of range");
        return symbolTable[idx];
    }

    void SetStrIdxStIdxMap(GStrIdx strIdx, StIdx stIdx)
    {
        strIdxToStIdxMap[strIdx] = stIdx;
    }

    StIdx GetStIdxFromStrIdx(GStrIdx idx) const
    {
        const auto it = strIdxToStIdxMap.find(idx);
        if (it == strIdxToStIdxMap.cend()) {
            return StIdx();
        }
        return it->second;
    }

    MIRSymbol *GetSymbolFromStrIdx(GStrIdx idx, bool checkFirst = false) const
    {
        return GetSymbolFromStidx(GetStIdxFromStrIdx(idx).Idx(), checkFirst);
    }

    auto &GetTable()
    {
        return symbolTable;
    }

    size_t GetSymbolTableSize() const
    {
        return symbolTable.size();
    }

    MIRSymbol *GetSymbol(uint32 idx) const
    {
        DEBUG_ASSERT(idx < symbolTable.size(), "array index out of range");
        return symbolTable.at(idx);
    }

    MIRSymbol *CreateSymbol(uint8 scopeID);
    bool AddToStringSymbolMap(const MIRSymbol &st);
#ifdef ARK_LITECG_DEBUG
    void Dump(bool isLocal, int32 indent = 0) const;
#endif

private:
    MIRModule *module = nullptr;
    // hash table mapping string index to st index
    std::unordered_map<GStrIdx, StIdx, GStrIdxHash> strIdxToStIdxMap;
    std::vector<MIRSymbol *> symbolTable;  // map symbol idx to symbol node
};

class ConstPool {
public:
    std::unordered_map<std::u16string, MIRSymbol *> &GetConstU16StringPool()
    {
        return constU16StringPool;
    }

    void Reset()
    {
        constMap.clear();
        importedLiteralNames.clear();
        constU16StringPool.clear();
    }

    void InsertConstPool(GStrIdx strIdx, MIRConst *cst)
    {
        (void)constMap.emplace(strIdx, cst);
    }

    MIRConst *GetConstFromPool(GStrIdx strIdx)
    {
        return constMap[strIdx];
    }

    void PutLiteralNameAsImported(GStrIdx gIdx)
    {
        (void)importedLiteralNames.insert(gIdx);
    }

    bool LookUpLiteralNameFromImported(GStrIdx gIdx)
    {
        return importedLiteralNames.find(gIdx) != importedLiteralNames.end();
    }

protected:
    std::unordered_map<GStrIdx, MIRConst *, GStrIdxHash> constMap;
    std::set<GStrIdx> importedLiteralNames;

private:
    std::unordered_map<std::u16string, MIRSymbol *> constU16StringPool;
};

class GlobalTables {
public:
    static GlobalTables &GetGlobalTables();

    static StringTable<std::string, GStrIdx> &GetStrTable()
    {
        return globalTables.gStringTable;
    }

    static StringTable<std::string, UStrIdx> &GetUStrTable()
    {
        return globalTables.uStrTable;
    }

    static StringTable<std::u16string, U16StrIdx> &GetU16StrTable()
    {
        return globalTables.u16StringTable;
    }

    static TypeTable &GetTypeTable()
    {
        return globalTables.typeTable;
    }

    static FPConstTable &GetFpConstTable()
    {
        return *(globalTables.fpConstTablePtr);
    }

    static STypeNameTable &GetTypeNameTable()
    {
        return globalTables.typeNameTable;
    }

    static FunctionTable &GetFunctionTable()
    {
        return globalTables.functionTable;
    }

    static GSymbolTable &GetGsymTable()
    {
        return globalTables.gSymbolTable;
    }

    static ConstPool &GetConstPool()
    {
        return globalTables.constPool;
    }

    static IntConstTable &GetIntConstTable()
    {
        return *(globalTables.intConstTablePtr);
    }

    static void Reset()
    {
        globalTables.typeTable.Reset();
        globalTables.typeNameTable.Reset();
        globalTables.functionTable.Reset();
        globalTables.gSymbolTable.Reset();
        globalTables.constPool.Reset();
        globalTables.fpConstTablePtr = FPConstTable::Create();
        globalTables.intConstTablePtr = IntConstTable::Create();
        globalTables.gStringTable.Reset();
        globalTables.uStrTable.Reset();
        globalTables.u16StringTable.Reset();
    }

    GlobalTables(const GlobalTables &globalTables) = delete;
    GlobalTables(const GlobalTables &&globalTables) = delete;
    GlobalTables &operator=(const GlobalTables &globalTables) = delete;
    GlobalTables &operator=(const GlobalTables &&globalTables) = delete;

private:
    GlobalTables() : fpConstTablePtr(FPConstTable::Create()), intConstTablePtr(IntConstTable::Create())
    {
        gStringTable.Init();
        uStrTable.Init();
        u16StringTable.Init();
    }
    virtual ~GlobalTables() = default;
    static GlobalTables globalTables;

    TypeTable typeTable;
    STypeNameTable typeNameTable;
    FunctionTable functionTable;
    GSymbolTable gSymbolTable;
    ConstPool constPool;
    std::unique_ptr<FPConstTable> fpConstTablePtr;
    std::unique_ptr<IntConstTable> intConstTablePtr;
    StringTable<std::string, GStrIdx> gStringTable;
    StringTable<std::string, UStrIdx> uStrTable;
    StringTable<std::u16string, U16StrIdx> u16StringTable;
};

inline MIRType &GetTypeFromTyIdx(TyIdx idx)
{
    return *(GlobalTables::GetTypeTable().GetTypeFromTyIdx(idx));
}
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_GLOBAL_TABLES_H
