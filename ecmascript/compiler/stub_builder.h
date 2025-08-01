/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_STUB_BUILDER_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/lcr_gate_meta_data.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/ic/mega_ic_cache.h"

namespace panda::ecmascript::kungfu {
struct StringInfoGateRef;
using namespace panda::ecmascript;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFVARIABLE(varname, type, val) Variable varname(GetEnvironment(), type, NextVariableId(), val)

#define SUBENTRY(messageId, condition)                                              \
    GateRef glueArg = PtrArgument(0);                                               \
    auto env = GetEnvironment();                                                    \
    Label subEntry(env);                                                            \
    env->SubCfgEntry(&subEntry);                                                    \
    Label nextLabel(env);                                                           \
    Assert(messageId, __LINE__, glueArg, condition, &nextLabel);                    \
    Bind(&nextLabel)
#define SUBENTRY_WITH_LINE(messageId, condition, line)       \
    GateRef glueArg = PtrArgument(0);                        \
    auto env = GetEnvironment();                             \
    Label subEntry(env);                                     \
    env->SubCfgEntry(&subEntry);                             \
    Label nextLabel(env);                                    \
    Assert(messageId, line, glueArg, condition, &nextLabel); \
    Bind(&nextLabel)
#define SUBENTRY_WITH_GLUE(messageId, condition, glueArg)                           \
    auto env = GetEnvironment();                                                    \
    Label subEntry(env);                                                            \
    env->SubCfgEntry(&subEntry);                                                    \
    Label nextLabel(env);                                                           \
    Assert(messageId, __LINE__, glueArg, condition, &nextLabel);                    \
    Bind(&nextLabel)

#ifndef NDEBUG
#define ASM_ASSERT(messageId, condition)                                            \
    if (!GetEnvironment()->GetCircuit()->IsOptimizedOrFastJit() &&                  \
        !GetEnvironment()->IsBaselineBuiltin()) {                                   \
        SUBENTRY(messageId, condition);                                             \
        EXITENTRY();                                                                \
    }
#define ASM_ASSERT_WITH_LINE(messageId, condition, line)                                                     \
    if (!GetEnvironment()->GetCircuit()->IsOptimizedOrFastJit() && !GetEnvironment()->IsBaselineBuiltin()) { \
        SUBENTRY_WITH_LINE(messageId, condition, line);                                                      \
        EXITENTRY();                                                                                         \
    }
#define ASM_ASSERT_WITH_GLUE(messageId, condition, glue)                            \
    SUBENTRY_WITH_GLUE(messageId, condition, glue)
#elif defined(ENABLE_ASM_ASSERT)
#define ASM_ASSERT(messageId, condition)                                            \
    if (!GetEnvironment()->GetCircuit()->IsOptimizedOrFastJit() &&                  \
        !GetEnvironment()->IsBaselineBuiltin()) {                                   \
        SUBENTRY(messageId, condition);                                             \
        EXITENTRY();                                                                \
    }
#define ASM_ASSERT_WITH_GLUE(messageId, condition, glue)                            \
    SUBENTRY_WITH_GLUE(messageId, condition, glue)
#else
#define ASM_ASSERT(messageId, ...) ((void)0)
#define ASM_ASSERT_WITH_GLUE(messageId, ...) ((void)0)
#define ASM_ASSERT_WITH_LINE(messageId, ...) ((void)0)
#endif

#ifndef NDEBUG
#define EXITENTRY()                                                                 \
    GetEnvironment()->SubCfgExit()
#elif defined(ENABLE_ASM_ASSERT)
#define EXITENTRY()                                                                 \
    GetEnvironment()->SubCfgExit()
#else
#define EXITENTRY() ((void)0)
#endif

class StubBuilder {
public:
    explicit StubBuilder(StubBuilder *parent)
        : callSignature_(parent->GetCallSignature()), env_(parent->GetEnvironment()) {}
    StubBuilder(StubBuilder *parent, GateRef globalEnv)
        : callSignature_(parent->GetCallSignature()), env_(parent->GetEnvironment()), globalEnv_(globalEnv) {}
    StubBuilder(CallSignature *callSignature, Environment *env)
        : callSignature_(callSignature), env_(env) {}
    StubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)
        : callSignature_(callSignature), env_(env), globalEnv_(globalEnv) {}
    explicit StubBuilder(Environment *env)
        : env_(env) {}
    StubBuilder(Environment *env, GateRef globalEnv)
        : env_(env), globalEnv_(globalEnv) {}
    virtual ~StubBuilder() = default;
    NO_MOVE_SEMANTIC(StubBuilder);
    NO_COPY_SEMANTIC(StubBuilder);
    virtual void GenerateCircuit() = 0;
    Environment *GetEnvironment() const
    {
        return env_;
    }
    CallSignature *GetCallSignature() const
    {
        return callSignature_;
    }
    inline GateRef GetCurrentGlobalEnv()
    {
        if (globalEnv_ == Gate::InvalidGateRef) {
            LOG_FULL(FATAL) << "globalEnv_ is InvalidGateRef";
            UNREACHABLE();
        }
        return globalEnv_;
    }
    inline void SetCurrentGlobalEnv(GateRef globalEnv)
    {
        globalEnv_ = globalEnv;
    }
    int NextVariableId();
    // constant
    GateRef Int8(int8_t value);
    GateRef Int16(int16_t value);
    GateRef Int32(int32_t value);
    GateRef Int64(int64_t value);
    GateRef TaggedInt(int32_t value);
    GateRef StringPtr(std::string_view str);
    GateRef IntPtr(int64_t value);
    GateRef IntPtrSize();
    GateRef RelocatableData(uint64_t value);
    GateRef True();
    GateRef False();
    GateRef Boolean(bool value);
    GateRef Double(double value);
    GateRef Undefined();
    GateRef Hole();
    GateRef SpecialHole();
    GateRef Null();
    GateRef NullPtr();
    GateRef Exception();
    // parameter
    GateRef Argument(size_t index);
    GateRef Int1Argument(size_t index);
    GateRef Int32Argument(size_t index);
    GateRef Int64Argument(size_t index);
    GateRef TaggedArgument(size_t index);
    GateRef TaggedPointerArgument(size_t index);
    GateRef PtrArgument(size_t index);
    GateRef Float32Argument(size_t index);
    GateRef Float64Argument(size_t index);
    GateRef Alloca(int size);
    // control flow
    GateRef Return(GateRef value);
    GateRef Return();
    void Bind(Label *label);
    void Jump(Label *label);
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel,
                       uint32_t trueWeight = BranchWeight::ONE_WEIGHT, uint32_t falseWeight = BranchWeight::ONE_WEIGHT,
                       const char *comment = nullptr);

    void Switch(GateRef index, Label *defaultLabel,
                const int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void Switch(GateRef index, Label *defaultLabel,
                const int64_t *keysValue, Label * const *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    void LoopEndWithCheckSafePoint(Label *loopHead, Environment *env, GateRef glue);
    GateRef CheckSuspend(GateRef glue);
    GateRef CheckSuspendForCMCGC(GateRef glue);
    // call operation
    GateRef CallRuntime(GateRef glue, int index, const std::vector<GateRef>& args);
    GateRef CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv);
    GateRef CallRuntimeWithGlobalEnv(GateRef glue, GateRef globalEnv, int index, const std::vector<GateRef>& args);
    GateRef CallRuntimeWithGlobalEnv(GateRef glue, GateRef globalEnv, int index, GateRef argc, GateRef argv);
    GateRef CallRuntimeWithCurrentEnv(GateRef glue, GateRef currentEnv, int index, const std::vector<GateRef>& args);

    GateRef CallNGCRuntime(GateRef glue, int index,
                           const std::vector<GateRef>& args, GateRef hir = Circuit::NullGate());
    GateRef FastCallOptimized(GateRef glue, GateRef code,
                              const std::vector<GateRef>& args, GateRef hir = Circuit::NullGate());
    GateRef CallOptimized(GateRef glue, GateRef code,
                          const std::vector<GateRef>& args, GateRef hir = Circuit::NullGate());
    GateRef GetAotCodeAddr(GateRef jsFunc);
    GateRef CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef CallCommonStub(GateRef glue, int index, const std::initializer_list<GateRef>& args,
                           GateRef hir = Circuit::NullGate());
    GateRef CallBuiltinRuntime(GateRef glue, const std::initializer_list<GateRef>& args, bool isNew = false);
    GateRef CallBuiltinRuntimeWithNewTarget(GateRef glue, const std::initializer_list<GateRef>& args);
    void DebugPrint(GateRef thread, std::initializer_list<GateRef> args);
    void FatalPrint(GateRef thread, std::initializer_list<GateRef> args);
    // memory
    GateRef Load(VariableType type, GateRef glue, GateRef base, GateRef offset,
                 MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef LoadZeroOffset(VariableType type, GateRef glue, GateRef base,
                 MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef LoadPrimitive(VariableType type, GateRef base, GateRef offset);
    GateRef LoadZeroOffsetPrimitive(VariableType type, GateRef base);
    void Store(VariableType type,
               GateRef glue,
               GateRef base,
               GateRef offset,
               GateRef value,
               MemoryAttribute mAttr = MemoryAttribute::Default());
    // arithmetic
    GateRef TaggedCastToIntPtr(GateRef x);
    GateRef Int16Add(GateRef x, GateRef y);
    GateRef Int32Add(GateRef x, GateRef y);
    GateRef Int64Add(GateRef x, GateRef y);
    GateRef DoubleAdd(GateRef x, GateRef y);
    GateRef PtrAdd(GateRef x, GateRef y);
    GateRef PtrSub(GateRef x, GateRef y);
    GateRef PtrMul(GateRef x, GateRef y);
    GateRef IntPtrEqual(GateRef x, GateRef y);
    GateRef Int16Sub(GateRef x, GateRef y);
    GateRef Int32Sub(GateRef x, GateRef y);
    GateRef Int64Sub(GateRef x, GateRef y);
    GateRef DoubleSub(GateRef x, GateRef y);
    GateRef Int32Mul(GateRef x, GateRef y);
    GateRef Int64Mul(GateRef x, GateRef y);
    GateRef DoubleMul(GateRef x, GateRef y);
    GateRef DoubleDiv(GateRef x, GateRef y);
    GateRef Int32Div(GateRef x, GateRef y);
    GateRef Int32Mod(GateRef x, GateRef y);
    GateRef DoubleMod(GateRef x, GateRef y);
    GateRef Int64Div(GateRef x, GateRef y);
    GateRef IntPtrDiv(GateRef x, GateRef y);
    GateRef IntPtrMod(GateRef x, GateRef y);
    GateRef Int32Min(GateRef x, GateRef y);
    GateRef Int32Max(GateRef x, GateRef y);
    // bit operation
    GateRef Int16Or(GateRef x, GateRef y);
    GateRef Int16And(GateRef x, GateRef y);
    GateRef Int32Or(GateRef x, GateRef y);
    GateRef Int8And(GateRef x, GateRef y);
    GateRef Int8Xor(GateRef x, GateRef y);
    GateRef Int32And(GateRef x, GateRef y);
    GateRef IntPtrAnd(GateRef x, GateRef y);
    GateRef BitAnd(GateRef x, GateRef y);
    GateRef BitOr(GateRef x, GateRef y);
    GateRef Int32Not(GateRef x);
    GateRef IntPtrNot(GateRef x);
    GateRef BoolNot(GateRef x);
    GateRef Int32Xor(GateRef x, GateRef y);
    GateRef FixLoadType(GateRef x);
    GateRef Int64Or(GateRef x, GateRef y);
    GateRef Int64FetchOr(GateRef x, GateRef y, MemoryAttribute mAttr);
    GateRef IntPtrOr(GateRef x, GateRef y);
    GateRef Int64And(GateRef x, GateRef y);
    GateRef Int64Xor(GateRef x, GateRef y);
    GateRef Int64Not(GateRef x);
    GateRef Int16LSL(GateRef x, GateRef y);
    GateRef Int32LSL(GateRef x, GateRef y);
    GateRef Int64LSL(GateRef x, GateRef y);
    GateRef IntPtrLSL(GateRef x, GateRef y);
    GateRef Int8LSR(GateRef x, GateRef y);
    GateRef Int16LSR(GateRef x, GateRef y);
    GateRef Int32LSR(GateRef x, GateRef y);
    GateRef Int64LSR(GateRef x, GateRef y);
    GateRef IntPtrLSR(GateRef x, GateRef y);
    GateRef Int32ASR(GateRef x, GateRef y);
    GateRef TaggedIsInt(GateRef x);
    GateRef TaggedIsDouble(GateRef x);
    GateRef TaggedIsObject(GateRef x);
    GateRef TaggedIsNumber(GateRef x);
    GateRef TaggedIsNumeric(GateRef glue, GateRef x);
    GateRef TaggedIsHole(GateRef x);
    GateRef TaggedIsNotHole(GateRef x);
    GateRef TaggedIsUndefined(GateRef x);
    GateRef TaggedIsException(GateRef x);
    GateRef TaggedIsSpecial(GateRef x);
    GateRef TaggedIsRegularObject(GateRef glue, GateRef x);
    GateRef TaggedIsHeapObject(GateRef x);
    GateRef TaggedIsAccessor(GateRef glue, GateRef x);
    GateRef TaggedIsInternalAccessor(GateRef glue, GateRef x);
    GateRef TaggedIsNativeModuleFailureInfo(GateRef glue, GateRef x);
    GateRef ObjectAddressToRange(GateRef x);
    GateRef RegionInSpace(GateRef region, RegionSpaceFlag space);
    GateRef RegionInSpace(GateRef region, RegionSpaceFlag spaceBegin, RegionSpaceFlag spaceEnd);
    GateRef InYoungGeneration(GateRef region);
    GateRef InGeneralOldGeneration(GateRef region);
    GateRef InSharedHeap(GateRef region);
    GateRef InSharedSweepableSpace(GateRef region);
    GateRef TaggedIsGeneratorObject(GateRef glue, GateRef x);
    GateRef TaggedIsJSArray(GateRef glue, GateRef x);
    GateRef TaggedIsJSProxy(GateRef glue, GateRef x);
    GateRef IsTaggedArray(GateRef glue, GateRef x);
    GateRef TaggedIsAsyncGeneratorObject(GateRef glue, GateRef x);
    GateRef TaggedIsJSGlobalObject(GateRef glue, GateRef x);
    GateRef TaggedIsWeak(GateRef x);
    GateRef TaggedIsPrototypeHandler(GateRef glue, GateRef x);
    GateRef TaggedIsStoreAOTHandler(GateRef glue, GateRef x);
    GateRef TaggedIsTransWithProtoHandler(GateRef glue, GateRef x);
    GateRef TaggedIsTransitionHandler(GateRef glue, GateRef x);
    GateRef TaggedIsString(GateRef glue, GateRef obj);
    GateRef TaggedIsStringIterator(GateRef glue, GateRef obj);
    GateRef TaggedIsSharedObj(GateRef glue, GateRef obj);
    GateRef BothAreString(GateRef glue, GateRef x, GateRef y);
    GateRef TaggedIsStringOrSymbol(GateRef glue, GateRef obj);
    GateRef TaggedIsSymbol(GateRef glue, GateRef obj);
    GateRef TaggedIsArrayIterator(GateRef glue, GateRef obj);
    GateRef TaggedIsArrayBuffer(GateRef glue, GateRef obj);
    GateRef TaggedIsProtoChangeMarker(GateRef glue, GateRef obj);
    GateRef GetNextPositionForHash(GateRef last, GateRef count, GateRef size);
    GateRef DoubleIsNAN(GateRef x);
    GateRef DoubleIsINF(GateRef x);
    GateRef DoubleIsNanOrInf(GateRef x);
    GateRef DoubleAbs(GateRef x);
    GateRef DoubleIsInteger(GateRef x);
    GateRef DoubleIsWithinInt32(GateRef x);
    GateRef DoubleTrunc(GateRef x);
    GateRef TaggedIsNull(GateRef x);
    GateRef TaggedIsNotNull(GateRef x);
    GateRef TaggedIsUndefinedOrNull(GateRef x);
    GateRef TaggedIsUndefinedOrNullOrHole(GateRef x);
    GateRef TaggedIsTrue(GateRef x);
    GateRef TaggedIsFalse(GateRef x);
    GateRef TaggedIsBoolean(GateRef x);
    GateRef TaggedIsNativePointer(GateRef glue, GateRef x);
    GateRef TaggedGetInt(GateRef x);
    GateRef NumberGetInt(GateRef glue, GateRef x);
    GateRef TaggedGetNumber(GateRef x);
    GateRef Int8ToTaggedInt(GateRef x);
    GateRef Int16ToTaggedInt(GateRef x);
    GateRef IntToTaggedPtr(GateRef x);
    GateRef IntToTaggedInt(GateRef x);
    GateRef Int64ToTaggedInt(GateRef x);
    GateRef Int64ToTaggedIntPtr(GateRef x);
    GateRef DoubleToTaggedDouble(GateRef x);
    GateRef DoubleToTaggedDoublePtr(GateRef x);
    GateRef BooleanToTaggedBooleanPtr(GateRef x);
    GateRef TaggedPtrToTaggedDoublePtr(GateRef x);
    GateRef TaggedPtrToTaggedIntPtr(GateRef x);
    GateRef CastDoubleToInt64(GateRef x);
    GateRef CastFloat32ToInt32(GateRef x);
    GateRef TaggedTrue();
    GateRef TaggedFalse();
    GateRef TaggedUndefined();
    GateRef Int64BitReverse(GateRef x);
    GateRef Int32BitReverse(GateRef x);
    GateRef Int16BitReverse(GateRef x);
    GateRef Int8BitReverse(GateRef x);
    // compare operation
    GateRef Int8Equal(GateRef x, GateRef y);
    GateRef Int8GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Equal(GateRef x, GateRef y);
    GateRef NotEqual(GateRef x, GateRef y);
    GateRef Int32Equal(GateRef x, GateRef y);
    GateRef Int32NotEqual(GateRef x, GateRef y);
    GateRef Int64Equal(GateRef x, GateRef y);
    GateRef DoubleEqual(GateRef x, GateRef y);
    GateRef DoubleNotEqual(GateRef x, GateRef y);
    GateRef Int64NotEqual(GateRef x, GateRef y);
    GateRef DoubleLessThan(GateRef x, GateRef y);
    GateRef DoubleLessThanOrEqual(GateRef x, GateRef y);
    GateRef DoubleGreaterThan(GateRef x, GateRef y);
    GateRef DoubleGreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int32GreaterThan(GateRef x, GateRef y);
    GateRef Int32LessThan(GateRef x, GateRef y);
    GateRef Int32GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int32LessThanOrEqual(GateRef x, GateRef y);
    GateRef Int32UnsignedGreaterThan(GateRef x, GateRef y);
    GateRef Int32UnsignedLessThan(GateRef x, GateRef y);
    GateRef Int32UnsignedGreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int32UnsignedLessThanOrEqual(GateRef x, GateRef y);
    GateRef Int64GreaterThan(GateRef x, GateRef y);
    GateRef Int64LessThan(GateRef x, GateRef y);
    GateRef Int64LessThanOrEqual(GateRef x, GateRef y);
    GateRef Int64GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int64UnsignedLessThanOrEqual(GateRef x, GateRef y);
    GateRef Int64UnsignedGreaterThan(GateRef x, GateRef y);
    GateRef Int64UnsignedGreaterThanOrEqual(GateRef x, GateRef y);
    GateRef IntPtrLessThan(GateRef x, GateRef y);
    GateRef IntPtrLessThanOrEqual(GateRef x, GateRef y);
    GateRef IntPtrGreaterThan(GateRef x, GateRef y);
    GateRef IntPtrGreaterThanOrEqual(GateRef x, GateRef y);
    // cast operation
    GateRef ChangeInt64ToIntPtr(GateRef val);
    GateRef ZExtInt32ToPtr(GateRef val);
    GateRef ChangeIntPtrToInt32(GateRef val);
    GateRef ToLength(GateRef glue, GateRef target);
    GateRef ToIndex(GateRef glue, GateRef tagged);

    // math operation
    GateRef Sqrt(GateRef x);
    GateRef GetSetterFromAccessor(GateRef glue, GateRef accessor);
    GateRef GetElementsArray(GateRef glue, GateRef object);
    void SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray,
                          MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef GetPropertiesArray(GateRef glue, GateRef object);
    // SetProperties in js_object.h
    void SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray,
                            MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef GetHash(GateRef glue, GateRef object);
    void SetHash(GateRef glue, GateRef object, GateRef hash);
    GateRef GetLengthOfTaggedArray(GateRef array);
    GateRef GetLengthOfJSTypedArray(GateRef array);
    GateRef GetExtraLengthOfTaggedArray(GateRef array);
    void SetExtraLengthOfTaggedArray(GateRef glue, GateRef array, GateRef len);
    // object operation
    GateRef IsJSHClass(GateRef glue, GateRef obj);
    GateRef LoadHclassImpl(GateRef glue, GateRef object, int line);

    void CanNotConvertNotValidObject(GateRef glue, GateRef obj);
    void IsNotValidObject(GateRef flag);
    void IsNotPropertyKey(GateRef obj);
    GateRef CreateDataProperty(GateRef glue, GateRef obj, GateRef proKey, GateRef value);
    GateRef CreateDataPropertyOrThrow(GateRef glue, GateRef onj, GateRef proKey, GateRef value);
    GateRef DefineField(GateRef glue, GateRef obj, GateRef proKey, GateRef value);
    void StoreHClass(GateRef glue, GateRef object, GateRef hClass,
                     MemoryAttribute mAttr = MemoryAttribute::NeedBarrier());
    void TransitionHClass(GateRef glue, GateRef object, GateRef hClass,
                          MemoryAttribute mAttr = MemoryAttribute::NeedBarrier());
    void StoreBuiltinHClass(GateRef glue, GateRef object, GateRef hClass);
    void StorePrototype(GateRef glue, GateRef hclass, GateRef prototype);
    void CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef scrHClass);
    void FuncOrHClassCompare(GateRef glue, GateRef funcOrHClass,
                             Label *match, Label *slowPath, size_t index);
    void GetIteratorResult(GateRef glue, Variable *result, GateRef obj,
                           Label *isPendingException, Label *noPendingException);
    void HClassCompareAndCheckDetector(GateRef glue, GateRef hclass,
                                       Label *match, Label *slowPath,
                                       size_t indexHClass, bool isMap);
    void TryFastGetArrayIterator(GateRef glue, GateRef hclass, GateRef jsType,
                                 Label *slowPath2, Label *matchArray);
    void TryFastGetIterator(GateRef glue, GateRef obj, GateRef hclass,
                            Variable &result, Label *slowPath, Label *exit,
                            Label *isPendingException);
    GateRef IsDetectorInvalid(GateRef glue, size_t indexDetector);
    GateRef GetObjectType(GateRef hClass);
    GateRef IsDictionaryMode(GateRef glue, GateRef object);
    GateRef IsDictionaryModeByHClass(GateRef hClass);
    GateRef IsDictionaryElement(GateRef hClass);
    GateRef IsJSArrayPrototypeModified(GateRef hClass);
    GateRef IsStableElements(GateRef hClass);
    GateRef HasConstructorByHClass(GateRef hClass);
    GateRef HasConstructor(GateRef glue, GateRef object);
    GateRef IsClassConstructorFromBitField(GateRef bitfield);
    GateRef IsClassConstructor(GateRef glue, GateRef object);
    GateRef IsClassPrototype(GateRef glue, GateRef object);
    GateRef IsExtensible(GateRef glue, GateRef object);
    GateRef TaggedObjectIsEcmaObject(GateRef glue, GateRef obj);
    GateRef IsEcmaObject(GateRef glue, GateRef obj);
    GateRef IsDataView(GateRef glue, GateRef obj);
    GateRef IsSymbol(GateRef glue, GateRef obj);
    GateRef IsString(GateRef glue, GateRef obj);
    GateRef IsLineString(GateRef glue, GateRef obj);
    GateRef IsSlicedString(GateRef glue, GateRef obj);
    GateRef IsTreeString(GateRef glue, GateRef obj);
    GateRef TreeStringIsFlat(GateRef glue, GateRef string);
    GateRef TaggedIsBigInt(GateRef glue, GateRef obj);
    GateRef TaggedIsPropertyBox(GateRef glue, GateRef obj);
    GateRef TaggedObjectIsBigInt(GateRef glue, GateRef obj);
    GateRef IsJsProxy(GateRef glue, GateRef obj);
    GateRef IsJSShared(GateRef glue, GateRef obj);
    GateRef IsProfileTypeInfoCell0(GateRef glue, GateRef obj);
    GateRef IsJSGlobalObject(GateRef glue, GateRef obj);
    GateRef IsNativeModuleFailureInfo(GateRef glue, GateRef obj);
    GateRef IsModuleNamespace(GateRef glue, GateRef obj);
    GateRef IsNativePointer(GateRef glue, GateRef obj);
    GateRef IsSourceTextModule(GateRef glue, GateRef obj);
    GateRef IsSpecialContainer(GateRef glue, GateRef obj);
    GateRef IsJSPrimitiveRef(GateRef glue, GateRef obj);
    GateRef IsJSFunctionBase(GateRef glue, GateRef obj);
    GateRef IsConstructor(GateRef glue, GateRef object);
    GateRef IsBase(GateRef glue, GateRef func);
    GateRef IsDerived(GateRef glue, GateRef func);
    GateRef IsJsArray(GateRef glue, GateRef obj);
    GateRef IsJsSArray(GateRef glue, GateRef obj);
    GateRef IsByteArray(GateRef glue, GateRef obj);
    GateRef IsJsCOWArray(GateRef glue, GateRef obj);
    GateRef IsCOWArray(GateRef glue, GateRef obj);
    GateRef IsMutantTaggedArray(GateRef glue, GateRef elements);
    GateRef IsJSObject(GateRef glue, GateRef obj);
    GateRef IsGlobalEnv(GateRef glue, GateRef obj);
    GateRef IsLexicalEnv(GateRef glue, GateRef obj);
    GateRef IsSFunctionEnv(GateRef glue, GateRef obj);
    GateRef IsEnumerable(GateRef attr);
    GateRef IsWritable(GateRef attr);
    GateRef IsConfigable(GateRef attr);
    GateRef IsDefaultAttribute(GateRef attr);
    GateRef IsArrayLengthWritable(GateRef glue, GateRef receiver);
    GateRef IsArrayLengthWritableForNonDictMode(GateRef glue, GateRef receiver);
    GateRef IsAccessor(GateRef attr);
    GateRef IsInlinedProperty(GateRef attr);
    GateRef IsField(GateRef attr);
    GateRef IsNonSharedStoreField(GateRef attr);
    GateRef IsStoreShared(GateRef attr);
    GateRef IsElement(GateRef attr);
    GateRef IsStringElement(GateRef attr);
    GateRef IsNumberHandler(GateRef attr);
    GateRef IsBooleanHandler(GateRef attr);
    GateRef IsStringLength(GateRef attr);
    GateRef IsTypedArrayElement(GateRef attr);
    GateRef IsNonExist(GateRef attr);
    GateRef IsJSAPIVector(GateRef glue, GateRef attr);
    GateRef IsJSAPIStack(GateRef glue, GateRef obj);
    GateRef IsJSAPIPlainArray(GateRef glue, GateRef obj);
    GateRef IsJSAPIQueue(GateRef glue, GateRef obj);
    GateRef IsJSAPIDeque(GateRef glue, GateRef obj);
    GateRef IsJSAPILightWeightMap(GateRef glue, GateRef obj);
    GateRef IsJSAPILightWeightSet(GateRef glue, GateRef obj);
    GateRef IsLinkedNode(GateRef glue, GateRef obj);
    GateRef IsJSAPIHashMap(GateRef glue, GateRef obj);
    GateRef IsJSAPIHashSet(GateRef glue, GateRef obj);
    GateRef IsJSAPILinkedList(GateRef glue, GateRef obj);
    GateRef IsJSAPIList(GateRef glue, GateRef obj);
    GateRef IsJSAPIArrayList(GateRef glue, GateRef obj);
    GateRef IsJSCollator(GateRef glue, GateRef obj);
    GateRef IsJSObjectType(GateRef glue, GateRef obj, JSType jsType);
    GateRef IsJSRegExp(GateRef glue, GateRef obj);
    GateRef GetTarget(GateRef glue, GateRef proxyObj);
    GateRef HandlerBaseIsAccessor(GateRef attr);
    GateRef HandlerBaseIsJSArray(GateRef attr);
    GateRef HandlerBaseIsInlinedProperty(GateRef attr);
    GateRef HandlerBaseGetOffset(GateRef attr);
    GateRef HandlerBaseGetAttrIndex(GateRef attr);
    GateRef HandlerBaseGetRep(GateRef attr);
    GateRef IsInvalidPropertyBox(GateRef glue, GateRef obj);
    GateRef IsAccessorPropertyBox(GateRef glue, GateRef obj);
    GateRef GetValueFromPropertyBox(GateRef glue, GateRef obj);
    void SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value);
    GateRef GetTransitionHClass(GateRef glue, GateRef obj);
    GateRef GetTransitionHandlerInfo(GateRef glue, GateRef obj);
    GateRef GetTransWithProtoHClass(GateRef glue, GateRef obj);
    GateRef GetTransWithProtoHandlerInfo(GateRef glue, GateRef obj);
    GateRef GetPrototypeHandlerProtoCell(GateRef glue, GateRef object);
    GateRef GetTransWithProtoHandlerProtoCell(GateRef glue, GateRef object);
    GateRef GetStoreAOTHandlerProtoCell(GateRef glue, GateRef object);
    GateRef GetPrototypeHandlerHolder(GateRef glue, GateRef object);
    GateRef GetPrototypeHandlerHandlerInfo(GateRef glue, GateRef object);
    void SetPrototypeHandlerHandlerInfo(GateRef glue, GateRef obj, GateRef value);
    GateRef GetStoreAOTHandlerHolder(GateRef glue, GateRef object);
    GateRef GetStoreAOTHandlerHandlerInfo(GateRef glue, GateRef object);
    inline GateRef GetLengthOfJSArray(GateRef array);
    inline GateRef GetPrototype(GateRef glue, GateRef object);
    inline GateRef GetHasChanged(GateRef object);
    inline GateRef GetNotFoundHasChanged(GateRef object);
    GateRef HclassIsPrototypeHandler(GateRef hClass);
    GateRef HclassIsTransitionHandler(GateRef hClass);
    GateRef HclassIsPropertyBox(GateRef hClass);
    GateRef PropAttrGetOffset(GateRef attr);
    GateRef GetCtorPrototype(GateRef glue, GateRef ctor);
    GateRef HasFunctionPrototype(GateRef glue, GateRef ctor);
    GateRef InstanceOf(GateRef glue, GateRef object, GateRef target, GateRef profileTypeInfo, GateRef slotId,
        ProfileOperation callback);
    GateRef OrdinaryHasInstance(GateRef glue, GateRef target, GateRef obj);
    void TryFastHasInstance(GateRef glue, GateRef instof, GateRef target, GateRef object, Label *fastPath,
                            Label *exit, Variable *result, ProfileOperation callback);
    GateRef ConvertTaggedValueWithElementsKind(GateRef glue, GateRef value, GateRef extraKind);
    GateRef SameValue(GateRef glue, GateRef left, GateRef right);
    GateRef SameValueZero(GateRef glue, GateRef left, GateRef right);
    GateRef IsStableJSArguments(GateRef glue, GateRef obj);
    GateRef IsStableJSArray(GateRef glue, GateRef obj);
    GateRef IsTypedArray(GateRef glue, GateRef obj);
    GateRef IsSharedTypedArray(GateRef glue, GateRef obj);
    GateRef IsStableArguments(GateRef hClass);
    GateRef IsStableArray(GateRef hClass);
    GateRef GetProfileTypeInfo(GateRef glue, GateRef jsFunc);
    GateRef UpdateProfileTypeInfo(GateRef glue, GateRef jsFunc);
    // SetDictionaryOrder func in property_attribute.h
    GateRef SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value);
    GateRef GetPrototypeFromHClass(GateRef glue, GateRef hClass);
    GateRef GetEnumCacheFromHClass(GateRef glue, GateRef hClass);
    GateRef GetProtoChangeMarkerFromHClass(GateRef glue, GateRef hClass);
    GateRef GetLayoutFromHClass(GateRef glue, GateRef hClass);
    GateRef GetBitFieldFromHClass(GateRef hClass);
    GateRef GetArrayElementsGuardians(GateRef env);
    GateRef GetTypedArraySpeciesProtectDetector(GateRef env);
    GateRef GetArrayIteratorDetector(GateRef env);
    GateRef GetMapIteratorDetector(GateRef env);
    GateRef GetSetIteratorDetector(GateRef env);
    void SetArrayElementsGuardians(GateRef glue, GateRef env, GateRef value);
    GateRef GetLengthFromString(GateRef value);
    GateRef CalcHashcodeForInt(GateRef value);
    void CalcHashcodeForDouble(GateRef value, Variable *res, Label *exit);
    void CalcHashcodeForObject(GateRef glue, GateRef value, Variable *res, Label *exit);
    GateRef GetHashcodeFromString(GateRef glue, GateRef value, GateRef hir = Circuit::NullGate());
    inline void SetMixHashcode(GateRef glue, GateRef str, GateRef mixHashcode);
    GateRef TryGetHashcodeFromString(GateRef string);
    GateRef GetFirstFromTreeString(GateRef glue, GateRef string);
    GateRef GetSecondFromTreeString(GateRef glue, GateRef string);
    GateRef GetIsAllTaggedPropFromHClass(GateRef hclass);
    void SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield);
    void SetIsAllTaggedProp(GateRef glue, GateRef hclass, GateRef hasRep);
    void SetPrototypeToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef proto);
    GateRef GetProtoChangeDetails(GateRef glue, GateRef hClass);
    void SetProtoChangeDetailsToHClass(VariableType type, GateRef glue, GateRef hClass,
                                       GateRef protoChange);
    void SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr,
                           MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetHClassTypeIDToHClass(GateRef glue, GateRef hClass, GateRef id);
    void SetEnumCacheToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef key);
    void SetDependentInfosToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef value);
    void SetTransitionsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef transition);
    GateRef GetTransitionsFromHClass(GateRef glue, GateRef hClass);
    void SetParentToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef parent);
    void SetIsPrototypeToHClass(GateRef glue, GateRef hClass, GateRef value);
    GateRef SetIsStableToBitField(GateRef bitfield, GateRef isStable);
    inline void SetIsAOT(GateRef glue, GateRef hClass, GateRef value);
    GateRef IsPrototypeHClass(GateRef hClass);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
                                 GateRef value, GateRef attrOffset, VariableType type = VariableType::JS_ANY(),
                                 MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef GetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
        GateRef index);
    GateRef GetInlinedPropOffsetFromHClass(GateRef hclass, GateRef attrOffset);
    inline GateRef IsObjSizeTrackingInProgress(GateRef hclass);
    inline GateRef GetConstructionCounter(GateRef hclass);
    inline void SetConstructionCounter(GateRef glue, GateRef hclass, GateRef count);

    void IncNumberOfProps(GateRef glue, GateRef hClass);
    GateRef GetNumberOfPropsFromHClass(GateRef hClass);
    GateRef HasDeleteProperty(GateRef hClass);
    GateRef IsAOTHClass(GateRef hClass);
    void SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value);
    void SetElementsKindToTrackInfo(GateRef glue, GateRef trackInfo, GateRef elementsKind);
    void SetSpaceFlagToTrackInfo(GateRef glue, GateRef trackInfo, GateRef spaceFlag);
    GateRef GetElementsKindFromHClass(GateRef hClass);
    GateRef GetObjectSizeFromHClass(GateRef hClass);
    GateRef GetInlinedPropsStartFromHClass(GateRef hClass);
    GateRef GetInlinedPropertiesFromHClass(GateRef hClass);
    void ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val);
    GateRef GetValueFromTaggedArray(GateRef glue, GateRef elements, GateRef index);
    GateRef GetDataPtrInTaggedArray(GateRef array);
    GateRef GetDataPtrInTaggedArray(GateRef array, GateRef index);
    GateRef GetUnsharedConstpoolIndex(GateRef glue, GateRef constpool);
    GateRef GetUnsharedConstpoolFromGlue(GateRef glue, GateRef constpool);
    GateRef GetUnsharedConstpool(GateRef glue, GateRef array, GateRef index);
    GateRef GetValueFromMutantTaggedArray(GateRef elements, GateRef index);
    void SharedObjectStoreBarrierWithTypeCheck(bool isDicMode, Variable *result, GateRef glue, GateRef attr,
        GateRef value, Variable *newValue, Label *executeSharedSetProp, Label *exit);
    void SharedObjectStoreBarrierWithTypeCheck(bool isDicMode, Variable *result, GateRef glue, GateRef attr,
        GateRef value, Variable *newValue, Label *executeSharedSetProp, Label *exit, GateRef SCheckModelIsCHECK);
    void SharedObjectStoreBarrierWithTypeCheck(Variable *result, GateRef glue, GateRef fieldType, GateRef value,
        Variable *newValue, Label *executeSharedSetProp, Label *exit);
    void SharedObjectStoreBarrier(GateRef glue, GateRef value, Variable *newValue, Label *exit);
    GateRef GetFieldTypeFromHandler(GateRef attr);
    GateRef ClearSharedStoreKind(GateRef handlerInfo);
    GateRef UpdateSOutOfBoundsForHandler(GateRef handlerInfo);
    void RestoreElementsKindToGeneric(GateRef glue, GateRef jsHClass);
    GateRef GetTaggedValueWithElementsKind(GateRef glue, GateRef receiver, GateRef index);
    void FastSetValueWithElementsKind(GateRef glue, GateRef receiver, GateRef elements, GateRef rawValue,
                                      GateRef index, ElementsKind kind, bool needTransition = false);
    GateRef SetValueWithElementsKind(GateRef glue, GateRef receiver, GateRef rawValue, GateRef index,
                                     GateRef needTransition, GateRef extraKind);
    GateRef CopyJSArrayToTaggedArrayArgs(GateRef glue, GateRef srcObj);
    void SetValueToTaggedArrayWithAttr(
        GateRef glue, GateRef array, GateRef index, GateRef key, GateRef val, GateRef attr);
    void SetValueToTaggedArrayWithRep(
        GateRef glue, GateRef array, GateRef index, GateRef val, GateRef rep, Label *repChange);

    void SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val,
                               MemoryAttribute mAttr = MemoryAttribute::Default());
    void UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index, GateRef value, GateRef attr);
    GateRef IsSpecialIndexedObj(GateRef jsType);
    GateRef IsArrayListOrVector(GateRef jsType);
    GateRef IsSharedArray(GateRef jsType);
    GateRef IsAccessorInternal(GateRef glue, GateRef value);
    template<typename DictionaryT>
    GateRef GetAttributesFromDictionary(GateRef glue, GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetValueFromDictionary(GateRef glue, GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetKeyFromDictionary(GateRef glue, GateRef elements, GateRef entry);
    GateRef GetPropAttrFromLayoutInfo(GateRef glue, GateRef layout, GateRef entry);
    void UpdateFieldType(GateRef glue, GateRef hclass, GateRef attr);
    GateRef GetPropertiesAddrFromLayoutInfo(GateRef layout);
    GateRef GetPropertyMetaDataFromAttr(GateRef attr);
    GateRef TranslateToRep(GateRef value);
    GateRef GetKeyFromLayoutInfo(GateRef glue, GateRef layout, GateRef entry);
    void MatchFieldType(GateRef glue, GateRef fieldType, GateRef value, Label *executeSetProp, Label *typeMismatch);
    GateRef FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
        GateRef key, GateRef propsNum, GateRef hir = Circuit::NullGate());
    GateRef FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index);
    template<typename HashTableT>
    GateRef FindEntryFromHashTable(GateRef glue, GateRef elements, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr);
    GateRef FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData);
    GateRef JSObjectHasProperty(GateRef glue, GateRef obj, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef JSObjectGetPropertyWithRep(GateRef glue, GateRef obj, GateRef hClass, GateRef propAttr);
    void JSObjectSetProperty(GateRef glue, GateRef obj, GateRef hClass, GateRef attr, GateRef key, GateRef value);
    GateRef ShouldCallSetter(GateRef glue, GateRef receiver, GateRef holder, GateRef accessor, GateRef attr);
    GateRef CallSetterHelper(GateRef glue,
                             GateRef holder,
                             GateRef accessor,
                             GateRef value,
                             ProfileOperation callback,
                             bool mayThrow = true);
    GateRef AddPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef propertyAttributes,
        ProfileOperation callback);
    GateRef IsUtf16String(GateRef string);
    GateRef IsUtf8String(GateRef string);
    GateRef IsInternalString(GateRef string);
    GateRef IsDigit(GateRef ch);
    GateRef ComputeElementCapacity(GateRef oldLength);
    GateRef ComputeNonInlinedFastPropsCapacity(GateRef glue, GateRef oldLength,
                                               GateRef maxNonInlinedFastPropsCapacity);
    GateRef FindTransitions(GateRef glue, GateRef hClass, GateRef key, GateRef attr, GateRef value);
    GateRef CheckHClassForRep(GateRef glue, GateRef hClass, GateRef rep);
    void TransitionForRepChange(GateRef glue, GateRef receiver, GateRef key, GateRef attr);
    void TransitToElementsKind(GateRef glue, GateRef receiver, GateRef value, GateRef kind);
    void TryMigrateToGenericKindForJSObject(GateRef glue, GateRef receiver, GateRef oldKind);
    GateRef TaggedToRepresentation(GateRef value);
    GateRef TaggedToElementKind(GateRef glue, GateRef value);
    GateRef LdGlobalRecord(GateRef glue, GateRef key);
    GateRef LoadFromField(GateRef glue, GateRef receiver, GateRef handlerInfo);
    GateRef LoadGlobal(GateRef glue, GateRef cell);
    GateRef LoadElement(GateRef glue, GateRef receiver, GateRef key);
    GateRef LoadStringElement(GateRef glue, GateRef receiver, GateRef key);
    GateRef TryToElementsIndex(GateRef glue, GateRef key);
    GateRef CheckPolyHClass(GateRef glue, GateRef cachedValue, GateRef hClass);
    GateRef LoadICWithHandler(
        GateRef glue, GateRef receiver, GateRef holder, GateRef handler, ProfileOperation callback);
    GateRef StoreICWithHandler(GateRef glue, GateRef receiver, GateRef holder,
                               GateRef value, GateRef handler, ProfileOperation callback = ProfileOperation());
    GateRef TaggedArraySetValue(GateRef glue, GateRef receiver, GateRef value, GateRef index, GateRef capacity);
    GateRef ICStoreElement(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef handlerInfo,
                           bool updateHandler = false, GateRef profileTypeInfo = Gate::InvalidGateRef,
                           GateRef slotId = Gate::InvalidGateRef);
    GateRef GetArrayLength(GateRef object);
    GateRef DoubleToInt(GateRef glue, GateRef x, size_t bits = base::INT32_BITS);
    void SetArrayLength(GateRef glue, GateRef object, GateRef len);
    GateRef StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler, ProfileOperation callback);
    GateRef StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler,
                             ProfileOperation callback, bool withPrototype = false);
    GateRef StoreGlobal(GateRef glue, GateRef value, GateRef cell);
    void JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr, GateRef value);
    void NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass);
    GateRef GetInt64OfTInt(GateRef x);
    GateRef GetInt32OfTInt(GateRef x);
    GateRef GetDoubleOfTInt(GateRef x);
    GateRef GetDoubleOfTDouble(GateRef x);
    GateRef GetInt32OfTNumber(GateRef x);
    GateRef GetDoubleOfTNumber(GateRef x);
    GateRef LoadObjectFromWeakRef(GateRef x);
    GateRef ExtFloat32ToDouble(GateRef x);
    GateRef ChangeInt32ToFloat32(GateRef x);
    GateRef ChangeInt32ToFloat64(GateRef x);
    GateRef ChangeUInt32ToFloat64(GateRef x);
    GateRef ChangeFloat64ToInt32(GateRef x);
    GateRef TruncDoubleToFloat32(GateRef x);
    GateRef DeletePropertyOrThrow(GateRef glue, GateRef obj, GateRef value);
    inline GateRef ToObject(GateRef glue, GateRef globalEnv, GateRef obj);
    GateRef DeleteProperty(GateRef glue, GateRef obj, GateRef value);
    inline GateRef OrdinaryNewJSObjectCreate(GateRef glue, GateRef proto);
    inline GateRef NewJSPrimitiveRef(GateRef glue, GateRef globalEnv, size_t index, GateRef obj);
    GateRef ModuleNamespaceDeleteProperty(GateRef glue, GateRef obj, GateRef value);
    GateRef Int64ToTaggedPtr(GateRef x);
    GateRef TruncInt16ToInt8(GateRef x);
    GateRef TruncInt32ToInt16(GateRef x);
    GateRef TruncInt32ToInt8(GateRef x);
    GateRef TruncFloatToInt64(GateRef x);
    GateRef CastInt32ToFloat32(GateRef x);
    GateRef CastInt64ToFloat64(GateRef x);
    GateRef SExtInt32ToInt64(GateRef x);
    GateRef SExtInt16ToInt64(GateRef x);
    GateRef SExtInt16ToInt32(GateRef x);
    GateRef SExtInt8ToInt64(GateRef x);
    GateRef SExtInt8ToInt32(GateRef x);
    GateRef SExtInt1ToInt64(GateRef x);
    GateRef SExtInt1ToInt32(GateRef x);
    GateRef ZExtInt8ToInt16(GateRef x);
    GateRef ZExtInt32ToInt64(GateRef x);
    GateRef ZExtInt1ToInt64(GateRef x);
    GateRef ZExtInt1ToInt32(GateRef x);
    GateRef ZExtInt8ToInt32(GateRef x);
    GateRef ZExtInt8ToInt64(GateRef x);
    GateRef ZExtInt8ToPtr(GateRef x);
    GateRef ZExtInt16ToPtr(GateRef x);
    GateRef SExtInt32ToPtr(GateRef x);
    GateRef ZExtInt16ToInt32(GateRef x);
    GateRef ZExtInt16ToInt64(GateRef x);
    GateRef TruncInt64ToInt32(GateRef x);
    GateRef TruncInt64ToInt16(GateRef x);
    GateRef TruncPtrToInt32(GateRef x);
    GateRef TruncInt64ToInt1(GateRef x);
    GateRef TruncInt32ToInt1(GateRef x);
    GateRef GetGlobalConstantAddr(GateRef index);
    GateRef GetGlobalConstantOffset(ConstantIndex index);
    GateRef IsCallableFromBitField(GateRef bitfield);
    GateRef IsCallable(GateRef glue, GateRef obj);
    GateRef TaggedIsCallable(GateRef glue, GateRef obj);
    GateRef GetOffsetFieldInPropAttr(GateRef attr);
    GateRef SetOffsetFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetTrackTypeInPropAttr(GateRef attr, GateRef type);
    GateRef GetTrackTypeInPropAttr(GateRef attr);
    GateRef GetSharedFieldTypeInPropAttr(GateRef attr);
    GateRef GetDictSharedFieldTypeInPropAttr(GateRef attr);
    GateRef GetRepInPropAttr(GateRef attr);
    GateRef IsIntRepInPropAttr(GateRef attr);
    GateRef IsDoubleRepInPropAttr(GateRef attr);
    GateRef IsTaggedRepInPropAttr(GateRef attr);
    GateRef SetTaggedRepInPropAttr(GateRef attr);
    GateRef SetEnumerableFiledInPropAttr(GateRef attr, GateRef value);
    GateRef SetWritableFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetConfigurableFieldInPropAttr(GateRef attr, GateRef value);
    template<class T>
    void SetHClassBit(GateRef glue, GateRef hClass, GateRef value);
    template<typename DictionaryT>
    void UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value);
    GateRef GetBitMask(GateRef bitoffset);
    GateRef IntPtrEuqal(GateRef x, GateRef y);
    GateRef IntPtrNotEqual(GateRef x, GateRef y);
    void SetValueWithAttr(GateRef glue, GateRef obj, GateRef offset, GateRef key, GateRef value, GateRef attr);
    void SetValueWithRep(GateRef glue, GateRef obj, GateRef offset, GateRef value, GateRef rep, Label *repChange);
    void VerifyBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value);
    GateRef GetCMCRegionRSet(GateRef obj);
    GateRef GetCMCRegionType(GateRef obj);
    GateRef GetGCPhase(GateRef glue);
    GateRef GetGCReason(GateRef glue);
    GateRef CMCIsInYoungSpace(GateRef regionType);
    GateRef IsOldToYoung(GateRef objRegionType, GateRef valueRegionType);
    void MarkRSetCardTable(GateRef obj, Label *exit);
    GateRef ShouldGetGCReason(GateRef gcPhase);
    GateRef ShouldProcessSATB(GateRef gcPhase);
    GateRef ShouldUpdateRememberSet(GateRef glue, GateRef gcPhase);
    GateRef NeedSkipReadBarrier(GateRef glue);
    void CMCSetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value);
    void CMCArrayCopyWriteBarrier(GateRef glue, GateRef dstObj, GateRef src, GateRef dst, GateRef count);
    void CMCArrayCopyWriteBarrierSameArray(GateRef glue, GateRef dstObj, GateRef src, GateRef dst, GateRef count);
    void SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value,
                             MemoryAttribute::ShareFlag share = MemoryAttribute::UNKNOWN);
    GateRef GetValueWithBarrier(GateRef glue, GateRef addr);
    GateRef FastReadBarrier(GateRef glue, GateRef addr, GateRef value);
    GateRef IsHeapAddress(GateRef glue, GateRef value);
    GateRef GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index,
                               ProfileOperation callback, GateRef hir = Circuit::NullGate());
                               GateRef GetPropertyByName(GateRef glue,
                                GateRef target,
                                GateRef propKey,
                                GateRef receiver = Circuit::NullGate(),
                                ProfileOperation callback = ProfileOperation(),
                                GateRef isInternal = Circuit::NullGate(),
                                bool canUseIsInternal = false,
                                GateRef hir = Circuit::NullGate());
    void CallGetterIfAccessor(GateRef glue, GateRef holder, Variable *value, Variable *attr,
                              Label *isFoundData, Label *isFoundAccessor);
    void TryGetOwnProperty(GateRef glue, GateRef holder, GateRef key, GateRef hir,
                           Variable *rValue, Variable *rAttr, Label *isFoundData, Label *isFoundAccessor,
                           Label *notFound, Label *callRuntime);
    GateRef FastGetPropertyByName(GateRef glue, GateRef obj, GateRef key,
                                    ProfileOperation callback, GateRef hir = Circuit::NullGate());
    GateRef FastGetPropertyByIndex(GateRef glue, GateRef obj, GateRef index,
                                   ProfileOperation callback, GateRef hir = Circuit::NullGate());
    GateRef GetPropertyByValue(GateRef glue,
                               GateRef target,
                               GateRef propKey,
                               GateRef receiver = Circuit::NullGate(),
                               ProfileOperation callback = ProfileOperation());
    void FastSetPropertyByName(GateRef glue, GateRef obj, GateRef key, GateRef value,
        ProfileOperation callback = ProfileOperation());
    void FastSetPropertyByIndex(GateRef glue, GateRef obj, GateRef index, GateRef value);
    GateRef SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value, bool useOwn,
                               ProfileOperation callback = ProfileOperation(), bool defineSemantics = false,
                               bool mayThrow = true);
    GateRef DefinePropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value);
    GateRef SetPropertyByName(GateRef glue,
                              GateRef receiver,
                              GateRef key,
                              GateRef value,
                              bool useOwn,
                              GateRef isInternal,
                              ProfileOperation callback = ProfileOperation(),
                              bool canUseIsInternal = false,
                              bool defineSemantics = false,
                              bool mayThrow = true); // Crawl prototype chain
    GateRef DefinePropertyByName(GateRef glue, GateRef receiver, GateRef key,
        GateRef value, GateRef isInternal, GateRef SCheckModelIsCHECK,
        ProfileOperation callback = ProfileOperation());
    GateRef SetPropertyByValue(GateRef glue,
                               GateRef receiver,
                               GateRef key,
                               GateRef value,
                               bool useOwn,
                               ProfileOperation callback = ProfileOperation(),
                               bool defineSemantics = false,
                               bool mayThrow = true);
    GateRef DefinePropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value,
        GateRef SCheckModelIsCHECK, ProfileOperation callback = ProfileOperation());
    GateRef GetParentEnv(GateRef glue, GateRef object);
    GateRef GetSendableParentEnv(GateRef glue, GateRef object);
    GateRef GetPropertiesFromLexicalEnv(GateRef glue, GateRef object, GateRef index);
    GateRef GetPropertiesFromSendableEnv(GateRef glue, GateRef object, GateRef index);
    GateRef GetKeyFromLexivalEnv(GateRef glue, GateRef lexicalEnv, GateRef levelIndex, GateRef slotIndex);
    void SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    void SetPropertiesToSendableEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    GateRef GetHomeObjectFromJSFunction(GateRef glue, GateRef object);
    GateRef GetCallFieldFromMethod(GateRef method);
    GateRef GetSendableEnvFromModule(GateRef glue, GateRef module);
    GateRef GetProtoOrHClass(GateRef glue, GateRef function);
    GateRef GetViewedArrayBufferOrByteArray(GateRef glue, GateRef typedArray);
    GateRef IsSendableFunctionModule(GateRef glue, GateRef module);
    GateRef GetFunctionLexicalEnv(GateRef glue, GateRef function);
    inline GateRef GetBuiltinId(GateRef method);
    void SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv,
                                 MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetProtoOrHClassToFunction(GateRef glue, GateRef function, GateRef value,
                                    MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetWorkNodePointerToFunction(GateRef glue, GateRef function, GateRef value,
                                      MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value,
                                 MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetModuleToFunction(GateRef glue, GateRef function, GateRef value,
                             MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetMethodToFunction(GateRef glue, GateRef function, GateRef value,
                             MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetCodeEntryToFunctionFromMethod(GateRef glue, GateRef function, GateRef value);
    void SetCodeEntryToFunctionFromFuncEntry(GateRef glue, GateRef function, GateRef value);
    void SetNativePointerToFunctionFromMethod(GateRef glue, GateRef function, GateRef value);
    void SetCompiledCodeFlagToFunctionFromMethod(GateRef glue, GateRef function, GateRef value);
    void SetLengthToFunction(GateRef glue, GateRef function, GateRef value);
    void SetRawProfileTypeInfoToFunction(GateRef glue, GateRef function, GateRef value,
                                         MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetValueToProfileTypeInfoCell(GateRef glue, GateRef profileTypeInfoCell, GateRef value);
    void UpdateProfileTypeInfoCellType(GateRef glue, GateRef profileTypeInfoCell);
    void SetJSObjectTaggedField(GateRef glue, GateRef object, size_t offset, GateRef value);
    void SetSendableEnvToModule(GateRef glue, GateRef module, GateRef value,
                                MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetCompiledCodeFlagToFunction(GateRef glue, GateRef function, GateRef value);
    void SetCompiledFastCallFlagToFunction(GateRef glue, GateRef function, GateRef value);
    void SetCompiledFuncEntry(GateRef glue, GateRef jsFunc, GateRef codeEntry, GateRef isFastCall);
    GateRef GetFuncEntryDes(GateRef glue, GateRef machineCode, GateRef codeAddr);
    GateRef GetFuncEntryDesAddress(GateRef machineCode);
    GateRef IsAlign(GateRef address, GateRef alignByte);
    void SetTaskConcurrentFuncFlagToFunction(GateRef glue, GateRef function, GateRef value);
    void SetBitFieldToFunction(GateRef glue, GateRef function, GateRef value,
                               MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetMachineCodeToFunction(GateRef glue, GateRef function, GateRef value,
                                  MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetBaselineJitCodeToFunction(GateRef glue, GateRef function, GateRef value,
                                      MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetTypedArrayName(GateRef glue, GateRef typedArray, GateRef name,
                           MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetContentType(GateRef glue, GateRef typedArray, GateRef type);
    void SetViewedArrayBufferOrByteArray(GateRef glue, GateRef typedArray, GateRef data,
                                         MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetByteLength(GateRef glue, GateRef typedArray, GateRef byteLength);
    void SetByteOffset(GateRef glue, GateRef typedArray, GateRef offset);
    void SetTypedArrayLength(GateRef glue, GateRef typedArray, GateRef arrayLength);
    GateRef GetGlobalEnv(GateRef glue);
    GateRef GetGlobalObject(GateRef glue, GateRef globalEnv);
    GateRef GetCurrentGlobalEnv(GateRef glue, GateRef currentEnv);
    void SetGlueGlobalEnv(GateRef glue, GateRef globalEnv);
    void SetGlueGlobalEnvFromCurrentEnv(GateRef glue, GateRef currentEnv);
    GateRef GetMethodFromFunction(GateRef glue, GateRef function);
    GateRef GetModuleFromFunction(GateRef glue, GateRef function);
    GateRef GetLengthFromFunction(GateRef function);
    GateRef GetHomeObjectFromFunction(GateRef glue, GateRef function);
    GateRef GetEntryIndexOfGlobalDictionary(GateRef entry);
    GateRef GetBoxFromGlobalDictionary(GateRef glue, GateRef object, GateRef entry);
    GateRef GetValueFromGlobalDictionary(GateRef glue, GateRef object, GateRef entry);
    GateRef GetPropertiesFromJSObject(GateRef glue, GateRef object);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOpWithOverflow(GateRef x, GateRef y);
    GateRef GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key, ProfileOperation callback);
    GateRef AddElementInternal(GateRef glue, GateRef receiver, GateRef index, GateRef value, GateRef attr);
    GateRef ShouldTransToDict(GateRef capcity, GateRef index);
    void NotifyArrayPrototypeChangedGuardians(GateRef glue, GateRef receiver);
    GateRef GrowElementsCapacity(GateRef glue, GateRef receiver, GateRef capacity);

    inline GateRef GetObjectFromConstPool(GateRef glue, GateRef constpool, GateRef index);
    GateRef GetConstPoolFromFunction(GateRef glue, GateRef jsFunc);
    GateRef GetStringFromConstPool(GateRef glue, GateRef constpool, GateRef index);
    GateRef GetMethodFromConstPool(GateRef glue, GateRef constpool, GateRef index);
    GateRef GetArrayLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module);
    GateRef GetObjectLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module);
    void SetElementsKindToJSHClass(GateRef glue, GateRef jsHclass, GateRef elementsKind);
    void SetExtensibleToBitfield(GateRef glue, GateRef obj, bool isExtensible);
    void SetCallableToBitfield(GateRef glue, GateRef obj, bool isCallable);

    // fast path
    GateRef FastEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastStrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastStringEqual(GateRef glue, GateRef left, GateRef right);
    GateRef FastStringEqualWithoutRTStub(GateRef glue, GateRef left, GateRef right);
    GateRef StringCompare(GateRef glue, GateRef left, GateRef right);
    GateRef FastMod(GateRef gule, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastTypeOf(GateRef left, GateRef right);
    GateRef FastMul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastDiv(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastAdd(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastSub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastToBoolean(GateRef glue, GateRef value, bool flag = true);
    GateRef FastToBooleanWithProfile(GateRef glue, GateRef value, ProfileOperation callback, bool flag = true);
    GateRef FastToBooleanWithProfileBaseline(GateRef glue, GateRef value, ProfileOperation callback, bool flag = true);

    // Add SpecialContainer
    GateRef JSAPIContainerGet(GateRef glue, GateRef receiver, GateRef index);

    // for-in
    GateRef NextInternal(GateRef glue, GateRef iter);
    GateRef GetCacheKindFromForInIterator(GateRef iter);
    GateRef GetLengthFromForInIterator(GateRef iter);
    GateRef GetIndexFromForInIterator(GateRef iter);
    GateRef GetKeysFromForInIterator(GateRef glue, GateRef iter);
    GateRef GetObjectFromForInIterator(GateRef glue, GateRef iter);
    GateRef GetCachedHClassFromForInIterator(GateRef glue, GateRef iter);
    void SetLengthOfForInIterator(GateRef glue, GateRef iter, GateRef length);
    void SetIndexOfForInIterator(GateRef glue, GateRef iter, GateRef index);
    void SetCacheKindForInIterator(GateRef glue, GateRef iter, GateRef cacheKind);
    void SetKeysOfForInIterator(GateRef glue, GateRef iter, GateRef keys);
    void SetObjectOfForInIterator(GateRef glue, GateRef iter, GateRef object);
    void SetCachedHClassOfForInIterator(GateRef glue, GateRef iter, GateRef hclass);
    void IncreaseIteratorIndex(GateRef glue, GateRef iter, GateRef index);
    void SetNextIndexOfArrayIterator(GateRef glue, GateRef iter, GateRef nextIndex);
    void IncreaseArrayIteratorIndex(GateRef glue, GateRef iter, GateRef index);
    void SetIteratedArrayOfArrayIterator(GateRef glue, GateRef iter, GateRef iteratedArray);
    void SetBitFieldOfArrayIterator(GateRef glue, GateRef iter, GateRef kind);
    GateRef GetArrayIterationKind(GateRef iter);
    GateRef GetEnumCacheKindFromEnumCache(GateRef enumCache);
    GateRef GetEnumCacheOwnFromEnumCache(GateRef glue, GateRef enumCache);
    GateRef GetEnumCacheAllFromEnumCache(GateRef glue, GateRef enumCache);
    GateRef GetProtoChainInfoEnumCacheFromEnumCache(GateRef glue, GateRef enumCache);
    GateRef GetEmptyArray(GateRef glue);
    GateRef IsEnumCacheValid(GateRef glue, GateRef receiver, GateRef cachedHclass, GateRef kind);
    GateRef NeedCheckProperty(GateRef glue, GateRef receiver);

    GateRef EnumerateObjectProperties(GateRef glue, GateRef obj);
    GateRef GetOrCreateEnumCacheFromHClass(GateRef glue, GateRef hClass);
    GateRef GetFunctionPrototype(GateRef glue, size_t index);
    GateRef ToPrototypeOrObj(GateRef glue, GateRef obj);
    GateRef ToPropertyKey(GateRef glue, GateRef tagged);
    GateRef TaggedIsEnumCache(GateRef glue, GateRef obj);
    GateRef TaggedIsPropertyKey(GateRef glue, GateRef obj);
    GateRef HasProperty(GateRef glue, GateRef obj, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef IsIn(GateRef glue, GateRef prop, GateRef obj);
    GateRef IsSpecialKeysObject(GateRef glue, GateRef obj);
    GateRef IsSlowKeysObject(GateRef glue, GateRef obj);
    GateRef IsSimpleEnumCacheValid(GateRef glue, GateRef obj);
    GateRef IsProtoChainCacheValid(GateRef glue, GateRef obj);
    GateRef TryGetEnumCache(GateRef glue, GateRef obj);
    GateRef GetNumberOfElements(GateRef glue, GateRef obj);
    GateRef IsEnumCacheWithProtoChainInfoValid(GateRef glue, GateRef obj);

    // Exception handle
    GateRef HasPendingException(GateRef glue);
    void ReturnExceptionIfAbruptCompletion(GateRef glue);

    // ElementsKind Operations
    GateRef ValueIsSpecialHole(GateRef x);
    GateRef ElementsKindIsInt(GateRef kind);
    GateRef ElementsKindIsIntOrHoleInt(GateRef kind);
    GateRef ElementsKindIsNumber(GateRef kind);
    GateRef ElementsKindIsNumOrHoleNum(GateRef kind);
    GateRef ElementsKindIsString(GateRef kind);
    GateRef ElementsKindIsStringOrHoleString(GateRef kind);
    GateRef ElementsKindIsHeapKind(GateRef kind);
    GateRef ElementsKindHasHole(GateRef kind);
    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    void ArrayCopyAndHoleToUndefined(GateRef glue, GateRef srcObj, GateRef srcAddr, GateRef dstObj,
                                     GateRef dstAddr, GateRef length, GateRef needBarrier);
    GateRef ThreeInt64Min(GateRef first, GateRef second, GateRef third);
    void MigrateArrayWithKind(GateRef glue, GateRef object, GateRef oldKind, GateRef newKind);
    GateRef MigrateFromRawValueToHeapValues(GateRef glue, GateRef object, GateRef needCOW, GateRef isIntKind);
    GateRef MigrateFromHeapValueToRawValue(GateRef glue, GateRef object, GateRef needCOW, GateRef isIntKind);
    void MigrateFromHoleIntToHoleNumber(GateRef glue, GateRef object);
    void MigrateFromHoleNumberToHoleInt(GateRef glue, GateRef object);

    // method operator
    GateRef IsJSFunction(GateRef glue, GateRef obj);
    GateRef IsBoundFunction(GateRef glue, GateRef obj);
    GateRef IsJSOrBoundFunction(GateRef glue, GateRef obj);
    GateRef GetMethodFromJSFunctionOrProxy(GateRef glue, GateRef object);
    GateRef IsNativeMethod(GateRef method);
    GateRef GetFuncKind(GateRef method);
    GateRef HasPrototype(GateRef kind);
    GateRef HasAccessor(GateRef kind);
    GateRef IsClassConstructorKind(GateRef kind);
    GateRef IsGeneratorKind(GateRef kind);
    GateRef IsBaseKind(GateRef kind);
    GateRef IsBaseConstructorKind(GateRef kind);
    GateRef IsSendableFunction(GateRef method);

    GateRef IsAOTLiteralInfo(GateRef glue, GateRef info);
    GateRef GetIhcFromAOTLiteralInfo(GateRef glue, GateRef info);
    GateRef IsAotWithCallField(GateRef method);
    GateRef IsFastCall(GateRef method);
    GateRef JudgeAotAndFastCall(GateRef jsFunc, CircuitBuilder::JudgeMethodType type);
    GateRef GetInternalString(GateRef glue, GateRef key);
    GateRef GetExpectedNumOfArgs(GateRef method);
    GateRef GetMethod(GateRef glue, GateRef obj, GateRef key, GateRef profileTypeInfo, GateRef slotId);
    // proxy operator
    GateRef GetMethodFromJSProxy(GateRef glue, GateRef proxy);
    GateRef GetHandlerFromJSProxy(GateRef glue, GateRef proxy);
    GateRef GetTargetFromJSProxy(GateRef glue, GateRef proxy);
    inline void SetHotnessCounter(GateRef glue, GateRef method, GateRef value);
    inline void SaveHotnessCounterIfNeeded(GateRef glue, GateRef sp, GateRef hotnessCounter, JSCallMode mode);
    inline void SavePcIfNeeded(GateRef glue);
    inline void SaveJumpSizeIfNeeded(GateRef glue, GateRef jumpSize);
    inline GateRef ComputeTaggedArraySize(GateRef length);
    inline GateRef GetGlobalConstantValue(
        VariableType type, GateRef glue, ConstantIndex index);
    GateRef GetGlobalConstantValue(VariableType type, GateRef glue, GateRef index);
    inline GateRef GetSingleCharTable(GateRef glue);
    inline GateRef IsEnableMutantArray(GateRef glue);
    inline GateRef IsEnableElementsKind(GateRef glue);
    inline GateRef GetGlobalEnvValue(VariableType type, GateRef glue, GateRef env, size_t index);
    GateRef CallGetterHelper(GateRef glue, GateRef receiver, GateRef holder,
                             GateRef accessor, ProfileOperation callback, GateRef hir = Circuit::NullGate());
    GateRef ConstructorCheck(GateRef glue, GateRef ctor, GateRef outPut, GateRef thisObj);
    GateRef GetCallSpreadArgs(GateRef glue, GateRef array, ProfileOperation callBack);
    GateRef GetIterator(GateRef glue, GateRef obj, ProfileOperation callback);
    // For BaselineJIT
    GateRef FastToBooleanBaseline(GateRef glue, GateRef value, bool flag = true);
    GateRef GetBaselineCodeAddr(GateRef baselineCode);

    GateRef IsFastTypeArray(GateRef jsType);
    GateRef IsJSProxy(GateRef jsType);
    GateRef GetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key, GateRef jsType);
    GateRef SetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key, GateRef value,
                                       GateRef jsType);
    GateRef TryStringOrSymbolToElementIndex(GateRef glue, GateRef key);
    inline GateRef DispatchBuiltins(GateRef glue, GateRef builtinsId, const std::vector<GateRef>& args,
                                    GateRef hir = Circuit::NullGate());
    inline GateRef DispatchBuiltinsWithArgv(GateRef glue, GateRef builtinsId, const std::vector<GateRef> &args);
    GateRef ComputeSizeUtf8(GateRef length);
    GateRef ComputeSizeUtf16(GateRef length);
    GateRef AlignUp(GateRef x, GateRef alignment);
    GateRef AlignDown(GateRef x, GateRef alignment);
    inline void InitStringLengthAndFlags(GateRef glue, GateRef str, GateRef length, bool compressed);
    inline void InitStringLengthAndFlags(GateRef glue, GateRef str, GateRef length, GateRef isCompressed);
    void Assert(int messageId, int line, GateRef glue, GateRef condition, Label *nextLabel);

    GateRef GetNormalStringData(GateRef glue, const StringInfoGateRef &stringInfoGate);

    void Comment(GateRef glue, const std::string &str);
    GateRef ToNumber(GateRef glue, GateRef tagged);
    inline GateRef LoadPfHeaderFromConstPool(GateRef glue, GateRef jsFunc);
    GateRef RemoveTaggedWeakTag(GateRef weak);
    inline GateRef LoadHCIndexFromConstPool(GateRef glue, GateRef cachedArray, GateRef cachedLength, GateRef traceId,
        Label *miss);
    inline GateRef LoadHCIndexInfosFromConstPool(GateRef glue, GateRef jsFunc);
    inline GateRef GetAttrIndex(GateRef index);
    inline GateRef GetAttr(GateRef glue, GateRef layoutInfo, GateRef index);
    inline GateRef GetKey(GateRef glue, GateRef layoutInfo, GateRef index);
    inline GateRef GetKeyIndex(GateRef index);
    GateRef CalArrayRelativePos(GateRef index, GateRef arrayLen);
    GateRef AppendSkipHole(GateRef glue, GateRef first, GateRef second, GateRef copyLength);
    GateRef IntToEcmaString(GateRef glue, GateRef number);
    GateRef ToCharCode(GateRef number);
    GateRef NumberToString(GateRef glue, GateRef number);
    inline GateRef GetAccGetter(GateRef glue, GateRef accesstor);
    inline GateRef GetAccSetter(GateRef glue, GateRef accesstor);
    inline GateRef GetViewedArrayBuffer(GateRef glue, GateRef dataView);
    inline GateRef GetByteOffset(GateRef dataView);
    inline GateRef GetByteLength(GateRef dataView);
    inline GateRef GetArrayBufferData(GateRef glue, GateRef buffer);
    inline GateRef GetArrayBufferByteLength(GateRef buffer);
    inline void SetArrayBufferByteLength(GateRef glue, GateRef buffer, GateRef length);
    GateRef IsDetachedBuffer(GateRef glue, GateRef buffer);
    inline GateRef IsMarkerCellValid(GateRef cell);
    inline GateRef GetAccessorHasChanged(GateRef obj);
    inline GateRef ComputeTaggedTypedArraySize(GateRef elementSize, GateRef length);
    GateRef ChangeTaggedPointerToInt64(GateRef x);
    inline GateRef GetPropertiesCache(GateRef glue);
    inline GateRef GetMegaICCache(GateRef glue, MegaICCache::MegaICKind kind);
    inline GateRef GetModuleLogger(GateRef glue);
    inline GateRef GetStageOfHotReload(GateRef glue);
    inline GateRef GetModuleManager(GateRef glue);
    inline void IncMegaProbeCount(GateRef glue);
    inline void IncMegaHitCount(GateRef glue);
    GateRef GetIndexFromPropertiesCache(GateRef glue, GateRef cache, GateRef cls, GateRef key,
                                        GateRef hir = Circuit::NullGate());
    GateRef GetHandlerFromMegaICCache(GateRef glue, GateRef cache, GateRef cls, GateRef key);
    inline void SetToPropertiesCache(GateRef glue, GateRef cache, GateRef cls, GateRef key, GateRef result,
                                     GateRef hir = Circuit::NullGate());
    GateRef HashFromHclassAndKey(GateRef glue, GateRef cls, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef HashFromHclassAndStringKey(GateRef glue, GateRef cls, GateRef key);
    GateRef HashSecondaryFromHclassAndStringKey(GateRef glue, GateRef cls, GateRef key);
    GateRef GetKeyHashCode(GateRef glue, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef GetStringKeyHashCode(GateRef glue, GateRef key, GateRef hir = Circuit::NullGate());
    inline GateRef GetSortedKey(GateRef glue, GateRef layoutInfo, GateRef index);
    inline GateRef GetSortedIndex(GateRef glue, GateRef layoutInfo, GateRef index);
    inline GateRef GetSortedIndex(GateRef attr);
    inline void StoreWithoutBarrier(VariableType type, GateRef base, GateRef offset, GateRef value);
    GateRef DefineFunc(GateRef glue, GateRef constpool, GateRef index,
                       FunctionKind targetKind = FunctionKind::LAST_FUNCTION_KIND);
    GateRef BinarySearch(GateRef glue, GateRef layoutInfo, GateRef key, GateRef propsNum,
                         GateRef hir = Circuit::NullGate());
    GateRef GetLastLeaveFrame(GateRef glue);
    void UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                             GateRef profileTypeInfo, GateRef slotId);

    // start: Fast path of Loading Module variable.
    GateRef Loadlocalmodulevar(GateRef glue, GateRef index, GateRef module);
    inline void ResolvedModuleMustBeSourceTextModule(GateRef glue, GateRef resolvedModule);
    inline void ModuleEnvMustBeDefined(GateRef curEnv);
    inline void CheckIsResolvedIndexBinding(GateRef glue, GateRef resolution);
    inline void RecordNameMustBeString(GateRef glue, GateRef recordName);
    inline GateRef GetNameDictionary(GateRef glue, GateRef module);
    inline GateRef GetCurrentModuleEnv(GateRef glue, GateRef curModule);
    inline GateRef GetBitFieldFromSourceTextModule(GateRef curModule);
    inline GateRef GetRequestedModules(GateRef glue, GateRef module);
    inline GateRef GetNamespaceFromSourceTextModule(GateRef glue, GateRef module);
    inline GateRef GetResolveModuleFromResolvedIndexBinding(GateRef glue, GateRef resolvedBinding);
    inline GateRef GetResolveModuleFromResolvedBinding(GateRef glue, GateRef resolvedBinding);
    inline GateRef GetIdxOfResolvedIndexBinding(GateRef resolvedBinding);
    inline void SetIsUpdatedFromResolvedBindingOfResolvedIndexBinding(GateRef glue, GateRef resolvedBinding,
                                                                      GateRef value);
    inline GateRef GetIdxOfResolvedRecordIndexBinding(GateRef resolvedBinding);
    inline GateRef GetModuleRecord(GateRef glue, GateRef resolvedBinding);
    inline GateRef GetBindingName(GateRef glue, GateRef resolvedBinding);
    inline GateRef IsResolvedIndexBinding(GateRef glue, GateRef resolvedBinding);
    inline GateRef IsResolvedBinding(GateRef glue, GateRef resolvedBinding);
    inline GateRef IsResolvedRecordIndexBinding(GateRef glue, GateRef resolvedBinding);
    inline GateRef IsResolvedRecordBinding(GateRef glue, GateRef resolvedBinding);
    inline GateRef IsLdEndExecPatchMain(GateRef glue);
    inline GateRef GetModuleType(GateRef module);
    inline GateRef IsNativeModule(GateRef module);
    inline GateRef IsCjsModule(GateRef module);
    inline GateRef GetSharedType(GateRef module);
    inline GateRef IsSharedModule(GateRef module);
    inline GateRef GetCjsModuleFunction(GateRef glue);
    void ModuleEnvMustBeValid(GateRef glue, GateRef curEnv);
    GateRef SearchFromModuleCache(GateRef glue, GateRef moduleName);
    GateRef GetNativeOrCjsExports(GateRef glue, GateRef resolvedModule);
    GateRef GetValueFromExportObject(GateRef glue, GateRef exports, GateRef index);
    GateRef GetNativeOrCjsModuleValue(GateRef glue, GateRef module, GateRef index);
    GateRef GetModuleValueByIndex(GateRef glue, GateRef module, GateRef index, GateRef isThrow);
    GateRef GetModuleValue(GateRef glue, GateRef module, GateRef index);
    GateRef GetNativeOrCjsModuleValueByName(GateRef glue, GateRef module, GateRef bindingName);
    GateRef ResolveElementOfObject(GateRef glue, GateRef hClass, GateRef exportName,
                                   GateRef module, GateRef layOutInfo);
    GateRef ResolveExportObject(GateRef glue, GateRef module, GateRef exports, GateRef exportName);
    GateRef UpdateBindingAndGetModuleValue(GateRef glue, GateRef module, GateRef requiredModule,
                                           GateRef index, GateRef bindingName);
    GateRef GetResolvedRecordIndexBindingModule(GateRef glue, GateRef module, GateRef resolvedBinding);
    GateRef GetResolvedRecordBindingModule(GateRef glue, GateRef module, GateRef resolvedBinding);
    GateRef LoadExternalmodulevar(GateRef glue, GateRef index, GateRef curModule);
    // end: Fast path of Loading Module variable.

    GateRef LoadModuleNamespaceByIndex(GateRef glue, GateRef index, GateRef module);
    GateRef GetArgumentsElements(GateRef glue, GateRef argvTaggedArray, GateRef argv);
    void TryToJitReuseCompiledFunc(GateRef glue, GateRef jsFunc, GateRef profileTypeInfoCell);
    void TryToBaselineJitReuseCompiledFunc(GateRef glue, GateRef jsFunc, GateRef profileTypeInfoCell);
    void StartTraceLoadDetail(GateRef glue, GateRef receiver, GateRef profileTypeInfo, GateRef slotId);
    void StartTraceLoadGetter(GateRef glue);
    void StartTraceLoadSlowPath(GateRef glue);
    void EndTraceLoad(GateRef glue);
    void StartTraceLoadValueDetail(GateRef glue, GateRef receiver, GateRef profileTypeInfo,
                                   GateRef slotId, GateRef key);
    void StartTraceLoadValueSlowPath(GateRef glue);
    void EndTraceLoadValue(GateRef glue);
    void StartTraceStoreDetail(GateRef glue, GateRef receiver, GateRef profileTypeInfo, GateRef slotId);
    void StartTraceStoreFastPath(GateRef glue);
    void StartTraceStoreSlowPath(GateRef glue);
    void EndTraceStore(GateRef glue);
    void StartTraceCallDetail(GateRef glue, GateRef profileTypeInfo, GateRef slotId);
    void EndTraceCall(GateRef glue);
    void StartTraceDefineFunc(GateRef glue, GateRef methodId, GateRef profileTypeInfo, GateRef slotId);
    void EndTraceDefineFunc(GateRef glue);
    void UpdateProfileTypeInfoAsMega(GateRef glue, GateRef profileTypeInfo, GateRef slotId);
    GateRef GetIsFastCall(GateRef machineCode);
    // compute new elementKind from sub elements
    GateRef ComputeTaggedArrayElementKind(GateRef glue, GateRef array, GateRef offset, GateRef end);
    GateRef GetElementsKindHClass(GateRef glue, GateRef elementKind);
    GateRef NeedBarrier(GateRef kind);
    GateRef JSTaggedValueToString(GateRef glue, GateRef val, GateRef hir = Circuit::NullGate());
    GateRef SpecialToString(GateRef glue, GateRef specialVal);
    GateRef ToPrimitive(GateRef glue, GateRef value, PreferredPrimitiveType type,
                        GateRef hir = Circuit::NullGate());
    GateRef GetPrimitiveTypeString(GateRef glue, PreferredPrimitiveType type);
    GateRef OrdinaryToPrimitive(GateRef glue, GateRef value,
                                PreferredPrimitiveType type, GateRef hir = Circuit::NullGate());
    GateRef CallFunction(GateRef glue, GateRef func);

    enum CopyKind {
        SameArray,
        DifferentArray,
    };
    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    void ArrayCopy(GateRef glue, GateRef srcObj, GateRef srcAddr, GateRef dstObj, GateRef dstAddr,
                   GateRef taggedValueCount, GateRef needBarrier, CopyKind copyKind);
protected:
    static constexpr int LOOP_UNROLL_FACTOR = 2;
    static constexpr int ELEMENTS_KIND_HCLASS_NUM = 12;
    static constexpr int SPECIAL_VALUE_NUM = 5;
    static int64_t SPECIAL_VALUE[SPECIAL_VALUE_NUM];
    static ConstantIndex SPECIAL_STRING_INDEX[SPECIAL_VALUE_NUM];

private:
    using BinaryOperation = std::function<GateRef(Environment*, GateRef, GateRef)>;
    template<OpCode Op>
    GateRef FastAddSubAndMul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastIntDiv(GateRef left, GateRef right, Label *bailout, ProfileOperation callback);
    template<OpCode Op>
    GateRef FastBinaryOp(GateRef glue, GateRef left, GateRef right,
                         const BinaryOperation& intOp, const BinaryOperation& floatOp, ProfileOperation callback);
    GateRef TryStringAdd(Environment *env, GateRef glue, GateRef left, GateRef right,
                         const BinaryOperation& intOp, const BinaryOperation& floatOp, ProfileOperation callback);
    GateRef NumberOperation(Environment *env, GateRef left, GateRef right,
                            const BinaryOperation& intOp,
                            const BinaryOperation& floatOp,
                            ProfileOperation callback);
    void SetSValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value, GateRef objectRegion,
                                      GateRef valueRegion);

    void SetNonSValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value, GateRef objectRegion,
                                     GateRef valueRegion);
    void InitializeArguments();
    void CheckDetectorName(GateRef glue, GateRef key, Label *fallthrough, Label *slow);
    GateRef CanDoubleRepresentInt(GateRef exp, GateRef expBits, GateRef fractionBits);
    GateRef CalIteratorKey(GateRef glue);

    template <class LabelPtrGetter>
    void SwitchGeneric(GateRef index, Label *defaultLabel, Span<const int64_t> keysValue,
                       LabelPtrGetter getIthLabelFn);
    GateRef StringCompareContents(GateRef glue, GateRef left, GateRef right, GateRef init, GateRef minLength);

    CallSignature *callSignature_ {nullptr};
    Environment *env_;
    GateRef globalEnv_ {Gate::InvalidGateRef};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_BUILDER_H
