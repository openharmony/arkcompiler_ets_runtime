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

#include "ecmascript/compiler/new_object_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_function_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_proxy_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_map.h"

namespace panda::ecmascript::kungfu {
void NewObjectStubBuilder::NewLexicalEnv(Variable *result, Label *exit, GateRef numSlots, GateRef parent)
{
    auto env = GetEnvironment();

    auto length = Int32Add(numSlots, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(length));
    // Be careful. NO GC is allowed when initization is not complete.
    Label hasPendingException(env);
    Label noException(env);
    auto hclass = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue_, ConstantIndex::LEXICAL_ENV_CLASS_INDEX);
    AllocateInYoung(result, &hasPendingException, &noException, hclass);
    Bind(&noException);
    {
        StoreHClass(glue_, result->ReadVariable(), hclass);
        Label afterInitialize(env);
        InitializeTaggedArrayWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), Hole(), Int32(LexicalEnv::RESERVED_ENV_LENGTH), length);
        Bind(&afterInitialize);
        SetValueToTaggedArray(VariableType::INT64(),
            glue_, result->ReadVariable(), Int32(LexicalEnv::SCOPE_INFO_INDEX), Hole());
        SetValueToTaggedArray(VariableType::JS_POINTER(),
            glue_, result->ReadVariable(), Int32(LexicalEnv::PARENT_ENV_INDEX), parent);
        // currentEnv is LexicalEnv/GlobalEnv for normal function, and is SFunctionEnv for SharedFunction.
        GateRef globalEnv = GetValueFromTaggedArray(glue_, parent, Int32(BaseEnv::GLOBAL_ENV_INDEX));
        Label isHole(env);
        Label notHole(env);
        BRANCH_UNLIKELY(TaggedIsHole(globalEnv), &isHole, &notHole);
        Bind(&isHole);
        // get globalenv from thread
        SetValueToTaggedArray(VariableType::JS_POINTER(),
            glue_, result->ReadVariable(), Int32(LexicalEnv::GLOBAL_ENV_INDEX), GetGlobalEnv(glue_));
        Jump(exit);
        Bind(&notHole);
        SetValueToTaggedArray(VariableType::JS_POINTER(),
            glue_, result->ReadVariable(), Int32(LexicalEnv::GLOBAL_ENV_INDEX), globalEnv);
        Jump(exit);
    }
    Bind(&hasPendingException);
    {
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::NewJSArrayWithSize(GateRef hclass, GateRef size)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label exit(env);
    env->SubCfgEntry(&entry);

    GateRef result = NewJSObject(glue_, hclass);
    Label enabledElementsKind(env);
    Label notEmptyArray(env);
    Label initObj(env);
    BRANCH(IsEnableElementsKind(glue_), &enabledElementsKind, &initObj);
    Bind(&enabledElementsKind);
    {
        // For new Array(Len), the elementsKind should be Hole
        BRANCH(Equal(TruncInt64ToInt32(size), Int32(0)), &initObj, &notEmptyArray);
        Bind(&notEmptyArray);
        {
            GateRef globalEnv = GetCurrentGlobalEnv();
            #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
            GateRef holeKindArrayClass =
                GetGlobalEnvValue(VariableType::JS_ANY(), glue_, globalEnv,
                    static_cast<size_t>(GlobalEnvField::ELEMENT_HOLE_TAGGED_HCLASS_INDEX));
            StoreHClass(glue_, result, holeKindArrayClass);
            #else
            GateRef holeKindArrayClass =
                GetGlobalEnvValue(VariableType::JS_ANY(), glue_, globalEnv,
                    static_cast<size_t>(GlobalEnvField::ELEMENT_HOLE_HCLASS_INDEX));
            StoreHClass(glue_, result, holeKindArrayClass);
            #endif
            Jump(&initObj);
        }
    }
    Bind(&initObj);
    DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
    NewTaggedArrayChecked(&array, TruncInt64ToInt32(size), &exit);
    Bind(&exit);
    auto arrayRet = *array;
    env->SubCfgExit();
    SetElementsArray(VariableType::JS_POINTER(), glue_, result, arrayRet);
    return result;
}

GateRef NewObjectStubBuilder::NewJSArrayWithHClass(GateRef hclass, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label exit(env);
    env->SubCfgEntry(&entry);
    GateRef result = NewJSObject(glue_, hclass);
    Label enabledMutantArray(env);
    Label initObj(env);
    DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
    BRANCH_UNLIKELY(IsEnableMutantArray(glue_), &enabledMutantArray, &initObj);
    Bind(&enabledMutantArray);
    {
        Label initMutantArray(env);
        GateRef kind = GetElementsKindFromHClass(hclass);
        GateRef needMutant = BitOr(ElementsKindIsIntOrHoleInt(kind), ElementsKindIsNumOrHoleNum(kind));
        BRANCH_NO_WEIGHT(needMutant, &initMutantArray, &initObj);
        Bind(&initMutantArray);
        {
            NewMutantTaggedArrayChecked(&array, length, &exit);
        }
    }
    Bind(&initObj);
    NewTaggedArrayChecked(&array, length, &exit);
    Bind(&exit);
    auto arrayRet = *array;
    env->SubCfgExit();
    SetElementsArray(VariableType::JS_POINTER(), glue_, result, arrayRet);
    return result;
}

GateRef NewObjectStubBuilder::NewJSFunctionByHClass(GateRef glue,
    GateRef method, GateRef hclass, FunctionKind targetKind)
{
    GateRef result = NewJSObject(glue, hclass);
    SetExtensibleToBitfield(glue, hclass, true);
    GateRef kind = GetFuncKind(method);
    BuiltinsFunctionStubBuilder builtinsFunctionStubBuilder(this, GetCurrentGlobalEnv());
    builtinsFunctionStubBuilder.InitializeJSFunction(glue, result, kind, targetKind);
    builtinsFunctionStubBuilder.InitializeFunctionWithMethod(glue, result, method, hclass);
    return result;
}

GateRef NewObjectStubBuilder::NewSFunctionByHClass(GateRef glue,
    GateRef method, GateRef hclass, FunctionKind targetKind)
{
    GateRef result = result = NewSObject(glue, hclass);
    GateRef kind = GetFuncKind(method);
    BuiltinsFunctionStubBuilder builtinsFunctionStubBuilder(this, GetCurrentGlobalEnv());
    builtinsFunctionStubBuilder.InitializeSFunction(glue, result, kind, targetKind);
    builtinsFunctionStubBuilder.InitializeFunctionWithMethod(glue, result, method, hclass);
    return result;
}

GateRef NewObjectStubBuilder::CloneJSFunction(GateRef glue, GateRef value)
{
    GateRef hclass = LoadHClass(glue, value);
    GateRef method = GetMethodFromFunction(glue, value);
    GateRef cloneFunc = NewJSFunctionByHClass(glue, method, hclass);
    GateRef length = GetPropertyInlinedProps(glue, value, hclass,
                                             Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX));
    SetPropertyInlinedProps(glue, value, hclass, length,
                            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                            VariableType::JS_ANY());
    SetLengthToFunction(glue, cloneFunc, GetLengthFromFunction(value));
    SetModuleToFunction(glue, cloneFunc, GetModuleFromFunction(glue, value));
    return cloneFunc;
}

