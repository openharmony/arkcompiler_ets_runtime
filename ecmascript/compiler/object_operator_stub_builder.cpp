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

#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_operator.h"

namespace panda::ecmascript::kungfu {

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

    BRANCH(TaggedIsInt(key), &isInt, &notInt);

    Bind(&isInt);
    {
        Label numberToString(env);
        Label indexIsValid(env);
        DEFVARIABLE(index, VariableType::INT32(), Int32(-1));
        index = GetInt32OfTInt(key);
        BRANCH(Int32GreaterThanOrEqual(*index, Int32(0)), &indexIsValid, &numberToString);
        Bind(&indexIsValid);
        {
            *elemKey = *index;
            Jump(isElement);
        }
        Bind(&numberToString);
        {
            *propKey = NumberToString(glue, key);
            Jump(isProperty);
        }
    }

    Bind(&notInt);
    BRANCH(TaggedIsString(glue, key), &isString, &notString);

    Bind(&isString);
    {
        Label toInternString(env);
        Label index64To32(env);
        Label notInternString(env);
        DEFVARIABLE(index64, VariableType::INT64(), Int64(-1));
        BuiltinsStringStubBuilder stringStub(this, GetCurrentGlobalEnv());
        index64 = stringStub.StringToUint(glue, key, JSObject::MAX_ELEMENT_INDEX - 1);
        BRANCH(Int64Equal(*index64, Int64(-1)), &toInternString, &index64To32);
        Bind(&toInternString);
        {
            *propKey = key;
            BRANCH(IsInternalString(propKey->ReadVariable()), isProperty, &notInternString);
            Bind(&notInternString);
            {
                // fixme(hewei): need the implementation of StringTable IR, callruntime now.
                *propKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), {propKey->ReadVariable()});
                Jump(isProperty);
            }
        }

        Bind(&index64To32);
        {
            *elemKey = TruncInt64ToInt32(*index64);
            Jump(isElement);
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
            BRANCH(IsInternalString(propKey->ReadVariable()), isProperty, &notInternString);
            Bind(&notInternString);
            {
                // fixme(hewei): need the implementation of StringTable IR, callruntime now.
                *propKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), {propKey->ReadVariable()});
                Jump(isProperty);
            }
        }
    }

    Bind(&notDouble);
    BRANCH(IsSymbol(glue, key), &isSymbol, &notSymbol);

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
            BRANCH(IsSymbol(glue, propKey->ReadVariable()), isProperty, &toString);
            Bind(&toString);
            {
                *propKey = JSTaggedValueToString(glue, propKey->ReadVariable(), hir);
                // fixme(hewei): need the implementation of StringTable IR, callruntime now.
                *propKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), {propKey->ReadVariable()});
                Jump(isProperty);
            }
        }
    }
}

template <bool keyIsElement>
void ObjectOperatorStubBuilder::CheckValidIndexOrKeyIsLength(GateRef glue, GateRef holder, GateRef receiver,
                                                             GateRef key, Label *checkSucc, Label *checkFail)
{
    auto env = GetEnvironment();
    if constexpr (keyIsElement) {
        /// For element, key is an int32 number.
        BRANCH(Int32LessThan(key, GetLengthFromString(holder)), checkSucc, checkFail);
    } else {
        Label keyIsString(env);
        BRANCH(BitAnd(TaggedIsString(glue, key), TaggedIsString(glue, receiver)), &keyIsString, checkFail);
        Bind(&keyIsString);
        {
            GateRef lengthString =
                GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::LENGTH_STRING_INDEX);
            BRANCH(FastStringEqual(glue, key, lengthString), checkSucc, checkFail);
        }
    }
}

