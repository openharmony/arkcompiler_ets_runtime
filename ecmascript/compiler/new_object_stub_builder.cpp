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

#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/mem/mem.h"
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
        VariableType::JS_POINTER(), glue_, ConstantIndex::ENV_CLASS_INDEX);
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
    GateRef isElementsKindEnabled = CallRuntime(glue_, RTSTUB_ID(IsElementsKindSwitchOn), {});
    BRANCH(TaggedIsTrue(isElementsKindEnabled), &enabledElementsKind, &initObj);
    Bind(&enabledElementsKind);
    {
        // For new Array(Len), the elementsKind should be Hole
        BRANCH(Equal(TruncInt64ToInt32(size), Int32(0)), &initObj, &notEmptyArray);
        Bind(&notEmptyArray);
        {
            #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
            GateRef holeKindArrayClass = GetGlobalConstantValue(VariableType::JS_ANY(), glue_,
                                                                ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX);
            StoreHClass(glue_, result, holeKindArrayClass);
            #else
            GateRef holeKindArrayClass = GetGlobalConstantValue(VariableType::JS_ANY(), glue_,
                                                                ConstantIndex::ELEMENT_HOLE_HCLASS_INDEX);
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

void NewObjectStubBuilder::NewJSObject(Variable *result, Label *exit, GateRef hclass, MemoryOrder order)
{
    auto env = GetEnvironment();

    size_ = GetObjectSizeFromHClass(hclass);
    // Be careful. NO GC is allowed when initization is not complete.
    Label hasPendingException(env);
    Label noException(env);
    AllocateInYoung(result, &hasPendingException, &noException, hclass);
    Bind(&noException);
    {
        if (order.Value() == MemoryOrder::NoBarrier().Value()) {
            StoreHClassWithoutBarrier(glue_, result->ReadVariable(), hclass);
        } else {
            StoreHClass(glue_, result->ReadVariable(), hclass);
        }
        DEFVARIABLE(initValue, VariableType::JS_ANY(), Undefined());
        Label isTS(env);
        Label initialize(env);
        BRANCH(IsTSHClass(hclass), &isTS, &initialize);
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
            result->ReadVariable(), *initValue, Int32(JSObject::SIZE), ChangeIntPtrToInt32(size_), order);
        Bind(&afterInitialize);
        auto emptyArray = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        SetHash(glue_, result->ReadVariable(), Int64(JSTaggedValue(0).GetRawData()));
        SetPropertiesArray(VariableType::INT64(),
            glue_, result->ReadVariable(), emptyArray, MemoryOrder::NoBarrier());
        SetElementsArray(VariableType::INT64(),
            glue_, result->ReadVariable(), emptyArray, MemoryOrder::NoBarrier());
        Jump(exit);
    }
    Bind(&hasPendingException);
    {
        Jump(exit);
    }
}

void NewObjectStubBuilder::NewJSObject(Variable *result, Label *exit, GateRef hclass, GateRef size)
{
    auto env = GetEnvironment();
    size_ = size;
    Label initialize(env);
    HeapAlloc(result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE, hclass);
    Bind(&initialize);
    StoreHClassWithoutBarrier(glue_, result->ReadVariable(), hclass);
    DEFVARIABLE(initValue, VariableType::JS_ANY(), Undefined());
    Label afterInitialize(env);
    InitializeWithSpeicalValue(&afterInitialize,
        result->ReadVariable(), *initValue, Int32(JSObject::SIZE), ChangeIntPtrToInt32(size_));
    Bind(&afterInitialize);
    auto emptyArray = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    SetHash(glue_, result->ReadVariable(), Int64(JSTaggedValue(0).GetRawData()));
    SetPropertiesArray(VariableType::INT64(),
        glue_, result->ReadVariable(), emptyArray, MemoryOrder::NoBarrier());
    SetElementsArray(VariableType::INT64(),
        glue_, result->ReadVariable(), emptyArray, MemoryOrder::NoBarrier());
    Jump(exit);
}

GateRef NewObjectStubBuilder::NewJSObject(GateRef glue, GateRef hclass, MemoryOrder order)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    SetGlue(glue);
    NewJSObject(&result, &exit, hclass, order);

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
        VariableType::JS_POINTER(), glue_, ConstantIndex::ARRAY_CLASS_INDEX);
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

