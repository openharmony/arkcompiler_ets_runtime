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
#include "ecmascript/compiler/ic_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/ic/mega_ic_cache.h"

namespace panda::ecmascript::kungfu {
template<ICStubType type>
void ICStubBuilder::NamedICAccessorWithMega(Variable *cachedHandler, Label *tryICHandler)
{
    auto env = GetEnvironment();
    Label receiverIsHeapObject(env);
    BRANCH_LIKELY(TaggedIsHeapObject(receiver_), &receiverIsHeapObject, slowPath_);
    Bind(&receiverIsHeapObject);
    {
        // This function is only invoked by the JIT, so it is assumed that profiletypeinfo must exist and the first slot
        // must be a hole, thus no checks are performed
        Label exit(env);
        Label find(env);
        GateRef hclass = LoadHClass(receiver_);
        GateRef hash = HashFromHclassAndStringKey(glue_, hclass, propKey_);
        GateRef prop = PtrAdd(megaStubCache_,
                              PtrMul(ZExtInt32ToPtr(hash), IntPtr(MegaICCache::PropertyKey::GetPropertyKeySize())));
        GateRef propHclass =
            Load(VariableType::JS_POINTER(), prop, IntPtr(MegaICCache::PropertyKey::GetHclassOffset()));
        GateRef propKey = Load(VariableType::JS_ANY(), prop, IntPtr(MegaICCache::PropertyKey::GetKeyOffset()));
        GateRef hclassIsEqual = IntPtrEqual(hclass, propHclass);
        GateRef keyIsEqual = IntPtrEqual(propKey_, propKey);
        // profiling code
        IncMegaProbeCount(glue_);
        BRANCH_LIKELY(BitAnd(hclassIsEqual, keyIsEqual), &find, slowPath_);
        Bind(&find);
        {
            cachedHandler->WriteVariable(
                Load(VariableType::JS_ANY(), prop, IntPtr(MegaICCache::PropertyKey::GetResultsOffset())));
            // profiling code
            IncMegaHitCount(glue_);
            Jump(tryICHandler);
        }
    }
}

template<ICStubType type>
void ICStubBuilder::NamedICAccessor(Variable* cachedHandler, Label *tryICHandler)
{
    auto env = GetEnvironment();
    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label tryIC(env);
    if constexpr (type == ICStubType::LOAD) {
        BRANCH(TaggedIsHeapObject(receiver_), &receiverIsHeapObject, &receiverNotHeapObject);
    } else {
        BRANCH(TaggedIsHeapObject(receiver_), &receiverIsHeapObject, slowPath_);
    }
    Bind(&receiverIsHeapObject);
    {
        BRANCH(TaggedIsUndefined(profileTypeInfo_), tryFastPath_, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(
                profileTypeInfo_, slotId_);
            BRANCH(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
            Bind(&isHeapObject);
            {
                GateRef secondValue = GetValueFromTaggedArray(profileTypeInfo_, Int32Add(slotId_, Int32(1)));
                cachedHandler->WriteVariable(secondValue);
                Label tryPoly(env);
                GateRef hclass = LoadHClass(receiver_);
                BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
                       tryICHandler,
                       &tryPoly);
                Bind(&tryPoly);
                {
                    cachedHandler->WriteVariable(CheckPolyHClass(firstValue, hclass));
                    BRANCH(TaggedIsHole(cachedHandler->ReadVariable()), slowPath_, tryICHandler);
                }
            }
            Bind(&notHeapObject);
            {
                BRANCH(TaggedIsUndefined(firstValue), slowPath_, tryFastPath_);
            }
        }
    }
    if constexpr (type == ICStubType::STORE) {
        return;
    }
    Bind(&receiverNotHeapObject);
    {
        Label tryNumber(env);
        Label profileNotUndefined(env);
        BRANCH(TaggedIsNumber(receiver_), &tryNumber, slowPath_);
        Bind(&tryNumber);
        {
            BRANCH(TaggedIsUndefined(profileTypeInfo_), slowPath_, &profileNotUndefined);
            Bind(&profileNotUndefined);
            {
                GateRef firstValue = GetValueFromTaggedArray(profileTypeInfo_, slotId_);
                GateRef secondValue = GetValueFromTaggedArray(profileTypeInfo_, Int32Add(slotId_, Int32(1)));
                cachedHandler->WriteVariable(secondValue);
                Label isHeapObject(env);
                BRANCH(TaggedIsHeapObject(firstValue), &isHeapObject, slowPath_)
                Bind(&isHeapObject);
                {
                    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);
                    auto numberFunction = GetGlobalEnvValue(VariableType::JS_ANY(),
                                                            glueGlobalEnv, GlobalEnv::NUMBER_FUNCTION_INDEX);
                    GateRef ctorProtoOrHC =
                            Load(VariableType::JS_POINTER(), numberFunction,
                                 IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                    BRANCH(Equal(LoadObjectFromWeakRef(firstValue), ctorProtoOrHC), tryICHandler, slowPath_);
                }
            }
        }
    }
}

void ICStubBuilder::ValuedICAccessor(Variable* cachedHandler, Label *tryICHandler, Label* tryElementIC)
{
    auto env = GetEnvironment();
    Label receiverIsHeapObject(env);

    BRANCH(TaggedIsHeapObject(receiver_), &receiverIsHeapObject, slowPath_);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        BRANCH(TaggedIsUndefined(profileTypeInfo_), tryFastPath_, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(
                profileTypeInfo_, slotId_);
            BRANCH(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label tryWithElementPoly(env);
                GateRef hclass = LoadHClass(receiver_);
                BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
                       tryElementIC,
                       &tryPoly);
                Bind(&tryPoly);
                {
                    Label firstIsKey(env);
                    BRANCH(Int64Equal(firstValue, propKey_), &firstIsKey, &tryWithElementPoly);
                    Bind(&firstIsKey);
                    {
                        GateRef handler = CheckPolyHClass(cachedHandler->ReadVariable(), hclass);
                        cachedHandler->WriteVariable(handler);
                        BRANCH(TaggedIsHole(cachedHandler->ReadVariable()), slowPath_, tryICHandler);
                    }
                    Bind(&tryWithElementPoly);
                    {
                        Label checkSecond(env);
                        Label checkPoly(env);
                        BRANCH(TaggedIsWeak(firstValue), slowPath_, &checkSecond);
                        Bind(&checkSecond);
                        {
                            BRANCH(TaggedIsHole(cachedHandler->ReadVariable()), &checkPoly, slowPath_);
                        }
                        Bind(&checkPoly);
                        {
                            cachedHandler->WriteVariable(CheckPolyHClass(firstValue, hclass));
                            BRANCH(TaggedIsHole(cachedHandler->ReadVariable()), slowPath_, tryElementIC);
                        }
                    }
                }
            }
            Bind(&notHeapObject);
            {
                BRANCH(TaggedIsUndefined(firstValue), slowPath_, tryFastPath_);
            }
        }
    }
}

