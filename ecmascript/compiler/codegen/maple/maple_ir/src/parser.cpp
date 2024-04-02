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

#include <climits>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include "mir_parser.h"
#include "mir_scope.h"
#include "mir_function.h"
#include "namemangler.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "bin_mplt.h"
#include "option.h"
#include "clone.h"
#include "string_utils.h"
#include "debug_info.h"

namespace {
using namespace maple;
constexpr char kLexerStringSp[] = "SP";
constexpr char kLexerStringFp[] = "FP";
constexpr char kLexerStringGp[] = "GP";
constexpr char kLexerStringThrownval[] = "thrownval";
constexpr char kLexerStringRetval[] = "retval";
const std::map<std::string, SpecialReg> pregMapIdx = {{kLexerStringSp, kSregSp},
                                                      {kLexerStringFp, kSregFp},
                                                      {kLexerStringGp, kSregGp},
                                                      {kLexerStringThrownval, kSregThrownval},
                                                      {kLexerStringRetval, kSregRetval0}};

const std::map<TokenKind, PragmaKind> tkPragmaKind = {{TK_class, kPragmaClass},
                                                      {TK_func, kPragmaFunc},
                                                      {TK_var, kPragmaVar},
                                                      {TK_param, kPragmaParam},
                                                      {TK_func_ex, kPragmaFuncExecptioni},
                                                      {TK_func_var, kPragmaFuncVar}};
const std::map<TokenKind, PragmaValueType> tkPragmaValType = {
    {TK_i8, kValueByte},          {TK_i16, kValueShort},  {TK_u16, kValueChar},    {TK_i32, kValueInt},
    {TK_i64, kValueLong},         {TK_f32, kValueFloat},  {TK_f64, kValueDouble},  {TK_retype, kValueMethodType},
    {TK_ref, kValueMethodHandle}, {TK_ptr, kValueString}, {TK_type, kValueType},   {TK_var, kValueField},
    {TK_func, kValueMethod},      {TK_enum, kValueEnum},  {TK_array, kValueArray}, {TK_annotation, kValueAnnotation},
    {TK_const, kValueNull},       {TK_u1, kValueBoolean}};
}  // namespace

namespace maple {
std::map<TokenKind, MIRParser::FuncPtrParseMIRForElem> MIRParser::funcPtrMapForParseMIR =
    MIRParser::InitFuncPtrMapForParseMIR();

MIRFunction *MIRParser::CreateDummyFunction()
{
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("__$$__");
    MIRBuilder mirBuilder(&mod);
    MIRSymbol *funcSt = mirBuilder.CreateSymbol(TyIdx(0), strIdx, kStFunc, kScUnused, nullptr, kScopeGlobal);
    CHECK_FATAL(funcSt != nullptr, "Failed to create MIRSymbol");
    // Don't add the function to the function table.
    // It appears Storage class kScUnused is not honored.
    MIRFunction *func = mirBuilder.CreateFunction(funcSt->GetStIdx(), false);
    CHECK_FATAL(func != nullptr, "Failed to create MIRFunction");
    func->SetPuidxOrigin(func->GetPuidx());
    MIRType *returnType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_void));
    func->SetReturnTyIdx(returnType->GetTypeIndex());
    func->SetClassTyIdx(0U);
    func->SetBaseClassFuncNames(strIdx);
    funcSt->SetFunction(func);
    return func;
}

bool MIRParser::IsDelimitationTK(TokenKind tk) const
{
    switch (tk) {
        case TK_rparen:
        case TK_coma:
            return true;
        default:
            return false;
    }
}

inline bool IsPowerOf2(uint64 num)
{
    if (num == 0) {
        return false;
    }
    return (~(num - 1) & num) == num;
}

Opcode MIRParser::GetOpFromToken(TokenKind tk) const
{
    switch (tk) {
#define OPCODE(X, Y, Z, S) \
    case TK_##X:           \
        return OP_##X;
#include "opcodes.def"
#undef OPCODE
        default:
            return OP_undef;
    }
}

static bool IsClassInterfaceTypeName(const std::string &nameStr)
{
    return (!nameStr.empty() && nameStr.front() == 'L' && StringUtils::EndsWith(nameStr, "_3B"));
}

bool MIRParser::IsStatement(TokenKind tk) const
{
    if (tk == TK_LOC || tk == TK_ALIAS || tk == TK_SCOPE) {
        return true;
    }
    Opcode op = GetOpFromToken(tk);
    return kOpcodeInfo.IsStmt(op);
}

PrimType MIRParser::GetPrimitiveType(TokenKind tk) const
{
#define LOAD_ALGO_PRIMARY_TYPE
    switch (tk) {
#define PRIMTYPE(P)                       \
    case TK_##P: {                        \
        if (tk == TK_ptr) {               \
            return GetExactPtrPrimType(); \
        }                                 \
        return PTY_##P;                   \
    }
#include "prim_types.def"
#undef PRIMTYPE
        default:
            return kPtyInvalid;
    }
}

MIRIntrinsicID MIRParser::GetIntrinsicID(TokenKind tk) const
{
    switch (tk) {
        default:
#define DEF_MIR_INTRINSIC(P, NAME, INTRN_CLASS, RETURN_TYPE, ...) \
    case TK_##P: {                                                \
        return INTRN_##P;                                         \
    }
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
    }
}

void MIRParser::Error(const std::string &str)
{
    std::stringstream strStream;
    const std::string &lexName = lexer.GetName();
    int curIdx = static_cast<int>(lexer.GetCurIdx()) - static_cast<int>(lexName.length()) + 1;
    strStream << "line: " << lexer.GetLineNum() << ":" << curIdx << ":";
    message += strStream.str();
    message += str;
    message += ": ";
    message += lexer.GetTokenString();
    message += "\n";
    mod.GetDbgInfo()->SetErrPos(lexer.GetLineNum(), lexer.GetCurIdx());
}

const std::string &MIRParser::GetError()
{
    if (lexer.GetTokenKind() == TK_invalid) {
        std::stringstream strStream;
        strStream << "line: " << lexer.GetLineNum() << ":" << lexer.GetCurIdx() << ":";
        message += strStream.str();
        message += " invalid token\n";
    }
    return message;
}

void MIRParser::Warning(const std::string &str)
{
    std::stringstream strStream;
    const std::string &lexName = lexer.GetName();
    int curIdx = static_cast<int>(lexer.GetCurIdx()) - static_cast<int>(lexName.length()) + 1;
    strStream << "  >> warning line: " << lexer.GetLineNum() << ":" << curIdx << ":";
    warningMessage += strStream.str();
    warningMessage += str;
    warningMessage += "\n";
}

const std::string &MIRParser::GetWarning() const
{
    return warningMessage;
}

bool MIRParser::ParseSpecialReg(PregIdx &pRegIdx)
{
    const std::string &lexName = lexer.GetName();
    size_t lexSize = lexName.size();
    size_t retValSize = strlen(kLexerStringRetval);
    if (strncmp(lexName.c_str(), kLexerStringRetval, retValSize) == 0 && (lexSize > retValSize) &&
        isdigit(lexName[retValSize])) {
        int32 retValNo = lexName[retValSize] - '0';
        for (size_t i = retValSize + 1; (i < lexSize) && isdigit(lexName[i]); ++i) {
            retValNo = retValNo * 10 + lexName[i] - '0'; // 10 for decimal
        }
        pRegIdx = -kSregRetval0 - retValNo;
        lexer.NextToken();
        return true;
    }

    auto it = pregMapIdx.find(lexName);
    if (it != pregMapIdx.end()) {
        pRegIdx = -(it->second);
        lexer.NextToken();
        return true;
    }

    Error("unrecognized special register ");
    return false;
}

bool MIRParser::ParsePseudoReg(PrimType primType, PregIdx &pRegIdx)
{
    uint32 pregNo = static_cast<uint32>(lexer.GetTheIntVal());
    DEBUG_ASSERT(pregNo <= 0xffff, "preg number must be 16 bits");
    MIRFunction *curfunc = mod.CurFunction();
    pRegIdx = curfunc->GetPregTab()->EnterPregNo(pregNo, primType);
    MIRPreg *preg = curfunc->GetPregTab()->PregFromPregIdx(pRegIdx);
    if (primType != kPtyInvalid) {
        if (preg->GetPrimType() != primType) {
            if (IsAddress(preg->GetPrimType()) && IsAddress(primType)) {
            } else {
                Error("inconsistent preg primitive type at ");
                return false;
            }
        }
    }

    lexer.NextToken();
    return true;
}

bool MIRParser::CheckPrimAndDerivedType(TokenKind tokenKind, TyIdx &tyIdx)
{
    if (IsPrimitiveType(tokenKind)) {
        return ParsePrimType(tyIdx);
    }
    if (tokenKind == TK_langle) {
        return ParseDerivedType(tyIdx);
    }
    return false;
}

bool MIRParser::ParseType(TyIdx &tyIdx)
{
    TokenKind tk = lexer.GetTokenKind();
    if (IsVarName(tk)) {
        return ParseDefinedTypename(tyIdx);
    }
    if (CheckPrimAndDerivedType(tk, tyIdx)) {
        return true;
    }
    Error("token is not a type ");
    return false;
}

bool MIRParser::ParseFarrayType(TyIdx &arrayTyIdx)
{
    TokenKind tokenKind = lexer.NextToken();
    TyIdx tyIdx;
    if (!CheckPrimAndDerivedType(tokenKind, tyIdx)) {
        Error("unexpect token parsing flexible array element type ");
        return false;
    }
    DEBUG_ASSERT(tyIdx != 0u, "error encountered parsing flexible array element type ");
    if (mod.IsJavaModule() || mod.IsCharModule()) {
        MIRJarrayType jarrayType(tyIdx);
        arrayTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&jarrayType);
    } else {
        MIRFarrayType farrayType(tyIdx);
        arrayTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&farrayType);
    }
    return true;
}

bool MIRParser::ParseArrayType(TyIdx &arrayTyIdx)
{
    TokenKind tokenKind = lexer.GetTokenKind();
    if (tokenKind != TK_lbrack) {
        Error("expect [ for array type but get ");
        return false;
    }
    std::vector<uint64> vec;
    while (tokenKind == TK_lbrack) {
        tokenKind = lexer.NextToken();
        if (tokenKind == TK_rbrack && vec.empty()) {
            break;
        }
        if (tokenKind != TK_intconst) {
            Error("expect int value parsing array type after [ but get ");
            return false;
        }
        int64 val = static_cast<int64>(lexer.GetTheIntVal());
        if (val < 0) {
            Error("expect array value >= 0 ");
            return false;
        }
        vec.push_back(val);
        if (lexer.NextToken() != TK_rbrack) {
            Error("expect ] after int value parsing array type but get ");
            return false;
        }
        tokenKind = lexer.NextToken();
    }
    if (tokenKind == TK_rbrack && vec.empty()) {
        return ParseFarrayType(arrayTyIdx);
    }
    TyIdx tyIdx;
    if (!CheckPrimAndDerivedType(tokenKind, tyIdx)) {
        Error("unexpect token parsing array type after ] ");
        return false;
    }
    DEBUG_ASSERT(tyIdx != 0u, "something wrong with parsing element type ");
    MIRArrayType arrayType(tyIdx, vec);
    if (!ParseTypeAttrs(arrayType.GetTypeAttrs())) {
        Error("bad type attribute in pointer type specification");
        return false;
    }
    arrayTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&arrayType);
    return true;
}

bool MIRParser::ParseBitFieldType(TyIdx &fieldTyIdx)
{
    if (lexer.GetTokenKind() != TK_colon) {
        Error("expect : parsing field type but get ");
        return false;
    }
    if (lexer.NextToken() != TK_intconst) {
        Error("expect int const val parsing field type but get ");
        return false;
    }
    DEBUG_ASSERT(lexer.GetTheIntVal() <= 0xFFU, "lexer.theIntVal is larger than max uint8 bitsize value.");
    uint8 bitSize = static_cast<uint8>(lexer.GetTheIntVal()) & 0xFFU;
    PrimType primType = GetPrimitiveType(lexer.NextToken());
    if (primType == kPtyInvalid) {
        Error("expect primitive type but get ");
        return false;
    }
    if (!IsPrimitiveInteger(primType)) {
        Error("syntax error bit field should be integer type but get ");
        return false;
    }
    MIRBitFieldType bitFieldType(bitSize, primType);
    fieldTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&bitFieldType);
    lexer.NextToken();
    return true;
}

