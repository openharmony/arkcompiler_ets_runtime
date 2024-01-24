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
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/ecma_string.h"
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
    Label overflow(env);
    Label notOverflow(env);
    Branch(Int32UnsignedGreaterThan(len, Int32(INT32_MAX)), &overflow, &notOverflow);
    Bind(&overflow);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(LenGreaterThanMax));
        CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result->WriteVariable(Exception());
        Jump(exit);
    }
    Bind(&notOverflow);
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
    Branch(IsMutantTaggedArray(elements),
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
        Branch(Int32UnsignedLessThan(*index, oldL), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            Branch(IsMutantTaggedArray(elements),
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
    LoopEnd(&loopHead);
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
            Branch(Int32UnsignedLessThan(*index, newLen), &storeValue1, &afterLoop1);
            Bind(&storeValue1);
            {
                Branch(IsMutantTaggedArray(elements),
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
    Branch(Int32Equal(newLen, Int32(0)), &emptyArray, &notEmptyArray);
    Bind(&emptyArray);
    result = GetEmptyArray(glue);
    Jump(&exit);
    Bind(&notEmptyArray);
    {
        Label extendArray(env);
        Label notExtendArray(env);
        Branch(Int32GreaterThan(newLen, oldLen), &extendArray, &notExtendArray);
        Bind(&extendArray);
        {
            result = ExtendArray(glue, elements, newLen);
            Jump(&exit);
        }
        Bind(&notExtendArray);
        {
            GateRef array = newBuilder.NewTaggedArray(glue, newLen);
            Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::LENGTH_OFFSET), newLen);
            GateRef oldExtractLen = GetExtractLengthOfTaggedArray(elements);
            Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Label storeValue(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Branch(Int32UnsignedLessThan(*index, newLen), &storeValue, &afterLoop);
                Bind(&storeValue);
                {
                    GateRef value = GetValueFromTaggedArray(elements, *index);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, value);
                    index = Int32Add(*index, Int32(1));
                    Jump(&loopEnd);
                }
            }
            Bind(&loopEnd);
            LoopEnd(&loopHead);
            Bind(&afterLoop);
            {
                result = array;
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
    Branch(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ARROW_FUNCTION))),
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
        Branch(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ASYNC_FUNCTION))),
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

GateRef NewObjectStubBuilder::NewJSFunction(GateRef glue, GateRef constpool, GateRef module, GateRef index)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(ihc, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    auto val = GetValueFromTaggedArray(constpool, index);
    Label isHeapObject(env);
    Label afterAOTLiteral(env);
    Branch(TaggedIsHeapObject(val), &isHeapObject, &afterAOTLiteral);
    {
        Bind(&isHeapObject);
        Label isAOTLiteral(env);
        Branch(IsAOTLiteralInfo(val), &isAOTLiteral, &afterAOTLiteral);
        {
            Bind(&isAOTLiteral);
            ihc = GetIhcFromAOTLiteralInfo(val);
            Jump(&afterAOTLiteral);
        }
    }
    Bind(&afterAOTLiteral);
    GateRef method = GetMethodFromConstPool(glue, constpool, module, index);
    GateRef hclass = LoadHClassFromMethod(glue, method);
    result = NewJSObject(glue, hclass);
    SetExtensibleToBitfield(glue, hclass, true);
    SetCallableToBitfield(glue, hclass, true);
    GateRef kind = GetFuncKind(method);
    InitializeJSFunction(glue, *result, kind);
    SetMethodToFunction(glue, *result, method);

    Label isAotWithCallField(env);
    Label afterAotWithCallField(env);
    Branch(IsAotWithCallField(method), &isAotWithCallField, &afterAotWithCallField);
    {
        Bind(&isAotWithCallField);
        SetCodeEntryToFunction(glue, *result, method);
        Jump(&afterAotWithCallField);
    }
    Bind(&afterAotWithCallField);

    Label ihcNotUndefined(env);
    Branch(TaggedIsUndefined(*ihc), &exit, &ihcNotUndefined);
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
                                         Variable *result, Label *success, Label *failed)
{
    auto env = GetEnvironment();
    Label hasException(env);
    Label notException(env);
    GateRef constPool = GetConstPoolFromFunction(jsFunc);
    GateRef module = GetModuleFromFunction(jsFunc);
    result->WriteVariable(NewJSFunction(glue, constPool, module, index));
    Branch(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(failed);
    }
    Bind(&notException);
    {
        SetLengthToFunction(glue_, result->ReadVariable(), length);
        SetLexicalEnvToFunction(glue_, result->ReadVariable(), lexEnv);
        SetHomeObjectToFunction(glue_, result->ReadVariable(), GetHomeObjectFromFunction(jsFunc));
        Jump(success);
    }
}

void NewObjectStubBuilder::InitializeJSFunction(GateRef glue, GateRef func, GateRef kind)
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

    SetLexicalEnvToFunction(glue, func, Undefined());
    SetHomeObjectToFunction(glue, func, Undefined());
    SetProtoOrHClassToFunction(glue, func, Hole());
    SetWorkNodePointerToFunction(glue, func, NullPtr());
    SetMethodToFunction(glue, func, Undefined());

    Branch(HasPrototype(kind), &hasProto, &notProto);
    Bind(&hasProto);
    {
        auto funcprotoAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                        ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
        Branch(IsBaseKind(kind), &isBase, &notBase);
        Bind(&isBase);
        {
            SetPropertyInlinedProps(glue, func, hclass, funcprotoAccessor,
                                    Int32(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX));
            auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                       ConstantIndex::FUNCTION_NAME_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                    Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX));
            funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                    Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX));
            Branch(IsGeneratorKind(kind), &isGenerator, &exit);
            Bind(&isGenerator);
            {
                thisObj = CallRuntime(glue, RTSTUB_ID(InitializeGeneratorFunction), {kind});
                SetProtoOrHClassToFunction(glue, func, *thisObj);
                Jump(&exit);
            }
        }
        Bind(&notBase);
        {
            Branch(IsClassConstructorKind(kind), &exit, &notClassConstructor);
            Bind(&notClassConstructor);
            {
                CallRuntime(glue, RTSTUB_ID(FunctionDefineOwnProperty), {func, funcprotoAccessor, kind});
                Jump(&exit);
            }
        }
    }
    Bind(&notProto);
    {
        Branch(HasAccessor(kind), &hasAccess, &exit);
        Bind(&hasAccess);
        {
            auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                       ConstantIndex::FUNCTION_NAME_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX));
            funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX));
            Jump(&exit);
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
    Branch(TaggedIsString(obj), &isString, &isNotString);
    Bind(&isString);
    {
        object = CallRuntime(glue, RTSTUB_ID(PrimitiveStringCreate), { obj });;
        Jump(&afterObjectTransform);
    }
    Bind(&isNotString);
    {
        object = ToPrototypeOrObj(glue, obj);
        Jump(&afterObjectTransform);
    }
    Bind(&afterObjectTransform);
    Branch(TaggedIsUndefinedOrNull(*object), &empty, &tryGetEnumCache);
    Bind(&tryGetEnumCache);
    GateRef enumCache = TryGetEnumCache(glue, *object);
    Branch(TaggedIsUndefined(enumCache), &slowpath, &cacheHit);
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
                                             GateRef hclass, GateRef trackInfo, bool isEmptyArray)
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
    HeapAlloc(&result, &initialize, RegionSpaceFlag::IN_YOUNG_SPACE);
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
    SetRawHashcode(glue_, result->ReadVariable(), Int32(0), False());
    Jump(exit);
}