GateRef NewObjectStubBuilder::ExtendArray(GateRef glue, GateRef elements, GateRef newLen)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label newMutantArray(env);
    Label newNormalArray(env);
    Label afterNew(env);
    Label exit(env);
    NewObjectStubBuilder newBuilder(this);
    SetGlue(glue);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
    BRANCH(IsMutantTaggedArray(elements),
           &newMutantArray, &newNormalArray);
    Bind(&newNormalArray);
    {
        array = newBuilder.NewTaggedArray(glue, newLen);
        Jump(&afterNew);
    }
    Bind(&newMutantArray);
    {
        array = CallRuntime(glue_, RTSTUB_ID(NewMutantTaggedArray), { IntToTaggedInt(newLen) });
        Jump(&afterNew);
    }
    Bind(&afterNew);
    Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::LENGTH_OFFSET), newLen);
    GateRef oldExtractLen = GetExtractLengthOfTaggedArray(elements);
    Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
    GateRef oldL = GetLengthOfTaggedArray(elements);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Label storeToNormalArray(env);
    Label storeToMutantArray(env);
    Label finishStore(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*index, oldL), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            BRANCH(IsMutantTaggedArray(elements),
                   &storeToMutantArray, &storeToNormalArray);
            Bind(&storeToNormalArray);
            {
                GateRef value = GetValueFromTaggedArray(elements, *index);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *array, *index, value);
                Jump(&finishStore);
            }
            Bind(&storeToMutantArray);
            {
                GateRef value = GetValueFromMutantTaggedArray(elements, *index);
                SetValueToTaggedArray(VariableType::INT64(), glue, *array, *index, value);
                Jump(&finishStore);
            }
            Bind(&finishStore);
            {
                index = Int32Add(*index, Int32(1));
                Jump(&loopEnd);
            }
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead, env, glue);
    Bind(&afterLoop);
    {
        Label loopHead1(env);
        Label loopEnd1(env);
        Label afterLoop1(env);
        Label storeValue1(env);
        Jump(&loopHead1);
        Label storeNormalHole(env);
        Label storeMutantHole(env);
        Label finishStoreHole(env);
        LoopBegin(&loopHead1);
        {
            BRANCH(Int32UnsignedLessThan(*index, newLen), &storeValue1, &afterLoop1);
            Bind(&storeValue1);
            {
                BRANCH(IsMutantTaggedArray(elements),
                       &storeMutantHole, &storeNormalHole);
                Bind(&storeNormalHole);
                {
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, *array, *index, Hole());
                    Jump(&finishStoreHole);
                }
                Bind(&storeMutantHole);
                {
                    SetValueToTaggedArray(VariableType::INT64(), glue, *array, *index, SpecialHole());
                    Jump(&finishStoreHole);
                }
                Bind(&finishStoreHole);
                {
                    index = Int32Add(*index, Int32(1));
                    Jump(&loopEnd1);
                }
            }
        }
        Bind(&loopEnd1);
        LoopEnd(&loopHead1);
        Bind(&afterLoop1);
        {
            res = *array;
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::CopyArray(GateRef glue, GateRef elements, GateRef oldLen, GateRef newLen)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    NewObjectStubBuilder newBuilder(this);
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
        BRANCH(Int32GreaterThan(newLen, oldLen), &extendArray, &notExtendArray);
        Bind(&extendArray);
        {
            result = ExtendArray(glue, elements, newLen);
            Jump(&exit);
        }
        Bind(&notExtendArray);
        {
            DEFVARIABLE(array, VariableType::JS_ANY(), Undefined());
            Label isMutantTaggedArray(env);
            Label isNotMutantTaggedArray(env);
            Label afterInitializeElements(env);
            GateRef checkIsMutantTaggedArray = IsMutantTaggedArray(*array);
            BRANCH(checkIsMutantTaggedArray, &isMutantTaggedArray, &isNotMutantTaggedArray);
            Bind(&isMutantTaggedArray);
            {
                array = newBuilder.NewMutantTaggedArray(glue, newLen);
                Jump(&afterInitializeElements);
            }
            Bind(&isNotMutantTaggedArray);
            {
                array = newBuilder.NewTaggedArray(glue, newLen);
                Jump(&afterInitializeElements);
            }
            Bind(&afterInitializeElements);
            Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::LENGTH_OFFSET), newLen);
            GateRef oldExtractLen = GetExtractLengthOfTaggedArray(elements);
            Store(VariableType::INT32(), glue, *array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Label storeValue(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32UnsignedLessThan(*index, newLen), &storeValue, &afterLoop);
                Bind(&storeValue);
                {
                    Label storeToTaggedArray(env);
                    Label storeToMutantTaggedArray(env);
                    Label finishStore(env);
                    BRANCH(checkIsMutantTaggedArray, &storeToMutantTaggedArray, &storeToTaggedArray);
                    Bind(&storeToMutantTaggedArray);
                    {
                        GateRef value = GetValueFromMutantTaggedArray(elements, *index);
                        SetValueToTaggedArray(VariableType::INT64(), glue, *array, *index, value);
                        Jump(&finishStore);
                    }
                    Bind(&storeToTaggedArray);
                    {
                        GateRef value = GetValueFromTaggedArray(elements, *index);
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, *array, *index, value);
                        Jump(&finishStore);
                    }
                    Bind(&finishStore);
                    index = Int32Add(*index, Int32(1));
                    Jump(&loopEnd);
                }
            }
            Bind(&loopEnd);
            LoopEnd(&loopHead, env, glue);
            Bind(&afterLoop);
            {
                result = *array;
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef NewObjectStubBuilder::NewJSForinIterator(GateRef glue, GateRef receiver, GateRef keys, GateRef cachedHclass)
{
    auto env = GetEnvironment();
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::FOR_IN_ITERATOR_CLASS_INDEX);
    GateRef iter = NewJSObject(glue, hclass);
    // init JSForinIterator
    SetObjectOfForInIterator(glue, iter, receiver);
    SetCachedHclassOfForInIterator(glue, iter, cachedHclass);
    SetKeysOfForInIterator(glue, iter, keys);
    SetIndexOfForInIterator(glue, iter, Int32(EnumCache::ENUM_CACHE_HEADER_SIZE));
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
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    BRANCH(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ARROW_FUNCTION))),
        &isNormal, &notNormal);
    Bind(&isNormal);
    {
        // 2 : this switch has 2 case
        Switch(kind, &defaultLabel, valueBuffer, labelBuffer, 2);
        Bind(&labelBuffer[0]);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::FUNCTION_CLASS_WITH_PROTO);
            Jump(&exit);
        }
        Bind(&labelBuffer[1]);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::FUNCTION_CLASS_WITHOUT_PROTO);
            Jump(&exit);
        }
    }
    Bind(&notNormal);
    {
        BRANCH(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ASYNC_FUNCTION))),
            &isAsync, &notAsync);
        Bind(&isAsync);
        {
            hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ASYNC_FUNCTION_CLASS);
            Jump(&exit);
        }
        Bind(&notAsync);
        {
            // 3 : this switch has 3 case
            Switch(kind, &defaultLabel, valueBuffer1, labelBuffer1, 3);
            Bind(&labelBuffer1[0]);
            {
                hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::FUNCTION_CLASS_WITH_PROTO);
                Jump(&exit);
            }
            Bind(&labelBuffer1[1]);
            {
                hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::GENERATOR_FUNCTION_CLASS);
                Jump(&exit);
            }
            // 2 : index of kind
            Bind(&labelBuffer1[2]);
            {
                hclass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
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

GateRef NewObjectStubBuilder::NewJSFunction(GateRef glue, GateRef constpool, GateRef index, FunctionKind targetKind)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(ihc, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(val, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    val = GetValueFromTaggedArray(constpool, index);

    Label isHeapObject(env);
    Label afterAOTLiteral(env);
    BRANCH(TaggedIsHeapObject(*val), &isHeapObject, &afterAOTLiteral);
    {
        Bind(&isHeapObject);
        Label isAOTLiteral(env);
        BRANCH(IsAOTLiteralInfo(*val), &isAOTLiteral, &afterAOTLiteral);
        {
            Bind(&isAOTLiteral);
            // Avoiding shareobj references to unshareobj.
            GateRef unshareIdx = GetUnsharedConstpoolIndex(constpool);
            GateId unshareCpOffset = JSThread::GlueData::GetUnSharedConstpoolsOffset(env->Is32Bit());
            GateRef unshareCpAddr = Load(VariableType::NATIVE_POINTER(), glue, IntPtr(unshareCpOffset));
            GateRef unshareCp = GetUnsharedConstpool(unshareCpAddr, unshareIdx);
            val = GetValueFromTaggedArray(unshareCp, index);
            ihc = GetIhcFromAOTLiteralInfo(*val);
            Jump(&afterAOTLiteral);
        }
    }
    Bind(&afterAOTLiteral);
    GateRef method = GetMethodFromConstPool(glue, constpool, index);
    GateRef hclass = LoadHClassFromMethod(glue, method);
    bool knownKind = JSFunction::IsNormalFunctionAndCanSkipWbWhenInitialization(targetKind);
    result = NewJSObject(glue, hclass, knownKind ? MemoryOrder::NoBarrier() : MemoryOrder::Default());
    SetExtensibleToBitfield(glue, hclass, true);
    SetCallableToBitfield(glue, hclass, true);
    GateRef kind = GetFuncKind(method);
    InitializeJSFunction(glue, *result, kind, targetKind);
    SetMethodToFunction(glue, *result, method, knownKind ? MemoryOrder::NoBarrier() : MemoryOrder::Default());

    Label isAotWithCallField(env);
    Label afterAotWithCallField(env);
    Label isJitCompiledCode(env);
    Label afterJitCompiledCode(env);
    BRANCH(IsAotWithCallField(method), &isAotWithCallField, &afterAotWithCallField);
    {
        Bind(&isAotWithCallField);
        BRANCH(IsJitCompiledCode(method), &isJitCompiledCode, &afterJitCompiledCode);
        {
            Bind(&isJitCompiledCode);
            ClearJitCompiledCodeFlags(glue, method);
            Jump(&afterAotWithCallField);
        }
        Bind(&afterJitCompiledCode);

        SetCodeEntryToFunction(glue, *result, method);
        Jump(&afterAotWithCallField);
    }
    Bind(&afterAotWithCallField);

    Label ihcNotUndefined(env);
    BRANCH(TaggedIsUndefined(*ihc), &exit, &ihcNotUndefined);
    Bind(&ihcNotUndefined);
    {
        CallRuntime(glue, RTSTUB_ID(AOTEnableProtoChangeMarker), { *result, *ihc});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void NewObjectStubBuilder::NewJSFunction(GateRef glue, GateRef jsFunc, GateRef index, GateRef length, GateRef lexEnv,
                                         Variable *result, Label *success, Label *failed, FunctionKind targetKind)
{
    auto env = GetEnvironment();
    Label hasException(env);
    Label notException(env);
    GateRef constPool = GetConstPoolFromFunction(jsFunc);
    result->WriteVariable(NewJSFunction(glue, constPool, index, targetKind));
    BRANCH(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(failed);
    }
    Bind(&notException);
    {
        bool knownKind = JSFunction::IsNormalFunctionAndCanSkipWbWhenInitialization(targetKind);
        SetLengthToFunction(glue_, result->ReadVariable(), length);
        SetLexicalEnvToFunction(glue_, result->ReadVariable(), lexEnv,
                                knownKind ? MemoryOrder::NoBarrier() : MemoryOrder::Default());
        SetModuleToFunction(glue_, result->ReadVariable(), GetModuleFromFunction(jsFunc),
                            knownKind ? MemoryOrder::NoBarrier() : MemoryOrder::Default());
        SetHomeObjectToFunction(glue_, result->ReadVariable(), GetHomeObjectFromFunction(jsFunc),
                                knownKind ? MemoryOrder::NoBarrier() : MemoryOrder::Default());
        Jump(success);
    }
}

void NewObjectStubBuilder::InitializeJSFunction(GateRef glue, GateRef func, GateRef kind, FunctionKind getKind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label hasProto(env);
    Label notProto(env);
    Label hasAccess(env);
    Label isBase(env);
    Label notBase(env);
    Label isGenerator(env);
    Label notClassConstructor(env);

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    GateRef hclass = LoadHClass(func);

    if (JSFunction::IsNormalFunctionAndCanSkipWbWhenInitialization(getKind)) {
        SetProtoOrHClassToFunction(glue, func, Hole(), MemoryOrder::NoBarrier());
        SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryOrder::NoBarrier());

        if (JSFunction::HasPrototype(getKind)) {
            auto funcprotoAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                            ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
            if (getKind == FunctionKind::BASE_CONSTRUCTOR || getKind == FunctionKind::GENERATOR_FUNCTION ||
                getKind == FunctionKind::ASYNC_GENERATOR_FUNCTION) {
                SetPropertyInlinedProps(glue, func, hclass, funcprotoAccessor,
                                        Int32(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryOrder::NoBarrier());
                auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::FUNCTION_NAME_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryOrder::NoBarrier());
                funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryOrder::NoBarrier());
                if (getKind != FunctionKind::BASE_CONSTRUCTOR) {
                    thisObj = CallRuntime(glue, RTSTUB_ID(InitializeGeneratorFunction), {kind});
                    SetProtoOrHClassToFunction(glue, func, *thisObj);
                }
            } else if (!JSFunction::IsClassConstructor(getKind)) {
                CallRuntime(glue, RTSTUB_ID(FunctionDefineOwnProperty), {func, funcprotoAccessor, kind});
            }
            Jump(&exit);
        } else if (JSFunction::HasAccessor(getKind)) {
            auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                       ConstantIndex::FUNCTION_NAME_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(),
                                    MemoryOrder::NoBarrier());
            funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(),
                                    MemoryOrder::NoBarrier());
            Jump(&exit);
        }
    } else {
        SetLexicalEnvToFunction(glue, func, Undefined(), MemoryOrder::NoBarrier());
        SetHomeObjectToFunction(glue, func, Undefined(), MemoryOrder::NoBarrier());
        SetProtoOrHClassToFunction(glue, func, Hole(), MemoryOrder::NoBarrier());
        SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryOrder::NoBarrier());
        SetMethodToFunction(glue, func, Undefined(), MemoryOrder::NoBarrier());

        BRANCH(HasPrototype(kind), &hasProto, &notProto);
        Bind(&hasProto);
        {
            auto funcprotoAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                            ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
            BRANCH(IsBaseKind(kind), &isBase, &notBase);
            Bind(&isBase);
            {
                SetPropertyInlinedProps(glue, func, hclass, funcprotoAccessor,
                                        Int32(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryOrder::NoBarrier());
                auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::FUNCTION_NAME_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryOrder::NoBarrier());
                funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryOrder::NoBarrier());
                BRANCH(IsGeneratorKind(kind), &isGenerator, &exit);
                Bind(&isGenerator);
                {
                    thisObj = CallRuntime(glue, RTSTUB_ID(InitializeGeneratorFunction), {kind});
                    SetProtoOrHClassToFunction(glue, func, *thisObj);
                    Jump(&exit);
                }
            }
            Bind(&notBase);
            {
                BRANCH(IsClassConstructorKind(kind), &exit, &notClassConstructor);
                Bind(&notClassConstructor);
                {
                    CallRuntime(glue, RTSTUB_ID(FunctionDefineOwnProperty), {func, funcprotoAccessor, kind});
                    Jump(&exit);
                }
            }
        }
        Bind(&notProto);
        {
            BRANCH(HasAccessor(kind), &hasAccess, &exit);
            Bind(&hasAccess);
            {
                auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::FUNCTION_NAME_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryOrder::NoBarrier());
                funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryOrder::NoBarrier());
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
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
    BRANCH(TaggedIsString(obj), &isString, &isNotString);
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
    BRANCH(TaggedIsUndefinedOrNull(*object), &empty, &tryGetEnumCache);
    Bind(&tryGetEnumCache);
    GateRef enumCache = TryGetEnumCache(glue, *object);
    BRANCH(TaggedIsUndefined(enumCache), &slowpath, &cacheHit);
    Bind(&cacheHit);
    {
        GateRef hclass = LoadHClass(*object);
        result = NewJSForinIterator(glue, *object, enumCache, hclass);
        Jump(&exit);
    }
    Bind(&empty);
    {
        GateRef emptyArray = GetEmptyArray(glue);
        result = NewJSForinIterator(glue, Undefined(), emptyArray, Undefined());
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

void NewObjectStubBuilder::NewArgumentsList(Variable *result, Label *exit,
    GateRef sp, GateRef startIdx, GateRef numArgs)
{
    auto env = GetEnvironment();
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label setHClass(env);
    size_ = ComputeTaggedArraySize(ZExtInt32ToPtr(numArgs));
    GateRef arrayClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                ConstantIndex::ARRAY_CLASS_INDEX);
    AllocateInYoung(result, exit, &setHClass, arrayClass);
    Bind(&setHClass);
    StoreHClass(glue_, result->ReadVariable(), arrayClass);
    Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(TaggedArray::LENGTH_OFFSET), numArgs);
    Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), Int32(0));
    // skip InitializeTaggedArrayWithSpeicalValue due to immediate setting arguments
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    GateRef idx = ZExtInt32ToPtr(Int32Add(startIdx, *i));
    GateRef argument = Load(VariableType::JS_ANY(), sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue_, result->ReadVariable(), *i, argument);
    i = Int32Add(*i, Int32(1));
    BRANCH(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
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
    BRANCH(TaggedIsException(result->ReadVariable()), exit, &setArgumentsObjProperties);
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
                                             GateRef hclass, GateRef trackInfo, bool isEmptyArray)
{
    auto env = GetEnvironment();
    Label initializeArray(env);
    Label afterInitialize(env);
    HeapAlloc(result, &initializeArray, spaceType, hclass);
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
    GateRef trackInfoOffset = IntPtr(JSArray::TRACK_INFO_OFFSET);
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
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), trackInfoOffset, trackInfo);

    auto accessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    SetPropertyInlinedProps(glue_, result->ReadVariable(), hclass, accessor,
        Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_POINTER());
    Jump(exit);
}

