/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_object_stub_builder.h"

#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/typed_array_stub_builder.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript::kungfu {
GateRef BuiltinsObjectStubBuilder::CreateListFromArrayLike(GateRef glue, GateRef arrayObj)
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
    Branch(TaggedIsHeapObject(arrayObj), &targetIsHeapObject, &targetNotEcmaObject);
    Bind(&targetIsHeapObject);
    Branch(TaggedObjectIsEcmaObject(arrayObj), &targetIsEcmaObject, &targetNotEcmaObject);
    Bind(&targetNotEcmaObject);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(TargetTypeNotObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        Jump(&exit);
    }
    Bind(&targetIsEcmaObject);
    {
        // 4. Let len be ToLength(Get(obj, "length")).
        GateRef lengthString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::LENGTH_STRING_INDEX);
        GateRef value = FastGetPropertyByName(glue, arrayObj, lengthString, ProfileOperation());
        GateRef number = ToLength(glue, value);
        // 5. ReturnIfAbrupt(len).
        Label isPendingException1(env);
        Label noPendingException1(env);
        Branch(HasPendingException(glue), &isPendingException1, &noPendingException1);
        Bind(&isPendingException1);
        {
            Jump(&exit);
        }
        Bind(&noPendingException1);
        {
            Label indexInRange(env);
            Label indexOutRange(env);

            GateRef doubleLen = GetDoubleOfTNumber(number);
            Branch(DoubleGreaterThan(doubleLen, Double(JSObject::MAX_ELEMENT_INDEX)), &indexOutRange, &indexInRange);
            Bind(&indexOutRange);
            {
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(LenGreaterThanMax));
                CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                Jump(&exit);
            }
            Bind(&indexInRange);
            {
                GateRef int32Len = DoubleToInt(glue, doubleLen);
                // 6. Let list be an empty List.
                NewObjectStubBuilder newBuilder(this);
                GateRef array = newBuilder.NewTaggedArray(glue, int32Len);
                Label targetIsTypeArray(env);
                Label targetNotTypeArray(env);
                Branch(IsTypedArray(arrayObj), &targetIsTypeArray, &targetNotTypeArray);
                Bind(&targetIsTypeArray);
                {
                    TypedArrayStubBuilder arrayStubBuilder(this);
                    arrayStubBuilder.FastCopyElementToArray(glue, arrayObj, array);
                    // c. ReturnIfAbrupt(next).
                    Label isPendingException2(env);
                    Label noPendingException2(env);
                    Branch(HasPendingException(glue), &isPendingException2, &noPendingException2);
                    Bind(&isPendingException2);
                    {
                        Jump(&exit);
                    }
                    Bind(&noPendingException2);
                    {
                        res = array;
                        Jump(&exit);
                    }
                }
                Bind(&targetNotTypeArray);
                // 8. Repeat while index < len
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Label isPendingException3(env);
                Label noPendingException3(env);
                Label storeValue(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int32UnsignedLessThan(*index, int32Len), &storeValue, &afterLoop);
                    Bind(&storeValue);
                    {
                        GateRef next = FastGetPropertyByIndex(glue, arrayObj, *index, ProfileOperation());
                        // c. ReturnIfAbrupt(next).
                        Branch(HasPendingException(glue), &isPendingException3, &noPendingException3);
                        Bind(&isPendingException3);
                        {
                            Jump(&exit);
                        }
                        Bind(&noPendingException3);
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, next);
                        index = Int32Add(*index, Int32(1));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead);
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

GateRef BuiltinsObjectStubBuilder::CreateArrayFromList(GateRef glue, GateRef elements)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    GateRef intialHClass = Load(VariableType::JS_ANY(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    GateRef len = GetLengthOfTaggedArray(elements);
    result = newBuilder.NewJSObject(glue, intialHClass);
    SetPropertyInlinedProps(glue, *result, intialHClass, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    SetArrayLength(glue, *result, len);
    SetExtensibleToBitfield(glue, *result, true);
    SetElementsArray(VariableType::JS_POINTER(), glue_, *result, elements);
    auto res = *result;
    return res;
}

void BuiltinsObjectStubBuilder::ToString(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label ecmaObj(env);
    // undefined
    Label undefined(env);
    Label checknull(env);
    Branch(TaggedIsUndefined(thisValue_), &undefined, &checknull);
    Bind(&undefined);
    {
        *result = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::UNDEFINED_TO_STRING_INDEX);
        Jump(exit);
    }
    // null
    Bind(&checknull);
    Label null(env);
    Label checkObject(env);
    Branch(TaggedIsUndefined(thisValue_), &null, &checkObject);
    Bind(&null);
    {
        *result = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::NULL_TO_STRING_INDEX);
        Jump(exit);
    }

    Bind(&checkObject);
    Branch(IsEcmaObject(thisValue_), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    {
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);
        GateRef toStringTagSymbol = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                      GlobalEnv::TOSTRINGTAG_SYMBOL_INDEX);
        GateRef tag = FastGetPropertyByName(glue_, thisValue_, toStringTagSymbol, ProfileOperation());

        Label defaultToString(env);
        Branch(TaggedIsString(tag), slowPath, &defaultToString);
        Bind(&defaultToString);
        {
            // default object
            Label objectTag(env);
            Branch(IsJSObjectType(thisValue_, JSType::JS_OBJECT), &objectTag, slowPath);
            Bind(&objectTag);
            {
                // [object object]
                *result = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::OBJECT_TO_STRING_INDEX);
                Jump(exit);
            }
        }
    }
}