void ICStubBuilder::LoadICByName(
    Variable *result, Label *tryFastPath, Label *slowPath, Label *success, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label loadWithHandler(env);

    SetLabels(tryFastPath, slowPath, success);
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), Undefined());
    NamedICAccessor<ICStubType::LOAD>(&cachedHandler, &loadWithHandler);
    Bind(&loadWithHandler);
    {
        GateRef ret = LoadICWithHandler(glue_, receiver_, receiver_, *cachedHandler, callback);
        result->WriteVariable(ret);
        BRANCH(TaggedIsHole(ret), slowPath_, success_);
    }
}

void ICStubBuilder::LoadICByNameWithMega(Variable *result, Label *tryFastPath, Label *slowPath, Label *success,
                                         ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label loadWithHandler(env);

    SetLabels(tryFastPath, slowPath, success);
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), Undefined());
    NamedICAccessorWithMega<ICStubType::LOAD>(&cachedHandler, &loadWithHandler);
    Bind(&loadWithHandler);
    {
        GateRef ret = LoadICWithHandler(glue_, receiver_, receiver_, *cachedHandler, callback);
        result->WriteVariable(ret);
        BRANCH(TaggedIsNotHole(ret), success_, slowPath_);
    }
}

void ICStubBuilder::StoreICByName(Variable* result, Label* tryFastPath, Label *slowPath, Label *success)
{
    auto env = GetEnvironment();
    Label storeWithHandler(env);

    SetLabels(tryFastPath, slowPath, success);
    GateRef secondValue = GetValueFromTaggedArray(
        profileTypeInfo_, Int32Add(slotId_, Int32(1)));
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
    NamedICAccessor<ICStubType::STORE>(&cachedHandler, &storeWithHandler);
    Bind(&storeWithHandler);
    {
        GateRef ret = StoreICWithHandler(glue_, receiver_, receiver_, value_, *cachedHandler, callback_);
        result->WriteVariable(ret);
        BRANCH(TaggedIsHole(ret), slowPath_, success_);
    }
}

void ICStubBuilder::StoreICByNameWithMega(Variable *result, Label *tryFastPath, Label *slowPath, Label *success)
{
    auto env = GetEnvironment();
    Label storeWithHandler(env);

    SetLabels(tryFastPath, slowPath, success);
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), Undefined());
    NamedICAccessorWithMega<ICStubType::STORE>(&cachedHandler, &storeWithHandler);
    Bind(&storeWithHandler);
    {
        GateRef ret = StoreICWithHandler(glue_, receiver_, receiver_, value_, *cachedHandler, callback_);
        result->WriteVariable(ret);
        BRANCH(TaggedIsHole(ret), slowPath_, success_);
    }
}


