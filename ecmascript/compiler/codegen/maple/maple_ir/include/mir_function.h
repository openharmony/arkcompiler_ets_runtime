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

#ifndef MAPLE_IR_INCLUDE_MIR_FUNCTION_H
#define MAPLE_IR_INCLUDE_MIR_FUNCTION_H
#include <string>
#include "mir_module.h"
#include "mir_const.h"
#include "mir_symbol.h"
#include "mir_preg.h"
#include "intrinsics.h"
#include "mir_nodes.h"
#include "mir_type.h"
#include "func_desc.h"

#define DEBUGME true

namespace maple {
enum PointerAttr : uint32_t { kPointerUndeiced = 0x1, kPointerNull = 0x2, kPointerNoNull = 0x3 };

struct SaveInfo {
    int offset;
    bool shouldSave;
    int idx;
};

enum FuncAttrProp : uint32_t {
    kNoThrowException = 0x1,
    kNoRetNewlyAllocObj = 0x2,
    kNoDefEffect = 0x4,
    kNoDefArgEffect = 0x8,
    kPureFunc = 0x10,
    kIpaSeen = 0x20,
    kUseEffect = 0x40,
    kDefEffect = 0x80
};

// file-layout is shared between maple compiler and runtime, thus not in namespace maplert
enum LayoutType : uint8_t {
    kLayoutBootHot,
    kLayoutBothHot,
    kLayoutRunHot,
    kLayoutStartupOnly,
    kLayoutUsedOnce,
    kLayoutExecuted,  // method excuted in some condition
    kLayoutUnused,
    kLayoutTypeCount
};
// this used for c string layout
static constexpr uint8_t kCStringShift = 1;

// describe a formal definition in a function declaration
class FormalDef {
public:
    GStrIdx formalStrIdx = GStrIdx(0);  // used when processing the prototype
    MIRSymbol *formalSym = nullptr;     // used in the function definition
    TyIdx formalTyIdx = TyIdx();
    TypeAttrs formalAttrs = TypeAttrs();  // the formal's type attributes

    FormalDef() {};
    virtual ~FormalDef() {}
    FormalDef(MIRSymbol *s, const TyIdx &tidx, const TypeAttrs &at) : formalSym(s), formalTyIdx(tidx), formalAttrs(at)
    {
    }
    FormalDef(const GStrIdx &sidx, MIRSymbol *s, const TyIdx &tidx, const TypeAttrs &at)
        : formalStrIdx(sidx), formalSym(s), formalTyIdx(tidx), formalAttrs(at)
    {
    }
};

class MeFunction;         // circular dependency exists, no other choice
class EAConnectionGraph;  // circular dependency exists, no other choice
class MemReferenceTable;
class MIRFunction {
public:
    MIRFunction(MIRModule *mod, StIdx idx) : module(mod), symbolTableIdx(idx) {}

    ~MIRFunction() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(bool withoutBody = false);
    void DumpUpFormal(int32 indent) const;
    void DumpFrame(int32 indent) const;
    void DumpFuncBody(int32 indent);
#endif

    const MIRSymbol *GetFuncSymbol() const;
    MIRSymbol *GetFuncSymbol();

    void SetMemPool(MemPool *memPool)
    {
        SetCodeMemPool(memPool);
        codeMemPoolAllocator.SetMemPool(codeMemPool);
    }

    const std::string &GetName() const;

#ifdef ARK_LITECG_DEBUG
    GStrIdx GetNameStrIdx() const;
#endif

    const std::string &GetBaseClassName() const;

    const std::string &GetBaseFuncName() const;

    GStrIdx GetBaseClassNameStrIdx() const
    {
        return baseClassStrIdx;
    }

    GStrIdx GetBaseFuncNameStrIdx() const
    {
        return baseFuncStrIdx;
    }

    GStrIdx GetBaseFuncNameWithTypeStrIdx() const
    {
        return baseFuncWithTypeStrIdx;
    }

    GStrIdx GetBaseFuncSigStrIdx() const
    {
        return baseFuncSigStrIdx;
    }

    void SetBaseClassNameStrIdx(GStrIdx id)
    {
        baseClassStrIdx = id;
    }

    void SetBaseFuncNameStrIdx(GStrIdx id)
    {
        baseFuncStrIdx = id;
    }

    void SetBaseFuncNameWithTypeStrIdx(GStrIdx id)
    {
        baseFuncWithTypeStrIdx = id;
    }

    const MIRType *GetReturnType() const;
    MIRType *GetReturnType();
    bool IsReturnVoid() const
    {
        return GetReturnType()->GetPrimType() == PTY_void;
    }
    TyIdx GetReturnTyIdx() const
    {
        CHECK_FATAL(funcType != nullptr, "funcType is nullptr");
        return funcType->GetRetTyIdx();
    }
    void SetReturnTyIdx(TyIdx tyidx)
    {
        CHECK_FATAL(funcType != nullptr, "funcType is nullptr");
        funcType->SetRetTyIdx(tyidx);
    }

    void SetFrameTypeInfo(int offset, bool shouldSave, int idx)
    {
        frameTypeInfo.offset = offset;
        frameTypeInfo.shouldSave = shouldSave;
        frameTypeInfo.idx = idx;
    }