GateRef NewObjectStubBuilder::CloneProperties(GateRef glue, GateRef currentEnv,
    GateRef elements, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isZero(env);
    Label notZero(env);
    Label initialize(env);
    Label afterInitialize(env);
    Label begin(env);
    Label body(env);
    Label isJSFunction(env);
    Label notJSFunction(env);
    Label endLoop(env);
    Label isJSFCondition(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef length = GetLengthOfTaggedArray(elements);
    BRANCH(Int32Equal(length, Int32(0)), &isZero, &notZero);
    Bind(&isZero);
    {
        result = GetEmptyArray(glue);
        Jump(&exit);
    }
    Bind(&notZero);
    {
        GateRef hclass = LoadHClass(glue, elements);
        size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(length));
        HeapAlloc(&result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    }
    Bind(&initialize);
    {
        GateRef hclass = LoadHClass(glue, elements);
        Store(VariableType::JS_POINTER(), glue, result.ReadVariable(), IntPtr(0), hclass);
        InitializeTaggedArrayWithSpeicalValue(&afterInitialize, result.ReadVariable(), Hole(), Int32(0), length);
    }
    Bind(&afterInitialize);
    {
        Jump(&begin);
    }
    LoopBegin(&begin);
    {
        BRANCH(Int32UnsignedLessThan(*i, length), &body, &exit);
        Bind(&body);
        {
            GateRef value = GetValueFromTaggedArray(glue, elements, *i);
            BRANCH(TaggedIsHeapObject(value), &isJSFCondition, &notJSFunction);
            Bind(&isJSFCondition);
            {
                BRANCH(IsJSFunction(glue, value), &isJSFunction, &notJSFunction);
            }
            Bind(&isJSFunction);
            {
                GateRef newFunc = CloneJSFunction(glue, value);
                SetLexicalEnvToFunction(glue, newFunc, currentEnv);
                SetHomeObjectToFunction(glue, newFunc, obj);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *result, *i, newFunc);
                Jump(&endLoop);
            }
            Bind(&notJSFunction);
            {
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *result, *i, value);
                Jump(&endLoop);
            }
            Bind(&endLoop);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&begin);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewAccessorData(GateRef glue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label noException(env);

    GateRef hclass = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::ACCESSOR_DATA_CLASS_INDEX);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    size_ = GetObjectSizeFromHClass(hclass);
    HeapAlloc(&result, &noException, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    Bind(&noException);
    {
        Store(VariableType::JS_POINTER(), glue, result.ReadVariable(), IntPtr(0), hclass);
        Store(VariableType::JS_ANY(), glue, *result, IntPtr(AccessorData::GETTER_OFFSET), Undefined());
        Store(VariableType::JS_ANY(), glue, *result, IntPtr(AccessorData::SETTER_OFFSET), Undefined());
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CloneObjectLiteral(GateRef glue, GateRef literal, GateRef currentEnv)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    // canShareHClass is true default
    GateRef hclass = LoadHClass(glue, literal);
    Label exit(env);
    Label less(env);
    Label greater(env);
    Label startLoop(env);
    Label begin(env);
    Label body(env);
    Label isTaggedRep(env);
    Label notTaggedRep(env);
    Label isJSFunction(env);
    Label notJSFunction(env);
    Label endLoop(env);
    Label isAccessorData(env);
    Label notAccessorData(env);
    Label middleCondition(env);
    Label isJSFCondition(env);
    Label isACCCondition(env);
    Label isAccessorJudgment(env);

    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    GateRef result = NewJSObject(glue, hclass);

    GateRef elements = GetElementsArray(glue, literal);
    GateRef newElements = CloneProperties(glue, currentEnv, elements, result);
    SetElementsArray(VariableType::JS_ANY(), glue, result, newElements);

    GateRef properties = GetPropertiesArray(glue, literal);
    GateRef newProperties = CloneProperties(glue, currentEnv, properties, result);
    SetPropertiesArray(VariableType::JS_ANY(), glue, result, newProperties);

    GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
    GateRef numberOfProps = GetNumberOfPropsFromHClass(hclass);
    BRANCH(Int32UnsignedLessThan(inlineProps, numberOfProps), &less, &greater);
    Bind(&less);
    {
        length = inlineProps;
        Jump(&startLoop);
    }
    Bind(&greater);
    {
        length = numberOfProps;
        Jump(&startLoop);
    }
    Bind(&startLoop);
    {
        Jump(&begin);
    }
    LoopBegin(&begin);
    {
        BRANCH(Int32UnsignedLessThan(*i, *length), &body, &exit);
        Bind(&body);
        {
            GateRef layout = GetLayoutFromHClass(glue, hclass);
            value = GetPropertyInlinedProps(glue, literal, hclass, *i);
            GateRef attr = GetPropAttrFromLayoutInfo(glue, layout, *i);
            BRANCH(BoolNot(IsTaggedRepInPropAttr(attr)), &notTaggedRep, &middleCondition);
        }
        Bind(&middleCondition);
        {
            BRANCH(TaggedIsHeapObject(*value), &isJSFCondition, &isTaggedRep);
        }
        Bind(&isJSFCondition);
        {
            BRANCH(IsJSFunction(glue, *value), &isJSFunction, &isAccessorJudgment);
        }
        Bind(&isAccessorJudgment);
        {
            BRANCH(Int32Equal(GetObjectType(LoadHClass(glue, *value)),
                Int32(static_cast<int32_t>(JSType::ACCESSOR_DATA))),
                &isAccessorData, &isTaggedRep);
        }
        Bind(&isJSFunction);
        {
            GateRef newFunc = CloneJSFunction(glue, *value);
            SetLexicalEnvToFunction(glue, newFunc, currentEnv);
            SetHomeObjectToFunction(glue, newFunc, result);
            SetPropertyInlinedProps(glue, result, hclass, newFunc, *i,
                VariableType::JS_ANY());
            Jump(&endLoop);
        }
        Bind(&isAccessorData);
        {
            auto objLiteral = NewAccessorData(glue);
            value = objLiteral;
            Jump(&isTaggedRep);
        }
        Bind(&notTaggedRep);
        {
            SetPropertyInlinedProps(glue, result, hclass, *value, *i,
                VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            Jump(&endLoop);
        }
        Bind(&isTaggedRep);
        {
            SetPropertyInlinedProps(glue, result, hclass, *value, *i);
            Jump(&endLoop);
        }
        Bind(&endLoop);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&begin);
    }
    Bind(&exit);
    auto ret = result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CreateObjectHavingMethod(GateRef glue, GateRef literal, GateRef currentEnv)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label isPendingException(env);
    Label exit(env);
    auto objLiteral = CloneObjectLiteral(glue, literal, currentEnv);
    DEFVARIABLE(result, VariableType::JS_ANY(), objLiteral);
    BRANCH(HasPendingException(glue), &isPendingException, &exit);
    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::NewJSObject(Variable *result, Label *exit, GateRef hclass)
{
    auto env = GetEnvironment();

    size_ = GetObjectSizeFromHClass(hclass);
    // Be careful. NO GC is allowed when initization is not complete.
    Label hasPendingException(env);
    Label noException(env);
    AllocateInYoung(result, &hasPendingException, &noException, hclass);
    Bind(&noException);
    {
        StoreHClass(glue_, result->ReadVariable(), hclass);
        DEFVARIABLE(initValue, VariableType::JS_ANY(), Undefined());
        Label isAOT(env);
        Label initialize(env);
        Label inProgress(env);
        Label notInProgress(env);
        GateRef isObjSizeTrackingInProgress = IsObjSizeTrackingInProgress(hclass);
        Branch(isObjSizeTrackingInProgress, &inProgress, &notInProgress);
        Bind(&inProgress);
        {
            initValue = GetGlobalConstantValue(
                VariableType::JS_POINTER(), glue_, ConstantIndex::FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX);
            Jump(&initialize);
        }
        Bind(&notInProgress);
        BRANCH(IsAOTHClass(hclass), &isAOT, &initialize);
        Bind(&isAOT);
        {
            // The object which created by AOT speculative hclass, should be initialized as hole, means does not exist,
            // to follow ECMA spec.
            initValue = Hole();
            Jump(&initialize);
        }
        Bind(&initialize);
        Label afterInitialize(env);
        InitializeWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), *initValue, Int32(JSObject::SIZE), ChangeIntPtrToInt32(size_),
            MemoryAttribute::NoBarrier());
        Bind(&afterInitialize);
        Label objSizeTrackingStep(env);
        InitializeObject(result);
        Branch(isObjSizeTrackingInProgress, &objSizeTrackingStep, exit);
        Bind(&objSizeTrackingStep);
        {
            Label checkNext(env);
            BRANCH_UNLIKELY(LoadPrimitive(VariableType::BOOL(), glue_,
                                          IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
                            exit, &checkNext);
            Bind(&checkNext);
            Label calcuFinalCount(env);
            GateRef count = GetConstructionCounter(hclass);
            GateRef nextCount = Int32Sub(count, Int32(1));
            SetConstructionCounter(glue_, hclass, nextCount);
            Branch(Int32Equal(nextCount, Int32(0)), &calcuFinalCount, exit);
            Bind(&calcuFinalCount);
            {
                CallNGCRuntime(glue_, RTSTUB_ID(FinishObjSizeTracking), { hclass });
                Jump(exit);
            }
        }
    }
    Bind(&hasPendingException);
    {
        Jump(exit);
    }
}

void NewObjectStubBuilder::NewSObject(Variable *result, Label *exit, GateRef hclass)
{
    auto env = GetEnvironment();

    size_ = GetObjectSizeFromHClass(hclass);
    Label afterAllocate(env);
    AllocateInSOld(result, &afterAllocate, hclass);
    Bind(&afterAllocate);
    {
        StoreHClass(glue_, result->ReadVariable(), hclass);
        DEFVARIABLE(initValue, VariableType::JS_ANY(), Undefined());
        Label isAOT(env);
        Label initialize(env);
        BRANCH(IsAOTHClass(hclass), &isAOT, &initialize);
        Bind(&isAOT);
        {
            // The object which created by AOT speculative hclass, should be initialized as hole, means does not exist,
            // to follow ECMA spec.
            initValue = Hole();
            Jump(&initialize);
        }
        Bind(&initialize);
        Label afterInitialize(env);
        InitializeWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), *initValue, Int32(JSObject::SIZE), ChangeIntPtrToInt32(size_),
            MemoryAttribute::NoBarrier());
        Bind(&afterInitialize);
        InitializeObject(result);
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::NewJSProxy(GateRef glue, GateRef target, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isCallable(env);
    Label notCallable(env);
    Label noException(env);
    Label allocateProxy(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    DEFVARIABLE(hclass, VariableType::JS_ANY(), Undefined());
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef objectFunctionPrototype = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                        GlobalEnv::OBJECT_FUNCTION_PROTOTYPE_INDEX);
    GateRef emptyObject = OrdinaryNewJSObjectCreate(glue, objectFunctionPrototype);
    BRANCH(IsCallable(glue, target), &isCallable, &notCallable);
    Bind(&isCallable);
    {
        Label isConstructor(env);
        Label notConstructor(env);
        BRANCH(IsConstructor(glue, target), &isConstructor, &notConstructor);
        Bind(&isConstructor);
        {
            hclass = GetGlobalConstantValue(
                VariableType::JS_POINTER(), glue, ConstantIndex::JS_PROXY_CONSTRUCT_CLASS_INDEX);
            Jump(&allocateProxy);
        }
        Bind(&notConstructor);
        {
            hclass = GetGlobalConstantValue(
                VariableType::JS_POINTER(), glue, ConstantIndex::JS_PROXY_CALLABLE_CLASS_INDEX);
            Jump(&allocateProxy);
        }
    }
    Bind(&notCallable);
    {
        hclass = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::JS_PROXY_ORDINARY_CLASS_INDEX);
        Jump(&allocateProxy);
    }
    Bind(&allocateProxy);
    {
        size_ = GetObjectSizeFromHClass(*hclass);
        AllocateInYoung(&result, &exit, &noException, *hclass);
    }
    Bind(&noException);
    {
        GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
        Store(VariableType::INT64(), glue, *result, hashOffset, Int64(JSTaggedValue(0).GetRawData()));
        GateRef proxyMethod = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::PROXY_METHOD_INDEX);
        StoreHClass(glue_, *result, *hclass, MemoryAttribute::NoBarrier());
        BuiltinsProxyStubBuilder builtinsProxyStubBuilder(this, globalEnv);
        builtinsProxyStubBuilder.SetMethod(glue, *result, proxyMethod);
        builtinsProxyStubBuilder.SetTarget(glue, *result, target);
        builtinsProxyStubBuilder.SetHandler(glue, *result, handler);
        builtinsProxyStubBuilder.SetPrivateField(glue, *result, emptyObject);
        builtinsProxyStubBuilder.SetIsRevoked(glue, *result, False());
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::DefineFuncForJit(GateRef glue, GateRef method, GateRef hclass, FunctionKind targetKind)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label initialize(env);
    NewJSObject(&result, &initialize, hclass, GetObjectSizeFromHClass(hclass));
    Bind(&initialize);
    SetExtensibleToBitfield(glue, hclass, true);
    GateRef func = *result;

    SetProtoOrHClassToFunction(glue, func, Hole(), MemoryAttribute::NoBarrier());
    SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryAttribute::NoBarrier());

    if (targetKind == FunctionKind::BASE_CONSTRUCTOR) {
        auto funcprotoAccessor =
            GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
        Store(VariableType::JS_ANY(), glue, func,
              IntPtr(JSFunction::GetInlinedPropertyOffset(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX)),
              funcprotoAccessor, MemoryAttribute::NoBarrier());
    } else {
        ASSERT(targetKind == FunctionKind::ARROW_FUNCTION || targetKind == FunctionKind::NORMAL_FUNCTION);
    }
    auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::FUNCTION_NAME_ACCESSOR);
    Store(VariableType::JS_ANY(), glue, func,
          IntPtr(JSFunction::GetInlinedPropertyOffset(JSFunction::NAME_INLINE_PROPERTY_INDEX)), funcAccessor,
          MemoryAttribute::NoBarrier());
    funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
    Store(VariableType::JS_ANY(), glue, func,
          IntPtr(JSFunction::GetInlinedPropertyOffset(JSFunction::LENGTH_INLINE_PROPERTY_INDEX)), funcAccessor,
          MemoryAttribute::NoBarrier());

    SetCallableToBitfield(glue, hclass, true);
    SetMethodToFunction(glue, func, method);
    SetBitFieldToFunction(glue, func, Int32(0), MemoryAttribute::NoBarrier());
    SetMachineCodeToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
    SetBaselineJitCodeToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
    SetCodeEntryToFunctionFromMethod(glue, func, method);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::NewJSObject(Variable *result, Label *exit, GateRef hclass, GateRef size)
{
    auto env = GetEnvironment();
    size_ = size;
    Label initialize(env);
    HeapAlloc(result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    Bind(&initialize);
    StoreHClass(glue_, result->ReadVariable(), hclass, MemoryAttribute::NoBarrier());
    DEFVARIABLE(initValue, VariableType::JS_ANY(), Undefined());
    Label afterInitialize(env);
    InitializeWithSpeicalValue(&afterInitialize,
        result->ReadVariable(), *initValue, Int32(JSObject::SIZE), ChangeIntPtrToInt32(size_),
        MemoryAttribute::NoBarrier());
    Bind(&afterInitialize);
    InitializeObject(result);
    Jump(exit);
}

GateRef NewObjectStubBuilder::NewJSObject(GateRef glue, GateRef hclass, GateRef size)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    SetParameters(glue, size);
    NewJSObject(&result, &exit, hclass, size);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewJSObject(GateRef glue, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    SetGlue(glue);
    NewJSObject(&result, &exit, hclass);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewSObject(GateRef glue, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    SetGlue(glue);
    NewSObject(&result, &exit, hclass);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::NewTaggedArrayChecked(Variable *result, GateRef len, Label *exit)
{
    auto env = GetEnvironment();
    Label overflow(env);
    Label notOverflow(env);
    BRANCH(Int32UnsignedGreaterThan(len, Int32(INT32_MAX)), &overflow, &notOverflow);
    Bind(&overflow);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(LenGreaterThanMax));
        CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result->WriteVariable(Exception());
        Jump(exit);
    }
    Bind(&notOverflow);
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(len));
    // Be careful. NO GC is allowed when initization is not complete.
    Label noException(env);
    auto hclass = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue_, ConstantIndex::TAGGED_ARRAY_CLASS_INDEX);
    AllocateInYoung(result, exit, &noException, hclass);
    Bind(&noException);
    {
        StoreBuiltinHClass(glue_, result->ReadVariable(), hclass);
        Label afterInitialize(env);
        InitializeTaggedArrayWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), Hole(), Int32(0), len);
        Bind(&afterInitialize);
        Jump(exit);
    }
}

