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

#ifndef MAPLE_IR_INCLUDE_MIR_MODULE_H
#define MAPLE_IR_INCLUDE_MIR_MODULE_H
#include "types_def.h"
#include "prim_types.h"
#include "intrinsics.h"
#include "opcodes.h"
#include "mpl_logging.h"
#include "namemangler.h"
#include "gcov_profile.h"
#include "string_utils.h"
#if MIR_FEATURE_FULL
#include <string>
#include <unordered_set>
#include <shared_mutex>
#include <thread>
#include <mutex>
#include <map>
#include "mempool.h"
#include "mempool_allocator.h"
#include "maple_string.h"
#endif  // MIR_FEATURE_FULL

namespace maple {
class CallInfo;    // circular dependency exists, no other choice
class MIRModule;   // circular dependency exists, no other choice
class MIRBuilder;  // circular dependency exists, no other choice
using MIRModulePtr = MIRModule *;
using MIRBuilderPtr = MIRBuilder *;

enum MIRFlavor {
    kFlavorUnknown,
    kFeProduced,
    kMeProduced,
    kBeLowered,
    kFlavorMbc,
    kMmpl,
    kCmplV1,
    kCmpl,  // == CMPLv2
    kFlavorLmbc,
};

enum MIRSrcLang {
    kSrcLangUnknown,
    kSrcLangC,
    kSrcLangJs,
    kSrcLangCPlusPlus,
    kSrcLangChar,
    // SrcLangSwift : when clang adds support for Swift.
};

class CalleePair {
public:
    CalleePair(PUIdx id, int32_t index) : id(id), index(index) {}
    bool operator<(const CalleePair &func) const
    {
        if (id < func.id)
            return true;
        else if (id == func.id && index < func.index) {
            return true;
        } else {
            return false;
        }
    }

private:
    PUIdx id;
    int32_t index;
};

class CallerSummary {
public:
    CallerSummary(PUIdx id, uint32 stmtId) : id(id), stmtId(stmtId) {}
    PUIdx GetPuidx() const
    {
        return id;
    };
    uint32 GetStmtId() const
    {
        return stmtId;
    }

private:
    PUIdx id;
    uint32 stmtId;
};

// This data structure is for the ipa-cp. Important expresstion is about the condtion statement.
class ImpExpr {
public:
    ImpExpr(uint32 stmtId, uint32 paramIndex) : stmtId(stmtId), paramIndex(paramIndex) {}
    uint32 GetStmtId() const
    {
        return stmtId;
    }
    uint32 GetParamIndex() const
    {
        return paramIndex;
    }

private:
    uint32 stmtId;
    uint32 paramIndex;
};

// blksize gives the size of the memory block in bytes; there are (blksize+3)/4
// words; 1 bit for each word, so the bit vector's length in bytes is
// ((blksize+3)/4+7)/8
static inline uint32 BlockSize2BitVectorSize(uint32 blkSize)
{
    uint32 bitVectorLen = ((blkSize + 3) / 4 + 7) / 8; // the bit vector's length in bytes is ((blksize+3)/4+7)/8
    constexpr uint32 kRoundUp2Bit = 0xfffffffc;        // 11111111111111111111111111111100
    return (bitVectorLen + 3) & kRoundUp2Bit;  // add 3 and round up to word boundary
}

#if MIR_FEATURE_FULL
class MIRType;            // circular dependency exists, no other choice
class MIRFunction;        // circular dependency exists, no other choice
class MIRSymbol;          // circular dependency exists, no other choice
class MIRSymbolTable;     // circular dependency exists, no other choice
class MIRFloatConst;      // circular dependency exists, no other choice
class MIRDoubleConst;     // circular dependency exists, no other choice
class MIRBuilder;         // circular dependency exists, no other choice
class DebugInfo;          // circular dependency exists, no other choice
class EAConnectionGraph;  // circular dependency exists, no other choice
using MIRInfoPair = std::pair<GStrIdx, uint32>;
using MIRInfoVector = MapleVector<MIRInfoPair>;
using MIRDataPair = std::pair<GStrIdx, std::vector<uint8>>;
using MIRDataVector = MapleVector<MIRDataPair>;
constexpr int kMaxEncodedValueLen = 10;
struct EncodedValue {
    uint8 encodedValue[kMaxEncodedValueLen] = {0};
};

class MIRTypeNameTable {
public:
    explicit MIRTypeNameTable(MapleAllocator &allocator) : gStrIdxToTyIdxMap(std::less<GStrIdx>(), allocator.Adapter())
    {
    }