template <bool keyIsElement>
void ObjectOperatorStubBuilder::UpdateHolder(GateRef glue, ObjectOperatorResult &results,
                                             GateRef key, Label *holderUpdated)
{
    auto env = GetEnvironment();
    Label notEcmaObject(env);
    Label isString(env);
    Label checkIsPrimitiveBoolOrNumber(env);
    Label setOnProtoType(env);
    Label toProtoType(env);
    BRANCH(IsEcmaObject(glue, results.GetHolder()), holderUpdated, &notEcmaObject);

    Bind(&notEcmaObject);
    {
        BRANCH(TaggedIsString(glue, results.GetHolder()), &isString, &checkIsPrimitiveBoolOrNumber);
    }
    Bind(&isString);
    {
        Label checkSucess(env);
        CheckValidIndexOrKeyIsLength<keyIsElement>(glue, results.GetHolder(), results.GetReceiver(),
                                                   key, &checkSucess, &setOnProtoType);

        Bind(&checkSucess);
        {
            // fixme(hewei): need the implementation of JSTaggedValue::DefinePropertyOrThrow IR, callruntime now.
            *results.holder = CallRuntime(glue, RTSTUB_ID(PrimitiveStringCreate), {results.GetHolder()});
            Jump(holderUpdated);
        }
    }

    Bind(&checkIsPrimitiveBoolOrNumber);
    {
        BRANCH(BitOr(TaggedIsNumber(results.GetHolder()), TaggedIsBoolean(results.GetHolder())),
               &setOnProtoType, &toProtoType);
    }

    Bind(&setOnProtoType);
    {
        SetIsOnProtoType(results);
        Jump(&toProtoType);
    }

    Bind(&toProtoType);
    *results.holder = ToPrototypeOrObj(glue, results.GetHolder());
    Jump(holderUpdated);
}

void ObjectOperatorStubBuilder::LookupPropertyInlinedProps(GateRef glue, GateRef obj, GateRef key, Label *exit,
                                                           const ObjectOperatorOptions &options,
                                                           ObjectOperatorResult &results,
                                                           GateRef hir)
{
    auto env = GetEnvironment();
    Label isJsObject(env);
    Label notJsGlobalObject(env);
    Label isDicMode(env);

    BRANCH(IsJSObject(glue, obj), &isJsObject, exit);
    Bind(&isJsObject);
    {
        BRANCH(IsJSGlobalObject(glue, obj), &isDicMode, &notJsGlobalObject);
    }

    Bind(&notJsGlobalObject);
    {
        Label notDicMod(env);
        GateRef hclass = LoadHClass(glue, obj);
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMod);
        Bind(&notDicMod);
        {
            Label hasEntry(env);
            GateRef layOutInfo = GetLayoutFromHClass(glue, hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
            GateRef entryA = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum, hir);
            // if branch condition : entry != -1
            BRANCH(Int32NotEqual(entryA, Int32(-1)), &hasEntry, exit);
            Bind(&hasEntry);
            {
                if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
                    SetFound(results);
                    Jump(exit);
                } else {
                    ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
                           options.lookupKind == LookupKind::KIND_SET_PROPERTY);
                    GateRef attr = GetPropAttrFromLayoutInfo(glue, layOutInfo, entryA);
                    GateRef result = JSObjectGetPropertyWithRep(glue, obj, hclass, attr);
                    SetFound(results, result, entryA, attr);
                    Jump(exit);
                }
            }
        }
    }
    Bind(&isDicMode);
    {
        Label findInDic(env);
        Label hasEntry(env);
        GateRef array = GetPropertiesArray(glue, obj);
        GateRef len = GetLengthOfTaggedArray(array);
        BRANCH(Int32Equal(len, Int32(0)), exit, &findInDic);

        Bind(&findInDic);
        // int entry = dict->FindEntry(key)
        GateRef entryB = FindEntryFromHashTable<NameDictionary>(glue, array, key, hir);
        // if branch condition : entry != -1

        BRANCH(Int32NotEqual(entryB, Int32(-1)), &hasEntry, exit);
        Bind(&hasEntry);
        {
            if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
                SetFound(results);
                Jump(exit);
            } else {
                ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
                       options.lookupKind == LookupKind::KIND_SET_PROPERTY);
                GateRef attr = GetAttributesFromDictionary<NumberDictionary>(glue, array, entryB);
                GateRef value = GetValueFromDictionary<NumberDictionary>(glue, array, entryB);
                SetFound(results, value, entryB, attr);
                Jump(exit);
            }
        }
    }
}