bool MIRParser::ParsePragmaElement(MIRPragmaElement &elem)
{
    TokenKind tk = lexer.GetTokenKind();
    lexer.NextToken();
    auto it = tkPragmaValType.find(tk);
    if (it == tkPragmaValType.end()) {
        Error("parsing pragma error: wrong element type");
        return false;
    }
    elem.SetType(it->second);

    switch (tk) {
        case TK_i8:
        case TK_i16:
        case TK_i32:
            elem.SetI32Val(static_cast<int32>(lexer.GetTheIntVal()));
            break;
        case TK_u16:
        case TK_ref:
            elem.SetU64Val(static_cast<uint64>(lexer.GetTheIntVal()));
            break;
        case TK_i64:
        case TK_retype:
        case TK_const:
        case TK_u1:
            elem.SetI64Val(lexer.GetTheIntVal());
            break;
        case TK_f32:
            elem.SetFloatVal(lexer.GetTheFloatVal());
            break;
        case TK_f64:
            elem.SetDoubleVal(lexer.GetTheDoubleVal());
            break;
        case TK_ptr:
        case TK_var:
        case TK_func:
        case TK_enum:
            elem.SetI32Val(static_cast<int32>(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName())));
            break;
        case TK_type:
            lexer.NextToken();
            elem.SetI32Val(static_cast<int32>(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName())));
            lexer.NextToken();
            break;
        case TK_array:
            if (!ParsePragmaElementForArray(elem)) {
                return false;
            }
            break;
        case TK_annotation:
            if (!ParsePragmaElementForAnnotation(elem)) {
                return false;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool MIRParser::ParsePragmaElementForArray(MIRPragmaElement &elem)
{
    TokenKind tk;
    tk = lexer.GetTokenKind();
    if (tk != TK_lbrack) {
        Error("parsing pragma error: expecting [ but get ");
        return false;
    }
    tk = lexer.NextToken();
    if (tk != TK_intconst) {
        Error("parsing pragma error: expecting int but get ");
        return false;
    }
    int64 size = static_cast<int64>(lexer.GetTheIntVal());
    tk = lexer.NextToken();
    if (tk != TK_coma && size) {
        Error("parsing pragma error: expecting , but get ");
        return false;
    }
    for (int64 i = 0; i < size; ++i) {
        auto *e0 = mod.GetMemPool()->New<MIRPragmaElement>(mod);
        tk = lexer.NextToken();
        if (!ParsePragmaElement(*e0)) {
            Error("parsing pragma error type ");
            return false;
        }
        elem.SubElemVecPushBack(e0);
        tk = lexer.NextToken();
        if (tk != TK_coma && tk != TK_rbrack) {
            Error("parsing pragma error: expecting , or ] but get ");
            return false;
        }
    }
    return true;
}

bool MIRParser::ParsePragmaElementForAnnotation(MIRPragmaElement &elem)
{
    TokenKind tk;
    tk = lexer.GetTokenKind();
    if (tk != TK_langle) {
        Error("parsing pragma error: expecting < but get ");
        return false;
    }
    tk = lexer.NextToken();
    elem.SetTypeStrIdx(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName()));
    tk = lexer.NextToken();
    if (tk != TK_rangle) {
        Error("parsing pragma error: expecting > but get ");
        return false;
    }
    tk = lexer.NextToken();
    if (tk != TK_lbrack) {
        Error("parsing pragma error: expecting [ but get ");
        return false;
    }
    tk = lexer.NextToken();
    if (tk != TK_intconst) {
        Error("parsing pragma error: expecting int but get ");
        return false;
    }
    int64 size = static_cast<int64>(lexer.GetTheIntVal());
    tk = lexer.NextToken();
    if (tk != TK_coma && size) {
        Error("parsing pragma error: expecting , but get ");
        return false;
    }
    for (int64 i = 0; i < size; ++i) {
        auto *e0 = mod.GetMemPool()->New<MIRPragmaElement>(mod);
        tk = lexer.NextToken();
        if (tk != TK_label) {
            Error("parsing pragma error: expecting @ but get ");
            return false;
        }
        e0->SetNameStrIdx(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName()));
        tk = lexer.NextToken();
        if (!ParsePragmaElement(*e0)) {
            Error("parsing pragma error type ");
            return false;
        }
        elem.SubElemVecPushBack(e0);
        tk = lexer.NextToken();
        if (tk != TK_coma && tk != TK_rbrack) {
            Error("parsing pragma error: expecting , or ] but get ");
            return false;
        }
    }
    return true;
}

bool MIRParser::ParsePragma(MIRStructType &type)
{
    auto *p = mod.GetMemPool()->New<MIRPragma>(mod);
    p->SetVisibility(lexer.GetTheIntVal());
    TokenKind tk = lexer.NextToken();

    auto it = tkPragmaKind.find(tk);
    if (it == tkPragmaKind.end()) {
        Error("parsing pragma error: wrong kind ");
        return false;
    }
    p->SetKind(it->second);

    if (tk == TK_param) {
        lexer.NextToken();
        p->SetParamNum(lexer.GetTheIntVal());
    }
    tk = lexer.NextToken();
    if (tk != TK_gname && tk != TK_lname && tk != TK_fname) {
        Error("expect global or local token parsing pragma but get ");
        return false;
    }
    p->SetStrIdx(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName()));
    lexer.NextToken();
    TyIdx tyIdx;
    if (!ParseType(tyIdx)) {
        Error("parsing pragma error: wrong type ");
        return false;
    }
    p->SetTyIdx(tyIdx);

    tk = lexer.GetTokenKind();
    if (tk != TK_lbrace) {
        TyIdx tyIdxEx;
        if (!ParseType(tyIdxEx)) {
            Error("parsing pragma error: wrong type ");
            return false;
        }
        p->SetTyIdxEx(tyIdxEx);
    }

    tk = lexer.NextToken();
    while (tk != TK_rbrace) {
        auto *e = mod.GetMemPool()->New<MIRPragmaElement>(mod);
        e->SetNameStrIdx(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName()));
        tk = lexer.NextToken();
        if (!ParsePragmaElement(*e)) {
            Error("parsing pragma error type ");
            return false;
        }
        p->PushElementVector(e);
        tk = lexer.NextToken();
        if (tk != TK_rbrace && tk != TK_coma) {
            Error("parsing pragma error syntax ");
            return false;
        }
        if (tk == TK_coma) {
            lexer.NextToken();
        }
    }
    lexer.NextToken();
    type.PushbackPragma(p);
    return true;
}

// lexer.GetTokenKind() assumed to be the TK_lbrace that starts the fields
bool MIRParser::ParseFields(MIRStructType &type)
{
    if (type.IsIncomplete()) {
        Warning("incomplete class/interface type");
    }
    TokenKind tk = lexer.NextToken();
    MIRTypeKind tyKind = type.GetKind();
    while (tk == TK_label || tk == TK_prntfield || tk == TK_pragma) {
        bool isPragma = (tk == TK_pragma);
        bool notaType = false;
        TyIdx fieldTyIdx(0);
        bool isParentField = false;
        if (tk == TK_prntfield) {
            isParentField = true;
        }
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        tk = lexer.NextToken();
        if (isPragma) {
            if (tyKind == kTypeClass || tyKind == kTypeClassIncomplete || tyKind == kTypeInterface ||
                tyKind == kTypeInterfaceIncomplete) {
                if (!ParsePragma(type)) {
                    Error("parsing pragma error ");
                    return false;
                }
            } else {
                Error("parsing pragma error ");
                return false;
            }
            notaType = true;
        } else if ((tk == TK_lbrack) && (GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx) == "staticvalue")) {
            while (tk != TK_coma) {
                EncodedValue elem;
                if (tk != TK_lbrack) {
                    Error("parsing staticvalue error ");
                    return false;
                }
                tk = lexer.NextToken();
                uint32 i = 0;
                while (tk != TK_rbrack) {
                    if (tk != TK_intconst) {
                        Error("parsing staticvalue error ");
                        return false;
                    }
                    elem.encodedValue[i++] = lexer.GetTheIntVal();
                    tk = lexer.NextToken();
                }
                tk = lexer.NextToken();
                if (tyKind == kTypeClass || tyKind == kTypeClassIncomplete || tyKind == kTypeInterface ||
                    tyKind == kTypeInterfaceIncomplete) {
                    type.PushbackStaticValue(elem);
                } else {
                    Error("parsing staticvalue error ");
                    return false;
                }
            }
            notaType = true;
        } else if (tk == TK_colon) {  // a bitfield
            if (!ParseBitFieldType(fieldTyIdx)) {
                Error("parsing struct type error ");
                return false;
            }
        } else if (tk == TK_langle) {
            if (!ParseDerivedType(fieldTyIdx)) {
                Error("parsing struct type error ");
                return false;
            }
        } else if ((IsPrimitiveType(tk))) {
            if (!ParsePrimType(fieldTyIdx)) {
                Error("expect :<val> or primitive type or derived type parsing struct type ");
                return false;
            }
        } else if ((tk == TK_intconst || tk == TK_string) && !isParentField &&
                   (tyKind == kTypeClass || tyKind == kTypeClassIncomplete || tyKind == kTypeInterface ||
                    tyKind == kTypeInterfaceIncomplete)) {
            uint32 infoVal =
                (tk == TK_intconst)
                    ? static_cast<uint32>(lexer.GetTheIntVal())
                    : static_cast<uint32>(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName()));
            type.PushbackMIRInfo(MIRInfoPair(strIdx, infoVal));
            type.PushbackIsString(tk != TK_intconst);
            notaType = true;
            lexer.NextToken();
        } else {
            Error("unexpected type parsing struct type at ");
            return false;
        }
        DEBUG_ASSERT((fieldTyIdx != 0u || notaType), "something wrong parsing struct type");
        if (!notaType) {
            FieldAttrs tA;
            if (!ParseFieldAttrs(tA)) {
                Error("bad type attribute in struct field at ");
                return false;
            }
            if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx)->HasTypeParam()) {
                tA.SetAttr(FLDATTR_generic);
            }
            FieldPair p = FieldPair(strIdx, TyIdxFieldAttrPair(fieldTyIdx, tA));
            bool isStaticField = false;
            if (tA.GetAttr(FLDATTR_static)) {
                // static and parent share the same ^ token
                isStaticField = true;
                isParentField = false;
            }
            if (isParentField) {
                type.GetParentFields().push_back(p);
            } else if (isStaticField) {
                type.GetStaticFields().push_back(p);
            } else {
                type.GetFields().push_back(p);
            }
            tk = lexer.GetTokenKind();
            bool isConst = tA.GetAttr(FLDATTR_static) && tA.GetAttr(FLDATTR_final) &&
                           (tA.GetAttr(FLDATTR_public) || tA.GetAttr(FLDATTR_protected));
            if (isConst && tk == TK_eqsign) {
                tk = lexer.NextToken();
                MIRConst *mirConst = nullptr;
                if (!ParseInitValue(mirConst, fieldTyIdx)) {
                    Error("wrong initialization value at ");
                    return false;
                }
                GlobalTables::GetConstPool().InsertConstPool(p.first, mirConst);
                tk = lexer.GetTokenKind();
            }
        } else {
            tk = lexer.GetTokenKind();
        }
        tk = lexer.GetTokenKind();
        if (tk == TK_rbrace) {
            return true;
        }
        if (tk != TK_coma) {
            Error(", missing after ");
            return false;
        }
        tk = lexer.NextToken();
        if (tk == TK_rbrace) {
            Error(",} is not legal, expect another field type after ,");
            return false;
        }
    }
    while (tk == TK_fname) {
        const std::string &funcName = lexer.GetName();
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
        MIRSymbol *prevFuncSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
        if (prevFuncSymbol && (prevFuncSymbol->GetStorageClass() != kScText || prevFuncSymbol->GetSKind() != kStFunc)) {
            // Based on the current maple format, a previous declaration at this
            // point can only come from another module. Check consistency.
            Error("redeclaration of name as func in ");
            return false;
        }
        // Always create a new symbol because we can not reuse symbol from other module
        maple::MIRBuilder mirBuilder(&mod);
        MIRSymbol *funcSymbol = mirBuilder.CreateSymbol(TyIdx(0), strIdx, kStFunc, kScText, nullptr, kScopeGlobal);
        DEBUG_ASSERT(funcSymbol != nullptr, "Failed to create MIRSymbol");
        SetSrcPos(funcSymbol->GetSrcPosition(), lexer.GetLineNum());

        MIRFunction *fn = mod.GetMemPool()->New<MIRFunction>(&mod, funcSymbol->GetStIdx());
        fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
        GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
        funcSymbol->SetFunction(fn);
        fn->SetFileIndex(0);
        fn->SetBaseClassFuncNames(funcSymbol->GetNameStrIdx());
        FuncAttrs tA;
        if (lexer.NextToken() != TK_lparen) {
            if (!ParseFuncAttrs(tA)) {
                return false;
            }
            // Skip attribute checking
            fn->SetFuncAttrs(tA);
        }
        TyIdx funcTyIdx;
        if (!ParseFuncType(funcTyIdx)) {
            return false;
        }
        // tyIdx does not work. Calling EqualTo does not work either.
        auto *funcType = static_cast<MIRFuncType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcTyIdx));
        fn->SetMIRFuncType(funcType);
        fn->SetReturnStruct(*GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx()));
        funcSymbol->SetTyIdx(funcTyIdx);

        for (size_t i = 0; i < funcType->GetParamTypeList().size(); i++) {
            FormalDef formalDef(nullptr, funcType->GetParamTypeList()[i], funcType->GetParamAttrsList()[i]);
            fn->GetFormalDefVec().push_back(formalDef);
        }

        MethodPair p = MethodPair(funcSymbol->GetStIdx(), TyidxFuncAttrPair(funcTyIdx, FuncAttrs(tA)));
        type.GetMethods().push_back(p);
        tk = lexer.GetTokenKind();
        if (tk == TK_coma) {
            tk = lexer.NextToken();
            if (tk == TK_rbrace) {
                Error(",} is not legal, expect another field type after ,");
                return false;
            }
        } else if (tk != TK_rbrace) {
            Error(", missing after ");
            return false;
        } else {
            return true;
        }
    }
    // interfaces_implemented
    while (tk == TK_gname) {
        tk = lexer.NextToken();
        if ((tk == TK_coma || tk == TK_rbrace) && (tyKind == kTypeClass || tyKind == kTypeClassIncomplete)) {
            auto *classType = static_cast<MIRClassType *>(&type);
            std::string nameStr = lexer.GetName();
            GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(nameStr);
            TyIdx tyIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
            if (tyIdx == 0u) {
                MIRInterfaceType interfaceType(kTypeInterfaceIncomplete);
                interfaceType.SetNameStrIdx(strIdx);
                tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&interfaceType);
                mod.AddClass(tyIdx);
                mod.AddExternStructType(tyIdx);
                mod.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, tyIdx);
            }
            classType->GetInterfaceImplemented().push_back(tyIdx);
        }
        if (tk == TK_coma) {
            tk = lexer.NextToken();
        }
    }
    // allow empty class for third party classes we do not have info
    if (tk == TK_rbrace) {
        return true;
    }
    Error("expect field or member function name in struct/class body but get ");
    return false;
}

