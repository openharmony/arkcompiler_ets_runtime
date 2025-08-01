/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_RUNTIME_STUB_LIST_H
#define ECMASCRIPT_RUNTIME_STUB_LIST_H

#include "ecmascript/stubs/test_runtime_stubs.h"

namespace panda::ecmascript {

#define RUNTIME_ASM_STUB_LIST(V)             \
    JS_CALL_TRAMPOLINE_LIST(V)               \
    FAST_CALL_TRAMPOLINE_LIST(V)             \
    ASM_INTERPRETER_TRAMPOLINE_LIST(V)       \
    BASELINE_TRAMPOLINE_LIST(V)

#define ASM_INTERPRETER_TRAMPOLINE_LIST(V)   \
    V(AsmInterpreterEntry)                   \
    V(GeneratorReEnterAsmInterp)             \
    V(PushCallArgsAndDispatchNative)         \
    V(PushCallArg0AndDispatch)               \
    V(PushCallArg1AndDispatch)               \
    V(PushCallArgs2AndDispatch)              \
    V(PushCallArgs3AndDispatch)              \
    V(PushCallThisArg0AndDispatch)           \
    V(PushCallThisArg1AndDispatch)           \
    V(PushCallThisArgs2AndDispatch)          \
    V(PushCallThisArgs3AndDispatch)          \
    V(PushCallRangeAndDispatch)              \
    V(PushCallNewAndDispatch)                \
    V(PushSuperCallAndDispatch)              \
    V(PushCallNewAndDispatchNative)          \
    V(PushNewTargetAndDispatchNative)        \
    V(PushCallRangeAndDispatchNative)        \
    V(PushCallThisRangeAndDispatch)          \
    V(ResumeRspAndDispatch)                  \
    V(ResumeRspAndReturn)                    \
    V(ResumeRspAndReturnBaseline)            \
    V(ResumeCaughtFrameAndDispatch)          \
    V(ResumeUncaughtFrameAndReturn)          \
    V(ResumeRspAndRollback)                  \
    V(CallSetter)                            \
    V(CallGetter)                            \
    V(CallContainersArgs2)                   \
    V(CallContainersArgs3)                   \
    V(CallReturnWithArgv)                    \
    V(CallGetterToBaseline)                  \
    V(CallSetterToBaseline)                  \
    V(CallContainersArgs2ToBaseline)         \
    V(CallContainersArgs3ToBaseline)         \
    V(CallReturnWithArgvToBaseline)          \
    V(ASMFastWriteBarrier)

#define BASELINE_TRAMPOLINE_LIST(V)                   \
    V(CallArg0AndCheckToBaseline)                     \
    V(CallArg1AndCheckToBaseline)                     \
    V(CallArgs2AndCheckToBaseline)                    \
    V(CallArgs3AndCheckToBaseline)                    \
    V(CallThisArg0AndCheckToBaseline)                 \
    V(CallThisArg1AndCheckToBaseline)                 \
    V(CallThisArgs2AndCheckToBaseline)                \
    V(CallThisArgs3AndCheckToBaseline)                \
    V(CallRangeAndCheckToBaseline)                    \
    V(CallNewAndCheckToBaseline)                      \
    V(SuperCallAndCheckToBaseline)                    \
    V(CallThisRangeAndCheckToBaseline)                \
    V(CallArg0AndDispatchFromBaseline)                \
    V(CallArg1AndDispatchFromBaseline)                \
    V(CallArgs2AndDispatchFromBaseline)               \
    V(CallArgs3AndDispatchFromBaseline)               \
    V(CallThisArg0AndDispatchFromBaseline)            \
    V(CallThisArg1AndDispatchFromBaseline)            \
    V(CallThisArgs2AndDispatchFromBaseline)           \
    V(CallThisArgs3AndDispatchFromBaseline)           \
    V(CallRangeAndDispatchFromBaseline)               \
    V(CallNewAndDispatchFromBaseline)                 \
    V(SuperCallAndDispatchFromBaseline)               \
    V(CallThisRangeAndDispatchFromBaseline)           \
    V(CallArg0AndCheckToBaselineFromBaseline)         \
    V(CallArg1AndCheckToBaselineFromBaseline)         \
    V(CallArgs2AndCheckToBaselineFromBaseline)        \
    V(CallArgs3AndCheckToBaselineFromBaseline)        \
    V(CallThisArg0AndCheckToBaselineFromBaseline)     \
    V(CallThisArg1AndCheckToBaselineFromBaseline)     \
    V(CallThisArgs2AndCheckToBaselineFromBaseline)    \
    V(CallThisArgs3AndCheckToBaselineFromBaseline)    \
    V(CallRangeAndCheckToBaselineFromBaseline)        \
    V(CallNewAndCheckToBaselineFromBaseline)          \
    V(SuperCallAndCheckToBaselineFromBaseline)        \
    V(CallThisRangeAndCheckToBaselineFromBaseline)    \
    V(GetBaselineBuiltinFp)

#define JS_CALL_TRAMPOLINE_LIST(V)           \
    V(CallRuntime)                           \
    V(CallRuntimeWithArgv)                   \
    V(JSFunctionEntry)                       \
    V(JSCall)                                \
    V(JSCallWithArgV)                        \
    V(JSCallWithArgVAndPushArgv)             \
    V(JSProxyCallInternalWithArgV)           \
    V(SuperCallWithArgV)                     \
    V(OptimizedCallAndPushArgv)              \
    V(DeoptHandlerAsm)                       \
    V(JSCallNew)                             \
    V(CallOptimized)                         \
    V(AOTCallToAsmInterBridge)               \
    V(FastCallToAsmInterBridge)

#define FAST_CALL_TRAMPOLINE_LIST(V)         \
    V(OptimizedFastCallEntry)                \
    V(OptimizedFastCallAndPushArgv)          \
    V(JSFastCallWithArgV)                    \
    V(JSFastCallWithArgVAndPushArgv)

#define RUNTIME_STUB_WITH_DFX(V)                \
    V(TraceLoadGetter)                          \
    V(TraceLoadSlowPath)                        \
    V(TraceLoadDetail)                          \
    V(TraceLoadEnd)                             \
    V(TraceLoadValueSlowPath)                   \
    V(TraceLoadValueDetail)                     \
    V(TraceLoadValueEnd)                        \
    V(TraceStoreFastPath)                       \
    V(TraceStoreSlowPath)                       \
    V(TraceStoreDetail)                         \
    V(TraceStoreEnd)                            \
    V(TraceNum)                                 \
    V(TraceCallDetail)                          \
    V(TraceCallEnd)                             \
    V(TraceDefineFunc)                          \
    V(TraceDefineFuncEnd)                       \
    V(TraceLazyDeoptNum)                        \
    V(TraceLazyDeoptFailNum)

#define RUNTIME_STUB_WITHOUT_GC_LIST(V)        \
    V(Dump)                                    \
    V(DebugDump)                               \
    V(DumpWithHint)                            \
    V(DebugDumpWithHint)                       \
    V(DebugPrint)                              \
    V(DebugPrintCustom)                        \
    V(DebugPrintInstruction)                   \
    V(CollectingOpcodes)                       \
    V(DebugOsrEntry)                           \
    V(Comment)                                 \
    V(FatalPrint)                              \
    V(FatalPrintCustom)                        \
    V(GetActualArgvNoGC)                       \
    V(InsertOldToNewRSet)                      \
    V(InsertLocalToShareRSet)                  \
    V(SetBitAtomic)                            \
    V(MarkingBarrier)                          \
    V(SharedGCMarkingBarrier)                  \
    V(CMCGCMarkingBarrier)                     \
    V(DoubleToInt)                             \
    V(SaturateTruncDoubleToInt32)              \
    V(FloatMod)                                \
    V(FloatAcos)                               \
    V(FloatAcosh)                              \
    V(FloatAsin)                               \
    V(FloatAsinh)                              \
    V(FloatAtan)                               \
    V(FloatAtan2)                              \
    V(FloatAtanh)                              \
    V(FloatCos)                                \
    V(FloatCosh)                               \
    V(FloatSin)                                \
    V(FloatSinh)                               \
    V(FloatTan)                                \
    V(FloatTanh)                               \
    V(FloatTrunc)                              \
    V(FloatLog)                                \
    V(FloatLog2)                               \
    V(FloatLog10)                              \
    V(FloatLog1p)                              \
    V(FloatExp)                                \
    V(FloatExpm1)                              \
    V(FloatCbrt)                               \
    V(FloatFloor)                              \
    V(FloatPow)                                \
    V(FloatCeil)                               \
    V(CallDateNow)                             \
    V(UpdateFieldType)                         \
    V(BigIntEquals)                            \
    V(TimeClip)                                \
    V(LazyDeoptEntry)                          \
    V(SetDateValues)                           \
    V(StartCallTimer)                          \
    V(EndCallTimer)                            \
    V(BigIntSameValueZero)                     \
    V(JSHClassFindProtoTransitions)            \
    V(FinishObjSizeTracking)                   \
    V(NumberHelperStringToDouble)              \
    V(IntLexicographicCompare)                 \
    V(DoubleLexicographicCompare)              \
    V(FastArraySortString)                     \
    V(StringToNumber)                          \
    V(StringGetStart)                          \
    V(StringGetEnd)                            \
    V(ArrayTrim)                               \
    V(SortTypedArray)                          \
    V(ReverseTypedArray)                       \
    V(CopyTypedArrayBuffer)                    \
    V(IsFastRegExp)                            \
    V(CopyObjectPrimitive)                     \
    V(CreateLocalToShare)                      \
    V(CreateOldToNew)                          \
    V(ObjectCopy)                              \
    V(FillObject)                              \
    V(ReverseArray)                            \
    V(LrInt)                                   \
    V(FindPatchModule)                         \
    V(FatalPrintMisstakenResolvedBinding)      \
    V(LoadNativeModuleFailed)                  \
    V(GetExternalModuleVar)                    \
    V(ReadBarrier)                             \
    V(CopyCallTarget)                          \
    V(CopyArgvArray)                           \
    V(MarkRSetCardTable)                       \
    V(MarkInBuffer)                            \
    V(BatchMarkInBuffer)                       \
    V(UpdateSharedModule)

// When ASM enters C++ via CallNGCRuntime, if the C++ process requires GetGlobalEnv(),
// the current globalenv in ASM must be set to glue before CallNGCRuntime!
// None of the current NGC stubs are related to globalenv and are not distinguished.

#define RUNTIME_STUB_WITH_GC_WITH_GLOBALENV_LIST(V)            \
    V(TypedArraySpeciesCreate)                                 \
    V(TypedArrayCreateSameType)                                \
    V(CallInternalGetter)                                      \
    V(CallInternalSetter)                                      \
    V(CallInternalSetterNoThrow)                               \
    V(CallGetPrototype)                                        \
    V(RegularJSObjDeletePrototype)                             \
    V(CallJSObjDeletePrototype)                                \
    V(ToPropertyKey)                                           \
    V(ThrowTypeError)                                          \
    V(MismatchError)                                           \
    V(NotifyArrayPrototypeChanged)                             \
    V(NumberToString)                                          \
    V(NameDictPutIfAbsent)                                     \
    V(JSArrayReduceUnStable)                                   \
    V(JSArrayFilterUnStable)                                   \
    V(JSArrayMapUnStable)                                      \
    V(CheckAndCopyArray)                                       \
    V(UpdateHClassForElementsKind)                             \
    V(StGlobalRecord)                                          \
    V(SetFunctionNameNoPrefix)                                 \
    V(StOwnByValueWithNameSet)                                 \
    V(StOwnByName)                                             \
    V(StOwnByNameWithNameSet)                                  \
    V(SuspendGenerator)                                        \
    V(IsIn)                                                    \
    V(InstanceOf)                                              \
    V(CreateGeneratorObj)                                      \
    V(ThrowConstAssignment)                                    \
    V(GetTemplateObject)                                       \
    V(CreateStringIterator)                                    \
    V(NewJSArrayIterator)                                      \
    V(NewJSTypedArrayIterator)                                 \
    V(StringIteratorNext)                                      \
    V(ArrayIteratorNext)                                       \
    V(IteratorReturn)                                          \
    V(GetNextPropNameSlowpath)                                 \
    V(ThrowIfNotObject)                                        \
    V(CloseIterator)                                           \
    V(SuperCallSpread)                                         \
    V(OptSuperCallSpread)                                      \
    V(GetCallSpreadArgs)                                       \
    V(DelObjProp)                                              \
    V(NewObjApply)                                             \
    V(CreateIterResultObj)                                     \
    V(AsyncFunctionAwaitUncaught)                              \
    V(AsyncFunctionResolveOrReject)                            \
    V(ThrowUndefinedIfHole)                                    \
    V(CopyDataProperties)                                      \
    V(StArraySpread)                                           \
    V(GetIteratorNext)                                         \
    V(SetObjectWithProto)                                      \
    V(LoadICByValue)                                           \
    V(StoreICByValue)                                          \
    V(StoreOwnICByValue)                                       \
    V(StOwnByValue)                                            \
    V(LdSuperByValue)                                          \
    V(StSuperByValue)                                          \
    V(StObjByValue)                                            \
    V(LdObjByIndex)                                            \
    V(StObjByIndex)                                            \
    V(StOwnByIndex)                                            \
    V(CreateClassWithBuffer)                                   \
    V(LoadICByName)                                            \
    V(StoreICByName)                                           \
    V(StoreOwnICByName)                                        \
    V(GetModuleNamespaceByIndex)                               \
    V(GetModuleNamespaceByIndexOnJSFunc)                       \
    V(GetModuleNamespace)                                      \
    V(StModuleVarByIndex)                                      \
    V(StModuleVarByIndexOnJSFunc)                              \
    V(StModuleVar)                                             \
    V(LdLocalModuleVarByIndex)                                 \
    V(LdLocalModuleVarByIndexWithModule)                       \
    V(LdExternalModuleVarByIndex)                              \
    V(LdExternalModuleVarByIndexWithModule)                    \
    V(LdLazyExternalModuleVarByIndex)                          \
    V(LdLocalModuleVarByIndexOnJSFunc)                         \
    V(LdExternalModuleVarByIndexOnJSFunc)                      \
    V(LdModuleVar)                                             \
    V(GetModuleValueOuterInternal)                             \
    V(ThrowExportsIsHole)                                      \
    V(HandleResolutionIsNullOrString)                          \
    V(CheckAndThrowModuleError)                                \
    V(GetResolvedRecordIndexBindingModule)                     \
    V(GetResolvedRecordBindingModule)                          \
    V(GetPropIteratorSlowpath)                                 \
    V(PrimitiveStringCreate)                                   \
    V(AsyncFunctionEnter)                                      \
    V(GetAsyncIterator)                                        \
    V(ThrowThrowNotExists)                                     \
    V(ThrowPatternNonCoercible)                                \
    V(ThrowDeleteSuperProperty)                                \
    V(TryLdGlobalICByName)                                     \
    V(LoadMiss)                                                \
    V(StoreMiss)                                               \
    V(TryUpdateGlobalRecord)                                   \
    V(ThrowReferenceError)                                     \
    V(StGlobalVar)                                             \
    V(LdGlobalICVar)                                           \
    V(ToIndex)                                                 \
    V(NewJSObjectByConstructor)                                \
    V(AllocateTypedArrayBuffer)                                \
    V(ToNumber)                                                \
    V(CreateEmptyObject)                                       \
    V(GetUnmapedArgs)                                          \
    V(CopyRestArgs)                                            \
    V(CreateArrayWithBuffer)                                   \
    V(CreateObjectWithBuffer)                                  \
    V(NewThisObject)                                           \
    V(NewObjRange)                                             \
    V(DefineFunc)                                              \
    V(CreateRegExpWithLiteral)                                 \
    V(ThrowIfSuperNotCorrectCall)                              \
    V(CreateObjectHavingMethod)                                \
    V(CreateObjectWithExcludedKeys)                            \
    V(DefineMethod)                                            \
    V(ThrowSetterIsUndefinedException)                         \
    V(ThrowNotCallableException)                               \
    V(ThrowCallConstructorException)                           \
    V(ThrowNonConstructorException)                            \
    V(ThrowStackOverflowException)                             \
    V(ThrowDerivedMustReturnException)                         \
    V(CallSpread)                                              \
    V(DefineGetterSetterByValue)                               \
    V(SuperCall)                                               \
    V(OptSuperCall)                                            \
    V(LdBigInt)                                                \
    V(ToNumeric)                                               \
    V(ToNumericConvertBigInt)                                  \
    V(CallBigIntAsIntN)                                        \
    V(CallBigIntAsUintN)                                       \
    V(DynamicImport)                                           \
    V(CreateAsyncGeneratorObj)                                 \
    V(AsyncGeneratorResolve)                                   \
    V(AsyncGeneratorReject)                                    \
    V(OptGetUnmapedArgs)                                       \
    V(OptCopyRestArgs)                                         \
    V(OptSuspendGenerator)                                     \
    V(OptAsyncGeneratorResolve)                                \
    V(OptCreateObjectWithExcludedKeys)                         \
    V(OptNewObjRange)                                          \
    V(SetTypeArrayPropertyByIndex)                             \
    V(FastCopyElementToArray)                                  \
    V(GetPropertyByName)                                       \
    V(JSObjectGetMethod)                                       \
    V(OptLdSuperByValue)                                       \
    V(OptStSuperByValue)                                       \
    V(LdPatchVar)                                              \
    V(StPatchVar)                                              \
    V(ContainerRBTreeForEach)                                  \
    V(DefineField)                                             \
    V(CreatePrivateProperty)                                   \
    V(DefinePrivateProperty)                                   \
    V(LdPrivateProperty)                                       \
    V(StPrivateProperty)                                       \
    V(TestIn)                                                  \
    V(LocaleCompare)                                           \
    V(ArraySort)                                               \
    V(FastStringify)                                           \
    V(ObjectSlowAssign)                                        \
    V(LocaleCompareWithGc)                                     \
    V(ParseInt)                                                \
    V(LocaleCompareCacheable)                                  \
    V(ArrayForEachContinue)                                    \
    V(NumberDictionaryPut)                                     \
    V(ThrowRangeError)                                         \
    V(InitializeGeneratorFunction)                             \
    V(FunctionDefineOwnProperty)                               \
    V(DefineOwnProperty)                                       \
    V(AOTEnableProtoChangeMarker)                              \
    V(CheckGetTrapResult)                                      \
    V(CheckSetTrapResult)                                      \
    V(JSProxyGetProperty)                                      \
    V(JSProxySetProperty)                                      \
    V(JSProxyHasProperty)                                      \
    V(JSTypedArrayHasProperty)                                 \
    V(ModuleNamespaceHasProperty)                              \
    V(JSObjectHasProperty)                                     \
    V(HasProperty)                                             \
    V(FastCopyFromArrayToTypedArray)                           \
    V(BigIntConstructor)                                       \
    V(ObjectPrototypeHasOwnProperty)                           \
    V(ReflectHas)                                              \
    V(ReflectConstruct)                                        \
    V(ReflectApply)                                            \
    V(FunctionPrototypeApply)                                  \
    V(FunctionPrototypeBind)                                   \
    V(FunctionPrototypeCall)                                   \
    V(SuperCallForwardAllArgs)                                 \
    V(OptSuperCallForwardAllArgs)

// When ASM enters C++ via CallRuntime, if the C++ process requires GetGlobalEnv(),
// the current globalenv in ASM must be set to glue before CallRuntime!
// Use CallRuntimeWithGlobalEnv or CallRuntimeWithCurrentEnv.

#define RUNTIME_STUB_WITH_GC_WITHOUT_GLOBALENV_LIST(V)         \
    V(HeapAlloc)                                               \
    V(AllocateInYoung)                                         \
    V(AllocateInOld)                                           \
    V(AllocateInSOld)                                          \
    V(AllocateInSNonMovable)                                   \
    V(GetHash32)                                               \
    V(NewInternalString)                                       \
    V(NewTaggedArray)                                          \
    V(NewCOWTaggedArray)                                       \
    V(NewMutantTaggedArray)                                    \
    V(NewCOWMutantTaggedArray)                                 \
    V(CopyArray)                                               \
    V(IntToString)                                             \
    V(RTSubstitution)                                          \
    V(NameDictionaryGetAllEnumKeys)                            \
    V(NumberDictionaryGetAllEnumKeys)                          \
    V(PropertiesSetValue)                                      \
    V(NewEcmaHClass)                                           \
    V(UpdateLayOutAndAddTransition)                            \
    V(CopyAndUpdateObjLayout)                                  \
    V(RuntimeDump)                                             \
    V(ForceGC)                                                 \
    V(NoticeThroughChainAndRefreshUser)                        \
    V(JumpToCInterpreter)                                      \
    V(UpFrame)                                                 \
    V(Neg)                                                     \
    V(Not)                                                     \
    V(Inc)                                                     \
    V(Dec)                                                     \
    V(Shl2)                                                    \
    V(Shr2)                                                    \
    V(Ashr2)                                                   \
    V(Or2)                                                     \
    V(Xor2)                                                    \
    V(And2)                                                    \
    V(Exp)                                                     \
    V(Eq)                                                      \
    V(NotEq)                                                   \
    V(Less)                                                    \
    V(LessEq)                                                  \
    V(Greater)                                                 \
    V(GreaterEq)                                               \
    V(Add2)                                                    \
    V(Sub2)                                                    \
    V(Mul2)                                                    \
    V(Div2)                                                    \
    V(Mod2)                                                    \
    V(SetClassConstructorLength)                               \
    V(UpdateHotnessCounter)                                    \
    V(CheckSafePoint)                                          \
    V(PGODump)                                                 \
    V(PGOPreDump)                                              \
    V(JitCompile)                                              \
    V(CountInterpExecFuncs)                                    \
    V(BaselineJitCompile)                                      \
    V(UpdateHotnessCounterWithProf)                            \
    V(CreateSharedClass)                                       \
    V(LdSendableClass)                                         \
    V(LdSendableExternalModuleVarByIndex)                      \
    V(LdSendableLocalModuleVarByIndex)                         \
    V(LdLazySendableExternalModuleVarByIndex)                  \
    V(GetModuleName)                                           \
    V(NewResolvedIndexBindingRecord)                           \
    V(Throw)                                                   \
    V(SetGeneratorState)                                       \
    V(CloneHclass)                                             \
    V(ToBoolean)                                               \
    V(SetPatchModule)                                          \
    V(NewLexicalEnvWithName)                                   \
    V(OptNewLexicalEnvWithName)                                \
    V(NewSendableEnv)                                          \
    V(NotifyBytecodePcChanged)                                 \
    V(NotifyDebuggerStatement)                                 \
    V(MethodEntry)                                             \
    V(MethodExit)                                              \
    V(GetTypeArrayPropertyByIndex)                             \
    V(DebugAOTPrint)                                           \
    V(ProfileOptimizedCode)                                    \
    V(ProfileTypedOp)                                          \
    V(VerifyVTableLoading)                                     \
    V(VerifyVTableStoring)                                     \
    V(GetMethodFromCache)                                      \
    V(GetArrayLiteralFromCache)                                \
    V(GetObjectLiteralFromCache)                               \
    V(GetStringFromCache)                                      \
    V(BigIntEqual)                                             \
    V(StringEqual)                                             \
    V(StringIndexOf)                                           \
    V(DeoptHandler)                                            \
    V(InsertStringToTable)                                     \
    V(GetOrInternStringFromHashTrieTable)                      \
    V(SlowFlattenString)                                       \
    V(NotifyConcurrentResult)                                  \
    V(UpdateAOTHClass)                                         \
    V(AotInlineTrace)                                          \
    V(AotInlineBuiltinTrace)                                   \
    V(GetLinkedHash)                                           \
    V(LinkedHashMapComputeCapacity)                            \
    V(LinkedHashSetComputeCapacity)                            \
    V(JSObjectGrowElementsCapacity)                            \
    V(HClassCloneWithAddProto)                                 \
    V(DumpObject)                                              \
    V(DumpHeapObjectAddress)                                   \
    V(TryGetInternString)                                      \
    V(SetPrototypeTransition)                                  \
    V(GetSharedModule)                                         \
    V(GetCollationValueFromIcuCollator)                        \
    V(DecodeURIComponent)                                      \
    V(GetAllFlagsInternal)                                     \
    V(SlowSharedObjectStoreBarrier)                            \
    V(GetNativePcOfstForBaseline)                              \
    V(AotCallBuiltinTrace)                                     \
    V(NumberBigIntNativePointerToString)                       \
    V(ComputeHashcode)


#define RUNTIME_STUB_WITH_GC_LIST(V)               \
    RUNTIME_STUB_WITH_GC_WITH_GLOBALENV_LIST(V)    \
    RUNTIME_STUB_WITH_GC_WITHOUT_GLOBALENV_LIST(V)

#define RUNTIME_STUB_LIST(V)                     \
    RUNTIME_ASM_STUB_LIST(V)                     \
    RUNTIME_STUB_WITHOUT_GC_LIST(V)              \
    RUNTIME_STUB_WITH_GC_LIST(V)                 \
    RUNTIME_STUB_WITH_DFX(V)                     \
    TEST_RUNTIME_STUB_GC_LIST(V)

}  // namespace panda::ecmascript
#endif // ECMASCRIPT_RUNTIME_STUB_LIST_H
