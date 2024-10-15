/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
    StubBuilder(CallSignature *callSignature, Environment *env)
        : callSignature_(callSignature), env_(env) {}
    explicit StubBuilder(Environment *env)
        : env_(env) {}
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

#define BRANCH(condition, trueLabel, falseLabel)                       \
    {                                                                  \
        std::ostringstream os;                                         \
        os << __func__ << ": " << #trueLabel << "- " << #falseLabel;   \
        Branch(condition, trueLabel, falseLabel, os.str().c_str());    \
    }

    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel, const char *comment = nullptr);

#define BRANCH_LIKELY(condition, trueLabel, falseLabel)                                  \
    {                                                                                    \
        std::ostringstream os;                                                           \
        os << __func__ << ": " << #trueLabel << "(likely)- " << #falseLabel;             \
        BranchPredict(condition, trueLabel, falseLabel,                                  \
            BranchWeight::DEOPT_WEIGHT, BranchWeight::ONE_WEIGHT, os.str().c_str());     \
    }

#define BRANCH_UNLIKELY(condition, trueLabel, falseLabel)                                \
    {                                                                                    \
        std::ostringstream os;                                                           \
        os << __func__ << ": " << #trueLabel << "(unlikely)- " << #falseLabel;           \
        BranchPredict(condition, trueLabel, falseLabel,                                  \
            BranchWeight::ONE_WEIGHT, BranchWeight::DEOPT_WEIGHT, os.str().c_str());     \
    }

