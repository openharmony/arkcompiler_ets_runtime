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

#include "ecmascript/compiler/object_operator_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_primitive_ref.h"

namespace panda::ecmascript::kungfu {

void ObjectOperatorStubBuilder::TryFastHandleStringKey(GateRef key, Variable *propKey, Variable *elemKey,
                                                       Label *isProperty, Label *isElement, Label *tryFailed)
{
    auto env = GetEnvironment();
    Label isInternString(env);

    BRANCH(IsInternalString(key), &isInternString, tryFailed);

    Bind(&isInternString);
    {
        Label isIntegerString(env);
        Label notIntegerString(env);

        BRANCH(IsIntegerString(key), &isIntegerString, &notIntegerString);

        Bind(&isIntegerString);
        {
            *elemKey = GetRawHashFromString(key);
            Jump(isElement);
        }

        Bind(&notIntegerString);
        {
            Label keyIsProperty(env);
            GateRef len = GetLengthFromString(key);
            BRANCH(Int32LessThanOrEqual(len, Int32(EcmaString::MAX_CACHED_INTEGER_SIZE)), &keyIsProperty, tryFailed);

            Bind(&keyIsProperty);
            *propKey = key;
            Jump(isProperty);
        }
    }
}

// ObjectOperator::HandleKey
void ObjectOperatorStubBuilder::HandleKey(GateRef glue, GateRef key, Variable *propKey, Variable *elemKey,
                                          Label *isProperty, Label *isElement, Label *hasException, GateRef hir)
{
    auto env = GetEnvironment();
    Label isInt(env);
    Label notInt(env);
    Label isString(env);
    Label notString(env);
    Label isDouble(env);
    Label notDouble(env);
    Label isSymbol(env);
    Label notSymbol(env);
    Label index64To32(env);

    DEFVARIABLE(index64, VariableType::INT64(), Int64(-1));
    BRANCH(TaggedIsInt(key), &isInt, &notInt);

    Bind(&isInt);
    {
        Label numberToString(env);
        index64 = GetInt64OfTInt(key);
        BRANCH(Int64GreaterThanOrEqual(*index64, Int64(0)), &index64To32, &numberToString);
        Bind(&numberToString);
        {
            *propKey = NumberToString(glue, key);
            Jump(isProperty);
        }
    }

    Bind(&notInt);
    BRANCH(TaggedIsString(key), &isString, &notString);

    Bind(&isString);
    {
        Label toInternString(env);
        Label notInternString(env);
        Label tryFailed(env);
        TryFastHandleStringKey(key, propKey, elemKey, isProperty, isElement, &tryFailed);

        Bind(&tryFailed);
        index64 = StringToElementIndex(glue, key);
        BRANCH(Int64Equal(*index64, Int64(-1)), &toInternString, &index64To32);
        Bind(&toInternString);
        {
            *propKey = key;
            BRANCH(IsInternalString(**propKey), isProperty, &notInternString);
            Bind(&notInternString);
            {
                // fixme(hewei): need the implementation of StringTable IR, callruntime now.
                *propKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), {**propKey});
                Jump(isProperty);
            }
        }
    }

    Bind(&notString);
    BRANCH(TaggedIsDouble(key), &isDouble, &notDouble);

    Bind(&isDouble);
    {
        GateRef number = GetDoubleOfTDouble(key);
        GateRef integer = ChangeFloat64ToInt32(number);
        Label inRange(env);
        Label isEqual(env);
        Label tryToString(env);

        BRANCH(LogicAndBuilder(env)
                   .And(DoubleGreaterThanOrEqual(number, Double(0)))
                   .And(DoubleLessThan(number, Double(JSObject::MAX_ELEMENT_INDEX)))
                   .Done(),
               &inRange,
               &tryToString);

        Bind(&inRange);
        BRANCH(DoubleEqual(number, ChangeInt32ToFloat64(integer)), &isEqual, &tryToString);

        Bind(&isEqual);
        {
            *elemKey = integer;
            Jump(isElement);
        }

        Bind(&tryToString);
        {
            Label notInternString(env);
            *propKey = NumberToString(glue, key);
            BRANCH(IsInternalString(**propKey), isProperty, &notInternString);
            Bind(&notInternString);
            {
                // fixme(hewei): need the implementation of StringTable IR, callruntime now.
                *propKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), {**propKey});
                Jump(isProperty);
            }
        }
    }

    Bind(&notDouble);
    BRANCH(IsSymbol(key), &isSymbol, &notSymbol);

    Bind(&isSymbol);
    {
        *propKey = key;
        Jump(isProperty);
    }

    Bind(&notSymbol);
    {
        Label noException(env);
        *propKey = ToPrimitive(glue, key, PreferredPrimitiveType::PREFER_STRING, hir);
        BRANCH(HasPendingException(glue), hasException, &noException);
        Bind(&noException);
        {
            Label toString(env);
            BRANCH(IsSymbol(**propKey), isProperty, &toString);
            Bind(&toString);
            {
                *propKey = JSTaggedValueToString(glue, **propKey, hir);
                // fixme(hewei): need the implementation of StringTable IR, callruntime now.
                *propKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), {**propKey});
                Jump(isProperty);
            }
        }
    }

    Bind(&index64To32);
    {
        *elemKey = TruncInt64ToInt32(*index64);
        Jump(isElement);
    }
}

