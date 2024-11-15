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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ARK_TOOLS_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ARK_TOOLS_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_thread.h"

// List of functions in ArkTools, extension of ArkTS engine.
// V(name, func, length, stubIndex)
// where BuiltinsArkTools::func refers to the native implementation of ArkTools[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_ARK_TOOLS_FUNCTIONS_COMMON(V)                                             \
    V("compareHClass",                  CompareHClass,                  2, INVALID)       \
    V("dumpHClass",                     DumpHClass,                     1, INVALID)       \
    V("excutePendingJob",               ExcutePendingJob,               0, INVALID)       \
    V("forceFullGC",                    ForceFullGC,                    0, INVALID)       \
    V("getHClass",                      GetHClass,                      1, INVALID)       \
    V("getLexicalEnv",                  GetLexicalEnv,                  1, INVALID)       \
    V("isSlicedString",                 IsSlicedString,                 1, INVALID)       \
    V("hiddenStackSourceFile",          HiddenStackSourceFile,          0, INVALID)       \
    V("hintGC",                         HintGC,                         0, INVALID)       \
    V("isNotHoleProperty",              IsNotHoleProperty,              2, INVALID)       \
    V("isPrototype",                    IsPrototype,                    1, INVALID)       \
    V("isRegExpReplaceDetectorValid",   IsRegExpReplaceDetectorValid,   0, INVALID)       \
    V("isRegExpFlagsDetectorValid",     IsRegExpFlagsDetectorValid,     0, INVALID)       \
    V("isNumberStringNotRegexpLikeDetectorValid", IsNumberStringNotRegexpLikeDetectorValid, 0, INVALID)      \
    V("isSymbolIteratorDetectorValid",  IsSymbolIteratorDetectorValid,  1, INVALID)       \
    V("isTSHClass",                     IsTSHClass,                     1, INVALID)       \
    V("pgoAssertType",                  PGOAssertType,                  2, INVALID)       \
    V("print",                          ObjectDump,                     0, INVALID)       \
    V("removeAOTFlag",                  RemoveAOTFlag,                  1, INVALID)       \
    V("timeInUs",                       TimeInUs,                       0, INVALID)       \
    V("getElementsKind",                GetElementsKind,                1, INVALID)       \
    V("isAOTCompiled",                  IsAOTCompiled,                  1, INVALID)       \
    V("isSameProfileTypeInfo",          IsSameProfileTypeInfo,          2, INVALID)       \
    V("isProfileTypeInfoValid",         IsProfileTypeInfoValid,         1, INVALID)       \
    V("isAOTDeoptimized",               IsAOTDeoptimized,               1, INVALID)       \
    V("printTypedOpProfiler",           PrintTypedOpProfiler,           1, INVALID)       \
    V("clearTypedOpProfiler",           ClearTypedOpProfiler,           0, INVALID)       \
    V("isOnHeap",                       IsOnHeap,                       1, INVALID)       \
    V("checkDeoptStatus",               CheckDeoptStatus,               2, INVALID)       \
    V("checkCircularImport",            CheckCircularImport,            2, INVALID)       \
    V("hashCode",                       HashCode,                       1, ArkToolsHashCode)    \
    V("startRuntimeStat",               StartRuntimeStat,               0, INVALID)       \
    V("stopRuntimeStat",                StopRuntimeStat,                0, INVALID)       \
    V("iterateFrame",                   IterateFrame,                   0, INVALID)

