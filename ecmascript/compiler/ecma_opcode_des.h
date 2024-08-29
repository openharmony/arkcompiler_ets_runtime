/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_ECMA_OPCODE_DES_H
#define ECMASCRIPT_COMPILER_ECMA_OPCODE_DES_H

#include <map>
#include <string>
#include <vector>

#include "ecmascript/compiler/bytecodes.h"

namespace panda::ecmascript::kungfu {

#define ECMA_OPCODE_LIST(V)                                                \
    V(LDNAN)  															   \
    V(LDINFINITY)                                                          \
    V(LDNEWTARGET)                                                         \
    V(LDUNDEFINED)                                                         \
    V(LDNULL)                                                              \
    V(LDSYMBOL)                                                            \
    V(LDGLOBAL)                                                            \
    V(LDTRUE)                                                              \
    V(LDFALSE)                                                             \
    V(LDHOLE)                                                              \
    V(LDTHIS)                                                              \
    V(POPLEXENV)                                                           \
    V(GETUNMAPPEDARGS)                                                     \
    V(ASYNCFUNCTIONENTER)                                                  \
    V(CREATEASYNCGENERATOROBJ_V8)                                          \
    V(LDFUNCTION)                                                          \
    V(DEBUGGER)                                                            \
    V(GETPROPITERATOR)                                                     \
    V(GETITERATOR_IMM8)                                                    \
    V(GETITERATOR_IMM16)                                                   \
    V(CLOSEITERATOR_IMM8_V8)                                               \
    V(CLOSEITERATOR_IMM16_V8)                                              \
    V(ASYNCGENERATORRESOLVE_V8_V8_V8)                                      \
    V(CREATEEMPTYOBJECT)                                                   \
    V(CREATEEMPTYARRAY_IMM8)                                               \
    V(CREATEEMPTYARRAY_IMM16)                                              \
    V(CREATEGENERATOROBJ_V8)                                               \
    V(CREATEITERRESULTOBJ_V8_V8)                                           \
    V(CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8)                             \
    V(CALLTHIS0_IMM8_V8)                                                   \
    V(CREATEARRAYWITHBUFFER_IMM8_ID16)                                     \
    V(CREATEARRAYWITHBUFFER_IMM16_ID16)                                    \
    V(CALLTHIS1_IMM8_V8_V8)                                                \
    V(CALLTHIS2_IMM8_V8_V8_V8)                                             \
    V(CREATEOBJECTWITHBUFFER_IMM8_ID16)                                    \
    V(CREATEOBJECTWITHBUFFER_IMM16_ID16)                                   \
    V(CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8)                              \
    V(CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8)                             \
    V(NEWOBJAPPLY_IMM8_V8)                                                 \
    V(NEWOBJAPPLY_IMM16_V8)                                                \
    V(NEWOBJRANGE_IMM8_IMM8_V8)                                            \
    V(NEWOBJRANGE_IMM16_IMM8_V8)                                           \
    V(WIDE_NEWOBJRANGE_PREF_IMM16_V8)                                      \
    V(NEWLEXENV_IMM8)                                                      \
    V(NEWLEXENVWITHNAME_IMM8_ID16)                                         \
    V(ADD2_IMM8_V8)                                                        \
    V(SUB2_IMM8_V8)                                                        \
    V(MUL2_IMM8_V8)                                                        \
    V(DIV2_IMM8_V8)                                                        \
    V(MOD2_IMM8_V8)                                                        \
    V(EQ_IMM8_V8)                                                          \
    V(NOTEQ_IMM8_V8)                                                       \
    V(LESS_IMM8_V8)                                                        \
    V(LESSEQ_IMM8_V8)                                                      \
    V(GREATER_IMM8_V8)                                                     \
    V(GREATEREQ_IMM8_V8)                                                   \
    V(SHL2_IMM8_V8)                                                        \
    V(SHR2_IMM8_V8)                                                        \
    V(ASHR2_IMM8_V8)                                                       \
    V(AND2_IMM8_V8)                                                        \
    V(OR2_IMM8_V8)                                                         \
    V(XOR2_IMM8_V8)                                                        \
    V(EXP_IMM8_V8)                                                         \
    V(TYPEOF_IMM8)                                                         \
    V(TYPEOF_IMM16)                                                        \
    V(TONUMBER_IMM8)                                                       \
    V(TONUMERIC_IMM8)                                                      \
    V(NEG_IMM8)                                                            \
    V(NOT_IMM8)                                                            \
    V(INC_IMM8)                                                            \
    V(DEC_IMM8)                                                            \
    V(ISIN_IMM8_V8)                                                        \
    V(INSTANCEOF_IMM8_V8)                                                  \
    V(STRICTNOTEQ_IMM8_V8)                                                 \
    V(STRICTEQ_IMM8_V8)                                                    \
    V(ISTRUE)                                                              \
    V(ISFALSE)                                                             \
    V(CALLTHIS3_IMM8_V8_V8_V8_V8)                                          \
    V(CALLTHISRANGE_IMM8_IMM8_V8)                                          \
    V(WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8)                               \
    V(SUPERCALLTHISRANGE_IMM8_IMM8_V8)                                     \
    V(SUPERCALLARROWRANGE_IMM8_IMM8_V8)                                    \
    V(CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8)                         \
    V(DEFINEFUNC_IMM8_ID16_IMM8)                                           \
    V(DEFINEFUNC_IMM16_ID16_IMM8)                                          \
    V(DEFINEMETHOD_IMM8_ID16_IMM8)                                         \
    V(DEFINEMETHOD_IMM16_ID16_IMM8)                                        \
    V(CALLARG0_IMM8)                                                       \
    V(SUPERCALLSPREAD_IMM8_V8)                                             \
    V(APPLY_IMM8_V8_V8)                                                    \
    V(CALLARGS2_IMM8_V8_V8)                                                \
    V(CALLARGS3_IMM8_V8_V8_V8)                                             \
    V(CALLRANGE_IMM8_IMM8_V8)                                              \
    V(LDEXTERNALMODULEVAR_IMM8)                                            \
    V(LDTHISBYNAME_IMM8_ID16)                                              \
    V(LDTHISBYNAME_IMM16_ID16)                                             \
    V(LDOBJBYNAME_IMM8_ID16)                                               \
    V(LDOBJBYNAME_IMM16_ID16)                                              \
    V(DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8)                               \
    V(STTHISBYNAME_IMM8_ID16)                                              \
    V(STTHISBYNAME_IMM16_ID16)                                             \
    V(STOBJBYNAME_IMM8_ID16_V8)                                            \
    V(STOBJBYNAME_IMM16_ID16_V8)                                           \
    V(DEFINEFIELDBYNAME_IMM8_ID16_V8)                                      \
    V(DEFINEPROPERTYBYNAME_IMM8_ID16_V8)                                   \
    V(LDTHISBYVALUE_IMM8)                                                  \
    V(LDTHISBYVALUE_IMM16)                                                 \
    V(LDOBJBYVALUE_IMM8_V8)                                                \
    V(LDOBJBYVALUE_IMM16_V8)                                               \
    V(STTHISBYVALUE_IMM8_V8)                                               \
    V(STTHISBYVALUE_IMM16_V8)                                              \
    V(DYNAMICIMPORT)                                                       \
    V(DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8)                       \
    V(DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8)                      \
    V(RESUMEGENERATOR)                                                     \
    V(GETRESUMEMODE)                                                       \
    V(GETTEMPLATEOBJECT_IMM8)                                              \
    V(GETTEMPLATEOBJECT_IMM16)                                             \
    V(GETNEXTPROPNAME_V8)                                                  \
    V(JEQZ_IMM8)                                                           \
    V(JEQZ_IMM16)                                                          \
    V(JEQZ_IMM32)                                                          \
    V(SETOBJECTWITHPROTO_IMM8_V8)                                          \
    V(DELOBJPROP_V8)                                                       \
    V(SUSPENDGENERATOR_V8)                                                 \
    V(ASYNCFUNCTIONAWAITUNCAUGHT_V8)                                       \
    V(COPYDATAPROPERTIES_V8)                                               \
    V(STARRAYSPREAD_V8_V8)                                                 \
    V(SETOBJECTWITHPROTO_IMM16_V8)                                         \
    V(STOBJBYVALUE_IMM8_V8_V8)                                             \
    V(STOBJBYVALUE_IMM16_V8_V8)                                            \
    V(STOWNBYVALUE_IMM8_V8_V8)                                             \
    V(STOWNBYVALUE_IMM16_V8_V8)                                            \
    V(LDSUPERBYVALUE_IMM8_V8)                                              \
    V(LDSUPERBYVALUE_IMM16_V8)                                             \
    V(STSUPERBYVALUE_IMM8_V8_V8)                                           \
    V(STSUPERBYVALUE_IMM16_V8_V8)                                          \
    V(LDOBJBYINDEX_IMM8_IMM16)                                             \
    V(LDOBJBYINDEX_IMM16_IMM16)                                            \
    V(WIDE_LDOBJBYINDEX_PREF_IMM32)                                        \
    V(STOBJBYINDEX_IMM8_V8_IMM16)                                          \
    V(STOBJBYINDEX_IMM16_V8_IMM16)                                         \
    V(WIDE_STOBJBYINDEX_PREF_V8_IMM32)                                     \
    V(STOWNBYINDEX_IMM8_V8_IMM16)                                          \
    V(STOWNBYINDEX_IMM16_V8_IMM16)                                         \
    V(ASYNCFUNCTIONRESOLVE_V8)                                             \
    V(ASYNCFUNCTIONREJECT_V8)                                              \
    V(COPYRESTARGS_IMM8)                                                   \
    V(LDLEXVAR_IMM4_IMM4)                                                  \
    V(STLEXVAR_IMM4_IMM4)                                                  \
    V(GETMODULENAMESPACE_IMM8)                                             \
    V(STMODULEVAR_IMM8)                                                    \
    V(TRYLDGLOBALBYNAME_IMM8_ID16)                                         \
    V(TRYLDGLOBALBYNAME_IMM16_ID16)                                        \
    V(TRYSTGLOBALBYNAME_IMM8_ID16)                                         \
    V(TRYSTGLOBALBYNAME_IMM16_ID16)                                        \
    V(LDGLOBALVAR_IMM16_ID16)                                              \
    V(STGLOBALVAR_IMM16_ID16)                                              \
    V(STOWNBYNAME_IMM8_ID16_V8)                                            \
    V(STOWNBYNAME_IMM16_ID16_V8)                                           \
    V(LDSUPERBYNAME_IMM8_ID16)                                             \
    V(LDSUPERBYNAME_IMM16_ID16)                                            \
    V(STSUPERBYNAME_IMM8_ID16_V8)                                          \
    V(STSUPERBYNAME_IMM16_ID16_V8)                                         \
    V(LDLOCALMODULEVAR_IMM8)                                               \
    V(STCONSTTOGLOBALRECORD_IMM16_ID16)                                    \
    V(STTOGLOBALRECORD_IMM16_ID16)                                         \
    V(JNEZ_IMM8)                                                           \
    V(JNEZ_IMM16)                                                          \
    V(JNEZ_IMM32)                                                          \
    V(STOWNBYVALUEWITHNAMESET_IMM8_V8_V8)                                  \
    V(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8)                                 \
    V(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8)                                 \
    V(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8)                                \
    V(LDBIGINT_ID16)                                                       \
    V(LDA_STR_ID16)                                                        \
    V(JMP_IMM8)                                                            \
    V(JMP_IMM16)                                                           \
    V(JMP_IMM32)                                                           \
    V(JSTRICTEQZ_IMM8)                                                     \
    V(JSTRICTEQZ_IMM16)                                                    \
    V(JNSTRICTEQZ_IMM8)                                                    \
    V(JNSTRICTEQZ_IMM16)                                                   \
    V(JEQNULL_IMM8)                                                        \
    V(JEQNULL_IMM16)                                                       \
    V(LDA_V8)                                                              \
    V(STA_V8)                                                              \
    V(LDAI_IMM32)                                                          \
    V(FLDAI_IMM64)                                                         \
    V(RETURN)                                                              \
    V(RETURNUNDEFINED)                                                     \
    V(LDLEXVAR_IMM8_IMM8)                                                  \
    V(JNENULL_IMM8)                                                        \
    V(STLEXVAR_IMM8_IMM8)                                                  \
    V(JNENULL_IMM16)                                                       \
    V(CALLARG1_IMM8_V8)                                                    \
    V(JSTRICTEQNULL_IMM8)                                                  \
    V(JSTRICTEQNULL_IMM16)                                                 \
    V(JNSTRICTEQNULL_IMM8)                                                 \
    V(JNSTRICTEQNULL_IMM16)                                                \
    V(JEQUNDEFINED_IMM8)                                                   \
    V(JEQUNDEFINED_IMM16)                                                  \
    V(JNEUNDEFINED_IMM8)                                                   \
    V(JNEUNDEFINED_IMM16)                                                  \
    V(JSTRICTEQUNDEFINED_IMM8)                                             \
    V(JSTRICTEQUNDEFINED_IMM16)                                            \
    V(JNSTRICTEQUNDEFINED_IMM8)                                            \
    V(JNSTRICTEQUNDEFINED_IMM16)                                           \
    V(JEQ_V8_IMM8)                                                         \
    V(JEQ_V8_IMM16)                                                        \
    V(JNE_V8_IMM8)                                                         \
    V(JNE_V8_IMM16)                                                        \
    V(JSTRICTEQ_V8_IMM8)                                                   \
    V(JSTRICTEQ_V8_IMM16)                                                  \
    V(JNSTRICTEQ_V8_IMM8)                                                  \
    V(JNSTRICTEQ_V8_IMM16)                                                 \
    V(MOV_V4_V4)                                                           \
    V(MOV_V8_V8)                                                           \
    V(MOV_V16_V16)                                                         \
    V(ASYNCGENERATORREJECT_V8)                                             \
    V(NOP)                                                                 \
    V(SETGENERATORSTATE_IMM8)                                              \
    V(GETASYNCITERATOR_IMM8)                                               \
    V(LDPRIVATEPROPERTY_IMM8_IMM16_IMM16)                                  \
    V(STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8)                               \
    V(TESTIN_IMM8_IMM16_IMM16)                                             \
    V(DEPRECATED_LDLEXENV_PREF_NONE)                                       \
    V(WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8)                  \
    V(THROW_PREF_NONE)                                                     \
    V(DEPRECATED_POPLEXENV_PREF_NONE)                                      \
    V(THROW_NOTEXISTS_PREF_NONE)                                           \
    V(DEPRECATED_GETITERATORNEXT_PREF_V8_V8)                               \
    V(WIDE_NEWLEXENV_PREF_IMM16)                                           \
    V(THROW_PATTERNNONCOERCIBLE_PREF_NONE)                                 \
    V(DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16)                         \
    V(WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16)                              \
    V(THROW_DELETESUPERPROPERTY_PREF_NONE)                                 \
    V(DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16)                        \
    V(WIDE_CALLRANGE_PREF_IMM16_V8)                                        \
    V(THROW_CONSTASSIGNMENT_PREF_V8)                                       \
    V(DEPRECATED_TONUMBER_PREF_V8)                                         \
    V(WIDE_CALLTHISRANGE_PREF_IMM16_V8)                                    \
    V(THROW_IFNOTOBJECT_PREF_V8)                                           \
    V(DEPRECATED_TONUMERIC_PREF_V8)                                        \
    V(THROW_UNDEFINEDIFHOLE_PREF_V8_V8)                                    \
    V(THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16)                             \
    V(DEPRECATED_NEG_PREF_V8)                                              \
    V(WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8)                              \
    V(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8)                               \
    V(DEPRECATED_NOT_PREF_V8)                                              \
    V(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16)                              \
    V(DEPRECATED_INC_PREF_V8)                                              \
    V(DEPRECATED_DEC_PREF_V8)                                              \
    V(WIDE_STOWNBYINDEX_PREF_V8_IMM32)                                     \
    V(DEPRECATED_CALLARG0_PREF_V8)                                         \
    V(WIDE_COPYRESTARGS_PREF_IMM16)                                        \
    V(DEPRECATED_CALLARG1_PREF_V8_V8)                                      \
    V(WIDE_LDLEXVAR_PREF_IMM16_IMM16)                                      \
    V(DEPRECATED_CALLARGS2_PREF_V8_V8_V8)                                  \
    V(WIDE_STLEXVAR_PREF_IMM16_IMM16)                                      \
    V(DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8)                               \
    V(WIDE_GETMODULENAMESPACE_PREF_IMM16)                                  \
    V(DEPRECATED_CALLRANGE_PREF_IMM16_V8)                                  \
    V(WIDE_STMODULEVAR_PREF_IMM16)                                         \
    V(DEPRECATED_CALLSPREAD_PREF_V8_V8_V8)                                 \
    V(WIDE_LDLOCALMODULEVAR_PREF_IMM16)                                    \
    V(DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8)                              \
    V(WIDE_LDEXTERNALMODULEVAR_PREF_IMM16)                                 \
    V(DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8)        \
    V(WIDE_LDPATCHVAR_PREF_IMM16)                                          \
    V(DEPRECATED_RESUMEGENERATOR_PREF_V8)                                  \
    V(WIDE_STPATCHVAR_PREF_IMM16)                                          \
    V(DEPRECATED_GETRESUMEMODE_PREF_V8)                                    \
    V(DEPRECATED_GETTEMPLATEOBJECT_PREF_V8)                                \
    V(DEPRECATED_DELOBJPROP_PREF_V8_V8)                                    \
    V(DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8)                              \
    V(DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8)                    \
    V(DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8)                            \
    V(DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8)                            \
    V(DEPRECATED_LDOBJBYVALUE_PREF_V8_V8)                                  \
    V(DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8)                                \
    V(DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32)                               \
    V(DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8)                       \
    V(DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8)                        \
    V(DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8)                               \
    V(DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8)                               \
    V(DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8)                             \
    V(DEPRECATED_GETMODULENAMESPACE_PREF_ID32)                             \
    V(DEPRECATED_STMODULEVAR_PREF_ID32)                                    \
    V(DEPRECATED_LDOBJBYNAME_PREF_ID32_V8)                                 \
    V(DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8)                               \
    V(DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8)                               \
    V(DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32)                          \
    V(DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32)                            \
    V(DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32)                          \
    V(DEPRECATED_LDHOMEOBJECT_PREF_NONE)                                   \
    V(DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16)                      \
    V(DEPRECATED_DYNAMICIMPORT_PREF_V8)                                    \
    V(CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE)                        \
    V(CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8)                      \
    V(CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8)                   \
    V(CALLRUNTIME_TOPROPERTYKEY_PREF_NONE)                                 \
    V(CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16)                   \
    V(CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8)          \
    V(CALLRUNTIME_CALLINIT_PREF_IMM8_V8)                                   \
    V(CALLRUNTIME_DEFINESENDABLECLASS_PREF_IMM16_ID16_ID16_IMM16_V8)       \
    V(CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16)                              \
    V(CALLRUNTIME_LDSENDABLEEXTERNALMODULEVAR_PREF_IMM8)                   \
    V(CALLRUNTIME_WIDELDSENDABLEEXTERNALMODULEVAR_PREF_IMM16)              \
    V(CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8)                                \
    V(CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16)                           \
    V(CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4)                            \
    V(CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8)                            \
    V(CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16)                      \
    V(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4)                            \
    V(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8)                            \
    V(CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16)                      \
    V(CALLRUNTIME_ISTRUE_PREF_IMM8)                                        \
    V(CALLRUNTIME_ISFALSE_PREF_IMM8)                                       \
    V(CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8)                               \
    V(CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16)                          \
    V(CALLRUNTIME_LDLAZYSENDABLEMODULEVAR_PREF_IMM8)                       \
    V(CALLRUNTIME_WIDELDLAZYSENDABLEMODULEVAR_PREF_IMM16)                  \

inline std::string GetEcmaOpcodeStr(EcmaOpcode opcode)
{
    const std::map<EcmaOpcode, const char *> strMap = {
#define BYTECODE_NAME_MAP(name) { EcmaOpcode::name, #name },
        ECMA_OPCODE_LIST(BYTECODE_NAME_MAP)
#undef BYTECODE_NAME_MAP
    };
    if (strMap.count(opcode) > 0) {
        return strMap.at(opcode);
    }
    return "bytecode-" + std::to_string(static_cast<uint16_t>(opcode));
}

inline std::vector<EcmaOpcode> GetEcmaCodeListForRange()
{
    std::vector<EcmaOpcode> lorwingOpCodeList = {
#define BYTECODE_NAME_VECTOR(name) EcmaOpcode::name,
        ECMA_OPCODE_LIST(BYTECODE_NAME_VECTOR)
#undef BYTECODE_NAME_VECTOR
    };
    return lorwingOpCodeList;
}

inline std::string GetHelpForEcmaCodeListForRange()
{
    int32_t i = 0;
    std::vector<EcmaOpcode> lorwingOpCodeList = GetEcmaCodeListForRange();
    std::string optBCRangeStr;
    optBCRangeStr.append(" bytecode for list: \n");
    for (auto ecmaOpCode : lorwingOpCodeList) {
        optBCRangeStr.append(" index:" + std::to_string(i) + "     ecmaOpCode:" + GetEcmaOpcodeStr(ecmaOpCode) + "\n");
        i++;
    }
    return optBCRangeStr;
}
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_ECMA_OPCODE_DES_H_