void NewObjectStubBuilder::NewMutantTaggedArrayChecked(Variable *result, GateRef len, Label *exit)
{
    auto env = GetEnvironment();
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(len));
    Label afterAllocate(env);
    auto hclass = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue_, ConstantIndex::MUTANT_TAGGED_ARRAY_CLASS_INDEX);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, &afterAllocate, hclass);
    Bind(&afterAllocate);
    Label noException(env);
    BRANCH(TaggedIsException(result->ReadVariable()), exit, &noException);
    Bind(&noException);
    {
        StoreHClass(glue_, result->ReadVariable(), hclass);
        Label afterInitialize(env);
        InitializeTaggedArrayWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), SpecialHole(), Int32(0), len);
        Bind(&afterInitialize);
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::NewTaggedArray(GateRef glue, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isEmpty(env);
    Label notEmpty(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    SetGlue(glue);
    BRANCH(Int32Equal(len, Int32(0)), &isEmpty, &notEmpty);
    Bind(&isEmpty);
    {
        result = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        Jump(&exit);
    }
    Bind(&notEmpty);
    {
        Label next(env);
        Label slowPath(env);
        BRANCH(Int32LessThan(len, Int32(MAX_TAGGED_ARRAY_LENGTH)), &next, &slowPath);
        Bind(&next);
        {
            NewTaggedArrayChecked(&result, len, &exit);
        }
        Bind(&slowPath);
        {
            result = CallRuntime(glue_, RTSTUB_ID(NewTaggedArray), { IntToTaggedInt(len) });
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewMutantTaggedArray(GateRef glue, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isEmpty(env);
    Label notEmpty(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    SetGlue(glue);
    BRANCH(Int32Equal(len, Int32(0)), &isEmpty, &notEmpty);
    Bind(&isEmpty);
    {
        result = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        Jump(&exit);
    }
    Bind(&notEmpty);
    {
        Label next(env);
        Label slowPath(env);
        BRANCH(Int32LessThan(len, Int32(MAX_TAGGED_ARRAY_LENGTH)), &next, &slowPath);
        Bind(&next);
        {
            NewMutantTaggedArrayChecked(&result, len, &exit);
        }
        Bind(&slowPath);
        {
            result = CallRuntime(glue_, RTSTUB_ID(NewMutantTaggedArray), { IntToTaggedInt(len) });
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::ExtendArray(Variable *res, GateRef glue, GateRef elements,
                                       GateRef newLen, Label *exit, RegionSpaceFlag spaceType, bool isMutantArray)
{
    auto env = GetEnvironment();
    SetGlue(glue);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(newLen));
    // Be careful. NO GC is allowed when initization is not complete.
    Label allocArray(env);
    Label afterAllocate(env);
    auto hclass = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue_,
        isMutantArray ? ConstantIndex::MUTANT_TAGGED_ARRAY_CLASS_INDEX : ConstantIndex::TAGGED_ARRAY_CLASS_INDEX);
    HeapAlloc(&array, &allocArray, spaceType, hclass);
    Bind(&allocArray);
    StoreBuiltinHClass(glue_, array.ReadVariable(), hclass);
    Store(VariableType::INT32(), glue_, *array, IntPtr(TaggedArray::LENGTH_OFFSET), newLen);
    GateRef oldExtractLen = GetExtraLengthOfTaggedArray(elements);
    Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
    GateRef oldL = GetLengthOfTaggedArray(elements);
    Label afterCopy(env);
    GateRef startOffset = Int32Mul(oldL, Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef endOffset = Int32Mul(newLen, Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef srcDataPtr = GetDataPtrInTaggedArray(elements);
    GateRef dstDataPtr = GetDataPtrInTaggedArray(*array);
    if (isMutantArray) {
        ArrayCopy(glue, elements, srcDataPtr, *array, dstDataPtr, oldL, Boolean(false), DifferentArray);
        InitializeWithSpeicalValue(&afterCopy, dstDataPtr, SpecialHole(), startOffset,
                                   endOffset, MemoryAttribute::NoBarrier());
    } else {
        ArrayCopy(glue, elements, srcDataPtr, *array, dstDataPtr, oldL, Boolean(true), DifferentArray);
        InitializeWithSpeicalValue(&afterCopy, dstDataPtr, Hole(), startOffset, endOffset,
                                   MemoryAttribute::NoBarrier());
    }
    Bind(&afterCopy);
    {
        *res = *array;
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::ExtendArrayCheck(GateRef glue, GateRef elements, GateRef newLen,
                                               RegionSpaceFlag spaceType)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    SetGlue(glue);
    Label extendMutantArray(env);
    Label extendNormalArray(env);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    BRANCH(IsMutantTaggedArray(glue, elements),
           &extendMutantArray, &extendNormalArray);
    Bind(&extendNormalArray);
    ExtendArray(&res, glue, elements, newLen, &exit, spaceType);
    Bind(&extendMutantArray);
    ExtendArray(&res, glue, elements, newLen, &exit, spaceType, true);
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::ExtendArrayWithOptimizationCheck(GateRef glue, GateRef elements, GateRef newLen)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    SetGlue(glue);
    DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
    GateRef checkIsMutantTaggedArray = IsMutantTaggedArray(glue, elements);
    Label exit(env);
    Label next(env);
    Label slowPath(env);
    BRANCH(Int32UnsignedLessThan(newLen, Int32(MAX_EXTEND_ARRAY_LENGTH)), &slowPath, &next);
    Bind(&slowPath);
    {
        array = ExtendArrayCheck(glue, elements, newLen);
        Jump(&exit);
    }
    Bind(&next);
    {
        Label extendMutantArray(env);
        Label extendNormalArray(env);
        Label afterCreate(env);
        GateRef oldExtractLen = GetExtraLengthOfTaggedArray(elements);
        GateRef oldL = GetLengthOfTaggedArray(elements);
        BRANCH(checkIsMutantTaggedArray, &extendMutantArray, &extendNormalArray);
        Bind(&extendMutantArray);
        {
            array = CallRuntime(glue_, RTSTUB_ID(NewMutantTaggedArray), { IntToTaggedInt(newLen) });
            Jump(&afterCreate);
        }
        Bind(&extendNormalArray);
        {
            array = CallRuntime(glue_, RTSTUB_ID(NewTaggedArray), { IntToTaggedInt(newLen) });
            Jump(&afterCreate);
        }
        Bind(&afterCreate);
        Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::LENGTH_OFFSET), newLen);
        Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
        ArrayCopy(glue, elements, GetDataPtrInTaggedArray(elements), *array,
            GetDataPtrInTaggedArray(*array), oldL, BoolNot(checkIsMutantTaggedArray), DifferentArray);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *array;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CopyArray(GateRef glue, GateRef elements, GateRef oldLen,
                                        GateRef newLen, RegionSpaceFlag spaceType)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label emptyArray(env);
    Label notEmptyArray(env);
    BRANCH(Int32Equal(newLen, Int32(0)), &emptyArray, &notEmptyArray);
    Bind(&emptyArray);
    result = GetEmptyArray(glue);
    Jump(&exit);
    Bind(&notEmptyArray);
    {
        Label extendArray(env);
        Label notExtendArray(env);
        BRANCH(Int32UnsignedGreaterThan(newLen, oldLen), &extendArray, &notExtendArray);
        Bind(&extendArray);
        {
            result = ExtendArrayCheck(glue, elements, newLen, spaceType);
            Jump(&exit);
        }
        Bind(&notExtendArray);
        {
            DEFVARIABLE(array, VariableType::JS_ANY(), elements);
            Label isMutantTaggedArray(env);
            Label isNotMutantTaggedArray(env);
            Label afterInitializeElements(env);
            GateRef checkIsMutantTaggedArray = IsMutantTaggedArray(glue, *array);
            BRANCH(checkIsMutantTaggedArray, &isMutantTaggedArray, &isNotMutantTaggedArray);
            Bind(&isMutantTaggedArray);
            {
                array = NewMutantTaggedArray(glue, newLen);
                Jump(&afterInitializeElements);
            }
            Bind(&isNotMutantTaggedArray);
            {
                array = NewTaggedArray(glue, newLen);
                Jump(&afterInitializeElements);
            }
            Bind(&afterInitializeElements);
            Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::LENGTH_OFFSET), newLen);
            GateRef oldExtractLen = GetExtraLengthOfTaggedArray(elements);
            Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
            ArrayCopy(glue, elements, GetDataPtrInTaggedArray(elements), *array,
                      GetDataPtrInTaggedArray(*array), newLen, BoolNot(checkIsMutantTaggedArray), DifferentArray);
            result = *array;
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewJSForinIterator(GateRef glue, GateRef receiver, GateRef keys, GateRef cachedHClass,
                                                 GateRef EnumCacheKind)
{
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                       GlobalEnv::FOR_IN_ITERATOR_CLASS_INDEX);
    GateRef iter = NewJSObject(glue, hclass);
    // init JSForinIterator
    SetObjectOfForInIterator(glue, iter, receiver);
    SetCachedHClassOfForInIterator(glue, iter, cachedHClass);
    SetKeysOfForInIterator(glue, iter, keys);
    SetIndexOfForInIterator(glue, iter, Int32(0));
    SetCacheKindForInIterator(glue, iter, EnumCacheKind);
    GateRef length = GetLengthOfTaggedArray(keys);
    SetLengthOfForInIterator(glue, iter, length);
    return iter;
}

GateRef NewObjectStubBuilder::LoadHClassFromMethod(GateRef glue, GateRef method)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(hclass, VariableType::JS_ANY(), Undefined());
    GateRef kind = GetFuncKind(method);
    Label exit(env);
    Label defaultLabel(env);
    Label isNormal(env);
    Label notNormal(env);
    Label isAsync(env);
    Label notAsync(env);

    Label labelBuffer[2] = { Label(env), Label(env) };
    Label labelBuffer1[3] = { Label(env), Label(env), Label(env) };
    int64_t valueBuffer[2] = {
        static_cast<int64_t>(FunctionKind::NORMAL_FUNCTION), static_cast<int64_t>(FunctionKind::ARROW_FUNCTION) };
    int64_t valueBuffer1[3] = {
        static_cast<int64_t>(FunctionKind::BASE_CONSTRUCTOR), static_cast<int64_t>(FunctionKind::GENERATOR_FUNCTION),
        static_cast<int64_t>(FunctionKind::ASYNC_GENERATOR_FUNCTION) };
    GateRef globalEnv = GetCurrentGlobalEnv();
    BRANCH(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ARROW_FUNCTION))),
        &isNormal, &notNormal);
    Bind(&isNormal);
    {
        // 2 : this switch has 2 case
        Switch(kind, &defaultLabel, valueBuffer, labelBuffer, 2);
        Bind(&labelBuffer[0]);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::FUNCTION_CLASS_WITH_PROTO);
            Jump(&exit);
        }
        Bind(&labelBuffer[1]);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                       GlobalEnv::FUNCTION_CLASS_WITHOUT_PROTO);
            Jump(&exit);
        }
    }
    Bind(&notNormal);
    {
        BRANCH(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ASYNC_FUNCTION))),
            &isAsync, &notAsync);
        Bind(&isAsync);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ASYNC_FUNCTION_CLASS);
            Jump(&exit);
        }
        Bind(&notAsync);
        {
            // 3 : this switch has 3 case
            Switch(kind, &defaultLabel, valueBuffer1, labelBuffer1, 3);
            Bind(&labelBuffer1[0]);
            {
                hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                           GlobalEnv::FUNCTION_CLASS_WITH_PROTO);
                Jump(&exit);
            }
            Bind(&labelBuffer1[1]);
            {
                hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                           GlobalEnv::GENERATOR_FUNCTION_CLASS);
                Jump(&exit);
            }
            // 2 : index of kind
            Bind(&labelBuffer1[2]);
            {
                hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                           GlobalEnv::ASYNC_GENERATOR_FUNCTION_CLASS);
                Jump(&exit);
            }
        }
    }
    Bind(&defaultLabel);
    {
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *hclass;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::LoadSHClassFromMethod(GateRef glue, GateRef method)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(hclass, VariableType::JS_ANY(), Undefined());
    GateRef kind = GetFuncKind(method);
    Label exit(env);
    Label isSendableFunc(env);
    Label isNotSendableFunc(env);
    Label isNormal(env);
    Label notNormal(env);

    GateRef globalEnv = GetCurrentGlobalEnv();
    BRANCH(IsSendableFunction(method), &isSendableFunc, &isNotSendableFunc);
    Bind(&isSendableFunc);
    {
        BRANCH(Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::BASE_CONSTRUCTOR))), &isNormal, &notNormal);
        Bind(&isNormal);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                       GlobalEnv::SHARED_FUNCTION_CLASS_WITH_PROTO);
            Jump(&exit);
        }
        Bind(&notNormal);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                       GlobalEnv::SHARED_ASYNC_FUNCTION_CLASS);
            Jump(&exit);
        }
    }
    Bind(&isNotSendableFunc);
    {
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *hclass;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewJSFunction(GateRef glue, GateRef method, FunctionKind targetKind)
{
    GateRef hclass = LoadHClassFromMethod(glue, method);
    return NewJSFunctionByHClass(glue, method, hclass, targetKind);
}

GateRef NewObjectStubBuilder::NewJSSendableFunction(GateRef glue, GateRef method, FunctionKind targetKind)
{
    GateRef hclass = LoadSHClassFromMethod(glue, method);
    return NewSFunctionByHClass(glue, method, hclass, targetKind);
}

void NewObjectStubBuilder::NewJSFunction(GateRef glue, GateRef jsFunc, GateRef index, GateRef length, GateRef lexEnv,
                                         Variable *result, Label *success, Label *failed, GateRef slotId,
                                         FunctionKind targetKind)
{
    auto env = GetEnvironment();
    Label hasException(env);
    Label notException(env);
    Label isSendableFunc(env);
    Label isNotSendableFunc(env);
    Label afterSendableFunc(env);
    GateRef constPool = GetConstPoolFromFunction(glue, jsFunc);
    SetGlue(glue);
    result->WriteVariable(DefineFunc(glue, constPool, index, targetKind));
    BRANCH(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(failed);
    }
    Bind(&notException);
    {
        GateRef module = GetModuleFromFunction(glue, jsFunc);
        SetLengthToFunction(glue, result->ReadVariable(), length);
        BRANCH(IsSendableFunction(GetMethodFromFunction(glue, result->ReadVariable())),
            &isSendableFunc, &isNotSendableFunc);
        Bind(&isSendableFunc);
        {
            GateRef smodule = CallRuntime(glue, RTSTUB_ID(GetSharedModule), { module });
            SetSendableEnvToModule(glue, smodule, GetSendableEnvFromModule(glue, module));
            SetModuleToFunction(glue, result->ReadVariable(), smodule);
            GateRef emptySFuncEnv = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                           ConstantIndex::EMPTY_SFUNCTION_ENV_INDEX);
            SetLexicalEnvToFunction(glue, result->ReadVariable(), emptySFuncEnv);
            Jump(&afterSendableFunc);
        }
        Bind(&isNotSendableFunc);
        {
            SetLexicalEnvToFunction(glue, result->ReadVariable(), lexEnv);
            SetModuleToFunction(glue, result->ReadVariable(), module);
            SetHomeObjectToFunction(glue, result->ReadVariable(), GetHomeObjectFromFunction(glue, jsFunc));
#if ECMASCRIPT_ENABLE_IC
            SetProfileTypeInfoCellToFunction(jsFunc, result->ReadVariable(), slotId);
#endif
            Jump(&afterSendableFunc);
        }
        Bind(&afterSendableFunc);
        Jump(success);
    }
}

GateRef NewObjectStubBuilder::NewJSFunction(GateRef glue, GateRef method, GateRef homeObject)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsEcmaObject), TaggedObjectIsEcmaObject(glue, homeObject));

    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::FUNCTION_CLASS_WITHOUT_PROTO);

    GateRef jsFunc = NewJSFunctionByHClass(glue, method, hclass);
    SetHomeObjectToFunction(glue, jsFunc, homeObject);
    ASM_ASSERT(GET_MESSAGE_STRING_ID(HasPendingException), BoolNot(HasPendingException(glue)));
    return jsFunc;
}

GateRef NewObjectStubBuilder::DefineMethod(GateRef glue, GateRef method, GateRef homeObject, GateRef length,
                                           GateRef lexEnv, GateRef module)
{
    GateRef func = NewJSFunction(glue, method, homeObject);

    SetLengthToFunction(glue, func, length);
    SetLexicalEnvToFunction(glue, func, lexEnv);
    SetModuleToFunction(glue, func, module);
    return func;
}

void NewObjectStubBuilder::NewJSFunctionForJit(GateRef glue, GateRef jsFunc, GateRef hclass, GateRef method,
                                               GateRef length, GateRef lexEnv, Variable *result, Label *success,
                                               Label *failed, GateRef slotId, FunctionKind targetKind)
{
    auto env = GetEnvironment();
    Label hasException(env);
    Label notException(env);
    Label isSendableFunc(env);
    Label isNotSendableFunc(env);
    Label afterSendableFunc(env);
    Label slotValueUpdate(env);
    Label slotValueNotUndefined(env);
    Label profileTypeInfoEnd(env);
    SetGlue(glue);
    result->WriteVariable(DefineFuncForJit(glue, method, hclass, targetKind));
    BRANCH(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(failed);
    }
    Bind(&notException);
    {
        GateRef module = GetModuleFromFunction(glue, jsFunc);
        SetLengthToFunction(glue, result->ReadVariable(), length);
        SetLexicalEnvToFunction(glue, result->ReadVariable(), lexEnv);
        SetModuleToFunction(glue, result->ReadVariable(), module);
        SetHomeObjectToFunction(glue, result->ReadVariable(), GetHomeObjectFromFunction(glue, jsFunc));
#if ECMASCRIPT_ENABLE_IC
        GateRef profileTypeInfo = GetProfileTypeInfo(glue, jsFunc);
        GateRef slotValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);

        BRANCH(TaggedIsUndefined(slotValue), &slotValueUpdate, &slotValueNotUndefined);
        Bind(&slotValueUpdate);
        {
            GateRef newProfileTypeInfoCell = NewProfileTypeInfoCell(glue, Undefined());
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, newProfileTypeInfoCell);
            SetRawProfileTypeInfoToFunction(glue, result->ReadVariable(), newProfileTypeInfoCell);
            Jump(&profileTypeInfoEnd);
        }
        Bind(&slotValueNotUndefined);
        {
            UpdateProfileTypeInfoCellType(glue, slotValue);
            SetRawProfileTypeInfoToFunction(glue, result->ReadVariable(), slotValue);
            TryToJitReuseCompiledFunc(glue, result->ReadVariable(), slotValue);
            Jump(&profileTypeInfoEnd);
        }
        Bind(&profileTypeInfoEnd);