    void SetFuncInfo(int offset, bool shouldSave, int idx)
    {
        funcIdxInfo.offset = offset;
        funcIdxInfo.shouldSave = shouldSave;
        funcIdxInfo.idx = idx;
    }

    SaveInfo &GetFrameTypeInfo()
    {
        return frameTypeInfo;
    }

    SaveInfo &GetFuncInfo()
    {
        return funcIdxInfo;
    }

    TyIdx GetClassTyIdx() const
    {
        return classTyIdx;
    }
    void SetClassTyIdx(TyIdx tyIdx)
    {
        classTyIdx = tyIdx;
    }
    void SetClassTyIdx(uint32 idx)
    {
        classTyIdx.reset(idx);
    }

    void AddArgument(MIRSymbol *st)
    {
        DEBUG_ASSERT(st != nullptr, "null ptr check");
        FormalDef formalDef(st->GetNameStrIdx(), st, st->GetTyIdx(), st->GetAttrs());
        formalDefVec.push_back(formalDef);
    }

    void AddFormalDef(const FormalDef &formalDef)
    {
        formalDefVec.push_back(formalDef);
    }

    size_t GetParamSize() const
    {
        CHECK_FATAL(funcType != nullptr, "funcType is nullptr");
        return funcType->GetParamTypeList().size();
    }

    auto &GetParamTypes() const
    {
        CHECK_FATAL(funcType != nullptr, "funcType is nullptr");
        return funcType->GetParamTypeList();
    }

    TyIdx GetNthParamTyIdx(size_t i) const
    {
        DEBUG_ASSERT(i < funcType->GetParamTypeList().size(), "array index out of range");
        return funcType->GetParamTypeList()[i];
    }

    const MIRType *GetNthParamType(size_t i) const;
    MIRType *GetNthParamType(size_t i);

    const TypeAttrs &GetNthParamAttr(size_t i) const
    {
        DEBUG_ASSERT(i < formalDefVec.size(), "array index out of range");
        DEBUG_ASSERT(formalDefVec[i].formalSym != nullptr, "null ptr check");
        return formalDefVec[i].formalSym->GetAttrs();
    }
    void UpdateFuncTypeAndFormalsAndReturnType(const std::vector<MIRSymbol *> &symbols, const TyIdx &retTyIdx,
                                               bool clearOldArgs = false);
    LabelIdx GetOrCreateLableIdxFromName(const std::string &name);
    GStrIdx GetLabelStringIndex(LabelIdx labelIdx) const
    {
        CHECK_FATAL(labelTab != nullptr, "labelTab is nullptr");
        DEBUG_ASSERT(labelIdx < labelTab->Size(), "index out of range in GetLabelStringIndex");
        return labelTab->GetSymbolFromStIdx(labelIdx);
    }
    const std::string &GetLabelName(LabelIdx labelIdx) const
    {
        GStrIdx strIdx = GetLabelStringIndex(labelIdx);
        return GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx);
    }

    const MIRSymbol *GetLocalOrGlobalSymbol(const StIdx &idx, bool checkFirst = false) const;
    MIRSymbol *GetLocalOrGlobalSymbol(const StIdx &idx, bool checkFirst = false);

    const FuncAttrs &GetAttrs() const
    {
        return funcAttrs;
    }

    void SetAttrs(FuncAttrs attr)
    {
        funcAttrs = attr;
    }

    bool GetAttr(FuncAttrKind attrKind) const
    {
        return funcAttrs.GetAttr(attrKind);
    }

    void SetAttr(FuncAttrKind attrKind)
    {
        funcAttrs.SetAttr(attrKind);
    }

    void UnSetAttr(FuncAttrKind attrKind)
    {
        funcAttrs.SetAttr(attrKind, true);
    }

    bool IsVarargs() const
    {
        return funcAttrs.GetAttr(FUNCATTR_varargs);
    }

    bool IsWeak() const
    {
        return funcAttrs.GetAttr(FUNCATTR_weak);
    }

    bool IsStatic() const
    {
        return funcAttrs.GetAttr(FUNCATTR_static);
    }

    bool IsInline() const
    {
        return funcAttrs.GetAttr(FUNCATTR_inline);
    }

    bool IsExtern() const
    {
        return funcAttrs.GetAttr(FUNCATTR_extern);
    }

    bool IsNative() const
    {
        return funcAttrs.GetAttr(FUNCATTR_native);
    }

    bool IsFinal() const
    {
        return funcAttrs.GetAttr(FUNCATTR_final);
    }

    bool IsAbstract() const
    {
        return funcAttrs.GetAttr(FUNCATTR_abstract);
    }

    bool IsPublic() const
    {
        return funcAttrs.GetAttr(FUNCATTR_public);
    }

    bool IsPrivate() const
    {
        return funcAttrs.GetAttr(FUNCATTR_private);
    }

    bool IsProtected() const
    {
        return funcAttrs.GetAttr(FUNCATTR_protected);
    }

    bool IsConstructor() const
    {
        return funcAttrs.GetAttr(FUNCATTR_constructor);
    }

    bool IsLocal() const
    {
        return funcAttrs.GetAttr(FUNCATTR_local);
    }

    bool IsNoDefArgEffect() const
    {
        return funcAttrs.GetAttr(FUNCATTR_nodefargeffect);
    }

    bool IsNoDefEffect() const
    {
        return funcAttrs.GetAttr(FUNCATTR_nodefeffect);
    }

