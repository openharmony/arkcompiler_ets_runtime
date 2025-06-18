/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/frames.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/method.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/stubs/runtime_stub_list.h"

namespace panda::ecmascript {
class EcmaVM;
class GlobalEnv;
class JSThread;
class JSFunction;
class ObjectFactory;
class JSBoundFunction;
class JSProxy;
class JSTypedArray;
class AOTLiteralInfo;
class GeneratorContext;
struct EcmaRuntimeCallInfo;

using JSFunctionEntryType = JSTaggedValue (*)(uintptr_t glue, uint32_t argc, const JSTaggedType argV[],
                                              uintptr_t prevFp, bool needPushArgv);
using FastCallAotEntryType = JSTaggedValue (*)(uintptr_t glue, uint32_t argc, const JSTaggedType argV[],
                                              uintptr_t prevFp);

PUBLIC_API extern "C" JSTaggedType LazyDeoptEntry();
class RuntimeStubs {
public:
    static constexpr uint32_t TRACE_NUMBER = 100;
    static constexpr uint32_t RT_STUB_FUNC_SIZE = 8;

    static void Initialize(JSThread *thread);

#define DECLARE_RUNTIME_STUBS(name) \
    static JSTaggedType name(uintptr_t argGlue, uint32_t argc, uintptr_t argv);
    RUNTIME_STUB_WITH_GC_LIST(DECLARE_RUNTIME_STUBS)
    RUNTIME_STUB_WITH_DFX(DECLARE_RUNTIME_STUBS)
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

    static void Dump(JSTaggedType value);
    static void DebugDump(JSTaggedType value);
    static void DumpWithHint(uintptr_t hintStrAddress, JSTaggedType value);
    static void DebugDumpWithHint(uintptr_t hintStrAddress, JSTaggedType value);
    static void DebugPrint(int fmtMessageId, ...);
    static void DebugPrintCustom(uintptr_t fmt, ...);
    static void DebugPrintInstruction([[maybe_unused]] uintptr_t argGlue, const uint8_t *pc);
    static void CollectingOpcodes([[maybe_unused]] uintptr_t argGlue, const uint8_t *pc);
    static void DebugOsrEntry([[maybe_unused]] uintptr_t argGlue, const uint8_t *codeEntry);
    static void Comment(uintptr_t argStr);
    static void FatalPrint(int fmtMessageId, ...);
    static void FatalPrintCustom(uintptr_t fmt, ...);
    static void MarkingBarrier([[maybe_unused]] uintptr_t argGlue,
        uintptr_t object, size_t offset, TaggedObject *value);
    static void SharedGCMarkingBarrier(uintptr_t argGlue, uintptr_t object, size_t offset, TaggedObject *value);
    static void CMCGCMarkingBarrier(uintptr_t argGlue, uintptr_t object, size_t offset, TaggedObject *value);
    static JSTaggedType ReadBarrier(uintptr_t argGlue, uintptr_t addr);
    static void CopyCallTarget(uintptr_t argGlue, uintptr_t callTarget);
    static void CopyArgvArray(uintptr_t argGlue, uintptr_t argv, uint64_t argc);
    static JSTaggedType GetActualArgvNoGC(uintptr_t argGlue);
    static void InsertOldToNewRSet([[maybe_unused]] uintptr_t argGlue, uintptr_t object, size_t offset);
    static void InsertLocalToShareRSet([[maybe_unused]] uintptr_t argGlue, uintptr_t object, size_t offset);
    static void SetBitAtomic(GCBitset::GCBitsetWord *word, GCBitset::GCBitsetWord mask,
                             GCBitset::GCBitsetWord oldValue);
    static int32_t DoubleToInt(double x, size_t bits);
    static int32_t SaturateTruncDoubleToInt32(double x);
    static uint8_t LrInt(double x);
    static double FloatMod(double x, double y);
    static double FloatAcos(double x);
    static double FloatAcosh(double x);
    static double FloatAsin(double x);
    static double FloatAsinh(double x);
    static double FloatAtan(double x);
    static double FloatAtan2(double y, double x);
    static double FloatAtanh(double x);
    static double FloatCos(double x);
    static double FloatCosh(double x);
    static double FloatSin(double x);
    static double FloatSinh(double x);
    static double FloatTan(double x);
    static double FloatTanh(double x);
    static double FloatTrunc(double x);
    static double FloatFloor(double x);
    static double FloatLog(double x);
    static double FloatLog2(double x);
    static double FloatLog10(double x);
    static double FloatLog1p(double x);
    static double FloatExp(double x);
    static double FloatExpm1(double x);
    static double FloatPow(double base, double exp);
    static double FloatCbrt(double x);
    static double FloatCeil(double x);
    static double CallDateNow();
    static void UpdateFieldType(JSTaggedType hclass, uint64_t value);
    static bool BigIntEquals(JSTaggedType left, JSTaggedType right);
    static bool BigIntSameValueZero(JSTaggedType key, JSTaggedType other);
    static JSTaggedValue JSHClassFindProtoTransitions(JSHClass *cls, JSTaggedValue key, JSTaggedValue proto);
    static void FinishObjSizeTracking(JSHClass *cls);
    static JSTaggedValue NumberHelperStringToDouble(EcmaString *str);
    static int IntLexicographicCompare(JSTaggedType x, JSTaggedType y);
    static int DoubleLexicographicCompare(JSTaggedType x, JSTaggedType y);
    static int FastArraySortString(uintptr_t argGlue, JSTaggedValue x, JSTaggedValue y);
    static JSTaggedValue StringToNumber(JSTaggedType numberString, int32_t radix);
    static void ArrayTrim(uintptr_t argGlue, TaggedArray *array, int64_t newLength);
    static double TimeClip(double time);
    static double SetDateValues(double year, double month, double day);
    static void StartCallTimer(uintptr_t argGlue, JSTaggedType func, bool isAot);
    static void EndCallTimer(uintptr_t argGlue, JSTaggedType func);
    static JSTaggedValue RuntimeArraySort(JSThread *thread, JSHandle<JSTaggedValue> thisHandle);