void ICStubBuilder::LoadICByValue(
    Variable *result, Label *tryFastPath, Label *slowPath, Label *success, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label loadWithHandler(env);
    Label loadElement(env);
    Label handlerInfoIsElement(env);
    Label handlerInfoNotElement(env);
    Label handlerInfoIsStringElement(env);
    Label handlerInfoNotStringElement(env);
    Label handlerInfoIsTypedArrayElement(env);
    Label exit(env);

    SetLabels(tryFastPath, slowPath, success);
    GateRef secondValue = GetValueFromTaggedArray(
        profileTypeInfo_, Int32Add(slotId_, Int32(1)));
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
    DEFVARIABLE(ret, VariableType::JS_ANY(), secondValue);

    ValuedICAccessor(&cachedHandler, &loadWithHandler, &loadElement);
    Bind(&loadElement);
    {
        GateRef handlerInfo = GetInt64OfTInt(*cachedHandler);
        BRANCH(IsElement(handlerInfo), &handlerInfoIsElement, &handlerInfoNotElement);
        Bind(&handlerInfoIsElement);
        {
            ret = LoadElement(glue_, receiver_, propKey_);
            Jump(&exit);
        }
        Bind(&handlerInfoNotElement);
        {
            BRANCH(IsStringElement(handlerInfo), &handlerInfoIsStringElement, &handlerInfoNotStringElement);
            Bind(&handlerInfoIsStringElement);
            {
                ret = LoadStringElement(glue_, receiver_, propKey_);
                Jump(&exit);
            }
            Bind(&handlerInfoNotStringElement);
            {
                BRANCH(IsTypedArrayElement(handlerInfo), &handlerInfoIsTypedArrayElement, &exit);
                Bind(&handlerInfoIsTypedArrayElement);
                {
                    GateRef hclass = LoadHClass(receiver_);
                    GateRef jsType = GetObjectType(hclass);
                    BuiltinsTypedArrayStubBuilder typedArrayBuilder(reinterpret_cast<StubBuilder*>(this));
                    ret = typedArrayBuilder.LoadTypedArrayElement(glue_, receiver_, propKey_, jsType);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&loadWithHandler);
    {
        ret = LoadICWithHandler(glue_, receiver_, receiver_, *cachedHandler, callback);
        Jump(&exit);
    }
    Bind(&exit);
    result->WriteVariable(*ret);
    BRANCH(TaggedIsHole(*ret), slowPath_, success_);
}

void ICStubBuilder::StoreICByValue(Variable* result, Label* tryFastPath, Label *slowPath, Label *success)
{
    auto env = GetEnvironment();
    Label storeWithHandler(env);
    Label storeElement(env);
    SetLabels(tryFastPath, slowPath, success);
    GateRef secondValue = GetValueFromTaggedArray(
        profileTypeInfo_, Int32Add(slotId_, Int32(1)));
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
    ValuedICAccessor(&cachedHandler, &storeWithHandler, &storeElement);
    Bind(&storeElement);
    {
        GateRef ret = ICStoreElement(glue_, receiver_, propKey_, value_, *cachedHandler,
                                     true, profileTypeInfo_, Int32Add(slotId_, Int32(1)));
        result->WriteVariable(ret);
        BRANCH(TaggedIsHole(ret), slowPath_, success_);
    }
    Bind(&storeWithHandler);
    {
        GateRef ret = StoreICWithHandler(glue_, receiver_, receiver_, value_, *cachedHandler, callback_);
        result->WriteVariable(ret);
        BRANCH(TaggedIsHole(ret), slowPath_, success_);
    }
}

void ICStubBuilder::TryLoadGlobalICByName(Variable* result, Label* tryFastPath, Label *slowPath, Label *success)
{
    auto env = GetEnvironment();
    Label tryIC(env);

    SetLabels(tryFastPath, slowPath, success);
    BRANCH(TaggedIsUndefined(profileTypeInfo_), tryFastPath_, &tryIC);
    Bind(&tryIC);
    {
        GateRef handler = GetValueFromTaggedArray(profileTypeInfo_, slotId_);
        Label isHeapObject(env);
        BRANCH(TaggedIsHeapObject(handler), &isHeapObject, slowPath_);
        Bind(&isHeapObject);
        {
            GateRef ret = LoadGlobal(handler);
            result->WriteVariable(ret);
            BRANCH(TaggedIsHole(ret), slowPath_, success_);
        }
    }
}

void ICStubBuilder::TryStoreGlobalICByName(Variable* result, Label* tryFastPath, Label *slowPath, Label *success)
{
    auto env = GetEnvironment();
    Label tryIC(env);

    SetLabels(tryFastPath, slowPath, success);
    BRANCH(TaggedIsUndefined(profileTypeInfo_), tryFastPath_, &tryIC);
    Bind(&tryIC);
    {
        GateRef handler = GetValueFromTaggedArray(profileTypeInfo_, slotId_);
        Label isHeapObject(env);
        BRANCH(TaggedIsHeapObject(handler), &isHeapObject, slowPath_);
        Bind(&isHeapObject);
        {
            GateRef ret = StoreGlobal(glue_, value_, handler);
            result->WriteVariable(ret);
            BRANCH(TaggedIsHole(ret), slowPath_, success_);
        }
    }
}
}  // namespace panda::ecmascript::kungfu