    bool IsNoRetGlobal() const
    {
        return funcAttrs.GetAttr(FUNCATTR_noretglobal);
    }

    bool IsNoThrowException() const
    {
        return funcAttrs.GetAttr(FUNCATTR_nothrow_exception);
    }

    bool IsNoRetArg() const
    {
        return funcAttrs.GetAttr(FUNCATTR_noretarg);
    }

    bool IsNoPrivateDefEffect() const
    {
        return funcAttrs.GetAttr(FUNCATTR_noprivate_defeffect);
    }

    bool IsIpaSeen() const
    {
        return funcAttrs.GetAttr(FUNCATTR_ipaseen);
    }

    bool IsPure() const
    {
        return funcAttrs.GetAttr(FUNCATTR_pure);
    }

    bool IsFirstArgReturn() const
    {
        return funcAttrs.GetAttr(FUNCATTR_firstarg_return);
    }

    bool IsUnSafe() const
    {
        return !funcAttrs.GetAttr(FUNCATTR_safed) || funcAttrs.GetAttr(FUNCATTR_unsafed);
    }

    bool IsSafe() const
    {
        return funcAttrs.GetAttr(FUNCATTR_safed);
    }

    void SetVarArgs()
    {
        funcAttrs.SetAttr(FUNCATTR_varargs);
    }

    void SetNoDefArgEffect()
    {
        funcAttrs.SetAttr(FUNCATTR_nodefargeffect);
    }

    void SetNoDefEffect()
    {
        funcAttrs.SetAttr(FUNCATTR_nodefeffect);
    }

    void SetNoRetGlobal()
    {
        funcAttrs.SetAttr(FUNCATTR_noretglobal);
    }

    void SetNoThrowException()
    {
        funcAttrs.SetAttr(FUNCATTR_nothrow_exception);
    }

    void SetNoRetArg()
    {
        funcAttrs.SetAttr(FUNCATTR_noretarg);
    }

    void SetNoPrivateDefEffect()
    {
        funcAttrs.SetAttr(FUNCATTR_noprivate_defeffect);
    }

    void SetIpaSeen()
    {
        funcAttrs.SetAttr(FUNCATTR_ipaseen);
    }

    void SetPure()
    {
        funcAttrs.SetAttr(FUNCATTR_pure);
    }

    void SetFirstArgReturn()
    {
        funcAttrs.SetAttr(FUNCATTR_firstarg_return);
    }

    void UnsetNoDefArgEffect()
    {
        funcAttrs.SetAttr(FUNCATTR_nodefargeffect, true);
    }

    void UnsetNoDefEffect()
    {
        funcAttrs.SetAttr(FUNCATTR_nodefeffect, true);
    }

    void UnsetNoRetGlobal()
    {
        funcAttrs.SetAttr(FUNCATTR_noretglobal, true);
    }

    void UnsetNoThrowException()
    {
        funcAttrs.SetAttr(FUNCATTR_nothrow_exception, true);
    }

    void UnsetPure()
    {
        funcAttrs.SetAttr(FUNCATTR_pure, true);
    }

    void UnsetNoRetArg()
    {
        funcAttrs.SetAttr(FUNCATTR_noretarg, true);
    }

    void UnsetNoPrivateDefEffect()
    {
        funcAttrs.SetAttr(FUNCATTR_noprivate_defeffect, true);
    }

    bool HasCall() const;
    void SetHasCall();

#ifdef ARK_LITECG_DEBUG
    bool IsInfoPrinted() const;
    void SetInfoPrinted();
    void ResetInfoPrinted();
#endif
    bool IsAFormal(const MIRSymbol *st) const
    {
        for (const auto &formalDef : formalDefVec) {
            if (st == formalDef.formalSym) {
                return true;
            }
        }
        return false;
    }

    uint32 GetFormalIndex(const MIRSymbol *symbol) const
    {
        for (size_t i = 0; i < formalDefVec.size(); ++i) {
            if (formalDefVec[i].formalSym == symbol) {
                return i;
            }
        }
        return 0xffffffff;
    }

    FormalDef &GetFormalDefFromMIRSymbol(const MIRSymbol *symbol)
    {
        for (auto &formalDef : formalDefVec) {
            if (formalDef.formalSym == symbol) {
                return formalDef;
            }
        }
        CHECK_FATAL(false, "Impossible.");
    }

    bool IsAFormalName(const GStrIdx idx) const
    {
        for (const auto &formalDef : formalDefVec) {
            if (idx == formalDef.formalStrIdx) {
                return true;
            }
        }
        return false;
    }

    const FormalDef GetFormalFromName(const GStrIdx idx) const
    {
        for (size_t i = 0; i < formalDefVec.size(); ++i) {
            if (formalDefVec[i].formalStrIdx == idx) {
                return formalDefVec[i];
            }
        }
        return FormalDef();
    }

    void ReleaseMemory()
    {
        if (codeMemPoolTmp != nullptr) {
            delete codeMemPoolTmp;
            codeMemPoolTmp = nullptr;
        }
    }

    void ReleaseCodeMemory()
    {
        if (codeMemPool != nullptr) {
            codeMemPoolAllocator.SetMemPool(nullptr);
            delete codeMemPool;
            codeMemPool = nullptr;
            SetMemPool(nullptr);
        }
    }