void ObjectOperatorStubBuilder::LookupElementInlinedProps(GateRef glue, GateRef obj, GateRef elementIdx, Label *exit,
                                                          const ObjectOperatorOptions &options,
                                                          ObjectOperatorResult &results,
                                                          GateRef hir)
{
    auto env = GetEnvironment();
    Label isPrimitiveRef(env);
    Label isStringObject(env);
    Label notStringObject(env);
    Label isTypedArray(env);
    Label ordinaryObject(env);
    // fastpath for string obj
    BRANCH(IsJSPrimitiveRef(glue, obj), &isPrimitiveRef, &notStringObject);
    Bind(&isPrimitiveRef);
    {
        GateRef value = Load(VariableType::JS_ANY(), glue, obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
        BRANCH(TaggedIsString(glue, value), &isStringObject, &notStringObject);
        Bind(&isStringObject);
        {
            Label elementFound(env);
            GateRef strLength = GetLengthFromString(value);
            BRANCH(Int32LessThan(elementIdx, strLength), &elementFound, &notStringObject);
            Bind(&elementFound);
            {
                if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
                    SetFound(results);
                    Jump(exit);
                } else {
                    ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
                           options.lookupKind == LookupKind::KIND_SET_PROPERTY);
                    // flatten string so next time we use the string, it is a flat string.
                    Label afterFlatten(env);
                    BuiltinsStringStubBuilder stringBuilder(this, GetCurrentGlobalEnv());
                    FlatStringStubBuilder thisFlat(this);
                    thisFlat.FlattenString(glue, results.GetHolder(), &afterFlatten);
                    Bind(&afterFlatten);
                    {
                        StringInfoGateRef stringInfoGate(&thisFlat);
                        // get value
                        auto strValue = stringBuilder.FastSubString(glue, results.GetHolder(),
                                                                    elementIdx, Int32(1), stringInfoGate);
                        // construct property attribute with EnumerableField
                        GateRef attr = Int64(PropertyAttributes::GetDefaultAttributes());
                        SetEnumerableFiledInPropAttr(attr, Int32(1));
                        SetFound(results, strValue, elementIdx, attr);
                        Jump(exit);
                    }
                }
            }
        }
    }

    Bind(&notStringObject);
    BRANCH(IsTypedArray(glue, obj), &isTypedArray, &ordinaryObject);

    Bind(&isTypedArray);
    {
        Label elementFound(env);
        BuiltinsTypedArrayStubBuilder typedArrayStubBuilder(this, GetCurrentGlobalEnv());
        GateRef element =
            typedArrayStubBuilder.FastGetPropertyByIndex(glue, obj, elementIdx, GetObjectType(LoadHClass(glue, obj)));

        BRANCH(TaggedIsHole(element), exit, &elementFound);
        Bind(&elementFound);
        if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
            SetFound(results);
            Jump(exit);
        } else {
            ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
                   options.lookupKind == LookupKind::KIND_SET_PROPERTY);
            SetFound(results, element, elementIdx, Int64(PropertyAttributes::GetDefaultAttributes()));
            Jump(exit);
        }
    }

    Bind(&ordinaryObject);
    {
        Label findByIndex(env);
        GateRef elements = GetElementsArray(glue, obj);
        GateRef len = GetLengthOfTaggedArray(elements);
        BRANCH(Int32Equal(len, Int32(0)), exit, &findByIndex);

        Bind(&findByIndex);
        {
            Label isDicMode(env);
            Label notDicMode(env);
            BRANCH(IsDictionaryMode(glue, elements), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                BRANCH(Int32UnsignedLessThanOrEqual(len, elementIdx), exit, &lessThanLength);
                Bind(&lessThanLength);
                {
                    Label notHole(env);
                    Label elementFound(env);
                    GateRef value = GetTaggedValueWithElementsKind(glue, obj, elementIdx);
                    BRANCH(TaggedIsHole(value), exit, &elementFound);
                    Bind(&elementFound);
                    {
                        if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
                            SetFound(results);
                            Jump(exit);
                        } else {
                            ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
                                   options.lookupKind == LookupKind::KIND_SET_PROPERTY);
                            SetFound(results, value, elementIdx, Int64(PropertyAttributes::GetDefaultAttributes()));
                            Jump(exit);
                        }
                    }
                }
            }
            Bind(&isDicMode);
            {
                Label elementFound(env);
                GateRef entryA = FindElementFromNumberDictionary(glue, elements, elementIdx);
                BRANCH(Int32NotEqual(entryA, Int32(-1)), &elementFound, exit);
                Bind(&elementFound);
                {
                    if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
                        SetFound(results);
                        Jump(exit);
                    } else {
                        ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
                               options.lookupKind == LookupKind::KIND_SET_PROPERTY);
                        GateRef attr = GetAttributesFromDictionary<NumberDictionary>(glue, elements, entryA);
                        GateRef value = GetValueFromDictionary<NumberDictionary>(glue, elements, entryA);
                        SetFound(results, value, elementIdx, attr);
                        Jump(exit);
                    }
                }
            }
        }
    }
}