// List of mock ArkTools extension builtins
#define BUILTIN_ARK_TOOLS_FUNCTIONS_REGRESS(V)                                                                \
    V("prepareFunctionForOptimization",            PrepareFunctionForOptimization,            1, INVALID)     \
    V("optimizeFunctionOnNextCall",                OptimizeFunctionOnNextCall,                1, INVALID)     \
    V("optimizeMaglevOnNextCall",                  OptimizeMaglevOnNextCall,                  1, INVALID)     \
    V("deoptimizeFunction",                        DeoptimizeFunction,                        1, INVALID)     \
    V("optimizeOsr",                               OptimizeOsr,                               1, INVALID)     \
    V("neverOptimizeFunction",                     NeverOptimizeFunction,                     1, INVALID)     \
    V("heapObjectVerify",                          HeapObjectVerify,                          1, INVALID)     \
    V("disableOptimizationFinalization",           DisableOptimizationFinalization,           0, INVALID)     \
    V("deoptimizeNow",                             DeoptimizeNow,                             0, INVALID)     \
    V("deoptimize_now",                            DeoptimizeNow,                             0, INVALID)     \
    V("waitForBackgroundOptimization",             WaitForBackgroundOptimization,             0, INVALID)     \
    V("gc",                                        Gc,                                        0, INVALID)     \
    V("toLength",                                  ToLength,                                  1, INVALID)     \
    V("hasHoleyElements",                          HasHoleyElements,                          1, INVALID)     \
    V("hasDictionaryElements",                     HasDictionaryElements,                     1, INVALID)     \
    V("hasSmiElements",                            HasSmiElements,                            1, INVALID)     \
    V("hasDoubleElements",                         HasDoubleElements,                         1, INVALID)     \
    V("hasObjectElements",                         HasObjectElements,                         1, INVALID)     \
    V("arrayBufferDetach",                         ArrayBufferDetach,                         1, INVALID)     \
    V("haveSameMap",                               HaveSameMap,                               2, INVALID)     \
    V("isSameHeapObject",                          IsSameHeapObject,                          2, INVALID)     \
    V("isSmi",                                     IsSmi,                                     1, INVALID)     \
    V("createPrivateSymbol",                       CreatePrivateSymbol,                       1, INVALID)     \
    V("isArray",                                   IsArray,                                   1, INVALID)     \
    V("isStableJsArray",                           IsStableJsArray,                           1, INVALID)     \
    V("createDataProperty",                        CreateDataProperty,                        3, INVALID)     \
    V("functionGetInferredName",                   FunctionGetInferredName,                   1, INVALID)     \
    V("stringLessThan",                            StringLessThan,                            2, INVALID)     \
    V("stringMaxLength",                           StringMaxLength,                           0, INVALID)     \
    V("arrayBufferMaxByteLength",                  ArrayBufferMaxByteLength,                  0, INVALID)     \
    V("typedArrayMaxLength",                       TypedArrayMaxLength,                       0, INVALID)     \
    V("maxSmi",                                    MaxSmi,                                    0, INVALID)     \
    V("is64Bit",                                   Is64Bit,                                   0, INVALID)     \
    V("finalizeOptimization",                      FinalizeOptimization,                      0, INVALID)     \
    V("ensureFeedbackVectorForFunction",           EnsureFeedbackVectorForFunction,           1, INVALID)     \
    V("compileBaseline",                           CompileBaseline,                           1, INVALID)     \
    V("debugGetLoadedScriptIds",                   DebugGetLoadedScriptIds,                   0, INVALID)     \
    V("toFastProperties",                          ToFastProperties,                          1, INVALID)     \
    V("abortJS",                                   AbortJS,                                   1, INVALID)     \
    V("internalizeString",                         InternalizeString,                         1, INVALID)     \
    V("handleDebuggerStatement",                   HandleDebuggerStatement,                   0, INVALID)     \
    V("setAllocationTimeout",                      SetAllocationTimeout,                      0, INVALID)     \
    V("hasFastProperties",                         HasFastProperties,                         1, INVALID)     \
    V("hasOwnConstDataProperty",                   HasOwnConstDataProperty,                   2, INVALID)     \
    V("getHoleNaNUpper",                           GetHoleNaNUpper,                           0, INVALID)     \
    V("getHoleNaNLower",                           GetHoleNaNLower,                           0, INVALID)     \
    V("systemBreak",                               SystemBreak,                               0, INVALID)     \
    V("scheduleBreak",                             ScheduleBreak,                             0, INVALID)     \
    V("enqueueMicrotask",                          EnqueueMicrotask,                          1, INVALID)     \
    V("debugPrint",                                DebugPrint,                                1, INVALID)     \
    V("getOptimizationStatus",                     GetOptimizationStatus,                     1, INVALID)     \
    V("getUndetectable",                           GetUndetectable,                           0, INVALID)     \
    V("setKeyedProperty",                          SetKeyedProperty,                          3, INVALID)     \
    V("disassembleFunction",                       DisassembleFunction,                       1, INVALID)     \
    V("tryMigrateInstance",                        TryMigrateInstance,                        1, INVALID)     \
    V("inLargeObjectSpace",                        InLargeObjectSpace,                        1, INVALID)     \
    V("performMicrotaskCheckpoint",                PerformMicrotaskCheckpoint,                0, INVALID)     \
    V("isJSReceiver",                              IsJSReceiver,                              1, INVALID)     \
    V("isDictPropertyConstTrackingEnabled",        IsDictPropertyConstTrackingEnabled,        1, INVALID)     \
    V("allocateHeapNumber",                        AllocateHeapNumber,                        1, INVALID)     \
    V("constructConsString",                       ConstructConsString,                       1, INVALID)     \
    V("completeInobjectSlackTracking",             CompleteInobjectSlackTracking,             1, INVALID)     \
    V("normalizeElements",                         NormalizeElements,                         1, INVALID)     \
    V("call",                                      Call,                                      2, INVALID)     \
    V("debugPushPromise",                          DebugPushPromise,                          1, INVALID)     \
    V("setForceSlowPath",                          SetForceSlowPath,                          1, INVALID)     \
    V("notifyContextDisposed",                     NotifyContextDisposed,                     0, INVALID)     \
    V("optimizeObjectForAddingMultipleProperties", OptimizeObjectForAddingMultipleProperties, 2, INVALID)     \
    V("isBeingInterpreted",                        IsBeingInterpreted,                        0, INVALID)     \
    V("clearFunctionFeedback",                     ClearFunctionFeedback,                     1, INVALID)     \
    V("inYoungSpace",                              InYoungSpace,                              1, INVALID)     \
    V("inOldSpace",                                InOldSpace,                                1, INVALID)

