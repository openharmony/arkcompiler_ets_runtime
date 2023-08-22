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

#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/mem/mem.h"
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
    Label afterAllocate(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, &afterAllocate);
    Bind(&afterAllocate);
    Label hasPendingException(env);
    Label noException(env);
    Branch(TaggedIsException(result->ReadVariable()), &hasPendingException, &noException);
    Bind(&noException);
    {
        auto hclass = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::ENV_CLASS_INDEX);
        StoreHClass(glue_, result->ReadVariable(), hclass);
        Label afterInitialize(env);
        InitializeTaggedArrayWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), Hole(), Int32(LexicalEnv::RESERVED_ENV_LENGTH), length);
        Bind(&afterInitialize);
        SetValueToTaggedArray(VariableType::INT64(),
            glue_, result->ReadVariable(), Int32(LexicalEnv::SCOPE_INFO_INDEX), Hole());
        SetValueToTaggedArray(VariableType::JS_POINTER(),
            glue_, result->ReadVariable(), Int32(LexicalEnv::PARENT_ENV_INDEX), parent);
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
    DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
    NewTaggedArrayChecked(&array, TruncInt64ToInt32(size), &exit);
    Bind(&exit);
    auto arrayRet = *array;
    env->SubCfgExit();
    SetElementsArray(VariableType::JS_POINTER(), glue_, result, arrayRet);
    return result;
}

void NewObjectStubBuilder::NewJSObject(Variable *result, Label *exit, GateRef hclass)
{
    auto env = GetEnvironment();

    size_ = GetObjectSizeFromHClass(hclass);
    Label afterAllocate(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, &afterAllocate);
    Bind(&afterAllocate);
    Label hasPendingException(env);
    Label noException(env);
    Branch(TaggedIsException(result->ReadVariable()), &hasPendingException, &noException);
    Bind(&noException);
    {
        StoreHClass(glue_, result->ReadVariable(), hclass);
        DEFVARIABLE(initValue, VariableType::JS_ANY(), Undefined());
        Label isTS(env);
        Label initialize(env);
        Branch(IsTSHClass(hclass), &isTS, &initialize);
        Bind(&isTS);
        {
            // The object which created by AOT speculative hclass, should be initialized as hole, means does not exist,
            // to follow ECMA spec.
            initValue = Hole();
            Jump(&initialize);
        }
        Bind(&initialize);
        Label afterInitialize(env);
        InitializeWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), *initValue, Int32(JSObject::SIZE), ChangeIntPtrToInt32(size_));
        Bind(&afterInitialize);
        auto emptyArray = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        SetHash(glue_, result->ReadVariable(), Int64(JSTaggedValue(0).GetRawData()));
        SetPropertiesArray(VariableType::INT64(),
            glue_, result->ReadVariable(), emptyArray);
        SetElementsArray(VariableType::INT64(),
            glue_, result->ReadVariable(), emptyArray);
        Jump(exit);
    }
    Bind(&hasPendingException);
    {
        Jump(exit);
    }
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

void NewObjectStubBuilder::NewTaggedArrayChecked(Variable *result, GateRef len, Label *exit)
{
    auto env = GetEnvironment();
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(len));
    Label afterAllocate(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, &afterAllocate);
    Bind(&afterAllocate);
    Label noException(env);
    Branch(TaggedIsException(result->ReadVariable()), exit, &noException);
    Bind(&noException);
    {
        auto hclass = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::ARRAY_CLASS_INDEX);
        StoreHClass(glue_, result->ReadVariable(), hclass);
        Label afterInitialize(env);
        InitializeTaggedArrayWithSpeicalValue(&afterInitialize,
            result->ReadVariable(), Hole(), Int32(0), len);
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
    Branch(Int32Equal(len, Int32(0)), &isEmpty, &notEmpty);
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
        Branch(Int32LessThan(len, Int32(MAX_TAGGED_ARRAY_LENGTH)), &next, &slowPath);
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

