/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_RUNTIME_STUBS_H
#define ECMASCRIPT_RUNTIME_STUBS_H

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/frames.h"
#include "ecmascript/stubs/test_runtime_stubs.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/method.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript {
using kungfu::CallSignature;
class EcmaVM;
class GlobalEnv;
class JSThread;
class JSFunction;
class ObjectFactory;
class JSBoundFunction;
class JSProxy;

class GeneratorContext;
struct EcmaRuntimeCallInfo;

using JSFunctionEntryType = JSTaggedValue (*)(uintptr_t glue, uint32_t argc, const JSTaggedType argV[],
                                              uintptr_t prevFp, bool needPushUndefined);
using FastCallAotEntryType = JSTaggedValue (*)(uintptr_t glue, uint32_t argc, const JSTaggedType argV[],
                                              uintptr_t prevFp);

#define RUNTIME_ASM_STUB_LIST(V)             \
    JS_CALL_TRAMPOLINE_LIST(V)               \
    FAST_CALL_TRAMPOLINE_LIST(V)             \
    ASM_INTERPRETER_TRAMPOLINE_LIST(V)

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
    V(PushCallNewAndDispatchNative)          \
    V(PushCallRangeAndDispatchNative)        \
    V(PushCallThisRangeAndDispatch)          \
    V(ResumeRspAndDispatch)                  \
    V(ResumeRspAndReturn)                    \
    V(ResumeCaughtFrameAndDispatch)          \
    V(ResumeUncaughtFrameAndReturn)          \
    V(ResumeRspAndRollback)                  \
    V(CallSetter)                            \
    V(CallGetter)                            \
    V(CallContainersArgs3)                   \
    V(CallReturnWithArgv)

#define JS_CALL_TRAMPOLINE_LIST(V)           \
    V(CallRuntime)                           \
    V(CallRuntimeWithArgv)                   \
    V(JSFunctionEntry)                       \
    V(JSCall)                                \
    V(JSCallWithArgV)                        \
    V(JSCallWithArgVAndPushUndefined)        \
    V(JSProxyCallInternalWithArgV)           \
    V(OptimizedCallAndPushUndefined)         \
    V(DeoptHandlerAsm)                       \
    V(JSCallNew)                             \
    V(CallOptimized)

#define FAST_CALL_TRAMPOLINE_LIST(V)         \
    V(OptimizedFastCallEntry)                \
    V(OptimizedFastCallAndPushUndefined)     \
    V(JSFastCallWithArgV)                    \
    V(JSFastCallWithArgVAndPushUndefined)


#define RUNTIME_STUB_WITHOUT_GC_LIST(V)        \
    V(DebugPrint)                              \
    V(DebugPrintCustom)                        \
    V(DebugPrintInstruction)                   \
    V(ProfileCall)                             \
    V(ProfileDefineClass)                      \
    V(ProfileCreateObject)                     \
    V(ProfileOpType)                           \
    V(ProfileObjLayout)                        \
    V(ProfileObjIndex)                         \
    V(Comment)                                 \
    V(FatalPrint)                              \
    V(FatalPrintCustom)                        \
    V(GetActualArgvNoGC)                       \
    V(InsertOldToNewRSet)                      \
    V(MarkingBarrier)                          \
    V(StoreBarrier)                            \
    V(DoubleToInt)                             \
    V(DoubleToLength)                          \
    V(FloatMod)                                \
    V(FloatSqrt)                               \
    V(FloatCos)                                \
    V(FloatSin)                                \
    V(FloatACos)                               \
    V(FloatATan)                               \
    V(FloatFloor)                              \
    V(FindElementWithCache)                    \
    V(CreateArrayFromList)                     \
    V(StringsAreEquals)                        \
    V(BigIntEquals)                            \
    V(TimeClip)                                \
    V(SetDateValues)                           \
    V(StartCallTimer)                          \
    V(EndCallTimer)