bool MIRParser::ParseStructType(TyIdx &styIdx, const GStrIdx &strIdx)
{
    MIRTypeKind tkind = kTypeInvalid;
    switch (lexer.GetTokenKind()) {
        case TK_struct:
            tkind = kTypeStruct;
            break;
        case TK_structincomplete:
            tkind = kTypeStructIncomplete;
            break;
        case TK_union:
            tkind = kTypeUnion;
            break;
        default:
            break;
    }
    MIRStructType structType(tkind, strIdx);
    if (lexer.NextToken() != TK_lbrace) {
        if (!ParseTypeAttrs(structType.GetTypeAttrs())) {
            Error("bad type attribute in struct type specification");
            return false;
        }
    }
    if (lexer.GetTokenKind() != TK_lbrace) {
        Error("expect { parsing struct body");
        return false;
    }
    if (mod.GetSrcLang() == kSrcLangCPlusPlus) {
        structType.SetIsCPlusPlus(true);
    }
    if (!ParseFields(structType)) {
        return false;
    }
    // Bytecode file create a struct type with name, but do not check the type field.
    if (styIdx != 0u) {
        MIRType *prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(styIdx);
        DEBUG_ASSERT(prevType->GetKind() == kTypeStruct || prevType->IsIncomplete(), "type kind should be consistent.");
        if (static_cast<MIRStructType *>(prevType)->IsIncomplete() && !(structType.IsIncomplete())) {
            structType.SetNameStrIdx(prevType->GetNameStrIdx());
            structType.SetTypeIndex(styIdx);
            GlobalTables::GetTypeTable().SetTypeWithTyIdx(styIdx, *structType.CopyMIRTypeNode());
        }
    } else {
        TyIdx prevTypeIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
        if (prevTypeIdx != 0u) {
            // if the MIRStructType has been created by name or incompletely, refresh the prev created type
            MIRType *prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(prevTypeIdx);
            if (prevType->GetKind() == kTypeByName ||
                (prevType->GetKind() == kTypeStructIncomplete && tkind == kTypeStruct)) {
                structType.SetTypeIndex(prevTypeIdx);
                GlobalTables::GetTypeTable().SetTypeWithTyIdx(prevTypeIdx, *structType.CopyMIRTypeNode());
            }
            styIdx = prevTypeIdx;
        } else {
            styIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&structType);
        }
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseClassType(TyIdx &styidx, const GStrIdx &strIdx)
{
    MIRTypeKind tkind = (lexer.GetTokenKind() == TK_class) ? kTypeClass : kTypeClassIncomplete;
    TyIdx parentTypeIdx(0);
    if (lexer.NextToken() == TK_langle) {
        // parsing parent as class
        if (!ParseDerivedType(parentTypeIdx, kTypeClass)) {
            Error("parsing class parent type error ");
            return false;
        }
    }
    MIRClassType classType(tkind, strIdx);
    classType.SetParentTyIdx(parentTypeIdx);
    if (!ParseFields(classType)) {
        return false;
    }
    // Bytecode file create a strtuct type with name, but donot check the type field.
    MIRType *prevType = nullptr;
    if (styidx != 0u) {
        prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(styidx);
    }
    if (prevType != nullptr && prevType->GetKind() != kTypeByName) {
        DEBUG_ASSERT(prevType->GetKind() == kTypeClass || prevType->IsIncomplete(), "type kind should be consistent.");
        if (static_cast<MIRClassType *>(prevType)->IsIncomplete() && !(classType.IsIncomplete())) {
            classType.SetNameStrIdx(prevType->GetNameStrIdx());
            classType.SetTypeIndex(styidx);
            GlobalTables::GetTypeTable().SetTypeWithTyIdx(styidx, *classType.CopyMIRTypeNode());
        }
    } else {
        styidx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&classType);
        // set up classTyIdx for methods
        for (size_t i = 0; i < classType.GetMethods().size(); ++i) {
            StIdx stIdx = classType.GetMethodsElement(i).first;
            MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
            DEBUG_ASSERT(st->GetSKind() == kStFunc, "unexpected st->sKind");
            st->GetFunction()->SetClassTyIdx(styidx);
        }
        mod.AddClass(styidx);
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseInterfaceType(TyIdx &sTyIdx, const GStrIdx &strIdx)
{
    MIRTypeKind tkind = (lexer.GetTokenKind() == TK_interface) ? kTypeInterface : kTypeInterfaceIncomplete;
    std::vector<TyIdx> parents;
    TokenKind tk = lexer.NextToken();
    while (tk == TK_langle) {
        TyIdx parentTypeIdx(0);
        // parsing parents as interfaces
        if (!ParseDerivedType(parentTypeIdx, kTypeInterface)) {
            Error("parsing interface parent type error ");
            return false;
        }
        parents.push_back(parentTypeIdx);
        tk = lexer.GetTokenKind();
    }
    MIRInterfaceType interfaceType(tkind, strIdx);
    interfaceType.SetParentsTyIdx(parents);
    if (!ParseFields(interfaceType)) {
        return false;
    }
    // Bytecode file create a strtuct type with name, but donot check the type field.
    if (sTyIdx != 0u) {
        MIRType *prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sTyIdx);
        DEBUG_ASSERT(prevType->GetKind() == kTypeInterface || prevType->IsIncomplete(),
                     "type kind should be consistent.");
        if (static_cast<MIRInterfaceType *>(prevType)->IsIncomplete() && !(interfaceType.IsIncomplete())) {
            interfaceType.SetNameStrIdx(prevType->GetNameStrIdx());
            interfaceType.SetTypeIndex(sTyIdx);
            GlobalTables::GetTypeTable().SetTypeWithTyIdx(sTyIdx, *interfaceType.CopyMIRTypeNode());
        }
    } else {
        sTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&interfaceType);
        // set up classTyIdx for methods
        for (size_t i = 0; i < interfaceType.GetMethods().size(); ++i) {
            StIdx stIdx = interfaceType.GetMethodsElement(i).first;
            MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
            DEBUG_ASSERT(st != nullptr, "st is null");
            DEBUG_ASSERT(st->GetSKind() == kStFunc, "unexpected st->sKind");
            st->GetFunction()->SetClassTyIdx(sTyIdx);
        }
        mod.AddClass(sTyIdx);
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::CheckAlignTk()
{
    if (lexer.NextToken() != TK_lparen) {
        Error("unexpected token in alignment specification after ");
        return false;
    }
    if (lexer.NextToken() != TK_intconst) {
        Error("unexpected token in alignment specification after ");
        return false;
    }
    if (!IsPowerOf2(lexer.GetTheIntVal())) {
        Error("specified alignment must be power of 2 instead of ");
        return false;
    }
    if (lexer.NextToken() != TK_rparen) {
        Error("unexpected token in alignment specification after ");
        return false;
    }
    return true;
}

bool MIRParser::ParseAlignAttrs(TypeAttrs &tA)
{
    if (lexer.GetTokenKind() != TK_align) {
        Error("wrong TK kind taken from file");
        return false;
    }
    if (!CheckAlignTk()) {
        return false;
    }
    tA.SetAlign(lexer.GetTheIntVal());
    return true;
}

bool MIRParser::ParsePackAttrs()
{
    if (lexer.GetTokenKind() != TK_pack) {
        Error("wrong TK kind taken from file");
        return false;
    }
    if (!CheckAlignTk()) {
        return false;
    }
    return true;
}

// for variable declaration type attribute specification
// it has also storage-class qualifier.
bool MIRParser::ParseVarTypeAttrs(MIRSymbol &st)
{
    do {
        switch (lexer.GetTokenKind()) {
// parse attr without no content
#define TYPE_ATTR
#define NOCONTENT_ATTR
#define ATTR(X)               \
    case TK_##X: {            \
        st.SetAttr(ATTR_##X); \
        break;                \
    }
#include "all_attributes.def"
#undef ATTR
#undef TYPE_ATTR
#undef NOCONTENT_ATTR
                // parse attr with no content
            case TK_align: {
                if (!ParseAlignAttrs(st.GetAttrs())) {
                    return false;
                }
                break;
            }
            default:
                return true;
        }  // switch
        lexer.NextToken();
    } while (true);
}

// for non-variable type attribute specification.
bool MIRParser::ParseTypeAttrs(TypeAttrs &attrs)
{
    do {
        switch (lexer.GetTokenKind()) {
// parse attr without no content
#define TYPE_ATTR
#define NOCONTENT_ATTR
#define ATTR(X)                  \
    case TK_##X:                 \
        attrs.SetAttr(ATTR_##X); \
        break;
#include "all_attributes.def"
#undef ATTR
#undef TYPE_ATTR
#undef NOCONTENT_ATTR
                // parse attr with no content
            case TK_align: {
                if (!ParseAlignAttrs(attrs)) {
                    return false;
                }
                break;
            }
            case TK_pack: {
                attrs.SetAttr(ATTR_pack);
                if (!ParsePackAttrs()) {
                    return false;
                }
                attrs.SetPack(static_cast<uint32>(lexer.GetTheIntVal()));
                break;
            }
            default:
                return true;
        }  // switch
        lexer.NextToken();
    } while (true);
}

bool MIRParser::ParseFieldAttrs(FieldAttrs &attrs)
{
    do {
        switch (lexer.GetTokenKind()) {
// parse attr without no content
#define FIELD_ATTR
#define NOCONTENT_ATTR
#define ATTR(X)                     \
    case TK_##X:                    \
        attrs.SetAttr(FLDATTR_##X); \
        break;
#include "all_attributes.def"
#undef ATTR
#undef NOCONTENT_ATTR
#undef FIELD_ATTR
                // parse attr with no content
            case TK_align: {
                if (!CheckAlignTk()) {
                    return false;
                }
                attrs.SetAlign(lexer.GetTheIntVal());
                break;
            }
            case TK_pack: {
                attrs.SetAttr(FLDATTR_pack);
                if (!ParsePackAttrs()) {
                    return false;
                }
                break;
            }
            default:
                return true;
        }  // switch
        lexer.NextToken();
    } while (true);
}

bool MIRParser::ParseFuncAttrs(FuncAttrs &attrs)
{
    do {
        FuncAttrKind attrKind;
        TokenKind currentToken = lexer.GetTokenKind();
        switch (currentToken) {
#define FUNC_ATTR
#define ATTR(X)                  \
    case TK_##X: {               \
        attrKind = FUNCATTR_##X; \
        attrs.SetAttr(attrKind); \
        break;                   \
    }
#include "all_attributes.def"
#undef ATTR
#undef FUNC_ATTR
            default:
                return true;
        }  // switch
        if (currentToken != TK_alias && currentToken != TK_section && currentToken != TK_constructor_priority &&
            currentToken != TK_destructor_priority) {
            lexer.NextToken();
            continue;
        }
        if (lexer.NextToken() != TK_lparen) {
            return false;
        }
        lexer.NextToken();
        SetAttrContent(attrs, attrKind, lexer);
        if (lexer.NextToken() != TK_rparen) {
            return false;
        }
        lexer.NextToken();
    } while (true);
}

void MIRParser::SetAttrContent(FuncAttrs &attrs, FuncAttrKind x, const MIRLexer &mirLexer)
{
    switch (x) {
        case FUNCATTR_alias: {
            attrs.SetAliasFuncName(mirLexer.GetName());
            break;
        }
        case FUNCATTR_section: {
            attrs.SetPrefixSectionName(mirLexer.GetName());
            break;
        }
        case FUNCATTR_constructor_priority: {
            attrs.SetConstructorPriority(static_cast<int32>(mirLexer.GetTheIntVal()));
            break;
        }
        case FUNCATTR_destructor_priority: {
            attrs.SetDestructorPriority(static_cast<int32>(mirLexer.GetTheIntVal()));
            break;
        }
        default:
            break;
    }
}

bool MIRParser::ParsePrimType(TyIdx &tyIdx)
{
    PrimType primType = GetPrimitiveType(lexer.GetTokenKind());
    if (primType == kPtyInvalid) {
        tyIdx = TyIdx(0);
        Error("ParsePrimType failed, invalid token");
        return false;
    }
    lexer.NextToken();
    tyIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(primType))->GetTypeIndex();
    return true;
}

bool MIRParser::ParseTypeParam(TyIdx &definedTyIdx)
{
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    MIRTypeParam typeParm(strIdx);
    definedTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&typeParm);
    lexer.NextToken();
    return true;
}

// LB not handled in binary format
bool MIRParser::ParseDefinedTypename(TyIdx &definedTyIdx, MIRTypeKind kind)
{
    TokenKind tk = lexer.GetTokenKind();
    if (!IsVarName(tk)) {
        Error("expect global or local token parsing typedef type but get ");
        return false;
    }
    std::string nameStr = lexer.GetName();
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(nameStr);
    // check if type already exist
    definedTyIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
    TyIdx prevTypeIdx(0);
    if (definedTyIdx) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(definedTyIdx);
        if (type->IsStructType()) {
            auto *stype = static_cast<MIRStructType *>(type);
            // check whether need to update from incomplete class to interface
            if (stype->GetKind() == kind) {
                lexer.NextToken();
                return true;
            }
            prevTypeIdx = definedTyIdx;
        }
    }
    if (tk == TK_gname) {
        definedTyIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
        if (definedTyIdx == 0u) {
            if (kind == kTypeInterface || kind == kTypeInterfaceIncomplete) {
                MIRInterfaceType interfaceType(kTypeInterfaceIncomplete);
                interfaceType.SetNameStrIdx(strIdx);
                definedTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&interfaceType);
                mod.AddClass(definedTyIdx);
                mod.AddExternStructType(definedTyIdx);
                mod.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, definedTyIdx);
            } else if (kind == kTypeClass || kind == kTypeClassIncomplete || IsClassInterfaceTypeName(nameStr)) {
                MIRClassType classType(kTypeClassIncomplete);
                classType.SetNameStrIdx(strIdx);
                definedTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&classType);
                mod.AddClass(definedTyIdx);
                mod.AddExternStructType(definedTyIdx);
                mod.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, definedTyIdx);
            } else {
                MIRTypeByName nameType(strIdx);
                definedTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&nameType);
                mod.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, definedTyIdx);
            }
        }
    } else {
        definedTyIdx = mod.CurFunction()->GetTyIdxFromGStrIdx(strIdx);
        if (definedTyIdx == 0u) {
            MIRTypeByName nameType(strIdx);
            definedTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&nameType);
            mod.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, definedTyIdx);
        }
    }
    // replace prevTypeIdx with definedTyIdx
    if (prevTypeIdx != 0u && prevTypeIdx != definedTyIdx) {
        // replace all uses of prevTypeIdx by tyIdx in type_table_
        typeDefIdxMap[prevTypeIdx] = definedTyIdx;
        // remove prevTypeIdx from classlist
        mod.RemoveClass(prevTypeIdx);
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::ParsePointType(TyIdx &tyIdx)
{
    TokenKind pdtk;
    if (lexer.GetTokenKind() == TK_func) {  // a function pointer
        pdtk = TK_func;
    } else if (lexer.GetTokenKind() != TK_asterisk) {
        Error("expect * for point type but get ");
        return false;
    } else {
        pdtk = lexer.NextToken();
    }
    TyIdx pointTypeIdx(0);
    if (IsPrimitiveType(pdtk)) {
        if (!ParsePrimType(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_asterisk) {  // a point type
        if (!ParsePointType(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_lbrack) {  // a array type
        if (!ParseArrayType(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_struct || pdtk == TK_union || pdtk == TK_structincomplete) {
        if (!ParseStructType(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_class || pdtk == TK_classincomplete) {
        if (!ParseClassType(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_gname) {
        if (!ParseDefinedTypename(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_langle) {
        if (!ParseDerivedType(pointTypeIdx)) {
            return false;
        }
    } else if (pdtk == TK_func) {
        lexer.NextToken();
        if (!ParseFuncType(pointTypeIdx)) {
            return false;
        }
    } else {
        Error("unexpect type ");
        return false;
    }
    DEBUG_ASSERT(pointTypeIdx != 0u, "something wrong with parsing element type ");
    PrimType pty = mod.IsJavaModule() ? PTY_ref : GetExactPtrPrimType();
    if (pdtk == maple::TK_constStr) {
        pty = GetExactPtrPrimType();
    }
    MIRPtrType pointType(pointTypeIdx, pty);  // use reference type here
    if (!ParseTypeAttrs(pointType.GetTypeAttrs())) {
        Error("bad type attribute in pointer type specification");
        return false;
    }
    tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&pointType);
    return true;
}

// used in parsing the parameter list (types only, without parameter names)
// in function pointer specification and member function prototypes inside
// structs and classes
bool MIRParser::ParseFuncType(TyIdx &tyIdx)
{
    // parse function attributes
    FuncAttrs fAttrs;
    if (lexer.GetTokenKind() != TK_lparen) {
        if (!ParseFuncAttrs(fAttrs)) {
            Error("bad function attribute specification in function type at ");
            return false;
        }
    }

    // parse parameters
    if (lexer.GetTokenKind() != TK_lparen) {
        Error("expect ( parse function type parameters but get ");
        return false;
    }
    std::vector<TyIdx> vecTyIdx;
    std::vector<TypeAttrs> vecAttrs;
    TokenKind tokenKind = lexer.NextToken();
    bool varargs = false;
    while (tokenKind != TK_rparen) {
        if (tokenKind == TK_dotdotdot) {
            if (vecTyIdx.size() == 0) {
                Error("variable arguments can only appear after fixed parameters ");
                return false;
            }
            varargs = true;
            tokenKind = lexer.NextToken();
            if (tokenKind != TK_rparen) {
                Error("expect ) after ... but get");
                return false;
            }
            break;
        }
        TyIdx tyIdxTmp(0);
        if (!ParseType(tyIdxTmp)) {
            Error("expect type parsing function parameters ");
            return false;
        }
        TypeAttrs typeAttrs;
        if (!ParseTypeAttrs(typeAttrs)) {
            Error("bad attribute in function parameter type at ");
            return false;
        }
        tokenKind = lexer.GetTokenKind();
        if (tokenKind == TK_coma) {
            tokenKind = lexer.NextToken();
            if (tokenKind == TK_rparen) {
                Error("syntax error, meeting ,) expect another type after , or ) without , ");
                return false;
            }
        }
        if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdxTmp)->HasTypeParam()) {
            typeAttrs.SetAttr(ATTR_generic);
        }
        vecTyIdx.push_back(tyIdxTmp);
        vecAttrs.push_back(typeAttrs);
    }
    // parse return type
    lexer.NextToken();
    TyIdx retTyIdx(0);
    if (!ParseType(retTyIdx)) {
        Error("expect return type for function type but get ");
        return false;
    }
    TypeAttrs retTypeAttrs;
    if (!ParseTypeAttrs(retTypeAttrs)) {
        Error("bad attribute in function ret type at ");
        return false;
    }
    MIRFuncType functype(retTyIdx, vecTyIdx, vecAttrs, retTypeAttrs);
    functype.funcAttrs = fAttrs;
    if (varargs) {
        functype.SetVarArgs();
    }
    tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&functype);
    return true;
}

// parse the generic type instantiation vector enclosed inside braces; syntax
// is: { <type-param> = <real-type> [, <type-param> = <real-type>] }
// where the contents enclosed in [ and ] can occur 0 or more times
bool MIRParser::ParseGenericInstantVector(MIRInstantVectorType &insVecType)
{
    TokenKind tokenKind;
    TyIdx typeParmIdx;
    do {
        tokenKind = lexer.NextToken();  // skip the lbrace or comma
        if (!ParseTypeParam(typeParmIdx)) {
            Error("type parameter incorrectly specified in generic type/function instantiation at ");
            return false;
        }
        tokenKind = lexer.GetTokenKind();
        if (tokenKind != TK_eqsign) {
            Error("missing = in generic type/function instantiation at ");
            return false;
        }
        tokenKind = lexer.NextToken();  // skip the =
        TyIdx realTyIdx;
        if (!ParseType(realTyIdx)) {
            Error("error parsing type in generic type/function instantiation at ");
            return false;
        }
        insVecType.AddInstant(TypePair(typeParmIdx, realTyIdx));
        tokenKind = lexer.GetTokenKind();
        if (tokenKind == TK_rbrace) {
            lexer.NextToken();  // skip the rbrace
            return true;
        }
    } while (tokenKind == TK_coma);
    Error("error parsing generic type/function instantiation at ");
    return false;
}

bool MIRParser::ParseDerivedType(TyIdx &tyIdx, MIRTypeKind kind, const GStrIdx &strIdx)
{
    if (lexer.GetTokenKind() != TK_langle) {
        Error("expect langle but get ");
        return false;
    }
    TokenKind ltk = lexer.NextToken();
    if (IsPrimitiveType(ltk)) {
        if (!ParsePrimType(tyIdx)) {
            Error("ParseDerivedType failed when parsing tyIdx at ");
            return false;
        }
    } else {
        switch (ltk) {
            case TK_asterisk:  // point type
            case TK_func:
                if (!ParsePointType(tyIdx)) {
                    Error("point type wrong when parsing derived type at ");
                    return false;
                }
                break;
            case TK_lbrack:  // array type
                if (!ParseArrayType(tyIdx)) {
                    Error("array type wrong when parsing derived type at ");
                    return false;
                }
                break;
            case TK_struct:            // struct type
            case TK_structincomplete:  // structincomplete type
            case TK_union:             // union type
                if (!ParseStructType(tyIdx, strIdx)) {
                    Error("struct/union type wrong when parsing derived type at ");
                    return false;
                }
                break;
            case TK_class:  // class type
            case TK_classincomplete:
                if (!ParseClassType(tyIdx, strIdx)) {
                    Error("class type wrong when parsing derived type at ");
                    return false;
                }
                break;
            case TK_interface:  // interface type
            case TK_interfaceincomplete:
                if (!ParseInterfaceType(tyIdx, strIdx)) {
                    Error("interface type wrong when parsing derived type at ");
                    return false;
                }
                break;
            case TK_lname:  // local type
            case TK_gname:  // global type
                if (!ParseDefinedTypename(tyIdx, kind)) {
                    Error("type name wrong when parsing derived type at ");
                    return false;
                }
                if (lexer.GetTokenKind() == TK_lbrace) {
                    MIRGenericInstantType genericinstty(tyIdx);
                    if (!ParseGenericInstantVector(genericinstty)) {
                        Error("error parsing generic type instantiation at ");
                        return false;
                    }
                    tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&genericinstty);
                }
                break;
            case TK_typeparam:
                if (!ParseTypeParam(tyIdx)) {
                    Error("type parameter wrong when parsing derived type at ");
                    return false;
                }
                break;
            default:
                Error("expect type token but get ");
                return false;
        }
    }
    // parse >
    if (lexer.GetTokenKind() != TK_rangle) {
        Error("expect > parse derived type but get ");
        return false;
    }
    lexer.NextToken();
    return true;
}

void MIRParser::FixForwardReferencedTypeForOneAgg(MIRType *type)
{
    if (type->GetKind() == kTypePointer) {
        auto *ptrType = static_cast<MIRPtrType *>(type);
        std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(ptrType->GetPointedTyIdx());
        if (it != typeDefIdxMap.end()) {
            ptrType->SetPointedTyIdx(it->second);
        }
    } else if (type->GetKind() == kTypeArray) {
        MIRArrayType *arrayType = static_cast<MIRArrayType *>(type);
        std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(arrayType->GetElemTyIdx());
        if (it != typeDefIdxMap.end()) {
            arrayType->SetElemTyIdx(it->second);
        }
    } else if (type->GetKind() == kTypeFArray || type->GetKind() == kTypeJArray) {
        MIRFarrayType *arrayType = static_cast<MIRFarrayType *>(type);
        std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(arrayType->GetElemTyIdx());
        if (it != typeDefIdxMap.end()) {
            arrayType->SetElemtTyIdx(it->second);
        }
    } else if (type->GetKind() == kTypeStruct || type->GetKind() == kTypeStructIncomplete ||
               type->GetKind() == kTypeUnion || type->GetKind() == kTypeClass ||
               type->GetKind() == kTypeClassIncomplete || type->GetKind() == kTypeInterface ||
               type->GetKind() == kTypeInterfaceIncomplete) {
        if (type->GetKind() == kTypeClass || type->GetKind() == kTypeClassIncomplete) {
            auto *classType = static_cast<MIRClassType *>(type);
            std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(classType->GetParentTyIdx());
            if (it != typeDefIdxMap.end()) {
                classType->SetParentTyIdx(it->second);
            }
            for (size_t j = 0; j < classType->GetInterfaceImplemented().size(); ++j) {
                std::map<TyIdx, TyIdx>::iterator it2 = typeDefIdxMap.find(classType->GetNthInterfaceImplemented(j));
                if (it2 != typeDefIdxMap.end()) {
                    classType->SetNthInterfaceImplemented(j, it2->second);
                }
            }
        } else if (type->GetKind() == kTypeInterface || type->GetKind() == kTypeInterfaceIncomplete) {
            auto *interfaceType = static_cast<MIRInterfaceType *>(type);
            for (uint32 j = 0; j < interfaceType->GetParentsTyIdx().size(); ++j) {
                std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(interfaceType->GetParentsElementTyIdx(j));
                if (it != typeDefIdxMap.end()) {
                    interfaceType->SetParentsElementTyIdx(j, it->second);
                }
            }
        }
        auto *structType = static_cast<MIRStructType *>(type);
        for (uint32 j = 0; j < structType->GetFieldsSize(); ++j) {
            TyIdx fieldTyIdx = structType->GetElemTyIdx(j);
            std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(fieldTyIdx);
            if (it != typeDefIdxMap.end()) {
                structType->SetElemtTyIdx(j, it->second);
            }
        }
        for (size_t j = 0; j < structType->GetStaticFields().size(); ++j) {
            TyIdx fieldTyIdx = structType->GetStaticElemtTyIdx(j);
            std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(fieldTyIdx);
            if (it != typeDefIdxMap.end()) {
                structType->SetStaticElemtTyIdx(j, it->second);
            }
        }
        for (size_t j = 0; j < structType->GetMethods().size(); ++j) {
            TyIdx methodTyIdx = structType->GetMethodsElement(j).second.first;
            std::map<TyIdx, TyIdx>::iterator it = typeDefIdxMap.find(methodTyIdx);
            if (it != typeDefIdxMap.end()) {
                structType->SetMethodTyIdx(j, it->second);
            }
        }
    }
}

void MIRParser::FixupForwardReferencedTypeByMap()
{
    for (size_t i = 1; i < GlobalTables::GetTypeTable().GetTypeTable().size(); ++i) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(i));
        FixForwardReferencedTypeForOneAgg(type);
    }
}

bool MIRParser::ParseTypedef()
{
    bool isLocal = paramParseLocalType;
    if (lexer.GetTokenKind() != TK_type) {
        Error("expect type but get ");
        return false;
    }
    TokenKind tokenKind = lexer.NextToken();
    if (tokenKind != TK_gname && tokenKind != TK_lname) {
        Error("expect type name but get ");
        return false;
    }
    const std::string &name = lexer.GetName();
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    TyIdx prevTyIdx;
    TyIdx tyIdx(0);
    if (tokenKind == TK_gname) {
        if (isLocal) {
            Error("A local type must use local type name ");
            return false;
        }
        prevTyIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
        if (prevTyIdx != 0u) {
            MIRType *prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(prevTyIdx);
            if (!mod.IsCModule()) {
                CHECK_FATAL(prevType->IsStructType(), "type error");
            }
            if ((prevType->GetKind() != kTypeByName) && !prevType->IsIncomplete()) {
                // allow duplicated type def if kKeepFirst is set which is the default
                if (options & kKeepFirst) {
                    lexer.NextToken();
                    Warning("redefined global type");
                    if (!ParseDerivedType(tyIdx, kTypeUnknown)) {
                        Error("error passing derived type at ");
                        return false;
                    }
                    return true;
                } else {
                    Error("redefined global type");
                    return false;
                }
            }
        }
    } else {
        if (!isLocal) {
            Error("A global type must use global type name ");
            return false;
        }
        prevTyIdx = mod.CurFunction()->GetTyIdxFromGStrIdx(strIdx);
        if (prevTyIdx != 0u) {
            MIRType *prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(prevTyIdx);
            if ((prevType->GetKind() != kTypeByName) && !prevType->IsIncomplete()) {
                Error("redefined local type name ");
                return false;
            }
        }
    }
    // at this point,if prev_tyidx is not zero, this type name has been
    // forward-referenced
    tokenKind = lexer.NextToken();
    tyIdx = kInitTyIdx;
    if (IsPrimitiveType(tokenKind)) {
        if (!ParsePrimType(tyIdx)) {
            Error("expect primitive type after typedef but get ");
            return false;
        }
    } else if (!ParseDerivedType(tyIdx, kTypeUnknown, strIdx)) {
        Error("error passing derived type at ");
        return false;
    }
    // for class/interface types, prev_tyidx could also be set during processing
    // so we check again right before SetGStrIdxToTyIdx
    if (isLocal) {
        prevTyIdx = mod.CurFunction()->GetTyIdxFromGStrIdx(strIdx);
        mod.CurFunction()->SetGStrIdxToTyIdx(strIdx, tyIdx);
        DEBUG_ASSERT(GlobalTables::GetTypeTable().GetTypeTable().empty() == false, "container check");
        if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetNameStrIdx() == 0u) {
            GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->SetNameIsLocal(true);
        }
    } else {
        prevTyIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
        mod.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, tyIdx);
        mod.PushbackTypeDefOrder(strIdx);
    }

    if (prevTyIdx != TyIdx(0) && prevTyIdx != tyIdx) {
        // replace all uses of prev_tyidx by tyIdx in typeTable
        typeDefIdxMap[prevTyIdx] = tyIdx;  // record the real tydix
        // remove prev_tyidx from classlist
        mod.RemoveClass(prevTyIdx);
    }

    // Merge class or interface type at the cross-module level
    DEBUG_ASSERT(GlobalTables::GetTypeTable().GetTypeTable().empty() == false, "container check");
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (!isLocal && (type->GetKind() == kTypeClass || type->GetKind() == kTypeClassIncomplete ||
                     type->GetKind() == kTypeInterface || type->GetKind() == kTypeInterfaceIncomplete)) {
        prevTyIdx = GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(strIdx);
        if (prevTyIdx == 0u) {
            GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(strIdx, tyIdx);
        }
        // setup eh root type
        MIRType *ehType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        if (mod.GetThrowableTyIdx() == 0u &&
            (ehType->GetKind() == kTypeClass || ehType->GetKind() == kTypeClassIncomplete)) {
            GStrIdx ehTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
                namemangler::GetInternalNameLiteral(namemangler::kJavaLangObjectStr));
            if (ehTypeNameIdx == ehType->GetNameStrIdx()) {
                mod.SetThrowableTyIdx(tyIdx);
            }
        }
    }
    return true;
}

bool MIRParser::ParseJavaClassInterface(MIRSymbol &symbol, bool isClass)
{
    TokenKind tk = lexer.NextToken();
    if (tk != TK_gname) {
        Error("expect global name for javaclass but get ");
        return false;
    }
    symbol.SetNameStrIdx(lexer.GetName());
    if (!GlobalTables::GetGsymTable().AddToStringSymbolMap(symbol)) {
        Error("duplicate symbol name used in javainterface at ");
        return false;
    }
    lexer.NextToken();
    TyIdx tyidx(0);
    if (!ParseType(tyidx)) {
        Error("ParseType failed trying parsing the type");
        return false;
    }
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyidx);
    if (isClass && type->GetKind() != kTypeClass && type->GetKind() != kTypeClassIncomplete) {
        Error("type in javaclass declaration must be of class type at ");
        return false;
    } else if (!isClass && type->GetKind() != kTypeInterface && type->GetKind() != kTypeInterfaceIncomplete) {
        Error("type in javainterface declaration must be of interface type at ");
        return false;
    }
    symbol.SetTyIdx(tyidx);
    if (!ParseTypeAttrs(symbol.GetAttrs())) {
        Error("bad type attribute in variable declaration at ");
        return false;
    }
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyidx)->HasTypeParam()) {
        symbol.SetAttr(ATTR_generic);
    }
    return true;
}