#endif
        Jump(success);
    }
}

void NewObjectStubBuilder::SetProfileTypeInfoCellToFunction(GateRef jsFunc, GateRef definedFunc, GateRef slotId)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label isValidSlotId(env);
    Label exit(env);

    BRANCH(Equal(slotId, Int32(ProfileTypeInfo::INVALID_SLOT_INDEX)), &exit, &isValidSlotId);
    Bind(&isValidSlotId);
    {
        Label isUndefined(env);
        Label notUndefined(env);
        DEFVARIABLE(profileTypeInfo, VariableType::JS_ANY(), GetProfileTypeInfo(glue_, jsFunc));
        BRANCH(TaggedIsUndefined(*profileTypeInfo), &isUndefined, &notUndefined);
        Bind(&isUndefined);
        {
            profileTypeInfo = CallRuntime(glue_, RTSTUB_ID(UpdateHotnessCounter), { jsFunc });
            Jump(&notUndefined);
        }
        Bind(&notUndefined);
        {
            UpdateProfileTypeInfoCellToFunction(glue_, definedFunc, *profileTypeInfo, slotId);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef NewObjectStubBuilder::NewJSBoundFunction(GateRef glue, GateRef target, GateRef boundThis, GateRef args)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::BOUND_FUNCTION_CLASS);
    result = NewJSObject(glue, hclass);
    GateRef nameAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_NAME_ACCESSOR);
    Store(VariableType::JS_POINTER(), glue, *result,
          IntPtr(JSBoundFunction::GetInlinedPropertyOffset(JSFunction::NAME_INLINE_PROPERTY_INDEX)), nameAccessor,
          MemoryAttribute::NoBarrier());
    GateRef lengthAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                    ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
    Store(VariableType::JS_POINTER(), glue, *result,
          IntPtr(JSBoundFunction::GetInlinedPropertyOffset(JSFunction::LENGTH_INLINE_PROPERTY_INDEX)), lengthAccessor,
          MemoryAttribute::NoBarrier());
    SetJSObjectTaggedField(glue, *result, JSBoundFunction::BOUND_TARGET_OFFSET, target);
    SetJSObjectTaggedField(glue, *result, JSBoundFunction::BOUND_THIS_OFFSET, boundThis);
    SetJSObjectTaggedField(glue, *result, JSBoundFunction::BOUND_ARGUMENTS_OFFSET, args);
    GateRef method = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                            ConstantIndex::BOUND_FUNCTION_METHOD_INDEX);
    SetMethodToFunction(glue, *result, method);
    SetNativePointerToFunctionFromMethod(glue, *result, method);

    Label targetIsHeapObject(env);
    Label targetIsConstructor(env);
    BRANCH(TaggedIsHeapObject(target), &targetIsHeapObject, &exit);
    Bind(&targetIsHeapObject);
    BRANCH(IsConstructor(glue, target), &targetIsConstructor, &exit);
    Bind(&targetIsConstructor);
    {
        GateRef resultHClass = LoadHClass(glue, *result);
        SetHClassBit<JSHClass::ConstructorBit>(glue, resultHClass, Int32(1));
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::EnumerateObjectProperties(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(object, VariableType::JS_ANY(), Undefined());

    Label isString(env);
    Label isNotString(env);
    Label afterObjectTransform(env);
    Label slowpath(env);
    Label empty(env);
    Label tryGetEnumCache(env);
    Label cacheHit(env);
    Label checkNativePointer(env);
    BRANCH(TaggedIsString(glue, obj), &isString, &isNotString);
    Bind(&isString);
    {
        object = CallRuntime(glue, RTSTUB_ID(PrimitiveStringCreate), { obj });
        Jump(&afterObjectTransform);
    }
    Bind(&isNotString);
    {
        object = ToPrototypeOrObj(glue, obj);
        Jump(&afterObjectTransform);
    }
    Bind(&afterObjectTransform);
    BRANCH(TaggedIsUndefinedOrNull(*object), &empty, &checkNativePointer);
    Bind(&checkNativePointer);
    BRANCH(IsNativePointer(glue, *object), &empty, &tryGetEnumCache);
    Bind(&tryGetEnumCache);
    GateRef enumCacheAll = TryGetEnumCache(glue, *object);
    BRANCH(TaggedIsUndefined(enumCacheAll), &slowpath, &cacheHit);
    Bind(&cacheHit);
    {
        GateRef hclass = LoadHClass(glue, *object);
        GateRef enumCache = GetEnumCacheFromHClass(glue, hclass);
        result = NewJSForinIterator(glue, *object, enumCacheAll, hclass,
            GetEnumCacheKindFromEnumCache(enumCache));
        Jump(&exit);
    }
    Bind(&empty);
    {
        GateRef emptyArray = GetEmptyArray(glue);
        result = NewJSForinIterator(glue, Undefined(), emptyArray, Undefined(),
                                    Int32(static_cast<int32_t>(EnumCacheKind::SIMPLE)));
        Jump(&exit);
    }

    Bind(&slowpath);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetPropIteratorSlowpath), { *object });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::FillArgumentsList(GateRef argumentsList,
    GateRef sp, GateRef startIdx, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    GateRef idx = ZExtInt32ToPtr(Int32Add(startIdx, *i));
    GateRef argument = LoadPrimitive(VariableType::JS_ANY(), sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue_, argumentsList, *i, argument);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
    Bind(&setArgumentsAgain);
    LoopEnd(&setArgumentsBegin);
    Bind(&setArgumentsEnd);
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef NewObjectStubBuilder::NewArgumentsListObj(GateRef numArgs)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label setHClass(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(numArgs));
    GateRef arrayClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                ConstantIndex::TAGGED_ARRAY_CLASS_INDEX);
    AllocateInYoung(&result, &exit, &setHClass, arrayClass);
    Bind(&setHClass);
    StoreHClass(glue_, *result, arrayClass);
    Store(VariableType::INT32(), glue_, *result, IntPtr(TaggedArray::LENGTH_OFFSET), numArgs);
    Store(VariableType::INT32(), glue_, *result, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), Int32(0));
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::NewArgumentsList(Variable *result, Label *exit,
    GateRef sp, GateRef startIdx, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label hasException(env);
    Label notException(env);

    GateRef argumentsList = NewArgumentsListObj(numArgs);
    result->WriteVariable(argumentsList);
    Branch(TaggedIsException(argumentsList), &hasException, &notException);
    Bind(&hasException);
    Jump(exit);
    Bind(&notException);
    FillArgumentsList(argumentsList, sp, startIdx, numArgs);
    Jump(exit);
}

void NewObjectStubBuilder::NewArgumentsObj(Variable *result, Label *exit,
    GateRef argumentsList, GateRef numArgs)
{
    auto env = GetEnvironment();

    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef argumentsClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue_, globalEnv,
                                               GlobalEnv::ARGUMENTS_CLASS);
    Label afterNewObject(env);
    NewJSObject(result, &afterNewObject, argumentsClass);
    Bind(&afterNewObject);
    Label setArgumentsObjProperties(env);
    BRANCH(TaggedIsException(result->ReadVariable()), exit, &setArgumentsObjProperties);
    Bind(&setArgumentsObjProperties);
    Store(VariableType::JS_ANY(), glue_, result->ReadVariable(),
          IntPtr(JSArguments::GetInlinedPropertyOffset(JSArguments::LENGTH_INLINE_PROPERTY_INDEX)),
          IntToTaggedInt(numArgs));
    SetElementsArray(VariableType::JS_ANY(), glue_, result->ReadVariable(), argumentsList);
    GateRef arrayProtoValuesFunction = GetGlobalEnvValue(VariableType::JS_ANY(), glue_, globalEnv,
                                                         GlobalEnv::ARRAY_PROTO_VALUES_FUNCTION_INDEX);
    Store(VariableType::JS_ANY(), glue_, result->ReadVariable(),
          IntPtr(JSArguments::GetInlinedPropertyOffset(JSArguments::ITERATOR_INLINE_PROPERTY_INDEX)),
          arrayProtoValuesFunction);
    GateRef accessorCaller = GetGlobalEnvValue(VariableType::JS_ANY(), glue_, globalEnv,
                                               GlobalEnv::ARGUMENTS_CALLER_ACCESSOR);
    Store(VariableType::JS_ANY(), glue_, result->ReadVariable(),
          IntPtr(JSArguments::GetInlinedPropertyOffset(JSArguments::CALLER_INLINE_PROPERTY_INDEX)),
          accessorCaller);
    GateRef accessorCallee = GetGlobalEnvValue(VariableType::JS_ANY(), glue_, globalEnv,
                                               GlobalEnv::ARGUMENTS_CALLEE_ACCESSOR);
    Store(VariableType::JS_ANY(), glue_, result->ReadVariable(),
          IntPtr(JSArguments::GetInlinedPropertyOffset(JSArguments::CALLEE_INLINE_PROPERTY_INDEX)),
          accessorCallee);
    Jump(exit);
}

void NewObjectStubBuilder::AssignRestArg(Variable *result, Label *exit,
    GateRef sp, GateRef startIdx, GateRef numArgs)
{
    auto env = GetEnvironment();
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue_, result->ReadVariable(), lengthOffset, TruncInt64ToInt32(numArgs));
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue_, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    Store(VariableType::JS_ANY(), glue_, result->ReadVariable(),
          IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)), accessor);
    SetExtensibleToBitfield(glue_, result->ReadVariable(), true);
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    GateRef elements = GetElementsArray(glue_, result->ReadVariable());
    BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    {
        GateRef idx = ZExtInt32ToPtr(Int32Add(startIdx, *i));
        GateRef receiver = LoadPrimitive(VariableType::JS_ANY(), sp,
                                         PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), idx));
        SetValueToTaggedArray(VariableType::JS_ANY(), glue_, elements, *i, receiver);
        i = Int32Add(*i, Int32(1));
        BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
        Bind(&setArgumentsAgain);
    }
    LoopEnd(&setArgumentsBegin);
    Bind(&setArgumentsEnd);
    Jump(exit);
}

void NewObjectStubBuilder::NewJSArrayLiteral(Variable *result, Label *exit, RegionSpaceFlag spaceType, GateRef obj,
                                             GateRef hclass, GateRef trackInfo, bool isEmptyArray)
{
    auto env = GetEnvironment();
    Label initializeArray(env);
    Label afterInitialize(env);
    HeapAlloc(result, &initializeArray, spaceType, hclass);
    Bind(&initializeArray);
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), IntPtr(0), hclass);
    InitializeWithSpeicalValue(&afterInitialize, result->ReadVariable(), Undefined(), Int32(JSArray::SIZE),
                               TruncInt64ToInt32(size_), MemoryAttribute::NoBarrier());
    Bind(&afterInitialize);
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    Store(VariableType::INT64(), glue_, result->ReadVariable(), hashOffset, Int64(JSTaggedValue(0).GetRawData()),
          MemoryAttribute::NoBarrier());

    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    GateRef trackInfoOffset = IntPtr(JSArray::TRACK_INFO_OFFSET);
    if (isEmptyArray) {
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), propertiesOffset, obj,
              MemoryAttribute::NoBarrier());
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), elementsOffset, obj,
              MemoryAttribute::NoBarrier());
        Store(VariableType::INT32(), glue_, result->ReadVariable(), lengthOffset, Int32(0),
              MemoryAttribute::NoBarrier());
    } else {
        auto newProperties = Load(VariableType::JS_POINTER(), glue_, obj, propertiesOffset);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), propertiesOffset, newProperties);

        auto newElements = Load(VariableType::JS_POINTER(), glue_, obj, elementsOffset);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), elementsOffset, newElements);

        GateRef arrayLength = LoadPrimitive(VariableType::INT32(), obj, lengthOffset);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), lengthOffset, arrayLength,
              MemoryAttribute::NoBarrier());
    }
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), trackInfoOffset, trackInfo);

    auto accessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), hclass, accessor,
        Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_POINTER(), MemoryAttribute::NoBarrier());
    Jump(exit);
}

void NewObjectStubBuilder::HeapAlloc(Variable *result, Label *exit, RegionSpaceFlag spaceType, GateRef hclass)
{
    switch (spaceType) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            AllocateInYoung(result, exit, hclass);
            break;
        case RegionSpaceFlag::IN_SHARED_OLD_SPACE:
            AllocateInSOld(result, exit, hclass);
            break;
        default:
            *result = CallRuntime(glue_, RTSTUB_ID(HeapAlloc),
                { Int64ToTaggedInt(size_), hclass, IntToTaggedInt(Int64(spaceType))});
            break;
    }
}