    ~MIRTypeNameTable() = default;

    const MapleMap<GStrIdx, TyIdx> &GetGStrIdxToTyIdxMap() const
    {
        return gStrIdxToTyIdxMap;
    }

    TyIdx GetTyIdxFromGStrIdx(GStrIdx idx) const
    {
        auto it = gStrIdxToTyIdxMap.find(idx);
        if (it == gStrIdxToTyIdxMap.end()) {
            return TyIdx(0);
        }
        return it->second;
    }

    void SetGStrIdxToTyIdx(GStrIdx gStrIdx, TyIdx tyIdx)
    {
        gStrIdxToTyIdxMap[gStrIdx] = tyIdx;
    }

    size_t Size() const
    {
        return gStrIdxToTyIdxMap.size();
    }

private:
    MapleMap<GStrIdx, TyIdx> gStrIdxToTyIdxMap;
};

class MIRModule {
public:
    bool firstInline = true;
    using CallSite = std::pair<CallInfo *, PUIdx>;

    explicit MIRModule(const std::string &fn = "");
    MIRModule(MIRModule &p) = delete;
    MIRModule &operator=(const MIRModule &module) = delete;
    ~MIRModule();

    MemPool *GetMemPool() const
    {
        return memPool;
    }
    MemPool *GetPragmaMemPool()
    {
        return pragmaMemPool;
    }
    MapleAllocator &GetPragmaMPAllocator()
    {
        return pragmaMemPoolAllocator;
    }
    const MapleAllocator &GetMPAllocator() const
    {
        return memPoolAllocator;
    }

    void ReleasePragmaMemPool()
    {
        if (pragmaMemPool) {
            memPoolCtrler.DeleteMemPool(pragmaMemPool);
        }
        pragmaMemPool = nullptr;
    }

    MapleAllocator &GetMPAllocator()
    {
        return memPoolAllocator;
    }

    const auto &GetFunctionList() const
    {
        return functionList;
    }
    auto &GetFunctionList()
    {
        return functionList;
    }

    const MapleVector<std::string> &GetImportedMplt() const
    {
        return importedMplt;
    }
    void PushbackImportedMplt(const std::string &importFileName)
    {
        importedMplt.push_back(importFileName);
    }

    MIRTypeNameTable *GetTypeNameTab()
    {
        return typeNameTab;
    }

    const MapleVector<GStrIdx> &GetTypeDefOrder() const
    {
        return typeDefOrder;
    }
    void PushbackTypeDefOrder(GStrIdx gstrIdx)
    {
        typeDefOrder.push_back(gstrIdx);
    }

    void SetCurFunction(MIRFunction *f)
    {
        curFunction = f;
    }

    MIRSrcLang GetSrcLang() const
    {
        return srcLang;
    }

    const MapleSet<StIdx> &GetSymbolSet() const
    {
        return symbolSet;
    }

    const MapleVector<StIdx> &GetSymbolDefOrder() const
    {
        return symbolDefOrder;
    }

    GcovProfileData *GetGcovProfile()
    {
        return gcovProfile;
    }
    void SetGcovProfile(GcovProfileData *info)
    {
        gcovProfile = info;
    }

    void SetSomeSymbolNeedForDecl(bool s)
    {
        someSymbolNeedForwDecl = s;
    }

    MIRFunction *CurFunction() const
    {
        return curFunction;
    }

    MemPool *CurFuncCodeMemPool() const;
    MapleAllocator *CurFuncCodeMemPoolAllocator() const;
    MapleAllocator &GetCurFuncCodeMPAllocator() const;
    void AddExternStructType(TyIdx tyIdx);
    void AddExternStructType(const MIRType *t);
    void AddSymbol(StIdx stIdx);
    void AddSymbol(const MIRSymbol *s);
    void AddFunction(MIRFunction *pf)
    {
        functionList.push_back(pf);
    }

#ifdef ARK_LITECG_DEBUG
    void DumpGlobals(bool emitStructureType = true) const;
    void Dump(bool emitStructureType = true, const std::unordered_set<std::string> *dumpFuncSet = nullptr) const;
    void DumpToFile(const std::string &fileNameStr, bool emitStructureType = true) const;
    void DumpInlineCandidateToFile(const std::string &fileNameStr);
    void DumpDefType();
#endif
#ifdef ARK_LITECG_DEBUG
    void DumpClassToFile(const std::string &path) const;
    void DumpFunctionList(const std::unordered_set<std::string> *dumpFuncSet) const;
    void DumpGlobalArraySymbol() const;
#endif