#define BUILTIN_ARK_TOOLS_FUNCTIONS_JITCOMPILE(V)                                                             \
    V("jitCompileSync",                            JitCompileSync,                            1, INVALID)     \
    V("jitCompileAsync",                           JitCompileAsync,                           1, INVALID)     \
    V("waitJitCompileFinish",                      WaitJitCompileFinish,                      1, INVALID)     \
    V("waitAllJitCompileFinish",                   WaitAllJitCompileFinish,                   0, INVALID)

#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
#define BUILTIN_ARK_TOOLS_FUNCTIONS_SCOPE_LOCK_STATS(V)                                   \
    V("startScopeLockStats",            StartScopeLockStats,            0, INVALID)       \
    V("stopScopeLockStats",             StopScopeLockStats,             0, INVALID)
#else
#define BUILTIN_ARK_TOOLS_FUNCTIONS_SCOPE_LOCK_STATS(V) // Nothing
#endif

#ifdef ECMASCRIPT_SUPPORT_CPUPROFILER
#define BUILTIN_ARK_TOOLS_FUNCTIONS_CPUPROFILER(V)      \
    V("startCpuProf", StartCpuProfiler, 0, INVALID)     \
    V("stopCpuProf",  StopCpuProfiler,  0, INVALID)
#else
#define BUILTIN_ARK_TOOLS_FUNCTIONS_CPUPROFILER(V) // Nothing
#endif

#define BUILTIN_ARK_TOOLS_FUNCTIONS(V)                  \
    BUILTIN_ARK_TOOLS_FUNCTIONS_COMMON(V)               \
    BUILTIN_ARK_TOOLS_FUNCTIONS_CPUPROFILER(V)          \
    BUILTIN_ARK_TOOLS_FUNCTIONS_REGRESS(V)              \
    BUILTIN_ARK_TOOLS_FUNCTIONS_SCOPE_LOCK_STATS(V)     \
    BUILTIN_ARK_TOOLS_FUNCTIONS_JITCOMPILE(V)