// StartLookup is the entrypoint for IR ObjectOperator.
template <ObjectOperatorStubBuilder::StartLookupType startLookupType>
void ObjectOperatorStubBuilder::StartLookup(GateRef glue, GateRef key, Label *exit,
                                            const ObjectOperatorOptions &options, ObjectOperatorResult &results,
                                            GateRef hir)
{
    auto env = GetEnvironment();
    Label updateReceiver(env);
    Label hasReceiver(env);
    Label handleKey(env);
    Label isProperty(env);
    Label isElement(env);
    Label lookupProperty(env);

    if (options.lookupKind == LookupKind::KIND_HAS_PROPERTY) {
        // HasProperty not need to check receiver, for efficiency
        Jump(&handleKey);
    } else {
        ASSERT(options.lookupKind == LookupKind::KIND_GET_PROPERTY ||
               options.lookupKind == LookupKind::KIND_SET_PROPERTY);

        if constexpr (startLookupType == StartLookupType::HAS_RECEIVER) {
            SetHasReceiver(results);
        } else {
            *results.receiver = results.GetHolder();
        }

        Jump(&handleKey);
    }

    // 1. handle property key
    Bind(&handleKey);
    HandleKey(glue, key, results.propKey, results.elemKey, &isProperty, &isElement, exit, hir);

    // 2(1). start lookup when key is property
    Bind(&isProperty);
    {
        Label holderUpdated(env);
        UpdateHolder<false>(glue, results, results.GetPropKey(), &holderUpdated);

        Bind(&holderUpdated);
        {
            if (options.type == OperatorType::OWN) {
                LookupPropertyInlinedProps(glue, results.GetHolder(), results.GetPropKey(),
                                           exit, options, results, hir);
            } else {
                ASSERT(options.type == OperatorType::PROTOTYPE_CHAIN);
                LookupProperty<false>(glue, results.GetPropKey(), exit, options, results, hir);
            }
        }
    }

    // 2(2). start lookup when key is element
    Bind(&isElement);
    {
        Label holderUpdated(env);
        UpdateHolder<true>(glue, results, results.GetElemKey(), &holderUpdated);

        Bind(&holderUpdated);
        {
            if (options.type == OperatorType::OWN) {
                LookupElementInlinedProps(glue, results.GetHolder(), results.GetElemKey(), exit, options, results, hir);
            } else {
                LookupProperty<true>(glue, results.GetElemKey(), exit, options, results, hir);
            }
        }
    }
}

template
void ObjectOperatorStubBuilder::StartLookup<ObjectOperatorStubBuilder::StartLookupType::HAS_RECEIVER>(
    GateRef glue, GateRef key, Label *exit,
    const ObjectOperatorOptions &options, ObjectOperatorResult &results,
    GateRef hir);
template
void ObjectOperatorStubBuilder::StartLookup<ObjectOperatorStubBuilder::StartLookupType::NO_RECEIVER>(
    GateRef glue, GateRef key, Label *exit,
    const ObjectOperatorOptions &options, ObjectOperatorResult &results,
    GateRef hir);

template <bool keyIsElement>
void ObjectOperatorStubBuilder::LookupProperty(GateRef glue, GateRef key,
                                               Label *exit, const ObjectOperatorOptions &options,
                                               ObjectOperatorResult &results, GateRef hir)
{
    auto env = GetEnvironment();
    Label continuelyLookup(env);
    Label lookupInProtoChain(env);
    BRANCH(TaggedIsJSProxy(glue, results.GetHolder()), exit, &continuelyLookup);

    Bind(&continuelyLookup);
    {
        Label checkResult(env);
        if constexpr (keyIsElement) {
            LookupElementInlinedProps(glue, results.GetHolder(), key, &checkResult, options, results, hir);
        } else {
            LookupPropertyInlinedProps(glue, results.GetHolder(), key, &checkResult, options, results, hir);
        }
        Bind(&checkResult);
        BRANCH(IsFound(results.metaData), exit, &lookupInProtoChain);
    }

    Bind(&lookupInProtoChain);
    {
        TryLookupInProtoChain<keyIsElement>(glue, key, exit, options, results, hir);
    }
}

