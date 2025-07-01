/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_RUNTIME_CALL_ID_H
#define ECMASCRIPT_RUNTIME_CALL_ID_H

#include "ecmascript/base/config.h"
#include "ecmascript/dfx/vmstat/runtime_stat.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_LIST(V)   \
    V(RunInternal)                   \
    V(Ldnan)                         \
    V(Ldinfinity)                    \
    V(Ldundefined)                   \
    V(Ldboolean)                     \
    V(Ldnumber)                      \
    V(Ldstring)                      \
    V(Ldnull)                        \
    V(Ldsymbol)                      \
    V(Ldfunction)                    \
    V(Ldglobal)                      \
    V(Ldtrue)                        \
    V(Ldfalse)                       \
    V(Tonumber)                      \
    V(Toboolean)                     \
    V(Add2)                          \
    V(Sub2)                          \
    V(Mul2)                          \
    V(Div2)                          \
    V(Mod2)                          \
    V(Eq)                            \
    V(NotEq)                         \
    V(Less)                          \
    V(LessEq)                        \
    V(Greater)                       \
    V(GreaterEq)                     \
    V(StrictNotEq)                   \
    V(StrictEq)                      \
    V(Shl2)                          \
    V(Shr2)                          \
    V(Ashr2)                         \
    V(And2)                          \
    V(Or2)                           \
    V(Xor2)                          \
    V(Neg)                           \
    V(Not)                           \
    V(Inc)                           \
    V(Dec)                           \
    V(Exp)                           \
    V(Throw)                         \
    V(LdObjByIndex)                  \
    V(StObjByIndex)                  \
    V(LdObjByName)                   \
    V(StObjByName)                   \
    V(LdObjByValue)                  \
    V(StObjByValue)                  \
    V(StOwnByName)                   \
    V(StOwnById)                     \
    V(StOwnByValue)                  \
    V(Trygetobjprop)                 \
    V(Delobjprop)                    \
    V(Defineglobalvar)               \
    V(Definelocalvar)                \
    V(Definefuncexpr)                \
    V(DefineFunc)                    \
    V(SetPatchModule)                \
    V(NewobjRange)                   \
    V(Refeq)                         \
    V(Typeof)                        \
    V(Ldnewobjrange)                 \
    V(IsIn)                          \
    V(Instanceof)                    \
    V(InstanceofByHandler)           \
    V(NewObjApply)                   \
    V(CallArg0)                      \
    V(CallArg1)                      \
    V(CallArg2)                      \
    V(CallArg3)                      \
    V(CallThisRange)                 \
    V(CallRange)                     \
    V(CallSpread)                    \
    V(Newlexenv)                     \
    V(NewlexenvwithName)             \
    V(NewSendableEnv)                \
    V(Stlexvar)                      \
    V(Ldlexvar)                      \
    V(Ldlexenv)                      \
    V(GetPropIterator)               \
    V(GetPropIteratorSlowpath)       \
    V(PrimitiveStringCreate)         \
    V(CreateIterResultObj)           \
    V(SuspendGenerator)              \
    V(ResumeGenerator)               \
    V(GetResumeMode)                 \
    V(CreateGeneratorObj)            \
    V(DefineGetterSetterByValue)     \
    V(AsyncFunctionEnter)            \
    V(AsyncFunctionAwaitUncaught)    \
    V(AsyncFunctionResolveOrReject ) \
    V(ThrowUndefined)                \
    V(ThrowConstAssignment)          \
    V(ThrowUndefinedIfHole)          \
    V(Copyrestargs)                  \
    V(Trystobjprop)                  \
    V(GetTemplateObject)             \
    V(CreateStringIterator)          \
    V(NewJSArrayIterator)            \
    V(NewJSTypedArrayIterator)       \
    V(MapIteratorNext)               \
    V(SetIteratorNext)               \
    V(StringIteratorNext)            \
    V(ArrayIteratorNext)             \
    V(GetIterator)                   \
    V(GetAsyncIterator)              \
    V(ThrowIfNotObject)              \
    V(ThrowThrowNotExists)           \
    V(CreateObjectWithExcludedKeys ) \
    V(ThrowPatternNonCoercible)      \
    V(IterNext)                      \
    V(CloseIterator)                 \
    V(StArraySpread)                 \
    V(GetCallSpreadArgs)             \
    V(TryLoadICByName)               \
    V(LoadICByName)                  \
    V(GetPropertyByName)             \
    V(TryLoadICByValue)              \
    V(LoadICByValue)                 \
    V(TryStoreICByName)              \
    V(StoreOwnICByValue)             \
    V(StoreICByName)                 \
    V(StoreOwnICByName)              \
    V(TryStoreICByValue)             \
    V(StoreICByValue)                \
    V(NotifyInlineCache)             \
    V(LoadGlobalICByName)            \
    V(StoreGlobalICByName)           \
    V(StoreICWithHandler)            \
    V(StorePrototype)                \
    V(StoreWithTransition)           \
    V(StoreTransWithProto)           \
    V(StoreWithAOT)                  \
    V(StoreField)                    \
    V(StoreGlobal)                   \
    V(LoadPrototype)                 \
    V(LoadICWithHandler)             \
    V(StoreElement)                  \
    V(CallGetter)                    \
    V(CallSetter)                    \
    V(AddPropertyByName)             \
    V(AddPropertyByIndex)            \
    V(GetPropertyByIndex)            \
    V(GetPropertyByValue)            \
    V(SetPropertyByIndex)            \
    V(SetPropertyByValue)            \
    V(FastTypeOf)                    \
    V(FastSetPropertyByIndex)        \
    V(FastSetPropertyByValue)        \
    V(FastGetPropertyByName)         \
    V(FastGetPropertyByValue)        \
    V(FastGetPropertyByIndex)        \
    V(NewLexicalEnv)                 \
    V(ExecuteNative)                 \
    V(Execute)                       \
    V(AsmExecute)                    \
    V(ExecuteAot)                    \
    V(ToJSTaggedValueWithInt32)      \
    V(ToJSTaggedValueWithUint32)     \
    V(ThrowIfSuperNotCorrectCall)    \
    V(CreateEmptyArray)              \
    V(CreateEmptyObject)             \
    V(CreateObjectWithBuffer)        \
    V(CreateObjectHavingMethod)      \
    V(SetObjectWithProto)            \
    V(getmodulenamespace)            \
    V(StModuleVar)                   \
    V(LdModuleVar)                   \
    V(CreateRegExpWithLiteral)       \
    V(CreateArrayWithBuffer)         \
    V(GetNextPropName)               \
    V(GetNextPropNameSlowpath)       \
    V(CopyDataProperties)            \
    V(GetUnmapedArgs)                \
    V(TryStGlobalByName)             \
    V(LdGlobalVar)                   \
    V(StGlobalVar)                   \
    V(TryUpdateGlobalRecord)         \
    V(LdGlobalRecord)                \
    V(StGlobalRecord)                \
    V(ThrowReferenceError)           \
    V(ThrowTypeError)                \
    V(MismatchError)                 \
    V(ThrowSyntaxError)              \
    V(NewClassFunc)                  \
    V(DefineClass)                   \
    V(SuperCall)                     \
    V(SuperCallSpread)               \
    V(DefineMethod)                  \
    V(LdSuperByValue)                \
    V(StSuperByValue)                \
    V(ThrowDeleteSuperProperty)      \
    V(SetPropertyByName)             \
    V(LdBigInt)                      \
    V(Tonumeric)                     \
    V(CreateAsyncGeneratorObj)       \
    V(AsyncGeneratorResolve)         \
    V(GetSuperConstructor)           \
    V(DynamicImport)                 \
    V(LdPatchVar)                    \
    V(StPatchVar)                    \
    V(AsyncGeneratorReject)          \
    V(LdPrivateProperty)             \
    V(StPrivateProperty)             \
    V(TestIn)                        \
    V(NotifyConcurrentResult)        \
    V(DefineField)                   \
    V(ToPropertyKey)                 \
    V(CreatePrivateProperty)         \
    V(DefinePrivateProperty)         \
    V(CreateSharedClass)             \
    V(LdSendableClass)               \
    V(UpdateAOTHClass)               \
    V(TraceNum)                      \
    V(TraceLazyDeoptNum)             \
    V(TraceLazyDeoptFailNum)         \
    V(SetGeneratorState)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_LIST(V)                   \
    V(Array, Constructor)                     \
    V(Array, From)                            \
    V(Array, Of)                              \
    V(Array, IsArray)                         \
    V(Array, Entries)                         \
    V(Array, Species)                         \
    V(Array, Concat)                          \
    V(Array, CopyWithin)                      \
    V(Array, Every)                           \
    V(Array, Fill)                            \
    V(Array, Filter)                          \
    V(Array, Find)                            \
    V(Array, FindIndex)                       \
    V(Array, FindLast)                        \
    V(Array, FindLastIndex)                   \
    V(Array, ForEach)                         \
    V(Array, IndexOf)                         \
    V(Array, Join)                            \
    V(Array, Keys)                            \
    V(Array, LastIndexOf)                     \
    V(Array, Map)                             \
    V(Array, Pop)                             \
    V(Array, Push)                            \
    V(Array, Reduce)                          \
    V(Array, ReduceRight)                     \
    V(Array, Reverse)                         \
    V(Array, Shift)                           \
    V(Array, Slice)                           \
    V(Array, Some)                            \
    V(Array, Sort)                            \
    V(Array, Splice)                          \
    V(Array, ToLocaleString)                  \
    V(Array, ToString)                        \
    V(Array, Unscopables)                     \
    V(Array, Unshift)                         \
    V(Array, Values)                          \
    V(Array, Includes)                        \
    V(Array, Flat)                            \
    V(Array, FlatMap)                         \
    V(Array, At)                              \
    V(Array, ToReversed)                      \
    V(Array, With)                            \
    V(Array, ToSorted)                        \
    V(Array, ToSpliced)                       \
    V(SharedArray, Constructor)               \
    V(SharedArray, From)                      \
    V(SharedArray, Create)                    \
    V(SharedArray, IsArray)                   \
    V(SharedArray, Entries)                   \
    V(SharedArray, Species)                   \
    V(SharedArray, Concat)                    \
    V(SharedArray, Fill)                      \
    V(SharedArray, Filter)                    \
    V(SharedArray, Find)                      \
    V(SharedArray, FindLast)                  \
    V(SharedArray, FindIndex)                 \
    V(SharedArray, FindLastIndex)             \
    V(SharedArray, ForEach)                   \
    V(SharedArray, IndexOf)                   \
    V(SharedArray, Join)                      \
    V(SharedArray, Keys)                      \
    V(SharedArray, Map)                       \
    V(SharedArray, Pop)                       \
    V(SharedArray, Push)                      \
    V(SharedArray, Reduce)                    \
    V(SharedArray, Shift)                     \
    V(SharedArray, Slice)                     \
    V(SharedArray, Sort)                      \
    V(SharedArray, Splice)                    \
    V(SharedArray, ToString)                  \
    V(SharedArray, ToLocaleString)            \
    V(SharedArray, Unscopables)               \
    V(SharedArray, Unshift)                   \
    V(SharedArray, Values)                    \
    V(SharedArray, Includes)                  \
    V(SharedArray, Flat)                      \
    V(SharedArray, FlatMap)                   \
    V(SharedArray, At)                        \
    V(SharedArray, With)                      \
    V(SharedArray, ShrinkTo)                  \
    V(SharedArray, ExtendTo)                  \
    V(SharedArray, Every)                     \
    V(SharedArray, Some)                      \
    V(SharedArray, LastIndexOf)               \
    V(SharedArray, Of)                        \
    V(SharedArray, CopyWithin)                \
    V(SharedArray, Reverse)                   \
    V(SharedArray, ReduceRight)               \
    V(ArrayBuffer, Constructor)               \
    V(ArrayBuffer, Slice)                     \
    V(ArrayBuffer, Species)                   \
    V(ArrayBuffer, GetByteLength)             \
    V(ArrayBuffer, GetValueFromBuffer)        \
    V(ArrayBuffer, SetValueInBuffer)          \
    V(ArrayBuffer, CloneArrayBuffer)          \
    V(ArrayBuffer, AllocateArrayBuffer)       \
    V(ArrayBuffer, IsView)                    \
    V(SharedArrayBuffer, Constructor)         \
    V(SharedArrayBuffer, Slice)               \
    V(SharedArrayBuffer, AllocateSharedArrayBuffer) \
    V(SharedArrayBuffer, IsSharedArrayBuffer) \
    V(SharedArrayBuffer, IsView)              \
    V(SharedArrayBuffer, Species)             \
    V(SharedArrayBuffer, GetByteLength)       \
    V(SendableArrayBuffer, Constructor)       \
    V(SendableArrayBuffer, Slice)             \
    V(SendableArrayBuffer, Species)           \
    V(SendableArrayBuffer, GetByteLength)     \
    V(SendableArrayBuffer, GetValueFromBuffer)  \
    V(SendableArrayBuffer, SetValueInBuffer)  \
    V(SendableArrayBuffer, CloneArrayBuffer)  \
    V(SendableArrayBuffer, AllocateSendableArrayBuffer) \
    V(SendableArrayBuffer, IsView)            \
    V(AsyncFromSyncIterator, Next)            \
    V(AsyncFromSyncIterator, Throw)           \
    V(AsyncFromSyncIterator, Return)          \
    V(AsyncFunction, Constructor)             \
    V(SharedAsyncFunction, Constructor)       \
    V(Boolean, BooleanPrototypeToString)      \
    V(Boolean, BooleanPrototypeValueOf)       \
    V(Boolean, Constructor)                   \
    V(Boolean, ThisBooleanValue)              \
    V(CjsExports, CjsExportsConstructor)      \
    V(CjsModule, CjsModuleConstructor)        \
    V(CjsModule, Compiler)                    \
    V(CjsModule, Load)                        \
    V(CjsModule, ResolveFilename)             \
    V(CjsModule, Require)                     \
    V(CjsModule, GetExportsForCircularRequire) \
    V(CjsModule, UpdateChildren)              \
    V(CjsRequire, CjsRequireConstructor)      \
    V(CjsRequire, Main)                       \
    V(CjsRequire, Resolve)                    \
    V(Collator, CollatorConstructor)          \
    V(Collator, SupportedLocalesOf)           \
    V(Collator, Compare)                      \
    V(Collator, AnonymousCollator)            \
    V(Collator, ResolvedOptions)              \
    V(DataView, Constructor)                  \
    V(DataView, GetBuffer)                    \
    V(DataView, GetByteLength)                \
    V(DataView, GetOffset)                    \
    V(DataView, GetViewValue)                 \
    V(DataView, SetViewValue)                 \
    V(DataView, GetFloat32)                   \
    V(DataView, GetFloat64)                   \
    V(DataView, GetInt8)                      \
    V(DataView, GetInt16)                     \
    V(DataView, GetInt32)                     \
    V(DataView, GetUint8)                     \
    V(DataView, GetUint16)                    \
    V(DataView, GetUint32)                    \
    V(DataView, GetBigInt64)                  \
    V(DataView, GetBigUint64)                 \
    V(DataView, SetFloat32)                   \
    V(DataView, SetFloat64)                   \
    V(DataView, SetInt8)                      \
    V(DataView, SetInt16)                     \
    V(DataView, SetInt32)                     \
    V(DataView, SetUint8)                     \
    V(DataView, SetUint16)                    \
    V(DataView, SetUint32)                    \
    V(DataView, SetBigInt64)                  \
    V(DataView, SetBigUint64)                 \
    V(DataView, GetTypedValue)                \
    V(DataView, SetTypedValue)                \
    V(Date, Constructor)                      \
    V(Date, Now)                              \
    V(Date, UTC)                              \
    V(Date, Parse)                            \
    V(Date, GetDateField)                     \
    V(Date, GetTime)                          \
    V(Date, SetTime)                          \
    V(Date, ToJSON)                           \
    V(Date, ValueOf)                          \
    V(Date, ToPrimitive)                      \
    V(Date, ToLocaleString)                   \
    V(Date, ToLocaleDateString)               \
    V(Date, ToLocaleTimeString)               \
    V(DateTimeFormat, Constructor)            \
    V(DateTimeFormat, SupportedLocalesOf)     \
    V(DateTimeFormat, Format)                 \
    V(DateTimeFormat, AnonymousDateTimeFormat) \
    V(DateTimeFormat, FormatToParts)          \
    V(DateTimeFormat, ResolvedOptions)        \
    V(DateTimeFormat, FormatRange)            \
    V(DateTimeFormat, FormatRangeToParts)     \
    V(DisplayNames, Constructor)              \
    V(DisplayNames, SupportedLocalesOf)       \
    V(DisplayNames, Of)                       \
    V(DisplayNames, ResolvedOptions)          \
    V(Segmenter, Constructor)                 \
    V(Segmenter, SupportedLocalesOf)          \
    V(Segmenter, ResolvedOptions)             \
    V(Segmenter, Segment)                     \
    V(Segments, Containing)                   \
    V(Segments, GetSegmentIterator)           \
    V(SegmentIterator, Next)                  \
    V(Error, ErrorConstructor)                \
    V(Error, ErrorToString)                   \
    V(Error, RangeErrorConstructor)           \
    V(Error, RangeErrorToString)              \
    V(Error, ReferenceErrorConstructor)       \
    V(Error, ReferenceErrorToString)          \
    V(Error, TypeErrorConstructor)            \
    V(Error, TypeErrorToString)               \
    V(Error, ThrowTypeError)                  \
    V(Error, URIErrorConstructor)             \
    V(Error, URIErrorToString)                \
    V(Error, SyntaxErrorConstructor)          \
    V(Error, SyntaxErrorToString)             \
    V(Error, EvalErrorConstructor)            \
    V(Error, EvalErrorToString)               \
    V(Error, AggregateErrorConstructor)       \
    V(Error, AggregateErrorToString)          \
    V(Error, OOMErrorConstructor)             \
    V(Error, OOMErrorToString)                \
    V(Error, TerminationErrorConstructor)     \
    V(Error, TerminationErrorToString)        \
    V(Function, Constructor)                  \
    V(Function, PrototypeApply)               \
    V(Function, PrototypeBind)                \
    V(Function, PrototypeCall)                \
    V(Function, PrototypeToString)            \
    V(Function, PrototypeHasInstance)         \
    V(Function, PrototypeInvokeSelf)          \
    V(Generator, Constructor)                 \
    V(Generator, PrototypeNext)               \
    V(Generator, PrototypeReturn)             \
    V(Generator, PrototypeThrow)              \
    V(Global, IsFinite)                       \
    V(Global, NotSupportEval)                 \
    V(Global, Encode)                         \
    V(Global, IsNaN)                          \
    V(Global, PrintEntryPoint)                \
    V(Global, NewobjRange)                    \
    V(Global, Decode)                         \
    V(Global, PrintString)                    \
    V(Global, StartRuntimeStat)               \
    V(Global, StopRuntimeStat)                \
    V(Global, PrintOptStat)                   \
    V(Global, PrintMegaICStat)                \
    V(Global, PrintFunctionCallStat)          \
    V(Global, CallJsBoundFunction)            \
    V(Global, CallJsProxy)                    \
    V(Global, DecodeURI)                      \
    V(Global, EncodeURI)                      \
    V(Global, DecodeURIComponent)             \
    V(Global, EncodeURIComponent)             \
    V(Global, Escape)                         \
    V(Global, Unescape)                       \
    V(Global, GetCurrentModuleName)           \
    V(Global, GetCurrentBundleName)           \
    V(Intl, GetCanonicalLocales)              \
    V(Iterator, Constructor)                  \
    V(Iterator, Next)                         \
    V(Iterator, Throw)                        \
    V(Iterator, Return)                       \
    V(Iterator, GetObj)                       \
    V(Locale, Constructor)                    \
    V(Locale, Maximize)                       \
    V(Locale, Minimize)                       \
    V(Locale, ToString)                       \
    V(Locale, GetBaseName)                    \
    V(Locale, GetCalendar)                    \
    V(Locale, GetCaseFirst)                   \
    V(Locale, GetCollation)                   \
    V(Locale, GetHourCycle)                   \
    V(Locale, GetNumeric)                     \
    V(Locale, GetNumberingSystem)             \
    V(Locale, GetLanguage)                    \
    V(Locale, GetScript)                      \
    V(Locale, GetRegion)                      \
    V(AsyncIterator, Constructor)             \
    V(AsyncIterator, Next)                    \
    V(AsyncIterator, Throw)                   \
    V(AsyncIterator, Return)                  \
    V(AsyncIterator, GetObj)                  \
    V(Json, Parse)                            \
    V(Json, Stringify)                        \
    V(Map, Constructor)                       \
    V(Map, Species)                           \
    V(Map, Clear)                             \
    V(Map, Delete)                            \
    V(Map, Entries)                           \
    V(Map, Get)                               \
    V(Map, Has)                               \
    V(Map, Keys)                              \
    V(Map, Set)                               \
    V(Map, GetSize)                           \
    V(Map, Values)                            \
    V(Map, ForEach)                           \
    V(Map, AddEntriesFromIterable)            \
    V(SharedMap, Constructor)               \
    V(SharedMap, Species)                   \
    V(SharedMap, Clear)                     \
    V(SharedMap, Delete)                    \
    V(SharedMap, Entries)                   \
    V(SharedMap, Get)                       \
    V(SharedMap, Has)                       \
    V(SharedMap, Keys)                      \
    V(SharedMap, Set)                       \
    V(SharedMap, GetSize)                   \
    V(SharedMap, Values)                    \
    V(SharedMap, ForEach)                   \
    V(SharedMap, AddEntriesFromIterable)    \
    V(Math, Abs)                              \
    V(Math, Acos)                             \
    V(Math, Acosh)                            \
    V(Math, Asin)                             \
    V(Math, Asinh)                            \
    V(Math, Atan)                             \
    V(Math, Atanh)                            \
    V(Math, Atan2)                            \
    V(Math, Cbrt)                             \
    V(Math, Ceil)                             \
    V(Math, Clz32)                            \
    V(Math, Cos)                              \
    V(Math, Cosh)                             \
    V(Math, Exp)                              \
    V(Math, Expm1)                            \
    V(Math, Floor)                            \
    V(Math, Fround)                           \
    V(Math, Hypot)                            \
    V(Math, Imul)                             \
    V(Math, Log)                              \
    V(Math, Log1p)                            \
    V(Math, Log10)                            \
    V(Math, Log2)                             \
    V(Math, Max)                              \
    V(Math, Min)                              \
    V(Math, Pow)                              \
    V(Math, Random)                           \
    V(Math, Round)                            \
    V(Math, Sign)                             \
    V(Math, Sin)                              \
    V(Math, Sinh)                             \
    V(Math, Sqrt)                             \
    V(Math, Tan)                              \
    V(Math, Tanh)                             \
    V(Math, Trunc)                            \
    V(NumberFormat, Constructor)              \
    V(NumberFormat, SupportedLocalesOf)       \
    V(NumberFormat, Format)                   \
    V(NumberFormat, FormatToParts)            \
    V(NumberFormat, ResolvedOptions)          \
    V(Atomics, AtomicReadModifyWrite)         \
    V(Atomics, AtomicReadModifyWriteCase)     \
    V(Atomics, Add)                           \
    V(Atomics, And)                           \
    V(Atomics, DoWait)                        \
    V(Atomics, HandleWithUint8)               \
    V(Atomics, HandleWithInt8)                \
    V(Atomics, HandleWithUint16)              \
    V(Atomics, HandleWithInt16)               \
    V(Atomics, HandleWithUint32)              \
    V(Atomics, HandleWithInt32)               \
    V(Atomics, HandleWithBigInt64)            \
    V(Atomics, HandleWithBigUint64)           \
    V(Atomics, Or)                            \
    V(Atomics, Xor)                           \
    V(Atomics, Sub)                           \
    V(Atomics, Exchange)                      \
    V(Atomics, CompareExchange)               \
    V(Atomics, Store)                         \
    V(Atomics, Load)                          \
    V(Atomics, IsLockFree)                    \
    V(Atomics, Wait)                          \
    V(Atomics, Notify)                        \
    V(Number, Constructor)                    \
    V(Number, IsFinite)                       \
    V(Number, IsInteger)                      \
    V(Number, IsNaN)                          \
    V(Number, IsSafeInteger)                  \
    V(Number, ParseFloat)                     \
    V(Number, ParseInt)                       \
    V(Number, ToExponential)                  \
    V(Number, ToFixed)                        \
    V(Number, ToLocaleString)                 \
    V(Number, ToPrecision)                    \
    V(Number, ToString)                       \
    V(Number, ValueOf)                        \
    V(Number, ThisNumberValue)                \
    V(BigInt, Constructor)                    \
    V(BigInt, AsUintN)                        \
    V(BigInt, AsIntN)                         \
    V(BigInt, ToLocaleString)                 \
    V(BigInt, ToString)                       \
    V(BigInt, ValueOf)                        \
    V(BigInt, ThisBigIntValue)                \
    V(AsyncGenerator, Constructor)            \
    V(AsyncGenerator, PrototypeNext)          \
    V(AsyncGenerator, PrototypeReturn)        \
    V(AsyncGenerator, PrototypeThrow)         \
    V(Object, Constructor)                    \
    V(Object, Assign)                         \
    V(Object, Create)                         \
    V(Object, CreateDataPropertyOnObjectFunctions) \
    V(Object, DefineProperties)               \
    V(Object, DefineProperty)                 \
    V(Object, Freeze)                         \
    V(Object, FromEntries)                    \
    V(Object, GetOwnPropertyDescriptor)       \
    V(Object, GetOwnPropertyDescriptors)      \
    V(Object, GetOwnPropertyKeys)             \
    V(Object, GetOwnPropertyNames)            \
    V(Object, GetOwnPropertySymbols)          \
    V(Object, GetPrototypeOf)                 \
    V(Object, Is)                             \
    V(Object, IsExtensible)                   \
    V(Object, IsFrozen)                       \
    V(Object, IsSealed)                       \
    V(Object, PropertyIsEnumerable)           \
    V(Object, CreateRealm)                    \
    V(Object, Keys)                           \
    V(Object, Values)                         \
    V(Object, PreventExtensions)              \
    V(Object, Seal)                           \
    V(Object, SetPrototypeOf)                 \
    V(Object, HasOwnProperty)                 \
    V(Object, HasOwn)                         \
    V(Object, IsPrototypeOf)                  \
    V(Object, ToLocaleString)                 \
    V(Object, GetBuiltinObjectToString)       \
    V(Object, ToString)                       \
    V(Object, ValueOf)                        \
    V(Object, ProtoGetter)                    \
    V(Object, ProtoSetter)                    \
    V(PluralRules, Constructor)               \
    V(PluralRules, SupportedLocalesOf)        \
    V(PluralRules, Select)                    \
    V(PluralRules, ResolvedOptions)           \
    V(PromiseHandler, Resolve)                \
    V(PromiseHandler, Reject)                 \
    V(PromiseHandler, Executor)               \
    V(PromiseHandler, ResolveElementFunction) \
    V(PromiseHandler, AllSettledResolveElementFunction) \
    V(PromiseHandler, AllSettledRejectElementFunction) \
    V(PromiseHandler, AnyRejectElementFunction) \
    V(PromiseHandler, AsyncAwaitRejected)     \
    V(PromiseHandler, AsyncAwaitFulfilled)    \
    V(PromiseHandler, valueThunkFunction)     \
    V(PromiseHandler, throwerFunction)        \
    V(PromiseHandler, ThenFinally)            \
    V(PromiseHandler, CatchFinally)           \
    V(PromiseHandler, PromiseResolve)         \
    V(PromiseJob, Reaction)                   \
    V(PromiseJob, ResolveThenableJob)         \
    V(PromiseJob, DynamicImportJob)           \
    V(PromiseJob, CatchException)             \
    V(Promise, Constructor)                   \
    V(Promise, All)                           \
    V(Promise, Race)                          \
    V(Promise, Reject)                        \
    V(Promise, Resolve)                       \
    V(Promise, GetSpecies)                    \
    V(Promise, GetPromiseResolve)             \
    V(Promise, Catch)                         \
    V(Promise, Then)                          \
    V(Promise, PerformPromiseThen)            \
    V(Promise, Finally)                       \
    V(Promise, Any)                           \
    V(Promise, PerformPromiseAny)             \
    V(Promise, AllSettled)                    \
    V(Promise, PerformPromiseRace)            \
    V(Promise, PerformPromiseAll)             \
    V(Promise, PerformPromiseAllSettled)      \
    V(Proxy, Constructor)                     \
    V(Proxy, Revocable)                       \
    V(Proxy, InvalidateProxyFunction)         \
    V(Reflect, Apply)                         \
    V(Reflect, Constructor)                   \
    V(Reflect, DefineProperty)                \
    V(Reflect, DeleteProperty)                \
    V(Reflect, Get)                           \
    V(Reflect, GetOwnPropertyDescriptor)      \
    V(Reflect, GetPrototypeOf)                \
    V(Reflect, Has)                           \
    V(Reflect, OwnKeys)                       \
    V(Reflect, PreventExtensions)             \
    V(Reflect, Set)                           \
    V(Reflect, SetPrototypeOf)                \
    V(RegExp, Constructor)                    \
    V(RegExp, Exec)                           \
    V(RegExp, Test)                           \
    V(RegExp, ToString)                       \
    V(RegExp, GetFlags)                       \
    V(RegExp, GetSpecies)                     \
    V(RegExp, Match)                          \
    V(RegExp, MatchAll)                       \
    V(RegExp, Replace)                        \
    V(RegExp, Search)                         \
    V(RegExp, Split)                          \
    V(RegExp, Create)                         \
    V(RegExp, GetGlobal)                      \
    V(RegExp, GetHasIndices)                  \
    V(RegExp, GetIgnoreCase)                  \
    V(RegExp, GetMultiline)                   \
    V(RegExp, GetDotAll)                      \
    V(RegExp, GetSource)                      \
    V(RegExp, GetSticky)                      \
    V(RegExp, GetUnicode)                     \
    V(RegExp, RegExpReplaceFast)              \
    V(RegExp, Matcher)                        \
    V(RegExp, GetFlagsInternal)               \
    V(RegExp, RegExpBuiltinExec)              \
    V(RegExp, RegExpExec)                     \
    V(RegExp, UpdateExpressionFlags)          \
    V(RegExp, RegExpAlloc)                    \
    V(RegExp, GetDollarString)                \
    V(RegExp, RegExpInitialize)               \
    V(RegExp, EscapeRegExpPattern)            \
    V(RegExp, FlagsBitsToString)              \
    V(RelativeTimeFormat, Constructor)        \
    V(RelativeTimeFormat, SupportedLocalesOf) \
    V(RelativeTimeFormat, Format)             \
    V(RelativeTimeFormat, FormatToParts)      \
    V(RelativeTimeFormat, ResolvedOptions)    \
    V(Set, Constructor)                       \
    V(Set, Species)                           \
    V(Set, Add)                               \
    V(Set, Clear)                             \
    V(Set, Delete)                            \
    V(Set, Entries)                           \
    V(Set, Has)                               \
    V(Set, GetSize)                           \
    V(Set, Values)                            \
    V(Set, ForEach)                           \
    V(SharedSet, Constructor)                 \
    V(SharedSet, Species)                     \
    V(SharedSet, Add)                         \
    V(SharedSet, Clear)                       \
    V(SharedSet, Delete)                      \
    V(SharedSet, Entries)                     \
    V(SharedSet, Has)                         \
    V(SharedSet, GetSize)                     \
    V(SharedSet, Values)                      \
    V(SharedSet, ForEach)                     \
    V(StringIterator, Next)                   \
    V(String, Constructor)                    \
    V(String, FromCharCode)                   \
    V(String, FromCodePoint)                  \
    V(String, Raw)                            \
    V(String, GetSubstitution)                \
    V(String, At)                             \
    V(String, CharAt)                         \
    V(String, CharCodeAt)                     \
    V(String, IsWellFormed)                   \
    V(String, ToWellFormed)                   \
    V(String, CodePointAt)                    \
    V(String, Concat)                         \
    V(String, EndsWith)                       \
    V(String, Includes)                       \
    V(String, IndexOf)                        \
    V(String, LastIndexOf)                    \
    V(String, LocaleCompare)                  \
    V(String, Match)                          \
    V(String, MatchAll)                       \
    V(String, Normalize)                      \
    V(String, PadStart)                       \
    V(String, PadEnd)                         \
    V(String, Repeat)                         \
    V(String, Replace)                        \
    V(String, ReplaceAll)                     \
    V(String, Search)                         \
    V(String, Slice)                          \
    V(String, Split)                          \
    V(String, StartsWith)                     \
    V(String, Substring)                      \
    V(String, ToLocaleLowerCase)              \
    V(String, ToLocaleUpperCase)              \
    V(String, ToLowerCase)                    \
    V(String, ToString)                       \
    V(String, ToUpperCase)                    \
    V(String, Trim)                           \
    V(String, TrimStart)                      \
    V(String, TrimEnd)                        \
    V(String, TrimLeft)                       \
    V(String, TrimRight)                      \
    V(String, GetStringIterator)              \
    V(String, SubStr)                         \
    V(String, GetLength)                      \
    V(String, ThisStringValue)                \
    V(String, Pad)                            \
    V(Symbol, Constructor)                    \
    V(Symbol, ToString)                       \
    V(Symbol, ValueOf)                        \
    V(Symbol, For)                            \
    V(Symbol, KeyFor)                         \
    V(Symbol, DescriptionGetter)              \
    V(Symbol, ThisSymbolValue)                \
    V(Symbol, ToPrimitive)                    \
    V(Symbol, SymbolDescriptiveString)        \
    V(TypedArray, BaseConstructor)            \
    V(TypedArray, Int8ArrayConstructor)       \
    V(TypedArray, Uint8ArrayConstructor)      \
    V(TypedArray, Uint8ClampedArrayConstructor) \
    V(TypedArray, Int16ArrayConstructor)      \
    V(TypedArray, Uint16ArrayConstructor)     \
    V(TypedArray, Int32ArrayConstructor)      \
    V(TypedArray, Uint32ArrayConstructor)     \
    V(TypedArray, Float32ArrayConstructor)    \
    V(TypedArray, Float64ArrayConstructor)    \
    V(TypedArray, BigInt64ArrayConstructor)   \
    V(TypedArray, BigUint64ArrayConstructor)  \
    V(TypedArray, Fill)                       \
    V(TypedArray, Find)                       \
    V(TypedArray, FindIndex)                  \
    V(TypedArray, FindLast)                   \
    V(TypedArray, FindLastIndex)              \
    V(TypedArray, IndexOf)                    \
    V(TypedArray, LastIndexOf)                \
    V(TypedArray, Reduce)                     \
    V(TypedArray, ReduceRight)                \
    V(TypedArray, Reverse)                    \
    V(TypedArray, Some)                       \
    V(TypedArray, ToLocaleString)             \
    V(TypedArray, ToString)                   \
    V(TypedArray, Includes)                   \
    V(TypedArray, From)                       \
    V(TypedArray, Of)                         \
    V(TypedArray, Species)                    \
    V(TypedArray, GetBuffer)                  \
    V(TypedArray, GetByteLength)              \
    V(TypedArray, GetByteOffset)              \
    V(TypedArray, CopyWithin)                 \
    V(TypedArray, Entries)                    \
    V(TypedArray, Every)                      \
    V(TypedArray, Filter)                     \
    V(TypedArray, ForEach)                    \
    V(TypedArray, Join)                       \
    V(TypedArray, Keys)                       \
    V(TypedArray, GetLength)                  \
    V(TypedArray, Map)                        \
    V(TypedArray, Set)                        \
    V(TypedArray, Slice)                      \
    V(TypedArray, Sort)                       \
    V(TypedArray, ToSorted)                   \
    V(TypedArray, Subarray)                   \
    V(TypedArray, Values)                     \
    V(TypedArray, With)                       \
    V(TypedArray, ToStringTag)                \
    V(TypedArray, At)                         \
    V(TypedArray, ToReversed)                 \
    V(SharedTypedArray, BaseConstructor)              \
    V(SharedTypedArray, Int8ArrayConstructor)         \
    V(SharedTypedArray, Uint8ArrayConstructor)        \
    V(SharedTypedArray, Uint8ClampedArrayConstructor) \
    V(SharedTypedArray, Int16ArrayConstructor)        \
    V(SharedTypedArray, Uint16ArrayConstructor)       \
    V(SharedTypedArray, Int32ArrayConstructor)        \
    V(SharedTypedArray, Uint32ArrayConstructor)       \
    V(SharedTypedArray, Float32ArrayConstructor)      \
    V(SharedTypedArray, Float64ArrayConstructor)      \
    V(SharedTypedArray, BigInt64ArrayConstructor)     \
    V(SharedTypedArray, BigUint64ArrayConstructor)    \
    V(SharedTypedArray, Fill)                 \
    V(SharedTypedArray, Find)                 \
    V(SharedTypedArray, FindIndex)            \
    V(SharedTypedArray, IndexOf)              \
    V(SharedTypedArray, LastIndexOf)          \
    V(SharedTypedArray, Reduce)               \
    V(SharedTypedArray, ReduceRight)          \
    V(SharedTypedArray, Reverse)              \
    V(SharedTypedArray, Some)                 \
    V(SharedTypedArray, ToLocaleString)       \
    V(SharedTypedArray, ToString)             \
    V(SharedTypedArray, Includes)             \
    V(SharedTypedArray, From)                 \
    V(SharedTypedArray, Of)                   \
    V(SharedTypedArray, Species)              \
    V(SharedTypedArray, GetBuffer)            \
    V(SharedTypedArray, GetByteLength)        \
    V(SharedTypedArray, GetByteOffset)        \
    V(SharedTypedArray, GetLength)            \
    V(SharedTypedArray, CopyWithin)           \
    V(SharedTypedArray, Entries)              \
    V(SharedTypedArray, Every)                \
    V(SharedTypedArray, Filter)               \
    V(SharedTypedArray, ForEach)              \
    V(SharedTypedArray, Join)                 \
    V(SharedTypedArray, Keys)                 \
    V(SharedTypedArray, Map)                  \
    V(SharedTypedArray, Set)                  \
    V(SharedTypedArray, Slice)                \
    V(SharedTypedArray, Sort)                 \
    V(SharedTypedArray, Subarray)             \
    V(SharedTypedArray, Values)               \
    V(SharedTypedArray, ToStringTag)          \
    V(SharedTypedArray, At)                   \
    V(WeakMap, Constructor)                   \
    V(WeakMap, Delete)                        \
    V(WeakMap, Get)                           \
    V(WeakMap, Has)                           \
    V(WeakMap, Set)                           \
    V(WeakSet, Constructor)                   \
    V(WeakSet, Delete)                        \
    V(WeakSet, Add)                           \
    V(WeakSet, Has)                           \
    V(WeakRef, Constructor)                   \
    V(WeakRef, Deref)                         \
    V(FinalizationRegistry, Constructor)      \
    V(FinalizationRegistry, Register)         \
    V(FinalizationRegistry, Unregister)       \
    V(ArrayList, Constructor)                 \
    V(ArrayList, Add)                         \
    V(ArrayList, Insert)                      \
    V(ArrayList, Clear)                       \
    V(ArrayList, Clone)                       \
    V(ArrayList, Has)                         \
    V(ArrayList, GetCapacity)                 \
    V(ArrayList, IncreaseCapacityTo)          \
    V(ArrayList, TrimToCurrentLength)         \
    V(ArrayList, GetIndexOf)                  \
    V(ArrayList, IsEmpty)                     \
    V(ArrayList, GetLastIndexOf)              \
    V(ArrayList, RemoveByIndex)               \
    V(ArrayList, Remove)                      \
    V(ArrayList, RemoveByRange)               \
    V(ArrayList, ReplaceAllElements)          \
    V(ArrayList, Sort)                        \
    V(ArrayList, SubArrayList)                \
    V(ArrayList, ConvertToArray)              \
    V(ArrayList, ForEach)                     \
    V(ArrayList, GetIteratorObj)              \
    V(ArrayList, Get)                         \
    V(ArrayList, Set)                         \
    V(ArrayList, GetSize)                     \
    V(BitVector, Constructor)                 \
    V(BitVector, Push)                        \
    V(BitVector, Pop)                         \
    V(BitVector, Has)                         \
    V(BitVector, SetBitsByRange)              \
    V(BitVector, GetBitsByRange)              \
    V(BitVector, Resize)                      \
    V(BitVector, SetAllBits)                  \
    V(BitVector, GetBitCountByRange)          \
    V(BitVector, GetIndexOf)                  \
    V(BitVector, GetLastIndexOf)              \
    V(BitVector, FlipBitByIndex)              \
    V(BitVector, FlipBitsByRange)             \
    V(BitVector, GetSize)                     \
    V(BitVector, GetIteratorObj)              \
    V(ListFormat, Constructor)                \
    V(ListFormat, SupportedLocalesOf)         \
    V(ListFormat, Format)                     \
    V(ListFormat, FormatToParts)              \
    V(ListFormat, ResolvedOptions)            \
    V(LightWeightMap, Constructor)            \
    V(LightWeightMap, HasAll)                 \
    V(LightWeightMap, HasKey)                 \
    V(LightWeightMap, HasValue)               \
    V(LightWeightMap, IncreaseCapacityTo)     \
    V(LightWeightMap, Entries)                \
    V(LightWeightMap, Get)                    \
    V(LightWeightMap, GetIndexOfKey)          \
    V(LightWeightMap, GetIndexOfValue)        \
    V(LightWeightMap, IsEmpty)                \
    V(LightWeightMap, GetKeyAt)               \
    V(LightWeightMap, Keys)                   \
    V(LightWeightMap, SetAll)                 \
    V(LightWeightMap, Set)                    \
    V(LightWeightMap, Remove)                 \
    V(LightWeightMap, RemoveAt)               \
    V(LightWeightMap, Clear)                  \
    V(LightWeightMap, SetValueAt)             \
    V(LightWeightMap, ForEach)                \
    V(LightWeightMap, ToString)               \
    V(LightWeightMap, GetValueAt)             \
    V(LightWeightMap, Length)                 \
    V(LightWeightSet, Constructor)            \
    V(LightWeightSet, Add)                    \
    V(LightWeightSet, AddAll)                 \
    V(LightWeightSet, IsEmpty)                \
    V(LightWeightSet, GetValueAt)             \
    V(LightWeightSet, HasAll)                 \
    V(LightWeightSet, Has)                    \
    V(LightWeightSet, HasHash)                \
    V(LightWeightSet, Equal)                  \
    V(LightWeightSet, IncreaseCapacityTo)     \
    V(LightWeightSet, GetIteratorObj)         \
    V(LightWeightSet, Values)                 \
    V(LightWeightSet, Entries)                \
    V(LightWeightSet, ForEach)                \
    V(LightWeightSet, GetIndexOf)             \
    V(LightWeightSet, Remove)                 \
    V(LightWeightSet, RemoveAt)               \
    V(LightWeightSet, Clear)                  \
    V(LightWeightSet, ToString)               \
    V(LightWeightSet, ToArray)                \
    V(LightWeightSet, GetSize)                \
    V(PlainArray, Constructor)                \
    V(PlainArray, Add)                        \
    V(PlainArray, Clear)                      \
    V(PlainArray, Clone)                      \
    V(PlainArray, Has)                        \
    V(PlainArray, Get)                        \
    V(PlainArray, GetIteratorObj)             \
    V(PlainArray, ForEach)                    \
    V(PlainArray, ToString)                   \
    V(PlainArray, GetIndexOfKey)              \
    V(PlainArray, GetIndexOfValue)            \
    V(PlainArray, IsEmpty)                    \
    V(PlainArray, GetKeyAt)                   \
    V(PlainArray, Remove)                     \
    V(PlainArray, RemoveAt)                   \
    V(PlainArray, RemoveRangeFrom)            \
    V(PlainArray, SetValueAt)                 \
    V(PlainArray, GetValueAt)                 \
    V(PlainArray, GetSize)                    \
    V(HashMap, Constructor)                   \
    V(HashMap, HasKey)                        \
    V(HashMap, HasValue)                      \
    V(HashMap, Replace)                       \
    V(HashMap, Keys)                          \
    V(HashMap, Values)                        \
    V(HashMap, GetIteratorObj)                \
    V(HashMap, Entries)                       \
    V(HashMap, ForEach)                       \
    V(HashMap, Set)                           \
    V(HashMap, SetAll)                        \
    V(HashMap, Remove)                        \
    V(HashMap, Get)                           \
    V(HashMap, Clear)                         \
    V(HashMap, GetLength)                     \
    V(HashMap, IsEmpty)                       \
    V(HashSet, Constructor)                   \
    V(HashSet, IsEmpty)                       \
    V(HashSet, Has)                           \
    V(HashSet, Add)                           \
    V(HashSet, Remove)                        \
    V(HashSet, Clear)                         \
    V(HashSet, GetLength)                     \
    V(HashSet, GetIteratorObj)                \
    V(HashSet, Values)                        \
    V(HashSet, Entries)                       \
    V(HashSet, ForEach)                       \
    V(TreeMap, Constructor)                   \
    V(TreeMap, HasKey)                        \
    V(TreeMap, HasValue)                      \
    V(TreeMap, GetFirstKey)                   \
    V(TreeMap, GetLastKey)                    \
    V(TreeMap, Set)                           \
    V(TreeMap, Get)                           \
    V(TreeMap, SetAll)                        \
    V(TreeMap, Remove)                        \
    V(TreeMap, Clear)                         \
    V(TreeMap, GetLowerKey)                   \
    V(TreeMap, GetHigherKey)                  \
    V(TreeMap, Replace)                       \
    V(TreeMap, IsEmpty)                       \
    V(TreeMap, GetLength)                     \
    V(TreeMap, Keys)                          \
    V(TreeMap, Values)                        \
    V(TreeMap, Entries)                       \
    V(TreeMap, ForEach)                       \
    V(TreeSet, Constructor)                   \
    V(TreeSet, Add)                           \
    V(TreeSet, Remove)                        \
    V(TreeSet, Clear)                         \
    V(TreeSet, Has)                           \
    V(TreeSet, GetFirstValue)                 \
    V(TreeSet, GetLastValue)                  \
    V(TreeSet, GetLowerValue)                 \
    V(TreeSet, GetHigherValue)                \
    V(TreeSet, PopFirst)                      \
    V(TreeSet, PopLast)                       \
    V(TreeSet, IsEmpty)                       \
    V(TreeSet, GetLength)                     \
    V(TreeSet, Values)                        \
    V(TreeSet, ForEach)                       \
    V(TreeSet, Entries)                       \
    V(Deque, Constructor)                     \
    V(Deque, InsertFront)                     \
    V(Deque, InsertEnd)                       \
    V(Deque, GetFirst)                        \
    V(Deque, GetLast)                         \
    V(Deque, GetFront)                        \
    V(Deque, GetTail)                         \
    V(Deque, Has)                             \
    V(Deque, PopFirst)                        \
    V(Deque, PopLast)                         \
    V(Deque, ForEach)                         \
    V(Deque, GetIteratorObj)                  \
    V(Deque, GetSize)                         \
    V(Stack, Constructor)                     \
    V(Stack, Iterator)                        \
    V(Stack, IsEmpty)                         \
    V(Stack, Push)                            \
    V(Stack, Peek)                            \
    V(Stack, Pop)                             \
    V(Stack, Locate)                          \
    V(Stack, ForEach)                         \
    V(Stack, GetLength)                       \
    V(Vector, Constructor)                    \
    V(Vector, Add)                            \
    V(Vector, Insert)                         \
    V(Vector, SetLength)                      \
    V(Vector, GetCapacity)                    \
    V(Vector, IncreaseCapacityTo)             \
    V(Vector, Get)                            \
    V(Vector, GetIndexOf)                     \
    V(Vector, GetIndexFrom)                   \
    V(Vector, IsEmpty)                        \
    V(Vector, GetLastElement)                 \
    V(Vector, GetLastIndexOf)                 \
    V(Vector, GetLastIndexFrom)               \
    V(Vector, Remove)                         \
    V(Vector, RemoveByIndex)                  \
    V(Vector, RemoveByRange)                  \
    V(Vector, Set)                            \
    V(Vector, SubVector)                      \
    V(Vector, ToString)                       \
    V(Vector, GetSize)                        \
    V(Vector, ForEach)                        \
    V(Vector, ReplaceAllElements)             \
    V(Vector, Has)                            \
    V(Vector, Sort)                           \
    V(Vector, Clear)                          \
    V(Vector, Clone)                          \
    V(Vector, CopyToArray)                    \
    V(Vector, ConvertToArray)                 \
    V(Vector, GetFirstElement)                \
    V(Vector, TrimToCurrentLength)            \
    V(Vector, GetIteratorObj)                 \
    V(Queue, Constructor)                     \
    V(Queue, Add)                             \
    V(Queue, GetFirst)                        \
    V(Queue, Pop)                             \
    V(Queue, ForEach)                         \
    V(Queue, GetIteratorObj)                  \
    V(Queue, GetSize)                         \
    V(List, Constructor)                      \
    V(List, Add)                              \
    V(List, GetFirst)                         \
    V(List, GetLast)                          \
    V(List, Insert)                           \
    V(List, Clear)                            \
    V(List, RemoveByIndex)                    \
    V(List, Remove)                           \
    V(List, Has)                              \
    V(List, IsEmpty)                          \
    V(List, Get)                              \
    V(List, GetIndexOf)                       \
    V(List, GetLastIndexOf)                   \
    V(List, Set)                              \
    V(List, ForEach)                          \
    V(List, ReplaceAllElements)               \
    V(List, GetIteratorObj)                   \
    V(List, Equal)                            \
    V(List, Sort)                             \
    V(List, ConvertToArray)                   \
    V(List, GetSubList)                       \
    V(List, Length)                           \
    V(LinkedList, Constructor)                \
    V(LinkedList, Length)                     \
    V(LinkedList, Add)                        \
    V(LinkedList, GetFirst)                   \
    V(LinkedList, GetLast)                    \
    V(LinkedList, Insert)                     \
    V(LinkedList, AddFirst)                   \
    V(LinkedList, Clear)                      \
    V(LinkedList, Clone)                      \
    V(LinkedList, Has)                        \
    V(LinkedList, Get)                        \
    V(LinkedList, GetIndexOf)                 \
    V(LinkedList, GetLastIndexOf)             \
    V(LinkedList, RemoveByIndex)              \
    V(LinkedList, Remove)                     \
    V(LinkedList, RemoveFirst)                \
    V(LinkedList, RemoveLast)                 \
    V(LinkedList, RemoveFirstFound)           \
    V(LinkedList, RemoveLastFound)            \
    V(LinkedList, Set)                        \
    V(LinkedList, ConvertToArray)             \
    V(LinkedList, ForEach)                    \
    V(LinkedList, GetIteratorObj)             \
    V(Buffer, Constructor)                    \
    V(Buffer, WriteUIntBE)                    \
    V(Buffer, WriteUIntLE)                    \
    V(Buffer, WriteUInt8BE)                   \
    V(Buffer, WriteUInt8LE)                   \
    V(Buffer, WriteUInt16BE)                  \
    V(Buffer, WriteUInt16LE)                  \
    V(Buffer, WriteUInt32BE)                  \
    V(Buffer, WriteUInt32LE)                  \
    V(Buffer, WriteBigUInt64BE)               \
    V(Buffer, WriteBigUInt64LE)               \
    V(Buffer, WriteIntBE)                     \
    V(Buffer, WriteIntLE)                     \
    V(Buffer, WriteInt8BE)                    \
    V(Buffer, WriteInt8LE)                    \
    V(Buffer, WriteInt16BE)                   \
    V(Buffer, WriteInt16LE)                   \
    V(Buffer, WriteInt32BE)                   \
    V(Buffer, WriteInt32LE)                   \
    V(Buffer, WriteBigInt64BE)                \
    V(Buffer, WriteBigInt64LE)                \
    V(Buffer, WriteFloat32BE)                 \
    V(Buffer, WriteFloat32LE)                 \
    V(Buffer, WriteFloat64BE)                 \
    V(Buffer, WriteFloat64LE)                 \
    V(Buffer, ReadUIntBE)                     \
    V(Buffer, ReadUIntLE)                     \
    V(Buffer, ReadUInt8BE)                    \
    V(Buffer, ReadUInt8LE)                    \
    V(Buffer, ReadUInt16BE)                   \
    V(Buffer, ReadUInt16LE)                   \
    V(Buffer, ReadUInt32BE)                   \
    V(Buffer, ReadUInt32LE)                   \
    V(Buffer, ReadBigUInt64BE)                \
    V(Buffer, ReadBigUInt64LE)                \
    V(Buffer, ReadIntBE)                      \
    V(Buffer, ReadIntLE)                      \
    V(Buffer, ReadInt8BE)                     \
    V(Buffer, ReadInt8LE)                     \
    V(Buffer, ReadInt16BE)                    \
    V(Buffer, ReadInt16LE)                    \
    V(Buffer, ReadInt32BE)                    \
    V(Buffer, ReadInt32LE)                    \
    V(Buffer, ReadBigInt64BE)                 \
    V(Buffer, ReadBigInt64LE)                 \
    V(Buffer, ReadFloat32BE)                  \
    V(Buffer, ReadFloat32LE)                  \
    V(Buffer, ReadFloat64BE)                  \
    V(Buffer, ReadFloat64LE)                  \
    V(Buffer, Write)                          \
    V(Buffer, Compare)                        \
    V(Buffer, Equals)                         \
    V(Buffer, IndexOf)                        \
    V(Buffer, LastIndexOf)                    \
    V(Buffer, Includes)                       \
    V(Buffer, Copy)                           \
    V(Buffer, Fill)                           \
    V(Buffer, GetSize)                        \
    V(Buffer, From)                           \
    V(Buffer, Entries)                        \
    V(Buffer, Keys)                           \
    V(Buffer, Values)                         \
    V(Buffer, Alloc)                          \
    V(Buffer, GetArrayBuffer)                 \
    V(Buffer, GetByteOffset)                  \
    V(Buffer, ToString)