void NewObjectStubBuilder::HeapAlloc(Variable *result, Label *exit, RegionSpaceFlag spaceType, GateRef hclass)
{
    switch (spaceType) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            AllocateInYoung(result, exit, hclass);
            break;
        default:
            break;
    }
}

void NewObjectStubBuilder::AllocateInSOldPrologue(Variable *result, Label *callRuntime, Label *exit)
{
    auto env = GetEnvironment();
    Label success(env);
    Label next(env);

#ifdef ARK_ASAN_ON
    Jump(callRuntime);
#else
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    auto isStartHeapSamplingOffset = JSThread::GlueData::GetIsStartHeapSamplingOffset(env->Is32Bit());
    auto isStartHeapSampling = Load(VariableType::JS_ANY(), glue_, IntPtr(isStartHeapSamplingOffset));
    BRANCH(TaggedIsTrue(isStartHeapSampling), callRuntime, &next);
    Bind(&next);
#endif
    auto topOffset = JSThread::GlueData::GetSOldSpaceAllocationTopAddressOffset(env->Is32Bit());
    auto endOffset = JSThread::GlueData::GetSOldSpaceAllocationEndAddressOffset(env->Is32Bit());
    auto topAddress = Load(VariableType::NATIVE_POINTER(), glue_, IntPtr(topOffset));
    auto endAddress = Load(VariableType::NATIVE_POINTER(), glue_, IntPtr(endOffset));
    auto top = Load(VariableType::JS_POINTER(), topAddress, IntPtr(0));
    auto end = Load(VariableType::JS_POINTER(), endAddress, IntPtr(0));
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
    auto env = GetEnvironment();
    Label callRuntime(env);
    AllocateInSOldPrologue(result, &callRuntime, exit);
    Bind(&callRuntime);
    {
        DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());
        ret = CallRuntime(glue_, RTSTUB_ID(AllocateInSOld), {IntToTaggedInt(size_), hclass});
        result->WriteVariable(*ret);
        Jump(exit);
    }
}

