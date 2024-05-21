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
    return false;
}

bool MIRParser::ParseType(TyIdx &tyIdx)
{
    TokenKind tk = lexer.GetTokenKind();
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
    if (mod.IsCharModule()) {
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
        elem.SubElemVecPushBack(e0);
        tk = lexer.NextToken();
        if (tk != TK_coma && tk != TK_rbrack) {
            Error("parsing pragma error: expecting , or ] but get ");
            return false;
        }
    }
    return true;
}

bool MIRParser::ParseClassType(TyIdx &styidx, const GStrIdx &strIdx)
{
    MIRTypeKind tkind = (lexer.GetTokenKind() == TK_class) ? kTypeClass : kTypeClassIncomplete;
    TyIdx parentTypeIdx(0);
    if (lexer.NextToken() == TK_langle) {
    }
    MIRClassType classType(tkind, strIdx);
    classType.SetParentTyIdx(parentTypeIdx);
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
        parents.push_back(parentTypeIdx);
        tk = lexer.GetTokenKind();
    }
    MIRInterfaceType interfaceType(tkind, strIdx);
    interfaceType.SetParentsTyIdx(parents);
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
        symbol.SetKonst(mirConst);
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

std::map<TokenKind, MIRParser::FuncPtrParseMIRForElem> MIRParser::InitFuncPtrMapForParseMIR()
{
    std::map<TokenKind, MIRParser::FuncPtrParseMIRForElem> funcPtrMap;
    funcPtrMap[TK_func] = &MIRParser::ParseMIRForFunc;
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
    funcPtrMap[TK_importpath] = &MIRParser::ParseMIRForImportPath;
    funcPtrMap[TK_asmdecl] = &MIRParser::ParseMIRForAsmdecl;
    funcPtrMap[TK_LOC] = &MIRParser::ParseLoc;
    return funcPtrMap;
}

bool MIRParser::ParseMIRForFunc()
{
    curFunc = nullptr;
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
