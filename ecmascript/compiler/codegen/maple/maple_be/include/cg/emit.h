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

#ifndef MAPLEBE_INCLUDE_CG_EMIT_H
#define MAPLEBE_INCLUDE_CG_EMIT_H

/* C++ headers */
#include <fstream>
#include <functional>
#include <map>
#include <array>
#include <sstream>
#include <sys/stat.h>
#include "isa.h"
#include "asm_info.h"
#include "cg.h"

/* Maple IR headers */
#include "mir_module.h"
#include "mir_const.h"
#include "mempool_allocator.h"
#include "namemangler.h"

namespace maple {
const char *GetDwTagName(unsigned n);
const char *GetDwFormName(unsigned n);
const char *GetDwAtName(unsigned n);
} /* namespace maple */

#if TARGRISCV64
#define CMNT "\t# "
#else
#define CMNT "\t// "
#endif
#define TEXT_BEGIN text0
#define TEXT_END etext0
#define DEBUG_INFO_0 debug_info0
#define DEBUG_ABBREV_0 debug_abbrev0
#define DEBUG_LINE_0 debug_line0
#define DEBUG_STR_LABEL ASF

namespace maplebe {
constexpr int32 kSizeOfDecoupleStaticStruct = 4;
constexpr uint32 kHugeSoInsnCountThreshold = 0x1f00000; /* 124M (4bytes per Insn), leave 4M rooms for 128M */
constexpr char kHugeSoPostFix[] = "$$hugeso_";
constexpr char kDebugMapleThis[] = "_this";
constexpr uint32 kDwarfVersion = 4;
constexpr uint32 kSizeOfPTR = 8;
class StructEmitInfo {
public:
    /* default ctor */
    StructEmitInfo() = default;

    ~StructEmitInfo() = default;

    uint16 GetNextFieldOffset() const
    {
        return nextFieldOffset;
    }

    void SetNextFieldOffset(uint16 offset)
    {
        nextFieldOffset = offset;
    }

    void IncreaseNextFieldOffset(uint16 value)
    {
        nextFieldOffset += value;
    }

    uint8 GetCombineBitFieldWidth() const
    {
        return combineBitFieldWidth;
    }

    void SetCombineBitFieldWidth(uint8 offset)
    {
        combineBitFieldWidth = offset;
    }

    void IncreaseCombineBitFieldWidth(uint8 value)
    {
        combineBitFieldWidth += value;
    }

    void DecreaseCombineBitFieldWidth(uint8 value)
    {
        combineBitFieldWidth -= value;
    }

    uint64 GetCombineBitFieldValue() const
    {
        return combineBitFieldValue;
    }

    void SetCombineBitFieldValue(uint64 value)
    {
        combineBitFieldValue = value;
    }

    uint64 GetTotalSize() const
    {
        return totalSize;
    }

    void SetTotalSize(uint64 value)
    {
        totalSize = value;
    }

    void IncreaseTotalSize(uint64 value)
    {
        totalSize += value;
    }

private:
    /* Next field offset in struct. */
    uint16 nextFieldOffset = 0;
    uint8 combineBitFieldWidth = 0;
    uint64 combineBitFieldValue = 0;
    /* Total size emitted in current struct. */
    uint64 totalSize = 0;
};

class FuncEmitInfo {
public:
    CGFunc &GetCGFunc()
    {
        return cgFunc;
    }

    const CGFunc &GetCGFunc() const
    {
        return cgFunc;
    }

protected:
    explicit FuncEmitInfo(CGFunc &func) : cgFunc(func) {}
    ~FuncEmitInfo() = default;

private:
    CGFunc &cgFunc;
};

class Emitter {
public:
    virtual void Finish() {}

    virtual void CloseOutput()
    {
        if (fileStream.is_open()) {
            fileStream << outStream.str();
            fileStream.close();
        }

        rangeIdx2PrefixStr.clear();
        hugeSoTargets.clear();
        fileMap.clear();
    }

    MOperator GetCurrentMOP() const
    {
        return currentMop;
    }

    void SetCurrentMOP(const MOperator &mOp)
    {
        currentMop = mOp;
    }