#define RUNTIME_STUB_WITH_GC_LIST(V)      \
    V(AddElementInternal)                 \
    V(AllocateInYoung)                    \
    V(CallInternalGetter)                 \
    V(CallInternalSetter)                 \
    V(CallGetPrototype)                   \
    V(ThrowTypeError)                     \
    V(Dump)                               \
    V(GetHash32)                          \
    V(ComputeHashcode)                    \
    V(GetTaggedArrayPtrTest)              \
    V(NewInternalString)                  \
    V(NewTaggedArray)                     \
    V(CopyArray)                          \
    V(NameDictPutIfAbsent)                \
    V(PropertiesSetValue)                 \
    V(TaggedArraySetValue)                \
    V(CheckAndCopyArray)                  \
    V(NewEcmaHClass)                      \
    V(UpdateLayOutAndAddTransition)       \
    V(CopyAndUpdateObjLayout)             \
    V(UpdateHClassForElementsKind)        \
    V(NoticeThroughChainAndRefreshUser)   \
    V(JumpToCInterpreter)                 \
    V(StGlobalRecord)                     \
    V(SetFunctionNameNoPrefix)            \
    V(StOwnByValueWithNameSet)            \
    V(StOwnByName)                        \
    V(StOwnByNameWithNameSet)             \
    V(SuspendGenerator)                   \
    V(UpFrame)                            \
    V(Neg)                                \
    V(Not)                                \
    V(Inc)                                \
    V(Dec)                                \
    V(Shl2)                               \
    V(Shr2)                               \
    V(Ashr2)                              \
    V(Or2)                                \
    V(Xor2)                               \
    V(And2)                               \
    V(Exp)                                \
    V(IsIn)                               \
    V(InstanceOf)                         \
    V(CreateGeneratorObj)                 \
    V(ThrowConstAssignment)               \
    V(GetTemplateObject)                  \
    V(GetNextPropName)                    \
    V(ThrowIfNotObject)                   \
    V(IterNext)                           \
    V(CloseIterator)                      \
    V(SuperCallSpread)                    \
    V(OptSuperCallSpread)                 \
    V(DelObjProp)                         \
    V(NewObjApply)                        \
    V(CreateIterResultObj)                \
    V(AsyncFunctionAwaitUncaught)         \
    V(AsyncFunctionResolveOrReject)       \
    V(ThrowUndefinedIfHole)               \
    V(CopyDataProperties)                 \
    V(StArraySpread)                      \
    V(GetIteratorNext)                    \
    V(SetObjectWithProto)                 \
    V(LoadICByValue)                      \
    V(StoreICByValue)                     \
    V(StOwnByValue)                       \
    V(LdSuperByValue)                     \
    V(StSuperByValue)                     \
    V(LdObjByIndex)                       \
    V(StObjByIndex)                       \
    V(StOwnByIndex)                       \
    V(CreateClassWithBuffer)              \
    V(SetClassConstructorLength)          \
    V(LoadICByName)                       \
    V(StoreICByName)                      \
    V(UpdateHotnessCounter)               \
    V(CheckSafePointAndStackOverflow)     \
    V(UpdateHotnessCounterWithProf)       \
    V(GetModuleNamespaceByIndex)          \
    V(GetModuleNamespaceByIndexOnJSFunc)  \
    V(GetModuleNamespace)                 \
    V(StModuleVarByIndex)                 \
    V(StModuleVarByIndexOnJSFunc)         \
    V(StModuleVar)                        \
    V(LdLocalModuleVarByIndex)            \
    V(LdExternalModuleVarByIndex)         \
    V(LdLocalModuleVarByIndexOnJSFunc)    \
    V(LdExternalModuleVarByIndexOnJSFunc) \
    V(LdModuleVar)                        \
    V(Throw)                              \
    V(GetPropIterator)                    \
    V(AsyncFunctionEnter)                 \
    V(GetIterator)                        \
    V(GetAsyncIterator)                   \
    V(SetGeneratorState)                  \
    V(ThrowThrowNotExists)                \
    V(ThrowPatternNonCoercible)           \
    V(ThrowDeleteSuperProperty)           \
    V(Eq)                                 \
    V(TryLdGlobalICByName)                \
    V(LoadMiss)                           \
    V(StoreMiss)                          \
    V(TryUpdateGlobalRecord)              \
    V(ThrowReferenceError)                \
    V(StGlobalVar)                        \
    V(LdGlobalICVar)                      \
    V(ToNumber)                           \
    V(ToBoolean)                          \
    V(NotEq)                              \
    V(Less)                               \
    V(LessEq)                             \
    V(Greater)                            \
    V(GreaterEq)                          \
    V(Add2)                               \
    V(Sub2)                               \
    V(Mul2)                               \
    V(Div2)                               \
    V(Mod2)                               \
    V(CreateEmptyObject)                  \
    V(CreateEmptyArray)                   \
    V(GetSymbolFunction)                  \
    V(GetUnmapedArgs)                     \
    V(CopyRestArgs)                       \
    V(CreateArrayWithBuffer)              \
    V(CreateObjectWithBuffer)             \
    V(NewThisObject)                      \
    V(NewObjRange)                        \
    V(DefineFunc)                         \
    V(CreateRegExpWithLiteral)            \
    V(ThrowIfSuperNotCorrectCall)         \
    V(CreateObjectHavingMethod)           \
    V(CreateObjectWithExcludedKeys)       \
    V(DefineMethod)                       \
    V(ThrowSetterIsUndefinedException)    \
    V(ThrowNotCallableException)          \
    V(ThrowCallConstructorException)      \
    V(ThrowNonConstructorException)       \
    V(ThrowStackOverflowException)        \
    V(ThrowDerivedMustReturnException)    \
    V(CallSpread)                         \
    V(DefineGetterSetterByValue)          \
    V(SuperCall)                          \
    V(OptSuperCall)                       \
    V(LdBigInt)                           \
    V(ToNumeric)                          \
    V(DynamicImport)                      \
    V(CreateAsyncGeneratorObj)            \
    V(AsyncGeneratorResolve)              \
    V(AsyncGeneratorReject)               \
    V(NewLexicalEnvWithName)              \
    V(OptGetUnmapedArgs)                  \
    V(OptCopyRestArgs)                    \
    V(NotifyBytecodePcChanged)            \
    V(NotifyDebuggerStatement)            \
    V(MethodEntry)                        \
    V(MethodExit)                         \
    V(OptNewLexicalEnvWithName)           \
    V(OptSuspendGenerator)                \
    V(OptAsyncGeneratorResolve)           \
    V(OptCreateObjectWithExcludedKeys)    \
    V(OptNewObjRange)                     \
    V(GetTypeArrayPropertyByIndex)        \
    V(SetTypeArrayPropertyByIndex)        \
    V(FastCopyElementToArray)             \
    V(JSObjectGetMethod)                  \
    V(DebugAOTPrint)                      \
    V(ProfileOptimizedCode)               \
    V(VerifyVTableLoading)                \
    V(VerifyVTableStoring)                \
    V(GetMethodFromCache)                 \
    V(GetArrayLiteralFromCache)           \
    V(GetObjectLiteralFromCache)          \
    V(GetStringFromCache)                 \
    V(OptLdSuperByValue)                  \
    V(OptStSuperByValue)                  \
    V(BigIntEqual)                        \
    V(StringEqual)                        \
    V(LdPatchVar)                         \
    V(StPatchVar)                         \
    V(DeoptHandler)                       \
    V(ContainerRBTreeForEach)             \
    V(SlowFlattenString)                  \
    V(NotifyConcurrentResult)             \
    V(AotInlineTrace)                     \
    V(LocaleCompare)                      \
    V(ArraySort)                          \
    V(FastStringify)