GateRef BuiltinsObjectStubBuilder::TransProtoWithoutLayout(GateRef hClass, GateRef proto)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef key = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
        ConstantIndex::PROTOTYPE_STRING_INDEX);
    GateRef newClass = CallNGCRuntime(glue_, RTSTUB_ID(JSHClassFindProtoTransitions), { hClass, key, proto });
    Label undef(env);
    Label find(env);
    Branch(IntPtrEqual(TaggedCastToIntPtr(newClass), IntPtr(0)), &undef, &find);
    Bind(&find);
    {
        result = newClass;
        Jump(&exit);
    }
    Bind(&undef);
    {
        result = CallRuntime(glue_, RTSTUB_ID(HClassCloneWithAddProto), { hClass, key, proto });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsObjectStubBuilder::OrdinaryNewJSObjectCreate(GateRef proto)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef hClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
        ConstantIndex::OBJECT_HCLASS_INDEX);
    GateRef newClass = TransProtoWithoutLayout(hClass, proto);
    Label exception(env);
    Label noexception(env);
    Branch(TaggedIsException(newClass), &exception, &noexception);
    Bind(&exception);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noexception);
    NewObjectStubBuilder newBuilder(this);
    GateRef newObj = newBuilder.NewJSObject(glue_, newClass);
    Label exceptionNewObj(env);
    Label noexceptionNewObj(env);
    Branch(TaggedIsException(newObj), &exceptionNewObj, &noexceptionNewObj);
    Bind(&exceptionNewObj);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noexceptionNewObj);
    {
        SetExtensibleToBitfield(glue_, newObj, True());
        result = newObj;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::Create(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label newObject(env);

    GateRef proto = GetCallArg0(numArgs_);
    GateRef protoIsNull = TaggedIsNull(proto);
    GateRef protoIsEcmaObj = IsEcmaObject(proto);
    GateRef protoIsJSShared = TaggedIsShared(proto);
    Branch(BoolOr(BoolAnd(BoolNot(protoIsEcmaObj), BoolNot(protoIsNull)), protoIsJSShared), slowPath, &newObject);
    Bind(&newObject);
    {
        Label noProperties(env);
        GateRef propertiesObject = GetCallArg1(numArgs_);
        Branch(TaggedIsUndefined(propertiesObject), &noProperties, slowPath);
        Bind(&noProperties);
        {
            // OrdinaryNewJSObjectCreate
            *result = OrdinaryNewJSObjectCreate(proto);
            Jump(exit);
        }
    }
}

void BuiltinsObjectStubBuilder::AssignEnumElementProperty(Variable *result, Label *funcExit,
    GateRef toAssign, GateRef source)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);

    GateRef elements = GetElementsArray(source);
    Label dictionaryMode(env);
    Label notDictionaryMode(env);
    Branch(IsDictionaryMode(elements), &dictionaryMode, &notDictionaryMode);
    Bind(&notDictionaryMode);
    {
        GateRef len = GetLengthOfTaggedArray(elements);
        DEFVARIABLE(idx, VariableType::INT32(), Int32(0));
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);

        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int32LessThan(*idx, len), &next, &loopExit);
            Bind(&next);
            GateRef value = GetValueFromTaggedArray(elements, *idx);
            Label notHole(env);
            Branch(TaggedIsHole(value), &loopEnd, &notHole);
            Bind(&notHole);
            {
                // key, value
                FastSetPropertyByIndex(glue_, toAssign, *idx, value);
                Label exception(env);
                Branch(HasPendingException(glue_), &exception, &loopEnd);
                Bind(&exception);
                {
                    *result = Exception();
                    Jump(funcExit);
                }
            }
        }
        Bind(&loopEnd);
        idx = Int32Add(*idx, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&exit);
    }
    Bind(&dictionaryMode);
    {
        // NumberDictionary::VisitAllEnumProperty
        GateRef sizeIndex = Int32(TaggedHashTable<NumberDictionary>::SIZE_INDEX);
        GateRef size = GetInt32OfTInt(GetValueFromTaggedArray(elements, sizeIndex));
        DEFVARIABLE(idx, VariableType::INT32(), Int32(0));
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);

        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int32LessThan(*idx, size), &next, &loopExit);
            Bind(&next);
            GateRef key = GetKeyFromDictionary<NumberDictionary>(elements, *idx);
            Label checkEnumerable(env);
            Branch(BoolOr(TaggedIsUndefined(key), TaggedIsHole(key)), &loopEnd, &checkEnumerable);
            Bind(&checkEnumerable);
            {
                GateRef attr = GetAttributesFromDictionary<NumberDictionary>(elements, *idx);
                Label enumerable(env);
                Branch(IsEnumerable(attr), &enumerable, &loopEnd);
                Bind(&enumerable);
                {
                    GateRef value = GetValueFromDictionary<NumberDictionary>(elements, *idx);
                    Label notHole(env);
                    Branch(TaggedIsHole(value), &loopEnd, &notHole);
                    Bind(&notHole);
                    {
                        // value
                        FastSetPropertyByIndex(glue_, toAssign, *idx, value);
                        Label exception(env);
                        Branch(HasPendingException(glue_), &exception, &loopEnd);
                        Bind(&exception);
                        {
                            *result = Exception();
                            Jump(funcExit);
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        idx = Int32Add(*idx, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsObjectStubBuilder::LayoutInfoAssignAllEnumProperty(Variable *result, Label *funcExit,
    GateRef toAssign, GateRef source)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);

    // LayoutInfo::VisitAllEnumProperty
    GateRef cls = LoadHClass(source);
    GateRef num = GetNumberOfPropsFromHClass(cls);
    GateRef layout = GetLayoutFromHClass(cls);
    DEFVARIABLE(idx, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*idx, num), &next, &loopExit);
        Bind(&next);

        GateRef key = GetKeyFromLayoutInfo(layout, *idx);
        GateRef attr = GetPropAttrFromLayoutInfo(layout, *idx);
        Label stringKey(env);
        Branch(TaggedIsString(key), &stringKey, &loopEnd);
        Bind(&stringKey);
        {
            Label enumerable(env);
            Branch(IsEnumerable(attr), &enumerable, &loopEnd);
            Bind(&enumerable);
            {
                DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
                value = JSObjectGetProperty(source, cls, attr);
                // exception
                Label exception0(env);
                Label noexception0(env);
                Branch(HasPendingException(glue_), &exception0, &noexception0);
                Bind(&exception0);
                {
                    *result = Exception();
                    Jump(funcExit);
                }
                Bind(&noexception0);
                Label propertyBox(env);
                Label checkAccessor(env);
                Label setValue(env);
                Branch(TaggedIsPropertyBox(*value), &propertyBox, &checkAccessor);
                Bind(&propertyBox);
                {
                    value = GetValueFromPropertyBox(*value);
                    Jump(&setValue);
                }
                Bind(&checkAccessor);
                Label isAccessor(env);
                Branch(IsAccessor(attr), &isAccessor, &setValue);
                Bind(&isAccessor);
                {
                    value = CallGetterHelper(glue_, source, source, *value, ProfileOperation());
                    Label exception(env);
                    Branch(HasPendingException(glue_), &exception, &setValue);
                    Bind(&exception);
                    {
                        *result = Exception();
                        Jump(funcExit);
                    }
                }
                Bind(&setValue);
                {
                    FastSetPropertyByName(glue_, toAssign, key, *value);
                    Label exception(env);
                    Branch(HasPendingException(glue_), &exception, &loopEnd);
                    Bind(&exception);
                    {
                        *result = Exception();
                        Jump(funcExit);
                    }
                }
            }
        }
    }
    Bind(&loopEnd);
    idx = Int32Add(*idx, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(&exit);

    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsObjectStubBuilder::NameDictionaryAssignAllEnumProperty(Variable *result, Label *funcExit,
    GateRef toAssign, GateRef source, GateRef properties)
{
    // NameDictionary::VisitAllEnumProperty
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);

    GateRef sizeIndex = Int32(TaggedHashTable<NameDictionary>::SIZE_INDEX);
    GateRef size = GetInt32OfTInt(GetValueFromTaggedArray(properties, sizeIndex));
    DEFVARIABLE(idx, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*idx, size), &next, &loopExit);
        Bind(&next);
        GateRef key = GetKeyFromDictionary<NameDictionary>(properties, *idx);
        Label stringKey(env);
        Branch(TaggedIsString(key), &stringKey, &loopEnd);
        Bind(&stringKey);
        {
            GateRef attr = GetAttributesFromDictionary<NameDictionary>(properties, *idx);
            Label enumerable(env);
            Branch(IsEnumerable(attr), &enumerable, &loopEnd);
            Bind(&enumerable);
            {
                DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
                value = GetValueFromDictionary<NameDictionary>(properties, *idx);
                Label notHole(env);
                Branch(TaggedIsHole(*value), &loopEnd, &notHole);
                Bind(&notHole);
                {
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        value = CallGetterHelper(glue_, source, source, *value, ProfileOperation());
                        // exception
                        Label exception(env);
                        Branch(HasPendingException(glue_), &exception, &notAccessor);
                        Bind(&exception);
                        {
                            *result = Exception();
                            Jump(funcExit);
                        }
                    }
                    Bind(&notAccessor);
                    {
                        FastSetPropertyByName(glue_, toAssign, key, *value);
                        Label exception(env);
                        Branch(HasPendingException(glue_), &exception, &loopEnd);
                        Bind(&exception);
                        {
                            *result = Exception();
                            Jump(funcExit);
                        }
                    }
                }
            }
        }
    }
    Bind(&loopEnd);
    idx = Int32Add(*idx, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(&exit);

    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsObjectStubBuilder::AssignAllEnumProperty(Variable *res, Label *funcExit,
    GateRef toAssign, GateRef source)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);

    GateRef properties = GetPropertiesArray(source);
    Label dictionaryMode(env);
    Label notDictionaryMode(env);
    Branch(IsDictionaryMode(properties), &dictionaryMode, &notDictionaryMode);
    Bind(&notDictionaryMode);
    {
        LayoutInfoAssignAllEnumProperty(res, funcExit, toAssign, source);
        Jump(&exit);
    }
    Bind(&dictionaryMode);
    {
        NameDictionaryAssignAllEnumProperty(res, funcExit, toAssign, source, properties);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsObjectStubBuilder::SlowAssign(Variable *result, Label *funcExit, GateRef toAssign, GateRef source)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);
    CallRuntime(glue_, RTSTUB_ID(ObjectSlowAssign), { toAssign, source });

    Label exception(env);
    Branch(HasPendingException(glue_), &exception, &exit);
    Bind(&exception);
    {
        *result = Exception();
        Jump(funcExit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsObjectStubBuilder::FastAssign(Variable *res, Label *funcExit, GateRef toAssign, GateRef source)
{
    // visit elements
    AssignEnumElementProperty(res, funcExit, toAssign, source);
    AssignAllEnumProperty(res, funcExit, toAssign, source);
}

void BuiltinsObjectStubBuilder::Assign(Variable *res, Label *nextIt, Label *funcExit,
    GateRef toAssign, GateRef source)
{
    auto env = GetEnvironment();
    Label checkJsObj(env);
    Branch(BoolOr(TaggedIsNull(source), TaggedIsUndefined(source)), nextIt, &checkJsObj);
    Bind(&checkJsObj);
    {
        Label fastAssign(env);
        Label slowAssign(env);
        Branch(IsJSObjectType(source, JSType::JS_OBJECT), &fastAssign, &slowAssign);
        Bind(&fastAssign);
        {
            FastAssign(res, funcExit, toAssign, source);
            Jump(nextIt);
        }
        Bind(&slowAssign);
        {
            SlowAssign(res, funcExit, toAssign, source);
            Jump(nextIt);
        }
    }
}

void BuiltinsObjectStubBuilder::Assign(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);

    GateRef target = GetCallArg0(numArgs_);
    *result = target;
    Label jsObject(env);
    Branch(IsJSObjectType(target, JSType::JS_OBJECT), &jsObject, slowPath);
    Bind(&jsObject);
    {
        Label twoArg(env);
        Label notTwoArg(env);
        Branch(Int64Equal(numArgs_, IntPtr(2)), &twoArg, &notTwoArg); // 2 : two args
        Bind(&twoArg);
        {
            GateRef source = GetCallArg1(numArgs_);
            Label next(env);
            Assign(result, &next, exit, target, source);
            Bind(&next);
            Jump(exit);
        }
        Bind(&notTwoArg);
        Label threeArg(env);
        Label notThreeArg(env);
        Branch(Int64Equal(numArgs_, IntPtr(3)), &threeArg, &notThreeArg); // 3 : three args
        Bind(&threeArg);
        {
            Label nextArg(env);
            GateRef source = GetCallArg1(numArgs_);
            Label next(env);
            Assign(result, &next, exit, target, source);
            Bind(&next);
            Label next1(env);
            GateRef source1 = GetCallArg2(numArgs_);
            Assign(result, &next1, exit, target, source1);
            Bind(&next1);
            Jump(exit);
        }
        Bind(&notThreeArg);
        {
            Jump(slowPath);
        }
    }
}

void BuiltinsObjectStubBuilder::HasOwnProperty(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label keyIsString(env);
    Label valid(env);
    Label isHeapObject(env);
    GateRef prop = GetCallArg0(numArgs_);
    Branch(TaggedIsHeapObject(thisValue_), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(TaggedIsRegularObject(thisValue_), &valid, slowPath);
    Bind(&valid);
    {
        Label isIndex(env);
        Label notIndex(env);
        Branch(TaggedIsString(prop), &keyIsString, slowPath); // 2 : two args
        Bind(&keyIsString);
        {
            GateRef res = CallRuntime(glue_, RTSTUB_ID(TryToElementsIndexOrFindInStringTable), { prop });
            Branch(TaggedIsNumber(res), &isIndex, &notIndex);
            Bind(&isIndex);
            {
                GateRef index = NumberGetInt(glue_, res);
                Label findByIndex(env);
                GateRef elements = GetElementsArray(thisValue_);
                GateRef len = GetLengthOfTaggedArray(elements);
                Branch(Int32Equal(len, Int32(0)), exit, &findByIndex);
                Bind(&findByIndex);
                {
                    Label isDictionaryElement(env);
                    Label notDictionaryElement(env);
                    Branch(IsDictionaryMode(elements), &isDictionaryElement, &notDictionaryElement);
                    Bind(&notDictionaryElement);
                    {
                        Label lessThanLength(env);
                        Label notLessThanLength(env);
                        Branch(Int32UnsignedLessThanOrEqual(len, index), exit, &lessThanLength);
                        Bind(&lessThanLength);
                        {
                            Label notHole(env);
                            GateRef value = GetValueFromTaggedArray(elements, index);
                            Branch(TaggedIsNotHole(value), &notHole, exit);
                            Bind(&notHole);
                            {
                                *result = TaggedTrue();
                                Jump(exit);
                            }
                        }
                    }
                    Bind(&isDictionaryElement);
                    {
                        GateRef entryA = FindElementFromNumberDictionary(glue_, elements, index);
                        Label notNegtiveOne(env);
                        Branch(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, exit);
                        Bind(&notNegtiveOne);
                        {
                            *result = TaggedTrue();
                            Jump(exit);
                        }
                    }
                }
            }
            Bind(&notIndex);
            {
                Label findInStringTabel(env);
                Branch(TaggedIsHole(res), exit, &findInStringTabel);
                Bind(&findInStringTabel);
                {
                    Label isDicMode(env);
                    Label notDicMode(env);
                    GateRef hclass = LoadHClass(thisValue_);
                    Branch(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
                    Bind(&notDicMode);
                    {
                        GateRef layOutInfo = GetLayoutFromHClass(hclass);
                        GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
                        // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                        GateRef entryA = FindElementWithCache(glue_, layOutInfo, hclass, res, propsNum);
                        Label hasEntry(env);
                        // if branch condition : entry != -1
                        Branch(Int32NotEqual(entryA, Int32(-1)), &hasEntry, exit);
                        Bind(&hasEntry);
                        {
                            *result = TaggedTrue();
                            Jump(exit);
                        }
                    }
                    Bind(&isDicMode);
                    {
                        GateRef array = GetPropertiesArray(thisValue_);
                        // int entry = dict->FindEntry(key)
                        GateRef entryB = FindEntryFromNameDictionary(glue_, array, res);
                        Label notNegtiveOne(env);
                        // if branch condition : entry != -1
                        Branch(Int32NotEqual(entryB, Int32(-1)), &notNegtiveOne, exit);
                        Bind(&notNegtiveOne);
                        {
                            *result = TaggedTrue();
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

GateRef BuiltinsObjectStubBuilder::GetNumKeysFromLayoutInfo(GateRef object, GateRef end, GateRef layoutInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label iLessEnd(env);
    Label isString(env);
    Label initializedProp(env);
    Label isEnumerable(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32UnsignedLessThan(*i, end), &iLessEnd, &exit);
        Bind(&iLessEnd);
        {
            GateRef key = GetKey(layoutInfo, *i);
            Branch(TaggedIsString(key), &isString, &loopEnd);
            Bind(&isString);
            Branch(IsUninitializedProperty(object, *i, layoutInfo), &loopEnd, &initializedProp);
            Bind(&initializedProp);
            Branch(IsEnumerable(GetAttr(layoutInfo, *i)), &isEnumerable, &loopEnd);
            Bind(&isEnumerable);
            result = Int32Add(*result, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsObjectStubBuilder::IsUninitializedProperty(GateRef object, GateRef index, GateRef layoutInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(result, VariableType::BOOL(), False());

    Label inlinedProp(env);
    GateRef attr = GetAttr(layoutInfo, index);
    GateRef rep = GetRepInPropAttr(attr);
    GateRef hclass = LoadHClass(object);
    Branch(IsInlinedProperty(attr), &inlinedProp, &exit);
    Bind(&inlinedProp);
    {
        value = GetPropertyInlinedProps(object, hclass, index);
        result = TaggedIsHole(*value);
        Label nonDoubleToTagged(env);
        Label doubleToTagged(env);
        Branch(IsDoubleRepInPropAttr(rep), &doubleToTagged, &nonDoubleToTagged);
        Bind(&doubleToTagged);
        {
            value = TaggedPtrToTaggedDoublePtr(*value);
            result = TaggedIsHole(*value);
            Jump(&exit);
        }
        Bind(&nonDoubleToTagged);
        {
            Label intToTagged(env);
            Branch(IsIntRepInPropAttr(rep), &intToTagged, &exit);
            Bind(&intToTagged);
            {
                value = TaggedPtrToTaggedIntPtr(*value);
                result = TaggedIsHole(*value);
                Jump(&exit);
            }
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsObjectStubBuilder::GetNumKeysFromDictionary(GateRef array)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    GateRef sizeIndex = Int32(TaggedHashTable<NameDictionary>::SIZE_INDEX);
    GateRef size = GetInt32OfTInt(GetValueFromTaggedArray(array, sizeIndex));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label iLessSize(env);
    Label isString(env);
    Label initializedProp(env);
    Label isEnumerable(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32UnsignedLessThan(*i, size), &iLessSize, &afterLoop);
        Bind(&iLessSize);
        {
            GateRef key = GetKeyFromDictionary<NameDictionary>(array, *i);
            Branch(TaggedIsString(key), &isString, &loopEnd);
            Bind(&isString);
            GateRef attr = GetAttributesFromDictionary<NameDictionary>(array, *i);
            Branch(IsEnumerable(attr), &isEnumerable, &loopEnd);
            Bind(&isEnumerable);
            result = Int32Add(*result, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
    }
    Bind(&afterLoop);
    Jump(&exit);
    
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::LayoutInfoGetAllEnumKeys(GateRef end, GateRef offset,
                                                         GateRef array, GateRef object, GateRef layoutInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(enumKeys, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label iLessEnd(env);
    Label isEnumerable(env);
    Label initializedProp(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32UnsignedLessThan(*i, end), &iLessEnd, &afterLoop);
        Bind(&iLessEnd);
        {
            GateRef key = GetKey(layoutInfo, *i);
            Branch(BoolAnd(TaggedIsString(key), IsEnumerable(GetAttr(layoutInfo, *i))), &isEnumerable, &loopEnd);
            Bind(&isEnumerable);
            Branch(IsUninitializedProperty(object, *i, layoutInfo), &loopEnd, &initializedProp);
            Bind(&initializedProp);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue_, array, Int32Add(*enumKeys, offset), key);
            enumKeys = Int32Add(*enumKeys, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
    }
    Bind(&afterLoop);
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

GateRef BuiltinsObjectStubBuilder::CopyFromEnumCache(GateRef glue, GateRef elements)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    DEFVARIABLE(newLen, VariableType::INT32(), Int32(0));
    NewObjectStubBuilder newBuilder(this);

    Label lenIsZero(env);
    Label lenNotZero(env);
    Label afterLenCon(env);
    GateRef oldLen = GetLengthOfTaggedArray(elements);
    Branch(Int32Equal(oldLen, Int32(0)), &lenIsZero, &lenNotZero);
    {
        Bind(&lenIsZero);
        {
            newLen = Int32(0);
            Jump(&afterLenCon);
        }
        Bind(&lenNotZero);
        newLen = Int32Sub(oldLen, Int32(EnumCache::ENUM_CACHE_HEADER_SIZE));
        Jump(&afterLenCon);
    }
    Bind(&afterLenCon);
    GateRef array = newBuilder.NewTaggedArray(glue, *newLen);
    Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::LENGTH_OFFSET), *newLen);
    GateRef oldExtractLen = GetExtractLengthOfTaggedArray(elements);
    Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32UnsignedLessThan(*index, *newLen), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            GateRef value = GetValueFromTaggedArray(elements, Int32Add(*index,
                Int32(EnumCache::ENUM_CACHE_HEADER_SIZE)));
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
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsObjectStubBuilder::GetAllEnumKeys(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label isDictionary(env);
    Label notDictionary(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef array = GetPropertiesArray(obj);
    Branch(IsDictionaryMode(array), &isDictionary, &notDictionary);
    Bind(&isDictionary);
    {
        Label propsNotZero(env);
        Label propsIsZero(env);
        GateRef numOfKeys = GetNumKeysFromDictionary(array);
        Branch(Int32GreaterThan(numOfKeys, Int32(0)), &propsNotZero, &propsIsZero);
        Bind(&propsNotZero);
        result = CallRuntime(glue, RTSTUB_ID(NameDictionaryGetAllEnumKeys), { obj, IntToTaggedInt(numOfKeys) });
        Jump(&exit);
        Bind(&propsIsZero);
        GateRef emptyArray = GetEmptyArray(glue);
        result = emptyArray;
        Jump(&exit);
    }
    Bind(&notDictionary);
    {
        Label hasProps(env);
        Label notHasProps(env);
        GateRef hclass = LoadHClass(obj);
        // JSObject::GetNumberOfEnumKeys()
        GateRef num = GetNumberOfPropsFromHClass(hclass);
        Branch(Int32GreaterThan(num, Int32(0)), &hasProps, &notHasProps);
        Bind(&hasProps);
        {
            Label isOnlyOwnKeys(env);
            Label notOnlyOwnKeys(env);
            GateRef layout = GetLayoutFromHClass(hclass);
            GateRef numOfKeys = GetNumKeysFromLayoutInfo(obj, num, layout);
            // JSObject::GetAllEnumKeys
            GateRef enumCache = GetEnumCacheFromHClass(hclass);
            GateRef kind = GetEnumCacheKind(glue, enumCache);
            Branch(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::ONLY_OWN_KEYS))),
                &isOnlyOwnKeys, &notOnlyOwnKeys);
            Bind(&isOnlyOwnKeys);
            {
                result = CopyFromEnumCache(glue, enumCache);
                Jump(&exit);
            }
            Bind(&notOnlyOwnKeys);
            {
                Label numNotZero(env);
                Branch(Int32GreaterThan(numOfKeys, Int32(0)), &numNotZero, &notHasProps);
                Bind(&numNotZero);
                NewObjectStubBuilder newBuilder(this);
                GateRef keyArray = newBuilder.NewTaggedArray(glue,
                    Int32Add(numOfKeys, Int32(static_cast<int32_t>(EnumCache::ENUM_CACHE_HEADER_SIZE))));
                LayoutInfoGetAllEnumKeys(num, Int32(EnumCache::ENUM_CACHE_HEADER_SIZE), keyArray, obj, layout);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, keyArray,
                    Int32(EnumCache::ENUM_CACHE_KIND_OFFSET),
                    IntToTaggedInt(Int32(static_cast<int32_t>(EnumCacheKind::ONLY_OWN_KEYS))));
                SetEnumCacheToHClass(VariableType::JS_ANY(), glue, hclass, keyArray);
                result = CopyFromEnumCache(glue, keyArray);
                Jump(&exit);
            }
        }
        Bind(&notHasProps);
        {
            GateRef emptyArray = GetEmptyArray(glue);
            result = emptyArray;
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsObjectStubBuilder::GetEnumElementKeys(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(j, VariableType::INT32(), Int32(0));
    DEFVARIABLE(elementIndex, VariableType::INT32(), Int32(0));

    Label propsNotZero(env);
    Label propsIsZero(env);
    GateRef numOfElements = GetNumberOfElements(obj);
    Branch(Int32GreaterThan(numOfElements, Int32(0)), &propsNotZero, &propsIsZero);
    Bind(&propsNotZero);
    {
        Label isJSPrimitiveRef(env);
        Label isPrimitiveString(env);
        Label notPrimitiveString(env);
        Label isDictMode(env);
        Label notDictMode(env);

        NewObjectStubBuilder newBuilder(this);
        GateRef elementArray = newBuilder.NewTaggedArray(glue, numOfElements);
        Branch(IsJSPrimitiveRef(obj), &isJSPrimitiveRef, &notPrimitiveString);
        Bind(&isJSPrimitiveRef);
        GateRef value = Load(VariableType::JS_ANY(), obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
        Branch(TaggedIsString(value), &isPrimitiveString, &notPrimitiveString);
        Bind(&isPrimitiveString);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label iLessLength(env);
            GateRef strLen = GetLengthFromString(value);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Branch(Int32UnsignedLessThan(*i, strLen), &iLessLength, &notPrimitiveString);
                Bind(&iLessLength);
                {
                    GateRef str = IntToEcmaString(glue, *i);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elementArray,
                                          *elementIndex, str);
                    elementIndex = Int32Add(*elementIndex, Int32(1));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                i = Int32Add(*i, Int32(1));
                LoopEnd(&loopHead);
            }
        }
        Bind(&notPrimitiveString);
        GateRef elements = GetElementsArray(obj);
        Branch(IsDictionaryMode(elements), &isDictMode, &notDictMode);
        Bind(&notDictMode);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label iLessLength(env);
            Label notHole(env);
            Label afterLoop(env);
            GateRef elementsLen = GetLengthOfTaggedArray(elements);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Branch(Int32UnsignedLessThan(*j, elementsLen), &iLessLength, &afterLoop);
                Bind(&iLessLength);
                {
                    GateRef element = GetValueFromTaggedArray(elements, *j);
                    Branch(TaggedIsHole(element), &loopEnd, &notHole);
                    Bind(&notHole);
                    GateRef str = IntToEcmaString(glue, *j);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elementArray,
                                          *elementIndex, str);
                    elementIndex = Int32Add(*elementIndex, Int32(1));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                j = Int32Add(*j, Int32(1));
                LoopEnd(&loopHead);
                Bind(&afterLoop);
                {
                    Store(VariableType::INT32(), glue_,
                        elementArray, IntPtr(TaggedArray::LENGTH_OFFSET), *elementIndex);
                    result = elementArray;
                    Jump(&exit);
                }
            }
        }
        Bind(&isDictMode);
        {
            CallRuntime(glue, RTSTUB_ID(NumberDictionaryGetAllEnumKeys),
                { elements, elementArray, IntToTaggedInt(*elementIndex) });
            result = elementArray;
            Jump(&exit);
        }
    }
    Bind(&propsIsZero);
    {
        GateRef emptyArray = GetEmptyArray(glue);
        result = emptyArray;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::Keys(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef msg = GetCallArg0(numArgs_);
    // 1. Let obj be ToObject(O).
    GateRef obj = ToObject(glue_, msg);
    Label isPendingException(env);
    Label noPendingException(env);
    Branch(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    Jump(exit);
    Bind(&noPendingException);
    Label isFast(env);
    // EnumerableOwnNames(obj)
    GateRef isSpecialKey = BoolOr(IsTypedArray(obj), IsModuleNamespace(obj));
    GateRef notSlowObjectKey = BoolNot(BoolOr(isSpecialKey, IsJSGlobalObject(obj)));
    Branch(BoolAnd(IsJSObject(obj), notSlowObjectKey), &isFast, slowPath);
    Bind(&isFast);
    {
        Label hasKeyAndEle(env);
        Label nonKeyAndEle(env);
        GateRef elementArray = GetEnumElementKeys(glue_, obj);
        GateRef keyArray = GetAllEnumKeys(glue_, obj);
        GateRef lengthOfKeys = GetLengthOfTaggedArray(keyArray);
        GateRef lengthOfElements = GetLengthOfTaggedArray(elementArray);
        GateRef KeyAndEle = BoolAnd(Int32NotEqual(lengthOfElements, Int32(0)), Int32NotEqual(lengthOfKeys, Int32(0)));
        Branch(KeyAndEle, &hasKeyAndEle, &nonKeyAndEle);
        Bind(&hasKeyAndEle);
        {
            GateRef allKeys = AppendSkipHole(glue_, elementArray, keyArray, Int32Add(lengthOfKeys, lengthOfElements));
            *result = CreateArrayFromList(glue_, allKeys);
            Jump(exit);
        }
        Bind(&nonKeyAndEle);
        {
            Label hasKey(env);
            Label nonKey(env);
            Branch(Int32NotEqual(lengthOfKeys, Int32(0)), &hasKey, &nonKey);
            Bind(&hasKey);
            {
                *result = CreateArrayFromList(glue_, keyArray);
                Jump(exit);
            }
            Bind(&nonKey);
            {
                Label hasEle(env);
                Label nonEle(env);
                Branch(Int32NotEqual(lengthOfElements, Int32(0)), &hasEle, &nonEle);
                Bind(&hasEle);
                {
                    *result = CreateArrayFromList(glue_, elementArray);
                    Jump(exit);
                }
                Bind(&nonEle);
                {
                    GateRef emptyArray = GetEmptyArray(glue_);
                    *result = CreateArrayFromList(glue_, emptyArray);
                    Jump(exit);
                }
            }
        }
    }
}

}  // namespace panda::ecmascript::kungfu