void NewObjectStubBuilder::AllocateInSOldPrologue([[maybe_unused]] Variable *result,
    Label *callRuntime, [[maybe_unused]] Label *exit)
{
    auto env = GetEnvironment();
    Label success(env);
    Label next(env);
    Label checkNext(env);
    BRANCH_UNLIKELY(
        LoadPrimitive(VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        callRuntime, &checkNext);
    Bind(&checkNext);

#if defined(ARK_ASAN_ON)
    Jump(callRuntime);
#else
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    auto isStartHeapSamplingOffset = JSThread::GlueData::GetIsStartHeapSamplingOffset(env->Is32Bit());
    auto isStartHeapSampling = LoadPrimitive(VariableType::JS_ANY(), glue_, IntPtr(isStartHeapSamplingOffset));
    BRANCH(TaggedIsTrue(isStartHeapSampling), callRuntime, &next);
    Bind(&next);
#endif
    auto topOffset = JSThread::GlueData::GetSOldSpaceAllocationTopAddressOffset(env->Is32Bit());
    auto endOffset = JSThread::GlueData::GetSOldSpaceAllocationEndAddressOffset(env->Is32Bit());
    auto topAddress = LoadPrimitive(VariableType::NATIVE_POINTER(), glue_, IntPtr(topOffset));
    auto endAddress = LoadPrimitive(VariableType::NATIVE_POINTER(), glue_, IntPtr(endOffset));
    auto top = LoadPrimitive(VariableType::JS_POINTER(), topAddress, IntPtr(0));
    auto end = LoadPrimitive(VariableType::JS_POINTER(), endAddress, IntPtr(0));
    auto newTop = PtrAdd(top, size_);
    BRANCH(IntPtrGreaterThan(newTop, end), callRuntime, &success);
    Bind(&success);
    {
        Store(VariableType::NATIVE_POINTER(), glue_, topAddress, IntPtr(0), newTop);
        if (env->Is32Bit()) {
            top = ZExtInt32ToInt64(top);
        }
        result->WriteVariable(top);
        Jump(exit);
    }
#endif
}

void NewObjectStubBuilder::AllocateInSOld(Variable *result, Label *exit, GateRef hclass)
{
    // disable fastpath for now
    auto env = GetEnvironment();
    Label callRuntime(env);
    Label checkNext(env);
    BRANCH_UNLIKELY(LoadPrimitive(
        VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &callRuntime, &checkNext);
    Bind(&checkNext);
    AllocateInSOldPrologue(result, &callRuntime, exit);
    Bind(&callRuntime);
    {
        DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());
        ret = CallRuntime(glue_, RTSTUB_ID(AllocateInSOld), {IntToTaggedInt(size_), hclass});
        result->WriteVariable(*ret);
        Jump(exit);
    }
}

void NewObjectStubBuilder::AllocateInYoungPrologueImplForCMCGC(Variable *result, Label *callRuntime, Label *exit)
{
    auto env = GetEnvironment();
    Label success(env);

    auto allocBufferOffset = JSThread::GlueData::GetAllocBufferOffset(env->Is32Bit());
    auto allocBufferAddress = LoadPrimitive(VariableType::NATIVE_POINTER(), glue_, IntPtr(allocBufferOffset));
    auto tlRegion = LoadPrimitive(VariableType::NATIVE_POINTER(), allocBufferAddress, IntPtr(0));
    auto allocPtr = LoadPrimitive(VariableType::JS_POINTER(), tlRegion, IntPtr(0));
    auto regionEnd = LoadPrimitive(VariableType::JS_POINTER(), tlRegion, env->Is32Bit()? IntPtr(4) : IntPtr(8));
    auto newAllocPtr = PtrAdd(allocPtr, size_);
    BRANCH(IntPtrGreaterThan(newAllocPtr, regionEnd), callRuntime, &success);
    Bind(&success);
    {
        Store(VariableType::NATIVE_POINTER(), glue_, tlRegion, IntPtr(0), newAllocPtr, MemoryAttribute::NoBarrier());
        if (env->Is32Bit()) {
            allocPtr = ZExtInt32ToInt64(allocPtr);
        }
        result->WriteVariable(allocPtr);
        Jump(exit);
    }
}

void NewObjectStubBuilder::AllocateInYoungPrologueImpl(Variable *result, Label *callRuntime, Label *exit)
{
    auto env = GetEnvironment();
    Label success(env);

    auto topOffset = JSThread::GlueData::GetNewSpaceAllocationTopAddressOffset(env->Is32Bit());
    auto endOffset = JSThread::GlueData::GetNewSpaceAllocationEndAddressOffset(env->Is32Bit());
    auto topAddress = LoadPrimitive(VariableType::NATIVE_POINTER(), glue_, IntPtr(topOffset));
    auto endAddress = LoadPrimitive(VariableType::NATIVE_POINTER(), glue_, IntPtr(endOffset));
    auto top = LoadPrimitive(VariableType::JS_POINTER(), topAddress, IntPtr(0));
    auto end = LoadPrimitive(VariableType::JS_POINTER(), endAddress, IntPtr(0));
    auto newTop = PtrAdd(top, size_);
    BRANCH(IntPtrGreaterThan(newTop, end), callRuntime, &success);
    Bind(&success);
    {
        Store(VariableType::NATIVE_POINTER(), glue_, topAddress, IntPtr(0), newTop, MemoryAttribute::NoBarrier());
        if (env->Is32Bit()) {
            top = ZExtInt32ToInt64(top);
        }
        result->WriteVariable(top);
        Jump(exit);
    }
}

void NewObjectStubBuilder::AllocateInYoungPrologue([[maybe_unused]] Variable *result,
    Label *callRuntime, [[maybe_unused]] Label *exit)
{
    auto env = GetEnvironment();
    Label next(env);
#if defined(ARK_ASAN_ON)
    Jump(callRuntime);
#else
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    auto isStartHeapSamplingOffset = JSThread::GlueData::GetIsStartHeapSamplingOffset(env->Is32Bit());
    auto isStartHeapSampling = LoadPrimitive(VariableType::JS_ANY(), glue_, IntPtr(isStartHeapSamplingOffset));
    BRANCH(TaggedIsTrue(isStartHeapSampling), callRuntime, &next);
    Bind(&next);
#endif
    Label isCMCGC(env);
    Label notCMCGC(env);
    BRANCH_UNLIKELY(
        LoadPrimitive(VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &isCMCGC, &notCMCGC);
    Bind(&isCMCGC);
    {
        AllocateInYoungPrologueImplForCMCGC(result, callRuntime, exit);
    }
    Bind(&notCMCGC);
    {
        AllocateInYoungPrologueImpl(result, callRuntime, exit);
    }
#endif
}

void NewObjectStubBuilder::AllocateInYoung(Variable *result, Label *exit, GateRef hclass)
{
    auto env = GetEnvironment();
    Label callRuntime(env);
    AllocateInYoungPrologue(result, &callRuntime, exit);
    Bind(&callRuntime);
    {
        DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());
        ret = CallRuntime(glue_, RTSTUB_ID(AllocateInYoung), {
            Int64ToTaggedInt(size_), hclass });
        result->WriteVariable(*ret);
        Jump(exit);
    }
}

void NewObjectStubBuilder::AllocateInYoung(Variable *result, Label *error, Label *noError, GateRef hclass)
{
    auto env = GetEnvironment();
    Label callRuntime(env);
    AllocateInYoungPrologue(result, &callRuntime, noError);
    Bind(&callRuntime);
    {
        DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());
        ret = CallRuntime(glue_, RTSTUB_ID(AllocateInYoung), {
            Int64ToTaggedInt(size_), hclass });
        result->WriteVariable(*ret);
        BRANCH(TaggedIsException(*ret), error, noError);
    }
}

GateRef NewObjectStubBuilder::NewTrackInfo(GateRef glue, GateRef cachedHClass, GateRef cachedFunc,
                                           RegionSpaceFlag spaceFlag, GateRef arraySize)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label initialize(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto hclass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::TRACK_INFO_CLASS_INDEX);
    GateRef size = GetObjectSizeFromHClass(hclass);
    SetParameters(glue, size);
    HeapAlloc(&result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    Bind(&initialize);
    Store(VariableType::JS_POINTER(), glue_, *result, IntPtr(0), hclass);
    GateRef cachedHClassOffset = IntPtr(TrackInfo::CACHED_HCLASS_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, cachedHClassOffset, cachedHClass);
    GateRef cachedFuncOffset = IntPtr(TrackInfo::CACHED_FUNC_OFFSET);
    GateRef weakCachedFunc = env->GetBuilder()->CreateWeakRef(cachedFunc);
    Store(VariableType::JS_POINTER(), glue, *result, cachedFuncOffset, weakCachedFunc);
    GateRef arrayLengthOffset = IntPtr(TrackInfo::ARRAY_LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, *result, arrayLengthOffset, arraySize);
    SetSpaceFlagToTrackInfo(glue, *result, Int32(spaceFlag));
    auto elementsKind = GetElementsKindFromHClass(cachedHClass);
    SetElementsKindToTrackInfo(glue, *result, elementsKind);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::InitializeWithSpeicalValue(Label *exit, GateRef object, GateRef value, GateRef start,
                                                      GateRef end, MemoryAttribute mAttr)
{
    {
        ASM_ASSERT(GET_MESSAGE_STRING_ID(InitializeWithSpeicalValue),
                   Int32Equal(Int32Mod(Int32Sub(end, start), Int32(JSTaggedValue::TaggedTypeSize())), Int32(0)));
    }
    auto env = GetEnvironment();
    Label begin(env);
    Label storeValue(env);
    Label endLoop(env);
    Label storeHead(env);
    Label enterLoop(env);
    DEFVARIABLE(startOffset, VariableType::INT32(), start);
    const auto tSize = static_cast<int32_t>(JSTaggedValue::TaggedTypeSize());
    static_assert((tSize & (tSize - 1)) == 0 && "tSize must be power of 2");
    GateRef length = Int32Sub(end, start);
    GateRef remainder = Int32And(length, Int32(LOOP_UNROLL_FACTOR * tSize - 1));
    BRANCH(Int32NotEqual(remainder, Int32(0)), &storeHead, &enterLoop);
    Bind(&storeHead);
    {
        // Now use 2 as loop unroll factor, so only store once if reminder is not 0.
        // But if using other loop unroll factor, the store head should also be refactored.
        Store(VariableType::INT64(), glue_, object, ZExtInt32ToPtr(start), value, mAttr);
        startOffset = Int32Add(start, Int32(tSize));
        Jump(&enterLoop);
    }
    Bind(&enterLoop);
    {
        Jump(&begin);
    }
    LoopBegin(&begin);
    {
        BRANCH(Int32UnsignedLessThan(*startOffset, end), &storeValue, exit);
        Bind(&storeValue);
        {
            auto off = *startOffset;
            for (auto i = 0; i < LOOP_UNROLL_FACTOR; i++) {
                Store(VariableType::INT64(), glue_, object, ZExtInt32ToPtr(off), value, mAttr);
                off = Int32Add(off, Int32(tSize));
            }
            startOffset = Int32Add(*startOffset, Int32(LOOP_UNROLL_FACTOR * tSize));
            Jump(&endLoop);
        }
        Bind(&endLoop);
        LoopEnd(&begin);
    }
}

void NewObjectStubBuilder::InitializeTaggedArrayWithSpeicalValue(Label *exit,
    GateRef array, GateRef value, GateRef start, GateRef length)
{
    Store(VariableType::INT32(), glue_, array, IntPtr(TaggedArray::LENGTH_OFFSET), length);
    Store(VariableType::INT32(), glue_, array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), Int32(0));
    auto offset = Int32Mul(start, Int32(JSTaggedValue::TaggedTypeSize()));
    auto dataOffset = Int32Add(offset, Int32(TaggedArray::DATA_OFFSET));
    offset = Int32Mul(length, Int32(JSTaggedValue::TaggedTypeSize()));
    auto endOffset = Int32Add(offset, Int32(TaggedArray::DATA_OFFSET));
    InitializeWithSpeicalValue(exit, array, value, dataOffset, endOffset, MemoryAttribute::NoBarrier());
}

void NewObjectStubBuilder::InitializeObject(Variable *result)
{
    auto emptyArray =
        GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    Store(VariableType::INT64(), glue_, result->ReadVariable(), hashOffset, Int64(JSTaggedValue(0).GetRawData()),
          MemoryAttribute::NoBarrier());
    SetPropertiesArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray, MemoryAttribute::NoBarrier());
    SetElementsArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray, MemoryAttribute::NoBarrier());
}

void NewObjectStubBuilder::AllocLineStringObject(Variable *result, Label *exit, GateRef length, bool compressed)
{
    auto env = GetEnvironment();
    if (compressed) {
        size_ = AlignUp(ComputeSizeUtf8(ZExtInt32ToPtr(length)),
            IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    } else {
        size_ = AlignUp(ComputeSizeUtf16(ZExtInt32ToPtr(length)),
            IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    }
    Label afterAllocate(env);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::LINE_STRING_CLASS_INDEX);
    AllocateInSOld(result, &afterAllocate, stringClass);

    Bind(&afterAllocate);
    StoreHClass(glue_, result->ReadVariable(), stringClass);
    StoreBaseAddressForBaseClass(glue_, result->ReadVariable(), stringClass);
    InitStringLengthAndFlags(glue_, result->ReadVariable(), length, compressed);
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0));
    Jump(exit);
}

void NewObjectStubBuilder::AllocSlicedStringObject(Variable *result, Label *exit, GateRef from, GateRef length,
    FlatStringStubBuilder *flatString)
{
    auto env = GetEnvironment();

    size_ = AlignUp(IntPtr(SlicedString::SIZE), IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    Label afterAllocate(env);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::SLICED_STRING_CLASS_INDEX);
    AllocateInSOld(result, &afterAllocate, stringClass);

    Bind(&afterAllocate);
    StoreHClass(glue_, result->ReadVariable(), stringClass);
    StoreBaseAddressForBaseClass(glue_, result->ReadVariable(), stringClass);
    GateRef mixLength = LoadPrimitive(VariableType::INT32(), flatString->GetFlatString(),
                                      IntPtr(BaseString::LENGTH_AND_FLAGS_OFFSET));
    GateRef compressedStatus = TruncInt32ToInt1(Int32And(Int32((1 << BaseString::CompressedStatusBit::SIZE) - 1),
                                                         mixLength));
    InitStringLengthAndFlags(glue_, result->ReadVariable(), length, BoolNot(compressedStatus));
    // decode compressedStatus to bool

    SetRawHashcode(glue_, result->ReadVariable(), Int32(0));
    BuiltinsStringStubBuilder builtinsStringStubBuilder(this, GetCurrentGlobalEnv());
    builtinsStringStubBuilder.StoreParent(glue_, result->ReadVariable(), flatString->GetFlatString());
    builtinsStringStubBuilder.StoreStartIndexAndBackingStore(glue_, result->ReadVariable(),
                                                             Int32Add(from, flatString->GetStartIndex()),
                                                             Boolean(false));
    Jump(exit);
}

void NewObjectStubBuilder::AllocTreeStringObject(Variable *result, Label *exit, GateRef first, GateRef second,
    GateRef length, bool compressed)
{
    auto env = GetEnvironment();

    size_ = AlignUp(IntPtr(TreeString::SIZE), IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    Label afterAllocate(env);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::TREE_STRING_CLASS_INDEX);
    AllocateInSOld(result, &afterAllocate, stringClass);

    Bind(&afterAllocate);
    StoreHClass(glue_, result->ReadVariable(), stringClass);
    StoreBaseAddressForBaseClass(glue_, result->ReadVariable(), stringClass);
    InitStringLengthAndFlags(glue_, result->ReadVariable(), length, compressed);
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0));
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), IntPtr(TreeString::FIRST_OFFSET), first);
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), IntPtr(TreeString::SECOND_OFFSET), second);
    Jump(exit);
}