#define RUNTIME_STUB_LIST(V)                     \
    RUNTIME_ASM_STUB_LIST(V)                     \
    RUNTIME_STUB_WITHOUT_GC_LIST(V)              \
    RUNTIME_STUB_WITH_GC_LIST(V)                 \
    TEST_RUNTIME_STUB_GC_LIST(V)

class RuntimeStubs {
public:
    static void Initialize(JSThread *thread);

#define DECLARE_RUNTIME_STUBS(name) \
    static JSTaggedType name(uintptr_t argGlue, uint32_t argc, uintptr_t argv);
    RUNTIME_STUB_WITH_GC_LIST(DECLARE_RUNTIME_STUBS)
    TEST_RUNTIME_STUB_GC_LIST(DECLARE_RUNTIME_STUBS)
#undef DECLARE_RUNTIME_STUBS

    inline static JSTaggedType GetTArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return *(reinterpret_cast<JSTaggedType *>(argv) + (index));
    }

    inline static JSTaggedValue GetArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return JSTaggedValue(*(reinterpret_cast<JSTaggedType *>(argv) + (index)));
    }

    template<typename T>
    inline static JSHandle<T> GetHArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return JSHandle<T>(&(reinterpret_cast<JSTaggedType *>(argv)[index]));
    }

    template<typename T>
    inline static T *GetPtrArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return reinterpret_cast<T*>(*(reinterpret_cast<JSTaggedType *>(argv) + (index)));
    }

    static void DebugPrint(int fmtMessageId, ...);
    static void DebugPrintCustom(uintptr_t fmt, ...);
    static void DebugPrintInstruction([[maybe_unused]] uintptr_t argGlue, const uint8_t *pc);
    static void Comment(uintptr_t argStr);
    static void ProfileCall(uintptr_t argGlue, uintptr_t func, uintptr_t target, int32_t pcOffset, uint32_t incCount);
    static void ProfileDefineClass(uintptr_t argGlue, uintptr_t func, int32_t offset, uintptr_t ctor);
    static void ProfileCreateObject(
        uintptr_t argGlue, JSTaggedType func, int32_t offset, JSTaggedType newObj, int32_t traceId);
    static void ProfileOpType(uintptr_t argGlue, uintptr_t func, int32_t offset, int32_t type);
    static void ProfileObjLayout(uintptr_t argGlue, uintptr_t func, int32_t offset, uintptr_t object, int32_t store);
    static void ProfileObjIndex(uintptr_t argGlue, uintptr_t func, int32_t offset, uintptr_t object);
    static void FatalPrint(int fmtMessageId, ...);
    static void FatalPrintCustom(uintptr_t fmt, ...);
    static void MarkingBarrier([[maybe_unused]] uintptr_t argGlue,
        uintptr_t object, size_t offset, TaggedObject *value);
    static void StoreBarrier([[maybe_unused]] uintptr_t argGlue,
        uintptr_t object, size_t offset, TaggedObject *value);
    static JSTaggedType CreateArrayFromList([[maybe_unused]] uintptr_t argGlue, int32_t argc, JSTaggedValue *argvPtr);
    static JSTaggedType GetActualArgvNoGC(uintptr_t argGlue);
    static void InsertOldToNewRSet([[maybe_unused]] uintptr_t argGlue, uintptr_t object, size_t offset);
    static int32_t DoubleToInt(double x);
    static JSTaggedType DoubleToLength(double x);
    static double FloatMod(double x, double y);
    static JSTaggedType FloatSqrt(double x);
    static JSTaggedType FloatCos(double x);
    static JSTaggedType FloatSin(double x);
    static JSTaggedType FloatACos(double x);
    static JSTaggedType FloatATan(double x);
    static JSTaggedType FloatFloor(double x);
    static int32_t FindElementWithCache(uintptr_t argGlue, JSTaggedType hclass,
                                        JSTaggedType key, int32_t num);
    static bool StringsAreEquals(EcmaString *str1, EcmaString *str2);
    static bool BigIntEquals(JSTaggedType left, JSTaggedType right);
    static double TimeClip(double time);
    static double SetDateValues(double year, double month, double day);
    static void StartCallTimer(uintptr_t argGlue, JSTaggedType func, bool isAot);
    static void EndCallTimer(uintptr_t argGlue, JSTaggedType func);
    static JSTaggedValue RuntimeArraySort(JSThread *thread, JSHandle<JSTaggedValue> thisHandle);

    static JSTaggedValue CallBoundFunction(EcmaRuntimeCallInfo *info);