bool MIRParser::ParseStorageClass(MIRSymbol &symbol) const
{
    TokenKind tk = lexer.GetTokenKind();
    switch (tk) {
        case TK_fstatic:
            symbol.SetStorageClass(kScFstatic);
            return true;
        case TK_pstatic:
            symbol.SetStorageClass(kScPstatic);
            return true;
        case TK_extern:
            symbol.SetStorageClass(kScExtern);
            return true;
        default:
            break;
    }
    return false;
}

bool MIRParser::ParseDeclareReg(MIRSymbol &symbol, const MIRFunction &func)
{
    TokenKind tk = lexer.GetTokenKind();
    // i.e, reg %1 u1
    if (tk != TK_reg) {  // reg
        Error("expect reg bug get ");
        return false;
    }
    TokenKind regNumTK = lexer.NextToken();
    if (regNumTK != TK_preg) {
        Error("expect preg but get");
        return false;
    }
    uint32 thePRegNO = static_cast<uint32>(lexer.GetTheIntVal());
    lexer.NextToken();
    // parse ty
    TyIdx tyIdx(0);
    if (!ParseType(tyIdx)) {
        Error("ParseDeclarePreg failed while parsing the type");
        return false;
    }
    DEBUG_ASSERT(tyIdx > 0u, "parse declare preg failed");
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetKind() == kTypeByName) {
        Error("type in var declaration cannot be forward-referenced at ");
        return false;
    }
    symbol.SetTyIdx(tyIdx);
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    PregIdx pRegIdx = func.GetPregTab()->EnterPregNo(thePRegNO, mirType->GetPrimType(), mirType);
    MIRPregTable *pRegTab = func.GetPregTab();
    MIRPreg *preg = pRegTab->PregFromPregIdx(pRegIdx);
    preg->SetPrimType(mirType->GetPrimType());
    symbol.SetPreg(preg);
    if (!ParseVarTypeAttrs(symbol)) {
        Error("bad type attribute in variable declaration at ");
        return false;
    }
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->HasTypeParam()) {
        symbol.SetAttr(ATTR_generic);
        mod.CurFunction()->SetAttr(FUNCATTR_generic);
    }
    return true;
}