    void Emit(const std::string &outFileName) const;
    uint32 GetAndIncFloatNum()
    {
        return floatNum++;
    }

    void SetEntryFunction(MIRFunction *f)
    {
        entryFunc = f;
    }

    MIRFunction *GetEntryFunction() const
    {
        return entryFunc;
    }

    uint32 GetFileinfo(GStrIdx strIdx) const;
    void OutputFunctionListAsciiMpl(const std::string &phaseName);
    const std::string &GetFileName() const
    {
        return fileName;
    }

    const std::string &GetFileText() const
    {
        return fileText;
    }

    bool IsNeedFile() const
    {
        return needFile;
    }

#ifdef ARK_LITECG_DEBUG
    std::string GetFileNameAsPostfix() const;
#endif
    void SetFileName(const std::string &name)
    {
        fileName = name;
    }

    std::string GetProfileDataFileName() const
    {
        std::string profileDataFileName = fileName.substr(0, fileName.find_last_of("."));
        std::replace(profileDataFileName.begin(), profileDataFileName.end(), '.', '_');
        std::replace(profileDataFileName.begin(), profileDataFileName.end(), '-', '_');
        std::replace(profileDataFileName.begin(), profileDataFileName.end(), '/', '_');
        profileDataFileName = profileDataFileName + namemangler::kProfFileNameExt;
        return profileDataFileName;
    }

    bool IsCModule() const
    {
        return srcLang == kSrcLangC || srcLang == kSrcLangCPlusPlus;
    }

    bool IsCPlusPlusModule() const
    {
        return srcLang == kSrcLangCPlusPlus;
    }

    bool IsCharModule() const
    {
        return srcLang == kSrcLangChar;
    }

    void addSuperCall(const std::string &func)
    {
        (void)superCallSet.insert(func);
    }

    bool findSuperCall(const std::string &func) const
    {
        return superCallSet.find(func) != superCallSet.end();
    }

    void SetUseFuncCodeMemPoolTmp()
    {
        useFuncCodeMemPoolTmp = true;
    }

    void ResetUseFuncCodeMemPoolTmp()
    {
        useFuncCodeMemPoolTmp = false;
    }

    size_t GetOptFuncsSize() const
    {
        return optimizedFuncs.size();
    }

    void AddOptFuncs(MIRFunction *func)
    {
        optimizedFuncs.emplace(func);
    }

    const MapleSet<MIRFunction *> &GetOptFuncs() const
    {
        return optimizedFuncs;
    }

    bool IsOptFunc(MIRFunction *func) const
    {
        if (std::find(optimizedFuncs.begin(), optimizedFuncs.end(), func) != optimizedFuncs.end()) {
            return true;
        }
        return false;
    }

    void AddOptFuncsType(MIRType *type)
    {
        optimizedFuncsType.emplace(type);
    }

    const MapleMap<PUIdx, MapleSet<FieldID> *> &GetPuIdxFieldInitializedMap() const
    {
        std::shared_lock<std::shared_timed_mutex> lock(fieldMapMutex);
        return puIdxFieldInitializedMap;
    }
    void SetPuIdxFieldSet(PUIdx puIdx, MapleSet<FieldID> *fieldIDSet)
    {
        std::unique_lock<std::shared_timed_mutex> lock(fieldMapMutex);
        puIdxFieldInitializedMap[puIdx] = fieldIDSet;
    }

    std::map<CalleePair, std::map<int64_t, std::vector<CallerSummary>>> &GetCalleeParamAboutInt()
    {
        return calleeParamAboutInt;
    }

    std::map<CalleePair, std::map<float, std::vector<CallerSummary>>> &GetCalleeParamAboutFloat()
    {
        return calleeParamAboutFloat;
    }

    std::map<CalleePair, std::map<double, std::vector<CallerSummary>>> &GetCalleeParamAboutDouble()
    {
        return calleeParamAboutDouble;
    }