private:
    static void PrintHeapReginInfo(uintptr_t argGlue);

    static inline JSTaggedValue RuntimeInc(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeDec(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeExp(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent);
    static inline JSTaggedValue RuntimeIsIn(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                            const JSHandle<JSTaggedValue> &obj);
    static inline JSTaggedValue RuntimeInstanceof(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &target);
    static inline JSTaggedValue RuntimeInstanceofByHandler(JSThread *thread, JSHandle<JSTaggedValue> target,
                                                           JSHandle<JSTaggedValue> object,
                                                           JSHandle<JSTaggedValue> instOfHandler);
    static inline JSTaggedValue RuntimeCreateGeneratorObj(JSThread *thread, const JSHandle<JSTaggedValue> &genFunc);

    static inline JSTaggedValue RuntimeCreateAsyncGeneratorObj(JSThread *thread,
                                                               const JSHandle<JSTaggedValue> &genFunc);

    static inline JSTaggedValue RuntimeAsyncGeneratorResolve(JSThread *thread, JSHandle<JSTaggedValue> asyncFuncObj,
                                                             JSHandle<JSTaggedValue> value, JSTaggedValue flag);
    static inline JSTaggedValue RuntimeAsyncGeneratorReject(JSThread *thread, JSHandle<JSTaggedValue> asyncFuncObj,
                                                            JSHandle<JSTaggedValue> value);
    static inline JSTaggedValue RuntimeGetTemplateObject(JSThread *thread, const JSHandle<JSTaggedValue> &literal);
    static inline JSTaggedValue RuntimeGetNextPropName(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    static inline JSTaggedValue RuntimeIterNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    static inline JSTaggedValue RuntimeCloseIterator(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    static inline JSTaggedValue RuntimeSuperCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                       const JSHandle<JSTaggedValue> &newTarget,
                                                       const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeDelObjProp(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &prop);
    static inline JSTaggedValue RuntimeNewObjApply(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                   const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeCreateIterResultObj(JSThread *thread, const JSHandle<JSTaggedValue> &value,
                                                           JSTaggedValue flag);
    static inline JSTaggedValue RuntimeAsyncFunctionAwaitUncaught(JSThread *thread,
                                                                  const JSHandle<JSTaggedValue> &asyncFuncObj,
                                                                  const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeAsyncFunctionResolveOrReject(JSThread *thread,
                                                                    const JSHandle<JSTaggedValue> &asyncFuncObj,
                                                                    const JSHandle<JSTaggedValue> &value,
                                                                    bool is_resolve);
    static inline JSTaggedValue RuntimeCopyDataProperties(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                          const JSHandle<JSTaggedValue> &src);
    static inline JSTaggedValue RuntimeStArraySpread(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                     JSTaggedValue index, const JSHandle<JSTaggedValue> &src);
    static inline JSTaggedValue RuntimeSetObjectWithProto(JSThread *thread, const JSHandle<JSTaggedValue> &proto,
                                                          const JSHandle<JSObject> &obj);
    static inline JSTaggedValue RuntimeGetIteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                       const JSHandle<JSTaggedValue> &method);
    static inline JSTaggedValue RuntimeLdObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &prop, bool callGetter,
                                                    JSTaggedValue receiver);
    static inline JSTaggedValue RuntimeStObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &prop,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStOwnByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &key,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeLdSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                      const JSHandle<JSTaggedValue> &key, JSTaggedValue thisFunc);
    static inline JSTaggedValue RuntimeStSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                      const JSHandle<JSTaggedValue> &key,
                                                      const JSHandle<JSTaggedValue> &value, JSTaggedValue thisFunc);
    static inline JSTaggedValue RuntimeLdObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t idx,
                                                    bool callGetter, JSTaggedValue receiver);
    static inline JSTaggedValue RuntimeStObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t idx,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStOwnByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &idx,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStGlobalRecord(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                      const JSHandle<JSTaggedValue> &value, bool isConst);
    static inline JSTaggedValue RuntimeNeg(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeNot(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeResolveClass(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                    const JSHandle<TaggedArray> &literal,
                                                    const JSHandle<JSTaggedValue> &base,
                                                    const JSHandle<JSTaggedValue> &lexenv);
    static inline JSTaggedValue RuntimeCloneClassFromTemplate(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                              const JSHandle<JSTaggedValue> &base,
                                                              const JSHandle<JSTaggedValue> &lexenv);
    static inline JSTaggedValue RuntimeCreateClassWithBuffer(JSThread *thread,
                                                             const JSHandle<JSTaggedValue> &base,
                                                             const JSHandle<JSTaggedValue> &lexenv,
                                                             const JSHandle<JSTaggedValue> &constpool,
                                                             uint16_t methodId, uint16_t literalId,
                                                             const JSHandle<JSTaggedValue> &module);
    static inline JSTaggedValue RuntimeSetClassInheritanceRelationship(JSThread *thread,
                                                                       const JSHandle<JSTaggedValue> &ctor,
                                                                       const JSHandle<JSTaggedValue> &base);
    static inline JSTaggedValue RuntimeSetClassConstructorLength(JSThread *thread, JSTaggedValue ctor,
                                                                 JSTaggedValue length);
    static inline JSTaggedValue RuntimeNotifyInlineCache(JSThread *thread, const JSHandle<Method> &method,
                                                         uint32_t icSlotSize);
    static inline JSTaggedValue RuntimeStOwnByValueWithNameSet(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                               const JSHandle<JSTaggedValue> &key,
                                                               const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStOwnByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                   const JSHandle<JSTaggedValue> &prop,
                                                   const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeSuspendGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                        const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeGetModuleNamespace(JSThread *thread, int32_t index);
    static inline JSTaggedValue RuntimeGetModuleNamespace(JSThread *thread, int32_t index,
                                                          JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName);
    static inline JSTaggedValue RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName,
                                                          JSTaggedValue jsFunc);
    static inline void RuntimeStModuleVar(JSThread *thread, int32_t index, JSTaggedValue value);
    static inline void RuntimeStModuleVar(JSThread *thread, int32_t index, JSTaggedValue value,
                                          JSTaggedValue jsFunc);
    static inline void RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static inline void RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value,
                                          JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdLocalModuleVar(JSThread *thread, int32_t index);
    static inline JSTaggedValue RuntimeLdLocalModuleVar(JSThread *thread, int32_t index,
                                                        JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdExternalModuleVar(JSThread *thread, int32_t index);
    static inline JSTaggedValue RuntimeLdExternalModuleVar(JSThread *thread, int32_t index,
                                                           JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner);
    static inline JSTaggedValue RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner,
                                                   JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeGetPropIterator(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeAsyncFunctionEnter(JSThread *thread);
    static inline JSTaggedValue RuntimeGetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static inline JSTaggedValue RuntimeGetAsyncIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static inline void RuntimeSetGeneratorState(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                        const int32_t index);
    static inline void RuntimeThrow(JSThread *thread, JSTaggedValue value);
    static inline void RuntimeThrowThrowNotExists(JSThread *thread);
    static inline void RuntimeThrowPatternNonCoercible(JSThread *thread);
    static inline void RuntimeThrowDeleteSuperProperty(JSThread *thread);
    static inline void RuntimeThrowUndefinedIfHole(JSThread *thread, const JSHandle<EcmaString> &obj);
    static inline void RuntimeThrowIfNotObject(JSThread *thread);
    static inline void RuntimeThrowConstAssignment(JSThread *thread, const JSHandle<EcmaString> &value);
    static inline JSTaggedValue RuntimeLdGlobalRecord(JSThread *thread, JSTaggedValue key);
    static inline JSTaggedValue RuntimeTryLdGlobalByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                         const JSHandle<JSTaggedValue> &prop);
    static inline JSTaggedValue RuntimeTryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value);
    static inline JSTaggedValue RuntimeThrowReferenceError(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                           const char *desc);
    static inline JSTaggedValue RuntimeLdGlobalVarFromProto(JSThread *thread, const JSHandle<JSTaggedValue> &globalObj,
                                                            const JSHandle<JSTaggedValue> &prop);
    static inline JSTaggedValue RuntimeStGlobalVar(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                   const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeDynamicImport(JSThread *thread, const JSHandle<JSTaggedValue> &specifier,
                                                     const JSHandle<JSTaggedValue> &func);
    static inline JSTaggedValue RuntimeToNumeric(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                          const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeLdObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                   bool callGetter, JSTaggedValue receiver);
    static inline JSTaggedValue RuntimeNotEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                             const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeLess(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeLessEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                              const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeGreater(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeGreaterEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                 const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeAdd2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeShl2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeShr2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeSub2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeMul2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeDiv2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeMod2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeAshr2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                             const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeAnd2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeOr2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeXor2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeStOwnByNameWithNameSet(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &obj,
                                                              const JSHandle<JSTaggedValue> &prop,
                                                              const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStObjByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                   const JSHandle<JSTaggedValue> &prop,
                                                   const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeToJSTaggedValueWithInt32(JSThread *thread,
                                                                const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeToJSTaggedValueWithUint32(JSThread *thread,
                                                                 const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeCreateEmptyObject(JSThread *thread, ObjectFactory *factory,
                                                         JSHandle<GlobalEnv> globalEnv);
    static inline JSTaggedValue RuntimeCreateEmptyArray(JSThread *thread, ObjectFactory *factory,
                                                        JSHandle<GlobalEnv> globalEnv);
    static inline JSTaggedValue RuntimeGetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                                      uint32_t startIdx);
    static inline JSTaggedValue RuntimeCopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs,
                                                    uint32_t startIdx);
    static inline JSTaggedValue RuntimeCreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                             const JSHandle<JSTaggedValue> &literal);
    static inline JSTaggedValue RuntimeCreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                              const JSHandle<JSObject> &literal);
    static inline JSTaggedValue RuntimeNewLexicalEnv(JSThread *thread, uint16_t numVars);
    static inline JSTaggedValue RuntimeNewObjRange(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                   const JSHandle<JSTaggedValue> &newTarget, uint16_t firstArgIdx,
                                                   uint16_t length);
    static inline JSTaggedValue RuntimeDefinefunc(JSThread *thread, const JSHandle<Method> &methodHandle);
    static inline JSTaggedValue RuntimeCreateRegExpWithLiteral(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                                               uint8_t flags);
    static inline JSTaggedValue RuntimeThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index,
                                                                  JSTaggedValue thisValue);
    static inline JSTaggedValue RuntimeCreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory,
                                                                const JSHandle<JSObject> &literal,
                                                                const JSHandle<JSTaggedValue> &env);
    static inline JSTaggedValue RuntimeCreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys,
                                                                    const JSHandle<JSTaggedValue> &objVal,
                                                                    uint16_t firstArgRegIdx);
    static inline JSTaggedValue RuntimeDefineMethod(JSThread *thread, const JSHandle<Method> &methodHandle,
                                                    const JSHandle<JSTaggedValue> &homeObject);
    static inline JSTaggedValue RuntimeCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                     const JSHandle<JSTaggedValue> &obj,
                                                     const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeDefineGetterSetterByValue(JSThread *thread, const JSHandle<JSObject> &obj,
                                                                 const JSHandle<JSTaggedValue> &prop,
                                                                 const JSHandle<JSTaggedValue> &getter,
                                                                 const JSHandle<JSTaggedValue> &setter, bool flag);
    static inline JSTaggedValue RuntimeSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                 const JSHandle<JSTaggedValue> &newTarget, uint16_t firstVRegIdx,
                                                 uint16_t length);
    static inline JSTaggedValue RuntimeOptSuperCall(JSThread *thread, uintptr_t argv, uint32_t argc);
    static inline JSTaggedValue RuntimeThrowTypeError(JSThread *thread, const char *message);
    static inline JSTaggedValue RuntimeGetCallSpreadArgs(JSThread *thread, const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc);
    static inline JSTaggedValue RuntimeThrowSyntaxError(JSThread *thread, const char *message);
    static inline JSTaggedValue RuntimeLdBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &numberBigInt);
    static inline JSTaggedValue RuntimeNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId);
    static inline JSTaggedValue RuntimeOptGetUnmapedArgs(JSThread *thread, uint32_t actualNumArgs);
    static inline JSTaggedValue RuntimeGetUnmapedJSArgumentObj(JSThread *thread,
                                                               const JSHandle<TaggedArray> &argumentsList);
    static inline JSTaggedValue RuntimeOptNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId,
                                                                JSHandle<JSTaggedValue> &currentLexEnv,
                                                                JSHandle<JSTaggedValue> &func);
    static inline JSTaggedValue RuntimeOptCopyRestArgs(JSThread *thread, uint32_t actualArgc, uint32_t restIndex);
    static inline JSTaggedValue RuntimeOptSuspendGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                           const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeOptAsyncGeneratorResolve(JSThread *thread, JSHandle<JSTaggedValue> asyncFuncObj,
                                                                JSHandle<JSTaggedValue> value, JSTaggedValue flag);
    static inline JSTaggedValue CommonCreateObjectWithExcludedKeys(JSThread *thread,
                                                                   const JSHandle<JSTaggedValue> &objVal,
                                                                   uint32_t numExcludedKeys,
                                                                   JSHandle<TaggedArray> excludedKeys);
    static inline JSTaggedValue RuntimeOptCreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys,
                                                                       const JSHandle<JSTaggedValue> &objVal,
                                                                       uint16_t firstArgRegIdx,
                                                                       uintptr_t argv, uint32_t argc);
    static inline JSTaggedValue RuntimeOptNewObjRange(JSThread *thread, uintptr_t argv, uint32_t argc);
    static inline JSTaggedValue RuntimeOptConstruct(JSThread *thread, JSHandle<JSTaggedValue> ctor,
                                                    JSHandle<JSTaggedValue> newTarget, JSHandle<JSTaggedValue> preArgs,
                                                    JSHandle<TaggedArray> args);
    static inline JSTaggedValue RuntimeOptConstructProxy(JSThread *thread, JSHandle<JSProxy> ctor,
                                                         JSHandle<JSTaggedValue> newTgt,
                                                         JSHandle<JSTaggedValue> preArgs, JSHandle<TaggedArray> args);
    static inline JSTaggedValue RuntimeOptConstructBoundFunction(JSThread *thread, JSHandle<JSBoundFunction> ctor,
                                                                 JSHandle<JSTaggedValue> newTgt,
                                                                 JSHandle<JSTaggedValue> preArgs,
                                                                 JSHandle<TaggedArray> args);
    static inline JSTaggedValue RuntimeOptConstructGeneric(JSThread *thread, JSHandle<JSFunction> ctor,
                                                           JSHandle<JSTaggedValue> newTgt,
                                                           JSHandle<JSTaggedValue> preArgs, JSHandle<TaggedArray> args);
    static inline JSTaggedValue RuntimeOptGenerateScopeInfo(JSThread *thread, uint16_t scopeId, JSTaggedValue func);
    static inline JSTaggedType *GetActualArgv(JSThread *thread);
    static inline JSTaggedType *GetActualArgvFromStub(JSThread *thread);
    static inline OptimizedJSFunctionFrame *GetOptimizedJSFunctionFrame(JSThread *thread);
    static inline OptimizedJSFunctionFrame *GetOptimizedJSFunctionFrameNoGC(JSThread *thread);

    static JSTaggedValue NewObject(EcmaRuntimeCallInfo *info);
    static void SaveFrameToContext(JSThread *thread, JSHandle<GeneratorContext> context);

    static inline JSTaggedValue RuntimeLdPatchVar(JSThread *thread, uint32_t index);
    static inline JSTaggedValue RuntimeStPatchVar(JSThread *thread, uint32_t index,
                                                  const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeNotifyConcurrentResult(JSThread *thread, JSTaggedValue result,
        JSTaggedValue hint);
    static inline JSTaggedValue RuntimeNotifyDebuggerStatement(JSThread *thread);
    static inline JSTaggedValue RuntimeThrowStackOverflowException(JSThread *thread);
    static inline bool CheckElementsNumber(JSHandle<TaggedArray> elements, uint32_t len);
    static inline JSHandle<JSTaggedValue> GetOrCreateNumberString(JSThread *thread,
        JSHandle<JSTaggedValue> presentValue, std::map<uint64_t, JSHandle<JSTaggedValue>> &cachedString);
    static inline JSTaggedValue TryCopyCOWArray(JSThread *thread, JSHandle<JSArray> holderHandler, bool &isCOWArray);
    static inline JSTaggedValue ArrayNumberSort(JSThread *thread, JSHandle<JSObject> thisObj, uint32_t len);
    friend class SlowRuntimeStub;
};
}  // namespace panda::ecmascript
#endif