#define ABSTRACT_OPERATION_LIST(V) \
    V(JSTaggedValue, ToString)     \

#define MEM_ALLOCATE_AND_GC_LIST(V)  \
    V(FullGC_RunPhases)              \
    V(PartialGC_RunPhases)           \
    V(ConcurrentMarking)             \
    V(ConcurrentMarkingInitialize)   \
    V(IncrementalMarking)            \
    V(IncrementalMarkingInitialize)  \
    V(WaitConcurrentMarkingFinished) \
    V(ReMarking)                     \
    V(ConcurrentSweepingInitialize)  \
    V(ConcurrentSweepingWait)        \
    V(ParallelEvacuationInitialize)  \
    V(ParallelEvacuation)            \
    V(ParallelUpdateReference)       \
    V(WaitUpdateFinished)            \
    V(UpdateRoot)                    \
    V(UpdateWeakReferenceOpt)        \
    V(ParallelEvacuator)             \
    V(ParallelEvacuatorInitialize)   \
    V(ParallelEvacuatorFinalize)     \
    V(HugeSpaceExpand)               \
    V(NonMovableSpaceExpand)         \
    V(HeapPrepare)                   \

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_ID(name) INTERPRETER_ID_##name,
#define RUNTIME_CALLER_ID(name) RUNTIME_ID_##name,
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_ID(class, name) BUILTINS_ID_##class##_##name,
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ABSTRACT_OPERATION_ID(class, name) ABSTRACT_ID_##class##_##name,