namespace panda::ecmascript::builtins {
class BuiltinsArkTools : public base::BuiltinsBase {
public:
    // Make sure the enable-ark-tools and open-ark-tools has been opened before use it
    // Use through ArkTools.print(msg, [obj1, obj2, ... objn]) in js
    static JSTaggedValue ObjectDump(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ExcutePendingJob(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetLexicalEnv(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CompareHClass(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DumpHClass(EcmaRuntimeCallInfo *info);

    // return whether the hclass used for object is created by AOT
    static JSTaggedValue IsTSHClass(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetHClass(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsSlicedString(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsStableJsArray(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsNotHoleProperty(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ForcePartialGC(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ForceFullGC(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HintGC(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HiddenStackSourceFile(EcmaRuntimeCallInfo *info);

    static JSTaggedValue RemoveAOTFlag(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CheckCircularImport(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HashCode(EcmaRuntimeCallInfo *info);

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    static JSTaggedValue StartCpuProfiler(EcmaRuntimeCallInfo *info);

    static JSTaggedValue StopCpuProfiler(EcmaRuntimeCallInfo *info);

    static std::string GetProfileName();

    static bool CreateFile(std::string &fileName);
#endif
    // ArkTools.isPrototype(object)
    static JSTaggedValue IsPrototype(EcmaRuntimeCallInfo *info);

    // ArkTools.isAOTCompiledAssert(func)
    static JSTaggedValue IsAOTCompiled(EcmaRuntimeCallInfo *info);

    // ArkTools.isSameProfileTypeInfo(func1, func2)
    static JSTaggedValue IsSameProfileTypeInfo(EcmaRuntimeCallInfo *info);

    // ArkTools.isProfileTypeInfoValid(func)
    static JSTaggedValue IsProfileTypeInfoValid(EcmaRuntimeCallInfo *info);

    // ArkTools.isAOTCompiledAssert(func)
    static JSTaggedValue IsAOTDeoptimized(EcmaRuntimeCallInfo *info);

    // ArkTools.CheckDeoptStatus(func, deopt?)
    static JSTaggedValue CheckDeoptStatus(EcmaRuntimeCallInfo *info);

    // ArkTools.isOnHeap(object)
    static JSTaggedValue IsOnHeap(EcmaRuntimeCallInfo *info);

    // ArkTools.GetElementsKind(array)
    static JSTaggedValue GetElementsKind(EcmaRuntimeCallInfo *info);

    static JSTaggedValue PrintTypedOpProfiler(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ClearTypedOpProfiler(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsRegExpReplaceDetectorValid(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsRegExpFlagsDetectorValid(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsNumberStringNotRegexpLikeDetectorValid(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsSymbolIteratorDetectorValid(EcmaRuntimeCallInfo *info);

    static JSTaggedValue TimeInUs(EcmaRuntimeCallInfo *info);

#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    static JSTaggedValue StartScopeLockStats(EcmaRuntimeCallInfo *info);

    static JSTaggedValue StopScopeLockStats(EcmaRuntimeCallInfo *info);
#endif

    static JSTaggedValue PrepareFunctionForOptimization(EcmaRuntimeCallInfo *info);

    static JSTaggedValue OptimizeFunctionOnNextCall(EcmaRuntimeCallInfo *info);

    static JSTaggedValue OptimizeMaglevOnNextCall(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DeoptimizeFunction(EcmaRuntimeCallInfo *info);

    static JSTaggedValue OptimizeOsr(EcmaRuntimeCallInfo *info);

    static JSTaggedValue NeverOptimizeFunction(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HeapObjectVerify(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DisableOptimizationFinalization(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DeoptimizeNow(EcmaRuntimeCallInfo *info);

    static JSTaggedValue WaitForBackgroundOptimization(EcmaRuntimeCallInfo *info);

    static JSTaggedValue Gc(EcmaRuntimeCallInfo *info);

    static JSTaggedValue PGOAssertType(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ToLength(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasHoleyElements(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasDictionaryElements(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasSmiElements(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasDoubleElements(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasObjectElements(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ArrayBufferDetach(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HaveSameMap(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsSameHeapObject(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsSmi(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CreatePrivateSymbol(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsArray(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CreateDataProperty(EcmaRuntimeCallInfo *info);

    static JSTaggedValue FunctionGetInferredName(EcmaRuntimeCallInfo *info);

    static JSTaggedValue StringLessThan(EcmaRuntimeCallInfo *info);

    static JSTaggedValue StringMaxLength(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ArrayBufferMaxByteLength(EcmaRuntimeCallInfo *info);

    static JSTaggedValue TypedArrayMaxLength(EcmaRuntimeCallInfo *info);

    static JSTaggedValue MaxSmi(EcmaRuntimeCallInfo *info);

    static JSTaggedValue Is64Bit(EcmaRuntimeCallInfo *info);

    static JSTaggedValue FinalizeOptimization(EcmaRuntimeCallInfo *info);

    static JSTaggedValue EnsureFeedbackVectorForFunction(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CompileBaseline(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DebugGetLoadedScriptIds(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ToFastProperties(EcmaRuntimeCallInfo *info);

    static JSTaggedValue AbortJS(EcmaRuntimeCallInfo *info);

    static JSTaggedValue InternalizeString(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HandleDebuggerStatement(EcmaRuntimeCallInfo *info);

    static JSTaggedValue SetAllocationTimeout(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasFastProperties(EcmaRuntimeCallInfo *info);

    static JSTaggedValue HasOwnConstDataProperty(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetHoleNaNUpper(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetHoleNaNLower(EcmaRuntimeCallInfo *info);

    static JSTaggedValue SystemBreak(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ScheduleBreak(EcmaRuntimeCallInfo *info);

    static JSTaggedValue EnqueueMicrotask(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DebugPrint(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetOptimizationStatus(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetUndetectable(EcmaRuntimeCallInfo *info);

    static JSTaggedValue SetKeyedProperty(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DisassembleFunction(EcmaRuntimeCallInfo *info);

    static JSTaggedValue TryMigrateInstance(EcmaRuntimeCallInfo *info);

    static JSTaggedValue InLargeObjectSpace(EcmaRuntimeCallInfo *info);

    static JSTaggedValue PerformMicrotaskCheckpoint(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsJSReceiver(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsDictPropertyConstTrackingEnabled(EcmaRuntimeCallInfo *info);

    static JSTaggedValue AllocateHeapNumber(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ConstructConsString(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CompleteInobjectSlackTracking(EcmaRuntimeCallInfo *info);

    static JSTaggedValue NormalizeElements(EcmaRuntimeCallInfo *info);

    static JSTaggedValue Call(EcmaRuntimeCallInfo *info);

    static JSTaggedValue DebugPushPromise(EcmaRuntimeCallInfo *info);

    static JSTaggedValue SetForceSlowPath(EcmaRuntimeCallInfo *info);

    static JSTaggedValue NotifyContextDisposed(EcmaRuntimeCallInfo *info);

    static JSTaggedValue OptimizeObjectForAddingMultipleProperties(EcmaRuntimeCallInfo *info);

    static JSTaggedValue IsBeingInterpreted(EcmaRuntimeCallInfo *info);

    static JSTaggedValue ClearFunctionFeedback(EcmaRuntimeCallInfo *info);

    static JSTaggedValue JitCompileSync(EcmaRuntimeCallInfo *info);
    static JSTaggedValue JitCompileAsync(EcmaRuntimeCallInfo *info);
    static JSTaggedValue WaitJitCompileFinish(EcmaRuntimeCallInfo *info);
    static JSTaggedValue WaitAllJitCompileFinish(EcmaRuntimeCallInfo *info);

    static JSTaggedValue StartRuntimeStat(EcmaRuntimeCallInfo *info);
    static JSTaggedValue StopRuntimeStat(EcmaRuntimeCallInfo *info);
    
    static JSTaggedValue IterateFrame(EcmaRuntimeCallInfo *info);

    static JSTaggedValue InYoungSpace(EcmaRuntimeCallInfo *info);
    static JSTaggedValue InOldSpace(EcmaRuntimeCallInfo *info);

    static Span<const base::BuiltinFunctionEntry> GetArkToolsFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(ARK_TOOLS_FUNCTIONS);
    }

private:
#define BUILTINS_ARK_TOOLS_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsArkTools::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array ARK_TOOLS_FUNCTIONS  = {
        BUILTIN_ARK_TOOLS_FUNCTIONS(BUILTINS_ARK_TOOLS_FUNCTION_ENTRY)
    };
#undef BUILTINS_ARK_TOOLS_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ARK_TOOLS_H