    void EmitAsmLabel(AsmLabel label);
    void EmitAsmLabel(const MIRSymbol &mirSymbol, AsmLabel label);
    void EmitFileInfo(const std::string &fileName);
    /* a symbol start/end a block */
    void EmitBlockMarker(const std::string &markerName, const std::string &sectionName, bool withAddr,
                         const std::string &addrName = "");
    void EmitNullConstant(uint64 size);
    void EmitScalarConstant(MIRConst &mirConst, bool newLine = true, bool flag32 = false, bool isIndirect = false);
    void EmitFunctionSymbolTable(FuncEmitInfo &funcEmitInfo);
    void EmitStr(const std::string &mplStr, bool emitAscii = false, bool emitNewline = false);
    void EmitStrConstant(const MIRStrConst &mirStrConst, bool isIndirect = false);
    void EmitStr16Constant(const MIRStr16Const &mirStr16Const);
    void EmitIntConst(const MIRSymbol &mirSymbol, MIRAggConst &aggConst, uint32 itabConflictIndex,
                      const std::map<GStrIdx, MIRType *> &strIdx2Type, size_t idx);
    void EmitAddrofFuncConst(const MIRSymbol &mirSymbol, MIRConst &elemConst, size_t idx);
    void EmitAddrofSymbolConst(const MIRSymbol &mirSymbol, MIRConst &elemConst, size_t idx);
    void EmitConstantTable(const MIRSymbol &mirSymbol, MIRConst &mirConst,
                           const std::map<GStrIdx, MIRType *> &strIdx2Type);
    void EmitClassInfoSequential(const MIRSymbol &mirSymbol, const std::map<GStrIdx, MIRType *> &strIdx2Type,
                                 const std::string &sectionName);
    void EmitMethodFieldSequential(const MIRSymbol &mirSymbol, const std::map<GStrIdx, MIRType *> &strIdx2Type,
                                   const std::string &sectionName);
    void EmitLiterals(std::vector<std::pair<MIRSymbol *, bool>> &literals,
                      const std::map<GStrIdx, MIRType *> &strIdx2Type);
    void EmitFuncLayoutInfo(const MIRSymbol &layout);
    std::string GetLayoutTypeString(uint32_t type);
    void EmitGlobalVars(std::vector<std::pair<MIRSymbol *, bool>> &globalVars);
    void EmitGlobalVar(const MIRSymbol &globalVar);
    void EmitStaticFields(const std::vector<MIRSymbol *> &fields);
    void EmitLiteral(const MIRSymbol &literal, const std::map<GStrIdx, MIRType *> &strIdx2Type);
    void EmitStringPointers();
    void GetHotAndColdMetaSymbolInfo(const std::vector<MIRSymbol *> &mirSymbolVec,
                                     std::vector<MIRSymbol *> &hotFieldInfoSymbolVec,
                                     std::vector<MIRSymbol *> &coldFieldInfoSymbolVec, const std::string &prefixStr,
                                     bool forceCold = false);
    void EmitMetaDataSymbolWithMarkFlag(const std::vector<MIRSymbol *> &mirSymbolVec,
                                        const std::map<GStrIdx, MIRType *> &strIdx2Type, const std::string &prefixStr,
                                        const std::string &sectionName, bool isHotFlag);
    void EmitMethodDeclaringClass(const MIRSymbol &mirSymbol, const std::string &sectionName);
    void MarkVtabOrItabEndFlag(const std::vector<MIRSymbol *> &mirSymbolVec);
    void EmitArrayConstant(MIRConst &mirConst);
    void EmitLocalVariable(const CGFunc &cgFunc);
    void EmitUninitializedSymbolsWithPrefixSection(const MIRSymbol &symbol, const std::string &sectionName);
    void EmitGlobalVariable();
    void EmitGlobalRootList(const MIRSymbol &mirSymbol);
    void EmitMuidTable(const std::vector<MIRSymbol *> &vec, const std::map<GStrIdx, MIRType *> &strIdx2Type,
                       const std::string &sectionName);
    MIRAddroffuncConst *GetAddroffuncConst(const MIRSymbol &mirSymbol, MIRAggConst &aggConst);
    int64 GetFieldOffsetValue(const std::string &className, const MIRIntConst &intConst,
                              const std::map<GStrIdx, MIRType *> &strIdx2Type);
    void WriteDebugCommentToFile();

    Emitter &Emit(int64 val)
    {
        outStream << val;
        return *this;
    }

    Emitter &Emit(const IntVal &val)
    {
        outStream << val.GetExtValue();
        return *this;
    }

    Emitter &Emit(const MapleString &str)
    {
        DEBUG_ASSERT(str.c_str() != nullptr, "nullptr check");
        outStream << str;
        return *this;
    }