bool MIRParser::ParseDeclareVarInitValue(MIRSymbol &symbol)
{
    TokenKind tk = lexer.GetTokenKind();
    // take a look if there are any initialized values
    if (tk == TK_eqsign) {
        // parse initialized values
        MIRConst *mirConst = nullptr;
        lexer.NextToken();
        if (!ParseInitValue(mirConst, symbol.GetTyIdx(), mod.IsCModule())) {
            Error("wrong initialization value at ");
            return false;
        }
        symbol.SetKonst(mirConst);
    }
    return true;
}

bool MIRParser::ParseDeclareVar(MIRSymbol &symbol)
{
    TokenKind tk = lexer.GetTokenKind();
    // i.e, var %i i32
    if (tk != TK_var && tk != TK_tempvar) {  // var
        Error("expect var but get ");
        return false;
    }
    bool isLocal = symbol.IsLocal();
    // %i
    TokenKind nameTk = lexer.NextToken();
    if (isLocal) {
        if (nameTk == TK_static) {
            symbol.SetStorageClass(kScPstatic);
            nameTk = lexer.NextToken();
        }
        if (nameTk != TK_lname) {
            Error("expect local name but get ");
            return false;
        }
    } else {
        if (nameTk != TK_gname) {
            Error("expect global name but get ");
            return false;
        }
    }
    std::string symbolStrName = lexer.GetName();
    GStrIdx symbolStrID = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(symbolStrName);
    symbol.SetNameStrIdx(symbolStrID);
    tk = lexer.NextToken();
    if (ParseStorageClass(symbol)) {
        lexer.NextToken();
    }
    // i32
    TyIdx tyIdx(0);
    if (!ParseType(tyIdx)) {
        Error("ParseDeclareVar failed when parsing the type");
        return false;
    }
    DEBUG_ASSERT(tyIdx > 0u, "parse declare var failed ");
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetKind() == kTypeByName) {
        Error("type in var declaration cannot be forward-referenced at ");
        return false;
    }
    symbol.SetTyIdx(tyIdx);
    /* parse section/register attribute from inline assembly */
    if (lexer.GetTokenKind() == TK_section) {
        lexer.NextToken();
        if (lexer.GetTokenKind() != TK_lparen) {
            Error("expect ( for section attribute but get ");
            return false;
        }
        lexer.NextToken();
        if (lexer.GetTokenKind() != TK_string) {
            Error("expect string literal for section attribute but get ");
            return false;
        }
        UStrIdx literalStrIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        symbol.sectionAttr = literalStrIdx;
        lexer.NextToken();
        if (lexer.GetTokenKind() != TK_rparen) {
            Error("expect ) for section attribute but get ");
            return false;
        }
        lexer.NextToken();
    } else if (lexer.GetTokenKind() == TK_asmattr) { /* Specifying Registers for Local Variables */
        lexer.NextToken();
        if (lexer.GetTokenKind() != TK_lparen) {
            Error("expect ( for register inline-asm attribute but get ");
            return false;
        }
        lexer.NextToken();
        if (lexer.GetTokenKind() != TK_string) {
            Error("expect string literal for section attribute but get ");
            return false;
        }
        UStrIdx literalStrIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        symbol.asmAttr = literalStrIdx;

        lexer.NextToken();
        if (lexer.GetTokenKind() != TK_rparen) {
            Error("expect ) for section attribute but get ");
            return false;
        }
        lexer.NextToken();
    }
    if (!ParseVarTypeAttrs(symbol)) {
        Error("bad type attribute in variable declaration at ");
        return false;
    }
    if (symbol.GetStorageClass() == kScExtern && symbol.IsStatic()) {
        const std::string &staticFieldName = symbol.GetName();
        constexpr int kPosOffset = 3;
        size_t pos = staticFieldName.find(namemangler::kClassNameSplitterStr) + kPosOffset;
        if (pos != 0 && pos != std::string::npos) {
            std::string className = staticFieldName.substr(0, pos);
            MIRSymbol *classSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
                GlobalTables::GetStrTable().GetStrIdxFromName(className));
            if (classSt != nullptr) {
                symbol.SetStorageClass(kScGlobal);
            }
        }
    }
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->HasTypeParam()) {
        symbol.SetAttr(ATTR_generic);
        if (isLocal) {
            mod.CurFunction()->SetAttr(FUNCATTR_generic);
        }
    }
    return true;
}

bool MIRParser::ParseDeclareFormal(FormalDef &formalDef)
{
    TokenKind tk = lexer.GetTokenKind();
    if (tk != TK_var && tk != TK_reg) {
        return false;
    }
    TokenKind nameTk = lexer.NextToken();
    if (tk == TK_var) {
        if (nameTk != TK_lname) {
            Error("expect local name but get ");
            return false;
        }
        formalDef.formalStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    } else {  // tk == TK_reg
        if (nameTk != TK_preg) {
            Error("expect preg but get ");
            return false;
        }
        formalDef.formalStrIdx =
            GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(std::to_string(lexer.GetTheIntVal()));
    }
    (void)lexer.NextToken();
    if (!ParseType(formalDef.formalTyIdx)) {
        Error("ParseDeclareFormal failed when parsing the type");
        return false;
    }
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDef.formalTyIdx)->GetKind() == kTypeByName) {
        Error("type in var declaration cannot be forward-referenced at ");
        return false;
    }
    if (!ParseTypeAttrs(formalDef.formalAttrs)) {
        Error("ParseDeclareFormal failed when parsing type attributes");
        return false;
    }
    return true;
}

bool MIRParser::ParsePrototype(MIRFunction &func, MIRSymbol &funcSymbol, TyIdx &funcTyIdx)
{
    if (lexer.GetTokenKind() == TK_lbrace) {
        if (mod.GetFlavor() < kMmpl) {
            Error("funcion prototype missing for non-MMPL flavor of Maple IR");
            return false;
        }
        // mmpl flavor has no prototype declaration, return normally
        return true;
    }
    std::vector<TyIdx> vecTy;      // for storing the parameter types
    std::vector<TypeAttrs> vecAt;  // for storing the parameter type attributes
    // this part for parsing the argument list and return type
    if (lexer.GetTokenKind() != TK_lparen) {
        Error("expect ( for func but get ");
        return false;
    }
    // parse parameters
    bool varArgs = false;
    TokenKind pmTk = lexer.NextToken();
    while (pmTk != TK_rparen) {
        if (pmTk == TK_dotdotdot) {
            varArgs = true;
            func.SetVarArgs();
            pmTk = lexer.NextToken();
            if (pmTk != TK_rparen) {
                Error("expect ) after ... but get");
                return false;
            }
            break;
        } else {
            FormalDef formalDef;
            if (!ParseDeclareFormal(formalDef)) {
                Error("ParsePrototype expects formal parameter declaration");
                return false;
            }
            func.GetFormalDefVec().push_back(formalDef);
            vecTy.push_back(formalDef.formalTyIdx);
            vecAt.push_back(formalDef.formalAttrs);
            pmTk = lexer.GetTokenKind();
            if (pmTk == TK_coma) {
                pmTk = lexer.NextToken();
                if (pmTk == TK_rparen) {
                    Error("\',\' cannot be followed by");
                    return false;
                }
            }
        }
    }

    // parse return type
    lexer.NextToken();
    TyIdx tyIdx(0);
    if (!ParseType(tyIdx)) {
        return false;
    }
    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    func.SetReturnStruct(*retType);
    MIRType *funcType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(tyIdx, vecTy, vecAt, varArgs);
    funcTyIdx = funcType->GetTypeIndex();
    funcSymbol.SetTyIdx(funcTyIdx);
    func.SetMIRFuncType(static_cast<MIRFuncType *>(funcType));
    return true;
}

