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

#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_primitive_ref.h"

namespace panda::ecmascript::kungfu {
void BuiltinsObjectStubBuilder::ToStringFunc(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label ecmaObj(env);
    // undefined
    Label undefined(env);
    Label checknull(env);
    BRANCH(TaggedIsUndefined(thisValue_), &undefined, &checknull);
    Bind(&undefined);
    {
        *result = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::UNDEFINED_TO_STRING_INDEX);
        Jump(exit);
    }
    // null
    Bind(&checknull);
    Label null(env);
    Label checkObject(env);
    BRANCH(TaggedIsUndefined(thisValue_), &null, &checkObject);
    Bind(&null);
    {
        *result = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::NULL_TO_STRING_INDEX);
        Jump(exit);
    }

    Bind(&checkObject);
    BRANCH(IsEcmaObject(thisValue_), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    {
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);
        GateRef toStringTagSymbol = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                      GlobalEnv::TOSTRINGTAG_SYMBOL_INDEX);
        GateRef tag = FastGetPropertyByName(glue_, thisValue_, toStringTagSymbol, ProfileOperation());

        Label defaultToString(env);
        BRANCH(TaggedIsString(tag), slowPath, &defaultToString);
        Bind(&defaultToString);
        {
            // default object
            Label objectTag(env);
            BRANCH(IsJSObjectType(thisValue_, JSType::JS_OBJECT), &objectTag, slowPath);
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

void BuiltinsObjectStubBuilder::Create(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label newObject(env);

    GateRef proto = GetCallArg0(numArgs_);
    GateRef protoCheck = LogicAndBuilder(env).And(BoolNot(IsEcmaObject(proto)))
        .And(BoolNot(TaggedIsNull(proto))).Done();
    BRANCH(LogicOrBuilder(env).Or(protoCheck).Or(TaggedIsSharedObj(proto)).Done(), slowPath, &newObject);
    Bind(&newObject);
    {
        Label noProperties(env);
        GateRef propertiesObject = GetCallArg1(numArgs_);
        BRANCH(TaggedIsUndefined(propertiesObject), &noProperties, slowPath);
        Bind(&noProperties);
        {
            // OrdinaryNewJSObjectCreate
            *result = OrdinaryNewJSObjectCreate(glue_, proto);
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
    BRANCH(IsDictionaryMode(elements), &dictionaryMode, &notDictionaryMode);
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
            BRANCH(Int32LessThan(*idx, len), &next, &loopExit);
            Bind(&next);
            GateRef value = GetTaggedValueWithElementsKind(glue_, source, *idx);
            Label notHole(env);
            BRANCH(TaggedIsHole(value), &loopEnd, &notHole);
            Bind(&notHole);
            {
                // key, value
                FastSetPropertyByIndex(glue_, toAssign, *idx, value);
                Label exception(env);
                BRANCH(HasPendingException(glue_), &exception, &loopEnd);
                Bind(&exception);
                {
                    *result = Exception();
                    Jump(funcExit);
                }
            }
        }
        Bind(&loopEnd);
        idx = Int32Add(*idx, Int32(1));
        LoopEnd(&loopHead, env, glue_);
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
            BRANCH(Int32LessThan(*idx, size), &next, &loopExit);
            Bind(&next);
            GateRef key = GetKeyFromDictionary<NumberDictionary>(elements, *idx);
            Label checkEnumerable(env);
            BRANCH(LogicOrBuilder(env).Or(TaggedIsUndefined(key)).Or(TaggedIsHole(key)).Done(),
                &loopEnd, &checkEnumerable);
            Bind(&checkEnumerable);
            {
                GateRef attr = GetAttributesFromDictionary<NumberDictionary>(elements, *idx);
                Label enumerable(env);
                BRANCH(IsEnumerable(attr), &enumerable, &loopEnd);
                Bind(&enumerable);
                {
                    GateRef value = GetValueFromDictionary<NumberDictionary>(elements, *idx);
                    Label notHole(env);
                    BRANCH(TaggedIsHole(value), &loopEnd, &notHole);
                    Bind(&notHole);
                    {
                        // value
                        FastSetPropertyByIndex(glue_, toAssign, *idx, value);
                        Label exception(env);
                        BRANCH(HasPendingException(glue_), &exception, &loopEnd);
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
        LoopEnd(&loopHead, env, glue_);
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
        BRANCH(Int32LessThan(*idx, num), &next, &loopExit);
        Bind(&next);

        GateRef key = GetKeyFromLayoutInfo(layout, *idx);
        GateRef attr = GetPropAttrFromLayoutInfo(layout, *idx);
        Label stringKey(env);
        BRANCH(TaggedIsStringOrSymbol(key), &stringKey, &loopEnd);
        Bind(&stringKey);
        {
            Label enumerable(env);
            BRANCH(IsEnumerable(attr), &enumerable, &loopEnd);
            Bind(&enumerable);
            {
                DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
                value = JSObjectGetProperty(source, cls, attr);
                // exception
                Label exception0(env);
                Label noexception0(env);
                BRANCH(HasPendingException(glue_), &exception0, &noexception0);
                Bind(&exception0);
                {
                    *result = Exception();
                    Jump(funcExit);
                }
                Bind(&noexception0);
                Label propertyBox(env);
                Label checkAccessor(env);
                Label setValue(env);
                BRANCH(TaggedIsPropertyBox(*value), &propertyBox, &checkAccessor);
                Bind(&propertyBox);
                {
                    value = GetValueFromPropertyBox(*value);
                    Jump(&setValue);
                }
                Bind(&checkAccessor);
                Label isAccessor(env);
                BRANCH(IsAccessor(attr), &isAccessor, &setValue);
                Bind(&isAccessor);
                {
                    value = CallGetterHelper(glue_, source, source, *value, ProfileOperation());
                    Label exception(env);
                    BRANCH(HasPendingException(glue_), &exception, &setValue);
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
                    BRANCH(HasPendingException(glue_), &exception, &loopEnd);
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
    LoopEnd(&loopHead, env, glue_);
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
        BRANCH(Int32LessThan(*idx, size), &next, &loopExit);
        Bind(&next);
        GateRef key = GetKeyFromDictionary<NameDictionary>(properties, *idx);
        Label stringKey(env);
        BRANCH(TaggedIsStringOrSymbol(key), &stringKey, &loopEnd);
        Bind(&stringKey);
        {
            GateRef attr = GetAttributesFromDictionary<NameDictionary>(properties, *idx);
            Label enumerable(env);
            BRANCH(IsEnumerable(attr), &enumerable, &loopEnd);
            Bind(&enumerable);
            {
                DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
                value = GetValueFromDictionary<NameDictionary>(properties, *idx);
                Label notHole(env);
                BRANCH(TaggedIsHole(*value), &loopEnd, &notHole);
                Bind(&notHole);
                {
                    Label isAccessor(env);
                    Label notAccessor(env);
                    BRANCH(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        value = CallGetterHelper(glue_, source, source, *value, ProfileOperation());
                        // exception
                        Label exception(env);
                        BRANCH(HasPendingException(glue_), &exception, &notAccessor);
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
                        BRANCH(HasPendingException(glue_), &exception, &loopEnd);
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
    LoopEnd(&loopHead, env, glue_);
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
    BRANCH(IsDictionaryMode(properties), &dictionaryMode, &notDictionaryMode);
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
    BRANCH(HasPendingException(glue_), &exception, &exit);
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
    BRANCH(TaggedIsUndefinedOrNull(source), nextIt, &checkJsObj);
    Bind(&checkJsObj);
    {
        Label fastAssign(env);
        Label slowAssign(env);
        BRANCH(IsJSObjectType(source, JSType::JS_OBJECT), &fastAssign, &slowAssign);
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
    BRANCH(IsJSObjectType(target, JSType::JS_OBJECT), &jsObject, slowPath);
    Bind(&jsObject);
    {
        Label twoArg(env);
        Label notTwoArg(env);
        BRANCH(Int64Equal(numArgs_, IntPtr(2)), &twoArg, &notTwoArg); // 2 : two args
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
        BRANCH(Int64Equal(numArgs_, IntPtr(3)), &threeArg, &notThreeArg); // 3 : three args
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
    GateRef prop = GetCallArg0(numArgs_);
    HasOwnProperty(result, exit, slowPath, thisValue_, prop);
}

void BuiltinsObjectStubBuilder::HasOwnProperty(Variable *result, Label *exit, Label *slowPath, GateRef thisValue,
                                               GateRef prop, GateRef hir)
{
    auto env = GetEnvironment();
    Label keyIsString(env);
    Label valid(env);
    Label isHeapObject(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(TaggedIsRegularObject(thisValue), &valid, slowPath);
    Bind(&valid);
    {
        Label isIndex(env);
        Label notIndex(env);
        BRANCH(TaggedIsString(prop), &keyIsString, slowPath); // 2 : two args
        Bind(&keyIsString);
        {
            GateRef res = StringToElementIndex(glue_, prop);
            // -1: not find element index
            BRANCH(Int64NotEqual(res, Int64(-1)), &isIndex, &notIndex);
            Bind(&isIndex);
            {
                GateRef index = TruncInt64ToInt32(res);
                Label findByIndex(env);
                GateRef elements = GetElementsArray(thisValue);
                GateRef len = GetLengthOfTaggedArray(elements);
                BRANCH(Int32Equal(len, Int32(0)), exit, &findByIndex);
                Bind(&findByIndex);
                {
                    Label isDictionaryElement(env);
                    Label notDictionaryElement(env);
                    BRANCH(IsDictionaryMode(elements), &isDictionaryElement, &notDictionaryElement);
                    Bind(&notDictionaryElement);
                    {
                        Label lessThanLength(env);
                        Label notLessThanLength(env);
                        BRANCH(Int32UnsignedLessThanOrEqual(len, index), exit, &lessThanLength);
                        Bind(&lessThanLength);
                        {
                            Label notHole(env);
                            GateRef value = GetTaggedValueWithElementsKind(glue_, thisValue, index);
                            BRANCH(TaggedIsNotHole(value), &notHole, exit);
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
                        BRANCH(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, exit);
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
                Label notInternString(env);
                DEFVARIABLE(stringTable, VariableType::JS_ANY(), prop);
                BRANCH(IsInternalString(prop), &findInStringTabel, &notInternString);
                Bind(&notInternString);
                {
                    GateRef internString = CallRuntime(glue_, RTSTUB_ID(TryGetInternString), { prop });
                    stringTable = internString;
                    BRANCH(TaggedIsHole(internString), exit, &findInStringTabel)
                }
                Bind(&findInStringTabel);
                {
                    Label isDicMode(env);
                    Label notDicMode(env);
                    GateRef hclass = LoadHClass(thisValue);
                    BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
                    Bind(&notDicMode);
                    {
                        GateRef layOutInfo = GetLayoutFromHClass(hclass);
                        GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
                        // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                        GateRef entryA = FindElementWithCache(glue_, layOutInfo, hclass, *stringTable, propsNum, hir);
                        Label hasEntry(env);
                        // if branch condition : entry != -1
                        BRANCH(Int32NotEqual(entryA, Int32(-1)), &hasEntry, exit);
                        Bind(&hasEntry);
                        {
                            *result = TaggedTrue();
                            Jump(exit);
                        }
                    }
                    Bind(&isDicMode);
                    {
                        GateRef array = GetPropertiesArray(thisValue);
                        // int entry = dict->FindEntry(key)
                        GateRef entryB = FindEntryFromNameDictionary(glue_, array, *stringTable, hir);
                        Label notNegtiveOne(env);
                        // if branch condition : entry != -1
                        BRANCH(Int32NotEqual(entryB, Int32(-1)), &notNegtiveOne, exit);
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

GateRef BuiltinsObjectStubBuilder::GetNumKeysFromLayoutInfo(GateRef end, GateRef layoutInfo)
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
    Label isEnumerable(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*i, end), &iLessEnd, &exit);
        Bind(&iLessEnd);
        {
            GateRef key = GetKey(layoutInfo, *i);
            BRANCH(TaggedIsString(key), &isString, &loopEnd);
            Bind(&isString);
            BRANCH(IsEnumerable(GetAttr(layoutInfo, *i)), &isEnumerable, &loopEnd);
            Bind(&isEnumerable);
            result = Int32Add(*result, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead, env, glue_);
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
    Label isEnumerable(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*i, size), &iLessSize, &afterLoop);
        Bind(&iLessSize);
        {
            GateRef key = GetKeyFromDictionary<NameDictionary>(array, *i);
            BRANCH(TaggedIsString(key), &isString, &loopEnd);
            Bind(&isString);
            GateRef attr = GetAttributesFromDictionary<NameDictionary>(array, *i);
            BRANCH(IsEnumerable(attr), &isEnumerable, &loopEnd);
            Bind(&isEnumerable);
            result = Int32Add(*result, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead, env, glue_);
    }
    Bind(&afterLoop);
    Jump(&exit);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::LayoutInfoGetAllEnumKeys(GateRef end, GateRef offset, GateRef array, GateRef layoutInfo)
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
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*i, end), &iLessEnd, &afterLoop);
        Bind(&iLessEnd);
        {
            GateRef key = GetKey(layoutInfo, *i);
            GateRef iVal = *i;
            BRANCH(LogicAndBuilder(env).And(TaggedIsString(key)).And(IsEnumerable(GetAttr(layoutInfo, iVal))).Done(),
                &isEnumerable, &loopEnd);
            Bind(&isEnumerable);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue_, array, Int32Add(*enumKeys, offset), key);
            enumKeys = Int32Add(*enumKeys, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead, env, glue_);
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
    BRANCH(Int32Equal(oldLen, Int32(0)), &lenIsZero, &lenNotZero);
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
    GateRef oldExtractLen = GetExtraLengthOfTaggedArray(elements);
    Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET), oldExtractLen);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*index, *newLen), &storeValue, &afterLoop);
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
    LoopEnd(&loopHead, env, glue);
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
    BRANCH(IsDictionaryMode(array), &isDictionary, &notDictionary);
    Bind(&isDictionary);
    {
        Label propsNotZero(env);
        Label propsIsZero(env);
        GateRef numOfKeys = GetNumKeysFromDictionary(array);
        BRANCH(Int32GreaterThan(numOfKeys, Int32(0)), &propsNotZero, &propsIsZero);
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
        BRANCH(Int32GreaterThan(num, Int32(0)), &hasProps, &notHasProps);
        Bind(&hasProps);
        {
            Label isOnlyOwnKeys(env);
            Label notOnlyOwnKeys(env);
            GateRef layout = GetLayoutFromHClass(hclass);
            GateRef numOfKeys = GetNumKeysFromLayoutInfo(num, layout);
            // JSObject::GetAllEnumKeys
            GateRef enumCache = GetEnumCacheFromHClass(hclass);
            GateRef kind = GetEnumCacheKind(glue, enumCache);
            BRANCH(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::ONLY_OWN_KEYS))),
                &isOnlyOwnKeys, &notOnlyOwnKeys);
            Bind(&isOnlyOwnKeys);
            {
                result = CopyFromEnumCache(glue, enumCache);
                Jump(&exit);
            }
            Bind(&notOnlyOwnKeys);
            {
                Label numNotZero(env);
                Label inSharedHeap(env);
                Label notInSharedHeap(env);
                BRANCH(Int32GreaterThan(numOfKeys, Int32(0)), &numNotZero, &notHasProps);
                Bind(&numNotZero);
                NewObjectStubBuilder newBuilder(this);
                GateRef keyArray = newBuilder.NewTaggedArray(glue,
                    Int32Add(numOfKeys, Int32(static_cast<int32_t>(EnumCache::ENUM_CACHE_HEADER_SIZE))));
                LayoutInfoGetAllEnumKeys(num, Int32(EnumCache::ENUM_CACHE_HEADER_SIZE), keyArray, layout);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, keyArray,
                    Int32(EnumCache::ENUM_CACHE_KIND_OFFSET),
                    IntToTaggedInt(Int32(static_cast<int32_t>(EnumCacheKind::ONLY_OWN_KEYS))));
                GateRef hclassRegion = ObjectAddressToRange(hclass);
                BRANCH(InSharedHeap(hclassRegion), &inSharedHeap, &notInSharedHeap);
                Bind(&inSharedHeap);
                {
                    result = CopyFromEnumCache(glue, keyArray);
                    Jump(&exit);
                }
                Bind(&notInSharedHeap);
                {
                    SetEnumCacheToHClass(VariableType::JS_ANY(), glue, hclass, keyArray);
                    result = CopyFromEnumCache(glue, keyArray);
                    Jump(&exit);
                }
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
    GateRef numOfElements = GetNumberOfElements(glue, obj);
    BRANCH(Int32GreaterThan(numOfElements, Int32(0)), &propsNotZero, &propsIsZero);
    Bind(&propsNotZero);
    {
        Label isJSPrimitiveRef(env);
        Label isPrimitiveString(env);
        Label notPrimitiveString(env);
        Label isDictMode(env);
        Label notDictMode(env);

        NewObjectStubBuilder newBuilder(this);
        GateRef elementArray = newBuilder.NewTaggedArray(glue, numOfElements);
        BRANCH(IsJSPrimitiveRef(obj), &isJSPrimitiveRef, &notPrimitiveString);
        Bind(&isJSPrimitiveRef);
        GateRef value = Load(VariableType::JS_ANY(), obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
        BRANCH(TaggedIsString(value), &isPrimitiveString, &notPrimitiveString);
        Bind(&isPrimitiveString);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label iLessLength(env);
            GateRef strLen = GetLengthFromString(value);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32UnsignedLessThan(*i, strLen), &iLessLength, &notPrimitiveString);
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
                LoopEnd(&loopHead, env, glue);
            }
        }
        Bind(&notPrimitiveString);
        GateRef elements = GetElementsArray(obj);
        BRANCH(IsDictionaryMode(elements), &isDictMode, &notDictMode);
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
                BRANCH(Int32UnsignedLessThan(*j, elementsLen), &iLessLength, &afterLoop);
                Bind(&iLessLength);
                {
                    GateRef element = GetTaggedValueWithElementsKind(glue, obj, *j);
                    BRANCH(TaggedIsHole(element), &loopEnd, &notHole);
                    Bind(&notHole);
                    GateRef str = IntToEcmaString(glue, *j);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elementArray,
                                          *elementIndex, str);
                    elementIndex = Int32Add(*elementIndex, Int32(1));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                j = Int32Add(*j, Int32(1));
                LoopEnd(&loopHead, env, glue);
                Bind(&afterLoop);
                {
                    result = elementArray;
                    Label needTrim(env);
                    BRANCH(Int32LessThan(*elementIndex, numOfElements), &needTrim, &exit);
                    Bind(&needTrim);
                    {
                        CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim),
                                       {glue, elementArray, ZExtInt32ToInt64(*elementIndex)});
                        Jump(&exit);
                    }
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

GateRef BuiltinsObjectStubBuilder::IsNotSlowObjectKey(GateRef obj)
{
    auto env = GetEnvironment();
    return LogicAndBuilder(env)
        .And(IsJSObject(obj))
        .And(BoolNot(LogicOrBuilder(env).Or(IsTypedArray(obj)).Or(IsModuleNamespace(obj))
                                        .Or(IsJSGlobalObject(obj)).Done()))
        .Done();
}

void BuiltinsObjectStubBuilder::Keys(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    NewObjectStubBuilder newBuilder(this);
    GateRef msg = GetCallArg0(numArgs_);
    // 1. Let obj be ToObject(O).
    GateRef obj = ToObject(glue_, msg);
    Label isPendingException(env);
    Label noPendingException(env);
    BRANCH(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    Jump(exit);
    Bind(&noPendingException);
    Label isFast(env);
    // EnumerableOwnNames(obj)
    BRANCH(IsNotSlowObjectKey(obj), &isFast, slowPath);
    Bind(&isFast);
    {
        Label hasKeyAndEle(env);
        Label nonKeyAndEle(env);
        // need to caclulate elementKind
        GateRef elementKind = Int32(Elements::ToUint(ElementsKind::TAGGED));
        GateRef elementArray = GetEnumElementKeys(glue_, obj);
        GateRef keyArray = GetAllEnumKeys(glue_, obj);
        GateRef lengthOfKeys = GetLengthOfTaggedArray(keyArray);
        GateRef lengthOfElements = GetLengthOfTaggedArray(elementArray);
        GateRef KeyAndEle = BitAnd(Int32NotEqual(lengthOfElements, Int32(0)), Int32NotEqual(lengthOfKeys, Int32(0)));
        BRANCH(KeyAndEle, &hasKeyAndEle, &nonKeyAndEle);
        Bind(&hasKeyAndEle);
        {
            GateRef allKeys = AppendSkipHole(glue_, elementArray, keyArray, Int32Add(lengthOfKeys, lengthOfElements));
            *result = newBuilder.CreateArrayFromList(glue_, allKeys, elementKind);
            Jump(exit);
        }
        Bind(&nonKeyAndEle);
        {
            Label hasKey(env);
            Label nonKey(env);
            BRANCH(Int32NotEqual(lengthOfKeys, Int32(0)), &hasKey, &nonKey);
            Bind(&hasKey);
            {
                *result = newBuilder.CreateArrayFromList(glue_, keyArray, elementKind);
                Jump(exit);
            }
            Bind(&nonKey);
            {
                Label hasEle(env);
                Label nonEle(env);
                BRANCH(Int32NotEqual(lengthOfElements, Int32(0)), &hasEle, &nonEle);
                Bind(&hasEle);
                {
                    *result = newBuilder.CreateArrayFromList(glue_, elementArray, elementKind);
                    Jump(exit);
                }
                Bind(&nonEle);
                {
                    GateRef emptyArray = GetEmptyArray(glue_);
                    *result = newBuilder.CreateArrayFromList(glue_, emptyArray, elementKind);
                    Jump(exit);
                }
            }
        }
    }
}

void BuiltinsObjectStubBuilder::GetPrototypeOf(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isPendingException(env);
    Label noPendingException(env);
    GateRef msg = GetCallArg0(numArgs_);
    GateRef obj = ToObject(glue_, msg);
    BRANCH(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        Jump(exit);
    }
    Bind(&noPendingException);
    {
        Label isEcmaObject(env);
        Label notJsProxy(env);
        BRANCH(IsEcmaObject(obj), &isEcmaObject, slowPath);
        Bind(&isEcmaObject);
        {
            BRANCH(IsJsProxy(obj), slowPath, &notJsProxy);
            Bind(&notJsProxy);
            {
                GateRef hClass = LoadHClass(obj);
                GateRef prototype = GetPrototypeFromHClass(hClass);
                *result = prototype;
                Jump(exit);
            }
        }
    }
}

void BuiltinsObjectStubBuilder::SetPrototypeOf(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs_);
    DEFVARIABLE(proto, VariableType::JS_ANY(), Undefined());
    Label checkJsObj(env);
    Label setProto(env);
    BRANCH(TaggedIsUndefinedOrNull(obj), slowPath, &checkJsObj);
    Bind(&checkJsObj);
    {
        Label checkProto(env);
        proto = GetCallArg1(numArgs_);
        GateRef protoVal = *proto;
        BRANCH(LogicOrBuilder(env).Or(TaggedIsNull(protoVal)).Or(IsEcmaObject(protoVal)).Done(),
            &checkProto, slowPath);
        Bind(&checkProto);
        {
            Label isEcmaObject(env);
            Label notEcmaObject(env);
            BRANCH(IsEcmaObject(obj), &isEcmaObject, &notEcmaObject);
            Bind(&isEcmaObject);
            Jump(&setProto);
            Bind(&notEcmaObject);
            {
                *result = obj;
                Jump(exit);
            }
        }
    }
    Bind(&setProto);
    {
        Label objNotSpecial(env);
        GateRef protoVal = *proto;
        GateRef isSpecialobj = LogicOrBuilder(env).Or(IsJsProxy(obj)).Or(TaggedIsSharedObj(obj))
            .Or(TaggedIsSharedObj(protoVal)).Or(IsSpecialContainer(obj)).Or(IsModuleNamespace(obj)).Done();
        BRANCH(isSpecialobj, slowPath, &objNotSpecial);
        Bind(&objNotSpecial);
        Label isFunction(env);
        Label notFunction(env);
        GateRef isFunc = LogicAndBuilder(env)
            .And(TaggedIsHeapObject(obj)).And(TaggedIsHeapObject(protoVal))
            .And(IsJSFunction(obj)).And(IsJSFunction(protoVal))
            .Done();
        BRANCH(isFunc, &isFunction, &notFunction);
        Bind(&isFunction);
        {
            Label isDerivedCtor(env);
            auto protoOrHclass = Load(VariableType::JS_ANY(), obj,
                                      IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            GateRef isDerivedCtorCheck = LogicAndBuilder(env).And(TaggedIsHeapObject(protoOrHclass))
                .And(IsJSHClass(protoOrHclass)).And(IsDerived(obj)).Done();
            BRANCH(isDerivedCtorCheck, &isDerivedCtor, &notFunction);
            Bind(&isDerivedCtor);
            auto cachedJSHClass = GetPrototypeFromHClass(protoOrHclass);
            SetProtoOrHClassToFunction(glue_, obj, cachedJSHClass);
            Jump(&notFunction);
        }
        Bind(&notFunction);
        {
            Label statusIsTrue(env);
            Label statusIsFalse(env);
            BRANCH(ObjectSetPrototype(glue_, obj, *proto), &statusIsTrue, &statusIsFalse);
            Bind(&statusIsTrue);
            *result = obj;
            Jump(exit);
            Bind(&statusIsFalse);
            {
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPrototypeOfFailed));
                CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                *result = Exception();
                Jump(exit);
            }
        }
    }
}

GateRef BuiltinsObjectStubBuilder::ObjectSetPrototype(GateRef glue, GateRef obj, GateRef proto)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isEqual(env);
    Label notEqual(env);
    Label isExtensible(env);
    Label notExtensible(env);
    GateRef current = GetPrototype(glue, obj);
    BRANCH(IntPtrEqual(proto, current), &isEqual, &notEqual);
    Bind(&isEqual);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&notEqual);
    {
        BRANCH(IsExtensible(obj), &isExtensible, &notExtensible);
        Bind(&isExtensible);
        {
            DEFVARIABLE(done, VariableType::BOOL(), False());
            DEFVARIABLE(tempProto, VariableType::JS_ANY(), proto);
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(BoolNot(*done), &next, &loopExit);
                Bind(&next);
                {
                    Label isNull(env);
                    Label notNull(env);
                    Label isEqual2(env);
                    Label notEqual2(env);
                    Label protoNotProxy(env);
                    GateRef tempProtoVal = *tempProto;
                    GateRef protoIsNull = LogicOrBuilder(env).Or(TaggedIsNull(tempProtoVal))
                        .Or(BoolNot(IsEcmaObject(tempProtoVal))).Done();
                    BRANCH(protoIsNull, &isNull, &notNull);
                    Bind(&isNull);
                    {
                        done = True();
                        Jump(&loopEnd);
                    }
                    Bind(&notNull);
                    {
                        BRANCH(IntPtrEqual(*tempProto, obj), &isEqual2, &notEqual2);
                        Bind(&isEqual2);
                        {
                            result = False();
                            Jump(&exit);
                        }
                        Bind(&notEqual2);
                        {
                            BRANCH(IsJsProxy(*tempProto), &loopExit, &protoNotProxy);
                            Bind(&protoNotProxy);
                            {
                                tempProto = GetPrototype(glue, *tempProto);
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead, env, glue_);
            }
            Bind(&loopExit);
            CallRuntime(glue, RTSTUB_ID(SetPrototypeTransition), { obj, proto});
            result = True();
            Jump(&exit);
        }
        Bind(&notExtensible);
        {
            result = False();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::GetOwnPropertyNames(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isPendingException(env);
    Label noPendingException(env);
    GateRef msg = GetCallArg0(numArgs_);
    GateRef obj = ToObject(glue_, msg);
    BRANCH(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        Jump(exit);
    }
    Bind(&noPendingException);
    {
        Label isFast(env);
        BRANCH(IsNotSlowObjectKey(obj), &isFast, slowPath);
        Bind(&isFast);
        {
            Label notDictMode(env);
            GateRef isDictMode = LogicOrBuilder(env).Or(IsDictionaryMode(GetElementsArray(obj)))
                .Or(IsDictionaryMode(GetPropertiesArray(obj))).Done();
            BRANCH(isDictMode, slowPath, &notDictMode);
            Bind(&notDictMode);
            {
                Label getAllElementKeys(env);
                Label checkNumOfKeys(env);
                GateRef hclass = LoadHClass(obj);
                GateRef numOfElements = GetNumberOfElements(glue_, obj);
                GateRef numOfKeys = GetNumberOfPropsFromHClass(hclass);
                GateRef keyLen = Int32Add(numOfElements, numOfKeys);
                GateRef elementKind = Int32(Elements::ToUint(ElementsKind::TAGGED));
                NewObjectStubBuilder newBuilder(this);
                GateRef keyArray = newBuilder.NewTaggedArray(glue_, keyLen);
                BRANCH(Int32GreaterThan(numOfElements, Int32(0)), &getAllElementKeys, &checkNumOfKeys);
                Bind(&getAllElementKeys);
                {
                    GetAllElementKeys(glue_, obj, Int32(0), keyArray);
                    Jump(&checkNumOfKeys);
                }
                Bind(&checkNumOfKeys);
                {
                    Label getAllPropertyKeys(env);
                    Label checkElementType(env);
                    BRANCH(Int32GreaterThan(numOfKeys, Int32(0)), &getAllPropertyKeys, &checkElementType);
                    Bind(&getAllPropertyKeys);
                    {
                        GetAllPropertyKeys(glue_, obj, numOfElements, keyArray);
                        Jump(&checkElementType);
                    }
                    Bind(&checkElementType);
                    {
                        DEFVARIABLE(i, VariableType::INT32(), Int32(0));
                        DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
                        Label loopHead(env);
                        Label loopEnd(env);
                        Label next(env);
                        Label loopExit(env);
                        Label needTrim(env);
                        Label setResult(env);
                        Jump(&loopHead);
                        LoopBegin(&loopHead);
                        {
                            BRANCH(Int32UnsignedLessThan(*i, keyLen), &next, &loopExit);
                            Bind(&next);
                            {
                                Label isString(env);
                                Label setValue(env);
                                Label adjustPos(env);
                                GateRef element = GetValueFromTaggedArray(keyArray, *i);
                                BRANCH(TaggedIsString(element), &isString, &loopEnd);
                                Bind(&isString);
                                {
                                    // compare pos and i to skip holes
                                    BRANCH(Int32Equal(*pos, *i), &adjustPos, &setValue);
                                    Bind(&setValue);
                                    {
                                        SetValueToTaggedArray(VariableType::JS_ANY(), glue_, keyArray, *pos, element);
                                        Jump(&adjustPos);
                                    }
                                    Bind(&adjustPos);
                                    {
                                        pos = Int32Add(*pos, Int32(1));
                                        Jump(&loopEnd);
                                    }
                                }
                            }
                            Bind(&loopEnd);
                            i = Int32Add(*i, Int32(1));
                            LoopEnd(&loopHead, env, glue_);
                        }
                        Bind(&loopExit);
                        BRANCH(Int32UnsignedLessThan(*pos, keyLen), &needTrim, &setResult);
                        Bind(&needTrim);
                        {
                            CallNGCRuntime(glue_, RTSTUB_ID(ArrayTrim), {glue_, keyArray, ZExtInt32ToInt64(*pos)});
                            Jump(&setResult);
                        }
                        Bind(&setResult);
                        {
                            *result = newBuilder.CreateArrayFromList(glue_, keyArray, elementKind);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsObjectStubBuilder::GetOwnPropertySymbols(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isPendingException(env);
    Label noPendingException(env);
    GateRef msg = GetCallArg0(numArgs_);
    GateRef obj = ToObject(glue_, msg);
    BRANCH(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        Jump(exit);
    }
    Bind(&noPendingException);
    {
        Label isFast(env);
        BRANCH(IsNotSlowObjectKey(obj), &isFast, slowPath);
        Bind(&isFast);
        {
            Label notDictMode(env);
            GateRef isDictMode = LogicOrBuilder(env).Or(IsDictionaryMode(GetElementsArray(obj)))
                .Or(IsDictionaryMode(GetPropertiesArray(obj))).Done();
            BRANCH(isDictMode, slowPath, &notDictMode);
            Bind(&notDictMode);
            {
                Label getAllElementKeys(env);
                Label checkNumOfKeys(env);
                GateRef hclass = LoadHClass(obj);
                GateRef numOfElements = GetNumberOfElements(glue_, obj);
                GateRef numOfKeys = GetNumberOfPropsFromHClass(hclass);
                GateRef keyLen = Int32Add(numOfElements, numOfKeys);
                // need to caclulate elementKind
                GateRef elementKind = Int32(Elements::ToUint(ElementsKind::TAGGED));
                NewObjectStubBuilder newBuilder(this);
                GateRef keyArray = newBuilder.NewTaggedArray(glue_, keyLen);
                BRANCH(Int32GreaterThan(numOfElements, Int32(0)), &getAllElementKeys, &checkNumOfKeys);
                Bind(&getAllElementKeys);
                {
                    GetAllElementKeys(glue_, obj, Int32(0), keyArray);
                    Jump(&checkNumOfKeys);
                }
                Bind(&checkNumOfKeys);
                {
                    Label getAllPropertyKeys(env);
                    Label checkElementType(env);
                    BRANCH(Int32GreaterThan(numOfKeys, Int32(0)), &getAllPropertyKeys, &checkElementType);
                    Bind(&getAllPropertyKeys);
                    {
                        GetAllPropertyKeys(glue_, obj, numOfElements, keyArray);
                        Jump(&checkElementType);
                    }
                    Bind(&checkElementType);
                    {
                        DEFVARIABLE(i, VariableType::INT32(), Int32(0));
                        DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
                        Label loopHead(env);
                        Label loopEnd(env);
                        Label next(env);
                        Label loopExit(env);
                        Label needTrim(env);
                        Label setResult(env);
                        Jump(&loopHead);
                        LoopBegin(&loopHead);
                        {
                            BRANCH(Int32UnsignedLessThan(*i, keyLen), &next, &loopExit);
                            Bind(&next);
                            {
                                Label isSymbol(env);
                                Label setValue(env);
                                Label adjustPos(env);
                                GateRef element = GetValueFromTaggedArray(keyArray, *i);
                                BRANCH(TaggedIsSymbol(element), &isSymbol, &loopEnd);
                                Bind(&isSymbol);
                                {
                                    // compare pos and i to skip holes
                                    BRANCH(Int32Equal(*pos, *i), &adjustPos, &setValue);
                                    Bind(&setValue);
                                    {
                                        SetValueToTaggedArray(VariableType::JS_ANY(), glue_, keyArray, *pos, element);
                                        Jump(&adjustPos);
                                    }
                                    Bind(&adjustPos);
                                    {
                                        pos = Int32Add(*pos, Int32(1));
                                        Jump(&loopEnd);
                                    }
                                }
                            }
                            Bind(&loopEnd);
                            i = Int32Add(*i, Int32(1));
                            LoopEnd(&loopHead, env, glue_);
                        }
                        Bind(&loopExit);
                        BRANCH(Int32UnsignedLessThan(*pos, keyLen), &needTrim, &setResult);
                        Bind(&needTrim);
                        {
                            CallNGCRuntime(glue_, RTSTUB_ID(ArrayTrim), {glue_, keyArray, ZExtInt32ToInt64(*pos)});
                            Jump(&setResult);
                        }
                        Bind(&setResult);
                        {
                            *result = newBuilder.CreateArrayFromList(glue_, keyArray, elementKind);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

GateRef BuiltinsObjectStubBuilder::GetAllElementKeys(GateRef glue, GateRef obj, GateRef offset, GateRef array)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    DEFVARIABLE(i, VariableType::INT32(), offset);
    DEFVARIABLE(j, VariableType::INT32(), Int32(0));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label isJSPrimitiveRef(env);
    Label isPrimitiveString(env);
    Label notPrimitiveString(env);
    BRANCH(IsJSPrimitiveRef(obj), &isJSPrimitiveRef, &notPrimitiveString);
    Bind(&isJSPrimitiveRef);
    {
        GateRef value = Load(VariableType::JS_ANY(), obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
        BRANCH(TaggedIsString(value), &isPrimitiveString, &notPrimitiveString);
        Bind(&isPrimitiveString);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            GateRef elementIndex = Int32Add(GetLengthFromString(value), offset);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32UnsignedLessThan(*i, elementIndex), &next, &notPrimitiveString);
                Bind(&next);
                {
                    GateRef str = IntToEcmaString(glue, *i);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *pos, str);
                    pos = Int32Add(*pos, Int32(1));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                i = Int32Add(*i, Int32(1));
                LoopEnd(&loopHead, env, glue);
            }
        }
    }
    Bind(&notPrimitiveString);
    {
        Label isDictMode(env);
        Label notDictMode(env);
        Label exit(env);
        GateRef elements = GetElementsArray(obj);
        BRANCH(IsDictionaryMode(elements), &isDictMode, &notDictMode);
        Bind(&isDictMode);
        {
            FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
            Jump(&exit);
        }
        Bind(&notDictMode);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            GateRef elementsLen = GetLengthOfTaggedArray(elements);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Label notHole(env);
                BRANCH(Int32UnsignedLessThan(*j, elementsLen), &next, &loopExit);
                Bind(&next);
                {
                    GateRef element = GetTaggedValueWithElementsKind(glue, obj, *j);
                    BRANCH(TaggedIsHole(element), &loopEnd, &notHole);
                    Bind(&notHole);
                    {
                        GateRef str = IntToEcmaString(glue, *j);
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *pos, str);
                        pos = Int32Add(*pos, Int32(1));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                j = Int32Add(*j, Int32(1));
                LoopEnd(&loopHead, env, glue);
                Bind(&loopExit);
                result = array;
                Jump(&exit);
            }
        }
        Bind(&exit);
        auto ret = *result;
        env->SubCfgExit();
        return ret;
    }
}

GateRef BuiltinsObjectStubBuilder::GetAllPropertyKeys(GateRef glue, GateRef obj, GateRef offset, GateRef array)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label isDictionary(env);
    Label notDictionary(env);
    Label exit(env);
    GateRef properties = GetPropertiesArray(obj);
    BRANCH(IsDictionaryMode(properties), &isDictionary, &notDictionary);
    Bind(&isDictionary);
    {
        FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
        Jump(&exit);
    }
    Bind(&notDictionary);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        GateRef hclass = LoadHClass(obj);
        GateRef layout = GetLayoutFromHClass(hclass);
        GateRef number = GetNumberOfPropsFromHClass(hclass);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH(Int32UnsignedLessThan(*i, number), &next, &loopExit);
            Bind(&next);
            {
                Label checkSymbol(env);
                Label setValue(env);
                GateRef key = GetKey(layout, *i);
                BRANCH(TaggedIsString(key), &setValue, &checkSymbol);
                Bind(&checkSymbol);
                {
                    BRANCH(TaggedIsSymbol(key), &setValue, &loopEnd);
                }
                Bind(&setValue);
                {
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue_, array, Int32Add(*pos, offset), key);
                    pos = Int32Add(*pos, Int32(1));
                    Jump(&loopEnd);
                }
            }
            Bind(&loopEnd);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&loopHead, env, glue_);
        }
        Bind(&loopExit);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::Entries(Variable* result, Label* exit, Label* slowPath)
{
    auto env = GetEnvironment();
    Label isPendingException(env);
    Label noPendingException(env);
    GateRef msg = GetCallArg0(numArgs_);
    GateRef obj = ToObject(glue_, msg);
    NewObjectStubBuilder newBuilder(this);
    BRANCH(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        Jump(exit);
    }
    Bind(&noPendingException);
    {
        Label isFast(env);
        BRANCH(IsNotSlowObjectKey(obj), &isFast, slowPath);
        Bind(&isFast);
        {
            Label notDictMode(env);
            GateRef isDictMode = LogicOrBuilder(env).Or(IsDictionaryMode(GetElementsArray(obj)))
                .Or(IsDictionaryMode(GetPropertiesArray(obj))).Done();
            BRANCH(isDictMode, slowPath, &notDictMode);
            Bind(&notDictMode);
            {
                Label hasKeyAndEle(env);
                Label nonKeyAndEle(env);
                GateRef elementArray = GetEnumElementEntries(glue_, obj, slowPath);
                GateRef propertyArray = GetEnumPropertyEntries(glue_, obj, slowPath);
                GateRef elementLen = GetLengthOfTaggedArray(elementArray);
                GateRef propertyLen = GetLengthOfTaggedArray(propertyArray);
                // need to caclulate elementKind
                GateRef elementKind = Int32(Elements::ToUint(ElementsKind::GENERIC));
                GateRef keyAndEle = BitAnd(Int32NotEqual(elementLen, Int32(0)), Int32NotEqual(propertyLen, Int32(0)));
                BRANCH(keyAndEle, &hasKeyAndEle, &nonKeyAndEle);
                Bind(&hasKeyAndEle);
                {
                    GateRef allEntries = AppendSkipHole(glue_, elementArray, propertyArray,
                        Int32Add(elementLen, propertyLen));
                    *result = newBuilder.CreateArrayFromList(glue_, allEntries, elementKind);
                    Jump(exit);
                }
                Bind(&nonKeyAndEle);
                {
                    Label hasKey(env);
                    Label nonKey(env);
                    BRANCH(Int32NotEqual(propertyLen, Int32(0)), &hasKey, &nonKey);
                    Bind(&hasKey);
                    {
                        *result = newBuilder.CreateArrayFromList(glue_, propertyArray, elementKind);
                        Jump(exit);
                    }
                    Bind(&nonKey);
                    {
                        Label hasEle(env);
                        Label nonEle(env);
                        BRANCH(Int32NotEqual(elementLen, Int32(0)), &hasEle, &nonEle);
                        Bind(&hasEle);
                        {
                            *result = newBuilder.CreateArrayFromList(glue_, elementArray, elementKind);
                            Jump(exit);
                        }
                        Bind(&nonEle);
                        {
                            GateRef emptyArray = GetEmptyArray(glue_);
                            *result = newBuilder.CreateArrayFromList(glue_, emptyArray, elementKind);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

GateRef BuiltinsObjectStubBuilder::GetEnumElementEntries(GateRef glue, GateRef obj, Label* slowPath)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    DEFVARIABLE(idx, VariableType::INT32(), Int32(0));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    GateRef elementKeys = GetEnumElementKeys(glue, obj);
    GateRef elements = GetElementsArray(obj);
    GateRef len = GetLengthOfTaggedArray(elements);
    GateRef realLen = GetLengthOfTaggedArray(elementKeys);
    NewObjectStubBuilder newBuilder(this);
    GateRef numElementArray = newBuilder.NewTaggedArray(glue, realLen);
    // need to caclulate elementKind
    GateRef elementKind = Int32(Elements::ToUint(ElementsKind::TAGGED));
    Label isJSPrimitiveRef(env);
    Label notPrimitiveString(env);
    Label notDictMode(env);
    BRANCH(IsJSPrimitiveRef(obj), &isJSPrimitiveRef, &notPrimitiveString);
    Bind(&isJSPrimitiveRef);
    GateRef els = Load(VariableType::JS_ANY(), obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
    BRANCH(TaggedIsString(els), slowPath, &notPrimitiveString);
    Bind(&notPrimitiveString);
    BRANCH(IsDictionaryMode(elements), slowPath, &notDictMode);
    Bind(&notDictMode);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label LoopNext(env);
        Label loopExit(env);

        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH(Int32LessThan(*idx, len), &LoopNext, &loopExit);
            Bind(&LoopNext);
            GateRef value = GetTaggedValueWithElementsKind(glue, obj, *idx);
            Label notHole(env);
            BRANCH(TaggedIsHole(value), &loopEnd, &notHole);
            Bind(&notHole);
            {
                GateRef arrayProp = newBuilder.NewTaggedArray(glue, Int32(2));
                GateRef str = IntToEcmaString(glue, *idx);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, arrayProp, Int32(0), str);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, arrayProp, Int32(1), value);
                GateRef propArray = newBuilder.CreateArrayFromList(glue, arrayProp, elementKind);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, numElementArray, *pos, propArray);
                pos = Int32Add(*pos, Int32(1));
                Jump(&loopEnd);
            }
        }

        Bind(&loopEnd);
        idx = Int32Add(*idx, Int32(1));
        LoopEnd(&loopHead, env, glue);
        Bind(&loopExit);
        auto ret = numElementArray;
        env->SubCfgExit();
        return ret;
    }
}

GateRef BuiltinsObjectStubBuilder::GetEnumPropertyEntries(GateRef glue, GateRef obj, Label* slowPath)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label notDictionary(env);
    GateRef array = GetPropertiesArray(obj);
    BRANCH(IsDictionaryMode(array), slowPath, &notDictionary);
    Bind(&notDictionary);

    DEFVARIABLE(idx, VariableType::INT32(), Int32(0));
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    GateRef cls = LoadHClass(obj);
    GateRef len = GetNumberOfPropsFromHClass(cls);
    GateRef layout = GetLayoutFromHClass(cls);

    NewObjectStubBuilder newBuilder(this);
    GateRef allEnumArray = newBuilder.NewTaggedArray(glue, len);
    // need to caclulate elementKind
    GateRef elementKind = Int32(Elements::ToUint(ElementsKind::TAGGED));
    Label loopHead(env);
    Label loopEnd(env);
    Label LoopNext(env);
    Label loopExit(env);
    Label propertyIsEnumerable(env);
    Label propertyIsString(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32LessThan(*idx, len), &LoopNext, &loopExit);
        Bind(&LoopNext);
        GateRef arrayProp = newBuilder.NewTaggedArray(glue, Int32(2)); // 2: length of array to store [key, value] is 2
        GateRef key = GetKeyFromLayoutInfo(layout, *idx);
        GateRef attr = GetPropAttrFromLayoutInfo(layout, *idx);
        GateRef value = JSObjectGetProperty(obj, cls, attr);
        BRANCH(IsEnumerable(attr), &propertyIsEnumerable, &loopEnd);
        Bind(&propertyIsEnumerable);
        {
            BRANCH(TaggedIsString(key), &propertyIsString, &loopEnd);
            Bind(&propertyIsString);
            {
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, arrayProp, Int32(0), key);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, arrayProp, Int32(1), value);
                GateRef propArray = newBuilder.CreateArrayFromList(glue, arrayProp, elementKind);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, allEnumArray, *length, propArray);
                length = Int32Add(*length, Int32(1));
                Jump(&loopEnd);
            }
        }
    }
    Bind(&loopEnd);
    idx = Int32Add(*idx, Int32(1));
    LoopEnd(&loopHead, env, glue);
    Bind(&loopExit);
    Label needTrim(env);
    BRANCH(Int32LessThan(*length, len), &needTrim, &exit);
    Bind(&needTrim);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, allEnumArray, ZExtInt32ToInt64(*length)});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = allEnumArray;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::IsFrozen(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs_);
    Label isEcmaObj(env);
    Label notEcmaObj(env);
    BRANCH(BoolNot(IsEcmaObject(obj)), &notEcmaObj, &isEcmaObj);
    Bind(&notEcmaObj);
    {
        result->WriteVariable(TaggedTrue());
        Jump(exit);
    }
    Bind(&isEcmaObj);
    {
        // 1: IntegrityLevel::FROZEN
        GateRef status = TestIntegrityLevel(glue_, obj, Int32(1), slowPath);
        Label statusIsTrue(env);
        Label statusIsFalse(env);
        BRANCH(status, &statusIsTrue, &statusIsFalse);
        Bind(&statusIsTrue);
        {
            result->WriteVariable(TaggedTrue());
            Jump(exit);
        }
        Bind(&statusIsFalse);
        {
            result->WriteVariable(TaggedFalse());
            Jump(exit);
        }
    }
}

void BuiltinsObjectStubBuilder::IsSealed(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs_);
    Label isEcmaObj(env);
    Label notEcmaObj(env);
    BRANCH(BoolNot(IsEcmaObject(obj)), &notEcmaObj, &isEcmaObj);
    Bind(&notEcmaObj);
    {
        result->WriteVariable(TaggedTrue());
        Jump(exit);
    }
    Bind(&isEcmaObj);
    {
        // 0: IntegrityLevel::SEALED
        GateRef status = TestIntegrityLevel(glue_, obj, Int32(0), slowPath);
        Label statusIsTrue(env);
        Label statusIsFalse(env);
        BRANCH(status, &statusIsTrue, &statusIsFalse);
        Bind(&statusIsTrue);
        {
            result->WriteVariable(TaggedTrue());
            Jump(exit);
        }
        Bind(&statusIsFalse);
        {
            result->WriteVariable(TaggedFalse());
            Jump(exit);
        }
    }
}

GateRef BuiltinsObjectStubBuilder::TestIntegrityLevel(GateRef glue,
    GateRef obj, GateRef level, Label *slowPath)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isExtensible(env);
    Label isNotExtensible(env);
    Label exit(env);

    Label isNotJsProxy(env);
    Label isNotTypedArray(env);
    Label isNotModuleNamespace(env);
    Label isNotSpecialContainer(env);
    Label notDicMode(env);
    Label notHasElementKey(env);
    BRANCH(IsJsProxy(obj), slowPath, &isNotJsProxy);
    Bind(&isNotJsProxy);
    BRANCH(IsTypedArray(obj), slowPath, &isNotTypedArray);
    Bind(&isNotTypedArray);
    BRANCH(IsModuleNamespace(obj), slowPath, &isNotModuleNamespace);
    Bind(&isNotModuleNamespace);
    BRANCH(IsArrayListOrVector(GetObjectType(LoadHClass(obj))), slowPath, &isNotSpecialContainer);
    Bind(&isNotSpecialContainer);
    GateRef cls = LoadHClass(obj);
    BRANCH(IsDictionaryModeByHClass(cls), slowPath, &notDicMode);
    Bind(&notDicMode);
    GateRef layout = GetLayoutFromHClass(cls);
    GateRef lengthOfKeys = GetNumberOfPropsFromHClass(cls);
    GateRef elementArray = GetEnumElementKeys(glue_, obj);
    GateRef lengthOfElements = GetLengthOfTaggedArray(elementArray);
    BRANCH(Int32Equal(lengthOfElements, Int32(0)), &notHasElementKey, slowPath);
    Bind(&notHasElementKey);

    GateRef status = IsExtensible(obj);
    BRANCH(status, &isExtensible, &isNotExtensible);
    Bind(&isExtensible);
    {
        result = False();
        Jump(&exit);
    }
    Bind(&isNotExtensible);
    {
        Label lengthIsZero(env);
        Label lengthNotZero(env);
        BRANCH(Int32Equal(lengthOfKeys, Int32(0)), &lengthIsZero, &lengthNotZero);
        Bind(&lengthIsZero);
        {
            result = True();
            Jump(&exit);
        }
        Bind(&lengthNotZero);
        {
            DEFVARIABLE(index, VariableType::INT32(), Int32(0));
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Label inRange(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32LessThan(*index, lengthOfKeys), &inRange, &afterLoop);
                Bind(&inRange);
                {
                    GateRef attr = GetPropAttrFromLayoutInfo(layout, *index);
                    Label configable(env);
                    Label notConfigable(env);
                    Label notFrozen(env);
                    BRANCH(IsConfigable(attr), &configable, &notConfigable);
                    Bind(&configable);
                    {
                        result = False();
                        Jump(&exit);
                    }
                    Bind(&notConfigable);
                    {
                        // 1: IntegrityLevel::FROZEN
                        GateRef isFrozen = LogicAndBuilder(env).And(Int32Equal(level, Int32(1)))
                            .And(BoolNot(IsAccessor(attr))).And(IsWritable(attr)).Done();
                        BRANCH(isFrozen, &notFrozen, &loopEnd);
                        Bind(&notFrozen);
                        {
                            result = False();
                            Jump(&exit);
                        }
                    }
                }
            }
            Bind(&loopEnd);
            index = Int32Add(*index, Int32(1));
            LoopEnd(&loopHead, env, glue);
            Bind(&afterLoop);
            {
                result = True();
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsObjectStubBuilder::GetOwnPropertyDescriptors(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef msg = GetCallArg0(numArgs_);
    GateRef obj = ToObject(glue_, msg);
    Label isPendingException(env);
    Label noPendingException(env);
    BRANCH(HasPendingException(glue_), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        Jump(exit);
    }
    Bind(&noPendingException);
    Label isFast(env);
    BRANCH(IsNotSlowObjectKey(obj), &isFast, slowPath);
    Bind(&isFast);
    Label notDictMode(env);
    GateRef properties = GetPropertiesArray(obj);
    BRANCH(IsDictionaryMode(properties), slowPath, &notDictMode);
    Bind(&notDictMode);
    Label onlyProperties(env);
    GateRef numOfElements = GetNumberOfElements(glue_, obj);
    BRANCH(Int32Equal(numOfElements, Int32(0)), &onlyProperties, slowPath);
    Bind(&onlyProperties);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef hclass = LoadHClass(obj);
    GateRef layout = GetLayoutFromHClass(hclass);
    GateRef number = GetNumberOfPropsFromHClass(hclass);
    GateRef valueStr = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::VALUE_STRING_INDEX);
    GateRef getterStr = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::GET_STRING_INDEX);
    GateRef setterStr = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::SET_STRING_INDEX);
    GateRef writableStr = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                 ConstantIndex::WRITABLE_STRING_INDEX);
    GateRef enumerableStr = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                   ConstantIndex::ENUMERABLE_STRING_INDEX);
    GateRef configurableStr = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                     ConstantIndex::CONFIGURABLE_STRING_INDEX);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue_, 0);
    GateRef descriptors = newBuilder.CreateEmptyObject(glue_);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*i, number), &next, &loopExit);
        Bind(&next);
        {
            Label isAccessor(env);
            Label setValueAndIsWritable(env);
            Label setIsEnumerable(env);
            Label setIsConfigable(env);
            Label setDescriptor(env);
            GateRef key = GetKey(layout, *i);
            GateRef attr = GetAttr(layout, *i);
            GateRef descriptor = newBuilder.CreateEmptyObject(glue_);
            DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
            value = JSObjectGetProperty(obj, hclass, attr);
            BRANCH(IsAccessor(attr), &isAccessor, &setValueAndIsWritable);
            Bind(&isAccessor);
            {
                Label propertyBox(env);
                Label checkInternalAccessor(env);
                BRANCH(TaggedIsPropertyBox(*value), &propertyBox, &checkInternalAccessor);
                Bind(&propertyBox);
                {
                    value = GetValueFromPropertyBox(*value);
                    Jump(&checkInternalAccessor);
                }
                Bind(&checkInternalAccessor);
                Label isInternalAccesstor(env);
                Label notInternalAccesstor(env);
                BRANCH_UNLIKELY(TaggedIsInternalAccessor(*value), &isInternalAccesstor, &notInternalAccesstor);
                Bind(&isInternalAccesstor);
                {
                    value = CallGetterHelper(glue_, obj, obj, *value, ProfileOperation());
                    Jump(&setValueAndIsWritable);
                }
                Bind(&notInternalAccesstor);
                {
                    Label setGetter(env);
                    Label getSetter(env);
                    Label setSetter(env);
                    GateRef getter = GetAccGetter(*value);
                    BRANCH(TaggedIsHeapObject(getter), &setGetter, &getSetter);
                    Bind(&setGetter);
                    {
                        FastSetPropertyByName(glue_, descriptor, getterStr, getter);
                        Jump(&getSetter);
                    }
                    Bind(&getSetter);
                    GateRef setter = GetAccSetter(*value);
                    BRANCH(TaggedIsHeapObject(setter), &setSetter, &setIsEnumerable);
                    Bind(&setSetter);
                    {
                        FastSetPropertyByName(glue_, descriptor, setterStr, setter);
                        Jump(&setIsEnumerable);
                    }
                }
            }
            Bind(&setValueAndIsWritable);
            {
                Label isWritable(env);
                Label notWritable(env);
                FastSetPropertyByName(glue_, descriptor, valueStr, *value);
                BRANCH(IsWritable(attr), &isWritable, &notWritable);
                Bind(&isWritable);
                {
                    FastSetPropertyByName(glue_, descriptor, writableStr, TaggedTrue());
                    Jump(&setIsEnumerable);
                }
                Bind(&notWritable);
                {
                    FastSetPropertyByName(glue_, descriptor, writableStr, TaggedFalse());
                    Jump(&setIsEnumerable);
                }
            }
            Bind(&setIsEnumerable);
            {
                Label isEnumerable(env);
                Label notEnumerable(env);
                BRANCH(IsEnumerable(attr), &isEnumerable, &notEnumerable);
                Bind(&isEnumerable);
                {
                    FastSetPropertyByName(glue_, descriptor, enumerableStr, TaggedTrue());
                    Jump(&setIsConfigable);
                }
                Bind(&notEnumerable);
                {
                    FastSetPropertyByName(glue_, descriptor, enumerableStr, TaggedFalse());
                    Jump(&setIsConfigable);
                }
            }
            Bind(&setIsConfigable);
            {
                Label isConfigable(env);
                Label notConfigable(env);
                BRANCH(IsConfigable(attr), &isConfigable, &notConfigable);
                Bind(&isConfigable);
                {
                    FastSetPropertyByName(glue_, descriptor, configurableStr, TaggedTrue());
                    Jump(&setDescriptor);
                }
                Bind(&notConfigable);
                {
                    FastSetPropertyByName(glue_, descriptor, configurableStr, TaggedFalse());
                    Jump(&setDescriptor);
                }
            }
            Bind(&setDescriptor);
            {
                FastSetPropertyByName(glue_, descriptors, key, descriptor);
                Jump(&loopEnd);
            }
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead, env, glue_);
    Bind(&loopExit);
    *result = descriptors;
    Jump(exit);
}

}  // namespace panda::ecmascript::kungfu
