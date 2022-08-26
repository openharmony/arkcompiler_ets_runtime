/**
 * Copyright (c)  \ 2021-2022 Huawei Device Co.)  \ Ltd.
 * Licensed under the Apache License)  \ Version 2.0 (the "License")  \;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing)  \ software
 * distributed under the License is distributed on an "AS IS" BASIS)  \
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND)  \ either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ECMASCRIPT_COMPILER_ECMA_OPCODE_H
#define ECMASCRIPT_COMPILER_ECMA_OPCODE_H

#include <map>
#include <string>

namespace panda::ecmascript::kungfu {

#define ECMA_BYTECODE_LIST(V)  \
    V(NOP)  \
    V(DEBUGGER) \
    V(LDNAN)  \
    V(LDINFINITY)  \
    V(LDUNDEFINED)  \
    V(LDNULL)  \
    V(LDSYMBOL)  \
    V(LDGLOBAL)  \
    V(LDTRUE)  \
    V(LDFALSE)  \
    V(LDHOLE)  \
    V(LDNEWTARGET)  \
    V(LDLEXENV)  \
    V(POPLEXENV)  \
    V(GETUNMAPPEDARGS)  \
    V(LDMODULEVAR)  \
    V(GETITERATORNEXT)  \
    V(ITERNEXT)  \
    V(GETITERATOR)  \
    V(GETPROPITERATOR)  \
    V(ASYNCFUNCTIONENTER)  \
    V(LDTHIS)  \
    V(LDFUNCTION)  \
    V(CLOSEITERATOR)  \
    V(CREATEASYNCGENERATOROBJ)  \
    V(CREATEEMPTYOBJECT)  \
    V(CREATEEMPTYARRAY)  \
    V(CREATEGENERATOROBJ)  \
    V(CREATEITERRESULTOBJ)  \
    V(CREATEOBJECTWITHEXCLUDEDKEYS)  \
    V(ASYNCGENERATORRESOLVE)  \
    V(CREATEARRAYWITHBUFFER)  \
    V(CALLTHIS0)  \
    V(CALLTHIS1)  \
    V(CREATEOBJECTWITHBUFFER)  \
    V(CREATEREGEXPWITHLITERAL)  \
    V(NEWOBJAPPLY)  \
    V(NEWOBJRANGE)  \
    V(NEWLEXENV)  \
    V(NEWLEXENVWITHNAME)  \
    V(ADD2)  \
    V(SUB2)  \
    V(MUL2)  \
    V(DIV2)  \
    V(MOD2)  \
    V(EQ)  \
    V(NOTEQ)  \
    V(LESS)  \
    V(LESSEQ)  \
    V(GREATER)  \
    V(GREATEREQ)  \
    V(SHL2)  \
    V(SHR2)  \
    V(ASHR2)  \
    V(AND2)  \
    V(OR2)  \
    V(XOR2)  \
    V(EXP)  \
    V(TYPEOF)  \
    V(TONUMBER)  \
    V(TONUMERIC)  \
    V(NEG)  \
    V(NOT)  \
    V(INC)  \
    V(DEC)  \
    V(ISIN)  \
    V(INSTANCEOF)  \
    V(STRICTNOTEQ)  \
    V(STRICTEQ)  \
    V(ISTRUE)  \
    V(ISFALSE)  \
    V(CALLTHIS2)  \
    V(CALLTHIS3)  \
    V(CALLTHISRANGE)  \
    V(SUPERCALL)  \
    V(SUPERCALLTHISRANGE)  \
    V(SUPERCALLARROWRANGE)  \
    V(DEFINEFUNC)  \
    V(DEFINENCFUNC)  \
    V(DEFINEASYNCFUNC)  \
    V(CALLARG0)  \
    V(CALLSPREAD)  \
    V(SUPERCALLSPREAD)  \
    V(APPLY)  \
    V(CALLARGS2)  \
    V(CALLARGS3)  \
    V(CALLRANGE)  \
    V(DEFINEMETHOD)  \
    V(DEFINEGENERATORFUNC) \
    V(DEFINEASYNCGENERATORFUNC) \
    V(LDEXTERNALMODULEVAR)  \
    V(DEFINEGETTERSETTERBYVALUE)  \
    V(LDTHISBYNAME)  \
    V(STTHISBYNAME)  \
    V(LDTHISBYVALUE)  \
    V(STTHISBYVALUE)  \
    V(LDPATCHVAR)  \
    V(STPATCHVAR)  \
    V(DEFINECLASSWITHBUFFER)  \
    V(RESUMEGENERATOR)  \
    V(GETRESUMEMODE)  \
    V(GETTEMPLATEOBJECT)  \
    V(GETNEXTPROPNAME)  \
    V(JSTRICTEQZ)  \
    V(DELOBJPROP)  \
    V(SUSPENDGENERATOR)  \
    V(ASYNCFUNCTIONAWAITUNCAUGHT)  \
    V(COPYDATAPROPERTIES)  \
    V(STARRAYSPREAD)  \
    V(SETOBJECTWITHPROTO)  \
    V(LDOBJBYVALUE)  \
    V(STOBJBYVALUE)  \
    V(STOWNBYVALUE)  \
    V(LDSUPERBYVALUE)  \
    V(STSUPERBYVALUE)  \
    V(LDOBJBYINDEX)  \
    V(STOBJBYINDEX)  \
    V(STOWNBYINDEX)  \
    V(ASYNCFUNCTIONRESOLVE)  \
    V(ASYNCFUNCTIONREJECT)  \
    V(COPYRESTARGS)  \
    V(GETMODULENAMESPACE)  \
    V(STMODULEVAR)  \
    V(TRYLDGLOBALBYNAME)  \
    V(TRYSTGLOBALBYNAME)  \
    V(LDGLOBALVAR)  \
    V(STGLOBALVAR)  \
    V(LDOBJBYNAME)  \
    V(STOBJBYNAME)  \
    V(STOWNBYNAME)  \
    V(LDSUPERBYNAME)  \
    V(STSUPERBYNAME)  \
    V(LDLOCALMODULEVAR)  \
    V(STCONSTTOGLOBALRECORD)  \
    V(STLETTOGLOBALRECORD)  \
    V(CREATEOBJECTHAVINGMETHOD)  \
    V(STTOGLOBALRECORD)  \
    V(JNSTRICTEQZ)  \
    V(JEQNULL)  \
    V(STOWNBYVALUEWITHNAMESET)  \
    V(STOWNBYNAMEWITHNAMESET)  \
    V(LDBIGINT)  \
    V(LDASTR)  \
    V(JMP)  \
    V(JEQZ)  \
    V(JNEZ)  \
    V(JNENULL)  \
    V(LDA)  \
    V(STA)  \
    V(LDAI)  \
    V(FLDAI)  \
    V(RETURN)  \
    V(RETURNUNDEFINED)  \
    V(LDLEXVAR)  \
    V(JSTRICTEQNULL)  \
    V(STLEXVAR)  \
    V(CALLARG1)  \
    V(JNSTRICTEQNULL)  \
    V(JEQUNDEFINED)  \
    V(JNEUNDEFINED)  \
    V(JSTRICTEQUNDEFINED)  \
    V(JNSTRICTEQUNDEFINED)  \
    V(JEQ)  \
    V(JNE)  \
    V(JSTRICTEQ)  \
    V(JNSTRICTEQ)  \
    V(MOV)  \
    V(THROW)  \
    V(THROW_NOTEXISTS)  \
    V(THROW_PATTERNNONCOERCIBLE)  \
    V(THROW_DELETESUPERPROPERTY)  \
    V(THROW_CONSTASSIGNMENT)  \
    V(THROW_IFNOTOBJECT)  \
    V(THROW_UNDEFINEDIFHOLE)  \
    V(THROW_IFSUPERNOTCORRECTCALL)

enum class EcmaBytecode {
#define BYTECODE_ENUM(name) name,
    ECMA_BYTECODE_LIST(BYTECODE_ENUM)
#undef BYTECODE_ENUM
};

inline std::string GetEcmaBytecodeStr(EcmaBytecode opcode) {
    const std::map<EcmaBytecode, const char *> strMap = {
#define BYTECODE_NAME_MAP(name) {EcmaBytecode::name, #name },
    ECMA_BYTECODE_LIST(BYTECODE_NAME_MAP)
#undef BYTECODE_NAME_MAP
    };
    if (strMap.count(opcode) > 0) {
        return strMap.at(opcode);
    }
    return "bytecode-" + std::to_string(static_cast<uint16_t>(opcode));
}

}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_ECMA_OPCODE_H