// if prototype declaration, only create the symbol and type;
// if a redeclaration, re-process the symbol and type declaration;
// if function declaration, link MIRFunction to ->mod in addition
bool MIRParser::ParseFunction(uint32 fileIdx)
{
    TokenKind tokenKind = lexer.GetTokenKind();
    if (tokenKind != TK_func) {
        Error("expect func but get ");
        return false;
    }
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_gname && lexer.GetTokenKind() != TK_fname) {
        Error("expect function name for func but get ");
        return false;
    }
    const std::string &funcName = lexer.GetName();
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
    MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
    MIRFunction *func = nullptr;
    lexer.NextToken();
    FuncAttrs funcAttrs;
    if (!ParseFuncAttrs(funcAttrs)) {
        Error("bad function attribute in function declaration at ");
        return false;
    }
    if (funcSymbol != nullptr && funcSymbol->GetSKind() == kStFunc && funcSymbol->IsNeedForwDecl() == true &&
        !funcSymbol->GetFunction()->GetBody()) {
        SetSrcPos(funcSymbol->GetSrcPosition(), lexer.GetLineNum());
    }
    if (funcSymbol != nullptr) {
        // there has been an earlier forward declaration, so check consistency
        if (funcSymbol->GetStorageClass() != kScText || funcSymbol->GetSKind() != kStFunc) {
            Error("redeclaration of name as func in ");
            return false;
        }
        if (funcSymbol->GetFunction()->GetBody()) {
            // Function definition has been processed. Here it may be
            // another declaration due to multi-mpl merge. If this
            // is indeed another definition, we will throw error.
            MIRSymbol *tmpSymbol = mod.GetMemPool()->New<MIRSymbol>();
            tmpSymbol->SetStorageClass(kScText);
            tmpSymbol->SetAppearsInCode(true);
            tmpSymbol->SetSKind(kStFunc);
            MIRFunction *tmpFunc = mod.GetMemPool()->New<MIRFunction>(&mod, tmpSymbol->GetStIdx());
            tmpSymbol->SetFunction(tmpFunc);
            TyIdx tmpTyIdx;
            if (!ParsePrototype(*tmpFunc, *tmpSymbol, tmpTyIdx)) {
                return false;
            }
            if (lexer.GetTokenKind() == TK_lbrace) {
                Error("function body defined second time in ");
                return false;
            }
            return true;
        }
        // Skip attribute checking
        func = funcSymbol->GetFunction();
        func->ClearFormals();

        // update with current attr
        if (funcAttrs.GetAttrFlag()) {
            if (func->IsIpaSeen()) {
                funcAttrs.SetAttr(FUNCATTR_ipaseen);
            }
            if (func->IsNoDefEffect()) {
                funcAttrs.SetAttr(FUNCATTR_nodefeffect);
            }
            if (func->IsNoRetGlobal()) {
                funcAttrs.SetAttr(FUNCATTR_noretglobal);
            }
            if (func->IsPure()) {
                funcAttrs.SetAttr(FUNCATTR_pure);
            }
            if (func->IsNoThrowException()) {
                funcAttrs.SetAttr(FUNCATTR_nothrow_exception);
            }
            if (func->IsNoDefArgEffect()) {
                funcAttrs.SetAttr(FUNCATTR_nodefargeffect);
            }
            if (func->IsNoRetArg()) {
                funcAttrs.SetAttr(FUNCATTR_noretarg);
            }
            if (func->IsNoPrivateDefEffect()) {
                funcAttrs.SetAttr(FUNCATTR_noprivate_defeffect);
            }
            func->SetFuncAttrs(funcAttrs);
        }
    } else {
        maple::MIRBuilder mirBuilder(&mod);
        funcSymbol = mirBuilder.CreateSymbol(TyIdx(0), strIdx, kStFunc, kScText, nullptr, kScopeGlobal);
        SetSrcPos(funcSymbol->GetSrcPosition(), lexer.GetLineNum());
        func = mod.GetMemPool()->New<MIRFunction>(&mod, funcSymbol->GetStIdx());
        func->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
        GlobalTables::GetFunctionTable().GetFuncTable().push_back(func);
        funcSymbol->SetFunction(func);
        func->SetFuncAttrs(funcAttrs);
    }
    func->SetFileIndex(fileIdx);
    curFunc = func;
    if (mod.IsJavaModule()) {
        func->SetBaseClassFuncNames(funcSymbol->GetNameStrIdx());
    }
    TyIdx funcTyidx;
    if (!ParsePrototype(*func, *funcSymbol, funcTyidx)) {
        return false;
    }
    if (lexer.GetTokenKind() == TK_lbrace) {  // #2 parse Function body
        funcSymbol->SetAppearsInCode(true);
        // when parsing func in mplt_inline file, set it as tmpunused.
        if (options & kParseInlineFuncBody) {
            funcSymbol->SetIsTmpUnused(true);
        }
        definedLabels.clear();
        mod.SetCurFunction(func);
        mod.AddFunction(func);
        // set maple line number for function
        func->GetSrcPosition().SetMplLineNum(lexer.GetLineNum());
        // initialize source line number to be 0
        // to avoid carrying over info from previous function
        firstLineNum = 0;
        lastLineNum = 0;
        lastColumnNum = 0;
        func->NewBody();
        BlockNode *block = nullptr;
        safeRegionFlag.push(curFunc->IsSafe());
        auto IsParseSucc = ParseStmtBlock(block);
        safeRegionFlag.pop();
        if (!IsParseSucc) {
            Error("ParseFunction failed when parsing stmt block");
            ResetCurrentFunction();
            return false;
        }
        func->SetBody(block);

        // set source file number for function
        func->GetSrcPosition().SetLineNum(firstLineNum);
        func->GetSrcPosition().SetFileNum(lastFileNum);
        func->GetSrcPosition().SetColumn(lastColumnNum);
        // check if any local type name is undefined
        for (auto it : func->GetGStrIdxToTyIdxMap()) {
            MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second);
            if (type->GetKind() == kTypeByName) {
                std::string strStream;
                const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first);
                strStream += "type %";
                strStream += name;
                strStream += " used but not defined\n";
                message += strStream;
                ResetCurrentFunction();
                return false;
            }
        }
    }
    ResetCurrentFunction();
    return true;
}

bool MIRParser::ParseInitValue(MIRConstPtr &theConst, TyIdx tyIdx, bool allowEmpty)
{
    TokenKind tokenKind = lexer.GetTokenKind();
    MIRType &type = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (tokenKind != TK_lbrack) {  // scalar
        MIRConst *mirConst = nullptr;
        if (IsConstValue(tokenKind)) {
            if (!ParseScalarValue(mirConst, type)) {
                Error("ParseInitValue expect scalar value");
                return false;
            }
            lexer.NextToken();
        } else if (IsConstAddrExpr(tokenKind)) {
            if (!ParseConstAddrLeafExpr(mirConst)) {
                Error("ParseInitValue expect const addr expr");
                return false;
            }
        } else {
            Error("initialiation value expected but get ");
            return false;
        }
        theConst = mirConst;
    } else {  // aggregates
        FixForwardReferencedTypeForOneAgg(&type);
        if (type.GetKind() == kTypeArray) {
            auto &arrayType = static_cast<MIRArrayType &>(type);
            MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType.GetElemTyIdx());
            MIRAggConst *newConst = mod.GetMemPool()->New<MIRAggConst>(mod, type);
            theConst = newConst;
            tokenKind = lexer.NextToken();
            if (tokenKind == TK_rbrack) {
                if (allowEmpty) {
                    lexer.NextToken();
                    return true;
                } else {
                    Error("illegal empty initialization for array at ");
                    return false;
                }
            }
            do {
                // parse single const or another dimension array
                MIRConst *subConst = nullptr;
                if (IsConstValue(tokenKind)) {
                    if (!ParseScalarValue(subConst, *elemType)) {
                        Error("ParseInitValue expect scalar value");
                        return false;
                    }
                    lexer.NextToken();
                } else if (IsConstAddrExpr(tokenKind)) {
                    if (!ParseConstAddrLeafExpr(subConst)) {
                        Error("ParseInitValue expect const addr expr");
                        return false;
                    }
                } else if (tokenKind == TK_lbrack) {
                    if (elemType->GetKind() == kTypeStruct && arrayType.GetDim() == 1) {
                        if (!ParseInitValue(subConst, arrayType.GetElemTyIdx(), allowEmpty)) {
                            Error("initializaton value wrong when parsing structure array ");
                            return false;
                        }
                    } else {
                        TyIdx elemTyIdx;
                        if (arrayType.GetDim() == 1) {
                            elemTyIdx = elemType->GetTypeIndex();
                        } else {
                            std::vector<uint64> sizeSubArray;
                            for (uint16 i = 1; i < arrayType.GetDim(); ++i) {
                                sizeSubArray.push_back(arrayType.GetSizeArrayItem(i));
                            }
                            MIRArrayType subArrayType(elemType->GetTypeIndex(), sizeSubArray);
                            elemTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&subArrayType);
                        }
                        if (!ParseInitValue(subConst, elemTyIdx, allowEmpty)) {
                            Error("initializaton value wrong when parsing sub array ");
                            return false;
                        }
                    }
                } else {
                    Error("expect const value or group of const values but get ");
                    return false;
                }
                newConst->AddItem(subConst, 0);
                // parse comma or rbrack
                tokenKind = lexer.GetTokenKind();
                if (tokenKind == TK_coma) {
                    tokenKind = lexer.NextToken();
                    if (tokenKind == TK_rbrack) {
                        Error("not expect, followed by ] ");
                        return false;
                    }
                }
            } while (tokenKind != TK_rbrack);
            lexer.NextToken();
        } else if (type.GetKind() == kTypeStruct || type.GetKind() == kTypeUnion) {
            MIRAggConst *newConst = mod.GetMemPool()->New<MIRAggConst>(mod, type);
            uint32 theFieldIdx;
            TyIdx fieldTyIdx;
            theConst = newConst;
            tokenKind = lexer.NextToken();
            if (tokenKind == TK_rbrack) {
                if (allowEmpty) {
                    lexer.NextToken();
                    return true;
                } else {
                    Error("illegal empty initialization for struct at ");
                    return false;
                }
            }
            do {
                if (lexer.GetTokenKind() != TK_intconst) {
                    Error("expect field ID in struct initialization but get ");
                    return false;
                }
                theFieldIdx = static_cast<uint32>(lexer.GetTheIntVal());
                if (lexer.NextToken() != TK_eqsign) {
                    Error("expect = after field ID in struct initialization but get ");
                    return false;
                }
                FieldPair thepair = static_cast<MIRStructType &>(type).GetFields()[theFieldIdx - 1];
                fieldTyIdx = thepair.second.first;
                if (fieldTyIdx == 0u) {
                    Error("field ID out of range at struct initialization at ");
                    return false;
                }
                tokenKind = lexer.NextToken();
                MIRConst *subConst = nullptr;
                if (IsConstValue(tokenKind)) {
                    if (!ParseScalarValue(subConst, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx))) {
                        Error("ParseInitValue expect scalar value");
                        return false;
                    }
                    lexer.NextToken();
                } else if (IsConstAddrExpr(tokenKind)) {
                    if (!ParseConstAddrLeafExpr(subConst)) {
                        Error("ParseInitValue expect const addr expr");
                        return false;
                    }
                } else if (tokenKind == TK_lbrack) {
                    if (!ParseInitValue(subConst, fieldTyIdx, allowEmpty)) {
                        Error("parse init value wrong when parse sub struct ");
                        return false;
                    }
                } else {
                    Error("expect const value or group of const values but get ");
                    return false;
                }
                DEBUG_ASSERT(subConst != nullptr, "subConst is null in MIRParser::ParseInitValue");
                newConst->AddItem(subConst, theFieldIdx);
                tokenKind = lexer.GetTokenKind();
                // parse comma or rbrack
                if (tokenKind == TK_coma) {
                    tokenKind = lexer.NextToken();
                    if (tokenKind == TK_rbrack) {
                        Error("not expect \',\' followed by ] ");
                        return false;
                    }
                }
            } while (tokenKind != TK_rbrack);
            lexer.NextToken();
        } else {
            Error("non-struct should not have aggregate initialization in ");
            return false;
        }
    }
    return true;
}

bool MIRParser::ParseFuncInfo()
{
    if (lexer.GetTokenKind() != TK_lbrace) {
        Error("expect left brace after funcinfo but get ");
        return false;
    }
    MIRFunction *func = mod.CurFunction();
    TokenKind tokenKind = lexer.NextToken();
    while (tokenKind == TK_label) {
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        tokenKind = lexer.NextToken();
        if (tokenKind == TK_intconst) {
            uint32 fieldVal = lexer.GetTheIntVal();
            func->PushbackMIRInfo(MIRInfoPair(strIdx, fieldVal));
            func->PushbackIsString(false);
        } else if (tokenKind == TK_string) {
            GStrIdx literalStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
            func->PushbackMIRInfo(MIRInfoPair(strIdx, literalStrIdx));
            func->PushbackIsString(true);
        } else {
            Error("illegal value after funcinfo field name at ");
            return false;
        }
        tokenKind = lexer.NextToken();
        if (tokenKind == TK_rbrace) {
            lexer.NextToken();
            return true;
        }
        if (tokenKind == TK_coma) {
            tokenKind = lexer.NextToken();
        } else {
            Error("expect comma after funcinfo field value but get ");
            return false;
        }
    }
    Error("expect field name in funcinfo but get ");
    return false;
}

bool MIRParser::ParsePosition(SrcPosition &pos)
{
    TokenKind nameTk = lexer.NextToken();
    if (nameTk != TK_lparen) {
        Error("expect ( in SrcPos but get ");
        return false;
    }
    nameTk = lexer.NextToken();
    if (nameTk != TK_intconst) {
        Error("expect int in SrcPos but get ");
        return false;
    }
    uint32 i = static_cast<uint32>(lexer.GetTheIntVal());
    pos.SetFileNum(static_cast<uint16>(i));
    nameTk = lexer.NextToken();
    if (nameTk != TK_coma) {
        Error("expect comma in SrcPos but get ");
        return false;
    }

    nameTk = lexer.NextToken();
    if (nameTk != TK_intconst) {
        Error("expect int in SrcPos but get ");
        return false;
    }
    i = static_cast<uint32>(lexer.GetTheIntVal());
    pos.SetLineNum(i);
    nameTk = lexer.NextToken();
    if (nameTk != TK_coma) {
        Error("expect comma in SrcPos but get ");
        return false;
    }

    nameTk = lexer.NextToken();
    if (nameTk != TK_intconst) {
        Error("expect int in SrcPos but get ");
        return false;
    }
    i = static_cast<uint32>(lexer.GetTheIntVal());
    pos.SetColumn(static_cast<uint16>(i));
    nameTk = lexer.NextToken();
    if (nameTk != TK_rparen) {
        Error("expect ) in SrcPos but get ");
        return false;
    }

    return true;
}