GateRef NewObjectStubBuilder::FastNewThisObject(GateRef glue, GateRef ctor)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    Label callRuntime(env);
    Label checkJSObject(env);
    Label newObject(env);
    Label isJSObject(env);

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto protoOrHClass = Load(VariableType::JS_ANY(), glue, ctor,
        IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(TaggedIsHeapObject(protoOrHClass), &isHeapObject, &callRuntime);
    Bind(&isHeapObject);
    BRANCH(IsJSHClass(glue, protoOrHClass), &checkJSObject, &callRuntime);
    Bind(&checkJSObject);
    auto objectType = GetObjectType(protoOrHClass);
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))), &isJSObject, &callRuntime);
    Bind(&isJSObject);
    {
        auto funcProto = GetPrototypeFromHClass(glue, protoOrHClass);
        BRANCH(IsEcmaObject(glue, funcProto), &newObject, &callRuntime);
    }
    Bind(&newObject);
    {
        SetParameters(glue, 0);
        NewJSObject(&thisObj, &exit, protoOrHClass);
    }
    Bind(&callRuntime);
    {
        thisObj = CallRuntime(glue, RTSTUB_ID(NewThisObject), {ctor, Undefined()});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *thisObj;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::FastSuperAllocateThis(GateRef glue, GateRef superCtor, GateRef newTarget)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    Label checkJSObject(env);
    Label callRuntime(env);
    Label newObject(env);
    Label isFunction(env);

    BRANCH(IsJSFunction(glue, newTarget), &isFunction, &callRuntime);
    Bind(&isFunction);
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(protoOrHClass, VariableType::JS_ANY(), Undefined());
    protoOrHClass = Load(VariableType::JS_ANY(), glue, newTarget,
        IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(TaggedIsHeapObject(*protoOrHClass), &isHeapObject, &callRuntime);
    Bind(&isHeapObject);
    BRANCH(IsJSHClass(glue, *protoOrHClass), &checkJSObject, &callRuntime);
    Bind(&checkJSObject);
    auto objectType = GetObjectType(*protoOrHClass);
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))),
        &newObject, &callRuntime);
    Bind(&newObject);
    {
        SetParameters(glue, 0);
        NewJSObject(&thisObj, &exit, *protoOrHClass);
    }
    Bind(&callRuntime);
    {
        thisObj = CallRuntime(glue, RTSTUB_ID(NewThisObject), {superCtor, newTarget});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *thisObj;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewThisObjectChecked(GateRef glue, GateRef ctor)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label ctorIsHeapObject(env);
    Label ctorIsJSFunction(env);
    Label fastPath(env);
    Label slowPath(env);
    Label ctorIsBase(env);

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());

    BRANCH(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    BRANCH(IsJSFunction(glue, ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(glue, ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        BRANCH(IsBase(glue, ctor), &ctorIsBase, &exit);
        Bind(&ctorIsBase);
        {
            thisObj = FastNewThisObject(glue, ctor);
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        thisObj = Hole();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *thisObj;
    env->SubCfgExit();
    return ret;
}

// The caller should ensure that the IC slot for LoadTrackInfo is valid (slotId is not 0xff or slotValue is not Hole).
GateRef NewObjectStubBuilder::LoadTrackInfo(GateRef glue, GateRef jsFunc, TraceIdInfo traceIdInfo,
    GateRef profileTypeInfo, GateRef slotId, GateRef slotValue, GateRef arrayLiteral, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(ret, VariableType::JS_POINTER(), Undefined());

    Label uninitialized(env);
    Label fastpath(env);
    BRANCH(TaggedIsHeapObject(slotValue), &fastpath, &uninitialized);
    Bind(&fastpath);
    {
        ret = slotValue;
        Jump(&exit);
    }
    Bind(&uninitialized);
    {
        auto hclass = LoadArrayHClassSlowPath(glue, jsFunc, traceIdInfo, arrayLiteral, callback);
        // emptyarray
        if (arrayLiteral == Circuit::NullGate()) {
            ret = NewTrackInfo(glue, hclass, jsFunc, RegionSpaceFlag::IN_YOUNG_SPACE, Int32(0));
        } else {
            GateRef arrayLength = GetArrayLength(arrayLiteral);
            ret = NewTrackInfo(glue, hclass, jsFunc, RegionSpaceFlag::IN_YOUNG_SPACE, arrayLength);
        }

        SetValueToTaggedArray(VariableType::JS_POINTER(), glue, profileTypeInfo, slotId, *ret);
        callback.TryPreDump();
        Jump(&exit);
    }
    Bind(&exit);
    auto result = *ret;
    env->SubCfgExit();
    return result;
}

GateRef NewObjectStubBuilder::LoadArrayHClassSlowPath(
    GateRef glue, GateRef jsFunc, TraceIdInfo traceIdInfo, GateRef arrayLiteral, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label originLoad(env);

    DEFVARIABLE(ret, VariableType::JS_POINTER(), Undefined());

    auto hcIndexInfos = LoadHCIndexInfosFromConstPool(glue, jsFunc);
    auto indexInfosLength = GetLengthOfTaggedArray(hcIndexInfos);
    Label aotLoad(env);
    BRANCH(Int32Equal(indexInfosLength, Int32(0)), &originLoad, &aotLoad);
    Bind(&aotLoad);
    {
        GateRef traceId = 0;
        if (traceIdInfo.isPc) {
            auto pfAddr = LoadPfHeaderFromConstPool(glue, jsFunc);
            traceId = TruncPtrToInt32(PtrSub(traceIdInfo.pc, pfAddr));
        } else {
            traceId = traceIdInfo.traceId;
        }

        GateRef hcIndex = LoadHCIndexFromConstPool(glue, hcIndexInfos, indexInfosLength, traceId, &originLoad);
        GateRef globalEnv = GetCurrentGlobalEnv();
        ret = GetGlobalEnvValue(VariableType::JS_POINTER(), glue, globalEnv, static_cast<size_t>(hcIndex));
        Jump(&exit);
    }
    Bind(&originLoad);
    {
        // emptyarray
        if (arrayLiteral == Circuit::NullGate()) {
            GateRef globalEnv = GetCurrentGlobalEnv();
            if (callback.IsEmpty()) {
                auto arrayFunc =
                    GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
                ret = Load(VariableType::JS_POINTER(), glue, arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            } else {
                ret = GetGlobalEnvValue(VariableType::JS_POINTER(), glue, globalEnv,
                    static_cast<size_t>(GlobalEnvField::ELEMENT_NONE_HCLASS_INDEX));
            }
        } else {
            ret = LoadHClass(glue, arrayLiteral);
        }
        Jump(&exit);
    }
    Bind(&exit);
    auto result = *ret;
    env->SubCfgExit();
    return result;
}

GateRef NewObjectStubBuilder::CreateEmptyArrayCommon(GateRef glue, GateRef hclass, GateRef trackInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef size = GetObjectSizeFromHClass(hclass);
    GateRef emptyArray = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    SetParameters(glue, size);
    NewJSArrayLiteral(&result, &exit, RegionSpaceFlag::IN_YOUNG_SPACE, emptyArray, hclass, trackInfo, true);
    Bind(&exit);
    GateRef ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CreateEmptyObject(GateRef glue)
{
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef objectFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::OBJECT_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), glue, objectFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    return NewJSObject(glue, hclass);
}

GateRef NewObjectStubBuilder::CreateEmptyArray(GateRef glue)
{
    DEFVARIABLE(trackInfo, VariableType::JS_ANY(), Undefined());
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), glue, arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    return CreateEmptyArrayCommon(glue, hclass, *trackInfo);
}

GateRef NewObjectStubBuilder::CreateEmptyArray(GateRef glue, GateRef jsFunc, TraceIdInfo traceIdInfo,
    GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(trackInfo, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(hclass, VariableType::JS_ANY(), Undefined());
    Label slowpath(env);
    Label mayFastpath(env);
    Label createArray(env);
    Label profileNotUndefined(env);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &slowpath, &profileNotUndefined);
    Bind(&profileNotUndefined);
    GateRef slotValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);
    BRANCH(TaggedIsHole(slotValue), &slowpath, &mayFastpath);
    Bind(&mayFastpath);
    {
        trackInfo = LoadTrackInfo(glue, jsFunc, traceIdInfo, profileTypeInfo,
            slotId, slotValue, Circuit::NullGate(), callback);
        hclass = Load(VariableType::JS_ANY(), glue, *trackInfo, IntPtr(TrackInfo::CACHED_HCLASS_OFFSET));
        trackInfo = env->GetBuilder()->CreateWeakRef(*trackInfo);
        Jump(&createArray);
    }
    Bind(&slowpath);
    {
        hclass = LoadArrayHClassSlowPath(glue, jsFunc, traceIdInfo, Circuit::NullGate(), callback);
        Jump(&createArray);
    }
    Bind(&createArray);
    GateRef result = CreateEmptyArrayCommon(glue, *hclass, *trackInfo);
    env->SubCfgExit();
    return result;
}

GateRef NewObjectStubBuilder::CreateArrayWithBuffer(GateRef glue, GateRef index, GateRef jsFunc,
    TraceIdInfo traceIdInfo, GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(trackInfo, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(hclass, VariableType::JS_ANY(), Undefined());

    GateRef method = GetMethodFromFunction(glue, jsFunc);
    GateRef constPool = Load(VariableType::JS_ANY(), glue, method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef module = GetModuleFromFunction(glue, jsFunc);

    auto obj = GetArrayLiteralFromConstPool(glue, constPool, index, module);

    Label slowpath(env);
    Label mayFastpath(env);
    Label createArray(env);
    Label profileNotUndefined(env);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &slowpath, &profileNotUndefined);
    Bind(&profileNotUndefined);
    GateRef slotValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);
    BRANCH(TaggedIsHole(slotValue), &slowpath, &mayFastpath);
    Bind(&mayFastpath);
    {
        trackInfo = LoadTrackInfo(glue, jsFunc, traceIdInfo, profileTypeInfo, slotId, slotValue, obj, callback);
        hclass = Load(VariableType::JS_ANY(), glue, *trackInfo, IntPtr(TrackInfo::CACHED_HCLASS_OFFSET));
        trackInfo = env->GetBuilder()->CreateWeakRef(*trackInfo);
        Jump(&createArray);
    }
    Bind(&slowpath);
    {
        hclass = LoadArrayHClassSlowPath(glue, jsFunc, traceIdInfo, obj, callback);
        Jump(&createArray);
    }
    Bind(&createArray);
    GateRef size = GetObjectSizeFromHClass(*hclass);

    SetParameters(glue, size);
    NewJSArrayLiteral(&result, &exit, RegionSpaceFlag::IN_YOUNG_SPACE, obj, *hclass, *trackInfo, false);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template <typename IteratorType, typename CollectionType>
void NewObjectStubBuilder::CreateJSCollectionIterator(
    Variable *result, Label *exit, GateRef thisValue, GateRef kind)
{
    ASSERT_PRINT((std::is_same_v<IteratorType, JSSetIterator> || std::is_same_v<IteratorType, JSMapIterator>),
        "IteratorType must be JSSetIterator or JSMapIterator type");
    auto env = GetEnvironment();
    ConstantIndex iterClassIdx = static_cast<ConstantIndex>(0);
    int32_t iterOffset = 0;       // ITERATED_SET_OFFSET
    size_t linkedOffset = 0;      // LINKED_MAP_OFFSET
    if constexpr (std::is_same_v<IteratorType, JSSetIterator>) {
        iterClassIdx = ConstantIndex::JS_SET_ITERATOR_CLASS_INDEX;
        iterOffset = IteratorType::ITERATED_SET_OFFSET;
        linkedOffset = CollectionType::LINKED_SET_OFFSET;
        size_ = IntPtr(JSSetIterator::SIZE);
    } else {
        iterClassIdx = ConstantIndex::JS_MAP_ITERATOR_CLASS_INDEX;
        iterOffset = IteratorType::ITERATED_MAP_OFFSET;
        linkedOffset = CollectionType::LINKED_MAP_OFFSET;
        size_ = IntPtr(JSMapIterator::SIZE);
    }
    GateRef iteratorHClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, iterClassIdx);

    Label noException(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, exit, &noException, iteratorHClass);
    Bind(&noException);
    {
        StoreBuiltinHClass(glue_, result->ReadVariable(), iteratorHClass);
        InitializeObject(result);

        // GetLinked
        GateRef linked = Load(VariableType::JS_ANY(), glue_, thisValue, IntPtr(linkedOffset));
        // SetIterated
        GateRef iteratorOffset = IntPtr(iterOffset);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), iteratorOffset, linked,
              MemoryAttribute::UnknownBarrier());

        // SetIteratorNextIndex
        GateRef nextIndexOffset = IntPtr(IteratorType::NEXT_INDEX_OFFSET);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), nextIndexOffset, Int32(0));

        // SetIterationKind
        GateRef kindBitfieldOffset = IntPtr(IteratorType::BIT_FIELD_OFFSET);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), kindBitfieldOffset, kind);
        Jump(exit);
    }
}

template void NewObjectStubBuilder::CreateJSCollectionIterator<JSSetIterator, JSSet>(
    Variable *result, Label *exit, GateRef set, GateRef kind);
template void NewObjectStubBuilder::CreateJSCollectionIterator<JSMapIterator, JSMap>(
    Variable *result, Label *exit, GateRef set, GateRef kind);