void NewObjectStubBuilder::NewArgumentsList(Variable *result, Label *exit,
    GateRef sp, GateRef startIdx, GateRef numArgs)
{
    auto env = GetEnvironment();
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label setHClass(env);
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(numArgs));
    Label afterAllocate(env);
    AllocateInYoung(result, &afterAllocate);
    Bind(&afterAllocate);
    Branch(TaggedIsException(result->ReadVariable()), exit, &setHClass);
    Bind(&setHClass);
    GateRef arrayClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                ConstantIndex::ARRAY_CLASS_INDEX);
    StoreHClass(glue_, result->ReadVariable(), arrayClass);
    Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(TaggedArray::LENGTH_OFFSET), numArgs);
    // skip InitializeTaggedArrayWithSpeicalValue due to immediate setting arguments
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    Branch(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    GateRef idx = ZExtInt32ToPtr(Int32Add(startIdx, *i));
    GateRef argument = Load(VariableType::JS_ANY(), sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue_, result->ReadVariable(), *i, argument);
    i = Int32Add(*i, Int32(1));
    Branch(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
    Bind(&setArgumentsAgain);
    LoopEnd(&setArgumentsBegin);
    Bind(&setArgumentsEnd);
    Jump(exit);
}

void NewObjectStubBuilder::NewArgumentsObj(Variable *result, Label *exit,
    GateRef argumentsList, GateRef numArgs)
{
    auto env = GetEnvironment();

    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);
    GateRef argumentsClass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                               GlobalEnv::ARGUMENTS_CLASS);
    Label afterNewObject(env);
    NewJSObject(result, &afterNewObject, argumentsClass);
    Bind(&afterNewObject);
    Label setArgumentsObjProperties(env);
    Branch(TaggedIsException(result->ReadVariable()), exit, &setArgumentsObjProperties);
    Bind(&setArgumentsObjProperties);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), argumentsClass, IntToTaggedInt(numArgs),
                            Int32(JSArguments::LENGTH_INLINE_PROPERTY_INDEX));
    SetElementsArray(VariableType::JS_ANY(), glue_, result->ReadVariable(), argumentsList);
    GateRef arrayProtoValuesFunction = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                         GlobalEnv::ARRAY_PROTO_VALUES_FUNCTION_INDEX);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), argumentsClass, arrayProtoValuesFunction,
                            Int32(JSArguments::ITERATOR_INLINE_PROPERTY_INDEX));
    GateRef accessorCaller = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                               GlobalEnv::ARGUMENTS_CALLER_ACCESSOR);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), argumentsClass, accessorCaller,
                            Int32(JSArguments::CALLER_INLINE_PROPERTY_INDEX));
    GateRef accessorCallee = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                               GlobalEnv::ARGUMENTS_CALLEE_ACCESSOR);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), argumentsClass, accessorCallee,
                            Int32(JSArguments::CALLEE_INLINE_PROPERTY_INDEX));
    Jump(exit);
}

void NewObjectStubBuilder::NewJSArrayLiteral(Variable *result, Label *exit, RegionSpaceFlag spaceType, GateRef obj,
                                             GateRef hclass, bool isEmptyArray)
{
    auto env = GetEnvironment();
    Label initializeArray(env);
    Label afterInitialize(env);
    HeapAlloc(result, &initializeArray, spaceType);
    Bind(&initializeArray);
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), IntPtr(0), hclass);
    InitializeWithSpeicalValue(&afterInitialize, result->ReadVariable(), Undefined(), Int32(JSArray::SIZE),
                               TruncInt64ToInt32(size_));
    Bind(&afterInitialize);
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    Store(VariableType::INT64(), glue_, result->ReadVariable(), hashOffset, Int64(JSTaggedValue(0).GetRawData()));

    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    if (isEmptyArray) {
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), propertiesOffset, obj);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), elementsOffset, obj);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), lengthOffset, Int32(0));
    } else {
        auto newProperties = Load(VariableType::JS_POINTER(), obj, propertiesOffset);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), propertiesOffset, newProperties);

        auto newElements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), elementsOffset, newElements);

        GateRef arrayLength = Load(VariableType::INT32(), obj, lengthOffset);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), lengthOffset, arrayLength);
    }

    auto accessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), hclass, accessor,
        Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_POINTER());
    Jump(exit);
}