    MemPool *GetCodeMempool()
    {
        if (useTmpMemPool) {
            if (codeMemPoolTmp == nullptr) {
                codeMemPoolTmp = new ThreadLocalMemPool(memPoolCtrler, "func code mempool");
                codeMemPoolTmpAllocator.SetMemPool(codeMemPoolTmp);
            }
            return codeMemPoolTmp;
        }
        if (codeMemPool == nullptr) {
            codeMemPool = new ThreadLocalMemPool(memPoolCtrler, "func code mempool");
            codeMemPoolAllocator.SetMemPool(codeMemPool);
        }
        return codeMemPool;
    }

    MapleAllocator &GetCodeMemPoolAllocator()
    {
        GetCodeMempool();
        if (useTmpMemPool) {
            return codeMemPoolTmpAllocator;
        }
        return codeMemPoolAllocator;
    }

    MapleAllocator &GetCodeMempoolAllocator()
    {
        if (codeMemPool == nullptr) {
            codeMemPool = new ThreadLocalMemPool(memPoolCtrler, "func code mempool");
            codeMemPoolAllocator.SetMemPool(codeMemPool);
        }
        return codeMemPoolAllocator;
    }

    void EnterFormals();
    void NewBody();

    MIRModule *GetModule()
    {
        return module;
    }

    PUIdx GetPuidx() const
    {
        return puIdx;
    }
    void SetPuidx(PUIdx idx)
    {
        puIdx = idx;
    }

    PUIdx GetPuidxOrigin() const
    {
        return puIdxOrigin;
    }
    void SetPuidxOrigin(PUIdx idx)
    {
        puIdxOrigin = idx;
    }

    StIdx GetStIdx() const
    {
        return symbolTableIdx;
    }
    void SetStIdx(StIdx stIdx)
    {
        symbolTableIdx = stIdx;
    }

    int32 GetSCCId() const
    {
        return sccID;
    }
    void SetSCCId(int32 id)
    {
        sccID = id;
    }

    MIRFuncType *GetMIRFuncType()
    {
        return funcType;
    }
    void SetMIRFuncType(MIRFuncType *type)
    {
        funcType = type;
    }

    TyIdx GetInferredReturnTyIdx() const
    {
        return inferredReturnTyIdx;
    }

    void SetInferredReturnTyIdx(TyIdx tyIdx)
    {
        inferredReturnTyIdx = tyIdx;
    }

    MIRTypeNameTable *GetTypeNameTab() const
    {
        return typeNameTab;
    }

    void AllocTypeNameTab()
    {
        if (typeNameTab == nullptr) {
            typeNameTab = module->GetMemPool()->New<MIRTypeNameTable>(module->GetMPAllocator());
        }
    }
    bool HaveTypeNameTab() const
    {
        return typeNameTab != nullptr;
    }
    const MapleMap<GStrIdx, TyIdx> &GetGStrIdxToTyIdxMap() const
    {
        CHECK_FATAL(typeNameTab != nullptr, "typeNameTab is nullptr");
        return typeNameTab->GetGStrIdxToTyIdxMap();
    }
    TyIdx GetTyIdxFromGStrIdx(GStrIdx idx) const
    {
        CHECK_FATAL(typeNameTab != nullptr, "typeNameTab is nullptr");
        return typeNameTab->GetTyIdxFromGStrIdx(idx);
    }
    void SetGStrIdxToTyIdx(GStrIdx gStrIdx, TyIdx tyIdx)
    {
        CHECK_FATAL(typeNameTab != nullptr, "typeNameTab is nullptr");
        typeNameTab->SetGStrIdxToTyIdx(gStrIdx, tyIdx);
    }

    const std::string &GetLabelTabItem(LabelIdx labelIdx) const
    {
        CHECK_FATAL(labelTab != nullptr, "labelTab is nullptr");
        return labelTab->GetName(labelIdx);
    }

    void AllocLabelTab()
    {
        if (labelTab == nullptr) {
            labelTab = module->GetMemPool()->New<MIRLabelTable>(module->GetMPAllocator());
        }
    }

    MIRPregTable *GetPregTab() const
    {
        return pregTab;
    }

    void SetPregTab(MIRPregTable *tab)
    {
        pregTab = tab;
    }
    void AllocPregTab()
    {
        if (pregTab == nullptr) {
            pregTab = module->GetMemPool()->New<MIRPregTable>(&module->GetMPAllocator());
        }
    }
    MIRPreg *GetPregItem(PregIdx idx)
    {
        return const_cast<MIRPreg *>(const_cast<const MIRFunction *>(this)->GetPregItem(idx));
    }
    const MIRPreg *GetPregItem(PregIdx idx) const
    {
        return pregTab->PregFromPregIdx(idx);
    }

    BlockNode *GetBody()
    {
        return body;
    }
    const BlockNode *GetBody() const
    {
        return body;
    }
    void SetBody(BlockNode *node)
    {
        body = node;
    }

    BlockNode *GetLastPosBody()
    {
        return bodyLast;
    }
    const BlockNode *GetLastPosBody() const
    {
        return bodyLast;
    }
    void SetLastPosBody(BlockNode *node)
    {
        bodyLast = node;
    }

    SrcPosition &GetSrcPosition()
    {
        DEBUG_ASSERT(GetFuncSymbol() != nullptr, "null ptr check");
        return GetFuncSymbol()->GetSrcPosition();
    }

