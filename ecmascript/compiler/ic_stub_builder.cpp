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
#include "ecmascript/compiler/rt_call_signature.h"
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
        Label findSecond(env);
        Label trySecondary(env);
        GateRef hclass = LoadHClass(glue_, receiver_);
        GateRef hash = HashFromHclassAndStringKey(glue_, hclass, propKey_);
        GateRef prop = PtrAdd(megaStubCache_,
                              PtrMul(ZExtInt32ToPtr(hash), IntPtr(MegaICCache::PropertyKey::GetPropertyKeySize())));
        GateRef propHclass =
            Load(VariableType::JS_POINTER(), glue_, prop, IntPtr(MegaICCache::PropertyKey::GetHclassOffset()));
        GateRef propKey =
            Load(VariableType::JS_ANY(), glue_, prop,
                 IntPtr(MegaICCache::PropertyKey::GetKeyOffset()));
        GateRef hclassIsEqual = IntPtrEqual(hclass, propHclass);
        GateRef keyIsEqual = IntPtrEqual(propKey_, propKey);

        // profiling code
        IncMegaProbeCount(glue_);
        BRANCH_LIKELY(BitAnd(hclassIsEqual, keyIsEqual), &find, &trySecondary);
        Bind(&find);
        {
            cachedHandler->WriteVariable(
                Load(VariableType::JS_ANY(), glue_, prop, IntPtr(MegaICCache::PropertyKey::GetResultsOffset())));
            // profiling code
            IncMegaHitCount(glue_);
            Jump(tryICHandler);
        }
        Bind(&trySecondary);
        {
            GateRef secondaryHash = HashSecondaryFromHclassAndStringKey(glue_, hclass, propKey_);
            GateRef secondaryArray = PtrAdd(
                megaStubCache_, IntPtr(MegaICCache::GetSecondaryOffset()));
            GateRef secondaryProp = PtrAdd(
                secondaryArray,
                PtrMul(ZExtInt32ToPtr(secondaryHash),
                       IntPtr(MegaICCache::PropertyKey::GetPropertyKeySize())));
            GateRef secondaryHclass =
                Load(VariableType::JS_POINTER(), glue_, secondaryProp,
                     IntPtr(MegaICCache::PropertyKey::GetHclassOffset()));
            GateRef secondaryKey =
                Load(VariableType::JS_ANY(), glue_, secondaryProp,
                     IntPtr(MegaICCache::PropertyKey::GetKeyOffset()));
            GateRef secondaryHclassIsEqual =
                IntPtrEqual(hclass, secondaryHclass);
            GateRef secondaryKeyIsEqual = IntPtrEqual(propKey_, secondaryKey);
            cachedHandler->WriteVariable(
                Load(VariableType::JS_ANY(), glue_, secondaryProp,
                     IntPtr(MegaICCache::PropertyKey::GetResultsOffset())));
            BRANCH_LIKELY(BitAnd(secondaryHclassIsEqual, secondaryKeyIsEqual),
                          &findSecond, slowPath_);
        }
        Bind(&findSecond);
        {
            // profiling code
            IncMegaHitCount(glue_);
            Jump(tryICHandler);
        }
    }
}