void NewObjectStubBuilder::HeapAlloc(Variable *result, Label *exit, RegionSpaceFlag spaceType)
{
    switch (spaceType) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            AllocateInYoung(result, exit);
            break;
        default:
            break;
    }
}

void NewObjectStubBuilder::AllocateInYoung(Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label success(env);
    Label callRuntime(env);
    Label next(env);

#ifdef ARK_ASAN_ON
    DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());
    Jump(&callRuntime);
#else
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    auto isStartHeapSamplingOffset = JSThread::GlueData::GetIsStartHeapSamplingOffset(env->Is32Bit());
    auto isStartHeapSampling = Load(VariableType::JS_ANY(), glue_, IntPtr(isStartHeapSamplingOffset));
    Branch(TaggedIsTrue(isStartHeapSampling), &callRuntime, &next);
    Bind(&next);
#endif
    auto topOffset = JSThread::GlueData::GetNewSpaceAllocationTopAddressOffset(env->Is32Bit());
    auto endOffset = JSThread::GlueData::GetNewSpaceAllocationEndAddressOffset(env->Is32Bit());
    auto topAddress = Load(VariableType::NATIVE_POINTER(), glue_, IntPtr(topOffset));
    auto endAddress = Load(VariableType::NATIVE_POINTER(), glue_, IntPtr(endOffset));
    auto top = Load(VariableType::JS_POINTER(), topAddress, IntPtr(0));
    auto end = Load(VariableType::JS_POINTER(), endAddress, IntPtr(0));
    DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());
    auto newTop = PtrAdd(top, size_);
    Branch(IntPtrGreaterThan(newTop, end), &callRuntime, &success);
    Bind(&success);
    {
        Store(VariableType::NATIVE_POINTER(), glue_, topAddress, IntPtr(0), newTop);
        if (env->Is32Bit()) {
            top = ZExtInt32ToInt64(top);
        }
        ret = top;
        result->WriteVariable(*ret);
        Jump(exit);
    }
#endif
    Bind(&callRuntime);
    {
        ret = CallRuntime(glue_, RTSTUB_ID(AllocateInYoung), {
            IntToTaggedInt(size_) });
        result->WriteVariable(*ret);
        Jump(exit);
    }
}