template <bool keyIsElement>
void ObjectOperatorStubBuilder::CheckValidIndexOrKeyIsLength(GateRef glue, GateRef obj, GateRef key, Label *checkSucc,
                                                             Label *checkFail)
{
    auto env = GetEnvironment();
    if constexpr (keyIsElement) {
        /// For element, key is an int32 number.
        BRANCH(Int32LessThan(key, GetLengthFromString(obj)), checkSucc, checkFail);
    } else {
        Label keyIsString(env);
        BRANCH(TaggedIsString(key), &keyIsString, checkFail);
        Bind(&keyIsString);
        {
            GateRef lengthString =
                GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::LENGTH_STRING_INDEX);
            BRANCH(FastStringEqual(glue, key, lengthString), checkSucc, checkFail);
        }
    }
}

template <bool keyIsElement>
void ObjectOperatorStubBuilder::UpdateHolder(GateRef glue, Variable *holder, GateRef key, Label *holderUpdated)
{
    auto env = GetEnvironment();
    Label notEcmaObject(env);
    Label isString(env);
    Label toProtoType(env);
    Branch(IsEcmaObject(**holder), holderUpdated, &notEcmaObject);

    Bind(&notEcmaObject);
    {
        Branch(TaggedIsString(**holder), &isString, &toProtoType);
    }
    Bind(&isString);
    {
        Label checkSucess(env);
        CheckValidIndexOrKeyIsLength<keyIsElement>(glue, **holder, key, &checkSucess, &toProtoType);

        Bind(&checkSucess);
        {
            // fixme(hewei): need the implementation of JSTaggedValue::DefinePropertyOrThrow IR, callruntime now.
            *holder = CallRuntime(glue, RTSTUB_ID(PrimitiveStringCreate), {**holder});
            Jump(holderUpdated);
        }
    }

    Bind(&toProtoType);
    *holder = ToPrototypeOrObj(glue, **holder);
    Jump(holderUpdated);
}

template
void ObjectOperatorStubBuilder::UpdateHolder<false>(GateRef glue, Variable *holder, GateRef key, Label *holderUpdated);
template
void ObjectOperatorStubBuilder::UpdateHolder<true>(GateRef glue, Variable *holder, GateRef key, Label *holderUpdated);

GateRef ObjectOperatorStubBuilder::LookupPropertyInlinedProps(GateRef glue, GateRef obj, GateRef key, GateRef hir)