void NewObjectStubBuilder::CreateJSTypedArrayIterator(Variable *result, Label *exit, GateRef thisValue, GateRef kind)
{
    auto env = GetEnvironment();
    size_ = IntPtr(JSArrayIterator::SIZE);

    ConstantIndex iterClassIdx = ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX;
    GateRef iteratorHClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, iterClassIdx);

    Label thisExists(env);
    Label isEcmaObject(env);
    Label isTypedArray(env);
    Label throwTypeError(env);

    BRANCH(TaggedIsUndefinedOrNullOrHole(thisValue), &throwTypeError, &thisExists);
    Bind(&thisExists);
    BRANCH(IsEcmaObject(glue_, thisValue), &isEcmaObject, &throwTypeError);
    Bind(&isEcmaObject);
    BRANCH(IsTypedArray(glue_, thisValue), &isTypedArray, &throwTypeError);
    Bind(&isTypedArray);

    Label noException(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, exit, &noException, iteratorHClass);
    Bind(&noException);
    {
        StoreBuiltinHClass(glue_, result->ReadVariable(), iteratorHClass);
        InitializeObject(result);

        GateRef iteratorOffset = IntPtr(JSArrayIterator::ITERATED_ARRAY_OFFSET);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), iteratorOffset, thisValue,
              MemoryAttribute::NeedBarrier());

        // SetIteratorNextIndex
        GateRef nextIndexOffset = IntPtr(JSArrayIterator::NEXT_INDEX_OFFSET);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), nextIndexOffset, Int32(0));

        // SetIterationKind
        GateRef kindBitfieldOffset = IntPtr(JSArrayIterator::BIT_FIELD_OFFSET);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), kindBitfieldOffset, kind);
        Jump(exit);
    }

    Bind(&throwTypeError);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(TargetTypeNotTypedArray));
        CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result->WriteVariable(Exception());
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::NewTaggedSubArray(GateRef glue, GateRef srcTypedArray,
    GateRef elementSize, GateRef newLength, GateRef beginIndex, GateRef arrayCls, GateRef buffer)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    GateRef constructorName = Load(VariableType::JS_POINTER(), glue, srcTypedArray,
        IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET));
    GateRef srcByteOffset =
        LoadPrimitive(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
    GateRef contentType =
        LoadPrimitive(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET));
    GateRef beginByteOffset = Int32Add(srcByteOffset, Int32Mul(beginIndex, elementSize));

    Label isOnHeap(env);
    Label isNotOnHeap(env);
    Label createNewArray(env);
    Label comparePrototype(env);
    Label protoChanged(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(newArrayHClass, VariableType::JS_ANY(), Undefined());
    GateRef srcType = GetObjectType(arrayCls);

    BRANCH(env->GetBuilder()->IsOnHeap(arrayCls), &isOnHeap, &isNotOnHeap);
    Bind(&isOnHeap);
    {
        newArrayHClass = GetOnHeapHClassFromType(glue, srcType);
        Jump(&comparePrototype);
    }
    Bind(&isNotOnHeap);
    {
        newArrayHClass = GetNotOnHeapHClassFromType(glue, srcType);
        Jump(&comparePrototype);
    }

    Bind(&comparePrototype);
    {
        GateRef newPrototype = GetPrototypeFromHClass(glue, *newArrayHClass);
        GateRef srcPrototype = GetPrototypeFromHClass(glue, arrayCls);
        BRANCH_LIKELY(Equal(newPrototype, srcPrototype), &createNewArray, &protoChanged);
        Bind(&protoChanged);
        {
            newArrayHClass = CallRuntime(glue, RTSTUB_ID(CloneHclass), { *newArrayHClass });
            StorePrototype(glue, *newArrayHClass, srcPrototype);
            Jump(&createNewArray);
        }
    }

    Bind(&createNewArray);
    {
        result = NewJSObject(glue, *newArrayHClass);
        GateRef newByteLength = Int32Mul(elementSize, newLength);
        Store(VariableType::JS_POINTER(), glue, *result, IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET), buffer);
        Store(VariableType::JS_POINTER(), glue, *result, IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET),
              constructorName);
        Store(VariableType::INT32(), glue, *result, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET), newByteLength);
        Store(VariableType::INT32(), glue, *result, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET), beginByteOffset);
        Store(VariableType::INT32(), glue, *result, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET), newLength);
        Store(VariableType::INT32(), glue, *result, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET), contentType);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewTypedArray(GateRef glue, GateRef srcTypedArray, GateRef srcType, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label slowPath(env);
    Label defaultConstr(env);
    Label markerCellValid(env);
    Label isProtoChangeMarker(env);
    Label accessorNotChanged(env);
    Label exit(env);
    BRANCH(HasConstructor(glue, srcTypedArray), &slowPath, &defaultConstr);
    Bind(&defaultConstr);
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef detector = GetTypedArraySpeciesProtectDetector(globalEnv);
    BRANCH(BoolNot(detector), &markerCellValid, &slowPath);
    Bind(&markerCellValid);
    GateRef marker = GetProtoChangeMarkerFromHClass(glue, LoadHClass(glue, srcTypedArray));
    BRANCH(TaggedIsProtoChangeMarker(glue, marker), &isProtoChangeMarker, &accessorNotChanged);
    Bind(&isProtoChangeMarker);
    BRANCH(GetAccessorHasChanged(marker), &slowPath, &accessorNotChanged);

    Bind(&accessorNotChanged);
    {
        DEFVARIABLE(buffer, VariableType::JS_ANY(), Undefined());
        Label next(env);
        GateRef hclass = LoadHClass(glue, srcTypedArray);
        GateRef obj = NewJSObject(glue, hclass);
        result = obj;
        GateRef ctorName = Load(VariableType::JS_POINTER(), glue, srcTypedArray,
            IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET));
        GateRef elementSize = GetElementSizeFromType(glue, srcType);
        GateRef newByteLength = Int32Mul(elementSize, length);
        GateRef contentType =
            LoadPrimitive(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET));
        BRANCH(Int32LessThanOrEqual(newByteLength, Int32(RangeInfo::TYPED_ARRAY_ONHEAP_MAX)), &next, &slowPath);
        Bind(&next);
        {
            Label sameObjectSize(env);
            Label newByteArrayExit(env);
            GateRef onHeapHClass = GetOnHeapHClassFromType(glue, srcType);
            GateRef originalHClassObjectSize = GetObjectSizeFromHClass(hclass);
            GateRef onHeapHClassObjectSize = GetObjectSizeFromHClass(onHeapHClass);
            BRANCH(Equal(originalHClassObjectSize, onHeapHClassObjectSize), &sameObjectSize, &slowPath);
            Bind(&sameObjectSize);
            NewByteArray(&buffer, &newByteArrayExit, elementSize, length);
            Bind(&newByteArrayExit);
            StoreHClass(glue, obj, onHeapHClass);
            Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET), *buffer);
            Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET), ctorName);
            Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET), newByteLength);
            Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET), Int32(0));
            Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET), length);
            Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET), contentType);
            Jump(&exit);
        }
    }

    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(TypedArraySpeciesCreate),
            { srcTypedArray, IntToTaggedInt(Int32(1)), IntToTaggedInt(length) });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewTypedArraySameType(GateRef glue, GateRef srcTypedArray, GateRef srcType,
                                                    GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(buffer, VariableType::JS_ANY(), Undefined());
    Label slowPath(env);
    Label next(env);
    Label exit(env);
    GateRef hclass = LoadHClass(glue, srcTypedArray);
    GateRef obj = NewJSObject(glue, hclass);
    result = obj;
    GateRef ctorName = Load(VariableType::JS_POINTER(), glue, srcTypedArray,
        IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET));
    GateRef elementSize = GetElementSizeFromType(glue, srcType);
    GateRef newByteLength = Int32Mul(elementSize, length);
    GateRef contentType =
        LoadPrimitive(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET));
    BRANCH(Int32LessThanOrEqual(newByteLength, Int32(RangeInfo::TYPED_ARRAY_ONHEAP_MAX)), &next, &slowPath);
    Bind(&next);
    {
        Label passFastGuard(env);
        Label checkSize(env);
        Label newByteArrayExit(env);
        /* 1. HClass pointer equality guard
         *    - if the instance already uses the canonical on-heap HClass, we can skip size comparison */
        GateRef onHeapHClass = GetOnHeapHClassFromType(glue, srcType);
        BRANCH(Equal(hclass, onHeapHClass), &passFastGuard, &checkSize);

        /* 2. size guard
         *    - pointer differs (sub-class, etc).
         *      ensure object size still match before staying on fast-path; otherwise fall back to slowPath. */
        Bind(&checkSize);
        GateRef originalSize = GetObjectSizeFromHClass(hclass);
        GateRef onHeapSize = GetObjectSizeFromHClass(onHeapHClass);
        BRANCH(Equal(originalSize, onHeapSize), &passFastGuard, &slowPath);

        /* 3. all guards passed
         *    - safe to stay on fast-path */
        Bind(&passFastGuard);
        NewByteArray(&buffer, &newByteArrayExit, elementSize, length);
        Bind(&newByteArrayExit);
        StoreHClass(glue, obj, onHeapHClass);
        Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET), *buffer);
        Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET), ctorName);
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET), newByteLength);
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET), Int32(0));
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET), length);
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET), contentType);
        Jump(&exit);
    }

    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(TypedArrayCreateSameType),
            { srcTypedArray, IntToTaggedInt(Int32(1)), IntToTaggedInt(length) });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// use NewJSObjectByConstructor need to InitializeJSObject by type