void NewObjectStubBuilder::InitializeWithSpeicalValue(Label *exit, GateRef object, GateRef value, GateRef start,
                                                      GateRef end)
{
    auto env = GetEnvironment();
    Label begin(env);
    Label storeValue(env);
    Label endLoop(env);

    DEFVARIABLE(startOffset, VariableType::INT32(), start);
    Jump(&begin);
    LoopBegin(&begin);
    {
        Branch(Int32UnsignedLessThan(*startOffset, end), &storeValue, exit);
        Bind(&storeValue);
        {
            Store(VariableType::INT64(), glue_, object, ZExtInt32ToPtr(*startOffset), value);
            startOffset = Int32Add(*startOffset, Int32(JSTaggedValue::TaggedTypeSize()));
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
    auto offset = Int32Mul(start, Int32(JSTaggedValue::TaggedTypeSize()));
    auto dataOffset = Int32Add(offset, Int32(TaggedArray::DATA_OFFSET));
    offset = Int32Mul(length, Int32(JSTaggedValue::TaggedTypeSize()));
    auto endOffset = Int32Add(offset, Int32(TaggedArray::DATA_OFFSET));
    InitializeWithSpeicalValue(exit, array, value, dataOffset, endOffset);
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
    AllocateInYoung(result, &afterAllocate);

    Bind(&afterAllocate);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::LINE_STRING_CLASS_INDEX);
    StoreHClass(glue_, result->ReadVariable(), stringClass);
    SetLength(glue_, result->ReadVariable(), length, compressed);
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0));
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

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto protoOrHclass = Load(VariableType::JS_ANY(), ctor,
        IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    Branch(TaggedIsHeapObject(protoOrHclass), &isHeapObject, &callRuntime);
    Bind(&isHeapObject);
    Branch(IsJSHClass(protoOrHclass), &checkJSObject, &callRuntime);
    Bind(&checkJSObject);
    auto objectType = GetObjectType(protoOrHclass);
    Branch(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))),
        &newObject, &callRuntime);
    Bind(&newObject);
    {
        SetParameters(glue, 0);
        NewJSObject(&thisObj, &exit, protoOrHclass);
    }
    Bind(&callRuntime);
    {
        thisObj = CallRuntime(glue, RTSTUB_ID(NewThisObject), {ctor});
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

    Branch(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Branch(IsBase(ctor), &ctorIsBase, &exit);
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

void NewObjectStubBuilder::LoadArrayHClass(Variable *hclass, Label *exit, GateRef glue,
    GateRef jsFunc, GateRef pc, GateRef profileTypeInfo, GateRef slotId, GateRef arrayLiteral)
{
    auto env = GetEnvironment();
    Label profiler(env);
    Label slowpath(env);
    DEFVARIABLE(ret, VariableType::JS_POINTER(), Undefined());

    Branch(TaggedIsUndefined(profileTypeInfo), &slowpath, &profiler);
    Bind(&profiler);
    {
        Label uninitialize(env);
        Label compareLabel(env);

        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        Branch(TaggedIsInt(slotValue), &compareLabel, &uninitialize);
        Bind(&compareLabel);
        {
            GateRef hclassIndex = TaggedGetInt(slotValue);
            GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
                IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit())));
            ret = Load(VariableType::JS_POINTER(), gConstAddr, hclassIndex);
            hclass->WriteVariable(*ret);
            Jump(exit);
        }
        Bind(&uninitialize);
        {
            Label fastpath(env);

            auto pfAddr = LoadPfHeaderFromConstPool(jsFunc);
            GateRef traceId = TruncPtrToInt32(PtrSub(IntPtr(pc), pfAddr));
            GateRef hclassIndex = LoadHCIndexFromConstPool(jsFunc, traceId);
            Branch(Int32NotEqual(hclassIndex,
                Int32(static_cast<int32_t>(ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX))),
                &fastpath, &slowpath);
            Bind(&fastpath);
            {
                Label updateSlot(env);

                GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
                    IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit())));
                ret = Load(VariableType::JS_POINTER(), gConstAddr, hclassIndex);
                hclass->WriteVariable(*ret);
                Branch(TaggedIsUndefined(slotValue), &updateSlot, exit);
                Bind(&updateSlot);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo,
                                      slotId, IntToTaggedInt(hclassIndex));
                Jump(exit);
            }
        }
    }
    Bind(&slowpath);
    {
        // emptyarray
        if (arrayLiteral == Circuit::NullGate()) {
            GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
            GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
            auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
            ret = Load(VariableType::JS_POINTER(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        } else {
            ret = LoadHClass(arrayLiteral);
        }
        hclass->WriteVariable(*ret);
        Jump(exit);
    }
}

GateRef NewObjectStubBuilder::CreateEmptyArrayCommon(GateRef glue, GateRef hclass, ProfileOperation callback)
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
    NewJSArrayLiteral(&result, &exit, RegionSpaceFlag::IN_YOUNG_SPACE, emptyArray, hclass, true);
    Bind(&exit);
    GateRef ret = *result;
    if (!callback.IsEmpty()) {
        GateRef noneHClass = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::ELEMENT_NONE_HCLASS_INDEX);
        StoreHClass(glue, ret, noneHClass);
        callback.ProfileCreateObject(ret);
    }
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CreateEmptyArray(GateRef glue, ProfileOperation callback)
{
    auto env = GetEnvironment();
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    return CreateEmptyArrayCommon(glue, hclass, callback);
}

GateRef NewObjectStubBuilder::CreateEmptyArray(
    GateRef glue, GateRef jsFunc, GateRef pc, GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label afterHClassInit(env);

    DEFVARIABLE(hclass, VariableType::JS_POINTER(), Undefined());
    LoadArrayHClass(&hclass, &afterHClassInit, glue, jsFunc, pc, profileTypeInfo, slotId);
    Bind(&afterHClassInit);
    GateRef result = CreateEmptyArrayCommon(glue, *hclass, callback);
    env->SubCfgExit();
    return result;
}