template <bool keyIsElement>
void ObjectOperatorStubBuilder::TryLookupInProtoChain(GateRef glue, GateRef key, Label *exit,
                                                      const ObjectOperatorOptions &options,
                                                      ObjectOperatorResult &results, GateRef hir)
{
    auto env = GetEnvironment();
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(IsFound(results.metaData), &loopExit, &next);
        Bind(&next);
        {
            Label noPendingException(env);
            Label notJSProxy(env);
            GateRef proto = GetPrototype(glue, results.GetHolder());
            BRANCH_UNLIKELY(HasPendingException(glue), &loopExit, &noPendingException);

            Bind(&noPendingException);
            {
                Label updateHolder(env);
                BRANCH(TaggedIsHeapObject(proto), &updateHolder, &loopExit);

                Bind(&updateHolder);
                {
                    *results.holder = proto;
                    SetIsOnProtoType(results);
                    BRANCH(TaggedIsJSProxy(glue, results.GetHolder()), exit, &notJSProxy);
                }
            }

            Bind(&notJSProxy);
            {
                if constexpr (keyIsElement) {
                    LookupElementInlinedProps(glue, results.GetHolder(), key, &loopEnd, options, results, hir);
                } else {
                    LookupPropertyInlinedProps(glue, results.GetHolder(), key, &loopEnd, options, results, hir);
                }
            }
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(exit);
}

void ObjectOperatorStubBuilder::InitializeOperatorResults(ObjectOperatorResult &result)
{
    auto env = GetEnvironment();
    result.metaData = new Variable(env, VariableType::INT32(), NextVariableId(), Int32(0));
    result.elemKey = new Variable(env, VariableType::INT32(), NextVariableId(), Int32(ObjectOperator::NOT_FOUND_INDEX));
    result.propKey = new Variable(env, VariableType::JS_ANY(), NextVariableId(), Undefined());
    result.index = new Variable(env, VariableType::INT32(), NextVariableId(), Int32(ObjectOperator::NOT_FOUND_INDEX));
    result.value = new Variable(env, VariableType::JS_ANY(), NextVariableId(), Undefined());
    result.holder = new Variable(env, VariableType::JS_ANY(), NextVariableId(), Undefined());
    result.receiver = new Variable(env, VariableType::JS_ANY(), NextVariableId(), Hole());
    result.attr = new Variable(env, VariableType::INT64(), NextVariableId(), Int64(-1));
}

void ObjectOperatorStubBuilder::FinalizeOperatorResults(ObjectOperatorResult &result)
{
    delete result.metaData;
    delete result.elemKey;
    delete result.propKey;
    delete result.index;
    delete result.value;
    delete result.holder;
    delete result.receiver;
    delete result.attr;
}

GateRef ObjectOperatorStubBuilder::IsFound(Variable *metaData)
{
    return Int32NotEqual(Int32And(metaData->ReadVariable(), Int32(1 << IS_FOUND_BIT)), Int32(0));
}

void ObjectOperatorStubBuilder::SetFound(ObjectOperatorResult &opResult)
{
    *opResult.metaData = Int32Or(opResult.GetMetaData(), Int32(1 << IS_FOUND_BIT));
}

void ObjectOperatorStubBuilder::SetFound(ObjectOperatorResult &opResult, GateRef value, GateRef index, GateRef attr)
{
    *opResult.metaData = Int32Or(opResult.GetMetaData(), Int32(1 << IS_FOUND_BIT));
    *opResult.value = value;
    *opResult.index = index;
    *opResult.attr = attr;
}

void ObjectOperatorStubBuilder::SetIsOnProtoType(ObjectOperatorResult &opResult)
{
    *opResult.metaData = Int32Or(opResult.GetMetaData(), Int32(1 << IS_ON_PROTOTYPE_BIT));
}

void ObjectOperatorStubBuilder::SetHasReceiver(ObjectOperatorResult &opResult)
{
    *opResult.metaData = Int32Or(opResult.GetMetaData(), Int32(1 << HAS_RECEIVER_BIT));
}

GateRef ObjectOperatorStubBuilder::IsElement(ObjectOperatorResult &opResult)
{
    return Int32NotEqual(opResult.elemKey->ReadVariable(), Int32(ObjectOperator::NOT_FOUND_INDEX));
}

GateRef ObjectOperatorStubBuilder::IsAccessorDescriptor(ObjectOperatorResult &opResult)
{
    return IsAccessor(opResult.GetAttr());
}
}  // namespace panda::ecmascript::kungfu
