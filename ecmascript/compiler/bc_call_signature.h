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

#ifndef ECMASCRIPT_COMPILER_BC_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_BC_CALL_SIGNATURE_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/call_signature.h"

namespace panda::ecmascript::kungfu {
#define IGNORE_BC_STUB(...)
#define ASM_UNUSED_BC_STUB_LIST(T)                      \
    T(HandleOverflowDD)                                 \
    T(HandleOverflowDE)                                 \
    T(HandleOverflowDF)                                 \
    T(HandleOverflowE0)                                 \
    T(HandleOverflowE1)                                 \
    T(HandleOverflowE2)                                 \
    T(HandleOverflowE3)                                 \
    T(HandleOverflowE4)                                 \
    T(HandleOverflowE5)                                 \
    T(HandleOverflowE6)                                 \
    T(HandleOverflowE7)                                 \
    T(HandleOverflowE8)                                 \
    T(HandleOverflowE9)                                 \
    T(HandleOverflowEA)                                 \
    T(HandleOverflowEB)                                 \
    T(HandleOverflowEC)                                 \
    T(HandleOverflowED)                                 \
    T(HandleOverflowEE)                                 \
    T(HandleOverflowEF)                                 \
    T(HandleOverflowF0)                                 \
    T(HandleOverflowF1)                                 \
    T(HandleOverflowF2)                                 \
    T(HandleOverflowF3)                                 \
    T(HandleOverflowF4)                                 \
    T(HandleOverflowF5)                                 \
    T(HandleOverflowF6)                                 \
    T(HandleOverflowF7)                                 \
    T(HandleOverflowF8)                                 \
    T(HandleOverflowF9)                                 \
    T(HandleOverflowFA)                                 \


// V: Not Enabled, T: Enabled, D: Always Disable SingleStepDebugging
#define ASM_INTERPRETER_BC_STUB_LIST(V, T, D)                             \
    T(HandleLdundefined)                                                  \
    T(HandleLdnull)                                                       \
    T(HandleLdtrue)                                                       \
    T(HandleLdfalse)                                                      \
    T(HandleCreateemptyobject)                                            \
    T(HandleCreateemptyarrayImm8)                                         \
    T(HandleCreatearraywithbufferImm8Id16)                                \
    T(HandleCreateobjectwithbufferImm8Id16)                               \
    T(HandleNewobjrangeImm8Imm8V8)                                        \
    T(HandleNewlexenvImm8)                                                \
    T(HandleAdd2Imm8V8)                                                   \
    T(HandleSub2Imm8V8)                                                   \
    T(HandleMul2Imm8V8)                                                   \
    T(HandleDiv2Imm8V8)                                                   \
    T(HandleMod2Imm8V8)                                                   \
    T(HandleEqImm8V8)                                                     \
    T(HandleNoteqImm8V8)                                                  \
    T(HandleLessImm8V8)                                                   \
    T(HandleLesseqImm8V8)                                                 \
    T(HandleGreaterImm8V8)                                                \
    T(HandleGreatereqImm8V8)                                              \
    T(HandleShl2Imm8V8)                                                   \
    T(HandleShr2Imm8V8)                                                   \
    T(HandleAshr2Imm8V8)                                                  \
    T(HandleAnd2Imm8V8)                                                   \
    T(HandleOr2Imm8V8)                                                    \
    T(HandleXor2Imm8V8)                                                   \
    T(HandleExpImm8V8)                                                    \
    T(HandleTypeofImm8)                                                   \
    T(HandleTonumberImm8)                                                 \
    T(HandleTonumericImm8)                                                \
    T(HandleNegImm8)                                                      \
    T(HandleNotImm8)                                                      \
    T(HandleIncImm8)                                                      \
    T(HandleDecImm8)                                                      \
    T(HandleIstrue)                                                       \
    T(HandleIsfalse)                                                      \
    T(HandleIsinImm8V8)                                                   \
    T(HandleInstanceofImm8V8)                                             \
    T(HandleStrictnoteqImm8V8)                                            \
    T(HandleStricteqImm8V8)                                               \
    D(HandleCallarg0Imm8)                                                 \
    D(HandleCallarg1Imm8V8)                                               \
    D(HandleCallargs2Imm8V8V8)                                            \
    D(HandleCallargs3Imm8V8V8V8)                                          \
    D(HandleCallthis0Imm8V8)                                              \
    D(HandleCallthis1Imm8V8V8)                                            \
    D(HandleCallthis2Imm8V8V8V8)                                          \
    D(HandleCallthis3Imm8V8V8V8V8)                                        \
    D(HandleCallthisrangeImm8Imm8V8)                                      \
    T(HandleSupercallthisrangeImm8Imm8V8)                                 \
    T(HandleDefinefuncImm8Id16Imm8)                                       \
    T(HandleDefinemethodImm8Id16Imm8)                                     \
    T(HandleDefineclasswithbufferImm8Id16Id16Imm16V8)                     \
    T(HandleGetnextpropnameV8)                                            \
    T(HandleLdobjbyvalueImm8V8)                                           \
    T(HandleStobjbyvalueImm8V8V8)                                         \
    T(HandleLdsuperbyvalueImm8V8)                                         \
    T(HandleLdobjbyindexImm8Imm16)                                        \
    T(HandleStobjbyindexImm8V8Imm16)                                      \
    T(HandleLdlexvarImm4Imm4)                                             \
    T(HandleStlexvarImm4Imm4)                                             \
    T(HandleLdaStrId16)                                                   \
    T(HandleTryldglobalbynameImm8Id16)                                    \
    T(HandleTrystglobalbynameImm8Id16)                                    \
    T(HandleLdglobalvarImm16Id16)                                         \
    T(HandleLdobjbynameImm8Id16)                                          \
    T(HandleStobjbynameImm8Id16V8)                                        \
    T(HandleMovV4V4)                                                      \
    T(HandleMovV8V8)                                                      \
    T(HandleLdsuperbynameImm8Id16)                                        \
    T(HandleStconsttoglobalrecordImm16Id16)                               \
    T(HandleSttoglobalrecordImm16Id16)                                    \
    T(HandleLdthisbynameImm8Id16)                                         \
    T(HandleStthisbynameImm8Id16)                                         \
    T(HandleLdthisbyvalueImm8)                                            \
    T(HandleStthisbyvalueImm8V8)                                          \
    T(HandleJmpImm8)                                                      \
    T(HandleJmpImm16)                                                     \
    T(HandleJeqzImm8)                                                     \
    T(HandleJeqzImm16)                                                    \
    T(HandleJnezImm8)                                                     \
    T(HandleJstricteqzImm8)                                               \
    T(HandleJnstricteqzImm8)                                              \
    T(HandleJeqnullImm8)                                                  \
    T(HandleJnenullImm8)                                                  \
    T(HandleJstricteqnullImm8)                                            \
    T(HandleJnstricteqnullImm8)                                           \
    T(HandleJequndefinedImm8)                                             \
    T(HandleJneundefinedImm8)                                             \
    T(HandleJstrictequndefinedImm8)                                       \
    T(HandleJnstrictequndefinedImm8)                                      \
    T(HandleJeqV8Imm8)                                                    \
    T(HandleJneV8Imm8)                                                    \
    T(HandleJstricteqV8Imm8)                                              \
    T(HandleJnstricteqV8Imm8)                                             \
    T(HandleLdaV8)                                                        \
    T(HandleStaV8)                                                        \
    T(HandleLdaiImm32)                                                    \
    T(HandleFldaiImm64)                                                   \
    T(HandleReturn)                                                       \
    T(HandleReturnundefined)                                              \
    T(HandleGetpropiterator)                                              \
    T(HandleGetiteratorImm8)                                              \
    T(HandleCloseiteratorImm8V8)                                          \
    T(HandlePoplexenv)                                                    \
    T(HandleLdnan)                                                        \
    T(HandleLdinfinity)                                                   \
    T(HandleGetunmappedargs)                                              \
    T(HandleLdglobal)                                                     \
    T(HandleLdnewtarget)                                                  \
    T(HandleLdthis)                                                       \
    T(HandleLdhole)                                                       \
    T(HandleCreateregexpwithliteralImm8Id16Imm8)                          \
    T(HandleCreateregexpwithliteralImm16Id16Imm8)                         \
    D(HandleCallrangeImm8Imm8V8)                                          \
    T(HandleDefinefuncImm16Id16Imm8)                                      \
    T(HandleDefineclasswithbufferImm16Id16Id16Imm16V8)                    \
    T(HandleGettemplateobjectImm8)                                        \
    T(HandleSetobjectwithprotoImm8V8)                                     \
    T(HandleStownbyvalueImm8V8V8)                                         \
    T(HandleStownbyindexImm8V8Imm16)                                      \
    T(HandleStownbynameImm8Id16V8)                                        \
    T(HandleGetmodulenamespaceImm8)                                       \
    T(HandleStmodulevarImm8)                                              \
    T(HandleLdlocalmodulevarImm8)                                         \
    T(HandleLdexternalmodulevarImm8)                                      \
    T(HandleStglobalvarImm16Id16)                                         \
    T(HandleCreateemptyarrayImm16)                                        \
    T(HandleCreatearraywithbufferImm16Id16)                               \
    T(HandleCreateobjectwithbufferImm16Id16)                              \
    T(HandleNewobjrangeImm16Imm8V8)                                       \
    T(HandleTypeofImm16)                                                  \
    T(HandleLdobjbyvalueImm16V8)                                          \
    T(HandleStobjbyvalueImm16V8V8)                                        \
    T(HandleLdsuperbyvalueImm16V8)                                        \
    T(HandleLdobjbyindexImm16Imm16)                                       \
    T(HandleStobjbyindexImm16V8Imm16)                                     \
    T(HandleLdlexvarImm8Imm8)                                             \
    T(HandleStlexvarImm8Imm8)                                             \
    T(HandleTryldglobalbynameImm16Id16)                                   \
    T(HandleTrystglobalbynameImm16Id16)                                   \
    T(HandleStownbynamewithnamesetImm8Id16V8)                             \
    T(HandleMovV16V16)                                                    \
    T(HandleLdobjbynameImm16Id16)                                         \
    T(HandleStobjbynameImm16Id16V8)                                       \
    T(HandleLdsuperbynameImm16Id16)                                       \
    T(HandleLdthisbynameImm16Id16)                                        \
    T(HandleStthisbynameImm16Id16)                                        \
    T(HandleLdthisbyvalueImm16)                                           \
    T(HandleStthisbyvalueImm16V8)                                         \
    T(HandleAsyncgeneratorrejectV8)                                       \
    T(HandleJmpImm32)                                                     \
    T(HandleStownbyvaluewithnamesetImm8V8V8)                              \
    T(HandleJeqzImm32)                                                    \
    T(HandleJnezImm16)                                                    \
    T(HandleJnezImm32)                                                    \
    T(HandleJstricteqzImm16)                                              \
    T(HandleJnstricteqzImm16)                                             \
    T(HandleJeqnullImm16)                                                 \
    T(HandleJnenullImm16)                                                 \
    T(HandleJstricteqnullImm16)                                           \
    T(HandleJnstricteqnullImm16)                                          \
    T(HandleJequndefinedImm16)                                            \
    T(HandleJneundefinedImm16)                                            \
    T(HandleJstrictequndefinedImm16)                                      \
    T(HandleJnstrictequndefinedImm16)                                     \
    T(HandleJeqV8Imm16)                                                   \
    T(HandleJneV8Imm16)                                                   \
    T(HandleJstricteqV8Imm16)                                             \
    T(HandleJnstricteqV8Imm16)                                            \
    T(HandleGetiteratorImm16)                                             \
    T(HandleCloseiteratorImm16V8)                                         \
    T(HandleLdsymbol)                                                     \
    T(HandleAsyncfunctionenter)                                           \
    T(HandleLdfunction)                                                   \
    T(HandleDebugger)                                                     \
    T(HandleCreategeneratorobjV8)                                         \
    T(HandleCreateiterresultobjV8V8)                                      \
    T(HandleCreateobjectwithexcludedkeysImm8V8V8)                         \
    T(HandleNewobjapplyImm8V8)                                            \
    T(HandleNewobjapplyImm16V8)                                           \
    T(HandleNewlexenvwithnameImm8Id16)                                    \
    T(HandleCreateasyncgeneratorobjV8)                                    \
    T(HandleAsyncgeneratorresolveV8V8V8)                                  \
    T(HandleSupercallspreadImm8V8)                                        \
    T(HandleApplyImm8V8V8)                                                \
    T(HandleSupercallarrowrangeImm8Imm8V8)                                \
    T(HandleDefinegettersetterbyvalueV8V8V8V8)                            \
    T(HandleDynamicimport)                                                \
    T(HandleDefinemethodImm16Id16Imm8)                                    \
    T(HandleResumegenerator)                                              \
    T(HandleGetresumemode)                                                \
    T(HandleGettemplateobjectImm16)                                       \
    T(HandleDelobjpropV8)                                                 \
    T(HandleSuspendgeneratorV8)                                           \
    T(HandleAsyncfunctionawaituncaughtV8)                                 \
    T(HandleCopydatapropertiesV8)                                         \
    T(HandleStarrayspreadV8V8)                                            \
    T(HandleSetobjectwithprotoImm16V8)                                    \
    T(HandleStownbyvalueImm16V8V8)                                        \
    T(HandleStsuperbyvalueImm8V8V8)                                       \
    T(HandleStsuperbyvalueImm16V8V8)                                      \
    T(HandleStownbyindexImm16V8Imm16)                                     \
    T(HandleStownbynameImm16Id16V8)                                       \
    T(HandleAsyncfunctionresolveV8)                                       \
    T(HandleAsyncfunctionrejectV8)                                        \
    T(HandleCopyrestargsImm8)                                             \
    T(HandleStsuperbynameImm8Id16V8)                                      \
    T(HandleStsuperbynameImm16Id16V8)                                     \
    T(HandleStownbyvaluewithnamesetImm16V8V8)                             \
    T(HandleLdbigintId16)                                                 \
    T(HandleStownbynamewithnamesetImm16Id16V8)                            \
    T(HandleNop)                                                          \
    T(HandleSetgeneratorstateImm8)                                        \
    T(HandleGetasynciteratorImm8)                                         \
    T(HandleLdPrivatePropertyImm8Imm16Imm16)                              \
    T(HandleStPrivatePropertyImm8Imm16Imm16V8)                            \
    T(HandleTestInImm8Imm16Imm16)                                         \
    T(HandleDefineFieldByNameImm8Id16V8)                                  \
    T(HandleDefinePropertyByNameImm8Id16V8)                               \
    ASM_UNUSED_BC_STUB_LIST(T)                                            \
    T(HandleCallRuntime)                                                  \
    T(HandleDeprecated)                                                   \
    T(HandleWide)                                                         \
    T(HandleThrow)                                                        \
    D(ExceptionHandler)

// V: Not Enabled, T: Enabled, D: Always Disable SingleStepDebugging
#define ASM_INTERPRETER_DEPRECATED_STUB_LIST(V, T, D)                     \
    T(HandleDeprecatedLdlexenvPrefNone)                                   \
    T(HandleDeprecatedPoplexenvPrefNone)                                  \
    T(HandleDeprecatedGetiteratornextPrefV8V8)                            \
    T(HandleDeprecatedCreatearraywithbufferPrefImm16)                     \
    T(HandleDeprecatedCreateobjectwithbufferPrefImm16)                    \
    T(HandleDeprecatedTonumberPrefV8)                                     \
    T(HandleDeprecatedTonumericPrefV8)                                    \
    T(HandleDeprecatedNegPrefV8)                                          \
    T(HandleDeprecatedNotPrefV8)                                          \
    T(HandleDeprecatedIncPrefV8)                                          \
    T(HandleDeprecatedDecPrefV8)                                          \
    D(HandleDeprecatedCallarg0PrefV8)                                     \
    D(HandleDeprecatedCallarg1PrefV8V8)                                   \
    D(HandleDeprecatedCallargs2PrefV8V8V8)                                \
    D(HandleDeprecatedCallargs3PrefV8V8V8V8)                              \
    D(HandleDeprecatedCallrangePrefImm16V8)                               \
    D(HandleDeprecatedCallspreadPrefV8V8V8)                               \
    D(HandleDeprecatedCallthisrangePrefImm16V8)                           \
    T(HandleDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8)        \
    T(HandleDeprecatedResumegeneratorPrefV8)                              \
    T(HandleDeprecatedGetresumemodePrefV8)                                \
    T(HandleDeprecatedGettemplateobjectPrefV8)                            \
    T(HandleDeprecatedDelobjpropPrefV8V8)                                 \
    T(HandleDeprecatedSuspendgeneratorPrefV8V8)                           \
    T(HandleDeprecatedAsyncfunctionawaituncaughtPrefV8V8)                 \
    T(HandleDeprecatedCopydatapropertiesPrefV8V8)                         \
    T(HandleDeprecatedSetobjectwithprotoPrefV8V8)                         \
    T(HandleDeprecatedLdobjbyvaluePrefV8V8)                               \
    T(HandleDeprecatedLdsuperbyvaluePrefV8V8)                             \
    T(HandleDeprecatedLdobjbyindexPrefV8Imm32)                            \
    T(HandleDeprecatedAsyncfunctionresolvePrefV8V8V8)                     \
    T(HandleDeprecatedAsyncfunctionrejectPrefV8V8V8)                      \
    T(HandleDeprecatedStlexvarPrefImm4Imm4V8)                             \
    T(HandleDeprecatedStlexvarPrefImm8Imm8V8)                             \
    T(HandleDeprecatedStlexvarPrefImm16Imm16V8)                           \
    T(HandleDeprecatedGetmodulenamespacePrefId32)                         \
    T(HandleDeprecatedStmodulevarPrefId32)                                \
    T(HandleDeprecatedLdobjbynamePrefId32V8)                              \
    T(HandleDeprecatedLdsuperbynamePrefId32V8)                            \
    T(HandleDeprecatedLdmodulevarPrefId32Imm8)                            \
    T(HandleDeprecatedStconsttoglobalrecordPrefId32)                      \
    T(HandleDeprecatedStlettoglobalrecordPrefId32)                        \
    T(HandleDeprecatedStclasstoglobalrecordPrefId32)                      \
    T(HandleDeprecatedLdhomeobjectPrefNone)                               \
    T(HandleDeprecatedCreateobjecthavingmethodPrefImm16)                  \
    T(HandleDeprecatedDynamicimportPrefV8)                                \
    T(HandleDeprecatedAsyncgeneratorrejectPrefV8V8)

// V: Not Enabled, T: Enabled, D: Always Disable SingleStepDebugging
#define ASM_INTERPRETER_WIDE_STUB_LIST(V, T, D)                           \
    T(HandleWideCreateobjectwithexcludedkeysPrefImm16V8V8)                \
    T(HandleWideNewobjrangePrefImm16V8)                                   \
    T(HandleWideNewlexenvPrefImm16)                                       \
    T(HandleWideNewlexenvwithnamePrefImm16Id16)                           \
    D(HandleWideCallrangePrefImm16V8)                                     \
    D(HandleWideCallthisrangePrefImm16V8)                                 \
    T(HandleWideSupercallthisrangePrefImm16V8)                            \
    T(HandleWideSupercallarrowrangePrefImm16V8)                           \
    T(HandleWideLdobjbyindexPrefImm32)                                    \
    T(HandleWideStobjbyindexPrefV8Imm32)                                  \
    T(HandleWideStownbyindexPrefV8Imm32)                                  \
    T(HandleWideCopyrestargsPrefImm16)                                    \
    T(HandleWideLdlexvarPrefImm16Imm16)                                   \
    T(HandleWideStlexvarPrefImm16Imm16)                                   \
    T(HandleWideGetmodulenamespacePrefImm16)                              \
    T(HandleWideStmodulevarPrefImm16)                                     \
    T(HandleWideLdlocalmodulevarPrefImm16)                                \
    T(HandleWideLdexternalmodulevarPrefImm16)                             \
    T(HandleWideLdpatchvarPrefImm16)                                      \
    T(HandleWideStpatchvarPrefImm16)

// V: Not Enabled, T: Enabled, D: Always Disable SingleStepDebugging
#define ASM_INTERPRETER_THROW_STUB_LIST(V, T, D)                          \
    T(HandleThrowPrefNone)                                                \
    T(HandleThrowNotexistsPrefNone)                                       \
    T(HandleThrowPatternnoncoerciblePrefNone)                             \
    T(HandleThrowDeletesuperpropertyPrefNone)                             \
    T(HandleThrowConstassignmentPrefV8)                                   \
    T(HandleThrowIfnotobjectPrefV8)                                       \
    T(HandleThrowUndefinedifholePrefV8V8)                                 \
    T(HandleThrowIfsupernotcorrectcallPrefImm8)                           \
    T(HandleThrowIfsupernotcorrectcallPrefImm16)                          \
    T(HandleThrowUndefinedifholewithnamePrefId16)

// V: Not Enabled, T: Enabled, D: Always Disable SingleStepDebugging
#define ASM_INTERPRETER_CALLRUNTIME_STUB_LIST(V, T, D)                    \
    T(HandleCallRuntimeNotifyConcurrentResultPrefNone)                    \
    T(HandleCallRuntimeDefineFieldByValuePrefImm8V8V8)                    \
    T(HandleCallRuntimeDefineFieldByIndexPrefImm8Imm32V8)                 \
    T(HandleCallRuntimeToPropertyKeyPrefNone)                             \
    T(HandleCallRuntimeCreatePrivatePropertyPrefImm16Id16)                \
    T(HandleCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8)         \
    T(HandleCallRuntimeCallInitPrefImm8V8)                                \
    T(HandleCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8)       \
    T(HandleCallRuntimeLdSendableClassPrefImm16)                          \
    T(HandleCallRuntimeLdsendableexternalmodulevarImm8)                   \
    T(HandleCallRuntimeWideLdsendableexternalmodulevarPrefImm16)          \
    T(HandleCallRuntimeNewSendableEnvImm8)                                \
    T(HandleCallRuntimeNewSendableEnvImm16)                               \
    T(HandleCallRuntimeStSendableVarImm4Imm4)                             \
    T(HandleCallRuntimeStSendableVarImm8Imm8)                             \
    T(HandleCallRuntimeStSendableVarImm16Imm16)                           \
    T(HandleCallRuntimeLdSendableVarImm4Imm4)                             \
    T(HandleCallRuntimeLdSendableVarImm8Imm8)                             \
    T(HandleCallRuntimeLdSendableVarImm16Imm16)                           \
    T(HandleCallRuntimeIstruePrefImm8)                                    \
    T(HandleCallRuntimeIsfalsePrefImm8)                                   \
    T(HandleCallRuntimeLdLazyModuleVarPrefImm8)                           \
    T(HandleCallRuntimeWideLdLazyModuleVarPrefImm16)                      \
    T(HandleCallRuntimeLdLazySendableModuleVarPrefImm8)                   \
    T(HandleCallRuntimeWideLdLazySendableModuleVarPrefImm16)              \
    T(HandleCallRuntimeSuperCallForwardAllArgsV8)                         \
    T(HandleCallRuntimeLdsendablelocalmodulevarImm8)                      \
    T(HandleCallRuntimeWideLdsendablelocalmodulevarPrefImm16)

#define ASM_INTERPRETER_BC_HELPER_STUB_LIST(V)          \
    V(SingleStepDebugging)                              \
    V(BCDebuggerEntry)                                  \
    V(BCDebuggerExceptionEntry)                         \
    V(NewObjectRangeThrowException)                     \
    V(ThrowStackOverflowException)                      \
    V(HandleDefinefuncImm8Id16Imm8ColdReload)           \
    V(HandleDefinefuncImm16Id16Imm8ColdReload)

#define APPEND_JIT_SUFFIX(name, V) \
    V(name##WithJitProf, name, SlotIDFormat::IMM8)

#define APPEND_JIT_SUFFIX_IMM16(name, V) \
    V(name##WithJitProf, name, SlotIDFormat::IMM16)

#define ASM_INTERPRETER_BC_JIT_PROFILER_STUB_LIST(V)          \
    ASM_INTERPRETER_BC_FUNC_HOT_JIT_PROFILER_STUB_LIST(V)     \

#define ASM_INTERPRETER_BC_FUNC_HOT_JIT_PROFILER_STUB_LIST(V) \
    APPEND_JIT_SUFFIX(HandleJmpImm8, V)                       \
    APPEND_JIT_SUFFIX_IMM16(HandleJmpImm16, V)                \
    APPEND_JIT_SUFFIX(HandleJmpImm32, V)                      \
    APPEND_JIT_SUFFIX(HandleJeqzImm8, V)                      \
    APPEND_JIT_SUFFIX_IMM16(HandleJeqzImm16, V)               \
    APPEND_JIT_SUFFIX(HandleJeqzImm32, V)                     \
    APPEND_JIT_SUFFIX(HandleJnezImm8, V)                      \
    APPEND_JIT_SUFFIX_IMM16(HandleJnezImm16, V)               \
    APPEND_JIT_SUFFIX(HandleJnezImm32, V)                     \
    APPEND_JIT_SUFFIX(HandleReturn, V)                        \
    APPEND_JIT_SUFFIX(HandleReturnundefined, V)               \
    APPEND_JIT_SUFFIX(HandleSuspendgeneratorV8, V)            \
    APPEND_JIT_SUFFIX(HandleAsyncgeneratorresolveV8V8V8, V)

#define APPEND_SUFFIX(name, V) \
    V(name##WithProf, name, SlotIDFormat::IMM8)

#define APPEND_SUFFIX_IMM8_SECONDBC(name, V) \
    V(name##WithProf, name, SlotIDFormat::PREF_IMM8)

#define APPEND_SUFFIX_IMM16(name, V) \
    V(name##WithProf, name, SlotIDFormat::IMM16)

#define ASM_INTERPRETER_BC_PROFILER_STUB_LIST(V)          \
    ASM_INTERPRETER_BC_TYPE_PROFILER_STUB_LIST(V)         \
    ASM_INTERPRETER_BC_LAYOUT_PROFILER_STUB_LIST(V)       \
    ASM_INTERPRETER_BC_FUNC_HOT_PROFILER_STUB_LIST(V)     \
    ASM_INTERPRETER_BC_FUNC_COUNT_PROFILER_STUB_LIST(V)   \

#define ASM_INTERPRETER_BC_TYPE_PROFILER_STUB_LIST(V) \
    APPEND_SUFFIX(HandleAdd2Imm8V8, V)                \
    APPEND_SUFFIX(HandleSub2Imm8V8, V)                \
    APPEND_SUFFIX(HandleMul2Imm8V8, V)                \
    APPEND_SUFFIX(HandleDiv2Imm8V8, V)                \
    APPEND_SUFFIX(HandleMod2Imm8V8, V)                \
    APPEND_SUFFIX(HandleShl2Imm8V8, V)                \
    APPEND_SUFFIX(HandleShr2Imm8V8, V)                \
    APPEND_SUFFIX(HandleAnd2Imm8V8, V)                \
    APPEND_SUFFIX(HandleOr2Imm8V8, V)                 \
    APPEND_SUFFIX(HandleXor2Imm8V8, V)                \
    APPEND_SUFFIX(HandleAshr2Imm8V8, V)               \
    APPEND_SUFFIX(HandleExpImm8V8, V)                 \
    APPEND_SUFFIX(HandleNegImm8, V)                   \
    APPEND_SUFFIX(HandleNotImm8, V)                   \
    APPEND_SUFFIX(HandleEqImm8V8, V)                  \
    APPEND_SUFFIX(HandleIncImm8, V)                   \
    APPEND_SUFFIX(HandleDecImm8, V)                   \
    APPEND_SUFFIX(HandleNoteqImm8V8, V)               \
    APPEND_SUFFIX(HandleLessImm8V8, V)                \
    APPEND_SUFFIX(HandleLesseqImm8V8, V)              \
    APPEND_SUFFIX(HandleGreaterImm8V8, V)             \
    APPEND_SUFFIX(HandleGreatereqImm8V8, V)           \
    APPEND_SUFFIX(HandleStrictnoteqImm8V8, V)         \
    APPEND_SUFFIX(HandleStricteqImm8V8, V)            \
    APPEND_SUFFIX(HandleTonumericImm8, V)             \
    APPEND_SUFFIX_IMM8_SECONDBC(HandleCallRuntimeIstruePrefImm8, V)       \
    APPEND_SUFFIX_IMM8_SECONDBC(HandleCallRuntimeIsfalsePrefImm8, V)      \

#define ASM_INTERPRETER_BC_FUNC_HOT_PROFILER_STUB_LIST(V) \
    APPEND_SUFFIX(HandleJmpImm8, V)                       \
    APPEND_SUFFIX_IMM16(HandleJmpImm16, V)                \
    APPEND_SUFFIX(HandleJmpImm32, V)                      \
    APPEND_SUFFIX(HandleJeqzImm8, V)                      \
    APPEND_SUFFIX_IMM16(HandleJeqzImm16, V)               \
    APPEND_SUFFIX(HandleJeqzImm32, V)                     \
    APPEND_SUFFIX(HandleJnezImm8, V)                      \
    APPEND_SUFFIX_IMM16(HandleJnezImm16, V)               \
    APPEND_SUFFIX(HandleJnezImm32, V)                     \
    APPEND_SUFFIX(HandleReturn, V)                        \
    APPEND_SUFFIX(HandleReturnundefined, V)               \
    APPEND_SUFFIX(HandleSuspendgeneratorV8, V)            \
    APPEND_SUFFIX(HandleAsyncgeneratorresolveV8V8V8, V)

#define ASM_INTERPRETER_BC_FUNC_COUNT_PROFILER_STUB_LIST(V)    \
    APPEND_SUFFIX(HandleCallarg0Imm8, V)                       \
    APPEND_SUFFIX(HandleCallarg1Imm8V8, V)                     \
    APPEND_SUFFIX(HandleCallargs2Imm8V8V8, V)                  \
    APPEND_SUFFIX(HandleCallargs3Imm8V8V8V8, V)                \
    APPEND_SUFFIX(HandleCallrangeImm8Imm8V8, V)                \
    APPEND_SUFFIX_IMM16(HandleWideCallrangePrefImm16V8, V)     \
    APPEND_SUFFIX(HandleCallthisrangeImm8Imm8V8, V)            \
    APPEND_SUFFIX_IMM16(HandleWideCallthisrangePrefImm16V8, V) \
    APPEND_SUFFIX(HandleCallthis0Imm8V8, V)                    \
    APPEND_SUFFIX(HandleCallthis1Imm8V8V8, V)                  \
    APPEND_SUFFIX(HandleCallthis2Imm8V8V8V8, V)                \
    APPEND_SUFFIX(HandleCallthis3Imm8V8V8V8V8, V)              \
    APPEND_SUFFIX_IMM8_SECONDBC(HandleCallRuntimeCallInitPrefImm8V8, V)  \
    APPEND_SUFFIX(HandleNewobjrangeImm8Imm8V8, V)              \
    APPEND_SUFFIX_IMM16(HandleNewobjrangeImm16Imm8V8, V)       \
    APPEND_SUFFIX_IMM16(HandleWideNewobjrangePrefImm16V8, V)   \
    APPEND_SUFFIX(HandleInstanceofImm8V8, V)                   \
    APPEND_SUFFIX(HandleGetiteratorImm8, V)                    \
    APPEND_SUFFIX_IMM16(HandleGetiteratorImm16, V)             \
    APPEND_SUFFIX(HandleTryldglobalbynameImm8Id16, V)          \
    APPEND_SUFFIX_IMM16(HandleTryldglobalbynameImm16Id16, V)   \
    APPEND_SUFFIX(HandleTrystglobalbynameImm8Id16, V)          \
    APPEND_SUFFIX_IMM16(HandleTrystglobalbynameImm16Id16, V)   \
    APPEND_SUFFIX_IMM16(HandleLdglobalvarImm16Id16, V)

#define ASM_INTERPRETER_BC_LAYOUT_PROFILER_STUB_LIST(V)                     \
    APPEND_SUFFIX(HandleDefineclasswithbufferImm8Id16Id16Imm16V8, V)        \
    APPEND_SUFFIX_IMM16(HandleDefineclasswithbufferImm16Id16Id16Imm16V8, V) \
    APPEND_SUFFIX(HandleDefinefuncImm8Id16Imm8, V)                          \
    APPEND_SUFFIX_IMM16(HandleDefinefuncImm16Id16Imm8, V)                   \
    APPEND_SUFFIX(HandleDefinegettersetterbyvalueV8V8V8V8, V)               \
    APPEND_SUFFIX(HandleCreateobjectwithbufferImm8Id16, V)                  \
    APPEND_SUFFIX_IMM16(HandleCreateobjectwithbufferImm16Id16, V)           \
    APPEND_SUFFIX(HandleCreatearraywithbufferImm8Id16, V)                   \
    APPEND_SUFFIX_IMM16(HandleCreatearraywithbufferImm16Id16, V)            \
    APPEND_SUFFIX(HandleCreateemptyobject, V)                               \
    APPEND_SUFFIX(HandleCreateemptyarrayImm8, V)                            \
    APPEND_SUFFIX_IMM16(HandleCreateemptyarrayImm16, V)                     \
    APPEND_SUFFIX(HandleLdobjbynameImm8Id16, V)                             \
    APPEND_SUFFIX_IMM16(HandleLdobjbynameImm16Id16, V)                      \
    APPEND_SUFFIX_IMM16(HandleLdthisbynameImm16Id16, V)                     \
    APPEND_SUFFIX(HandleLdthisbynameImm8Id16, V)                            \
    APPEND_SUFFIX(HandleStthisbynameImm8Id16, V)                            \
    APPEND_SUFFIX_IMM16(HandleStthisbynameImm16Id16, V)                     \
    APPEND_SUFFIX(HandleStthisbyvalueImm8V8, V)                             \
    APPEND_SUFFIX_IMM16(HandleStthisbyvalueImm16V8, V)                      \
    APPEND_SUFFIX_IMM16(HandleStobjbyvalueImm16V8V8, V)                     \
    APPEND_SUFFIX(HandleStobjbynameImm8Id16V8, V)                           \
    APPEND_SUFFIX_IMM16(HandleStobjbynameImm16Id16V8, V)                    \
    APPEND_SUFFIX(HandleDefineFieldByNameImm8Id16V8, V)                     \
    APPEND_SUFFIX(HandleDefinePropertyByNameImm8Id16V8, V)                  \
    APPEND_SUFFIX(HandleLdPrivatePropertyImm8Imm16Imm16, V)                 \
    APPEND_SUFFIX(HandleStPrivatePropertyImm8Imm16Imm16V8, V)               \
    APPEND_SUFFIX(HandleStobjbyvalueImm8V8V8, V)                            \
    APPEND_SUFFIX(HandleStobjbyindexImm8V8Imm16, V)                         \
    APPEND_SUFFIX_IMM16(HandleStobjbyindexImm16V8Imm16, V)                  \
    APPEND_SUFFIX(HandleLdobjbyvalueImm8V8, V)                              \
    APPEND_SUFFIX_IMM16(HandleLdobjbyvalueImm16V8, V)                       \
    APPEND_SUFFIX_IMM16(HandleLdthisbyvalueImm16, V)                        \
    APPEND_SUFFIX(HandleLdthisbyvalueImm8, V)                               \
    APPEND_SUFFIX(HandleLdobjbyindexImm8Imm16, V)                           \
    APPEND_SUFFIX_IMM16(HandleLdobjbyindexImm16Imm16, V)                    \
    APPEND_SUFFIX(HandleWideLdobjbyindexPrefImm32, V)                       \
    APPEND_SUFFIX(HandleWideStobjbyindexPrefV8Imm32, V)                     \
    APPEND_SUFFIX_IMM16(HandleStownbyindexImm16V8Imm16, V)                  \
    APPEND_SUFFIX(HandleStownbyindexImm8V8Imm16, V)                         \
    APPEND_SUFFIX(HandleWideStownbyindexPrefV8Imm32, V)                     \
    APPEND_SUFFIX_IMM16(HandleStownbyvaluewithnamesetImm16V8V8, V)          \
    APPEND_SUFFIX(HandleStownbyvaluewithnamesetImm8V8V8, V)                 \
    APPEND_SUFFIX(HandleStownbyvalueImm8V8V8, V)                            \
    APPEND_SUFFIX_IMM16(HandleStownbyvalueImm16V8V8, V)                     \
    APPEND_SUFFIX_IMM16(HandleStownbynamewithnamesetImm16Id16V8, V)         \
    APPEND_SUFFIX(HandleStownbynamewithnamesetImm8Id16V8, V)                \
    APPEND_SUFFIX_IMM16(HandleStownbynameImm16Id16V8, V)                    \
    APPEND_SUFFIX(HandleStownbynameImm8Id16V8, V)

#define INTERPRETER_DISABLE_SINGLE_STEP_DEBUGGING_BC_STUB_LIST(V)             \
    ASM_INTERPRETER_BC_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)           \
    ASM_INTERPRETER_DEPRECATED_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)   \
    ASM_INTERPRETER_WIDE_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)         \
    ASM_INTERPRETER_THROW_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)        \
    ASM_INTERPRETER_CALLRUNTIME_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)

#define INTERPRETER_BC_STUB_LIST(V)                              \
    ASM_INTERPRETER_BC_STUB_LIST(IGNORE_BC_STUB, V, V)           \
    ASM_INTERPRETER_DEPRECATED_STUB_LIST(IGNORE_BC_STUB, V, V)   \
    ASM_INTERPRETER_WIDE_STUB_LIST(IGNORE_BC_STUB, V, V)         \
    ASM_INTERPRETER_THROW_STUB_LIST(IGNORE_BC_STUB, V, V)        \
    ASM_INTERPRETER_CALLRUNTIME_STUB_LIST(IGNORE_BC_STUB, V, V)

#define ASM_INTERPRETER_BC_STUB_ID_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(V, V, V)

#define ASM_INTERPRETER_SECOND_BC_STUB_ID_LIST(V)   \
    ASM_INTERPRETER_WIDE_STUB_LIST(V, V, V)         \
    ASM_INTERPRETER_THROW_STUB_LIST(V, V, V)        \
    ASM_INTERPRETER_DEPRECATED_STUB_LIST(V, V, V)   \
    ASM_INTERPRETER_CALLRUNTIME_STUB_LIST(V, V, V)

class BytecodeStubCSigns {
public:
    // is uint8 max
    static constexpr size_t LAST_VALID_OPCODE = 0xFF;
    // all valid stub, include normal and helper stub
    enum ValidID {
#define DEF_VALID_BC_STUB_ID(name) name,
        INTERPRETER_BC_STUB_LIST(DEF_VALID_BC_STUB_ID)
        ASM_INTERPRETER_BC_HELPER_STUB_LIST(DEF_VALID_BC_STUB_ID)
#undef DEF_VALID_BC_STUB_ID
#define DEF_VALID_BC_STUB_ID(name, ...) name,
        ASM_INTERPRETER_BC_PROFILER_STUB_LIST(DEF_VALID_BC_STUB_ID)
#undef DEF_VALID_BC_STUB_ID
#define DEF_VALID_BC_STUB_ID(name, ...) name,
        ASM_INTERPRETER_BC_JIT_PROFILER_STUB_LIST(DEF_VALID_BC_STUB_ID)
#undef DEF_VALID_BC_STUB_ID
        NUM_OF_VALID_STUBS
    };

#define DEF_BC_STUB_ID(name) PREF_ID_##name,
    enum WideID {
        ASM_INTERPRETER_WIDE_STUB_LIST(DEF_BC_STUB_ID, DEF_BC_STUB_ID, DEF_BC_STUB_ID)
        NUM_OF_WIDE_STUBS
    };
    enum ThrowID {
        ASM_INTERPRETER_THROW_STUB_LIST(DEF_BC_STUB_ID, DEF_BC_STUB_ID, DEF_BC_STUB_ID)
        NUM_OF_THROW_STUBS
    };
    enum DeprecatedID {
        ASM_INTERPRETER_DEPRECATED_STUB_LIST(DEF_BC_STUB_ID, DEF_BC_STUB_ID, DEF_BC_STUB_ID)
        NUM_OF_DEPRECATED_STUBS
    };
    enum CallRuntimeID {
        ASM_INTERPRETER_CALLRUNTIME_STUB_LIST(DEF_BC_STUB_ID, DEF_BC_STUB_ID, DEF_BC_STUB_ID)
        NUM_OF_CALLRUNTIME_STUBS
    };
#undef DEF_BC_STUB_ID

#define DEF_BC_STUB_ID(name) ID_##name,
    enum ID {
        ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_BC_STUB_ID)
        NUM_OF_ALL_NORMAL_STUBS,
        lastOpcode = LAST_VALID_OPCODE, // last Opcode is max opcode size
        ASM_INTERPRETER_SECOND_BC_STUB_ID_LIST(DEF_BC_STUB_ID)
        ASM_INTERPRETER_BC_HELPER_STUB_LIST(DEF_BC_STUB_ID)
#undef DEF_BC_STUB_ID
#define DEF_BC_STUB_ID(name, ...) ID_##name,
        ASM_INTERPRETER_BC_PROFILER_STUB_LIST(DEF_BC_STUB_ID)
#undef DEF_BC_STUB_ID
#define DEF_BC_STUB_ID(name, ...) ID_##name,
        ASM_INTERPRETER_BC_JIT_PROFILER_STUB_LIST(DEF_BC_STUB_ID)
        NUM_OF_STUBS,
        ID_Wide_Start = lastOpcode + 1,
        ID_Throw_Start = ID_Wide_Start + NUM_OF_WIDE_STUBS,
        ID_Deprecated_Start = ID_Throw_Start + NUM_OF_THROW_STUBS,
        ID_CallRuntime_Start = ID_Deprecated_Start + NUM_OF_DEPRECATED_STUBS
    };
#undef DEF_BC_STUB_ID

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& outCSigns);

    static const CallSignature* Get(size_t index)
    {
        ASSERT(index < NUM_OF_VALID_STUBS);
        return &callSigns_[index];
    }

    static const CallSignature* BCDebuggerHandler()
    {
        return &bcDebuggerHandlerCSign_;
    }

    static const CallSignature* BCHandler()
    {
        return &bcHandlerCSign_;
    }

private:
    static CallSignature callSigns_[NUM_OF_VALID_STUBS];
    static CallSignature bcHandlerCSign_;
    static CallSignature bcDebuggerHandlerCSign_;
};

enum class InterpreterHandlerInputs : size_t {
    GLUE = 0,
    SP,
    PC,
    CONSTPOOL,
    PROFILE_TYPE_INFO,
    ACC,
    HOTNESS_COUNTER,
    NUM_OF_INPUTS
};

enum class CallDispatchInputs : size_t {
    GLUE = 0,
    SP,
    CALL_TARGET,
    METHOD,
    CALL_FIELD,
    ARG0,
    ARG1,
    ARG2,
    ARG3,
    NUM_OF_INPUTS,

    ARGC = ARG0,
    ARGV = ARG1,
};

#define BYTECODE_STUB_END_ID BytecodeStubCSigns::ID_ExceptionHandler
#define BCSTUB_ID(name) kungfu::BytecodeStubCSigns::ID_##name
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BC_CALL_SIGNATURE_H