    void SetSrcPosition(const SrcPosition &position)
    {
        DEBUG_ASSERT(GetFuncSymbol() != nullptr, "null ptr check");
        GetFuncSymbol()->SetSrcPosition(position);
    }

    const FuncAttrs &GetFuncAttrs() const
    {
        return funcAttrs;
    }
    FuncAttrs &GetFuncAttrs()
    {
        return funcAttrs;
    }

    void SetFuncAttrs(const FuncAttrs &attrs)
    {
        funcAttrs = attrs;
    }
    void SetFuncAttrs(uint64 attrFlag)
    {
        funcAttrs.SetAttrFlag(attrFlag);
    }

    uint32 GetFlag() const
    {
        return flag;
    }
    void SetFlag(uint32 newFlag)
    {
        flag = newFlag;
    }

    uint16 GetHashCode() const
    {
        return hashCode;
    }
    void SetHashCode(uint16 newHashCode)
    {
        hashCode = newHashCode;
    }

    void SetFileIndex(uint32 newFileIndex)
    {
        fileIndex = newFileIndex;
    }

    MIRInfoVector &GetInfoVector()
    {
        return info;
    }

    const MIRInfoPair &GetInfoPair(size_t i) const
    {
        return info.at(i);
    }

    void PushbackMIRInfo(const MIRInfoPair &pair)
    {
        info.push_back(pair);
    }

    void SetMIRInfoNum(size_t idx, uint32 num)
    {
        info[idx].second = num;
    }

    MapleVector<bool> &InfoIsString()
    {
        return infoIsString;
    }

    void PushbackIsString(bool isString)
    {
        infoIsString.push_back(isString);
    }

    bool HasVlaOrAlloca() const
    {
        return hasVlaOrAlloca;
    }
    void SetVlaOrAlloca(bool has)
    {
        hasVlaOrAlloca = has;
    }

    // Default freq is the lastStmtFreq
    bool HasFreqMap() const
    {
        return freqLastMap != nullptr;
    }

    bool HasFirstFreqMap() const
    {
        return freqFirstMap != nullptr;
    }

    const MapleMap<uint32, uint32> &GetFirstFreqMap() const
    {
        return *freqFirstMap;
    }

    void SetFirstFreqMap(uint32 stmtID, uint32 freq)
    {
        if (freqFirstMap == nullptr) {
            freqFirstMap = module->GetMemPool()->New<MapleMap<uint32, uint32>>(module->GetMPAllocator().Adapter());
        }
        (*freqFirstMap)[stmtID] = freq;
    }

    const MapleMap<uint32, uint32> &GetLastFreqMap() const
    {
        return *freqLastMap;
    }

    int32 GetFreqFromLastStmt(uint32 stmtId)
    {
        if (freqLastMap == nullptr) {
            return -1;
        }
        if ((*freqLastMap).find(stmtId) == (*freqLastMap).end()) {
            return -1;
        }
        return static_cast<int32>((*freqLastMap)[stmtId]);
    }

    int32 GetFreqFromFirstStmt(uint32 stmtId)
    {
        if (freqFirstMap == nullptr) {
            return -1;
        }
        if ((*freqFirstMap).find(stmtId) == (*freqFirstMap).end()) {
            return -1;
        }
        return static_cast<int32>((*freqFirstMap)[stmtId]);
    }

    void SetLastFreqMap(uint32 stmtID, uint32 freq)
    {
        if (freqLastMap == nullptr) {
            freqLastMap = module->GetMemPool()->New<MapleMap<uint32, uint32>>(module->GetMPAllocator().Adapter());
        }
        (*freqLastMap)[stmtID] = freq;
    }

    bool WithLocInfo() const
    {
        return withLocInfo;
    }
    void SetWithLocInfo(bool withInfo)
    {
        withLocInfo = withInfo;
    }

    bool IsDirty() const
    {
        return isDirty;
    }
    void SetDirty(bool dirty)
    {
        isDirty = dirty;
    }

    bool IsFromMpltInline() const
    {
        return fromMpltInline;
    }
    void SetFromMpltInline(bool isInline)
    {
        fromMpltInline = isInline;
    }

    uint8 GetLayoutType() const
    {
        return layoutType;
    }
    void SetLayoutType(uint8 type)
    {
        layoutType = type;
    }

    uint32 GetCallTimes() const
    {
        return callTimes;
    }
    void SetCallTimes(uint32 times)
    {
        callTimes = times;
    }

    uint32 GetFrameSize() const
    {
        return frameSize;
    }
    void SetFrameSize(uint32 size)
    {
        frameSize = size;
    }

    uint32 GetUpFormalSize() const
    {
        return upFormalSize;
    }
    void SetUpFormalSize(uint32 size)
    {
        upFormalSize = size;
    }

    uint32 GetOutParmSize() const
    {
        return outParmSize;
    }
    void SetOutParmSize(uint32 size)
    {
        outParmSize = size;
    }

    uint16 GetModuleId() const
    {
        return moduleID;
    }
    void SetModuleID(uint16 id)
    {
        moduleID = id;
    }

    uint32 GetFuncSize() const
    {
        return funcSize;
    }
    void SetFuncSize(uint32 size)
    {
        funcSize = size;
    }