    Emitter &Emit(const std::string &str)
    {
        outStream << str;
        return *this;
    }

    Emitter &Emit(const void *data, size_t size)
    {
        outStream.write(reinterpret_cast<const char *>(data), size);
        return *this;
    }

    void SetFileOffset(uint64 offset)
    {
        outStream.seekp(offset);
    }

    void EmitLabelRef(LabelIdx labIdx);
    void EmitStmtLabel(LabelIdx labIdx);
    void EmitLabelForFunc(const MIRFunction *func, LabelIdx labIdx);

    /* Emit signed/unsigned integer literals in decimal or hexadecimal */
    void EmitDecSigned(int64 num);
    void EmitDecUnsigned(uint64 num);
    void EmitHexUnsigned(uint64 num);

#if 1 /* REQUIRE TO SEPERATE TARGAARCH64 TARGARM32 */
      /* Following code is under TARGAARCH64 condition */
    void EmitHugeSoRoutines(bool lastRoutine = false);
    void EmitInlineAsmSection();

    uint64 GetFuncInsnCount() const
    {
        return funcInsnCount;
    }

    MapleMap<uint32_t, std::string> &GetFileMap()
    {
        return fileMap;
    }

    void SetFileMapValue(uint32_t n, const std::string &file)
    {
        fileMap[n] = file;
    }

    CG *GetCG() const
    {
        return cg;
    }

    void ClearFuncInsnCount()
    {
        funcInsnCount = 0;
    }

    bool NeedToDealWithHugeSo() const
    {
        return soInsnCount > kHugeSoInsnCountThreshold;
    }

    std::string HugeSoPostFix() const
    {
        return std::string(kHugeSoPostFix) + std::to_string(hugeSoSeqence);
    }

    void InsertHugeSoTarget(const std::string &target)
    {
        (void)hugeSoTargets.insert(target);
    }
#endif

protected:
    Emitter(CG &cg, const std::string &fileName)
        : cg(&cg),
          rangeIdx2PrefixStr(cg.GetMIRModule()->GetMPAllocator().Adapter()),
          stringPtr(cg.GetMIRModule()->GetMPAllocator().Adapter()),
          localStrPtr(cg.GetMIRModule()->GetMPAllocator().Adapter()),
          hugeSoTargets(cg.GetMIRModule()->GetMPAllocator().Adapter()),
          fileMap(std::less<uint32_t>(), cg.GetMIRModule()->GetMPAllocator().Adapter())
    {
        MIRModule &mirModule = *cg.GetMIRModule();
        memPool = mirModule.GetMemPool();
        asmInfo = memPool->New<AsmInfo>(*memPool);
    }

    ~Emitter() = default;

protected:
    std::ostringstream outStream;
    std::ofstream fileStream;
    MemPool *memPool;
    CG *cg;

private:
    AsmLabel GetTypeAsmInfoName(PrimType primType) const;
    void EmitDWRef(const std::string &name);
    void InitRangeIdx2PerfixStr();
    void EmitAddressString(const std::string &address);
    void EmitAliasAndRef(const MIRSymbol &sym); /* handle function symbol which has alias and weak ref */

    MOperator currentMop = UINT_MAX;
    MapleUnorderedMap<int, std::string> rangeIdx2PrefixStr;
    const AsmInfo *asmInfo;
#ifdef ARK_LITECG_DEBUG
    uint32 arraySize = 0;
    bool isFlexibleArray = false;
#endif
    MapleSet<UStrIdx> stringPtr;
    MapleVector<UStrIdx> localStrPtr;
#if 1 /* REQUIRE TO SEPERATE TARGAARCH64 TARGARM32 */
      /* Following code is under TARGAARCH64 condition */
    uint64 soInsnCount = 0;
    uint64 funcInsnCount = 0;
    MapleSet<std::string> hugeSoTargets;
    uint32 hugeSoSeqence = 2;
#endif
    MapleMap<uint32_t, std::string> fileMap;
};

class OpndEmitVisitor : public OperandVisitorBase,
                        public OperandVisitors<RegOperand, ImmOperand, MemOperand, OfstOperand, ListOperand,
                                               LabelOperand, FuncNameOperand, StImmOperand, CondOperand,
                                               BitShiftOperand, ExtendShiftOperand, CommentOperand> {
public:
    explicit OpndEmitVisitor(Emitter &asmEmitter) : emitter(asmEmitter) {}
    virtual ~OpndEmitVisitor() = default;

protected:
    Emitter &emitter;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_EMIT_H */