    std::map<PUIdx, std::vector<ImpExpr>> &GetFuncImportantExpr()
    {
        return funcImportantExpr;
    }

    const auto &GetRealCaller() const
    {
        return realCaller;
    }

    auto &GetRealCaller()
    {
        return realCaller;
    }

    const MapleSet<uint32_t> &GetInlineGlobals() const
    {
        return inliningGlobals;
    }
    void InsertInlineGlobal(uint32_t global)
    {
        (void)inliningGlobals.insert(global);
    }

    const MapleSet<FieldID> *GetPUIdxFieldInitializedMapItem(PUIdx key) const
    {
        std::shared_lock<std::shared_timed_mutex> lock(fieldMapMutex);
        auto it = puIdxFieldInitializedMap.find(key);
        if (it != puIdxFieldInitializedMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::ostream &GetOut() const
    {
        return out;
    }

    const MIRBuilderPtr &GetMIRBuilder() const
    {
        return mirBuilder;
    }

    const std::string &GetEntryFuncName() const
    {
        return entryFuncName;
    }
    void SetEntryFuncName(const std::string &entryFunctionName)
    {
        entryFuncName = entryFunctionName;
    }

    TyIdx GetThrowableTyIdx() const
    {
        return throwableTyIdx;
    }
    void SetThrowableTyIdx(TyIdx throwableTypeIndex)
    {
        throwableTyIdx = throwableTypeIndex;
    }

    bool GetWithProfileInfo() const
    {
        return withProfileInfo;
    }
    void SetWithProfileInfo(bool withProfInfo)
    {
        withProfileInfo = withProfInfo;
    }

    bool IsInIPA() const
    {
        return inIPA;
    }
    bool IsWithMe() const
    {
        return withMe;
    }
    void SetWithMe(bool isWithMe)
    {
        withMe = isWithMe;
    }
    void SetInIPA(bool isInIPA)
    {
        inIPA = isInIPA;
    }

    void SetFileText(const std::string &inText)
    {
        fileText = inText;
        needFile = false;
    }

    MIRInfoVector &GetFileInfo()
    {
        return fileInfo;
    }
    void PushFileInfoPair(MIRInfoPair pair)
    {
        fileInfo.push_back(pair);
    }
    void SetFileInfo(const MIRInfoVector &fileInf)
    {
        fileInfo = fileInf;
    }

    MapleVector<bool> &GetFileInfoIsString()
    {
        return fileInfoIsString;
    }
    void SetFileInfoIsString(const MapleVector<bool> &fileInfoIsStr)
    {
        fileInfoIsString = fileInfoIsStr;
    }
    void PushFileInfoIsString(bool isString)
    {
        fileInfoIsString.push_back(isString);
    }

    const MIRDataVector &GetFileData() const
    {
        return fileData;
    }
    void PushbackFileData(const MIRDataPair &pair)
    {
        fileData.push_back(pair);
    }

    const MIRInfoVector &GetSrcFileInfo() const
    {
        return srcFileInfo;
    }
    void PushbackFileInfo(const MIRInfoPair &pair)
    {
        srcFileInfo.push_back(pair);
    }

    const MIRFlavor &GetFlavor() const
    {
        return flavor;
    }
    void SetFlavor(MIRFlavor flv)
    {
        flavor = flv;
    }

    void SetSrcLang(MIRSrcLang sourceLanguage)
    {
        srcLang = sourceLanguage;
    }

    uint16 GetID() const
    {
        return id;
    }

    void SetID(uint16 num)
    {
        id = num;
    }

    uint32 GetGlobalMemSize() const
    {
        return globalMemSize;
    }
    void SetGlobalMemSize(uint32 globalMemberSize)
    {
        globalMemSize = globalMemberSize;
    }

    uint8 *GetGlobalBlockMap()
    {
        return globalBlkMap;
    }
    void SetGlobalBlockMap(uint8 *globalBlockMap)
    {
        globalBlkMap = globalBlockMap;
    }

    uint8 *GetGlobalWordsTypeTagged()
    {
        return globalWordsTypeTagged;
    }
    void SetGlobalWordsTypeTagged(uint8 *globalWordsTyTagged)
    {
        globalWordsTypeTagged = globalWordsTyTagged;
    }

    uint8 *GetGlobalWordsRefCounted()
    {
        return globalWordsRefCounted;
    }
    void SetGlobalWordsRefCounted(uint8 *counted)
    {
        globalWordsRefCounted = counted;
    }

    uint32 GetNumFuncs() const
    {
        return numFuncs;
    }

    void SetNumFuncs(uint32 numFunc)
    {
        numFuncs = numFunc;
    }

    MapleVector<GStrIdx> &GetImportFiles()
    {
        return importFiles;
    }

    void PushbackImportPath(GStrIdx path)
    {
        importPaths.push_back(path);
    }

    MapleVector<MapleString> &GetAsmDecls()
    {
        return asmDecls;
    }

    const MapleSet<uint32> &GetClassList() const
    {
        return classList;
    }

    const std::map<PUIdx, std::vector<CallInfo *>> &GetMethod2TargetMap() const
    {
        return method2TargetMap;
    }

    std::vector<CallInfo *> &GetMemFromMethod2TargetMap(PUIdx methodPuIdx)
    {
        return method2TargetMap[methodPuIdx];
    }

    void SetMethod2TargetMap(const std::map<PUIdx, std::vector<CallInfo *>> &map)
    {
        method2TargetMap = map;
    }

    void AddMemToMethod2TargetMap(PUIdx idx, const std::vector<CallInfo *> &callSite)
    {
        method2TargetMap[idx] = callSite;
    }

    bool HasTargetHash(PUIdx idx, uint32 key) const
    {
        auto it = method2TargetHash.find(idx);
        if (it == method2TargetHash.end()) {
            return false;
        }
        return it->second.find(key) != it->second.end();
    }
    void InsertTargetHash(PUIdx idx, uint32 key)
    {
        (void)method2TargetHash[idx].insert(key);
    }
    void AddValueToMethod2TargetHash(PUIdx idx, const std::unordered_set<uint32> &value)
    {
        method2TargetHash[idx] = value;
    }

    const std::map<GStrIdx, EAConnectionGraph *> &GetEASummary() const
    {
        return eaSummary;
    }
    void SetEAConnectionGraph(GStrIdx funcNameIdx, EAConnectionGraph *eaCg)
    {
        eaSummary[funcNameIdx] = eaCg;
    }

    bool IsWithDbgInfo() const
    {
        return withDbgInfo;
    }

    bool HasPartO2List() const
    {
        return hasPartO2List;
    }

    void SetHasPartO2List(bool value)
    {
        hasPartO2List = value;
    }

    bool IsInPartO2List(const GStrIdx &idx) const
    {
        return partO2FuncList.count(idx) > 0;
    }

    void SetBaseName(const std::string &curbaseName)
    {
        baseName = curbaseName;
    }
    const std::string &GetBaseName() const
    {
        return baseName;
    }
    void SetOutputFileName(const std::string &curOFileName)
    {
        outputFileName = curOFileName;
    }
    const std::string &GetOutputFileName() const
    {
        return outputFileName;
    }
    void SetInputFileName(const std::string &curInFileName)
    {
        inputFileName = curInFileName;
    }
    const std::string &GetInputFileName() const
    {
        return inputFileName;
    }

    uint32 GetUniqueID() const
    {
        return UINT_MAX;
    }

    void SetCurModulePC(uint32 pc)
    {
        curModulePC = pc;
    }

    uint32 GetCurModulePC() const
    {
        return curModulePC;
    }

    void SetLastModulePC(uint32 pc)
    {
        lastModulePC = pc;
    }

    uint32 GetLastModulePC() const
    {
        return lastModulePC;
    }

    void SetIsAArch64(bool isAArch)
    {
        isAArch64 = isAArch;
    }

    bool IsAArch64() const
    {
        return isAArch64;
    }

private:
    MemPool *memPool;
    MemPool *pragmaMemPool;
    MapleAllocator memPoolAllocator;
    MapleAllocator pragmaMemPoolAllocator;
    MapleList<MIRFunction *> functionList;  // function table in the order of the appearance of function bodies; it
    // excludes prototype-only functions
    MapleVector<std::string> importedMplt;
    MIRTypeNameTable *typeNameTab;
    MapleVector<GStrIdx> typeDefOrder;

    MapleSet<TyIdx> externStructTypeSet;
    MapleSet<StIdx> symbolSet;
    MapleVector<StIdx> symbolDefOrder;
    GcovProfileData *gcovProfile;
    bool someSymbolNeedForwDecl = false;  // some symbols' addressses used in initialization

    std::ostream &out;
    MIRBuilder *mirBuilder;
    std::string entryFuncName = "";  // name of the entry function
    std::string fileName;
    std::string fileText;
    bool needFile = true;
    TyIdx throwableTyIdx {0};
    bool withProfileInfo = false;

    bool withDbgInfo = false;
    bool inIPA = false;
    bool withMe = true;
    MIRInfoVector fileInfo;              // store info provided under fileInfo keyword
    MapleVector<bool> fileInfoIsString;  // tells if an entry has string value
    MIRDataVector fileData;
    MIRInfoVector srcFileInfo;  // store info provided under srcFileInfo keyword
    MIRFlavor flavor = kFlavorUnknown;
    MIRSrcLang srcLang = kSrcLangUnknown;  // the source language
    uint16 id = 0xffff;
    uint32 globalMemSize = 0;       // size of storage space for all global variables
    uint8 *globalBlkMap = nullptr;  // the memory map of the block containing all the
    // globals, for specifying static initializations
    uint8 *globalWordsTypeTagged = nullptr;  // bit vector where the Nth bit tells whether
    // the Nth word in globalBlkMap has typetag;
    // if yes, the typetag is the N+1th word; the
    // bitvector's size is given by
    // BlockSize2BitvectorSize(globalMemSize)
    uint8 *globalWordsRefCounted = nullptr;  // bit vector where the Nth bit tells whether
    // the Nth word points to a reference-counted
    // dynamic memory block; the bitvector's size
    // is given by BlockSize2BitvectorSize(globalMemSize)
    uint32 numFuncs = 0;  // because puIdx 0 is reserved, numFuncs is also the highest puIdx
    MapleVector<GStrIdx> importFiles;
    MapleVector<GStrIdx> importPaths;
    MapleVector<MapleString> asmDecls;
    MapleSet<uint32> classList;

    std::map<PUIdx, std::vector<CallInfo *>> method2TargetMap;
    std::map<PUIdx, std::unordered_set<uint32>> method2TargetHash;
    std::map<GStrIdx, EAConnectionGraph *> eaSummary;

    bool useFuncCodeMemPoolTmp = false;
    MIRFunction *entryFunc = nullptr;
    uint32 floatNum = 0;
    // curFunction for single thread, curFunctionMap for multiple threads
    std::map<std::thread::id, MIRFunction *> curFunctionMap;
    mutable std::mutex curFunctionMutex;
    MIRFunction *curFunction;
    MapleSet<MIRFunction *> optimizedFuncs;
    MapleSet<MIRType *> optimizedFuncsType;
    // Add the field for decouple optimization
    std::unordered_set<std::string> superCallSet;
    // record all the fields that are initialized in the constructor. module scope,
    // if puIdx doesn't appear in this map, it writes to all field id
    // if puIdx appears in the map, but it's corresponding MapleSet is nullptr, it writes nothing fieldID
    // if puIdx appears in the map, and the value of first corresponding MapleSet is 0, the puIdx appears in this module
    // and writes to all field id otherwise, it writes the field ids in MapleSet
    MapleMap<PUIdx, MapleSet<FieldID> *> puIdxFieldInitializedMap;
    mutable std::shared_timed_mutex fieldMapMutex;
    std::map<std::pair<GStrIdx, GStrIdx>, GStrIdx> realCaller;
    MapleSet<uint32_t> inliningGlobals;  // global symbols accessed, used for inlining
    bool hasPartO2List = false;
    MapleSet<GStrIdx> partO2FuncList;
    std::string inputFileName = "";
    std::string baseName = "";
    std::string outputFileName = "";
    MapleMap<uint32, MapleSet<uint32>> safetyWarningMap;  // <postion, stmt original id> indexed map for large module.
    std::map<CalleePair, std::map<int64_t, std::vector<CallerSummary>>> calleeParamAboutInt;
    std::map<CalleePair, std::map<double, std::vector<CallerSummary>>> calleeParamAboutDouble;
    std::map<CalleePair, std::map<float, std::vector<CallerSummary>>> calleeParamAboutFloat;
    std::map<PUIdx, std::vector<ImpExpr>> funcImportantExpr;
    uint32 lastModulePC = 0;
    uint32 curModulePC = 0;
    bool isAArch64 = false;
};
#endif  // MIR_FEATURE_FULL
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_MODULE_H
