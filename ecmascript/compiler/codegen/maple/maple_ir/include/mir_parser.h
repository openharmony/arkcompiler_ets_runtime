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

#ifndef MAPLE_IR_INCLUDE_MIR_PARSER_H
#define MAPLE_IR_INCLUDE_MIR_PARSER_H
#include "mir_module.h"
#include "lexer.h"
#include "mir_nodes.h"
#include "mir_preg.h"
#include "mir_scope.h"
#include "parser_opt.h"

namespace maple {
using BaseNodePtr = BaseNode *;
using StmtNodePtr = StmtNode *;
using BlockNodePtr = BlockNode *;

class FormalDef;

class MIRParser {
public:
    explicit MIRParser(MIRModule &md) : lexer(md), mod(md), definedLabels(mod.GetMPAllocator().Adapter())
    {
        safeRegionFlag.push(false);
    }

    ~MIRParser() = default;

    MIRFunction *CreateDummyFunction();
    void ResetCurrentFunction()
    {
        mod.SetCurFunction(dummyFunction);
    }

    bool ParseLoc();
    bool ParsePosition(SrcPosition &pos);
    bool ParseOneAlias(GStrIdx &strIdx, MIRAliasVars &aliasVar);
    uint8 *ParseWordsInfo(uint32 size);
    bool ParseSwitchCase(int64 &, LabelIdx &);
    bool ParseExprOneOperand(BaseNodePtr &expr);
    bool ParseExprTwoOperand(BaseNodePtr &opnd0, BaseNodePtr &opnd1);
    bool ParseExprNaryOperand(MapleVector<BaseNode *> &opndVec);
    bool IsDelimitationTK(TokenKind tk) const;
    Opcode GetOpFromToken(TokenKind tk) const;
    bool IsStatement(TokenKind tk) const;
    PrimType GetPrimitiveType(TokenKind tk) const;
    MIRIntrinsicID GetIntrinsicID(TokenKind tk) const;
    bool ParseScalarValue(MIRConstPtr &stype, MIRType &type);
    bool ParseDeclaredSt(StIdx &stidx);
    void CreateFuncMIRSymbol(PUIdx &puidx, GStrIdx strIdx);
    bool ParseDeclaredFunc(PUIdx &puidx);
    bool ParseTypeAttrs(TypeAttrs &attrs);
    bool ParseVarTypeAttrs(MIRSymbol &st);
    bool CheckAlignTk();
    bool ParseAlignAttrs(TypeAttrs &tA);
    bool ParsePackAttrs();
    bool ParseFieldAttrs(FieldAttrs &attrs);
    bool ParseFuncAttrs(FuncAttrs &attrs);
    void SetAttrContent(FuncAttrs &attrs, FuncAttrKind x, const MIRLexer &lexer);
    bool CheckPrimAndDerivedType(TokenKind tokenKind, TyIdx &tyIdx);
    bool ParsePrimType(TyIdx &tyIdx);
    bool ParseFarrayType(TyIdx &arrayTyIdx);
    bool ParseArrayType(TyIdx &arrayTyIdx);
    bool ParseBitFieldType(TyIdx &fieldTyIdx);
    bool ParsePragmaElementForArray(MIRPragmaElement &elem);
    bool ParseClassType(TyIdx &styidx, const GStrIdx &strIdx = GStrIdx(0));
    bool ParseInterfaceType(TyIdx &sTyIdx, const GStrIdx &strIdx = GStrIdx(0));
    bool ParseTypeParam(TyIdx &definedTyIdx);
    bool ParseGenericInstantVector(MIRInstantVectorType &insVecType);
    bool ParseType(TyIdx &tyIdx);
    bool ParseSpecialReg(PregIdx &pRegIdx);
    bool ParsePseudoReg(PrimType primType, PregIdx &pRegIdx);
    bool ParseStorageClass(MIRSymbol &symbol) const;
    bool ParseDeclareVarInitValue(MIRSymbol &symbol);
    bool ParseDeclareReg(MIRSymbol &symbol, const MIRFunction &func);
    bool ParseDeclareFormal(FormalDef &formalDef);
    bool ParsePrototypeRemaining(MIRFunction &func, std::vector<TyIdx> &vecTyIdx, std::vector<TypeAttrs> &vecAttrs,
                                 bool &varArgs);

    // Stmt Parser
    PUIdx EnterUndeclaredFunction(bool isMcount = false);  // for -pg in order to add "void _mcount()"
    bool ParseStmtCallMcount(StmtNodePtr &stmt);  // for -pg in order to add "void _mcount()" to all the functions
    bool ParseCallReturns(CallReturnVector &retsvec);
    bool ParseBinaryStmt(StmtNodePtr &stmt, Opcode op);