GateRef NewObjectStubBuilder::CreateArrayWithBuffer(GateRef glue,
    GateRef index, GateRef jsFunc, GateRef pc, GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label afterHClassInit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(hclass, VariableType::JS_POINTER(), Undefined());
    GateRef method = GetMethodFromFunction(jsFunc);
    GateRef constPool = Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef module = GetModuleFromFunction(jsFunc);

    auto obj = GetArrayLiteralFromConstPool(glue, constPool, index, module);
    LoadArrayHClass(&hclass, &afterHClassInit, glue, jsFunc, pc, profileTypeInfo, slotId, obj);
    Bind(&afterHClassInit);
    GateRef size = GetObjectSizeFromHClass(*hclass);

    SetParameters(glue, size);
    NewJSArrayLiteral(&result, &exit, RegionSpaceFlag::IN_YOUNG_SPACE, obj, *hclass, false);

    Bind(&exit);
    auto ret = *result;
    callback.ProfileCreateObject(ret);
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
    int32_t iterPrototypeIdx = 0; // ITERATOR_PROTOTYPE_INDEX
    size_t linkedOffset = 0;      // LINKED_MAP_OFFSET
    if constexpr (std::is_same_v<IteratorType, JSSetIterator>) {
        iterClassIdx = ConstantIndex::JS_SET_ITERATOR_CLASS_INDEX;
        iterOffset = IteratorType::ITERATED_SET_OFFSET;
        iterPrototypeIdx = GlobalEnv::SET_ITERATOR_PROTOTYPE_INDEX;
        linkedOffset = CollectionType::LINKED_SET_OFFSET;
    } else {
        iterClassIdx = ConstantIndex::JS_MAP_ITERATOR_CLASS_INDEX;
        iterOffset = IteratorType::ITERATED_MAP_OFFSET;
        iterPrototypeIdx = GlobalEnv::MAP_ITERATOR_PROTOTYPE_INDEX;
        linkedOffset = CollectionType::LINKED_MAP_OFFSET;
    }
    GateRef iteratorHClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, iterClassIdx);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);
    GateRef prototype = GetGlobalEnvValue(VariableType::JS_POINTER(), glueGlobalEnv, iterPrototypeIdx);
    SetPrototypeToHClass(VariableType::JS_POINTER(), glue_, iteratorHClass, prototype);

    Label afterAllocate(env);
    NewJSObject(result, &afterAllocate, iteratorHClass);
    Bind(&afterAllocate);
    Label setProperties(env);
    Branch(TaggedIsException(result->ReadVariable()), exit, &setProperties);
    Bind(&setProperties);

    SetExtensibleToBitfield(glue_, result->ReadVariable(), true);
    // GetLinked
    GateRef linked = Load(VariableType::JS_ANY(), thisValue, IntPtr(linkedOffset));

    // SetIterated
    GateRef iteratorOffset = IntPtr(iterOffset);
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), iteratorOffset, linked);

    // SetIteratorNextIndex
    GateRef nextIndexOffset = IntPtr(IteratorType::NEXT_INDEX_OFFSET);
    Store(VariableType::INT32(), glue_, result->ReadVariable(), nextIndexOffset, Int32(0));

    // SetIterationKind
    GateRef kindBitfieldOffset = IntPtr(IteratorType::BIT_FIELD_OFFSET);
    Store(VariableType::INT32(), glue_, result->ReadVariable(), kindBitfieldOffset, kind);
    Jump(exit);
}

template void NewObjectStubBuilder::CreateJSCollectionIterator<JSSetIterator, JSSet>(
    Variable *result, Label *exit, GateRef set, GateRef kind);
template void NewObjectStubBuilder::CreateJSCollectionIterator<JSMapIterator, JSMap>(
    Variable *result, Label *exit, GateRef set, GateRef kind);
}  // namespace panda::ecmascript::kungfu