void NewObjectStubBuilder::AllocateInYoungPrologue(Variable *result, Label *callRuntime, Label *exit)
{
    auto env = GetEnvironment();
    Label success(env);
    Label next(env);

#ifdef ARK_ASAN_ON
    Jump(callRuntime);
#else
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    auto isStartHeapSamplingOffset = JSThread::GlueData::GetIsStartHeapSamplingOffset(env->Is32Bit());
    auto isStartHeapSampling = Load(VariableType::JS_ANY(), glue_, IntPtr(isStartHeapSamplingOffset));
    BRANCH(TaggedIsTrue(isStartHeapSampling), callRuntime, &next);
    Bind(&next);
#endif
    auto topOffset = JSThread::GlueData::GetNewSpaceAllocationTopAddressOffset(env->Is32Bit());
    auto endOffset = JSThread::GlueData::GetNewSpaceAllocationEndAddressOffset(env->Is32Bit());
    auto topAddress = Load(VariableType::NATIVE_POINTER(), glue_, IntPtr(topOffset));
    auto endAddress = Load(VariableType::NATIVE_POINTER(), glue_, IntPtr(endOffset));
    auto top = Load(VariableType::JS_POINTER(), topAddress, IntPtr(0));
    auto end = Load(VariableType::JS_POINTER(), endAddress, IntPtr(0));
    auto newTop = PtrAdd(top, size_);
    BRANCH(IntPtrGreaterThan(newTop, end), callRuntime, &success);
    Bind(&success);
    {
        Store(VariableType::NATIVE_POINTER(), glue_, topAddress, IntPtr(0), newTop, MemoryOrder::NoBarrier());
        if (env->Is32Bit()) {
            top = ZExtInt32ToInt64(top);
        }
        result->WriteVariable(top);
        Jump(exit);
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
            IntToTaggedInt(size_), hclass });
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
            IntToTaggedInt(size_), hclass });
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
                                                      GateRef end, MemoryOrder order)
{
    auto env = GetEnvironment();
    Label begin(env);
    Label storeValue(env);
    Label endLoop(env);

    DEFVARIABLE(startOffset, VariableType::INT32(), start);
    Jump(&begin);
    LoopBegin(&begin);
    {
        BRANCH(Int32UnsignedLessThan(*startOffset, end), &storeValue, exit);
        Bind(&storeValue);
        {
            Store(VariableType::INT64(), glue_, object, ZExtInt32ToPtr(*startOffset), value, order);
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
    Store(VariableType::INT32(), glue_, array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), Int32(0));
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
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::LINE_STRING_CLASS_INDEX);
    AllocateInSOld(result, &afterAllocate, stringClass);

    Bind(&afterAllocate);
    StoreHClass(glue_, result->ReadVariable(), stringClass);
    SetLength(glue_, result->ReadVariable(), length, compressed);
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0), False());
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
    GateRef mixLength = Load(VariableType::INT32(), flatString->GetFlatString(), IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    GateRef isCompressed = Int32And(Int32(EcmaString::STRING_COMPRESSED_BIT), mixLength);
    SetLength(glue_, result->ReadVariable(), length, isCompressed);
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0), False());
    BuiltinsStringStubBuilder builtinsStringStubBuilder(this);
    builtinsStringStubBuilder.StoreParent(glue_, result->ReadVariable(), flatString->GetFlatString());
    builtinsStringStubBuilder.StoreStartIndex(glue_, result->ReadVariable(),
        Int32Add(from, flatString->GetStartIndex()));
    builtinsStringStubBuilder.StoreHasBackingStore(glue_, result->ReadVariable(), Int32(0));
    Jump(exit);
}