#define BRANCH_NO_WEIGHT(condition, trueLabel, falseLabel)                               \
    {                                                                                    \
        std::ostringstream os;                                                           \
        os << __func__ << ": " << #trueLabel << "(no weight)- " << #falseLabel;          \
        BranchPredict(condition, trueLabel, falseLabel,                                  \
            BranchWeight::ZERO_WEIGHT, BranchWeight::ZERO_WEIGHT, os.str().c_str());     \
    }

    void BranchPredict(GateRef condition, Label *trueLabel, Label *falseLabel,
                       uint32_t trueWeight = BranchWeight::ONE_WEIGHT, uint32_t falseWeight = BranchWeight::ONE_WEIGHT,
                       const char *comment = nullptr);

    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    /// LoopEnd with safepoint
    void LoopEnd(Label *loopHead, Environment *env, GateRef glue);
    GateRef CheckSuspend(GateRef glue);
    // call operation
    GateRef CallRuntime(GateRef glue, int index, const std::vector<GateRef>& args);
    GateRef CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv);
    GateRef CallNGCRuntime(GateRef glue, int index,
                           const std::vector<GateRef>& args, GateRef hir = Circuit::NullGate());
    GateRef FastCallOptimized(GateRef glue, GateRef code,
                              const std::vector<GateRef>& args, GateRef hir = Circuit::NullGate());
    GateRef CallOptimized(GateRef glue, GateRef code,
                          const std::vector<GateRef>& args, GateRef hir = Circuit::NullGate());
    GateRef GetAotCodeAddr(GateRef jsFunc);
    GateRef CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef CallBuiltinRuntime(GateRef glue, const std::initializer_list<GateRef>& args, bool isNew = false);
    GateRef CallBuiltinRuntimeWithNewTarget(GateRef glue, const std::initializer_list<GateRef>& args);
    void DebugPrint(GateRef thread, std::initializer_list<GateRef> args);
    void FatalPrint(GateRef thread, std::initializer_list<GateRef> args);
    // memory
    GateRef Load(VariableType type, GateRef base, GateRef offset);
    GateRef Load(VariableType type, GateRef base);
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
    GateRef TaggedIsNumeric(GateRef x);
    GateRef TaggedIsHole(GateRef x);
    GateRef TaggedIsNotHole(GateRef x);
    GateRef TaggedIsUndefined(GateRef x);
    GateRef TaggedIsException(GateRef x);
    GateRef TaggedIsSpecial(GateRef x);
    GateRef TaggedIsRegularObject(GateRef x);
    GateRef TaggedIsHeapObject(GateRef x);
    GateRef TaggedIsAccessor(GateRef x);
    GateRef ObjectAddressToRange(GateRef x);
    GateRef RegionInSpace(GateRef region, RegionSpaceFlag space);
    GateRef RegionInSpace(GateRef region, RegionSpaceFlag spaceBegin, RegionSpaceFlag spaceEnd);
    GateRef InEdenGeneration(GateRef region);
    GateRef InYoungGeneration(GateRef region);
    GateRef InGeneralYoungGeneration(GateRef region);
    GateRef InGeneralOldGeneration(GateRef region);
    GateRef InSharedHeap(GateRef region);
    GateRef InSharedSweepableSpace(GateRef region);
    GateRef TaggedIsGeneratorObject(GateRef x);
    GateRef TaggedIsJSArray(GateRef x);
    GateRef IsTaggedArray(GateRef x);
    GateRef TaggedIsAsyncGeneratorObject(GateRef x);
    GateRef TaggedIsJSGlobalObject(GateRef x);
    GateRef TaggedIsWeak(GateRef x);
    GateRef TaggedIsPrototypeHandler(GateRef x);
    GateRef TaggedIsStoreAOTHandler(GateRef x);
    GateRef TaggedIsTransWithProtoHandler(GateRef x);
    GateRef TaggedIsTransitionHandler(GateRef x);
    GateRef TaggedIsString(GateRef obj);
    GateRef TaggedIsStringIterator(GateRef obj);
    GateRef TaggedIsSharedObj(GateRef obj);
    GateRef BothAreString(GateRef x, GateRef y);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    GateRef TaggedIsSymbol(GateRef obj);
    GateRef TaggedIsArrayBuffer(GateRef obj);
    GateRef TaggedIsProtoChangeMarker(GateRef obj);
    GateRef GetNextPositionForHash(GateRef last, GateRef count, GateRef size);
    GateRef DoubleIsNAN(GateRef x);
    GateRef DoubleIsINF(GateRef x);
    GateRef DoubleIsNanOrInf(GateRef x);
    GateRef DoubleAbs(GateRef x);
    GateRef DoubleIsInteger(GateRef x);
    GateRef DoubleTrunc(GateRef x);
    GateRef TaggedIsNull(GateRef x);
    GateRef TaggedIsUndefinedOrNull(GateRef x);
    GateRef TaggedIsUndefinedOrNullOrHole(GateRef x);
    GateRef TaggedIsTrue(GateRef x);
    GateRef TaggedIsFalse(GateRef x);
    GateRef TaggedIsBoolean(GateRef x);
    GateRef TaggedGetInt(GateRef x);
    GateRef NumberGetInt(GateRef glue, GateRef x);
    GateRef TaggedGetNumber(GateRef x);
    GateRef Int8ToTaggedInt(GateRef x);
    GateRef Int16ToTaggedInt(GateRef x);
    GateRef IntToTaggedPtr(GateRef x);
    GateRef IntToTaggedInt(GateRef x);
    GateRef Int64ToTaggedInt(GateRef x);
    GateRef Int64ToTaggedIntPtr(GateRef x);
    GateRef DoubleToTaggedDoublePtr(GateRef x);
    GateRef BooleanToTaggedBooleanPtr(GateRef x);
    GateRef TaggedPtrToTaggedDoublePtr(GateRef x);
    GateRef TaggedPtrToTaggedIntPtr(GateRef x);
    GateRef CastDoubleToInt64(GateRef x);
    GateRef CastFloat32ToInt32(GateRef x);
    GateRef TaggedTrue();
    GateRef TaggedFalse();
    GateRef TaggedUndefined();
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
    GateRef IntPtrGreaterThan(GateRef x, GateRef y);
    // cast operation
    GateRef ChangeInt64ToIntPtr(GateRef val);
    GateRef ZExtInt32ToPtr(GateRef val);
    GateRef ChangeIntPtrToInt32(GateRef val);
    GateRef ToLength(GateRef glue, GateRef target);
    GateRef ToIndex(GateRef glue, GateRef tagged);

    // math operation
    GateRef Sqrt(GateRef x);
    GateRef GetSetterFromAccessor(GateRef accessor);
    GateRef GetElementsArray(GateRef object);
    void SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray,
                          MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef GetPropertiesArray(GateRef object);
    // SetProperties in js_object.h
    void SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray,
                            MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef GetHash(GateRef object);
    void SetHash(GateRef glue, GateRef object, GateRef hash);
    GateRef GetLengthOfTaggedArray(GateRef array);
    GateRef GetLengthOfJSTypedArray(GateRef array);
    GateRef GetExtraLengthOfTaggedArray(GateRef array);
    void SetExtraLengthOfTaggedArray(GateRef glue, GateRef array, GateRef len);
    // object operation
    GateRef IsJSHClass(GateRef obj);
    GateRef LoadHClass(GateRef object);
    void CanNotConvertNotValidObject(GateRef obj);
    void IsNotPropertyKey(GateRef obj);
    GateRef CreateDataProperty(GateRef glue, GateRef obj, GateRef proKey, GateRef value);
    GateRef CreateDataPropertyOrThrow(GateRef glue, GateRef onj, GateRef proKey, GateRef value);
    GateRef DefineField(GateRef glue, GateRef obj, GateRef proKey, GateRef value);
    void StoreHClass(GateRef glue, GateRef object, GateRef hClass);
    void StoreHClassWithoutBarrier(GateRef glue, GateRef object, GateRef hClass);
    void StoreBuiltinHClass(GateRef glue, GateRef object, GateRef hClass);
    void StorePrototype(GateRef glue, GateRef hclass, GateRef prototype);
    void CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef scrHClass);
    GateRef GetObjectType(GateRef hClass);
    GateRef IsDictionaryMode(GateRef object);
    GateRef IsDictionaryModeByHClass(GateRef hClass);
    GateRef IsDictionaryElement(GateRef hClass);
    GateRef IsJSArrayPrototypeModified(GateRef hClass);
    GateRef IsStableElements(GateRef hClass);
    GateRef HasConstructorByHClass(GateRef hClass);
    GateRef HasConstructor(GateRef object);
    GateRef IsClassConstructorFromBitField(GateRef bitfield);
    GateRef IsClassConstructor(GateRef object);
    GateRef IsClassPrototype(GateRef object);
    GateRef IsExtensible(GateRef object);
    GateRef TaggedObjectIsEcmaObject(GateRef obj);
    GateRef IsEcmaObject(GateRef obj);
    GateRef IsDataView(GateRef obj);
    GateRef IsSymbol(GateRef obj);
    GateRef IsString(GateRef obj);
    GateRef IsLineString(GateRef obj);
    GateRef IsSlicedString(GateRef obj);
    GateRef IsConstantString(GateRef obj);
    GateRef IsLiteralString(GateRef obj);
    GateRef IsTreeString(GateRef obj);
    GateRef TreeStringIsFlat(GateRef string);
    GateRef TaggedIsBigInt(GateRef obj);
    GateRef TaggedIsPropertyBox(GateRef obj);
    GateRef TaggedObjectIsBigInt(GateRef obj);
    GateRef IsJsProxy(GateRef obj);
    GateRef IsJSShared(GateRef obj);
    GateRef IsProfileTypeInfoCell0(GateRef obj);
    GateRef IsJSGlobalObject(GateRef obj);
    GateRef IsNativeModuleFailureInfo(GateRef obj);
    GateRef IsModuleNamespace(GateRef obj);
    GateRef IsNativePointer(GateRef obj);
    GateRef IsSourceTextModule(GateRef obj);
    GateRef ObjIsSpecialContainer(GateRef obj);
    GateRef IsJSPrimitiveRef(GateRef obj);
    GateRef IsJSFunctionBase(GateRef obj);
    GateRef IsConstructor(GateRef object);
    GateRef IsBase(GateRef func);
    GateRef IsDerived(GateRef func);
    GateRef IsJsArray(GateRef obj);
    GateRef IsJsSArray(GateRef obj);
    GateRef IsByteArray(GateRef obj);
    GateRef IsJsCOWArray(GateRef obj);
    GateRef IsCOWArray(GateRef obj);
    GateRef IsMutantTaggedArray(GateRef elements);
    GateRef IsJSObject(GateRef obj);
    GateRef IsEnumerable(GateRef attr);
    GateRef IsWritable(GateRef attr);
    GateRef IsConfigable(GateRef attr);
    GateRef IsDefaultAttribute(GateRef attr);
    GateRef IsArrayLengthWritable(GateRef glue, GateRef receiver);
    GateRef IsArrayLengthWritableForNonDictMode(GateRef receiver);
    GateRef IsAccessor(GateRef attr);
    GateRef IsInlinedProperty(GateRef attr);
    GateRef IsField(GateRef attr);
    GateRef IsNonSharedStoreField(GateRef attr);
    GateRef IsStoreShared(GateRef attr);
    GateRef IsElement(GateRef attr);
    GateRef IsStringElement(GateRef attr);
    GateRef IsNumber(GateRef attr);
    GateRef IsStringLength(GateRef attr);
    GateRef IsTypedArrayElement(GateRef attr);
    GateRef IsNonExist(GateRef attr);
    GateRef IsJSAPIVector(GateRef attr);
    GateRef IsJSAPIStack(GateRef obj);
    GateRef IsJSAPIPlainArray(GateRef obj);
    GateRef IsJSAPIQueue(GateRef obj);
    GateRef IsJSAPIDeque(GateRef obj);
    GateRef IsJSAPILightWeightMap(GateRef obj);
    GateRef IsJSAPILightWeightSet(GateRef obj);
    GateRef IsLinkedNode(GateRef obj);
    GateRef IsJSAPIHashMap(GateRef obj);
    GateRef IsJSAPIHashSet(GateRef obj);
    GateRef IsJSAPILinkedList(GateRef obj);
    GateRef IsJSAPIList(GateRef obj);
    GateRef IsJSAPIArrayList(GateRef obj);
    GateRef IsJSCollator(GateRef obj);
    GateRef IsJSObjectType(GateRef obj, JSType jsType);
    GateRef IsJSRegExp(GateRef obj);
    GateRef GetTarget(GateRef proxyObj);
    GateRef HandlerBaseIsAccessor(GateRef attr);
    GateRef HandlerBaseIsJSArray(GateRef attr);
    GateRef HandlerBaseIsInlinedProperty(GateRef attr);
    GateRef HandlerBaseGetOffset(GateRef attr);
    GateRef HandlerBaseGetAttrIndex(GateRef attr);
    GateRef HandlerBaseGetRep(GateRef attr);
    GateRef IsInvalidPropertyBox(GateRef obj);
    GateRef IsAccessorPropertyBox(GateRef obj);
    GateRef GetValueFromPropertyBox(GateRef obj);
    void SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value);
    GateRef GetTransitionHClass(GateRef obj);
    GateRef GetTransitionHandlerInfo(GateRef obj);
    GateRef GetTransWithProtoHClass(GateRef obj);
    GateRef GetTransWithProtoHandlerInfo(GateRef obj);
    GateRef GetProtoCell(GateRef object);
    GateRef GetPrototypeHandlerHolder(GateRef object);
    GateRef GetPrototypeHandlerHandlerInfo(GateRef object);
    GateRef GetStoreAOTHandlerHolder(GateRef object);
    GateRef GetStoreAOTHandlerHandlerInfo(GateRef object);
    inline GateRef GetLengthOfJSArray(GateRef array);
    inline GateRef GetPrototype(GateRef glue, GateRef object);
    GateRef GetHasChanged(GateRef object);
    GateRef HclassIsPrototypeHandler(GateRef hClass);
    GateRef HclassIsTransitionHandler(GateRef hClass);
    GateRef HclassIsPropertyBox(GateRef hClass);
    GateRef PropAttrGetOffset(GateRef attr);
    GateRef GetCtorPrototype(GateRef ctor);
    GateRef HasFunctionPrototype(GateRef ctor);
    GateRef InstanceOf(GateRef glue, GateRef object, GateRef target, GateRef profileTypeInfo, GateRef slotId,
        ProfileOperation callback);
    GateRef OrdinaryHasInstance(GateRef glue, GateRef target, GateRef obj);
    void TryFastHasInstance(GateRef glue, GateRef instof, GateRef target, GateRef object, Label *fastPath,
                            Label *exit, Variable *result, ProfileOperation callback);
    GateRef ConvertTaggedValueWithElementsKind(GateRef glue, GateRef value, GateRef extraKind);
    GateRef SameValue(GateRef glue, GateRef left, GateRef right);
    GateRef SameValueZero(GateRef glue, GateRef left, GateRef right);
    GateRef HasStableElements(GateRef glue, GateRef obj);
    GateRef IsStableJSArguments(GateRef glue, GateRef obj);
    GateRef IsStableJSArray(GateRef glue, GateRef obj);
    GateRef IsTypedArray(GateRef obj);
    GateRef IsStableArguments(GateRef hClass);
    GateRef IsStableArray(GateRef hClass);
    GateRef GetProfileTypeInfo(GateRef jsFunc);
    GateRef UpdateProfileTypeInfo(GateRef glue, GateRef jsFunc);
    // SetDictionaryOrder func in property_attribute.h
    GateRef SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value);
    GateRef GetPrototypeFromHClass(GateRef hClass);
    GateRef GetEnumCacheFromHClass(GateRef hClass);
    GateRef GetProtoChangeMarkerFromHClass(GateRef hClass);
    GateRef GetLayoutFromHClass(GateRef hClass);
    GateRef GetBitFieldFromHClass(GateRef hClass);
    GateRef GetLengthFromString(GateRef value);
    GateRef CalcHashcodeForInt(GateRef value);
    void CalcHashcodeForDouble(GateRef value, Variable *res, Label *exit);
    void CalcHashcodeForObject(GateRef glue, GateRef value, Variable *res, Label *exit);
    GateRef GetHashcodeFromString(GateRef glue, GateRef value, GateRef hir = Circuit::NullGate());
    inline GateRef IsIntegerString(GateRef string);
    inline void SetRawHashcode(GateRef glue, GateRef str, GateRef rawHashcode, GateRef isInteger);
    inline GateRef GetRawHashFromString(GateRef value);
    GateRef TryGetHashcodeFromString(GateRef string);
    inline GateRef GetMixHashcode(GateRef string);
    GateRef GetFirstFromTreeString(GateRef string);
    GateRef GetSecondFromTreeString(GateRef string);
    GateRef GetIsAllTaggedPropFromHClass(GateRef hclass);
    void SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield);
    void SetIsAllTaggedProp(GateRef glue, GateRef hclass, GateRef hasRep);
    void SetPrototypeToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef proto);
    void SetProtoChangeDetailsToHClass(VariableType type, GateRef glue, GateRef hClass,
                                       GateRef protoChange);
    void SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr,
                           MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetHClassTypeIDToHClass(GateRef glue, GateRef hClass, GateRef id);
    void SetEnumCacheToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef key);
    void SetTransitionsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef transition);
    void SetParentToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef parent);
    void SetIsPrototypeToHClass(GateRef glue, GateRef hClass, GateRef value);
    inline void SetIsAOT(GateRef glue, GateRef hClass, GateRef value);
    GateRef IsPrototypeHClass(GateRef hClass);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
                                 GateRef value, GateRef attrOffset, VariableType type = VariableType::JS_ANY(),
                                 MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef GetPropertyInlinedProps(GateRef obj, GateRef hClass,
        GateRef index);
    GateRef GetInlinedPropOffsetFromHClass(GateRef hclass, GateRef attrOffset);

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
    GateRef GetValueFromTaggedArray(GateRef elements, GateRef index);
    GateRef GetDataPtrInTaggedArray(GateRef array);
    GateRef GetDataPtrInTaggedArray(GateRef array, GateRef index);
    GateRef GetUnsharedConstpoolIndex(GateRef constpool);
    GateRef GetUnsharedConstpoolFromGlue(GateRef glue, GateRef constpool);
    GateRef GetUnsharedConstpool(GateRef array, GateRef index);
    GateRef GetValueFromMutantTaggedArray(GateRef elements, GateRef index);
    void CheckUpdateSharedType(bool isDicMode, Variable *result, GateRef glue, GateRef receiver, GateRef attr,
                               GateRef value, Label *executeSetProp, Label *exit);
    void CheckUpdateSharedType(bool isDicMode, Variable *result, GateRef glue, GateRef receiver, GateRef attr,
                               GateRef value, Label *executeSetProp, Label *exit, GateRef SCheckModelIsCHECK);
    void MatchFieldType(Variable *result, GateRef glue, GateRef fieldType, GateRef value, Label *executeSetProp,
                               Label *exit);
    GateRef GetFieldTypeFromHandler(GateRef attr);
    GateRef ClearSharedStoreKind(GateRef handlerInfo);
    GateRef UpdateSOutOfBoundsForHandler(GateRef handlerInfo);
    void RestoreElementsKindToGeneric(GateRef glue, GateRef jsHClass);
    GateRef GetTaggedValueWithElementsKind(GateRef receiver, GateRef index);
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
    GateRef IsSpecialContainer(GateRef jsType);
    GateRef IsSharedArray(GateRef jsType);
    GateRef IsAccessorInternal(GateRef value);
    template<typename DictionaryT>
    GateRef GetAttributesFromDictionary(GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetValueFromDictionary(GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetKeyFromDictionary(GateRef elements, GateRef entry);
    GateRef GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry);
    void UpdateFieldType(GateRef glue, GateRef hclass, GateRef attr);
    GateRef GetPropertiesAddrFromLayoutInfo(GateRef layout);
    GateRef GetPropertyMetaDataFromAttr(GateRef attr);
    GateRef TranslateToRep(GateRef value);
    GateRef GetKeyFromLayoutInfo(GateRef layout, GateRef entry);
    void MatchFieldType(GateRef glue, GateRef fieldType, GateRef value, Label *executeSetProp, Label *typeMismatch);
    GateRef FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
        GateRef key, GateRef propsNum, GateRef hir = Circuit::NullGate());
    GateRef FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index);
    GateRef FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr);
    GateRef FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData);
    GateRef JSObjectGetProperty(GateRef obj, GateRef hClass, GateRef propAttr);
    void JSObjectSetProperty(GateRef glue, GateRef obj, GateRef hClass, GateRef attr, GateRef key, GateRef value);
    GateRef ShouldCallSetter(GateRef receiver, GateRef holder, GateRef accessor, GateRef attr);
    GateRef CallSetterHelper(GateRef glue, GateRef holder, GateRef accessor,  GateRef value, ProfileOperation callback);
    GateRef SetHasConstructorCondition(GateRef glue, GateRef receiver, GateRef key);
    GateRef AddPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef propertyAttributes,
        ProfileOperation callback);
    GateRef IsUtf16String(GateRef string);
    GateRef IsUtf8String(GateRef string);
    GateRef IsInternalString(GateRef string);
    GateRef IsDigit(GateRef ch);
    void TryToGetInteger(GateRef string, Variable *num, Label *success, Label *failed);
    GateRef StringToElementIndex(GateRef glue, GateRef string);
    GateRef ComputeElementCapacity(GateRef oldLength);
    GateRef ComputeNonInlinedFastPropsCapacity(GateRef glue, GateRef oldLength,
                                               GateRef maxNonInlinedFastPropsCapacity);
    GateRef FindTransitions(GateRef glue, GateRef hClass, GateRef key, GateRef attr, GateRef value);
    GateRef CheckHClassForRep(GateRef hClass, GateRef rep);
    void TransitionForRepChange(GateRef glue, GateRef receiver, GateRef key, GateRef attr);
    void TransitToElementsKind(GateRef glue, GateRef receiver, GateRef value, GateRef kind);
    void TryMigrateToGenericKindForJSObject(GateRef glue, GateRef receiver, GateRef oldKind);
    GateRef TaggedToRepresentation(GateRef value);
    GateRef TaggedToElementKind(GateRef value);
    GateRef LdGlobalRecord(GateRef glue, GateRef key);
    GateRef LoadFromField(GateRef receiver, GateRef handlerInfo);
    GateRef LoadGlobal(GateRef cell);
    GateRef LoadElement(GateRef glue, GateRef receiver, GateRef key);
    GateRef LoadStringElement(GateRef glue, GateRef receiver, GateRef key);
    GateRef TryToElementsIndex(GateRef glue, GateRef key);
    GateRef CheckPolyHClass(GateRef cachedValue, GateRef hClass);
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
    inline GateRef ToObject(GateRef glue, GateRef obj);
    GateRef DeleteProperty(GateRef glue, GateRef obj, GateRef value);
    inline GateRef OrdinaryNewJSObjectCreate(GateRef glue, GateRef proto);
    inline GateRef NewJSPrimitiveRef(GateRef glue, size_t index, GateRef obj);
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
    GateRef TruncPtrToInt32(GateRef x);
    GateRef TruncInt64ToInt1(GateRef x);
    GateRef TruncInt32ToInt1(GateRef x);
    GateRef GetGlobalConstantAddr(GateRef index);
    GateRef GetGlobalConstantOffset(ConstantIndex index);
    GateRef IsCallableFromBitField(GateRef bitfield);
    GateRef IsCallable(GateRef obj);
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
    void SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value, bool withEden = false,
                             MemoryAttribute::ShareFlag share = MemoryAttribute::UNKNOWN);
    GateRef GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index,
                               ProfileOperation callback, GateRef hir = Circuit::NullGate());
    GateRef GetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
                              ProfileOperation callback, GateRef isInternal, bool canUseIsInternal = false);
    GateRef FastGetPropertyByName(GateRef glue, GateRef obj, GateRef key, ProfileOperation callback);
    GateRef FastGetPropertyByIndex(GateRef glue, GateRef obj, GateRef index,
                                   ProfileOperation callback, GateRef hir = Circuit::NullGate());
    GateRef GetPropertyByValue(GateRef glue, GateRef receiver, GateRef keyValue, ProfileOperation callback);
    void FastSetPropertyByName(GateRef glue, GateRef obj, GateRef key, GateRef value,
        ProfileOperation callback = ProfileOperation());
    void FastSetPropertyByIndex(GateRef glue, GateRef obj, GateRef index, GateRef value);
    GateRef SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index,
        GateRef value, bool useOwn, ProfileOperation callback = ProfileOperation(), bool defineSemantics = false);
    GateRef DefinePropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value);
    GateRef SetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
        GateRef value, bool useOwn, GateRef isInternal, ProfileOperation callback = ProfileOperation(),
        bool canUseIsInternal = false, bool defineSemantics = false); // Crawl prototype chain
    GateRef DefinePropertyByName(GateRef glue, GateRef receiver, GateRef key,
        GateRef value, GateRef isInternal, GateRef SCheckModelIsCHECK,
        ProfileOperation callback = ProfileOperation());
    GateRef SetPropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value, bool useOwn,
        ProfileOperation callback = ProfileOperation(), bool defineSemantics = false);
    GateRef DefinePropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value,
        GateRef SCheckModelIsCHECK, ProfileOperation callback = ProfileOperation());
    GateRef GetParentEnv(GateRef object);
    GateRef GetSendableParentEnv(GateRef object);
    GateRef GetPropertiesFromLexicalEnv(GateRef object, GateRef index);
    GateRef GetPropertiesFromSendableEnv(GateRef object, GateRef index);
    GateRef GetKeyFromLexivalEnv(GateRef lexicalEnv, GateRef levelIndex, GateRef slotIndex);
    void SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    void SetPropertiesToSendableEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    GateRef GetHomeObjectFromJSFunction(GateRef object);
    GateRef GetCallFieldFromMethod(GateRef method);
    GateRef GetSendableEnvFromModule(GateRef module);
    GateRef GetProtoOrHClass(GateRef function);
    GateRef GetViewedArrayBufferOrByteArray(GateRef typedArray);
    GateRef IsSendableFunctionModule(GateRef module);
    inline GateRef GetBuiltinId(GateRef method);
    void SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv,
                                 MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetProtoTransRootHClassToFunction(GateRef glue, GateRef object, GateRef hclass,
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
    void SetBitFieldToFunction(GateRef glue, GateRef function, GateRef value);
    void SetMachineCodeToFunction(GateRef glue, GateRef function, GateRef value,
                                  MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetTypedArrayName(GateRef glue, GateRef typedArray, GateRef name,
                           MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetContentType(GateRef glue, GateRef typedArray, GateRef type);
    void SetViewedArrayBufferOrByteArray(GateRef glue, GateRef typedArray, GateRef data,
                                         MemoryAttribute mAttr = MemoryAttribute::Default());
    void SetByteLength(GateRef glue, GateRef typedArray, GateRef byteLength);
    void SetByteOffset(GateRef glue, GateRef typedArray, GateRef offset);
    void SetTypedArrayLength(GateRef glue, GateRef typedArray, GateRef arrayLength);
    GateRef GetGlobalObject(GateRef glue);
    GateRef GetMethodFromFunction(GateRef function);
    GateRef GetModuleFromFunction(GateRef function);
    GateRef GetLengthFromFunction(GateRef function);
    GateRef GetHomeObjectFromFunction(GateRef function);
    GateRef GetEntryIndexOfGlobalDictionary(GateRef entry);
    GateRef GetBoxFromGlobalDictionary(GateRef object, GateRef entry);
    GateRef GetValueFromGlobalDictionary(GateRef object, GateRef entry);
    GateRef GetPropertiesFromJSObject(GateRef object);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOpWithOverflow(GateRef x, GateRef y);
    GateRef GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key, ProfileOperation callback);
    GateRef AddElementInternal(GateRef glue, GateRef receiver, GateRef index, GateRef value, GateRef attr);
    GateRef ShouldTransToDict(GateRef capcity, GateRef index);
    void NotifyArrayPrototypeChangedGuardians(GateRef glue, GateRef receiver);
    GateRef GrowElementsCapacity(GateRef glue, GateRef receiver, GateRef capacity);

    inline GateRef GetObjectFromConstPool(GateRef constpool, GateRef index);
    GateRef GetConstPoolFromFunction(GateRef jsFunc);
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
    GateRef FastMod(GateRef gule, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastTypeOf(GateRef left, GateRef right);
    GateRef FastMul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastDiv(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastAdd(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastSub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastToBoolean(GateRef value, bool flag = true);
    GateRef FastToBooleanWithProfile(GateRef value, ProfileOperation callback, bool flag = true);
    GateRef FastToBooleanWithProfileBaseline(GateRef value, ProfileOperation callback, bool flag = true);

    // Add SpecialContainer
    GateRef GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType);
    GateRef JSAPIContainerGet(GateRef glue, GateRef receiver, GateRef index);

    // for-in
    GateRef NextInternal(GateRef glue, GateRef iter);
    GateRef GetLengthFromForInIterator(GateRef iter);
    GateRef GetIndexFromForInIterator(GateRef iter);
    GateRef GetKeysFromForInIterator(GateRef iter);
    GateRef GetObjectFromForInIterator(GateRef iter);
    GateRef GetCachedHclassFromForInIterator(GateRef iter);
    void SetLengthOfForInIterator(GateRef glue, GateRef iter, GateRef length);
    void SetIndexOfForInIterator(GateRef glue, GateRef iter, GateRef index);
    void SetKeysOfForInIterator(GateRef glue, GateRef iter, GateRef keys);
    void SetObjectOfForInIterator(GateRef glue, GateRef iter, GateRef object);
    void SetCachedHclassOfForInIterator(GateRef glue, GateRef iter, GateRef hclass);
    void IncreaseInteratorIndex(GateRef glue, GateRef iter, GateRef index);
    void SetNextIndexOfArrayIterator(GateRef glue, GateRef iter, GateRef nextIndex);
    void SetIteratedArrayOfArrayIterator(GateRef glue, GateRef iter, GateRef iteratedArray);
    void SetBitFieldOfArrayIterator(GateRef glue, GateRef iter, GateRef kind);
    GateRef GetEnumCacheKind(GateRef glue, GateRef enumCache);
    GateRef GetEmptyArray(GateRef glue);
    GateRef IsEnumCacheValid(GateRef receiver, GateRef cachedHclass, GateRef kind);
    GateRef NeedCheckProperty(GateRef receiver);

    GateRef EnumerateObjectProperties(GateRef glue, GateRef obj);
    GateRef GetFunctionPrototype(GateRef glue, size_t index);
    GateRef ToPrototypeOrObj(GateRef glue, GateRef obj);
    GateRef IsSpecialKeysObject(GateRef obj);
    GateRef IsSlowKeysObject(GateRef obj);
    GateRef TryGetEnumCache(GateRef glue, GateRef obj);
    GateRef GetNumberOfElements(GateRef obj);
    GateRef IsSimpleEnumCacheValid(GateRef obj);
    GateRef IsEnumCacheWithProtoChainInfoValid(GateRef obj);

    // Exception handle
    GateRef HasPendingException(GateRef glue);
    void ReturnExceptionIfAbruptCompletion(GateRef glue);

    // ElementsKind Operations
    GateRef ValueIsSpecialHole(GateRef x);
    GateRef ElementsKindIsIntOrHoleInt(GateRef kind);
    GateRef ElementsKindIsNumOrHoleNum(GateRef kind);
    GateRef ElementsKindIsHeapKind(GateRef kind);
    GateRef ElementsKindHasHole(GateRef kind);
    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    void ArrayCopyAndHoleToUndefined(GateRef glue, GateRef srcAddr, GateRef dstObj, GateRef dstAddr,
                                     GateRef length, MemoryAttribute mAttr = MemoryAttribute::Default());
    void MigrateArrayWithKind(GateRef glue, GateRef object, GateRef oldKind, GateRef newKind);
    GateRef MigrateFromRawValueToHeapValues(GateRef glue, GateRef object, GateRef needCOW, GateRef isIntKind);
    GateRef MigrateFromHeapValueToRawValue(GateRef glue, GateRef object, GateRef needCOW, GateRef isIntKind);
    void MigrateFromHoleIntToHoleNumber(GateRef glue, GateRef object);
    void MigrateFromHoleNumberToHoleInt(GateRef glue, GateRef object);

    // method operator
    GateRef IsJSFunction(GateRef obj);
    GateRef IsBoundFunction(GateRef obj);
    GateRef IsJSOrBoundFunction(GateRef obj);
    GateRef GetMethodFromJSFunctionOrProxy(GateRef jsfunc);
    GateRef IsNativeMethod(GateRef method);
    GateRef GetFuncKind(GateRef method);
    GateRef HasPrototype(GateRef kind);
    GateRef HasAccessor(GateRef kind);
    GateRef IsClassConstructorKind(GateRef kind);
    GateRef IsGeneratorKind(GateRef kind);
    GateRef IsBaseKind(GateRef kind);
    GateRef IsBaseConstructorKind(GateRef kind);
    GateRef IsSendableFunction(GateRef method);

    GateRef IsAOTLiteralInfo(GateRef info);
    GateRef GetIhcFromAOTLiteralInfo(GateRef info);
    GateRef IsAotWithCallField(GateRef method);
    GateRef IsFastCall(GateRef method);
    GateRef JudgeAotAndFastCall(GateRef jsFunc, CircuitBuilder::JudgeMethodType type);
    GateRef GetInternalString(GateRef glue, GateRef key);
    GateRef GetExpectedNumOfArgs(GateRef method);
    GateRef GetMethod(GateRef glue, GateRef obj, GateRef key, GateRef profileTypeInfo, GateRef slotId);
    // proxy operator
    GateRef GetMethodFromJSProxy(GateRef proxy);
    GateRef GetHandlerFromJSProxy(GateRef proxy);
    GateRef GetTargetFromJSProxy(GateRef proxy);
    inline void SetHotnessCounter(GateRef glue, GateRef method, GateRef value);
    inline void SaveHotnessCounterIfNeeded(GateRef glue, GateRef sp, GateRef hotnessCounter, JSCallMode mode);
    inline void SavePcIfNeeded(GateRef glue);
    inline void SaveJumpSizeIfNeeded(GateRef glue, GateRef jumpSize);
    inline GateRef ComputeTaggedArraySize(GateRef length);
    inline GateRef GetGlobalConstantValue(
        VariableType type, GateRef glue, ConstantIndex index);
    inline GateRef GetSingleCharTable(GateRef glue);
    inline GateRef IsEnableElementsKind(GateRef glue);
    inline GateRef GetGlobalEnvValue(VariableType type, GateRef env, size_t index);
    GateRef CallGetterHelper(GateRef glue, GateRef receiver, GateRef holder,
                             GateRef accessor, ProfileOperation callback, GateRef hir = Circuit::NullGate());
    GateRef ConstructorCheck(GateRef glue, GateRef ctor, GateRef outPut, GateRef thisObj);
    GateRef GetCallSpreadArgs(GateRef glue, GateRef array, ProfileOperation callBack);
    GateRef GetIterator(GateRef glue, GateRef obj, ProfileOperation callback);
    // For BaselineJIT
    GateRef FastToBooleanBaseline(GateRef value, bool flag = true);
    GateRef GetBaselineCodeAddr(GateRef baselineCode);

    GateRef IsFastTypeArray(GateRef jsType);
    GateRef GetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key, GateRef jsType);
    GateRef SetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key, GateRef value,
                                       GateRef jsType);
    GateRef TryStringOrSymbolToElementIndex(GateRef glue, GateRef key);
    inline GateRef DispatchBuiltins(GateRef glue, GateRef builtinsId, const std::vector<GateRef>& args);
    inline GateRef DispatchBuiltinsWithArgv(GateRef glue, GateRef builtinsId, const std::vector<GateRef>& args);
    GateRef ComputeSizeUtf8(GateRef length);
    GateRef ComputeSizeUtf16(GateRef length);
    GateRef AlignUp(GateRef x, GateRef alignment);
    inline void SetLength(GateRef glue, GateRef str, GateRef length, bool compressed);
    inline void SetLength(GateRef glue, GateRef str, GateRef length, GateRef isCompressed);
    void Assert(int messageId, int line, GateRef glue, GateRef condition, Label *nextLabel);

    GateRef GetNormalStringData(const StringInfoGateRef &stringInfoGate);

    void Comment(GateRef glue, const std::string &str);
    GateRef ToNumber(GateRef glue, GateRef tagged);
    inline GateRef LoadPfHeaderFromConstPool(GateRef jsFunc);
    GateRef RemoveTaggedWeakTag(GateRef weak);
    inline GateRef LoadHCIndexFromConstPool(GateRef cachedArray, GateRef cachedLength, GateRef traceId, Label *miss);
    inline GateRef LoadHCIndexInfosFromConstPool(GateRef jsFunc);
    inline GateRef GetAttrIndex(GateRef index);
    inline GateRef GetAttr(GateRef layoutInfo, GateRef index);
    inline GateRef GetKey(GateRef layoutInfo, GateRef index);
    inline GateRef GetKeyIndex(GateRef index);
    GateRef CalArrayRelativePos(GateRef index, GateRef arrayLen);
    GateRef AppendSkipHole(GateRef glue, GateRef first, GateRef second, GateRef copyLength);
    GateRef IntToEcmaString(GateRef glue, GateRef number);
    GateRef ToCharCode(GateRef number);
    GateRef NumberToString(GateRef glue, GateRef number);
    inline GateRef GetViewedArrayBuffer(GateRef dataView);
    inline GateRef GetByteOffset(GateRef dataView);
    inline GateRef GetByteLength(GateRef dataView);
    inline GateRef GetArrayBufferData(GateRef buffer);
    inline GateRef GetArrayBufferByteLength(GateRef buffer);
    inline void SetArrayBufferByteLength(GateRef glue, GateRef buffer, GateRef length);
    GateRef IsDetachedBuffer(GateRef buffer);
    inline GateRef IsMarkerCellValid(GateRef cell);
    inline GateRef GetAccessorHasChanged(GateRef obj);
    inline GateRef ComputeTaggedTypedArraySize(GateRef elementSize, GateRef length);
    GateRef ChangeTaggedPointerToInt64(GateRef x);
    inline GateRef GetPropertiesCache(GateRef glue);
    GateRef GetIndexFromPropertiesCache(GateRef glue, GateRef cache, GateRef cls, GateRef key,
                                        GateRef hir = Circuit::NullGate());
    inline void SetToPropertiesCache(GateRef glue, GateRef cache, GateRef cls, GateRef key, GateRef result,
                                     GateRef hir = Circuit::NullGate());
    GateRef HashFromHclassAndKey(GateRef glue, GateRef cls, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef GetKeyHashCode(GateRef glue, GateRef key, GateRef hir = Circuit::NullGate());
    inline GateRef GetSortedKey(GateRef layoutInfo, GateRef index);
    inline GateRef GetSortedIndex(GateRef layoutInfo, GateRef index);
    inline GateRef GetSortedIndex(GateRef attr);
    inline void StoreWithoutBarrier(VariableType type, GateRef base, GateRef offset, GateRef value);
    GateRef DefineFunc(GateRef glue, GateRef constpool, GateRef index,
                       FunctionKind targetKind = FunctionKind::LAST_FUNCTION_KIND);
    GateRef BinarySearch(GateRef glue, GateRef layoutInfo, GateRef key, GateRef propsNum,
                         GateRef hir = Circuit::NullGate());
    GateRef GetLastLeaveFrame(GateRef glue);
    void UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                             GateRef profileTypeInfo, GateRef slotId);
    GateRef Loadlocalmodulevar(GateRef glue, GateRef index, GateRef module);
    GateRef GetArgumentsElements(GateRef glue, GateRef argvTaggedArray, GateRef argv);
    void TryToJitReuseCompiledFunc(GateRef glue, GateRef jsFunc, GateRef profileTypeInfoCell);
    GateRef GetIsFastCall(GateRef machineCode);

    enum OverlapKind {
        // NotOverlap means the source and destination memory are not overlap,
        // or overlap but the start of source is larger than destination.
        // then we will copy the memory from left to right.
        NotOverlap,
        // MustOverlap mean the source and destination memory are overlap,
        // and the start of source is lesser than destination.
        // then we will copy the memory from right to left.
        MustOverlap,
        // Unknown means all the kinds above are possible, it will select the suitable one in runtime.
        Unknown,
    };
    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    //
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    template <OverlapKind kind>
    void ArrayCopy(GateRef glue, GateRef srcAddr, GateRef dstObj, GateRef dstAddr, GateRef length,
                   MemoryAttribute mAttr = MemoryAttribute::Default());
protected:
    static constexpr int LOOP_UNROLL_FACTOR = 2;
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
                                     GateRef valueRegion, bool withEden);
    void InitializeArguments();
    void CheckDetectorName(GateRef glue, GateRef key, Label *fallthrough, Label *slow);
    GateRef CanDoubleRepresentInt(GateRef exp, GateRef expBits, GateRef fractionBits);
    GateRef CalIteratorKey(GateRef glue);

    CallSignature *callSignature_ {nullptr};
    Environment *env_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_BUILDER_H