    uint32 GetTempCount() const
    {
        return tempCount;
    }
    void IncTempCount()
    {
        ++tempCount;
    }

    uint8 *GetFormalWordsTypeTagged() const
    {
        return formalWordsTypeTagged;
    }
    void SetFormalWordsTypeTagged(uint8 *tagged)
    {
        formalWordsTypeTagged = tagged;
    }
    uint8 **GetFwtAddress()
    {
        return &formalWordsTypeTagged;
    }

    uint8 *GetLocalWordsTypeTagged() const
    {
        return localWordsTypeTagged;
    }
    void SetLocalWordsTypeTagged(uint8 *tagged)
    {
        localWordsTypeTagged = tagged;
    }
    uint8 **GetLwtAddress()
    {
        return &localWordsTypeTagged;
    }

    uint8 *GetFormalWordsRefCounted() const
    {
        return formalWordsRefCounted;
    }
    void SetFormalWordsRefCounted(uint8 *counted)
    {
        formalWordsRefCounted = counted;
    }
    uint8 **GetFwrAddress()
    {
        return &formalWordsRefCounted;
    }

    uint8 *GetLocalWordsRefCounted() const
    {
        return localWordsRefCounted;
    }
    void SetLocalWordsRefCounted(uint8 *counted)
    {
        localWordsRefCounted = counted;
    }

    MeFunction *GetMeFunc()
    {
        return meFunc;
    }

    void SetMeFunc(MeFunction *func)
    {
        meFunc = func;
    }

    EAConnectionGraph *GetEACG()
    {
        return eacg;
    }
    void SetEACG(EAConnectionGraph *eacgVal)
    {
        eacg = eacgVal;
    }

    void SetFormalDefVec(const MapleVector<FormalDef> &currFormals)
    {
        formalDefVec = currFormals;
    }

    MapleVector<FormalDef> &GetFormalDefVec()
    {
        return formalDefVec;
    }

    const FormalDef &GetFormalDefAt(size_t i) const
    {
        return formalDefVec[i];
    }

    FormalDef &GetFormalDefAt(size_t i)
    {
        return formalDefVec[i];
    }

    const MIRSymbol *GetFormal(size_t i) const
    {
        return formalDefVec[i].formalSym;
    }

    MIRSymbol *GetFormal(size_t i)
    {
        return formalDefVec[i].formalSym;
    }

    const std::string &GetFormalName(size_t i) const
    {
        auto *formal = formalDefVec[i].formalSym;
        if (formal != nullptr) {
            return formal->GetName();
        }
        return GlobalTables::GetStrTable().GetStringFromStrIdx(formalDefVec[i].formalStrIdx);
    }

    size_t GetFormalCount() const
    {
        return formalDefVec.size();
    }

    void ClearFormals()
    {
        formalDefVec.clear();
    }

    void ClearArguments()
    {
        formalDefVec.clear();
        funcType->GetParamTypeList().clear();
        funcType->GetParamAttrsList().clear();
    }

    size_t GetSymbolTabSize() const
    {
        DEBUG_ASSERT(symTab != nullptr, "symTab is nullptr");
        return symTab->GetSymbolTableSize();
    }
    MIRSymbol *GetSymbolTabItem(uint32 idx, bool checkFirst = false) const
    {
        return symTab->GetSymbolFromStIdx(idx, checkFirst);
    }
    const MIRSymbolTable *GetSymTab() const
    {
        return symTab;
    }
    MIRSymbolTable *GetSymTab()
    {
        return symTab;
    }
    void AllocSymTab()
    {
        if (symTab == nullptr) {
            symTab = module->GetMemPool()->New<MIRSymbolTable>(module->GetMPAllocator());
        }
    }
    MIRLabelTable *GetLabelTab() const
    {
        CHECK_FATAL(labelTab != nullptr, "must be");
        return labelTab;
    }
    MIRLabelTable *GetLabelTab()
    {
        if (labelTab == nullptr) {
            labelTab = module->GetMemPool()->New<MIRLabelTable>(module->GetMPAllocator());
        }
        return labelTab;
    }
    void SetLabelTab(MIRLabelTable *currLabelTab)
    {
        labelTab = currLabelTab;
    }

    const MapleSet<MIRSymbol *> &GetRetRefSym() const
    {
        return retRefSym;
    }
    void InsertMIRSymbol(MIRSymbol *sym)
    {
        (void)retRefSym.insert(sym);
    }

    MemPool *GetDataMemPool() const
    {
        return module->GetMemPool();
    }

    MemPool *GetCodeMemPool()
    {
        if (codeMemPool == nullptr) {
            codeMemPool = new ThreadLocalMemPool(memPoolCtrler, "func code mempool");
            codeMemPoolAllocator.SetMemPool(codeMemPool);
        }
        return codeMemPool;
    }

    void SetCodeMemPool(MemPool *currCodeMemPool)
    {
        codeMemPool = currCodeMemPool;
    }

    MapleAllocator &GetCodeMPAllocator()
    {
        GetCodeMemPool();
        return codeMemPoolAllocator;
    }

    void AddFuncGenericDeclare(GenericDeclare *g)
    {
        genericDeclare.push_back(g);
    }

    void AddFuncGenericArg(AnnotationType *a)
    {
        genericArg.push_back(a);
    }

    void AddFuncGenericRet(AnnotationType *r)
    {
        genericRet = r;
    }