void NewObjectStubBuilder::AllocTreeStringObject(Variable *result, Label *exit, GateRef first, GateRef second,
    GateRef length, bool compressed)
{
    auto env = GetEnvironment();

    size_ = AlignUp(IntPtr(TreeEcmaString::SIZE), IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    Label afterAllocate(env);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::TREE_STRING_CLASS_INDEX);
    AllocateInSOld(result, &afterAllocate, stringClass);

    Bind(&afterAllocate);
    StoreHClass(glue_, result->ReadVariable(), stringClass);
    SetLength(glue_, result->ReadVariable(), length, compressed);
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0), False());
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), IntPtr(TreeEcmaString::FIRST_OFFSET), first);
    Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), IntPtr(TreeEcmaString::SECOND_OFFSET), second);
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
    auto protoOrHclass = Load(VariableType::JS_ANY(), ctor,
        IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(TaggedIsHeapObject(protoOrHclass), &isHeapObject, &callRuntime);
    Bind(&isHeapObject);
    BRANCH(IsJSHClass(protoOrHclass), &checkJSObject, &callRuntime);
    Bind(&checkJSObject);
    auto objectType = GetObjectType(protoOrHclass);
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))), &isJSObject, &callRuntime);
    Bind(&isJSObject);
    {
        auto funcProto = GetPrototypeFromHClass(protoOrHclass);
        BRANCH(IsEcmaObject(funcProto), &newObject, &callRuntime);
    }
    Bind(&newObject);
    {
        SetParameters(glue, 0);
        NewJSObject(&thisObj, &exit, protoOrHclass);
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
    
    BRANCH(IsJSFunction(newTarget), &isFunction, &callRuntime);
    Bind(&isFunction);
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(protoOrHclass, VariableType::JS_ANY(), Undefined());
    protoOrHclass = Load(VariableType::JS_ANY(), newTarget,
        IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(TaggedIsHeapObject(*protoOrHclass), &isHeapObject, &callRuntime);
    Bind(&isHeapObject);
    BRANCH(IsJSHClass(*protoOrHclass), &checkJSObject, &callRuntime);
    Bind(&checkJSObject);
    auto objectType = GetObjectType(*protoOrHclass);
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))),
        &newObject, &callRuntime);
    Bind(&newObject);
    {
        SetParameters(glue, 0);
        NewJSObject(&thisObj, &exit, *protoOrHclass);
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
    BRANCH(IsJSFunction(ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    BRANCH(IsConstructor(ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        BRANCH(IsBase(ctor), &ctorIsBase, &exit);
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

GateRef NewObjectStubBuilder::LoadTrackInfo(GateRef glue, GateRef jsFunc, TraceIdInfo traceIdInfo,
    GateRef profileTypeInfo, GateRef slotId, GateRef arrayLiteral, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(ret, VariableType::JS_POINTER(), Undefined());

    Label uninitialized(env);
    Label fastpath(env);
    GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
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

    auto hcIndexInfos = LoadHCIndexInfosFromConstPool(jsFunc);
    auto indexInfosLength = GetLengthOfTaggedArray(hcIndexInfos);
    Label aotLoad(env);
    BRANCH(Int32Equal(indexInfosLength, Int32(0)), &originLoad, &aotLoad);
    Bind(&aotLoad);
    {
        GateRef traceId = 0;
        if (traceIdInfo.isPc) {
            auto pfAddr = LoadPfHeaderFromConstPool(jsFunc);
            traceId = TruncPtrToInt32(PtrSub(traceIdInfo.pc, pfAddr));
        } else {
            traceId = traceIdInfo.traceId;
        }

        GateRef hcIndex = LoadHCIndexFromConstPool(hcIndexInfos, indexInfosLength, traceId, &originLoad);
        GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
            IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit())));
        GateRef offset = Int32Mul(Int32(sizeof(JSTaggedValue)), hcIndex);
        ret = Load(VariableType::JS_POINTER(), gConstAddr, offset);
        Jump(&exit);
    }
    Bind(&originLoad);
    {
        // emptyarray
        if (arrayLiteral == Circuit::NullGate()) {
            if (callback.IsEmpty()) {
                GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
                auto arrayFunc =
                    GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
                ret = Load(VariableType::JS_POINTER(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            } else {
                ret =
                    GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::ELEMENT_NONE_HCLASS_INDEX);
            }
        } else {
            ret = LoadHClass(arrayLiteral);
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
    auto env = GetEnvironment();
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef objectFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::OBJECT_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), objectFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    return NewJSObject(glue, hclass);
}

GateRef NewObjectStubBuilder::CreateEmptyArray(GateRef glue)
{
    auto env = GetEnvironment();
    DEFVARIABLE(trackInfo, VariableType::JS_ANY(), Undefined());
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef hclass = Load(VariableType::JS_POINTER(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
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
    BRANCH(TaggedIsUndefined(profileTypeInfo), &slowpath, &mayFastpath);
    Bind(&mayFastpath);
    {
        trackInfo = LoadTrackInfo(glue, jsFunc, traceIdInfo, profileTypeInfo, slotId, Circuit::NullGate(), callback);
        hclass = Load(VariableType::JS_ANY(), *trackInfo, IntPtr(TrackInfo::CACHED_HCLASS_OFFSET));
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

    GateRef method = GetMethodFromFunction(jsFunc);
    GateRef constPool = Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef module = GetModuleFromFunction(jsFunc);

    auto obj = GetArrayLiteralFromConstPool(glue, constPool, index, module);

    Label slowpath(env);
    Label mayFastpath(env);
    Label createArray(env);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &slowpath, &mayFastpath);
    Bind(&mayFastpath);
    {
        trackInfo = LoadTrackInfo(glue, jsFunc, traceIdInfo, profileTypeInfo, slotId, obj, callback);
        hclass = Load(VariableType::JS_ANY(), *trackInfo, IntPtr(TrackInfo::CACHED_HCLASS_OFFSET));
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
        SetHash(glue_, result->ReadVariable(), Int64(JSTaggedValue(0).GetRawData()));
        auto emptyArray = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        SetPropertiesArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray);
        SetElementsArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray);

        // GetLinked
        GateRef linked = Load(VariableType::JS_ANY(), thisValue, IntPtr(linkedOffset));
        // SetIterated
        GateRef iteratorOffset = IntPtr(iterOffset);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), iteratorOffset, linked,
              MemoryOrder::NeedBarrier());

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

    BRANCH(BoolOr(TaggedIsHole(thisValue), TaggedIsUndefinedOrNull(thisValue)), &throwTypeError, &thisExists);
    Bind(&thisExists);
    BRANCH(IsEcmaObject(thisValue), &isEcmaObject, &throwTypeError);
    Bind(&isEcmaObject);
    BRANCH(IsTypedArray(thisValue), &isTypedArray, &throwTypeError);
    Bind(&isTypedArray);

    Label noException(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, exit, &noException, iteratorHClass);
    Bind(&noException);
    {
        StoreBuiltinHClass(glue_, result->ReadVariable(), iteratorHClass);
        SetHash(glue_, result->ReadVariable(), Int64(JSTaggedValue(0).GetRawData()));
        auto emptyArray = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        SetPropertiesArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray);
        SetElementsArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray);

        GateRef iteratorOffset = IntPtr(JSArrayIterator::ITERATED_ARRAY_OFFSET);
        Store(VariableType::JS_POINTER(), glue_, result->ReadVariable(), iteratorOffset, thisValue,
              MemoryOrder::NeedBarrier());

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
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef constructorName = Load(VariableType::JS_POINTER(), srcTypedArray,
        IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET));
    GateRef srcByteOffset = Load(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
    GateRef contentType = Load(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET));
    GateRef beginByteOffset = Int32Add(srcByteOffset, Int32Mul(beginIndex, elementSize));

    GateRef obj = NewJSObject(glue, arrayCls);
    result = obj;
    GateRef newByteLength = Int32Mul(elementSize, newLength);
    Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET), buffer);
    Store(VariableType::JS_POINTER(), glue, obj, IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET), constructorName);
    Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET), newByteLength);
    Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET), beginByteOffset);
    Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET), newLength);
    Store(VariableType::INT32(), glue, obj, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET), contentType);
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
    BRANCH(HasConstructor(srcTypedArray), &slowPath, &defaultConstr);
    Bind(&defaultConstr);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef markerCell = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
        GlobalEnv::TYPED_ARRAY_SPECIES_PROTECT_DETECTOR_INDEX);
    BRANCH(IsMarkerCellValid(markerCell), &markerCellValid, &slowPath);
    Bind(&markerCellValid);
    GateRef marker = GetProtoChangeMarkerFromHClass(LoadHClass(srcTypedArray));
    BRANCH(TaggedIsProtoChangeMarker(marker), &isProtoChangeMarker, &accessorNotChanged);
    Bind(&isProtoChangeMarker);
    BRANCH(GetAccessorHasChanged(marker), &slowPath, &accessorNotChanged);

    Bind(&accessorNotChanged);
    {
        DEFVARIABLE(buffer, VariableType::JS_ANY(), Undefined());
        Label next(env);
        GateRef hclass = LoadHClass(srcTypedArray);
        GateRef obj = NewJSObject(glue, hclass);
        result = obj;
        GateRef ctorName = Load(VariableType::JS_POINTER(), srcTypedArray,
            IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET));
        GateRef elementSize = GetElementSizeFromType(glue, srcType);
        GateRef newByteLength = Int32Mul(elementSize, length);
        GateRef contentType = Load(VariableType::INT32(), srcTypedArray, IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET));
        BRANCH(Int32LessThanOrEqual(newByteLength, Int32(RangeInfo::TYPED_ARRAY_ONHEAP_MAX)), &next, &slowPath);
        Bind(&next);
        {
            Label newByteArrayExit(env);
            NewByteArray(&buffer, &newByteArrayExit, elementSize, length);
            Bind(&newByteArrayExit);
            StoreHClass(glue, obj, GetOnHeapHClassFromType(glue, srcType));
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

void NewObjectStubBuilder::NewByteArray(Variable *result, Label *exit, GateRef elementSize, GateRef length)
{
    auto env = GetEnvironment();

    Label noError(env);
    Label initializeExit(env);
    size_ = AlignUp(ComputeTaggedTypedArraySize(ZExtInt32ToPtr(elementSize), ZExtInt32ToPtr(length)),
        IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    auto hclass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::BYTE_ARRAY_CLASS_INDEX);
    AllocateInYoung(result, exit, &noError, hclass);
    Bind(&noError);
    {
        StoreBuiltinHClass(glue_, result->ReadVariable(), hclass);
        GateRef byteLength = Int32Mul(elementSize, length);
        auto startOffset = Int32(ByteArray::DATA_OFFSET);
        auto endOffset = Int32Add(Int32(ByteArray::DATA_OFFSET), byteLength);
        InitializeWithSpeicalValue(&initializeExit, result->ReadVariable(), Int32(0), startOffset, endOffset);
        Bind(&initializeExit);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(ByteArray::ARRAY_LENGTH_OFFSET), length);
        Store(VariableType::INT32(), glue_, result->ReadVariable(), IntPtr(ByteArray::BYTE_LENGTH_OFFSET), elementSize);
        Jump(exit);
    }
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
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
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
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::INT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 1 : index of this buffer
    Bind(&labelBuffer[1]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::UINT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 2 : index of this buffer
    Bind(&labelBuffer[2]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 3 : index of this buffer
    Bind(&labelBuffer[3]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::INT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 4 : index of this buffer
    Bind(&labelBuffer[4]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::UINT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 5 : index of this buffer
    Bind(&labelBuffer[5]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::INT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 6 : index of this buffer
    Bind(&labelBuffer[6]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::UINT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 7 : index of this buffer
    Bind(&labelBuffer[7]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::FLOAT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 8 : index of this buffer
    Bind(&labelBuffer[8]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::FLOAT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 9 : index of this buffer
    Bind(&labelBuffer[9]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
            GlobalEnv::BIGINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX);
        Jump(&exit);
    }
    // 10 : index of this buffer
    Bind(&labelBuffer[10]);
    {
        result = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
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
}  // namespace panda::ecmascript::kungfu