bool MIRParser::ParseOneScope(MIRScope &scope)
{
    TokenKind nameTk = lexer.GetTokenKind();
    if (nameTk != TK_SCOPE) {
        Error("expect SCOPE but get ");
        return false;
    }
    nameTk = lexer.NextToken();
    if (nameTk != TK_langle) {
        Error("expect < in SCOPE but get ");
        return false;
    }
    SrcPosition low;
    if (!ParsePosition(low)) {
        Error("parsePosition low failed when parsing SCOPE ");
        return false;
    }
    nameTk = lexer.NextToken();
    if (nameTk != TK_coma) {
        Error("expect comma in SCOPE but get ");
        return false;
    }
    SrcPosition high;
    if (!ParsePosition(high)) {
        Error("parsePosition high failed when parsing SCOPE ");
        return false;
    }
    nameTk = lexer.NextToken();
    if (nameTk != TK_rangle) {
        Error("expect > in SCOPE but get ");
        return false;
    }
    nameTk = lexer.NextToken();
    if (nameTk != TK_lbrace) {
        Error("expect { in SCOPE but get ");
        return false;
    }
    scope.SetRange(low, high);
    nameTk = lexer.NextToken();
    while (1) {
        bool status = false;
        switch (lexer.GetTokenKind()) {
            case TK_ALIAS: {
                GStrIdx strIdx;
                MIRAliasVars aliasVar;
                status = ParseOneAlias(strIdx, aliasVar);
                if (status) {
                    scope.SetAliasVarMap(strIdx, aliasVar);
                }
                break;
            }
            case TK_SCOPE: {
                // initial level 1
                MIRScope *scp = mod.GetMemPool()->New<MIRScope>(&mod, 1);
                status = ParseOneScope(*scp);
                if (status) {
                    scope.AddScope(scp);
                } else {
                    delete scp;
                }
                break;
            }
            default:
                break;
        }
        if (!status) {
            break;
        }
    }
    nameTk = lexer.GetTokenKind();
    if (nameTk != TK_rbrace) {
        Error("expect } in SCOPE but get ");
        return false;
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseScope(StmtNodePtr &stmt)
{
    // initial level 1
    MIRScope *scp = mod.GetMemPool()->New<MIRScope>(&mod, 1);
    bool status = ParseOneScope(*scp);
    if (status) {
        mod.CurFunction()->GetScope()->AddScope(scp);
    } else {
        delete scp;
    }
    return true;
}

bool MIRParser::ParseOneAlias(GStrIdx &strIdx, MIRAliasVars &aliasVar)
{
    TokenKind nameTk = lexer.GetTokenKind();
    if (nameTk != TK_ALIAS) {
        Error("expect ALIAS but get ");
        return false;
    }
    nameTk = lexer.NextToken();
    if (nameTk != TK_lname) {
        Error("expect local in ALIAS but get ");
        return false;
    }
    strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    nameTk = lexer.NextToken();
    bool isLocal;
    if (nameTk == TK_lname) {
        isLocal = true;
    } else if (nameTk == TK_gname) {
        isLocal = false;
    } else {
        Error("expect name in ALIAS but get ");
        return false;
    }
    GStrIdx mplStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    lexer.NextToken();
    TyIdx tyIdx(0);
    if (!ParseType(tyIdx)) {
        Error("parseType failed when parsing ALIAS ");
        return false;
    }
    GStrIdx signStrIdx(0);
    if (lexer.GetTokenKind() == TK_string) {
        signStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        lexer.NextToken();
    }
    aliasVar.mplStrIdx = mplStrIdx;
    aliasVar.tyIdx = tyIdx;
    aliasVar.isLocal = isLocal;
    aliasVar.sigStrIdx = signStrIdx;
    return true;
}

bool MIRParser::ParseAlias(StmtNodePtr &stmt)
{
    GStrIdx strIdx;
    MIRAliasVars aliasVar;

    ParseOneAlias(strIdx, aliasVar);

    mod.CurFunction()->SetAliasVarMap(strIdx, aliasVar);
    return true;
}

uint8 *MIRParser::ParseWordsInfo(uint32 size)
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_eqsign) {
        Error("expect = after it but get ");
        return nullptr;
    }
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_lbrack) {
        Error("expect [ for it but get ");
        return nullptr;
    }
    uint8 *origp = static_cast<uint8 *>(mod.GetMemPool()->Malloc(BlockSize2BitVectorSize(size)));
    // initialize it based on the input
    for (uint32 *p = reinterpret_cast<uint32 *>(origp); lexer.NextToken() == TK_intconst; ++p) {
        *p = static_cast<uint32>(lexer.GetTheIntVal());
    }
    if (lexer.GetTokenKind() != TK_rbrack) {
        Error("expect ] at end of globalwordstypetagged but get ");
        return nullptr;
    }
    lexer.NextToken();
    return origp;
}

static void GenJStringType(MIRModule &module)
{
    MIRStructType metaClassType(kTypeStructIncomplete);
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("dummy");
    metaClassType.GetFields().push_back(FieldPair(strIdx, TyIdxFieldAttrPair(TyIdx(PTY_ref), FieldAttrs())));
    strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("__class_meta__");
    metaClassType.SetNameStrIdx(strIdx);
    TyIdx tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&metaClassType);
    // Global
    module.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, tyIdx);
    DEBUG_ASSERT(GlobalTables::GetTypeTable().GetTypeTable().empty() == false, "container check");
}

bool MIRParser::ParseMIR(std::ifstream &mplFile)
{
    std::ifstream *origFile = lexer.GetFile();
    // parse mplfile
    lexer.SetFile(mplFile);
    // try to read the first line
    if (lexer.ReadALine() < 0) {
        lexer.lineNum = 0;
    } else {
        lexer.lineNum = 1;
    }
    // for optimized functions file
    bool status = ParseMIR(0, kParseOptFunc);
    // restore airFile
    lexer.SetFile(*origFile);
    return status;
}

bool MIRParser::ParseInlineFuncBody(std::ifstream &mplFile)
{
    std::ifstream *origFile = lexer.GetFile();
    lexer.SetFile(mplFile);
    // try to read the first line
    if (lexer.ReadALine() < 0) {
        lexer.lineNum = 0;
    } else {
        lexer.lineNum = 1;
    }
    // parse mplFile
    bool status = ParseMIR(0, kParseOptFunc | kParseInlineFuncBody);
    // restore airFile
    lexer.SetFile(*origFile);
    return status;
}

bool MIRParser::ParseSrcLang(MIRSrcLang &srcLang)
{
    PrepareParsingMIR();
    bool atEof = false;
    lexer.NextToken();
    while (!atEof) {
        paramTokenKind = lexer.GetTokenKind();
        if (paramTokenKind == TK_eof) {
            atEof = true;
        } else if (paramTokenKind == TK_srclang) {
            lexer.NextToken();
            if (lexer.GetTokenKind() != TK_intconst) {
                Error("expect integer after srclang but get ");
                return false;
            }
            srcLang = static_cast<MIRSrcLang>(lexer.GetTheIntVal());
            return true;
        } else {
            lexer.NextToken();
        }
    }
    return false;
}

bool MIRParser::ParseMIR(uint32 fileIdx, uint32 option, bool isIPA, bool isComb)
{
    if ((option & kParseOptFunc) == 0) {
        PrepareParsingMIR();
    }
    if (option != 0) {
        this->options |= option;
    }
    // profiling setup
    mod.SetWithProfileInfo(((this->options & kWithProfileInfo) != 0));
    bool atEof = false;
    paramFileIdx = fileIdx;
    paramIsIPA = isIPA;
    paramIsComb = isComb;
    lexer.NextToken();
    while (!atEof) {
        paramTokenKind = lexer.GetTokenKind();
        std::map<TokenKind, FuncPtrParseMIRForElem>::iterator itFuncPtr = funcPtrMapForParseMIR.find(paramTokenKind);
        if (itFuncPtr == funcPtrMapForParseMIR.end()) {
            if (paramTokenKind == TK_eof) {
                atEof = true;
            } else {
                Error("expect func or var but get ");
                return false;
            }
        } else {
            if (!(this->*(itFuncPtr->second))()) {
                return false;
            }
        }
    }
    // fix the typedef type
    FixupForwardReferencedTypeByMap();
    // check if any global type name is undefined
    for (auto it = mod.GetTypeNameTab()->GetGStrIdxToTyIdxMap().begin();
         it != mod.GetTypeNameTab()->GetGStrIdxToTyIdxMap().end(); ++it) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it->second);
        CHECK_FATAL(type != nullptr, "type is null");
        if (type->GetKind() == kTypeByName) {
            std::string strStream;
            const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it->first);
            strStream += "type $";
            strStream += name;
            strStream += " used but not defined\n";
            message += strStream;
            return false;
        }
    }
    if (!isIPA && isComb) {
        for (auto it = paramImportFileList.begin(); it != paramImportFileList.end(); ++it) {
            BinaryMplt binMplt(mod);
            std::string importFilename = *it;
            if (!binMplt.Import(importFilename, false, true)) {  // not a binary mplt
                std::ifstream mpltFile(importFilename);
                if (!mpltFile.is_open()) {
                    FATAL(kLncFatal, "cannot open MPLT file: %s\n", importFilename.c_str());
                }
                bool failedParse = !ParseMPLT(mpltFile, importFilename);
                mpltFile.close();
                if (failedParse) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::map<TokenKind, MIRParser::FuncPtrParseMIRForElem> MIRParser::InitFuncPtrMapForParseMIR()
{
    std::map<TokenKind, MIRParser::FuncPtrParseMIRForElem> funcPtrMap;
    funcPtrMap[TK_func] = &MIRParser::ParseMIRForFunc;
    funcPtrMap[TK_tempvar] = &MIRParser::ParseMIRForVar;
    funcPtrMap[TK_var] = &MIRParser::ParseMIRForVar;
    funcPtrMap[TK_javaclass] = &MIRParser::ParseMIRForClass;
    funcPtrMap[TK_javainterface] = &MIRParser::ParseMIRForInterface;
    funcPtrMap[TK_type] = &MIRParser::ParseTypedef;
    funcPtrMap[TK_flavor] = &MIRParser::ParseMIRForFlavor;
    funcPtrMap[TK_srclang] = &MIRParser::ParseMIRForSrcLang;
    funcPtrMap[TK_globalmemsize] = &MIRParser::ParseMIRForGlobalMemSize;
    funcPtrMap[TK_globalmemmap] = &MIRParser::ParseMIRForGlobalMemMap;
    funcPtrMap[TK_globalwordstypetagged] = &MIRParser::ParseMIRForGlobalWordsTypeTagged;
    funcPtrMap[TK_globalwordsrefcounted] = &MIRParser::ParseMIRForGlobalWordsRefCounted;
    funcPtrMap[TK_id] = &MIRParser::ParseMIRForID;
    funcPtrMap[TK_numfuncs] = &MIRParser::ParseMIRForNumFuncs;
    funcPtrMap[TK_entryfunc] = &MIRParser::ParseMIRForEntryFunc;
    funcPtrMap[TK_fileinfo] = &MIRParser::ParseMIRForFileInfo;
    funcPtrMap[TK_filedata] = &MIRParser::ParseMIRForFileData;
    funcPtrMap[TK_srcfileinfo] = &MIRParser::ParseMIRForSrcFileInfo;
    funcPtrMap[TK_import] = &MIRParser::ParseMIRForImport;
    funcPtrMap[TK_importpath] = &MIRParser::ParseMIRForImportPath;
    funcPtrMap[TK_asmdecl] = &MIRParser::ParseMIRForAsmdecl;
    funcPtrMap[TK_LOC] = &MIRParser::ParseLoc;
    return funcPtrMap;
}

bool MIRParser::ParseMIRForFunc()
{
    curFunc = nullptr;
    if (!ParseFunction(paramFileIdx)) {
        return false;
    }
    // when parsing function in mplt_inline file, set fromMpltInline as true.
    if ((this->options & kParseInlineFuncBody) && curFunc) {
        curFunc->SetFromMpltInline(true);
        return true;
    }
    if ((this->options & kParseOptFunc) && curFunc) {
        curFunc->SetAttr(FUNCATTR_optimized);
        mod.AddOptFuncs(curFunc);
    }
    return true;
}

bool MIRParser::TypeCompatible(TyIdx typeIdx1, TyIdx typeIdx2)
{
    if (typeIdx1 == typeIdx2) {
        return true;
    }
    MIRType *type1 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx1);
    MIRType *type2 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx2);
    if (type1 == nullptr || type2 == nullptr) {
        // this means we saw the use of this symbol before def.
        return false;
    }
    while (type1->IsMIRPtrType() || type1->IsMIRArrayType()) {
        if (type1->IsMIRPtrType()) {
            CHECK_FATAL(type2->IsMIRPtrType(), "Error");
            type1 = static_cast<MIRPtrType *>(type1)->GetPointedType();
            type2 = static_cast<MIRPtrType *>(type2)->GetPointedType();
        } else {
            CHECK_FATAL(type2->IsMIRArrayType(), "Error");
            auto *arrayType1 = static_cast<MIRArrayType *>(type1);
            auto *arrayType2 = static_cast<MIRArrayType *>(type2);
            if (arrayType1->IsIncompleteArray() || arrayType2->IsIncompleteArray()) {
                return true;
            }
            type1 = static_cast<MIRArrayType *>(type1)->GetElemType();
            type2 = static_cast<MIRArrayType *>(type2)->GetElemType();
        }
    }
    if (type1 == type2) {
        return true;
    }
    if (type1->IsIncomplete() || type2->IsIncomplete()) {
        return true;
    }
    return false;
}

bool MIRParser::IsTypeIncomplete(MIRType *type)
{
    if (type->IsIncomplete()) {
        return true;
    }
    while (type->IsMIRPtrType() || type->IsMIRArrayType()) {
        if (type->IsMIRPtrType()) {
            type = static_cast<MIRPtrType *>(type)->GetPointedType();
        } else {
            auto *arrayType = static_cast<MIRArrayType *>(type);
            if (arrayType->IsIncompleteArray()) {
                return true;
            }
            type = static_cast<MIRArrayType *>(type)->GetElemType();
        }
    }
    return type->IsIncomplete();
}

bool MIRParser::ParseMIRForVar()
{
    MIRSymbol st(0, kScopeGlobal);
    st.SetStorageClass(kScGlobal);
    st.SetSKind(kStVar);
    if (paramTokenKind == TK_tempvar) {
        st.SetIsTmp(true);
    }
    if (!ParseDeclareVar(st)) {
        return false;
    }
    MIRSymbol *prevSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(st.GetNameStrIdx());
    if (prevSt != nullptr) {
        bool ret = TypeCompatible(prevSt->GetTyIdx(), st.GetTyIdx());
#ifdef DEBUG
        if (!ret) {
            LogInfo::MapleLogger() << "\n=====prevSymbol===== \n";
            prevSt->Dump(false, 0);
            LogInfo::MapleLogger() << "\n=====curSymbol===== \n";
            st.Dump(false, 0);
            LogInfo::MapleLogger() << "\n=====   end   ===== \n";
        }
#endif
        if (ret &&
            (prevSt->GetStorageClass() == st.GetStorageClass() || prevSt->GetStorageClass() == kScExtern ||
             st.GetStorageClass() == kScExtern) &&
            prevSt->GetSKind() == st.GetSKind() && (prevSt->GetKonst() == nullptr || st.GetKonst() == nullptr)) {
            // previously declared: accumulate new information to the symbol
            prevSt->GetAttrs().SetAttrFlag(prevSt->GetAttrs().GetAttrFlag() | st.GetAttrs().GetAttrFlag());
            if (prevSt->GetKonst() == nullptr) {
                prevSt->SetKonst(st.GetKonst());
            }
            if (prevSt->GetStorageClass() == kScExtern) {
                prevSt->SetStorageClass(st.GetStorageClass());
            }
            if (prevSt->sectionAttr == UStrIdx(0)) {
                prevSt->sectionAttr = st.sectionAttr;
            }
            if (IsTypeIncomplete(prevSt->GetType())) {
                prevSt->SetTyIdx(st.GetTyIdx());
            }
        } else if (prevSt->GetSKind() == maple::kStVar && prevSt->GetStorageClass() == maple::kScInvalid) {
            prevSt->SetStorageClass(kScGlobal);
            prevSt->SetAttrs(st.GetAttrs());
            prevSt->SetNameStrIdx(st.GetNameStrIdx());
            prevSt->sectionAttr = st.sectionAttr;
            prevSt->SetValue(st.GetValue());
            prevSt->SetTyIdx(st.GetTyIdx());
            SetSrcPos(prevSt->GetSrcPosition(), lexer.GetLineNum());
        }
        if (!ParseDeclareVarInitValue(*prevSt)) {
            return false;
        }
    } else {  // seeing the first time
        maple::MIRBuilder mirBuilder(&mod);
        MIRSymbol *newst = mirBuilder.CreateSymbol(st.GetTyIdx(), st.GetNameStrIdx(), st.GetSKind(),
                                                   st.GetStorageClass(), nullptr, kScopeGlobal);
        // when parsing var in mplt_inline file, set it as tmpunused.
        if (this->options & kParseInlineFuncBody) {
            newst->SetIsTmpUnused(true);
        }
        newst->SetAttrs(st.GetAttrs());
        newst->SetNameStrIdx(st.GetNameStrIdx());
        newst->sectionAttr = st.sectionAttr;
        newst->SetValue(st.GetValue());
        SetSrcPos(newst->GetSrcPosition(), lexer.GetLineNum());
        if (!ParseDeclareVarInitValue(*newst)) {
            return false;
        }
    }
    if (mod.CurFunction() && mod.GetFunctionList().empty()) {
        mod.CurFunction()->ReleaseCodeMemory();
    }
    return true;
}

bool MIRParser::ParseMIRForClass()
{
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    DEBUG_ASSERT(st != nullptr, "st nullptr check");
    st->SetStorageClass(kScInvalid);
    st->SetSKind(kStJavaClass);
    return ParseJavaClassInterface(*st, true);
}

bool MIRParser::ParseMIRForInterface()
{
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    DEBUG_ASSERT(st != nullptr, "st nullptr check");
    st->SetStorageClass(kScInvalid);
    st->SetSKind(kStJavaInterface);
    return ParseJavaClassInterface(*st, false);
}

bool MIRParser::ParseMIRForFlavor()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after flavor but get ");
        return false;
    }
    mod.SetFlavor(static_cast<MIRFlavor>(lexer.GetTheIntVal()));
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForSrcLang()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after srclang but get ");
        return false;
    }
    mod.SetSrcLang(static_cast<MIRSrcLang>(lexer.GetTheIntVal()));
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForGlobalMemSize()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after globalmemsize but get ");
        return false;
    }
    mod.SetGlobalMemSize(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForGlobalMemMap()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_eqsign) {
        Error("expect = after globalmemmap but get ");
        return false;
    }
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_lbrack) {
        Error("expect [ for globalmemmap but get ");
        return false;
    }
    mod.SetGlobalBlockMap(static_cast<uint8 *>(mod.GetMemPool()->Malloc(mod.GetGlobalMemSize())));
    // initialize globalblkmap based on the input
    for (uint32 *p = reinterpret_cast<uint32 *>(mod.GetGlobalBlockMap()); lexer.NextToken() == TK_intconst; ++p) {
        *p = static_cast<uint32>(lexer.GetTheIntVal());
    }
    if (lexer.GetTokenKind() != TK_rbrack) {
        Error("expect ] at end of globalmemmap but get ");
        return false;
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForGlobalWordsTypeTagged()
{
    uint8 *typeAddr = ParseWordsInfo(mod.GetGlobalMemSize());
    if (typeAddr == nullptr) {
        Error("parser error for globalwordstypetagged");
        return false;
    }
    mod.SetGlobalWordsTypeTagged(typeAddr);
    return true;
}

bool MIRParser::ParseMIRForGlobalWordsRefCounted()
{
    uint8 *refAddr = ParseWordsInfo(mod.GetGlobalMemSize());
    if (refAddr == nullptr) {
        Error("parser error for globalwordsrefcounted");
        return false;
    }
    mod.SetGlobalWordsRefCounted(refAddr);
    return true;
}

bool MIRParser::ParseMIRForID()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after id but get ");
        return false;
    }
    mod.SetID(static_cast<uint16>(lexer.GetTheIntVal()));
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForNumFuncs()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after numfuncs but get ");
        return false;
    }
    mod.SetNumFuncs(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForEntryFunc()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_fname) {
        Error("expect function name for func but get ");
        return false;
    }
    mod.SetEntryFuncName(lexer.GetName());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForFileInfo()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_lbrace) {
        Error("expect left brace after fileInfo but get ");
        return false;
    }
    TokenKind tk = lexer.NextToken();
    while (tk == TK_label) {
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        tk = lexer.NextToken();
        if (tk == TK_intconst) {
            uint32 fieldVal = lexer.GetTheIntVal();
            mod.PushFileInfoPair(MIRInfoPair(strIdx, fieldVal));
            mod.PushFileInfoIsString(false);
        } else if (tk == TK_string) {
            GStrIdx litStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
            mod.PushFileInfoPair(MIRInfoPair(strIdx, litStrIdx));
            mod.PushFileInfoIsString(true);
        } else {
            Error("illegal value after fileInfo field name at ");
            return false;
        }
        tk = lexer.NextToken();
        if (tk == TK_rbrace) {
            lexer.NextToken();
            return true;
        }
        if (tk == TK_coma) {
            tk = lexer.NextToken();
        } else {
            Error("expect comma after fileInfo field value but get ");
            return false;
        }
    }
    Error("expect field name in fileInfo but get ");
    return false;
}