void ICStubBuilder::TryDesignatePrimitiveLoadIC(Label &tryDesignatePrimitive, Label &notDesignatePrimitive,
                                                PrimitiveType primitiveType, PrimitiveLoadICInfo info)
{
    size_t globalEnvIndex = 0;
    switch (primitiveType) {
        case PrimitiveType::PRIMITIVE_BOOLEAN:
            BRANCH(TaggedIsBoolean(receiver_), &tryDesignatePrimitive, &notDesignatePrimitive);
            globalEnvIndex = GlobalEnv::BOOLEAN_FUNCTION_INDEX;
            break;
        case PrimitiveType::PRIMITIVE_NUMBER:
            BRANCH(TaggedIsNumber(receiver_), &tryDesignatePrimitive, &notDesignatePrimitive);
            globalEnvIndex = GlobalEnv::NUMBER_FUNCTION_INDEX;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }

    Bind(&tryDesignatePrimitive);
    {
        auto primitiveFunction =
            GetGlobalEnvValue(VariableType::JS_ANY(), info.glue, info.glueGlobalEnv, globalEnvIndex);
        GateRef ctorProtoOrHC = Load(VariableType::JS_POINTER(), info.glue,
            primitiveFunction, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        auto env = GetEnvironment();
        Label tryPoly(env);
        BRANCH(Equal(LoadObjectFromWeakRef(info.profileFirstValue), ctorProtoOrHC), info.tryICHandler, &tryPoly);
        Bind(&tryPoly);
        {
            info.cachedHandler->WriteVariable(CheckPolyHClass(info.glue, info.profileFirstValue, ctorProtoOrHC));
            BRANCH(TaggedIsHole(info.cachedHandler->ReadVariable()), slowPath_, info.tryICHandler);
        }
    }
}

void ICStubBuilder::TryPrimitiveLoadIC(Variable* cachedHandler, Label *tryICHandler)
{
    auto env = GetEnvironment();
    Label profileNotUndefined(env);
    BRANCH(TaggedIsUndefined(profileTypeInfo_), slowPath_, &profileNotUndefined);
    Bind(&profileNotUndefined);
    {
        GateRef firstValue = GetValueFromTaggedArray(glue_, profileTypeInfo_, slotId_);
        GateRef secondValue = GetValueFromTaggedArray(glue_, profileTypeInfo_, Int32Add(slotId_, Int32(1)));
        cachedHandler->WriteVariable(secondValue);
        Label isHeapObject(env);
        BRANCH(TaggedIsHeapObject(firstValue), &isHeapObject, slowPath_)
        Bind(&isHeapObject);
        {
            GateRef glueGlobalEnv = GetGlobalEnv(glue_);

            Label isNumber(env);
            Label notNumber(env);
            TryDesignatePrimitiveLoadIC(isNumber, notNumber, PrimitiveType::PRIMITIVE_NUMBER,
                { glue_, glueGlobalEnv, firstValue, tryICHandler, cachedHandler });
            Bind(&notNumber);
            Label isBoolean(env);
            TryDesignatePrimitiveLoadIC(isBoolean, *slowPath_, PrimitiveType::PRIMITIVE_BOOLEAN,
                { glue_, glueGlobalEnv, firstValue, tryICHandler, cachedHandler });
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
        BRANCH_LIKELY(TaggedIsHeapObject(receiver_), &receiverIsHeapObject, &receiverNotHeapObject);
    } else {
        BRANCH_LIKELY(TaggedIsHeapObject(receiver_), &receiverIsHeapObject, slowPath_);
    }
    Bind(&receiverIsHeapObject);
    {
        BRANCH_UNLIKELY(TaggedIsUndefined(profileTypeInfo_), tryFastPath_, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(glue_, profileTypeInfo_, slotId_);
            BRANCH(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
            Bind(&isHeapObject);
            {
                GateRef secondValue =
                    GetValueFromTaggedArray(glue_, profileTypeInfo_, Int32Add(slotId_, Int32(1))); // 1: second slot
                cachedHandler->WriteVariable(secondValue);
                Label tryPoly(env);
                GateRef hclass = LoadHClass(glue_, receiver_);
                BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
                       tryICHandler,
                       &tryPoly);
                Bind(&tryPoly);
                {
                    cachedHandler->WriteVariable(CheckPolyHClass(glue_, firstValue, hclass));
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
        TryPrimitiveLoadIC(cachedHandler, tryICHandler);
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
        BRANCH_UNLIKELY(TaggedIsUndefined(profileTypeInfo_), tryFastPath_, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(
                glue_, profileTypeInfo_, slotId_);
            GateRef secondValue = GetValueFromTaggedArray(
                glue_, profileTypeInfo_, Int32Add(slotId_, Int32(1)));
            cachedHandler->WriteVariable(secondValue);
            BRANCH(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label tryWithElementPoly(env);
                GateRef hclass = LoadHClass(glue_, receiver_);
                BRANCH(Equal(LoadObjectFromWeakRef(firstValue), hclass),
                       tryElementIC,
                       &tryPoly);
                Bind(&tryPoly);
                {
                    Label firstIsKey(env);
                    BRANCH(Int64Equal(firstValue, propKey_), &firstIsKey, &tryWithElementPoly);
                    Bind(&firstIsKey);
                    {
                        GateRef handler = CheckPolyHClass(glue_, cachedHandler->ReadVariable(), hclass);
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
                            cachedHandler->WriteVariable(CheckPolyHClass(glue_, firstValue, hclass));
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
        BRANCH_UNLIKELY(TaggedIsHole(ret), slowPath_, success_);
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
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), Undefined());
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
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(ret, VariableType::JS_ANY(), Undefined());

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
                    GateRef hclass = LoadHClass(glue_, receiver_);
                    GateRef jsType = GetObjectType(hclass);
                    BuiltinsTypedArrayStubBuilder typedArrayBuilder(this, GetGlobalEnv(glue_));
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
    DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), Undefined());
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
        GateRef handler = GetValueFromTaggedArray(glue_, profileTypeInfo_, slotId_);
        Label isHeapObject(env);
        BRANCH(TaggedIsHeapObject(handler), &isHeapObject, slowPath_);
        Bind(&isHeapObject);
        {
            GateRef ret = LoadGlobal(glue_, handler);
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
        GateRef handler = GetValueFromTaggedArray(glue_, profileTypeInfo_, slotId_);
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