    static JSTaggedValue CallBoundFunction(EcmaRuntimeCallInfo *info);

    static int32_t StringGetStart(bool isUtf8, EcmaString *srcString, int32_t length, int32_t startIndex);
    static int32_t StringGetEnd(bool isUtf8, EcmaString *srcString, int32_t start, int32_t length, int32_t startIndex);
    static void CopyTypedArrayBuffer(uintptr_t argGlue, JSTypedArray *srcArray, JSTypedArray *targetArray,
                                     int32_t srcStartPos, int32_t tarStartPos, int32_t count);
    static inline uint32_t RuntimeGetBytecodePcOfstForBaseline(const JSHandle<JSFunction> &func, uintptr_t nativePc);
    static void ReverseTypedArray(JSTypedArray *typedArray);
    static void SortTypedArray(JSTypedArray *typedArray);
    static inline uintptr_t RuntimeGetNativePcOfstForBaseline(const JSHandle<JSFunction> &func, uint64_t bytecodePos);
    static void ObjectCopy(uintptr_t argGlue, JSTaggedType *dstObj,
                            JSTaggedType *dst, JSTaggedType *src, uint32_t count);
    static void FillObject(JSTaggedType *dst, JSTaggedType value, uint32_t count);
    static void ReverseArray(uintptr_t argGlue, JSTaggedType *dst, uint32_t length);