#define MEM_ALLOCATE_AND_GC_ID(name) MEM_ID##name,

enum EcmaRuntimeCallerId {
    INTERPRETER_CALLER_LIST(INTERPRETER_CALLER_ID) BUILTINS_API_LIST(BUILTINS_API_ID)
    ABSTRACT_OPERATION_LIST(ABSTRACT_OPERATION_ID)
    MEM_ALLOCATE_AND_GC_LIST(MEM_ALLOCATE_AND_GC_ID)
#define DEF_RUNTIME_ID(name) RUNTIME_ID_##name,
    RUNTIME_STUB_WITH_GC_LIST(DEF_RUNTIME_ID)
    RUNTIME_STUB_WITH_DFX(DEF_RUNTIME_ID)
#undef DEF_RUNTIME_ID
    RUNTIME_CALLER_NUMBER,
};

#if ECMASCRIPT_ENABLE_INTERPRETER_RUNTIME_STAT
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_TRACE(thread, name)                                                                    \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                                       \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat();             \
    RuntimeTimerScope interpret_##name##_scope_(INTERPRETER_CALLER_ID(name) _run_stat_)
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RUNTIME_TRACE(thread, name)                                                                        \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                                       \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat();             \
    RuntimeTimerScope interpret_##name##_scope_(RUNTIME_CALLER_ID(name) _run_stat_);                       \
    [[maybe_unused]] RuntimeStateScope _runtime_state_##name##_scope_(_js_thread_)