    void AddFuncLocalGenericVar(const GStrIdx &str, AnnotationType *at)
    {
        genericLocalVar[str] = at;
    }

    MapleVector<GenericDeclare *> &GetFuncGenericDeclare()
    {
        return genericDeclare;
    }

    MapleVector<AnnotationType *> &GetFuncGenericArg()
    {
        return genericArg;
    }

    void SetRetrunAttrKind(const PointerAttr kind)
    {
        returnKind = kind;
    }

    PointerAttr GetRetrunAttrKind() const
    {
        return returnKind;
    }

    AnnotationType *GetFuncGenericRet()
    {
        return genericRet;
    }

    AnnotationType *GetFuncLocalGenericVar(const GStrIdx &str)
    {
        if (genericLocalVar.find(str) == genericLocalVar.end()) {
            return nullptr;
        }
        return genericLocalVar[str];
    }

    MemPool *GetCodeMemPoolTmp()
    {
        if (codeMemPoolTmp == nullptr) {
            codeMemPoolTmp = new ThreadLocalMemPool(memPoolCtrler, "func code mempool");
            codeMemPoolTmpAllocator.SetMemPool(codeMemPoolTmp);
        }
        return codeMemPoolTmp;
    }

    bool CheckParamNullType(MIRSymbol *sym)
    {
        return paramNonullTypeMap.find(sym) != paramNonullTypeMap.end();
    }

    PointerAttr GetParamNonull(MIRSymbol *sym)
    {
        return paramNonullTypeMap[sym];
    }

    void SetParamNonull(MIRSymbol *sym, PointerAttr type)
    {
        paramNonullTypeMap[sym] = type;
    }

    void CopyReferedRegs(std::set<uint32> regs)
    {
        for (auto reg : regs) {
            referedPregs.insert(reg);
        }
    }

    MapleSet<uint32> GetReferedRegs() const
    {
        return referedPregs;
    }

    void SetDerived2BaseRef(PregIdx deriveRef, PregIdx baseRef)
    {
        CHECK_FATAL(derived2BaseRef.find(deriveRef) == derived2BaseRef.end(), "derived2BaseRef double set");
        derived2BaseRef[deriveRef] = baseRef;
    }

    const MapleUnorderedMap<PregIdx, PregIdx> &GetDerived2BaseRef() const
    {
        return derived2BaseRef;
    }

    bool IsReferedRegsValid() const
    {
        return referedRegsValid;
    }

    void SetReferedRegsValid(bool val)
    {
        referedRegsValid = val;
    }

    FuncDesc &GetFuncDesc()
    {
        return funcDesc;
    }

    void SetFuncDesc(const FuncDesc &value)
    {
        funcDesc = value;
    }

    void InitFuncDescToBest()
    {
        funcDesc.InitToBest();
    }

    const FuncDesc &GetFuncDesc() const
    {
        return funcDesc;
    }

    bool IsVisited() const
    {
        return isVisited;
    }
    void SetIsVisited()
    {
        isVisited = true;
    }

    void SetFuncProfData(GcovFuncInfo *data)
    {
        funcProfData = data;
    }
    GcovFuncInfo *GetFuncProfData()
    {
        return funcProfData;
    }
    GcovFuncInfo *GetFuncProfData() const
    {
        return funcProfData;
    }
    void SetStmtFreq(uint32_t stmtID, uint64_t freq)
    {
        DEBUG_ASSERT((funcProfData != nullptr && freq > 0), "nullptr check");
        funcProfData->SetStmtFreq(stmtID, static_cast<int64_t>(freq));
    }

    void SetWithSrc(bool var)
    {
        withSrc = var;
    }

    bool GetWithSrc() const
    {
        return withSrc;
    }

    uint8 GetFrameReseverdSlot()
    {
        return funcAttrs.GetFrameResverdSlot();
    }

    MemReferenceTable *GetMemReferenceTable()
    {
        return memReferenceTable;
    }

    void DiscardMemReferenceTable()
    {
        memReferenceTable = nullptr;
    }

    void CreateMemReferenceTable();

    MemReferenceTable *GetOrCreateMemReferenceTable();

    const MapleString* AddNewDebugComment(const std::string& dbgComment)
    {
        return &debugComments.emplace_back(dbgComment, module->GetMPAllocator().GetMemPool());
    }

private:
    MIRModule *module;      // the module that owns this function
    PUIdx puIdx = 0;        // the PU index of this function
    PUIdx puIdxOrigin = 0;  // the original puIdx when initial generation
    StIdx symbolTableIdx;   // the symbol table index of this function
    int32 sccID = -1;       // the scc id of this function, for mplipa
    MIRFuncType *funcType = nullptr;
    TyIdx inferredReturnTyIdx {0};  // the actual return type of of this function (may be a
                                    // subclass of the above). 0 means can not be inferred.
    TyIdx classTyIdx {0};           // class/interface type this function belongs to
    MapleVector<FormalDef> formalDefVec {module->GetMPAllocator().Adapter()};  // the formals in function definition
    MapleSet<MIRSymbol *> retRefSym {module->GetMPAllocator().Adapter()};