void NewObjectStubBuilder::AllocSlicedStringObject(Variable *result, Label *exit, GateRef from, GateRef length,
    FlatStringStubBuilder *flatString)
{
    auto env = GetEnvironment();

    size_ = AlignUp(IntPtr(SlicedString::SIZE), IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    Label afterAllocate(env);
    AllocateInYoung(result, &afterAllocate);

    Bind(&afterAllocate);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::SLICED_STRING_CLASS_INDEX);
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
    AllocateInYoung(result, &afterAllocate);

    Bind(&afterAllocate);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::TREE_STRING_CLASS_INDEX);
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
    Label newTargetIsBase(env);
    Label newTargetNotBase(env);
    Label checkHeapObject(env);
    Label isHeapObject(env);
    Label checkJSObject(env);
    Label callRuntime(env);
    Label newObject(env);

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(protoOrHclass, VariableType::JS_ANY(), Undefined());
    Branch(IsBase(newTarget), &newTargetIsBase, &newTargetNotBase);
    Bind(&newTargetIsBase);
    {
        protoOrHclass = Load(VariableType::JS_ANY(), superCtor,
            IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        Jump(&checkHeapObject);
    }
    Bind(&newTargetNotBase);
    {
        protoOrHclass = Load(VariableType::JS_ANY(), newTarget,
            IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        Jump(&checkHeapObject);
    }
    Bind(&checkHeapObject);
    Branch(TaggedIsHeapObject(*protoOrHclass), &isHeapObject, &callRuntime);
    Bind(&isHeapObject);
    Branch(IsJSHClass(*protoOrHclass), &checkJSObject, &callRuntime);
    Bind(&checkJSObject);
    auto objectType = GetObjectType(*protoOrHclass);
    Branch(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))),
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