#else
#define RUNTIME_TRACE(thread, name)                                                                        \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                                       \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat();             \
    RuntimeTimerScope interpret_##name##_scope_(RUNTIME_CALLER_ID(name) _run_stat_);
#endif
#else
#define INTERPRETER_TRACE(thread, name) static_cast<void>(0) // NOLINT(cppcoreguidelines-macro-usage)
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RUNTIME_TRACE(thread, name)                                                            \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                           \
    [[maybe_unused]] RuntimeStateScope _runtime_state_##name##_scope_(_js_thread_)
#else
#define RUNTIME_TRACE(thread, name) static_cast<void>(0) // NOLINT(cppcoreguidelines-macro-usage)
#endif // defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#endif // ECMASCRIPT_ENABLE_INTERPRETER_RUNTIME_STAT

#if ECMASCRIPT_ENABLE_BUILTINS_RUNTIME_STAT
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_TRACE(thread, class, name)                                                            \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                                       \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat();             \
    RuntimeTimerScope builtins_##class##name##_scope_(BUILTINS_API_ID(class, name) _run_stat_)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ABSTRACT_OPERATION_TRACE(thread, class, name)                                                      \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                                       \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat();             \
    RuntimeTimerScope abstract_##class##name##_scope_(ABSTRACT_OPERATION_ID(class, name) _run_stat_)
#else
#define BUILTINS_API_TRACE(thread, class, name) static_cast<void>(0) // NOLINT(cppcoreguidelines-macro-usage)
#define ABSTRACT_OPERATION_TRACE(class, name) static_cast<void>(0) // NOLINT(cppcoreguidelines-macro-usage)
#endif // ECMASCRIPT_ENABLE_BUILTINS_RUNTIME_STAT

#if ECMASCRIPT_ENABLE_ALLOCATE_AND_GC_RUNTIME_STAT
#define MEM_ALLOCATE_AND_GC_TRACE(vm, name)                                                     \
    CHECK_JS_THREAD(vm);                                                                        \
    EcmaRuntimeStat *_run_stat_ = vm->GetRuntimeStat();                                         \
    RuntimeTimerScope mem_##name##_scope_(MEM_ALLOCATE_AND_GC_ID(name) _run_stat_)
#else
#define MEM_ALLOCATE_AND_GC_TRACE(vm, name) static_cast<void>(0) // NOLINT(cppcoreguidelines-macro-usage)
#endif // ECMASCRIPT_ENABLE_ALLOCATE_AND_GC_RUNTIME_STAT
}  // namespace panda::ecmascript
#endif
