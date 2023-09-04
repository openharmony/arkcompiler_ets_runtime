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
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/variable_type.h"

namespace panda::ecmascript::kungfu {
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
    SUBENTRY(messageId, condition)
#define ASM_ASSERT_WITH_GLUE(messageId, condition, glue)                            \
    SUBENTRY_WITH_GLUE(messageId, condition, glue)
#elif defined(ENABLE_ASM_ASSERT)
#define ASM_ASSERT(messageId, condition)                                            \
    SUBENTRY(messageId, condition)
#define ASM_ASSERT_WITH_GLUE(messageId, condition, glue)                            \
    SUBENTRY_WITH_GLUE(messageId, condition, glue)
#else
#define ASM_ASSERT(messageId, ...) ((void)0)
#define ASM_ASSERT_WITH_GLUE(messageId, ...) ((void)0)
#endif

#ifndef NDEBUG
#define EXITENTRY()                                                                 \
    env->SubCfgExit()
#elif defined(ENABLE_ASM_ASSERT)
#define EXITENTRY()                                                                 \
    env->SubCfgExit()
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
    int NextVariableId()
    {
        return env_->NextVariableId();
    }
    // constant
    GateRef Int8(int8_t value);
    GateRef Int16(int16_t value);
    GateRef Int32(int32_t value);
    GateRef Int64(int64_t value);
    GateRef StringPtr(const std::string &str);
    GateRef IntPtr(int64_t value);
    GateRef IntPtrSize();
    GateRef RelocatableData(uint64_t value);
    GateRef True();
    GateRef False();
    GateRef Boolean(bool value);
    GateRef Double(double value);
    GateRef Undefined();
    GateRef Hole();
    GateRef Null();
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
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    // call operation
    GateRef CallRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv);
    GateRef CallNGCRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef FastCallOptimized(GateRef glue, GateRef code, const std::initializer_list<GateRef>& args);
    GateRef CallOptimized(GateRef glue, GateRef code, const std::initializer_list<GateRef>& args);
    GateRef GetAotCodeAddr(GateRef method);
    GateRef CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef CallBuiltinRuntime(GateRef glue, const std::initializer_list<GateRef>& args,
                               bool isNew = false, const char* comment = nullptr);
    void DebugPrint(GateRef thread, std::initializer_list<GateRef> args);
    void FatalPrint(GateRef thread, std::initializer_list<GateRef> args);
    // memory
    GateRef Load(VariableType type, GateRef base, GateRef offset);
    GateRef Load(VariableType type, GateRef base);
    void Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value);
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
    GateRef Int32Or(GateRef x, GateRef y);
    GateRef Int8And(GateRef x, GateRef y);
    GateRef Int32And(GateRef x, GateRef y);
    GateRef IntPtrAnd(GateRef x, GateRef y);
    GateRef BoolAnd(GateRef x, GateRef y);
    GateRef BoolOr(GateRef x, GateRef y);
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
    GateRef TaggedIsHeapObject(GateRef x);
    GateRef TaggedIsAccessor(GateRef x);
    GateRef ObjectAddressToRange(GateRef x);
    GateRef InYoungGeneration(GateRef region);
    GateRef TaggedIsGeneratorObject(GateRef x);
    GateRef TaggedIsJSArray(GateRef x);
    GateRef TaggedIsAsyncGeneratorObject(GateRef x);
    GateRef TaggedIsJSGlobalObject(GateRef x);
    GateRef TaggedIsWeak(GateRef x);
    GateRef TaggedIsPrototypeHandler(GateRef x);
    GateRef TaggedIsStoreTSHandler(GateRef x);
    GateRef TaggedIsTransWithProtoHandler(GateRef x);
    GateRef TaggedIsTransitionHandler(GateRef x);
    GateRef TaggedIsString(GateRef obj);
    GateRef BothAreString(GateRef x, GateRef y);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    GateRef GetNextPositionForHash(GateRef last, GateRef count, GateRef size);
    GateRef DoubleIsNAN(GateRef x);
    GateRef DoubleIsINF(GateRef x);
    GateRef TaggedIsNull(GateRef x);
    GateRef TaggedIsUndefinedOrNull(GateRef x);
    GateRef TaggedIsTrue(GateRef x);
    GateRef TaggedIsFalse(GateRef x);
    GateRef TaggedIsBoolean(GateRef x);
    GateRef TaggedGetInt(GateRef x);
    GateRef TaggedGetNumber(GateRef x);
    GateRef Int8ToTaggedInt(GateRef x);
    GateRef Int16ToTaggedInt(GateRef x);
    GateRef IntToTaggedPtr(GateRef x);
    GateRef IntToTaggedInt(GateRef x);
    GateRef Int64ToTaggedInt(GateRef x);
    GateRef DoubleToTaggedDoublePtr(GateRef x);
    GateRef TaggedPtrToTaggedDoublePtr(GateRef x);
    GateRef TaggedPtrToTaggedIntPtr(GateRef x);
    GateRef CastDoubleToInt64(GateRef x);
    GateRef TaggedTrue();
    GateRef TaggedFalse();
    GateRef TaggedUndefined();
    // compare operation
    GateRef Int8Equal(GateRef x, GateRef y);
    GateRef Equal(GateRef x, GateRef y);
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
    GateRef IntPtrGreaterThan(GateRef x, GateRef y);
    // cast operation
    GateRef ChangeInt64ToIntPtr(GateRef val);
    GateRef ZExtInt32ToPtr(GateRef val);
    GateRef ChangeIntPtrToInt32(GateRef val);
    GateRef ToLength(GateRef glue, GateRef target);

    // math operation
    GateRef Sqrt(GateRef x);
    GateRef GetSetterFromAccessor(GateRef accessor);
    GateRef GetElementsArray(GateRef object);
    void SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray);
    GateRef GetPropertiesArray(GateRef object);
    // SetProperties in js_object.h
    void SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray);
    void SetHash(GateRef glue, GateRef object, GateRef hash);
    GateRef GetLengthOfTaggedArray(GateRef array);
    // object operation
    GateRef IsJSHClass(GateRef obj);
    GateRef LoadHClass(GateRef object);
    void StoreHClass(GateRef glue, GateRef object, GateRef hClass);
    void CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef scrHClass);
    GateRef GetObjectType(GateRef hClass);
    GateRef IsDictionaryMode(GateRef object);
    GateRef IsDictionaryModeByHClass(GateRef hClass);
    GateRef IsDictionaryElement(GateRef hClass);
    GateRef IsStableElements(GateRef hClass);
    GateRef HasConstructorByHClass(GateRef hClass);
    GateRef HasConstructor(GateRef object);
    GateRef IsClassConstructorFromBitField(GateRef bitfield);
    GateRef IsClassConstructor(GateRef object);
    GateRef IsClassPrototype(GateRef object);
    GateRef IsExtensible(GateRef object);
    GateRef TaggedObjectIsEcmaObject(GateRef obj);
    GateRef IsEcmaObject(GateRef obj);
    GateRef IsSymbol(GateRef obj);
    GateRef IsString(GateRef obj);
    GateRef IsLineString(GateRef obj);
    GateRef IsConstantString(GateRef obj);
    GateRef IsTreeString(GateRef obj);
    GateRef TreeStringIsFlat(GateRef string);
    GateRef TaggedIsBigInt(GateRef obj);
    GateRef TaggedIsPropertyBox(GateRef obj);
    GateRef TaggedObjectIsBigInt(GateRef obj);
    GateRef IsJsProxy(GateRef obj);
    GateRef IsJSFunctionBase(GateRef obj);
    GateRef IsConstructor(GateRef object);
    GateRef IsBase(GateRef func);
    GateRef IsJsArray(GateRef obj);
    GateRef IsByteArray(GateRef obj);
    GateRef IsJsCOWArray(GateRef obj);
    GateRef IsJSObject(GateRef obj);
    GateRef IsWritable(GateRef attr);
    GateRef IsAccessor(GateRef attr);
    GateRef IsInlinedProperty(GateRef attr);
    GateRef IsField(GateRef attr);
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
    GateRef IsJSObjectType(GateRef obj, JSType jsType);
    GateRef GetTarget(GateRef proxyObj);
    GateRef HandlerBaseIsAccessor(GateRef attr);
    GateRef HandlerBaseIsJSArray(GateRef attr);
    GateRef HandlerBaseIsInlinedProperty(GateRef attr);
    GateRef HandlerBaseGetOffset(GateRef attr);
    GateRef HandlerBaseGetAttrIndex(GateRef attr);
    GateRef HandlerBaseGetRep(GateRef attr);
    GateRef IsInvalidPropertyBox(GateRef obj);
    GateRef GetValueFromPropertyBox(GateRef obj);
    void SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value);
    GateRef GetTransitionHClass(GateRef obj);
    GateRef GetTransitionHandlerInfo(GateRef obj);
    GateRef GetTransWithProtoHClass(GateRef obj);
    GateRef GetTransWithProtoHandlerInfo(GateRef obj);
    GateRef IsInternalAccessor(GateRef attr);
    GateRef GetProtoCell(GateRef object);
    GateRef GetPrototypeHandlerHolder(GateRef object);
    GateRef GetPrototypeHandlerHandlerInfo(GateRef object);
    GateRef GetStoreTSHandlerHolder(GateRef object);
    GateRef GetStoreTSHandlerHandlerInfo(GateRef object);
    GateRef GetPrototype(GateRef glue, GateRef object);
    GateRef GetHasChanged(GateRef object);
    GateRef HclassIsPrototypeHandler(GateRef hClass);
    GateRef HclassIsTransitionHandler(GateRef hClass);
    GateRef HclassIsPropertyBox(GateRef hClass);
    GateRef PropAttrGetOffset(GateRef attr);
    GateRef InstanceOf(GateRef glue, GateRef object, GateRef target, GateRef profileTypeInfo, GateRef slotId,
        ProfileOperation callback);
    GateRef OrdinaryHasInstance(GateRef glue, GateRef target, GateRef obj);
    void TryFastHasInstance(GateRef glue, GateRef instof, GateRef target, GateRef object, Label *fastPath,
                            Label *exit, Variable *result, ProfileOperation callback);
    GateRef SameValue(GateRef glue, GateRef left, GateRef right);
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
    GateRef GetLayoutFromHClass(GateRef hClass);
    GateRef GetBitFieldFromHClass(GateRef hClass);
    GateRef GetLengthFromString(GateRef value);
    GateRef GetHashcodeFromString(GateRef glue, GateRef value);
    GateRef TryGetHashcodeFromString(GateRef string);
    GateRef GetFirstFromTreeString(GateRef string);
    GateRef GetSecondFromTreeString(GateRef string);
    GateRef GetIsAllTaggedPropFromHClass(GateRef hclass);
    void SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield);
    void SetIsAllTaggedProp(GateRef glue, GateRef hclass, GateRef hasRep);
    void SetPrototypeToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef proto);
    void SetProtoChangeDetailsToHClass(VariableType type, GateRef glue, GateRef hClass,
                                       GateRef protoChange);
    void SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr);
    void SetHClassTypeIDToHClass(GateRef glue, GateRef hClass, GateRef id);
    void SetEnumCacheToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef key);
    void SetTransitionsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef transition);
    void SetIsProtoTypeToHClass(GateRef glue, GateRef hClass, GateRef value);
    GateRef IsProtoTypeHClass(GateRef hClass);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
        GateRef value, GateRef attrOffset, VariableType type = VariableType::JS_ANY());
    GateRef GetPropertyInlinedProps(GateRef obj, GateRef hClass,
        GateRef index);
    GateRef GetInlinedPropOffsetFromHClass(GateRef hclass, GateRef attrOffset);

    void IncNumberOfProps(GateRef glue, GateRef hClass);
    GateRef GetNumberOfPropsFromHClass(GateRef hClass);
    GateRef IsTSHClass(GateRef hClass);
    void SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value);
    GateRef GetElementsKindFromHClass(GateRef hClass);
    GateRef GetObjectSizeFromHClass(GateRef hClass);
    GateRef GetInlinedPropsStartFromHClass(GateRef hClass);
    GateRef GetInlinedPropertiesFromHClass(GateRef hClass);
    void ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val);
    GateRef GetValueFromTaggedArray(GateRef elements, GateRef index);
    void SetValueToTaggedArrayWithAttr(
        GateRef glue, GateRef array, GateRef index, GateRef key, GateRef val, GateRef attr);
    void SetValueToTaggedArrayWithRep(
        GateRef glue, GateRef array, GateRef index, GateRef val, GateRef rep, Label *repChange);
    void SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val);
    void UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index, GateRef value, GateRef attr);
    GateRef IsSpecialIndexedObj(GateRef jsType);
    GateRef IsSpecialContainer(GateRef jsType);
    GateRef IsAccessorInternal(GateRef value);
    template<typename DictionaryT>
    GateRef GetAttributesFromDictionary(GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetValueFromDictionary(GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetKeyFromDictionary(GateRef elements, GateRef entry);
    GateRef GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry);
    void SetPropAttrToLayoutInfo(GateRef glue, GateRef layout, GateRef entry, GateRef attr);
    GateRef GetPropertiesAddrFromLayoutInfo(GateRef layout);
    GateRef GetPropertyMetaDataFromAttr(GateRef attr);
    GateRef GetKeyFromLayoutInfo(GateRef layout, GateRef entry);
    GateRef FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
        GateRef key, GateRef propsNum);
    GateRef FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index);
    GateRef FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key);
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
    GateRef StringToElementIndex(GateRef glue, GateRef string);
    GateRef ComputePropertyCapacityInJSObj(GateRef oldLength);
    GateRef FindTransitions(GateRef glue, GateRef receiver, GateRef hClass, GateRef key, GateRef attr);
    void TransitionForRepChange(GateRef glue, GateRef receiver, GateRef key, GateRef attr);
    void TransitToElementsKind(GateRef glue, GateRef receiver, GateRef value, GateRef kind);
    GateRef TaggedToRepresentation(GateRef value);
    GateRef TaggedToElementKind(GateRef value);
    GateRef LdGlobalRecord(GateRef glue, GateRef key);
    GateRef LoadFromField(GateRef receiver, GateRef handlerInfo);
    GateRef LoadGlobal(GateRef cell);
    GateRef LoadElement(GateRef glue, GateRef receiver, GateRef key, ProfileOperation callback);
    GateRef TryToElementsIndex(GateRef glue, GateRef key);
    GateRef CheckPolyHClass(GateRef cachedValue, GateRef hClass);
    GateRef LoadICWithHandler(
        GateRef glue, GateRef receiver, GateRef holder, GateRef handler, ProfileOperation callback);
    GateRef StoreICWithHandler(GateRef glue, GateRef receiver, GateRef holder,
                               GateRef value, GateRef handler, ProfileOperation callback = ProfileOperation());
    GateRef ICStoreElement(GateRef glue, GateRef receiver, GateRef key,
                           GateRef value, GateRef handlerInfo, ProfileOperation callback);
    GateRef GetArrayLength(GateRef object);
    GateRef DoubleToInt(GateRef glue, GateRef x);
    GateRef StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler, ProfileOperation callback);
    GateRef StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler,
                             ProfileOperation callback, bool withPrototype = false);
    GateRef StoreGlobal(GateRef glue, GateRef value, GateRef cell);
    void JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr);
    void NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass);
    GateRef GetInt64OfTInt(GateRef x);
    GateRef GetInt32OfTInt(GateRef x);
    GateRef GetDoubleOfTInt(GateRef x);
    GateRef GetDoubleOfTDouble(GateRef x);
    GateRef GetDoubleOfTNumber(GateRef x);
    GateRef LoadObjectFromWeakRef(GateRef x);
    GateRef ExtFloat32ToDouble(GateRef x);
    GateRef ChangeInt32ToFloat32(GateRef x);
    GateRef ChangeInt32ToFloat64(GateRef x);
    GateRef ChangeUInt32ToFloat64(GateRef x);
    GateRef ChangeFloat64ToInt32(GateRef x);
    GateRef Int64ToTaggedPtr(GateRef x);
    GateRef TruncInt16ToInt8(GateRef x);
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
    GateRef GetGlobalConstantString(ConstantIndex index);
    GateRef IsCallableFromBitField(GateRef bitfield);
    GateRef IsCallable(GateRef obj);
    GateRef GetOffsetFieldInPropAttr(GateRef attr);
    GateRef SetOffsetFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetTrackTypeInPropAttr(GateRef attr, GateRef type);
    GateRef GetTrackTypeInPropAttr(GateRef attr);
    GateRef GetRepInPropAttr(GateRef attr);
    GateRef IsIntRepInPropAttr(GateRef attr);
    GateRef IsDoubleRepInPropAttr(GateRef attr);
    GateRef SetTaggedRepInPropAttr(GateRef attr);
    void SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value);
    void UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value);
    GateRef GetBitMask(GateRef bitoffset);
    GateRef IntPtrEuqal(GateRef x, GateRef y);
    void SetValueWithAttr(GateRef glue, GateRef obj, GateRef offset, GateRef key, GateRef value, GateRef attr);
    void SetValueWithRep(GateRef glue, GateRef obj, GateRef offset, GateRef value, GateRef rep, Label *repChange);
    void SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value);
    GateRef GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, ProfileOperation callback);
    GateRef GetPropertyByName(GateRef glue, GateRef receiver, GateRef key, ProfileOperation callback);
    GateRef FastGetPropertyByName(GateRef glue, GateRef obj, GateRef key, ProfileOperation callback);
    GateRef FastGetPropertyByIndex(GateRef glue, GateRef obj, GateRef index, ProfileOperation callback);
    GateRef GetPropertyByValue(GateRef glue, GateRef receiver, GateRef keyValue, ProfileOperation callback);
    GateRef SetPropertyByIndex(
        GateRef glue, GateRef receiver, GateRef index, GateRef value, bool useOwn, ProfileOperation callback);
    GateRef SetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
        GateRef value, bool useOwn, ProfileOperation callback = ProfileOperation()); // Crawl prototype chain
    GateRef SetPropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value, bool useOwn,
        ProfileOperation callback = ProfileOperation());
    GateRef GetParentEnv(GateRef object);
    GateRef GetPropertiesFromLexicalEnv(GateRef object, GateRef index);
    void SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    GateRef GetHomeObjectFromJSFunction(GateRef object);
    GateRef GetCallFieldFromMethod(GateRef method);
    inline GateRef GetBuiltinId(GateRef method);
    void SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv);
    GateRef GetGlobalObject(GateRef glue);
    GateRef GetMethodFromFunction(GateRef function);
    GateRef GetModuleFromFunction(GateRef function);
    GateRef GetEntryIndexOfGlobalDictionary(GateRef entry);
    GateRef GetBoxFromGlobalDictionary(GateRef object, GateRef entry);
    GateRef GetValueFromGlobalDictionary(GateRef object, GateRef entry);
    GateRef GetPropertiesFromJSObject(GateRef object);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOpWithOverflow(GateRef x, GateRef y);
    GateRef GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key, ProfileOperation callback);

    inline GateRef GetObjectFromConstPool(GateRef constpool, GateRef index);
    GateRef GetConstPoolFromFunction(GateRef jsFunc);
    GateRef GetStringFromConstPool(GateRef glue, GateRef constpool, GateRef index);
    GateRef GetMethodFromConstPool(GateRef glue, GateRef constpool, GateRef index);
    GateRef GetArrayLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module);
    GateRef GetObjectLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module);
    void SetExtensibleToBitfield(GateRef glue, GateRef obj, bool isExtensible);

    // fast path
    GateRef FastEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastStrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastStringEqual(GateRef glue, GateRef left, GateRef right);
    GateRef FastMod(GateRef gule, GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastTypeOf(GateRef left, GateRef right);
    GateRef FastMul(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastDiv(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastAdd(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastSub(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastToBoolean(GateRef value);

    // Add SpecialContainer
    GateRef GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType);
    GateRef JSAPIContainerGet(GateRef glue, GateRef receiver, GateRef index);

    // Exception handle
    GateRef HasPendingException(GateRef glue);
    void ReturnExceptionIfAbruptCompletion(GateRef glue);

    // method operator
    GateRef IsJSFunction(GateRef obj);
    GateRef IsBoundFunction(GateRef obj);
    GateRef GetMethodFromJSFunction(GateRef jsfunc);
    GateRef IsNativeMethod(GateRef method);
    GateRef IsOptimizedWithBitField(GateRef bitfield);
    GateRef CanFastCallWithBitField(GateRef bitfield);
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
    inline GateRef GetGlobalEnvValue(VariableType type, GateRef env, size_t index);
    GateRef CallGetterHelper(
        GateRef glue, GateRef receiver, GateRef holder, GateRef accessor, ProfileOperation callback);
    GateRef ConstructorCheck(GateRef glue, GateRef ctor, GateRef outPut, GateRef thisObj);
    GateRef JSCallDispatch(GateRef glue, GateRef func, GateRef actualNumArgs, GateRef jumpSize, GateRef hotnessCounter,
                           JSCallMode mode, std::initializer_list<GateRef> args,
                           ProfileOperation callback = ProfileOperation());
    GateRef IsFastTypeArray(GateRef jsType);
    GateRef GetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key, GateRef jsType);
    GateRef SetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key, GateRef value,
                                       GateRef jsType);
    GateRef TryStringOrSymbolToElementIndex(GateRef glue, GateRef key);
    inline GateRef DispatchBuiltins(GateRef glue, GateRef builtinsId, const std::initializer_list<GateRef>& args);
    inline GateRef DispatchBuiltinsWithArgv(GateRef glue, GateRef builtinsId,
                                            const std::initializer_list<GateRef>& args);
    GateRef ComputeSizeUtf8(GateRef length);
    GateRef ComputeSizeUtf16(GateRef length);
    GateRef AlignUp(GateRef x, GateRef alignment);
    void CallFastPath(GateRef glue, GateRef nativeCode, GateRef func, GateRef thisValue, GateRef actualNumArgs,
                      GateRef callField, GateRef method, Label* notFastBuiltins, Label* exit, Variable* result,
                      std::initializer_list<GateRef> args, JSCallMode mode);
    inline void SetLength(GateRef glue, GateRef str, GateRef length, bool compressed);
    inline void SetRawHashcode(GateRef glue, GateRef str, GateRef rawHashcode);
    void Assert(int messageId, int line, GateRef glue, GateRef condition, Label *nextLabel);

    GateRef FlattenString(GateRef glue, GateRef str);
    void FlattenString(GateRef str, Variable *flatStr, Label *fastPath, Label *slowPath);
    GateRef GetNormalStringData(GateRef str);

    void Comment(GateRef glue, const std::string &str);
    GateRef ToNumber(GateRef glue, GateRef tagged);
    inline GateRef LoadObjectFromConstPool(GateRef jsFunc, GateRef index);
    inline GateRef LoadPfHeaderFromConstPool(GateRef jsFunc);
    inline GateRef LoadHCIndexFromConstPool(GateRef jsFunc, GateRef traceId);
private:
    using BinaryOperation = std::function<GateRef(Environment*, GateRef, GateRef)>;
    GateRef ChangeTaggedPointerToInt64(GateRef x);
    template<OpCode Op>
    GateRef FastAddSubAndMul(GateRef left, GateRef right, ProfileOperation callback);
    GateRef FastIntDiv(GateRef left, GateRef right, Label *bailout, ProfileOperation callback);
    GateRef FastBinaryOp(GateRef left, GateRef right,
                         const BinaryOperation& intOp, const BinaryOperation& floatOp, ProfileOperation callback);
    void InitializeArguments();
    CallSignature *callSignature_ {nullptr};
    Environment *env_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_BUILDER_H