{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isJsObject(env);
    Label notJsGlobalObject(env);
    Label isDicMode(env);
    Label hasEntry(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());

    BRANCH(IsJSObject(obj), &isJsObject, &exit);
    Bind(&isJsObject);
    {
        BRANCH(IsJSGlobalObject(obj), &isDicMode, &notJsGlobalObject);
    }

    Bind(&notJsGlobalObject);
    {
        Label notDicMod(env);
        GateRef hclass = LoadHClass(obj);
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMod);
        Bind(&notDicMod);
        {
            GateRef layOutInfo = GetLayoutFromHClass(hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
            GateRef entryA = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum, hir);
            // if branch condition : entry != -1
            BRANCH(Int32NotEqual(entryA, Int32(-1)), &hasEntry, &exit);
        }
    }
    Bind(&isDicMode);
    {
        Label findInDic(env);
        GateRef array = GetPropertiesArray(obj);
        GateRef len = GetLengthOfTaggedArray(array);
        BRANCH(Int32Equal(len, Int32(0)), &exit, &findInDic);

        Bind(&findInDic);
        // int entry = dict->FindEntry(key)
        GateRef entryB = FindEntryFromNameDictionary(glue, array, key, hir);
        // if branch condition : entry != -1
        BRANCH(Int32NotEqual(entryB, Int32(-1)), &hasEntry, &exit);
    }

    Bind(&hasEntry);
    {
        result = TaggedTrue();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef ObjectOperatorStubBuilder::LookupElementInlinedProps(GateRef glue, GateRef obj, GateRef elementIdx,
                                                             [[maybe_unused]] GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isPrimitiveRef(env);
    Label isStringObject(env);
    Label notStringObject(env);
    Label isTypedArray(env);
    Label ordinaryObject(env);
    Label elementFound(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
    // fastpath for string obj
    BRANCH(IsJSPrimitiveRef(obj), &isPrimitiveRef, &notStringObject);
    Bind(&isPrimitiveRef);
    {
        GateRef value = Load(VariableType::JS_ANY(), obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
        BRANCH(TaggedIsString(value), &isStringObject, &notStringObject);
        Bind(&isStringObject);
        {
            GateRef strLength = GetLengthFromString(value);
            BRANCH(Int32LessThan(elementIdx, strLength), &elementFound, &exit);
        }
    }

    Bind(&notStringObject);
    BRANCH(IsTypedArray(obj), &isTypedArray, &ordinaryObject);

    Bind(&isTypedArray);
    {
        BuiltinsTypedArrayStubBuilder typedArrayStubBuilder(this);
        GateRef element =
            typedArrayStubBuilder.FastGetPropertyByIndex(glue, obj, elementIdx, GetObjectType(LoadHClass(obj)));
        BRANCH(TaggedIsUndefined(element), &exit, &elementFound);
    }

    Bind(&ordinaryObject);
    {
        Label findByIndex(env);
        GateRef elements = GetElementsArray(obj);
        GateRef len = GetLengthOfTaggedArray(elements);
        BRANCH(Int32Equal(len, Int32(0)), &exit, &findByIndex);

        Bind(&findByIndex);
        {
            Label isDicMode(env);
            Label notDicMode(env);
            BRANCH(IsDictionaryMode(elements), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                BRANCH(Int32UnsignedLessThanOrEqual(len, elementIdx), &exit, &lessThanLength);
                Bind(&lessThanLength);
                {
                    Label notHole(env);
                    GateRef value = GetTaggedValueWithElementsKind(glue, obj, elementIdx);
                    BRANCH(TaggedIsNotHole(value), &elementFound, &exit);
                }
            }
            Bind(&isDicMode);
            {
                GateRef entryA = FindElementFromNumberDictionary(glue, elements, elementIdx);
                BRANCH(Int32NotEqual(entryA, Int32(-1)), &elementFound, &exit);
            }
        }
    }

    Bind(&elementFound);
    {
        result = TaggedTrue();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template <bool keyIsElement>
void ObjectOperatorStubBuilder::LookupProperty(GateRef glue, Variable *holder, GateRef key, Label *isJSProxy,
                                               Label *ifFound, Label *notFound, GateRef hir)
{
    auto env = GetEnvironment();
    Label continuelyLookup(env);
    Label lookupInProtoChain(env);
    BRANCH(IsJsProxy(**holder), isJSProxy, &continuelyLookup);

    Bind(&continuelyLookup);
    {
        DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
        if constexpr (keyIsElement) {
            result = LookupElementInlinedProps(glue, **holder, key, hir);
        } else {
            result = LookupPropertyInlinedProps(glue, **holder, key, hir);
        }
        BRANCH(TaggedIsTrue(*result), ifFound, &lookupInProtoChain);
    }

    Bind(&lookupInProtoChain);
    {
        TryLookupInProtoChain<keyIsElement>(glue, holder, key, ifFound, notFound, isJSProxy, hir);
    }
}

template <bool keyIsElement>
GateRef ObjectOperatorStubBuilder::ShouldContinuelyLookupInProtoChain(GateRef holder)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label dontContinuelyLookup(env);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    if constexpr (keyIsElement) {
        BRANCH(IsTypedArray(holder), &dontContinuelyLookup, &exit);
        Bind(&dontContinuelyLookup);
        {
            result = False();
            Jump(&exit);
        }
    } else {
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template
void ObjectOperatorStubBuilder::LookupProperty<false>(GateRef glue, Variable *holder, GateRef key, Label *isJSProxy,
                                                      Label *ifFound, Label *notFound, GateRef hir);
template
void ObjectOperatorStubBuilder::LookupProperty<true>(GateRef glue, Variable *holder, GateRef key, Label *isJSProxy,
                                                     Label *ifFound, Label *notFound, GateRef hir);

template <bool keyIsElement>
void ObjectOperatorStubBuilder::TryLookupInProtoChain(GateRef glue, Variable *holder, GateRef key, Label *ifFound,
                                                      Label *notFound, Label *isJSProxy, GateRef hir)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());

    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(TaggedIsTrue(*result), ifFound, &next);
        Bind(&next);
        {
            Label noPendingException(env);
            Label notJSProxy(env);
            GateRef proto = GetPrototype(glue, **holder);
            BRANCH_UNLIKELY(HasPendingException(glue), &loopExit, &noPendingException);

            Bind(&noPendingException);
            {
                Label updateHolder(env);
                BRANCH(TaggedIsHeapObject(proto), &updateHolder, &loopExit);

                Bind(&updateHolder);
                {
                    *holder = proto;
                    BRANCH(IsJsProxy(**holder), isJSProxy, &notJSProxy);
                }
            }

            Bind(&notJSProxy);
            {
                if constexpr (keyIsElement) {
                    result = LookupElementInlinedProps(glue, **holder, key, hir);
                } else {
                    result = LookupPropertyInlinedProps(glue, **holder, key, hir);
                }
                BRANCH(ShouldContinuelyLookupInProtoChain<keyIsElement>(**holder), &loopEnd, &loopExit);
            }
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&loopExit);
    BRANCH(TaggedIsTrue(*result), ifFound, notFound);
}
}  // namespace panda::ecmascript::kungfu