    MapleVector<GenericDeclare *> genericDeclare {module->GetMPAllocator().Adapter()};
    MapleVector<AnnotationType *> genericArg {module->GetMPAllocator().Adapter()};
    MapleMap<GStrIdx, AnnotationType *> genericLocalVar {module->GetMPAllocator().Adapter()};
    AnnotationType *genericRet = nullptr;

    MIRSymbolTable *symTab = nullptr;
    MIRTypeNameTable *typeNameTab = nullptr;
    MIRLabelTable *labelTab = nullptr;
    MIRPregTable *pregTab = nullptr;
    MemPool *codeMemPool = nullptr;
    MapleAllocator codeMemPoolAllocator {nullptr};
    uint32 callTimes = 0;
    BlockNode *body = nullptr;
    BlockNode *bodyLast = nullptr;
    FuncAttrs funcAttrs {};
    uint32 flag = 0;
    uint16 hashCode = 0;   // for methodmetadata order
    uint32 fileIndex = 0;  // this function belongs to which file, used by VM for plugin manager
    MIRInfoVector info {module->GetMPAllocator().Adapter()};
    MapleVector<bool> infoIsString {module->GetMPAllocator().Adapter()};  // tells if an entry has string value
    MapleMap<uint32, uint32> *freqFirstMap = nullptr;  // save bb frequency in its first_stmt, key is stmtId
    MapleMap<uint32, uint32> *freqLastMap = nullptr;   // save bb frequency in its last_stmt, key is stmtId
    MapleSet<uint32> referedPregs {module->GetMPAllocator().Adapter()};
    MapleUnorderedMap<PregIdx, PregIdx> derived2BaseRef {module->GetMPAllocator().Adapter()};
    bool referedRegsValid = false;
    bool hasVlaOrAlloca = false;
    bool withLocInfo = true;
    bool withSrc = true;
    bool isVisited = false;  // only used in inline phase.
    bool isDirty = false;
    bool fromMpltInline = false;  // Whether this function is imported from mplt_inline file or not.
    uint8_t layoutType = kLayoutUnused;
    uint32 frameSize = 0;
    uint32 upFormalSize = 0;
    uint32 outParmSize = 0;
    uint16 moduleID = 0;
    uint32 funcSize = 0;  // size of code in words
    uint32 tempCount = 0;
    uint8 *formalWordsTypeTagged = nullptr;  // bit vector where the Nth bit tells whether
    // the Nth word in the formal parameters area
    // addressed upward from %%FP (that means
    // the word at location (%%FP + N*4)) has
    // typetag; if yes, the typetag is the word
    // at (%%FP + N*4 + 4); the bitvector's size
    // is given by BlockSize2BitvectorSize(upFormalSize)
    uint8 *localWordsTypeTagged = nullptr;  // bit vector where the Nth bit tells whether
    // the Nth word in the local stack frame
    // addressed downward from %%FP (that means
    // the word at location (%%FP - N*4)) has
    // typetag; if yes, the typetag is the word
    // at (%%FP - N*4 + 4); the bitvector's size
    // is given by BlockSize2BitvectorSize(frameSize)
    uint8 *formalWordsRefCounted = nullptr;  // bit vector where the Nth bit tells whether
    // the Nth word in the formal parameters area
    // addressed upward from %%FP (that means
    // the word at location (%%FP + N*4)) points to
    // a dynamic memory block that needs reference
    // count; the bitvector's size is given by
    // BlockSize2BitvectorSize(upFormalSize)
    uint8 *localWordsRefCounted = nullptr;  // bit vector where the Nth bit tells whether
    // the Nth word in the local stack frame
    // addressed downward from %%FP (that means
    // the word at location (%%FP - N*4)) points to
    // a dynamic memory block that needs reference
    // count; the bitvector's size is given by
    // BlockSize2BitvectorSize(frameSize)
    // uint16 numlabels; // removed. label table size
    // StmtNode **lbl2stmt // lbl2stmt table, removed
    // to hold unmangled class and function names
    MeFunction *meFunc = nullptr;
    EAConnectionGraph *eacg = nullptr;
    GStrIdx baseClassStrIdx {0};  // the string table index of base class name
    GStrIdx baseFuncStrIdx {0};   // the string table index of base function name
    // the string table index of base function name mangled with type info
    GStrIdx baseFuncWithTypeStrIdx {0};
    // funcname + types of args, no type of retv
    GStrIdx baseFuncSigStrIdx {0};
    GStrIdx signatureStrIdx {0};
    MemPool *codeMemPoolTmp {nullptr};
    MapleAllocator codeMemPoolTmpAllocator {nullptr};
    bool useTmpMemPool = false;
    PointerAttr returnKind = PointerAttr::kPointerUndeiced;
    MapleMap<MIRSymbol *, PointerAttr> paramNonullTypeMap {module->GetMPAllocator().Adapter()};
    FuncDesc funcDesc {};
    GcovFuncInfo *funcProfData = nullptr;
    MemReferenceTable *memReferenceTable = nullptr;
#ifdef ARK_LITECG_DEBUG
    void DumpFlavorLoweredThanMmpl() const;
#endif
    MIRFuncType *ReconstructFormals(const std::vector<MIRSymbol *> &symbols, bool clearOldArgs);
    SaveInfo frameTypeInfo {0, false, 0};
    SaveInfo funcIdxInfo {0, false, 0};
    MapleList<MapleString> debugComments {module->GetMPAllocator().Adapter()};
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_FUNCTION_H