bool MIRParser::ParseMIRForFileData()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_lbrace) {
        Error("expect left brace after fileData but get ");
        return false;
    }
    TokenKind tk = lexer.NextToken();
    while (tk == TK_label) {
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
        tk = lexer.NextToken();
        std::vector<uint8> data;
        while (tk == TK_intconst) {
            uint32 fieldVal = lexer.GetTheIntVal();
            data.push_back(fieldVal);
            tk = lexer.NextToken();
        }
        mod.PushbackFileData(MIRDataPair(strIdx, data));
        if (tk == TK_coma) {
            tk = lexer.NextToken();
        } else if (tk == TK_rbrace) {
            lexer.NextToken();
            return true;
        } else {
            Error("expect comma after fileData field value but get ");
            return false;
        }
    }
    Error("expect field name in fileData but get ");
    return false;
}

bool MIRParser::ParseMIRForSrcFileInfo()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_lbrace) {
        Error("expect left brace after fileInfo but get ");
        return false;
    }
    TokenKind tk = lexer.NextToken();
    while (tk == TK_intconst) {
        uint32 fieldVal = lexer.GetTheIntVal();
        tk = lexer.NextToken();
        if (tk == TK_string) {
            GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
            mod.PushbackFileInfo(MIRInfoPair(strIdx, fieldVal));
        } else {
            Error("illegal value after srcfileinfo field name at ");
            return false;
        }
        tk = lexer.NextToken();
        if (tk == TK_rbrace) {
            lexer.NextToken();
            return true;
        }
        if (tk == TK_coma) {
            tk = lexer.NextToken();
        } else {
            Error("expect comma after srcfileinfo field value but get ");
            return false;
        }
    }
    Error("expect field name in srcfileinfo but get ");
    return false;
}

bool MIRParser::ParseMIRForImport()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_string) {
        Error("expect file name string after import but get ");
        return false;
    }
    std::string importFileName = lexer.GetName();
    paramImportFileList.push_back(importFileName);
    // check illegal file name for the .mplt file
    std::string::size_type lastDot = importFileName.find_last_of(".");
    if (lastDot == std::string::npos || lastDot != importFileName.length() - strlen(".mplt")) {
        FATAL(kLncFatal, "MPLT file has no or wrong suffix: %s\n", importFileName.c_str());
    }
    if (importFileName.compare(lastDot, strlen(".mplt"), ".mplt") != 0) {
        FATAL(kLncFatal, "MPLT file has wrong suffix: %s\n", importFileName.c_str());
    }
    if (paramIsIPA && firstImport) {
        BinaryMplt *binMplt = new BinaryMplt(mod);
        mod.SetBinMplt(binMplt);
        if (!(*binMplt).Import(importFileName, paramIsIPA && !firstImport, paramIsComb)) {  // not a binary mplt
            std::ifstream mpltFile(importFileName);
            if (!mpltFile.is_open()) {
                FATAL(kLncFatal, "cannot open MPLT file: %s\n", importFileName.c_str());
            }
            bool failedParse = !ParseMPLT(mpltFile, importFileName);
            mpltFile.close();
            if (failedParse) {  // parse the mplt file
                return false;
            }
        }
        firstImport = false;
    } else {
        BinaryMplt binMplt(mod);
        if (!binMplt.Import(importFileName, paramIsIPA, false)) {  // not a binary mplt
            std::ifstream mpltFile(importFileName);
            if (!mpltFile.is_open()) {
                FATAL(kLncFatal, "cannot open MPLT file: %s\n", importFileName.c_str());
            }
            bool failedParse = !ParseMPLT(mpltFile, importFileName);
            mpltFile.close();
            if (failedParse) {  // parse the mplt file
                return false;
            }
        }
    }
    if (GlobalTables::GetStrTable().GetStrIdxFromName("__class_meta__") == 0u) {
        GenJStringType(mod);
    }
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(importFileName);
    auto it = mod.GetImportFiles().begin();
    (void)mod.GetImportFiles().insert(it, strIdx);
    // record the imported file for later reading summary info, if exists
    mod.PushbackImportedMplt(importFileName);
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForImportPath()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_string) {
        Error("expect path string after importpath but get ");
        return false;
    }
    GStrIdx litStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    mod.PushbackImportPath(litStrIdx);
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseMIRForAsmdecl()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_string) {
        Error("expect asm string after iasm but get ");
        return false;
    }
    mod.GetAsmDecls().emplace_back(MapleString(lexer.GetName(), mod.GetMemPool()));
    lexer.NextToken();
    return true;
}

void MIRParser::PrepareParsingMIR()
{
    dummyFunction = CreateDummyFunction();
    mod.SetCurFunction(dummyFunction);
    if (mod.IsNeedFile()) {
        lexer.PrepareForFile(mod.GetFileName());
    } else {
        lexer.PrepareForString(mod.GetFileText());
    }
}

void MIRParser::PrepareParsingMplt()
{
    lexer.PrepareForFile(mod.GetFileName());
}

bool MIRParser::ParseTypeFromString(const std::string &src, TyIdx &tyIdx)
{
    lexer.PrepareForString(src);
    return ParseType(tyIdx);
}

bool MIRParser::ParseMPLT(std::ifstream &mpltFile, const std::string &importFileName)
{
    // save relevant values for the main input file
    std::ifstream *airFileSave = lexer.GetFile();
    int lineNumSave = static_cast<int>(lexer.lineNum);
    std::string modFileNameSave = mod.GetFileName();
    // set up to read next line from the import file
    lexer.curIdx = 0;
    lexer.currentLineSize = 0;
    lexer.SetFile(mpltFile);
    lexer.lineNum = 0;
    mod.SetFileName(importFileName);
    bool atEof = false;
    lexer.NextToken();
    while (!atEof) {
        TokenKind tokenKind = lexer.GetTokenKind();
        switch (tokenKind) {
            default: {
                Error("expect func or var but get ");
                return false;
            }
            case TK_eof:
                atEof = true;
                break;
            case TK_type:
                paramParseLocalType = false;
                if (!ParseTypedef()) {
                    return false;
                }
                break;
            case TK_var: {
                tokenKind = lexer.NextToken();
                if (tokenKind == TK_gname) {
                    std::string literalName = lexer.GetName();
                    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(literalName);
                    GlobalTables::GetConstPool().PutLiteralNameAsImported(strIdx);
                    lexer.NextToken();
                } else {
                    return false;
                }
                break;
            }
        }
    }
    // restore old values to continue reading from the main input file
    lexer.curIdx = 0;  // to force reading new line
    lexer.currentLineSize = 0;
    lexer.lineNum = static_cast<size_t>(lineNumSave);
    lexer.SetFile(*airFileSave);
    mod.SetFileName(modFileNameSave);
    return true;
}

bool MIRParser::ParseMPLTStandalone(std::ifstream &mpltFile, const std::string &importFileName)
{
    PrepareParsingMIR();
    if (!ParseMPLT(mpltFile, importFileName)) {
        return false;
    }
    lexer.lineNum = 0;
    // fix the typedef type
    FixupForwardReferencedTypeByMap();
    return true;
}

bool MIRParser::ParsePrototypeRemaining(MIRFunction &func, std::vector<TyIdx> &vecTyIdx,
                                        std::vector<TypeAttrs> &vecAttrs, bool &varArgs)
{
    TokenKind pmTk = lexer.GetTokenKind();
    while (pmTk != TK_rparen) {
        // parse ","
        if (pmTk != TK_coma) {
            Error("expect , after a declare var/preg but get");
            return false;
        }
        pmTk = lexer.NextToken();
        if (pmTk == TK_dotdotdot) {
            varArgs = true;
            func.SetVarArgs();
            pmTk = lexer.NextToken();
            if (pmTk != TK_rparen) {
                Error("expect ) after ... but get");
                return false;
            }
            break;
        }
        MIRSymbol *symbol = func.GetSymTab()->CreateSymbol(kScopeLocal);
        DEBUG_ASSERT(symbol != nullptr, "symbol nullptr check");
        symbol->SetStorageClass(kScFormal);
        TokenKind loopStTK = lexer.GetTokenKind();
        if (loopStTK == TK_reg) {
            symbol->SetSKind(kStPreg);
            if (!ParseDeclareReg(*symbol, func)) {
                Error("ParseFunction expect scalar value");
                return false;
            }
        } else {
            symbol->SetSKind(kStVar);
            if (!ParseDeclareVar(*symbol)) {
                Error("ParseFunction expect scalar value");
                return false;
            }
            (void)func.GetSymTab()->AddToStringSymbolMap(*symbol);
            if (!ParseDeclareVarInitValue(*symbol)) {
                return false;
            }
        }
        func.AddArgument(symbol);
        vecTyIdx.push_back(symbol->GetTyIdx());
        vecAttrs.push_back(symbol->GetAttrs());
        pmTk = lexer.GetTokenKind();
    }
    return true;
}

void MIRParser::EmitError(const std::string &fileName)
{
    if (!strlen(GetError().c_str())) {
        return;
    }
    mod.GetDbgInfo()->EmitMsg();
    ERR(kLncErr, "%s \n%s", fileName.c_str(), GetError().c_str());
}

void MIRParser::EmitWarning(const std::string &fileName)
{
    if (!strlen(GetWarning().c_str())) {
        return;
    }
    WARN(kLncWarn, "%s \n%s\n", fileName.c_str(), GetWarning().c_str());
}
}  // namespace maple