    // Expression Parser
    bool ParseExprIreadIaddrof(IreadNode &expr);
    bool ParseExprSTACKJarray(BaseNodePtr &expr);
    bool ParseNaryExpr(NaryStmtNode &stmtNode);

    // funcName and paramIndex is out parameter
    bool ParseCallAssertInfo(std::string &funcName, int *paramIndex, std::string &stmtFuncName);
    bool ParseAssertInfo(std::string &funcName);
    bool ParseIntrinsicId(IntrinsicopNode &intrnOpNode);
    void Error(const std::string &str);
    void Warning(const std::string &str);

    const std::string &GetError();
    const std::string &GetWarning() const;
    bool ParseFuncInfo(void);
    void PrepareParsingMIR();
    void PrepareParsingMplt();
    bool ParseSrcLang(MIRSrcLang &srcLang);
    bool ParseMPLT(std::ifstream &mpltFile, const std::string &importFileName);
    bool ParseTypeFromString(const std::string &src, TyIdx &tyIdx);
    void EmitError(const std::string &fileName);
    void EmitWarning(const std::string &fileName);
    uint32 GetOptions() const
    {
        return options;
    }

private:
    // func ptr map for ParseMIR()

    bool TypeCompatible(TyIdx typeIdx1, TyIdx typeIdx2);
    bool IsTypeIncomplete(MIRType *type);

    // func for ParseMIR
    bool ParseMIRForFunc();
    bool ParseMIRForFlavor();
    bool ParseMIRForSrcLang();
    bool ParseMIRForGlobalMemSize();
    bool ParseMIRForGlobalMemMap();
    bool ParseMIRForGlobalWordsTypeTagged();
    bool ParseMIRForGlobalWordsRefCounted();
    bool ParseMIRForID();
    bool ParseMIRForNumFuncs();
    bool ParseMIRForEntryFunc();
    bool ParseMIRForFileInfo();
    bool ParseMIRForFileData();
    bool ParseMIRForSrcFileInfo();
    bool ParseMIRForImportPath();
    bool ParseMIRForAsmdecl();

    // func and param for ParseStmtBlock
    void ParseStmtBlockForSeenComment(BlockNodePtr blk, uint32 mplNum);
    bool ParseStmtBlockForVar(TokenKind stmtTK);
    bool ParseStmtBlockForVar();
    bool ParseStmtBlockForTempVar();
    bool ParseStmtBlockForReg();
    bool ParseStmtBlockForType();
    bool ParseStmtBlockForFrameSize();
    bool ParseStmtBlockForUpformalSize();
    bool ParseStmtBlockForModuleID();
    bool ParseStmtBlockForFuncSize();
    bool ParseStmtBlockForFuncID();
    bool ParseStmtBlockForFormalWordsTypeTagged();
    bool ParseStmtBlockForLocalWordsTypeTagged();
    bool ParseStmtBlockForFormalWordsRefCounted();
    bool ParseStmtBlockForLocalWordsRefCounted();
    bool ParseStmtBlockForFuncInfo();

    // common func
    void SetSrcPos(SrcPosition &srcPosition, uint32 mplNum);

    // func for ParseExpr
    Opcode paramOpForStmt = OP_undef;
    TokenKind paramTokenKindForStmt = TK_invalid;
    // func and param for ParseStmtBlock
    MIRFunction *paramCurrFuncForParseStmtBlock = nullptr;
    MIRLexer lexer;
    MIRModule &mod;
    std::string message;
    std::string warningMessage;
    uint32 options = kKeepFirst;
    MapleVector<bool> definedLabels;  // true if label at labidx is defined
    MIRFunction *dummyFunction = nullptr;
    MIRFunction *curFunc = nullptr;
    uint16 lastFileNum = 0;                // to remember first number after LOC
    uint32 lastLineNum = 0;                // to remember second number after LOC
    uint16 lastColumnNum = 0;              // to remember third number after LOC
    uint32 firstLineNum = 0;               // to track function starting line
    std::map<TyIdx, TyIdx> typeDefIdxMap;  // map previous declared tyIdx
    bool firstImport = true;               // Mark the first imported mplt file
    bool paramParseLocalType = false;      // param for ParseTypedef
    uint32 paramFileIdx = 0;               // param for ParseMIR()
    bool paramIsIPA = false;
    bool paramIsComb = false;
    TokenKind paramTokenKind = TK_invalid;
    std::vector<std::string> paramImportFileList;
    std::stack<bool> safeRegionFlag;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PARSER_H