    static JSTaggedValue FindPatchModule(uintptr_t argGlue, JSTaggedValue resolvedModule);
    static void FatalPrintMisstakenResolvedBinding(int32_t index, JSTaggedValue curModule);
    static void LoadNativeModuleFailed(JSTaggedValue curModule);
    static void TraceLazyDeoptCommitSuccess(uintptr_t argGlue, JSHandle<JSTaggedValue> func);
    static JSTaggedValue GetExternalModuleVar(uintptr_t argGlue, JSFunction *jsFunc, int32_t index);

private:
    static void DumpToStreamWithHint(std::ostream &out, std::string_view prompt, JSTaggedValue value);
    static inline void DumpInfoForMoreLdInfo(JSThread *thread, JSHandle<JSTaggedValue> &receiver, CString &msg);
    static inline void DumpInfoForLdObjByValue(JSHandle<JSTaggedValue> &profile,
                                               JSTaggedValue slotId, JSTaggedValue key, CString &msg);
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
    static inline JSTaggedValue RuntimeOptSuperCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                          const JSHandle<JSTaggedValue> &newTarget,
                                                          const JSHandle<JSTaggedValue> &taggedArray);
    static inline JSTaggedValue RuntimeSuperCallForwardAllArgs(JSThread *thread, JSTaggedType *sp,
                                                               const JSHandle<JSTaggedValue> &superFunc,
                                                               const JSHandle<JSTaggedValue> &newTarget,
                                                               uint32_t restNumArgs, uint32_t startIdx);
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
                                                             const JSHandle<JSTaggedValue> &module,
                                                             const JSHandle<JSTaggedValue> &length);
    static inline void SetProfileTypeInfoCellToFunction(JSThread *thread, const JSHandle<JSFunction> &jsFunc,
                                                        const JSHandle<JSFunction> &definedFunc, uint16_t slotId);
    static inline JSTaggedValue RuntimeCreateSharedClass(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> &base,
                                                         const JSHandle<JSTaggedValue> &constpool,
                                                         uint16_t methodId, uint16_t literalId, uint16_t length,
                                                         const JSHandle<JSTaggedValue> &module);
    static inline JSTaggedValue RuntimeLdSendableClass(const JSHandle<JSTaggedValue> &env, uint16_t level);
    static inline JSTaggedValue RuntimeSetClassInheritanceRelationship(JSThread *thread,
                                                                       const JSHandle<JSTaggedValue> &ctor,
                                                                       const JSHandle<JSTaggedValue> &base,
                                                                       ClassKind kind = ClassKind::NON_SENDABLE);
    static inline JSTaggedValue RuntimeSetClassConstructorLength(JSThread *thread, JSTaggedValue ctor,
                                                                 JSTaggedValue length);
    static inline JSTaggedValue RuntimeNotifyInlineCache(JSThread *thread, const JSHandle<JSFunction> &function,
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
    static inline JSTaggedValue RuntimeLdLocalModuleVarWithModule(JSThread *thread, int32_t index,
        JSHandle<JSTaggedValue> moduleHdl);
    static inline JSTaggedValue RuntimeLdLocalModuleVar(JSThread *thread, int32_t index,
                                                        JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdExternalModuleVar(JSThread *thread, int32_t index);
    static inline JSTaggedValue RuntimeLdExternalModuleVarWithModule(JSThread *thread, int32_t index,
        JSHandle<JSTaggedValue> moduleHdl);
    static inline JSTaggedValue RuntimeLdSendableExternalModuleVar(JSThread *thread, int32_t index,
                                                                   JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdSendableLocalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdExternalModuleVar(JSThread *thread, int32_t index,
                                                           JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdLazySendableExternalModuleVar(JSThread *thread, int32_t index,
                                                                       JSTaggedValue jsFunc);
    static inline JSTaggedValue RuntimeLdLazyExternalModuleVar(JSThread *thread, int32_t index,
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
    static inline JSTaggedValue RuntimeNewSendableEnv(JSThread *thread, uint16_t numVars);
    static inline JSTaggedValue RuntimeNewObjRange(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                   const JSHandle<JSTaggedValue> &newTarget, uint16_t firstArgIdx,
                                                   uint16_t length);
    static inline JSTaggedValue RuntimeDefinefunc(JSThread *thread, const JSHandle<JSTaggedValue> &constpool,
                                                  uint16_t methodId, const JSHandle<JSTaggedValue> &module,
                                                  uint16_t length, const JSHandle<JSTaggedValue> &envHandle,
                                                  const JSHandle<JSTaggedValue> &homeObject);
    static inline void DefineFuncTryUseAOTHClass(JSThread* thread,
                                                 const JSHandle<JSFunction>& func,
                                                 const JSHandle<JSTaggedValue>& ihc,
                                                 const JSHandle<AOTLiteralInfo>& aotLiteralInfo);
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
                                                    const JSHandle<JSTaggedValue> &homeObject, uint16_t length,
                                                    const JSHandle<JSTaggedValue> &env,
                                                    const JSHandle<JSTaggedValue> &module);
    static inline JSTaggedValue RuntimeCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                     const JSHandle<JSTaggedValue> &obj,
                                                     const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeDefineGetterSetterByValue(JSThread *thread, const JSHandle<JSObject> &obj,
                                                                 const JSHandle<JSTaggedValue> &prop,
                                                                 const JSHandle<JSTaggedValue> &getter,
                                                                 const JSHandle<JSTaggedValue> &setter, bool flag,
                                                                 const JSHandle<JSTaggedValue> &func,
                                                                 int32_t pcOffset);
    static inline JSTaggedValue RuntimeSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                 const JSHandle<JSTaggedValue> &newTarget, uint16_t firstVRegIdx,
                                                 uint16_t length);
    static inline JSTaggedValue RuntimeOptSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                    const JSHandle<JSTaggedValue> &newTarget,
                                                    const JSHandle<TaggedArray> &argv,
                                                    uint16_t length);
    static inline JSTaggedValue RuntimeThrowTypeError(JSThread *thread, const char *message);
    static inline JSTaggedValue RuntimeGetCallSpreadArgs(JSThread *thread, const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc);
    static inline JSTaggedValue RuntimeThrowSyntaxError(JSThread *thread, const char *message);
    static inline JSTaggedValue RuntimeLdBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &numberBigInt);
    static inline JSTaggedValue RuntimeCallBigIntAsIntN(JSThread *thread, JSTaggedValue bits, JSTaggedValue bigint);
    static inline JSTaggedValue RuntimeCallBigIntAsUintN(JSThread *thread, JSTaggedValue bits, JSTaggedValue bigint);
    static inline JSTaggedValue RuntimeNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId);
    static inline JSTaggedValue RuntimeOptGetUnmapedArgs(JSThread *thread, uint32_t actualNumArgs);
    static inline JSTaggedValue RuntimeGetUnmapedJSArgumentObj(JSThread *thread,
                                                               const JSHandle<TaggedArray> &argumentsList);
    static inline JSTaggedValue RuntimeOptNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId,
                                                                JSHandle<JSTaggedValue> &currentEnv,
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
    static inline JSTaggedValue RuntimeOptCreateObjectWithExcludedKeys(JSThread *thread, uintptr_t argv, uint32_t argc);
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
    static inline JSTaggedValue GetResultValue(JSThread *thread, bool isAotMethod, JSHandle<JSFunction> ctor,
        CVector<JSTaggedType> &values, JSHandle<JSTaggedValue> newTgt, uint32_t &size, JSHandle<JSTaggedValue> obj);
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
    static inline JSTaggedValue RuntimeDefineField(JSThread *thread, JSTaggedValue obj,
                                                   JSTaggedValue propKey, JSTaggedValue value);
    static inline JSTaggedValue RuntimeCreatePrivateProperty(JSThread *thread, JSTaggedValue constpool,
        uint32_t count, JSTaggedValue lexicalEnv, uint32_t literalId, JSTaggedValue module);
    static inline JSTaggedValue RuntimeDefinePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value);
    static inline JSTaggedValue RuntimeLdPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj);
    static inline JSTaggedValue RuntimeStPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value);
    static inline JSTaggedValue RuntimeTestIn(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj);
    static inline JSTaggedValue RuntimeUpdateAOTHClass(JSThread *thread, const JSHandle<JSHClass> &oldhclass,
        const JSHandle<JSHClass> &newhclass, JSTaggedValue key);
    static inline JSTaggedValue RuntimeNotifyDebuggerStatement(JSThread *thread);
    static inline bool CheckElementsNumber(JSHandle<TaggedArray> elements, uint32_t len);
    static inline JSHandle<JSTaggedValue> GetOrCreateNumberString(JSThread *thread,
        JSHandle<JSTaggedValue> presentValue, std::map<uint64_t, JSHandle<JSTaggedValue>> &cachedString);
    static inline JSTaggedValue TryCopyCOWArray(JSThread *thread, JSHandle<JSArray> holderHandler, bool &isCOWArray);
    static inline JSTaggedValue ArrayNumberSort(JSThread *thread, JSHandle<JSObject> thisObj, uint32_t len);
    static inline bool ShouldUseAOTHClass(const JSHandle<JSTaggedValue> &ihc,
                                          const JSHandle<JSTaggedValue> &chc,
                                          const JSHandle<ClassLiteral> &classLiteral);
    static inline JSTaggedType RuntimeTryGetInternString(uintptr_t argGlue, const JSHandle<EcmaString> &string);
    static inline void RuntimeSetPatchModule(JSThread *thread, const JSHandle<JSFunction> &func);
    template <typename T>
    static inline JSTaggedValue RuntimeDecodeURIComponent(JSThread *thread, const JSHandle<EcmaString> &str,
                                                          const T *data);
    template <typename T>
    static inline uint16_t GetCodeUnit(Span<T> &sp, int32_t index, int32_t length);
    template <typename T>
    static inline JSTaggedValue DecodePercentEncoding(JSThread *thread, const JSHandle<EcmaString> &str, int32_t &k,
                                                      int32_t strLen, std::u16string &resStr, Span<T> &sp);
    template <typename T>
    static inline JSTaggedValue DecodePercentEncoding(JSThread *thread, int32_t &n, int32_t &k,
                                                      const JSHandle<EcmaString> &str, uint8_t &bb,
                                                      std::vector<uint8_t> &oct, Span<T> &sp,
                                                      int32_t strLen);
    static inline JSTaggedValue UTF16EncodeCodePoint(JSThread *thread, const std::vector<uint8_t> &oct,
                                                     const JSHandle<EcmaString> &str, std::u16string &resStr);
    static inline bool IsFastRegExp(uintptr_t argGlue, JSTaggedValue thisValue);

    static inline RememberedSet* CreateLocalToShare(Region *region);
    static inline RememberedSet* CreateOldToNew(Region *region);
    static inline uint8_t GetValueFromTwoHex(uint8_t front, uint8_t behind);
    static inline bool IsTargetBundleName(uintptr_t argGlue);
    friend class SlowRuntimeStub;
};
}  // namespace panda::ecmascript
#endif