GateRef NewObjectStubBuilder::NewJSObjectByConstructor(GateRef glue, GateRef constructor, GateRef newTarget)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(jshclass, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label slowPath(env);
    Label newObj(env);
    Label isValid(env);
    Label checkIsJSShared(env);
    Label constructorIsShared(env);
    SetGlue(glue);
    BRANCH(IntPtrEqual(constructor, newTarget), &isValid, &checkIsJSShared);
    Bind(&checkIsJSShared);
    BRANCH(IsJSShared(glue, constructor), &constructorIsShared, &isValid);
    Bind(&constructorIsShared);
    {
        BRANCH(IsJSShared(glue, newTarget), &isValid, &slowPath);
    }
    Bind(&isValid);
    {
        Label hasFunctionPrototype(env);
        Label notHasFunctionPrototype(env);
        BRANCH(HasFunctionPrototype(glue, constructor), &hasFunctionPrototype, &notHasFunctionPrototype);
        Bind(&hasFunctionPrototype);
        {
            GateRef proto = GetCtorPrototype(glue, constructor);
            BRANCH(IsEcmaObject(glue, proto), &notHasFunctionPrototype, &slowPath);
        }
        Bind(&notHasFunctionPrototype);
        {
            Label isEqual(env);
            Label isHClass(env);
            BRANCH(IntPtrEqual(constructor, newTarget), &isEqual, &slowPath);
            Bind(&isEqual);
            GateRef protoOrHClass = GetProtoOrHClass(glue, constructor);
            BRANCH(IsJSHClass(glue, protoOrHClass), &isHClass, &slowPath);
            Bind(&isHClass);
            jshclass = protoOrHClass;
            Jump(&newObj);
        }
    }
    Bind(&newObj);
    {
        Label notShared(env);
        BRANCH(IsJSShared(glue, *jshclass), &slowPath, &notShared);
        Bind(&notShared);
        {
            result = NewJSObject(glue, *jshclass);
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(NewJSObjectByConstructor), { constructor, newTarget });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewFloat32ArrayObj(GateRef glue)
{
    GateRef arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, GetCurrentGlobalEnv(),
                                          GlobalEnv::FLOAT32_ARRAY_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), glue, arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    GateRef obj = NewJSObject(glue, hclass);
    return obj;
}

GateRef NewObjectStubBuilder::NewFloat32ArrayWithSize(GateRef glue, GateRef size)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(buffer, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    GateRef obj = NewFloat32ArrayObj(glue);
    result = obj;
    GateRef ctorName = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                              ConstantIndex::FLOAT32_ARRAY_STRING_INDEX);
    GateRef elementSize = Int32(4);  // 4: float32 primtype's byte length
    GateRef newByteLength = Int32Mul(size, elementSize);
    GateRef contentType = Int32(static_cast<int32_t>(ContentType::Number));
    {
        Label newByteArrayExit(env);
        NewByteArray(&buffer, &newByteArrayExit, elementSize, size);
        Bind(&newByteArrayExit);
        GateRef onHeapHClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue,  GetCurrentGlobalEnv(),
                                                 GlobalEnv::FLOAT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        StoreHClass(glue, obj, onHeapHClass);
        Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET), *buffer);
        Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET),
              ctorName, MemoryAttribute::NoBarrier());
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET),
              newByteLength, MemoryAttribute::NoBarrier());
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET),
              Int32(0), MemoryAttribute::NoBarrier());
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET),
              size, MemoryAttribute::NoBarrier());
        Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET),
              contentType, MemoryAttribute::NoBarrier());
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewTypedArrayFromCtor(GateRef glue, GateRef ctor, GateRef length, Label *slowPath)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label thisObjIsECmaObject(env);
    Label thisObjIsFastTypedArray(env);
    Label defaultConstr(env);
    Label exit(env);

    GateRef thisObj = FastNewThisObject(glue, ctor);
    result = thisObj;
    GateRef arrayType = GetObjectType(LoadHClass(glue, thisObj));

    BRANCH(IsEcmaObject(glue, thisObj), &thisObjIsECmaObject, slowPath);
    Bind(&thisObjIsECmaObject);
    BRANCH(IsFastTypeArray(arrayType), &thisObjIsFastTypedArray, slowPath);
    Bind(&thisObjIsFastTypedArray);
    BRANCH(HasConstructor(glue, thisObj), slowPath, &defaultConstr);
    Bind(&defaultConstr);

    DEFVARIABLE(buffer, VariableType::JS_ANY(), Undefined());
    GateRef elementSize = GetElementSizeFromType(glue, arrayType);
    GateRef newByteLength = Int32Mul(elementSize, length);
    Label newByteArrayExit(env);
    NewByteArray(&buffer, &newByteArrayExit, elementSize, length);
    Bind(&newByteArrayExit);
    Store(VariableType::JS_POINTER(), glue, thisObj, IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET), *buffer);
    Store(VariableType::INT32(), glue, thisObj, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET), newByteLength);
    Store(VariableType::INT32(), glue, thisObj, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET), length);
    Jump(&exit);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::NewByteArray(Variable *result, Label *exit, GateRef elementSize, GateRef length)
{
    auto env = GetEnvironment();

    Label noError(env);
    Label initializeExit(env);
    GateRef size = AlignUp(ComputeTaggedTypedArraySize(ZExtInt32ToPtr(elementSize), ZExtInt32ToPtr(length)),
        IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    size_ = size;
    auto hclass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::BYTE_ARRAY_CLASS_INDEX);
    AllocateInYoung(result, exit, &noError, hclass);
    Bind(&noError);
    {
        StoreBuiltinHClass(glue_, result->ReadVariable(), hclass);
        auto startOffset = Int32(ByteArray::DATA_OFFSET);
        static_assert(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT) == 8);
        InitializeWithSpeicalValue(&initializeExit, result->ReadVariable(), Int64(0), startOffset,
                                   TruncPtrToInt32(size), MemoryAttribute::NoBarrier());
        Bind(&initializeExit);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(ByteArray::ARRAY_LENGTH_OFFSET), length);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(ByteArray::BYTE_LENGTH_OFFSET), elementSize);
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::NewProfileTypeInfoCell(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label initialize(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto hclass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                         ConstantIndex::PROFILE_TYPE_INFO_CELL_0_CLASS_INDEX);
    GateRef size = GetObjectSizeFromHClass(hclass);
    SetParameters(glue, size);
    HeapAlloc(&result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    Bind(&initialize);
    StoreHClass(glue, *result, hclass, MemoryAttribute::NoBarrier());
    SetValueToProfileTypeInfoCell(glue, *result, value);
    GateRef machineCodeOffset = IntPtr(ProfileTypeInfoCell::MACHINE_CODE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, machineCodeOffset, Hole());
    GateRef baselineCodeOffset = IntPtr(ProfileTypeInfoCell::BASELINE_CODE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, baselineCodeOffset, Hole());
    GateRef handleOffset = IntPtr(ProfileTypeInfoCell::HANDLE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, handleOffset, Undefined());

    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewEnumCache(GateRef glue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label initialize(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    auto hclass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
        ConstantIndex::ENUM_CACHE_CLASS_INDEX);
    GateRef size = GetObjectSizeFromHClass(hclass);
    SetParameters(glue, size);
    HeapAlloc(&result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    Bind(&initialize);
    Store(VariableType::JS_POINTER(), glue, result.ReadVariable(), IntPtr(0), hclass);
    GateRef enumCacheKindOffset = IntPtr(EnumCache::ENUM_CACHE_KIND_OFFSET);
    Store(VariableType::INT32(), glue, *result, enumCacheKindOffset,
        Int32(static_cast<uint32_t>(EnumCacheKind::NONE)));
    GateRef enumCacheAllOffset = IntPtr(EnumCache::ENUM_CACHE_ALL_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, enumCacheAllOffset, Null());
    GateRef enumCacheOwnOffset = IntPtr(EnumCache::ENUM_CACHE_OWN_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, enumCacheOwnOffset, Null());
    GateRef enumCacheProtoChainOffset = IntPtr(EnumCache::PROTO_CHAIN_INFO_ENUM_CACHE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, *result, enumCacheProtoChainOffset, Null());

    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::GetElementSizeFromType(GateRef glue, GateRef type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    Label defaultLabel(env);
    Label exit(env);
    Label labelBuffer[11] = {
        Label(env), Label(env), Label(env), Label(env), Label(env), Label(env),
        Label(env), Label(env), Label(env), Label(env), Label(env) };
    int64_t valueBuffer[11] = {
        static_cast<int64_t>(JSType::JS_INT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_CLAMPED_ARRAY),
        static_cast<int64_t>(JSType::JS_INT16_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT16_ARRAY),
        static_cast<int64_t>(JSType::JS_INT32_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT64_ARRAY),
        static_cast<int64_t>(JSType::JS_BIGINT64_ARRAY),
        static_cast<int64_t>(JSType::JS_BIGUINT64_ARRAY) };

    // 11 : this switch has 11 case
    Switch(type, &defaultLabel, valueBuffer, labelBuffer, 11);
    // 0 : index of this buffer
    Bind(&labelBuffer[0]);
    {
        // 1 : the elementSize of this type is 1
        result = Int32(1);
        Jump(&exit);
    }
    // 1 : index of this buffer
    Bind(&labelBuffer[1]);
    {
        // 1 : the elementSize of this type is 1
        result = Int32(1);
        Jump(&exit);
    }
    // 2 : index of this buffer
    Bind(&labelBuffer[2]);
    {
        // 1 : the elementSize of this type is 1
        result = Int32(1);
        Jump(&exit);
    }
    // 3 : index of this buffer
    Bind(&labelBuffer[3]);
    {
        // 2 : the elementSize of this type is 2
        result = Int32(2);
        Jump(&exit);
    }
    // 4 : index of this buffer
    Bind(&labelBuffer[4]);
    {
        // 2 : the elementSize of this type is 2
        result = Int32(2);
        Jump(&exit);
    }
    // 5 : index of this buffer
    Bind(&labelBuffer[5]);
    {
        // 4 : the elementSize of this type is 4
        result = Int32(4);
        Jump(&exit);
    }
    // 6 : index of this buffer
    Bind(&labelBuffer[6]);
    {
        // 4 : the elementSize of this type is 4
        result = Int32(4);
        Jump(&exit);
    }
    // 7 : index of this buffer
    Bind(&labelBuffer[7]);
    {
        // 4 : the elementSize of this type is 4
        result = Int32(4);
        Jump(&exit);
    }
    // 8 : index of this buffer
    Bind(&labelBuffer[8]);
    {
        // 8 : the elementSize of this type is 8
        result = Int32(8);
        Jump(&exit);
    }
    // 9 : index of this buffer
    Bind(&labelBuffer[9]);
    {
        // 8 : the elementSize of this type is 8
        result = Int32(8);
        Jump(&exit);
    }
    // 10 : index of this buffer
    Bind(&labelBuffer[10]);
    {
        // 8 : the elementSize of this type is 8
        result = Int32(8);
        Jump(&exit);
    }
    Bind(&defaultLabel);
    {
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::GetOnHeapHClassFromType(GateRef glue, GateRef type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef globalEnv = GetCurrentGlobalEnv();
    Label defaultLabel(env);
    Label exit(env);
    Label labelBuffer[11] = {
        Label(env), Label(env), Label(env), Label(env), Label(env), Label(env),
        Label(env), Label(env), Label(env), Label(env), Label(env) };
    int64_t valueBuffer[11] = {
        static_cast<int64_t>(JSType::JS_INT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_CLAMPED_ARRAY),
        static_cast<int64_t>(JSType::JS_INT16_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT16_ARRAY),
        static_cast<int64_t>(JSType::JS_INT32_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT64_ARRAY),
        static_cast<int64_t>(JSType::JS_BIGINT64_ARRAY),
        static_cast<int64_t>(JSType::JS_BIGUINT64_ARRAY) };

    // 11 : this switch has 11 case
    Switch(type, &defaultLabel, valueBuffer, labelBuffer, 11);
    // 0 : index of this buffer
    Bind(&labelBuffer[0]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::INT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 1 : index of this buffer
    Bind(&labelBuffer[1]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::UINT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 2 : index of this buffer
    Bind(&labelBuffer[2]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 3 : index of this buffer
    Bind(&labelBuffer[3]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::INT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 4 : index of this buffer
    Bind(&labelBuffer[4]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::UINT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 5 : index of this buffer
    Bind(&labelBuffer[5]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::INT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 6 : index of this buffer
    Bind(&labelBuffer[6]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::UINT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 7 : index of this buffer
    Bind(&labelBuffer[7]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::FLOAT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 8 : index of this buffer
    Bind(&labelBuffer[8]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::FLOAT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 9 : index of this buffer
    Bind(&labelBuffer[9]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::BIGINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 10 : index of this buffer
    Bind(&labelBuffer[10]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::BIGUINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    Bind(&defaultLabel);
    {
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::GetNotOnHeapHClassFromType(GateRef glue, GateRef type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef globalEnv = GetCurrentGlobalEnv();
    Label defaultLabel(env);
    Label exit(env);
    Label labelBuffer[11] = {
        Label(env), Label(env), Label(env), Label(env), Label(env), Label(env),
        Label(env), Label(env), Label(env), Label(env), Label(env) };
    int64_t valueBuffer[11] = {
        static_cast<int64_t>(JSType::JS_INT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_CLAMPED_ARRAY),
        static_cast<int64_t>(JSType::JS_INT16_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT16_ARRAY),
        static_cast<int64_t>(JSType::JS_INT32_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT64_ARRAY),
        static_cast<int64_t>(JSType::JS_BIGINT64_ARRAY),
        static_cast<int64_t>(JSType::JS_BIGUINT64_ARRAY) };

    // 11 : this switch has 11 case
    Switch(type, &defaultLabel, valueBuffer, labelBuffer, 11);
    // 0 : index of this buffer
    Bind(&labelBuffer[0]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::INT8_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 1 : index of this buffer
    Bind(&labelBuffer[1]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::UINT8_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 2 : index of this buffer
    Bind(&labelBuffer[2]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 3 : index of this buffer
    Bind(&labelBuffer[3]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::INT16_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 4 : index of this buffer
    Bind(&labelBuffer[4]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::UINT16_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 5 : index of this buffer
    Bind(&labelBuffer[5]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::INT32_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 6 : index of this buffer
    Bind(&labelBuffer[6]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::UINT32_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 7 : index of this buffer
    Bind(&labelBuffer[7]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::FLOAT32_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 8 : index of this buffer
    Bind(&labelBuffer[8]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::FLOAT64_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 9 : index of this buffer
    Bind(&labelBuffer[9]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::BIGINT64_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    // 10 : index of this buffer
    Bind(&labelBuffer[10]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                   GlobalEnv::BIGUINT64_ARRAY_ROOT_HCLASS_INDEX);
        Jump(&exit);
    }
    Bind(&defaultLabel);
    {
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CreateArrayFromList(GateRef glue, GateRef elements, GateRef kind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    GateRef intialHClass = GetElementsKindHClass(glue, kind);
    GateRef len = GetLengthOfTaggedArray(elements);
    GateRef result = NewJSObject(glue, intialHClass);
    SetPropertyInlinedProps(glue, result, intialHClass, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    SetArrayLength(glue, result, len);
    SetExtensibleToBitfield(glue, result, true);
    SetElementsArray(VariableType::JS_POINTER(), glue, result, elements);
    auto ret = result;
    env->SubCfgExit();
    return ret;
}
GateRef NewObjectStubBuilder::CreateListFromArrayLike(GateRef glue, GateRef arrayObj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label exit(env);

    // 3. If Type(obj) is Object, throw a TypeError exception.
    Label targetIsHeapObject(env);
    Label targetIsEcmaObject(env);
    Label targetNotEcmaObject(env);
    BRANCH(TaggedIsHeapObject(arrayObj), &targetIsHeapObject, &targetNotEcmaObject);
    Bind(&targetIsHeapObject);
    BRANCH(TaggedObjectIsEcmaObject(glue, arrayObj), &targetIsEcmaObject, &targetNotEcmaObject);
    Bind(&targetNotEcmaObject);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(TargetTypeNotObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
        Jump(&exit);
    }
    Bind(&targetIsEcmaObject);
    {
        Label targetIsTypeArray(env);
        Label targetNotTypeArray(env);
        BRANCH(IsTypedArray(glue, arrayObj), &targetIsTypeArray, &targetNotTypeArray);
        Bind(&targetIsTypeArray);
        {
            GateRef int32Len = GetLengthOfJSTypedArray(arrayObj);
            GateRef array = NewTaggedArray(glue, int32Len);
            BuiltinsTypedArrayStubBuilder arrayStubBuilder(this, GetCurrentGlobalEnv());
            arrayStubBuilder.FastCopyElementToArray(glue, arrayObj, array);
            // c. ReturnIfAbrupt(next).
            Label noPendingException1(env);
            BRANCH(HasPendingException(glue), &exit, &noPendingException1);
            Bind(&noPendingException1);
            {
                res = array;
                Jump(&exit);
            }
        }
        Bind(&targetNotTypeArray);
        // 4. Let len be ToLength(Get(obj, "length")).
        GateRef lengthString =
            GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::LENGTH_STRING_INDEX);
        GateRef value = FastGetPropertyByName(glue, arrayObj, lengthString, ProfileOperation());
        GateRef number = ToLength(glue, value);
        // 5. ReturnIfAbrupt(len).
        Label noPendingException2(env);
        BRANCH(HasPendingException(glue), &exit, &noPendingException2);
        Bind(&noPendingException2);
        {
            Label indexInRange(env);
            Label indexOutRange(env);

            GateRef doubleLen = GetDoubleOfTNumber(number);
            BRANCH(DoubleGreaterThan(doubleLen, Double(JSObject::MAX_ELEMENT_INDEX)), &indexOutRange, &indexInRange);
            Bind(&indexOutRange);
            {
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(LenGreaterThanMax));
                CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
                Jump(&exit);
            }
            Bind(&indexInRange);
            {
                // 8. Repeat while index < len
                GateRef int32Length = DoubleToInt(glue, doubleLen);
                GateRef array = NewTaggedArray(glue, int32Length);
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Label noPendingException3(env);
                Label storeValue(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH(Int32UnsignedLessThan(*index, int32Length), &storeValue, &afterLoop);
                    Bind(&storeValue);
                    {
                        GateRef next = FastGetPropertyByIndex(glue, arrayObj, *index, ProfileOperation());
                        // c. ReturnIfAbrupt(next).
                        BRANCH(HasPendingException(glue), &exit, &noPendingException3);
                        Bind(&noPendingException3);
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, next);
                        index = Int32Add(*index, Int32(1));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                LoopEndWithCheckSafePoint(&loopHead, env, glue);
                Bind(&afterLoop);
                {
                    res = array;
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    GateRef ret = *res;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::CreateJSIteratorResult(GateRef glue, Variable *res, GateRef value, GateRef done, Label *exit)
{
    auto env = GetEnvironment();
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef iterResultClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                GlobalEnv::ITERATOR_RESULT_CLASS_INDEX);
    Label afterNew(env);
    SetParameters(glue, 0);
    NewJSObject(res, &afterNew, iterResultClass);
    Bind(&afterNew);
    Store(VariableType::JS_POINTER(), glue, res->ReadVariable(),
          IntPtr(JSIterator::GetInlinedPropertyOffset(JSIterator::VALUE_INLINE_PROPERTY_INDEX)), value);
    Store(VariableType::JS_POINTER(), glue, res->ReadVariable(),
          IntPtr(JSIterator::GetInlinedPropertyOffset(JSIterator::DONE_INLINE_PROPERTY_INDEX)), done);

    Jump(exit);
}
}  // namespace panda::ecmascript::kungfu