GateRef NewObjectStubBuilder::LoadTrackInfo(GateRef glue, GateRef jsFunc, GateRef pc, GateRef profileTypeInfo,
    GateRef slotId, GateRef arrayLiteral, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(ret, VariableType::JS_POINTER(), Undefined());

    Label uninitialized(env);
    Label fastpath(env);
    GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
    Branch(TaggedIsHeapObject(slotValue), &fastpath, &uninitialized);
    Bind(&fastpath);
    {
        ret = slotValue;
        Jump(&exit);
    }
    Bind(&uninitialized);
    {
        auto hclass = LoadArrayHClassSlowPath(glue, jsFunc, pc, arrayLiteral, callback);
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
    GateRef glue, GateRef jsFunc, GateRef pc, GateRef arrayLiteral, ProfileOperation callback)
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
    Branch(Int32Equal(indexInfosLength, Int32(0)), &originLoad, &aotLoad);
    Bind(&aotLoad);
    {
        auto pfAddr = LoadPfHeaderFromConstPool(jsFunc);
        GateRef traceId = TruncPtrToInt32(PtrSub(pc, pfAddr));
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

GateRef NewObjectStubBuilder::CreateEmptyArray(
    GateRef glue, GateRef jsFunc, GateRef pc, GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(trackInfo, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(hclass, VariableType::JS_ANY(), Undefined());
    Label slowpath(env);
    Label mayFastpath(env);
    Label createArray(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &slowpath, &mayFastpath);
    Bind(&mayFastpath);
    {
        trackInfo = LoadTrackInfo(glue, jsFunc, pc, profileTypeInfo, slotId, Circuit::NullGate(), callback);
        hclass = Load(VariableType::JS_ANY(), *trackInfo, IntPtr(TrackInfo::CACHED_HCLASS_OFFSET));
        trackInfo = env->GetBuilder()->CreateWeakRef(*trackInfo);
        Jump(&createArray);
    }
    Bind(&slowpath);
    {
        hclass = LoadArrayHClassSlowPath(glue, jsFunc, pc, Circuit::NullGate(), callback);
        Jump(&createArray);
    }
    Bind(&createArray);
    GateRef result = CreateEmptyArrayCommon(glue, *hclass, *trackInfo);
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
    Branch(TaggedIsUndefined(profileTypeInfo), &slowpath, &mayFastpath);
    Bind(&mayFastpath);
    {
        trackInfo = LoadTrackInfo(glue, jsFunc, pc, profileTypeInfo, slotId, obj, callback);
        hclass = Load(VariableType::JS_ANY(), *trackInfo, IntPtr(TrackInfo::CACHED_HCLASS_OFFSET));
        trackInfo = env->GetBuilder()->CreateWeakRef(*trackInfo);
        Jump(&createArray);
    }
    Bind(&slowpath);
    {
        hclass = LoadArrayHClassSlowPath(glue, jsFunc, pc, obj, callback);
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

    Label afterAllocate(env);
    // Be careful. NO GC is allowed when initization is not complete.
    AllocateInYoung(result, &afterAllocate);
    Bind(&afterAllocate);
    Label noException(env);
    Branch(TaggedIsException(result->ReadVariable()), exit, &noException);
    Bind(&noException);
    {
        StoreHClass(glue_, result->ReadVariable(), iteratorHClass);
        SetHash(glue_, result->ReadVariable(), Int64(JSTaggedValue(0).GetRawData()));
        auto emptyArray = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue_, ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
        SetPropertiesArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray);
        SetElementsArray(VariableType::INT64(), glue_, result->ReadVariable(), emptyArray);

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
}

template void NewObjectStubBuilder::CreateJSCollectionIterator<JSSetIterator, JSSet>(
    Variable *result, Label *exit, GateRef set, GateRef kind);
template void NewObjectStubBuilder::CreateJSCollectionIterator<JSMapIterator, JSMap>(
    Variable *result, Label *exit, GateRef set, GateRef kind);

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
}  // namespace panda::ecmascript::kungfu
