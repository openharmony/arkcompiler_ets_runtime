/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/profiler_stub_builder.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/typed_array_stub_builder.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/message_string.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda::ecmascript::kungfu {
void StubBuilder::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = env_->GetBuilder()->Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    env_->SetCurrentLabel(nullptr);
}

void StubBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = env_->GetBuilder()->Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = env_->GetBuilder()->IfTrue(ifBranch);
    trueLabel->AppendPredecessor(env_->GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = env_->GetBuilder()->IfFalse(ifBranch);
    falseLabel->AppendPredecessor(env_->GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_->SetCurrentLabel(nullptr);
}

void StubBuilder::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = env_->GetBuilder()->SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = env_->GetBuilder()->SwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = env_->GetBuilder()->DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_->SetCurrentLabel(nullptr);
}

void StubBuilder::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = env_->GetBuilder()->LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    env_->SetCurrentLabel(loopHead);
}

void StubBuilder::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = env_->GetBuilder()->LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    env_->SetCurrentLabel(nullptr);
}

void StubBuilder::MatchTrackType(GateRef trackType, GateRef value, Label *executeSetProp, Label *typeMismatch)
{
    auto *env = GetEnvironment();
    Label isNumber(env);
    Label checkBoolean(env);
    Label isBoolean(env);
    Label checkString(env);
    Label isString(env);
    Label checkJSShared(env);
    Label isJSShared(env);
    Label checkJSNone(env);
    Label isJSNone(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(Equal(trackType, Int32(static_cast<int32_t>(TrackType::NUMBER))), &isNumber, &checkBoolean);
    Bind(&isNumber);
    {
        result = TaggedIsNumber(value);
        Jump(&exit);
    }
    Bind(&checkBoolean);
    {
        Branch(Equal(trackType, Int32(static_cast<int32_t>(TrackType::BOOLEAN))), &isBoolean, &checkString);
        Bind(&isBoolean);
        {
            result = TaggedIsBoolean(value);
            Jump(&exit);
        }
    }
    Bind(&checkString);
    {
        Branch(Equal(trackType, Int32(static_cast<int32_t>(TrackType::STRING))), &isString, &checkJSShared);
        Bind(&isString);
        {
            result = BoolOr(TaggedIsString(value), TaggedIsNull(value));
            Jump(&exit);
        }
    }
    Bind(&checkJSShared);
    {
        Branch(Equal(trackType, Int32(static_cast<int32_t>(TrackType::SENDABLE))), &isJSShared, &checkJSNone);
        Bind(&isJSShared);
        {
            result = BoolOr(TaggedIsShared(value), TaggedIsNull(value));
            Jump(&exit);
        }
    }
    Bind(&checkJSNone);
    {
        Branch(Equal(trackType, Int32(static_cast<int32_t>(TrackType::NONE))), &isJSNone, &exit);
        Bind(&isJSNone);
        {
            // bypass none type
            result = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    Branch(*result, executeSetProp, typeMismatch);
}

// FindElementWithCache in ecmascript/layout_info-inl.h
GateRef StubBuilder::FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hclass,
    GateRef key, GateRef propsNum)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label exit(env);
    Label notExceedUpper(env);
    Label exceedUpper(env);
    // 9 : Builtins Object properties number is nine
    Branch(Int32LessThanOrEqual(propsNum, Int32(9)), &notExceedUpper, &exceedUpper);
    Bind(&notExceedUpper);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label propsNumIsZero(env);
            Label propsNumNotZero(env);
            Branch(Int32Equal(propsNum, Int32(0)), &propsNumIsZero, &propsNumNotZero);
            Bind(&propsNumIsZero);
            Jump(&afterLoop);
            Bind(&propsNumNotZero);
            GateRef elementAddr = GetPropertiesAddrFromLayoutInfo(layoutInfo);
            GateRef keyInProperty = Load(VariableType::JS_ANY(), elementAddr,
                                         PtrMul(ZExtInt32ToPtr(*i), IntPtr(sizeof(panda::ecmascript::Properties))));
            Label equal(env);
            Label notEqual(env);
            Label afterEqualCon(env);
            Branch(Equal(keyInProperty, key), &equal, &notEqual);
            Bind(&equal);
            result = *i;
            Jump(&exit);
            Bind(&notEqual);
            Jump(&afterEqualCon);
            Bind(&afterEqualCon);
            i = Int32Add(*i, Int32(1));
            Branch(Int32UnsignedLessThan(*i, propsNum), &loopEnd, &afterLoop);
            Bind(&loopEnd);
            LoopEnd(&loopHead);
        }
        Bind(&afterLoop);
        result = Int32(-1);
        Jump(&exit);
    }
    Bind(&exceedUpper);
    result = CallNGCRuntime(glue, RTSTUB_ID(FindElementWithCache), { glue, hclass, key, propsNum });
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    Label exit(env);
    GateRef capcityoffset =
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()),
               IntPtr(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = IntPtr(TaggedArray::DATA_OFFSET);
    GateRef capacity = GetInt32OfTInt(Load(VariableType::INT64(), elements,
                                           PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, VariableType::INT32(), Int32(1));
    GateRef len = Int32(sizeof(int) / sizeof(uint8_t));
    GateRef hash = CallRuntime(glue, RTSTUB_ID(GetHash32),
        { IntToTaggedInt(index), IntToTaggedInt(len) });
    DEFVARIABLE(entry, VariableType::INT32(),
        Int32And(TruncInt64ToInt32(ChangeTaggedPointerToInt64(hash)), Int32Sub(capacity, Int32(1))));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    GateRef element = GetKeyFromDictionary<NumberDictionary>(elements, *entry);
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(element), &isHole, &notHole);
    Bind(&isHole);
    Jump(&loopEnd);
    Bind(&notHole);
    Label isUndefined(env);
    Label notUndefined(env);
    Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    result = Int32(-1);
    Jump(&exit);
    Bind(&notUndefined);
    Label isMatch(env);
    Label notMatch(env);
    Branch(Int32Equal(index, GetInt32OfTInt(element)), &isMatch, &notMatch);
    Bind(&isMatch);
    result = *entry;
    Jump(&exit);
    Bind(&notMatch);
    Jump(&loopEnd);
    Bind(&loopEnd);
    entry = GetNextPositionForHash(*entry, *count, capacity);
    count = Int32Add(*count, Int32(1));
    LoopEnd(&loopHead);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// int TaggedHashTable<Derived>::FindEntry(const JSTaggedValue &key) in tagged_hash_table.h
GateRef StubBuilder::FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->SubCfgEntry(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    GateRef capcityoffset =
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()),
               IntPtr(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = IntPtr(TaggedArray::DATA_OFFSET);
    GateRef capacity = GetInt32OfTInt(Load(VariableType::INT64(), elements,
                                           PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, VariableType::INT32(), Int32(1));
    DEFVARIABLE(hash, VariableType::INT32(), Int32(0));
    // NameDictionary::hash
    Label isSymbol(env);
    Label notSymbol(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label beforeDefineHash(env);
    Branch(IsSymbol(key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = GetInt32OfTInt(Load(VariableType::INT64(), key,
            IntPtr(JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        Branch(IsString(key), &isString, &notString);
        Bind(&isString);
        {
            hash = GetHashcodeFromString(glue, key);
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, VariableType::INT32(), Int32And(*hash, Int32Sub(capacity, Int32(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef element = GetKeyFromDictionary<NameDictionary>(elements, *entry);
        Label isHole(env);
        Label notHole(env);
        Branch(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
                {
                    Bind(&isUndefined);
                    {
                        result = Int32(-1);
                        Jump(&exit);
                    }
                    Bind(&notUndefined);
                    {
                        Label isMatch(env);
                        Label notMatch(env);
                        Branch(Equal(key, element), &isMatch, &notMatch);
                        {
                            Bind(&isMatch);
                            {
                                result = *entry;
                                Jump(&exit);
                            }
                            Bind(&notMatch);
                            {
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        {
            entry = GetNextPositionForHash(*entry, *count, capacity);
            count = Int32Add(*count, Int32(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr)
{
    return BoolAnd(Equal(element, key), Int32Equal(metaData, attr));
}

// metaData is int32 type
GateRef StubBuilder::FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->SubCfgEntry(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    GateRef capcityoffset =
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()),
               IntPtr(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = IntPtr(TaggedArray::DATA_OFFSET);
    GateRef capacity = GetInt32OfTInt(Load(VariableType::INT64(), elements,
                                           PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, VariableType::INT32(), Int32(1));
    DEFVARIABLE(hash, VariableType::INT32(), Int32(0));
    // TransitionDictionary::hash
    Label isSymbol(env);
    Label notSymbol(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label beforeDefineHash(env);
    Branch(IsSymbol(key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = GetInt32OfTInt(Load(VariableType::INT64(), key,
            IntPtr(panda::ecmascript::JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        Branch(IsString(key), &isString, &notString);
        Bind(&isString);
        {
            hash = GetHashcodeFromString(glue, key);
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    hash = Int32Add(*hash, metaData);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, VariableType::INT32(), Int32And(*hash, Int32Sub(capacity, Int32(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef element = GetKeyFromDictionary<TransitionsDictionary>(elements, *entry);
        Label isHole(env);
        Label notHole(env);
        Branch(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
                {
                    Bind(&isUndefined);
                    {
                        result = Int32(-1);
                        Jump(&exit);
                    }
                    Bind(&notUndefined);
                    {
                        Label isMatch(env);
                        Label notMatch(env);
                        Branch(
                            IsMatchInTransitionDictionary(element, key, metaData,
                                GetAttributesFromDictionary<TransitionsDictionary>(elements, *entry)),
                            &isMatch, &notMatch);
                        {
                            Bind(&isMatch);
                            {
                                result = *entry;
                                Jump(&exit);
                            }
                            Bind(&notMatch);
                            {
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        {
            entry = GetNextPositionForHash(*entry, *count, capacity);
            count = Int32Add(*count, Int32(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::JSObjectGetProperty(GateRef obj, GateRef hclass, GateRef attr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label inlinedProp(env);
    Label notInlinedProp(env);
    Label post(env);
    GateRef attrOffset = GetOffsetFieldInPropAttr(attr);
    GateRef rep = GetRepInPropAttr(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            result = GetPropertyInlinedProps(obj, hclass, attrOffset);
            Jump(&post);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array =
                Load(VariableType::INT64(), obj, IntPtr(JSObject::PROPERTIES_OFFSET));
            result = GetValueFromTaggedArray(array, Int32Sub(attrOffset,
                GetInlinedPropertiesFromHClass(hclass)));
            Jump(&post);
        }
    }
    Bind(&post);
    {
        Label nonDoubleToTagged(env);
        Label doubleToTagged(env);
        Branch(IsDoubleRepInPropAttr(rep), &doubleToTagged, &nonDoubleToTagged);
        Bind(&doubleToTagged);
        {
            result = TaggedPtrToTaggedDoublePtr(*result);
            Jump(&exit);
        }
        Bind(&nonDoubleToTagged);
        {
            Label intToTagged(env);
            Branch(IsIntRepInPropAttr(rep), &intToTagged, &exit);
            Bind(&intToTagged);
            {
                result = TaggedPtrToTaggedIntPtr(*result);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::JSObjectSetProperty(
    GateRef glue, GateRef obj, GateRef hclass, GateRef attr, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label inlinedProp(env);
    Label notInlinedProp(env);
    GateRef attrIndex = GetOffsetFieldInPropAttr(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            GateRef offset = GetInlinedPropOffsetFromHClass(hclass, attrIndex);
            SetValueWithAttr(glue, obj, offset, key, value, attr);
            Jump(&exit);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array = Load(VariableType::JS_POINTER(), obj,
                                 IntPtr(JSObject::PROPERTIES_OFFSET));
            GateRef offset = Int32Sub(attrIndex, GetInlinedPropertiesFromHClass(hclass));
            SetValueToTaggedArrayWithAttr(glue, array, offset, key, value, attr);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef StubBuilder::ComputeNonInlinedFastPropsCapacity(GateRef glue, GateRef oldLength,
                                                        GateRef maxNonInlinedFastPropsCapacity)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef propertiesStep = Load(VariableType::INT32(), glue,
        IntPtr(JSThread::GlueData::GetPropertiesGrowStepOffset(env->Is32Bit())));
    GateRef newL = Int32Add(oldLength, propertiesStep);
    Label reachMax(env);
    Label notReachMax(env);
    Branch(Int32GreaterThan(newL, maxNonInlinedFastPropsCapacity), &reachMax, &notReachMax);
    {
        Bind(&reachMax);
        result = maxNonInlinedFastPropsCapacity;
        Jump(&exit);
        Bind(&notReachMax);
        result = newL;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ComputeElementCapacity(GateRef oldLength)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef newL = Int32Add(oldLength, Int32LSR(oldLength, Int32(1)));
    Label reachMin(env);
    Label notReachMin(env);
    Branch(Int32GreaterThan(newL, Int32(JSObject::MIN_ELEMENTS_LENGTH)), &reachMin, &notReachMin);
    {
        Bind(&reachMin);
        result = newL;
        Jump(&exit);
        Bind(&notReachMin);
        result = Int32(JSObject::MIN_ELEMENTS_LENGTH);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CallGetterHelper(
    GateRef glue, GateRef receiver, GateRef holder, GateRef accessor, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label arrayLength(env);
        Label tryContinue(env);
        auto lengthAccessor = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
        Branch(Equal(accessor, lengthAccessor), &arrayLength, &tryContinue);
        Bind(&arrayLength);
        {
            auto length = Load(VariableType::INT32(), holder, IntPtr(JSArray::LENGTH_OFFSET));
            // TaggedInt supports up to INT32_MAX.
            // If length is greater than Int32_MAX, needs to be converted to TaggedDouble.
            auto condition = Int32UnsignedGreaterThan(length, Int32(INT32_MAX));
            Label overflow(env);
            Label notOverflow(env);
            Branch(condition, &overflow, &notOverflow);
            Bind(&overflow);
            {
                result = DoubleToTaggedDoublePtr(ChangeUInt32ToFloat64(length));
                Jump(&exit);
            }
            Bind(&notOverflow);
            {
                result = IntToTaggedPtr(length);
                Jump(&exit);
            }
        }
        Bind(&tryContinue);
        result = CallRuntime(glue, RTSTUB_ID(CallInternalGetter), { accessor, holder });
        Jump(&exit);
    }
    Bind(&notInternal);
    {
        auto getter = Load(VariableType::JS_ANY(), accessor,
                           IntPtr(AccessorData::GETTER_OFFSET));
        Label objIsUndefined(env);
        Label objNotUndefined(env);
        Branch(TaggedIsUndefined(getter), &objIsUndefined, &objNotUndefined);
        // if getter is undefined, return undefiend
        Bind(&objIsUndefined);
        {
            result = Undefined();
            Jump(&exit);
        }
        Bind(&objNotUndefined);
        {
            auto retValue = JSCallDispatch(glue, getter, Int32(0), 0, Circuit::NullGate(),
                                           JSCallMode::CALL_GETTER, { receiver }, callback);
            Label noPendingException(env);
            Branch(HasPendingException(glue), &exit, &noPendingException);
            Bind(&noPendingException);
            {
                result = retValue;
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CallSetterHelper(
    GateRef glue, GateRef receiver, GateRef accessor, GateRef value, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        result = CallRuntime(glue, RTSTUB_ID(CallInternalSetter), { receiver, accessor, value });
        Jump(&exit);
    }
    Bind(&notInternal);
    {
        auto setter = Load(VariableType::JS_ANY(), accessor,
                           IntPtr(AccessorData::SETTER_OFFSET));
        Label objIsUndefined(env);
        Label objNotUndefined(env);
        Branch(TaggedIsUndefined(setter), &objIsUndefined, &objNotUndefined);
        Bind(&objIsUndefined);
        {
            CallRuntime(glue, RTSTUB_ID(ThrowSetterIsUndefinedException), {});
            result = Exception();
            Jump(&exit);
        }
        Bind(&objNotUndefined);
        {
            auto retValue = JSCallDispatch(glue, setter, Int32(1), 0, Circuit::NullGate(),
                                           JSCallMode::CALL_SETTER, { receiver, value }, callback);
            Label noPendingException(env);
            Branch(HasPendingException(glue), &exit, &noPendingException);
            Bind(&noPendingException);
            {
                result = retValue;
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ShouldCallSetter(GateRef receiver, GateRef holder, GateRef accessor, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label receiverEqualsHolder(env);
        Label receiverNotEqualsHolder(env);
        Branch(Equal(receiver, holder), &receiverEqualsHolder, &receiverNotEqualsHolder);
        Bind(&receiverEqualsHolder);
        {
            result = IsWritable(attr);
            Jump(&exit);
        }
        Bind(&receiverNotEqualsHolder);
        {
            result = False();
            Jump(&exit);
        }
    }
    Bind(&notInternal);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    GateRef hclass = LoadHClass(receiver);
    GateRef metaData = GetPropertyMetaDataFromAttr(attr);
    GateRef newClass = FindTransitions(glue, receiver, hclass, key, metaData);
    Label findHClass(env);
    Label notFindHClass(env);
    Branch(Equal(newClass, Undefined()), &notFindHClass, &findHClass);
    Bind(&findHClass);
    {
        Jump(&exit);
    }
    Bind(&notFindHClass);
    {
        GateRef type = GetObjectType(hclass);
        GateRef size = Int32Mul(GetInlinedPropsStartFromHClass(hclass),
                                Int32(JSTaggedValue::TaggedTypeSize()));
        GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
        GateRef newJshclass = CallRuntime(glue, RTSTUB_ID(NewEcmaHClass),
            { IntToTaggedInt(size), IntToTaggedInt(type),
              IntToTaggedInt(inlineProps) });
        CopyAllHClass(glue, newJshclass, hclass);
        CallRuntime(glue, RTSTUB_ID(UpdateLayOutAndAddTransition),
                    { hclass, newJshclass, key, IntToTaggedInt(attr) });
#if ECMASCRIPT_ENABLE_IC
        NotifyHClassChanged(glue, hclass, newJshclass);
#endif
        StoreHClass(glue, receiver, newJshclass);
        // Because we currently only supports Fast ElementsKind
        CallRuntime(glue, RTSTUB_ID(TryRestoreElementsKind), { receiver, newJshclass });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

// if condition:(objHandle->IsJSArray() || objHandle->IsTypedArray()) &&
//      keyHandle.GetTaggedValue() == thread->GlobalConstants()->GetConstructorString()
GateRef StubBuilder::SetHasConstructorCondition(GateRef glue, GateRef receiver, GateRef key)
{
    GateRef gConstOffset = Load(VariableType::JS_ANY(), glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));

    GateRef gCtorStr = Load(VariableType::JS_ANY(),
        gConstOffset,
        Int64Mul(Int64(sizeof(JSTaggedValue)),
            Int64(static_cast<uint64_t>(ConstantIndex::CONSTRUCTOR_STRING_INDEX))));
    GateRef isCtorStr = Equal(key, gCtorStr);
    return BoolAnd(BoolOr(IsJsArray(receiver), IsTypedArray(receiver)), isCtorStr);
}

// Note: set return exit node
GateRef StubBuilder::AddPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value,
                                       GateRef propertyAttributes, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label setHasCtor(env);
    Label notSetHasCtor(env);
    Label afterCtorCon(env);
    GateRef hclass = LoadHClass(receiver);
    Branch(SetHasConstructorCondition(glue, receiver, key), &setHasCtor, &notSetHasCtor);
    {
        Bind(&setHasCtor);
        SetHasConstructorToHClass(glue, hclass, Int32(1));
        Jump(&afterCtorCon);
        Bind(&notSetHasCtor);
        Jump(&afterCtorCon);
    }
    Bind(&afterCtorCon);
    // 0x111 : default attribute for property: writable, enumerable, configurable
    DEFVARIABLE(attr, VariableType::INT32(), propertyAttributes);
    GateRef numberOfProps = GetNumberOfPropsFromHClass(hclass);
    GateRef inlinedProperties = GetInlinedPropertiesFromHClass(hclass);
    Label hasUnusedInProps(env);
    Label noUnusedInProps(env);
    Label afterInPropsCon(env);
    Branch(Int32UnsignedLessThan(numberOfProps, inlinedProperties), &hasUnusedInProps, &noUnusedInProps);
    {
        Bind(&noUnusedInProps);
        Jump(&afterInPropsCon);
        Bind(&hasUnusedInProps);
        {
            attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
            attr = SetIsInlinePropsFieldInPropAttr(*attr, Int32(1)); // 1: set inInlineProps true
            attr = SetTaggedRepInPropAttr(*attr);
            attr = ProfilerStubBuilder(env).UpdateTrackTypeInPropAttr(*attr, value, callback);
            JSHClassAddProperty(glue, receiver, key, *attr);
            GateRef newHclass = LoadHClass(receiver);
            GateRef newLayoutInfo = GetLayoutFromHClass(newHclass);
            GateRef offset = GetInlinedPropOffsetFromHClass(hclass, numberOfProps);
            attr = GetInt32OfTInt(GetPropAttrFromLayoutInfo(newLayoutInfo, numberOfProps));
            SetValueWithAttr(glue, receiver, offset, key, value, *attr);
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&afterInPropsCon);
    DEFVARIABLE(array, VariableType::JS_POINTER(), GetPropertiesArray(receiver));
    DEFVARIABLE(length, VariableType::INT32(), GetLengthOfTaggedArray(*array));
    Label lenIsZero(env);
    Label lenNotZero(env);
    Label afterLenCon(env);
    Branch(Int32Equal(*length, Int32(0)), &lenIsZero, &lenNotZero);
    {
        Bind(&lenIsZero);
        {
            length = Int32(JSObject::MIN_PROPERTIES_LENGTH);
            array = CallRuntime(glue, RTSTUB_ID(NewTaggedArray), { IntToTaggedInt(*length) });
            SetPropertiesArray(VariableType::JS_POINTER(), glue, receiver, *array);
            Jump(&afterLenCon);
        }
        Bind(&lenNotZero);
        Jump(&afterLenCon);
    }
    Bind(&afterLenCon);
    Label isDictMode(env);
    Label notDictMode(env);
    Branch(IsDictionaryMode(*array), &isDictMode, &notDictMode);
    {
        Bind(&isDictMode);
        {
            GateRef res = CallRuntime(glue, RTSTUB_ID(NameDictPutIfAbsent),
                                      {receiver, *array, key, value, IntToTaggedInt(*attr), TaggedFalse()});
            SetPropertiesArray(VariableType::JS_POINTER(), glue, receiver, res);
            Jump(&exit);
        }
        Bind(&notDictMode);
        {
            attr = SetIsInlinePropsFieldInPropAttr(*attr, Int32(0));
            GateRef outProps = Int32Sub(numberOfProps, inlinedProperties);
            Label ChangeToDict(env);
            Label notChangeToDict(env);
            Label afterDictChangeCon(env);
            Branch(Int32GreaterThanOrEqual(numberOfProps, Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY)),
                &ChangeToDict, &notChangeToDict);
            {
                Bind(&ChangeToDict);
                {
                    attr = SetDictionaryOrderFieldInPropAttr(*attr,
                        Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY));
                    GateRef res = CallRuntime(glue, RTSTUB_ID(NameDictPutIfAbsent),
                        { receiver, *array, key, value, IntToTaggedInt(*attr), TaggedTrue() });
                    SetPropertiesArray(VariableType::JS_POINTER(), glue, receiver, res);
                    result = Undefined();
                    Jump(&exit);
                }
                Bind(&notChangeToDict);
                Jump(&afterDictChangeCon);
            }
            Bind(&afterDictChangeCon);
            Label isArrayFull(env);
            Label arrayNotFull(env);
            Label afterArrLenCon(env);
            Branch(Int32Equal(*length, outProps), &isArrayFull, &arrayNotFull);
            {
                Bind(&isArrayFull);
                {
                    GateRef maxNonInlinedFastPropsCapacity =
                        Int32Sub(Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY), inlinedProperties);
                    GateRef capacity = ComputeNonInlinedFastPropsCapacity(glue, *length,
                        maxNonInlinedFastPropsCapacity);
                    array = CallRuntime(glue, RTSTUB_ID(CopyArray),
                        { *array, IntToTaggedInt(*length), IntToTaggedInt(capacity) });
                    SetPropertiesArray(VariableType::JS_POINTER(), glue, receiver, *array);
                    Jump(&afterArrLenCon);
                }
                Bind(&arrayNotFull);
                Jump(&afterArrLenCon);
            }
            Bind(&afterArrLenCon);
            {
                attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
                attr = SetTaggedRepInPropAttr(*attr);
                attr = ProfilerStubBuilder(env).UpdateTrackTypeInPropAttr(*attr, value, callback);
                JSHClassAddProperty(glue, receiver, key, *attr);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *array, outProps, value);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val)
{
    GateRef msgIntId = Int32(messageId);
    CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(msgIntId) });
    Return(val);
}

GateRef StubBuilder::TaggedToRepresentation(GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(resultRep, VariableType::INT64(),
                Int64(static_cast<int32_t>(Representation::TAGGED)));
    Label isInt(env);
    Label notInt(env);

    Branch(TaggedIsInt(value), &isInt, &notInt);
    Bind(&isInt);
    {
        resultRep = Int64(static_cast<int32_t>(Representation::INT));
        Jump(&exit);
    }
    Bind(&notInt);
    {
        Label isDouble(env);
        Label notDouble(env);
        Branch(TaggedIsDouble(value), &isDouble, &notDouble);
        Bind(&isDouble);
        {
            resultRep = Int64(static_cast<int32_t>(Representation::DOUBLE));
            Jump(&exit);
        }
        Bind(&notDouble);
        {
            resultRep = Int64(static_cast<int32_t>(Representation::TAGGED));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *resultRep;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedToElementKind(GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::INT32(), Int32(static_cast<int32_t>(ElementsKind::TAGGED)));
    Label isHole(env);
    Label isNotHole(env);
    Branch(TaggedIsHole(value), &isHole, &isNotHole);
    Bind(&isHole);
    {
        result = Int32(static_cast<int32_t>(ElementsKind::HOLE));
        Jump(&exit);
    }
    Bind(&isNotHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        Branch(TaggedIsInt(value), &isInt, &isNotInt);
        Bind(&isInt);
        {
            result = Int32(static_cast<int32_t>(ElementsKind::INT));
            Jump(&exit);
        }
        Bind(&isNotInt);
        {
            Label isObject(env);
            Label isDouble(env);
            Branch(TaggedIsObject(value), &isObject, &isDouble);
            Bind(&isDouble);
            {
                result = Int32(static_cast<int32_t>(ElementsKind::NUMBER));
                Jump(&exit);
            }
            Bind(&isObject);
            {
                Label isHeapObject(env);
                Branch(TaggedIsHeapObject(value), &isHeapObject, &exit);
                Bind(&isHeapObject);
                {
                    Label isString(env);
                    Label isNonString(env);
                    Branch(TaggedIsString(value), &isString, &isNonString);
                    Bind(&isString);
                    {
                        result = Int32(static_cast<int32_t>(ElementsKind::STRING));
                        Jump(&exit);
                    }
                    Bind(&isNonString);
                    {
                        result = Int32(static_cast<int32_t>(ElementsKind::OBJECT));
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    if (!env_->IsAsmInterp()) {
        env_->GetBuilder()->Store(type, glue, base, offset, value);
    } else {
        auto depend = env_->GetCurrentLabel()->GetDepend();
        GateRef ptr = PtrAdd(base, offset);
        auto bit = LoadStoreAccessor::ToValue(MemoryOrder::Default());
        GateRef result = env_->GetCircuit()->NewGate(
            env_->GetCircuit()->Store(bit), MachineType::NOVALUE,
            { depend, value, ptr }, type.GetGateType());
        env_->GetCurrentLabel()->SetDepend(result);
        if (type == VariableType::JS_POINTER() || type == VariableType::JS_ANY()) {
            auto env = GetEnvironment();
            Label entry(env);
            env->SubCfgEntry(&entry);
            Label exit(env);
            Label isHeapObject(env);

            Branch(TaggedIsHeapObject(value), &isHeapObject, &exit);
            Bind(&isHeapObject);
            {
                CallNGCRuntime(glue, RTSTUB_ID(StoreBarrier), { glue, base, offset, value });
                Jump(&exit);
            }
            Bind(&exit);
            env->SubCfgExit();
        }
    }
}

void StubBuilder::SetValueWithAttr(GateRef glue, GateRef obj, GateRef offset, GateRef key, GateRef value, GateRef attr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label exit(env);
    Label repChange(env);
    GateRef rep = GetRepInPropAttr(attr);
    SetValueWithRep(glue, obj, offset, value, rep, &repChange);
    Jump(&exit);
    Bind(&repChange);
    {
        attr = SetTaggedRepInPropAttr(attr);
        TransitionForRepChange(glue, obj, key, attr);
        Store(VariableType::JS_ANY(), glue, obj, offset, value);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::SetValueWithRep(
    GateRef glue, GateRef obj, GateRef offset, GateRef value, GateRef rep, Label *repChange)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label exit(env);
    Label repIsDouble(env);
    Label repIsNonDouble(env);
    Branch(IsDoubleRepInPropAttr(rep), &repIsDouble, &repIsNonDouble);
    Bind(&repIsDouble);
    {
        Label valueIsInt(env);
        Label valueIsNotInt(env);
        Branch(TaggedIsInt(value), &valueIsInt, &valueIsNotInt);
        Bind(&valueIsInt);
        {
            GateRef result = GetDoubleOfTInt(value);
            Store(VariableType::FLOAT64(), glue, obj, offset, result);
            Jump(&exit);
        }
        Bind(&valueIsNotInt);
        {
            Label valueIsObject(env);
            Label valueIsDouble(env);
            Branch(TaggedIsObject(value), &valueIsObject, &valueIsDouble);
            Bind(&valueIsDouble);
            {
                // TaggedDouble to double
                GateRef result = GetDoubleOfTDouble(value);
                Store(VariableType::FLOAT64(), glue, obj, offset, result);
                Jump(&exit);
            }
            Bind(&valueIsObject);
            {
                Jump(repChange);
            }
        }
    }
    Bind(&repIsNonDouble);
    {
        Label repIsInt(env);
        Label repIsTagged(env);
        Branch(IsIntRepInPropAttr(rep), &repIsInt, &repIsTagged);
        Bind(&repIsInt);
        {
            Label valueIsInt(env);
            Label valueIsNotInt(env);
            Branch(TaggedIsInt(value), &valueIsInt, &valueIsNotInt);
            Bind(&valueIsInt);
            {
                GateRef result = GetInt32OfTInt(value);
                Store(VariableType::INT32(), glue, obj, offset, result);
                Jump(&exit);
            }
            Bind(&valueIsNotInt);
            {
                Jump(repChange);
            }
        }
        Bind(&repIsTagged);
        {
            Store(VariableType::JS_ANY(), glue, obj, offset, value);
            Jump(&exit);
        }
    }

    Bind(&exit);
    env->SubCfgExit();
    return;
}


void StubBuilder::SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isVailedIndex(env);
    Label notValidIndex(env);
    Label shareBarrier(env);
    Label shareBarrierExit(env);
    // ObjectAddressToRange function may cause obj is not an object. GC may not mark this obj.
    GateRef objectRegion = ObjectAddressToRange(obj);
    GateRef valueRegion = ObjectAddressToRange(value);
    GateRef slotAddr = PtrAdd(TaggedCastToIntPtr(obj), offset);
    GateRef objectNotInShare = BoolNot(InSharedHeap(objectRegion));
    GateRef valueRegionInShare = InSharedSweepableSpace(valueRegion);
    Branch(BoolAnd(objectNotInShare, valueRegionInShare), &shareBarrier, &shareBarrierExit);
    Bind(&shareBarrier);
    {
        CallNGCRuntime(glue, RTSTUB_ID(InsertLocalToShareRSet), { glue, obj, offset });
        Jump(&shareBarrierExit);
    }
    Bind(&shareBarrierExit);
    GateRef objectNotInYoung = BoolNot(InYoungGeneration(objectRegion));
    GateRef valueRegionInYoung = InYoungGeneration(valueRegion);
    Branch(BoolAnd(objectNotInYoung, valueRegionInYoung), &isVailedIndex, &notValidIndex);
    Bind(&isVailedIndex);
    {
        GateRef loadOffset = IntPtr(Region::PackedData::GetOldToNewSetOffset(env_->Is32Bit()));
        auto oldToNewSet = Load(VariableType::NATIVE_POINTER(), objectRegion, loadOffset);
        Label isNullPtr(env);
        Label notNullPtr(env);
        Branch(IntPtrEuqal(oldToNewSet, IntPtr(0)), &isNullPtr, &notNullPtr);
        Bind(&notNullPtr);
        {
            // (slotAddr - this) >> TAGGED_TYPE_SIZE_LOG
            GateRef bitOffsetPtr = IntPtrLSR(PtrSub(slotAddr, objectRegion), IntPtr(TAGGED_TYPE_SIZE_LOG));
            GateRef bitOffset = TruncPtrToInt32(bitOffsetPtr);
            GateRef bitPerWordLog2 = Int32(GCBitset::BIT_PER_WORD_LOG2);
            GateRef bytePerWord = Int32(GCBitset::BYTE_PER_WORD);
            // bitOffset >> BIT_PER_WORD_LOG2
            GateRef index = Int32LSR(bitOffset, bitPerWordLog2);
            GateRef byteIndex = Int32Mul(index, bytePerWord);
            // bitset_[index] |= mask;
            GateRef bitsetData = PtrAdd(oldToNewSet, IntPtr(RememberedSet::GCBITSET_DATA_OFFSET));
            GateRef oldsetValue = Load(VariableType::INT32(), bitsetData, byteIndex);
            GateRef newmapValue = Int32Or(oldsetValue, GetBitMask(bitOffset));

            Store(VariableType::INT32(), glue, bitsetData, byteIndex, newmapValue);
            Jump(&notValidIndex);
        }
        Bind(&isNullPtr);
        {
            CallNGCRuntime(glue, RTSTUB_ID(InsertOldToNewRSet), { glue, obj, offset });
            Jump(&notValidIndex);
        }
    }
    Bind(&notValidIndex);
    {
        Label marking(env);
        bool isArch32 = GetEnvironment()->Is32Bit();
        GateRef stateBitFieldAddr = Int64Add(glue,
                                             Int64(JSThread::GlueData::GetStateBitFieldOffset(isArch32)));
        GateRef stateBitField = Load(VariableType::INT64(), stateBitFieldAddr, Int64(0));
        // mask: 1 << JSThread::CONCURRENT_MARKING_BITFIELD_NUM - 1
        GateRef markingBitMask = Int64Sub(
            Int64LSL(Int64(1), Int64(JSThread::CONCURRENT_MARKING_BITFIELD_NUM)), Int64(1));
        GateRef state = Int64And(stateBitField, markingBitMask);
        Branch(Int64Equal(state, Int64(static_cast<int64_t>(MarkStatus::READY_TO_MARK))), &exit, &marking);

        Bind(&marking);
        CallNGCRuntime(
            glue,
            RTSTUB_ID(MarkingBarrier), { glue, obj, offset, value });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::TaggedIsBigInt(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(obj)),
                            Int32(static_cast<int32_t>(JSType::BIGINT)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedIsPropertyBox(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef type = GetObjectType(LoadHClass(obj));
        result = Int32Equal(type, Int32(static_cast<int32_t>(JSType::PROPERTY_BOX)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedIsAccessor(GateRef x)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(TaggedIsHeapObject(x), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef type = GetObjectType(LoadHClass(x));
        result = BoolOr(Int32Equal(type, Int32(static_cast<int32_t>(JSType::ACCESSOR_DATA))),
                        Int32Equal(type, Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsUtf16String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_UNCOMPRESSED));
}

GateRef StubBuilder::IsUtf8String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_COMPRESSED));
}

GateRef StubBuilder::IsInternalString(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32NotEqual(
        Int32And(len, Int32(EcmaString::STRING_INTERN_BIT)),
        Int32(0));
}

GateRef StubBuilder::IsDigit(GateRef ch)
{
    return BoolAnd(Int32LessThanOrEqual(ch, Int32('9')),
        Int32GreaterThanOrEqual(ch, Int32('0')));
}

void StubBuilder::TryToGetInteger(GateRef string, Variable *num, Label *success, Label *failed)
{
    auto env = GetEnvironment();
    Label exit(env);
    Label inRange(env);
    Label isInteger(env);

    GateRef len = GetLengthFromString(string);
    Branch(Int32LessThan(len, Int32(MAX_ELEMENT_INDEX_LEN)), &inRange, failed);
    Bind(&inRange);
    {
        Branch(IsIntegerString(string), &isInteger, failed);
        Bind(&isInteger);
        {
            GateRef integerNum = ZExtInt32ToInt64(GetRawHashFromString(string));
            num->WriteVariable(integerNum);
            Jump(success);
        }
    }
}

GateRef StubBuilder::StringToElementIndex(GateRef glue, GateRef string)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT64(), Int64(-1));
    Label greatThanZero(env);
    Label inRange(env);
    Label flattenFastPath(env);
    auto len = GetLengthFromString(string);
    Branch(Int32Equal(len, Int32(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    Branch(Int32GreaterThan(len, Int32(MAX_ELEMENT_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        Label isUtf8(env);
        GateRef isUtf16String = IsUtf16String(string);
        Branch(isUtf16String, &exit, &isUtf8);
        Bind(&isUtf8);
        {
            Label getFailed(env);
            TryToGetInteger(string, &result, &exit, &getFailed);
            Bind(&getFailed);
            DEFVARIABLE(c, VariableType::INT32(), Int32(0));
            FlatStringStubBuilder thisFlat(this);
            thisFlat.FlattenString(glue, string, &flattenFastPath);
            Bind(&flattenFastPath);
            StringInfoGateRef stringInfoGate(&thisFlat);
            GateRef dataUtf8 = GetNormalStringData(stringInfoGate);
            c = ZExtInt8ToInt32(Load(VariableType::INT8(), dataUtf8));
            Label isDigitZero(env);
            Label notDigitZero(env);
            Branch(Int32Equal(*c, Int32('0')), &isDigitZero, &notDigitZero);
            Bind(&isDigitZero);
            {
                Label lengthIsOne(env);
                Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &exit);
                Bind(&lengthIsOne);
                {
                    result = Int64(0);
                    Jump(&exit);
                }
            }
            Bind(&notDigitZero);
            {
                Label isDigit(env);
                DEFVARIABLE(i, VariableType::INT32(), Int32(1));
                DEFVARIABLE(n, VariableType::INT64(), Int64Sub(ZExtInt32ToInt64(*c), Int64('0')));
                Branch(IsDigit(*c), &isDigit, &exit);
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Bind(&isDigit);
                Branch(Int32UnsignedLessThan(*i, len), &loopHead, &afterLoop);
                LoopBegin(&loopHead);
                {
                    c = ZExtInt8ToInt32(Load(VariableType::INT8(), dataUtf8, ZExtInt32ToPtr(*i)));
                    Label isDigit2(env);
                    Label notDigit2(env);
                    Branch(IsDigit(*c), &isDigit2, &notDigit2);
                    Bind(&isDigit2);
                    {
                        // 10 means the base of digit is 10.
                        n = Int64Add(Int64Mul(*n, Int64(10)), Int64Sub(ZExtInt32ToInt64(*c), Int64('0')));
                        i = Int32Add(*i, Int32(1));
                        Branch(Int32UnsignedLessThan(*i, len), &loopEnd, &afterLoop);
                    }
                    Bind(&notDigit2);
                    Jump(&exit);
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead);
                Bind(&afterLoop);
                {
                    Label lessThanMaxIndex(env);
                    Branch(Int64LessThan(*n, Int64(JSObject::MAX_ELEMENT_INDEX)),
                           &lessThanMaxIndex, &exit);
                    Bind(&lessThanMaxIndex);
                    {
                        result = *n;
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TryToElementsIndex(GateRef glue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isKeyInt(env);
    Label notKeyInt(env);

    DEFVARIABLE(resultKey, VariableType::INT64(), Int64(-1));
    Branch(TaggedIsInt(key), &isKeyInt, &notKeyInt);
    Bind(&isKeyInt);
    {
        resultKey = GetInt64OfTInt(key);
        Jump(&exit);
    }
    Bind(&notKeyInt);
    {
        Label isString(env);
        Label notString(env);
        Branch(TaggedIsString(key), &isString, &notString);
        Bind(&isString);
        {
            resultKey = StringToElementIndex(glue, key);
            Jump(&exit);
        }
        Bind(&notString);
        {
            Label isDouble(env);
            Branch(TaggedIsDouble(key), &isDouble, &exit);
            Bind(&isDouble);
            {
                GateRef number = GetDoubleOfTDouble(key);
                GateRef integer = ChangeFloat64ToInt32(number);
                Label isEqual(env);
                Branch(DoubleEqual(number, ChangeInt32ToFloat64(integer)), &isEqual, &exit);
                Bind(&isEqual);
                {
                    resultKey = SExtInt32ToInt64(integer);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *resultKey;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LdGlobalRecord(GateRef glue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef globalRecord = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::GLOBAL_RECORD);
    GateRef recordEntry = FindEntryFromNameDictionary(glue, globalRecord, key);
    Label foundInGlobalRecord(env);
    Branch(Int32NotEqual(recordEntry, Int32(-1)), &foundInGlobalRecord, &exit);
    Bind(&foundInGlobalRecord);
    {
        result = GetBoxFromGlobalDictionary(globalRecord, recordEntry);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadFromField(GateRef receiver, GateRef handlerInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerInfoIsInlinedProps(env);
    Label handlerInfoNotInlinedProps(env);
    Label handlerPost(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef index = HandlerBaseGetOffset(handlerInfo);
    Branch(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoIsInlinedProps);
    {
        result = Load(VariableType::JS_ANY(), receiver, PtrMul(ZExtInt32ToPtr(index),
            IntPtr(JSTaggedValue::TaggedTypeSize())));
        Jump(&handlerPost);
    }
    Bind(&handlerInfoNotInlinedProps);
    {
        result = GetValueFromTaggedArray(GetPropertiesArray(receiver), index);
        Jump(&handlerPost);
    }
    Bind(&handlerPost);
    {
        Label nonDoubleToTagged(env);
        Label doubleToTagged(env);
        GateRef rep = HandlerBaseGetRep(handlerInfo);
        Branch(IsDoubleRepInPropAttr(rep), &doubleToTagged, &nonDoubleToTagged);
        Bind(&doubleToTagged);
        {
            result = TaggedPtrToTaggedDoublePtr(*result);
            Jump(&exit);
        }
        Bind(&nonDoubleToTagged);
        {
            Label intToTagged(env);
            Branch(IsIntRepInPropAttr(rep), &intToTagged, &exit);
            Bind(&intToTagged);
            {
                result = TaggedPtrToTaggedIntPtr(*result);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadGlobal(GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label cellIsInvalid(env);
    Label cellNotInvalid(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(IsInvalidPropertyBox(cell), &cellIsInvalid, &cellNotInvalid);
    Bind(&cellIsInvalid);
    {
        Jump(&exit);
    }
    Bind(&cellNotInvalid);
    {
        result = GetValueFromPropertyBox(cell);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CheckPolyHClass(GateRef cachedValue, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label iLessLength(env);
    Label hasHclass(env);
    Label cachedValueNotWeak(env);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsWeak(cachedValue), &exit, &cachedValueNotWeak);
    Bind(&cachedValueNotWeak);
    {
        GateRef length = GetLengthOfTaggedArray(cachedValue);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int32UnsignedLessThan(*i, length), &iLessLength, &exit);
            Bind(&iLessLength);
            {
                GateRef element = GetValueFromTaggedArray(cachedValue, *i);
                Branch(Equal(LoadObjectFromWeakRef(element), hclass), &hasHclass, &loopEnd);
                Bind(&hasHclass);
                result = GetValueFromTaggedArray(cachedValue,
                                                 Int32Add(*i, Int32(1)));
                Jump(&exit);
            }
            Bind(&loopEnd);
            i = Int32Add(*i, Int32(2));  // 2 means one ic, two slot
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadICWithHandler(
    GateRef glue, GateRef receiver, GateRef argHolder, GateRef argHandler, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerInfoIsNonExist(env);
    Label handlerInfoNotNonExist(env);
    Label handlerInfoIsString(env);
    Label handlerInfoNotString(env);
    Label handlerInfoIsStringLength(env);
    Label handlerInfoNotStringLength(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label cellHasChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(holder, VariableType::JS_ANY(), argHolder);
    DEFVARIABLE(handler, VariableType::JS_ANY(), argHandler);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = GetInt32OfTInt(*handler);
            Branch(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                Label receiverIsNumber(env);
                Label receiverNotNumber(env);
                Branch(TaggedIsNumber(receiver), &receiverIsNumber, &receiverNotNumber);
                Bind(&receiverIsNumber);
                {
                    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
                    auto numberProto = GetGlobalEnvValue(VariableType::JS_ANY(),
                        glueGlobalEnv, GlobalEnv::NUMBER_PROTOTYPE_INDEX);
                    result = LoadFromField(numberProto, handlerInfo);
                    Jump(&exit);
                }
                Bind(&receiverNotNumber);
                {
                    result = LoadFromField(*holder, handlerInfo);
                    Jump(&exit);
                }
            }
            Bind(&handlerInfoNotField);
            {
                Branch(IsStringElement(handlerInfo), &handlerInfoIsString, &handlerInfoNotString);
                Bind(&handlerInfoIsString);
                {
                    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
                    auto stringProto = GetGlobalEnvValue(VariableType::JS_ANY(),
                        glueGlobalEnv, GlobalEnv::STRING_PROTOTYPE_INDEX);
                    result = LoadFromField(stringProto, handlerInfo);
                    Jump(&exit);
                }
                Bind(&handlerInfoNotString);
                {
                    Branch(IsNonExist(handlerInfo), &handlerInfoIsNonExist, &handlerInfoNotNonExist);
                    Bind(&handlerInfoIsNonExist);
                    Jump(&exit);
                    Bind(&handlerInfoNotNonExist);
                    {
                        Branch(IsStringLength(handlerInfo), &handlerInfoIsStringLength, &handlerInfoNotStringLength);
                        Bind(&handlerInfoNotStringLength);
                        {
                            // string or number hasaccessor
                            Label holderIsString(env);
                            Label holderNotString(env);
                            Label holderIsNumber(env);
                            Label holderNotNumber(env);
                            Branch(TaggedIsString(*holder), &holderIsString, &holderNotString);
                            Bind(&holderIsString);
                            {
                                GateRef glueGlobalEnvOffset =
                                    IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                                GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(),
                                    glue, glueGlobalEnvOffset);
                                holder = GetGlobalEnvValue(VariableType::JS_ANY(),
                                    glueGlobalEnv, GlobalEnv::STRING_PROTOTYPE_INDEX);
                                Jump(&exit);
                            }
                            Bind(&holderNotString);
                            Branch(TaggedIsNumber(*holder), &holderIsNumber, &holderNotNumber);
                            Bind(&holderIsNumber);
                            {
                                GateRef glueGlobalEnvOffset =
                                    IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                                GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(),
                                    glue, glueGlobalEnvOffset);
                                holder = GetGlobalEnvValue(VariableType::JS_ANY(),
                                    glueGlobalEnv, GlobalEnv::NUMBER_PROTOTYPE_INDEX);
                                Jump(&exit);
                            }
                            Bind(&holderNotNumber);
                            GateRef accessor = LoadFromField(*holder, handlerInfo);
                            result = CallGetterHelper(glue, receiver, *holder, accessor, callback);
                            Jump(&exit);
                        }
                        Bind(&handlerInfoIsStringLength);
                        {
                            result = IntToTaggedPtr(GetLengthFromString(receiver));
                            Jump(&exit);
                        }
                    }
                }
            }
        }
        Bind(&handlerNotInt);
        Branch(TaggedIsPrototypeHandler(*handler), &handlerIsPrototypeHandler, &handlerNotPrototypeHandler);
        Bind(&handlerIsPrototypeHandler);
        {
            GateRef cellValue = GetProtoCell(*handler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            Bind(&cellHasChanged);
            {
                result = Hole();
                Jump(&exit);
            }
            Bind(&loopEnd);
            holder = GetPrototypeHandlerHolder(*handler);
            handler = GetPrototypeHandlerHandlerInfo(*handler);
            LoopEnd(&loopHead);
        }
    }
    Bind(&handlerNotPrototypeHandler);
    result = LoadGlobal(*handler);
    Jump(&exit);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadElement(GateRef glue, GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label lengthLessIndex(env);
    Label lengthNotLessIndex(env);
    Label greaterThanInt32Max(env);
    Label notGreaterThanInt32Max(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef index64 = TryToElementsIndex(glue, key);
    Branch(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    Branch(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        GateRef elements = GetElementsArray(receiver);
        Branch(Int32LessThanOrEqual(GetLengthOfTaggedArray(elements), index), &lengthLessIndex, &lengthNotLessIndex);
        Bind(&lengthLessIndex);
        Jump(&exit);
        Bind(&lengthNotLessIndex);
        result = GetTaggedValueWithElementsKind(receiver, index);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadStringElement(GateRef glue, GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label lengthLessIndex(env);
    Label lengthNotLessIndex(env);
    Label greaterThanInt32Max(env);
    Label notGreaterThanInt32Max(env);
    Label flattenFastPath(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef index64 = TryToElementsIndex(glue, key);
    Branch(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    Branch(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        FlatStringStubBuilder thisFlat(this);
        thisFlat.FlattenString(glue, receiver, &flattenFastPath);
        Bind(&flattenFastPath);
        Branch(Int32LessThanOrEqual(GetLengthFromString(receiver), index), &lengthLessIndex, &lengthNotLessIndex);
        Bind(&lengthLessIndex);
        Jump(&exit);
        Bind(&lengthNotLessIndex);
        BuiltinsStringStubBuilder stringBuilder(this);
        StringInfoGateRef stringInfoGate(&thisFlat);
        result = stringBuilder.CreateFromEcmaString(glue, index, stringInfoGate);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ICStoreElement(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label handlerInfoIsTypedArray(env);
    Label handerInfoNotTypedArray(env);
    Label handerInfoIsJSArray(env);
    Label handerInfoNotJSArray(env);
    Label isJsCOWArray(env);
    Label isNotJsCOWArray(env);
    Label setElementsLength(env);
    Label indexGreaterLength(env);
    Label indexGreaterCapacity(env);
    Label callRuntime(env);
    Label storeElement(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label cellHasChanged(env);
    Label cellHasNotChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label greaterThanInt32Max(env);
    Label notGreaterThanInt32Max(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(varHandler, VariableType::JS_ANY(), handler);
    GateRef index64 = TryToElementsIndex(glue, key);
    Branch(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    Branch(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        Jump(&loopHead);
        LoopBegin(&loopHead);
        Branch(TaggedIsInt(*varHandler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = GetInt32OfTInt(*varHandler);
            Branch(IsTypedArrayElement(handlerInfo), &handlerInfoIsTypedArray, &handerInfoNotTypedArray);
            Bind(&handlerInfoIsTypedArray);
            {
                GateRef hclass = LoadHClass(receiver);
                GateRef jsType = GetObjectType(hclass);
                TypedArrayStubBuilder typedArrayBuilder(this);
                result = typedArrayBuilder.StoreTypedArrayElement(glue, receiver, index64, value, jsType);
                Jump(&exit);
            }
            Bind(&handerInfoNotTypedArray);
            Branch(HandlerBaseIsJSArray(handlerInfo), &handerInfoIsJSArray, &handerInfoNotJSArray);
            Bind(&handerInfoIsJSArray);
            {
                Branch(IsJsCOWArray(receiver), &isJsCOWArray, &isNotJsCOWArray);
                Bind(&isJsCOWArray);
                {
                    CallRuntime(glue, RTSTUB_ID(CheckAndCopyArray), {receiver});
                    Jump(&setElementsLength);
                }
                Bind(&isNotJsCOWArray);
                {
                    Jump(&setElementsLength);
                }
                Bind(&setElementsLength);
                {
                    GateRef oldLength = GetArrayLength(receiver);
                    Branch(Int32GreaterThanOrEqual(index, oldLength), &indexGreaterLength, &handerInfoNotJSArray);
                    Bind(&indexGreaterLength);
                    Store(VariableType::INT32(), glue, receiver,
                        IntPtr(panda::ecmascript::JSArray::LENGTH_OFFSET),
                        Int32Add(index, Int32(1)));
                }
                Jump(&handerInfoNotJSArray);
            }
            Bind(&handerInfoNotJSArray);
            {
                GateRef elements = GetElementsArray(receiver);
                GateRef capacity = GetLengthOfTaggedArray(elements);
                Branch(Int32GreaterThanOrEqual(index, capacity), &callRuntime, &storeElement);
                Bind(&callRuntime);
                {
                    result = CallRuntime(glue,
                        RTSTUB_ID(TaggedArraySetValue),
                        { receiver, value, elements, IntToTaggedInt(index),
                          IntToTaggedInt(capacity) });
                    Label transition(env);
                    Branch(TaggedIsHole(*result), &exit, &transition);
                    Bind(&transition);
                    {
                        Label hole(env);
                        Label notHole(env);
                        DEFVARIABLE(kind, VariableType::INT32(), Int32(static_cast<int32_t>(ElementsKind::NONE)));
                        Branch(Int32GreaterThan(index, capacity), &hole, &notHole);
                        Bind(&hole);
                        {
                            kind = Int32(static_cast<int32_t>(ElementsKind::HOLE));
                            Jump(&notHole);
                        }
                        Bind(&notHole);
                        {
                            SetValueWithElementsKind(glue, receiver, value, index, Boolean(true), *kind);
                            Jump(&exit);
                        }
                    }
                }
                Bind(&storeElement);
                {
                    SetValueWithElementsKind(glue, receiver, value, index, Boolean(true),
                                             Int32(static_cast<int32_t>(ElementsKind::NONE)));
                    result = Undefined();
                    Jump(&exit);
                }
            }
        }
        Bind(&handlerNotInt);
        {
            GateRef cellValue = GetProtoCell(*varHandler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            Bind(&cellHasChanged);
            {
                Jump(&exit);
            }
            Bind(&loopEnd);
            {
                varHandler = GetPrototypeHandlerHandlerInfo(*varHandler);
                LoopEnd(&loopHead);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetArrayLength(GateRef object)
{
    GateRef lengthOffset = IntPtr(panda::ecmascript::JSArray::LENGTH_OFFSET);
    GateRef result = Load(VariableType::INT32(), object, lengthOffset);
    return result;
}

void StubBuilder::SetArrayLength(GateRef glue, GateRef object, GateRef len)
{
    GateRef lengthOffset = IntPtr(panda::ecmascript::JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, object, lengthOffset, len);
}

GateRef StubBuilder::StoreICWithHandler(GateRef glue, GateRef receiver, GateRef argHolder,
                                        GateRef value, GateRef argHandler, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerIsTransitionHandler(env);
    Label handlerNotTransitionHandler(env);
    Label handlerIsTransWithProtoHandler(env);
    Label handlerNotTransWithProtoHandler(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label handlerIsPropertyBox(env);
    Label handlerNotPropertyBox(env);
    Label handlerIsStoreTSHandler(env);
    Label handlerNotStoreTSHandler(env);
    Label aotHandlerInfoIsField(env);
    Label aotHandlerInfoNotField(env);
    Label cellHasChanged(env);
    Label cellNotChanged(env);
    Label aotCellNotChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label cellNotNull(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(holder, VariableType::JS_ANY(), argHolder);
    DEFVARIABLE(handler, VariableType::JS_ANY(), argHandler);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = GetInt32OfTInt(*handler);
            Branch(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                result = StoreField(glue, receiver, value, handlerInfo, callback);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                GateRef accessor = LoadFromField(*holder, handlerInfo);
                result = CallSetterHelper(glue, receiver, accessor, value, callback);
                Jump(&exit);
            }
        }
        Bind(&handlerNotInt);
        {
            Branch(TaggedIsTransitionHandler(*handler), &handlerIsTransitionHandler, &handlerNotTransitionHandler);
            Bind(&handlerIsTransitionHandler);
            {
                result = StoreWithTransition(glue, receiver, value, *handler, callback);
                Jump(&exit);
            }
            Bind(&handlerNotTransitionHandler);
            {
                Branch(TaggedIsTransWithProtoHandler(*handler), &handlerIsTransWithProtoHandler,
                    &handlerNotTransWithProtoHandler);
                Bind(&handlerIsTransWithProtoHandler);
                {
                    GateRef cellValue = GetProtoCell(*handler);
                    Branch(GetHasChanged(cellValue), &cellHasChanged, &cellNotChanged);
                    Bind(&cellNotChanged);
                    {
                        result = StoreWithTransition(glue, receiver, value, *handler, callback, true);
                        Jump(&exit);
                    }
                }
                Bind(&handlerNotTransWithProtoHandler);
                {
                    Branch(TaggedIsPrototypeHandler(*handler), &handlerIsPrototypeHandler, &handlerNotPrototypeHandler);
                    Bind(&handlerNotPrototypeHandler);
                    {
                        Branch(TaggedIsPropertyBox(*handler), &handlerIsPropertyBox, &handlerNotPropertyBox);
                        Bind(&handlerIsPropertyBox);
                        StoreGlobal(glue, value, *handler);
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&handlerIsPrototypeHandler);
        {
            GateRef cellValue = GetProtoCell(*handler);
            Branch(TaggedIsNull(cellValue), &cellHasChanged, &cellNotNull);
            Bind(&cellNotNull);
            {
                Branch(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            }
            Bind(&loopEnd);
            {
                holder = GetPrototypeHandlerHolder(*handler);
                handler = GetPrototypeHandlerHandlerInfo(*handler);
                LoopEnd(&loopHead);
            }
        }
        Bind(&handlerNotPropertyBox);
        {
            Branch(TaggedIsStoreTSHandler(*handler), &handlerIsStoreTSHandler, &handlerNotStoreTSHandler);
            Bind(&handlerIsStoreTSHandler);
            {
                GateRef cellValue = GetProtoCell(*handler);
                Branch(GetHasChanged(cellValue), &cellHasChanged, &aotCellNotChanged);
                Bind(&aotCellNotChanged);
                {
                    holder = GetStoreTSHandlerHolder(*handler);
                    handler = GetStoreTSHandlerHandlerInfo(*handler);
                    GateRef handlerInfo = GetInt32OfTInt(*handler);
                    Branch(IsField(handlerInfo), &aotHandlerInfoIsField, &aotHandlerInfoNotField);
                    Bind(&aotHandlerInfoIsField);
                    {
                        result = StoreField(glue, receiver, value, handlerInfo, callback);
                        Jump(&exit);
                    }
                    Bind(&aotHandlerInfoNotField);
                    {
                        GateRef accessor = LoadFromField(*holder, handlerInfo);
                        result = CallSetterHelper(glue, receiver, accessor, value, callback);
                        Jump(&exit);
                    }
                }
            }
            Bind(&handlerNotStoreTSHandler);
            Jump(&exit);
        }
        Bind(&cellHasChanged);
        {
            result = Hole();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler,
    ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    ProfilerStubBuilder(env).UpdatePropAttrIC(glue, receiver, value, handler, callback);
    Label exit(env);
    Label handlerIsInlinedProperty(env);
    Label handlerNotInlinedProperty(env);
    GateRef index = HandlerBaseGetOffset(handler);
    GateRef rep = HandlerBaseGetRep(handler);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label repChange(env);
    Branch(HandlerBaseIsInlinedProperty(handler), &handlerIsInlinedProperty, &handlerNotInlinedProperty);
    Bind(&handlerIsInlinedProperty);
    {
        GateRef toOffset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
        SetValueWithRep(glue, receiver, toOffset, value, rep, &repChange);
        Jump(&exit);
    }
    Bind(&handlerNotInlinedProperty);
    {
        GateRef array = GetPropertiesArray(receiver);
        SetValueToTaggedArrayWithRep(glue, array, index, value, rep, &repChange);
        Jump(&exit);
    }
    Bind(&repChange);
    {
        result = Hole();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler,
                                         ProfileOperation callback, bool withPrototype)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label handlerInfoIsInlinedProps(env);
    Label handlerInfoNotInlinedProps(env);
    Label indexMoreCapacity(env);
    Label indexLessCapacity(env);
    Label capacityIsZero(env);
    Label capacityNotZero(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef newHClass;
    GateRef handlerInfo;
    if (withPrototype) {
        newHClass = GetTransWithProtoHClass(handler);
        handlerInfo = GetInt32OfTInt(GetTransWithProtoHandlerInfo(handler));
    } else {
        newHClass = GetTransitionHClass(handler);
        handlerInfo = GetInt32OfTInt(GetTransitionHandlerInfo(handler));
    }

    GateRef oldHClass = LoadHClass(receiver);
    GateRef prototype = GetPrototypeFromHClass(oldHClass);
    StorePrototype(glue, newHClass, prototype);
    StoreHClass(glue, receiver, newHClass);
    // Because we currently only supports Fast ElementsKind
    CallRuntime(glue, RTSTUB_ID(TryRestoreElementsKind), { receiver, newHClass });
    Branch(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoNotInlinedProps);
    {
        ProfilerStubBuilder(env).UpdatePropAttrIC(glue, receiver, value, handlerInfo, callback);
        Label repChange(env);
        GateRef array = GetPropertiesArray(receiver);
        GateRef capacity = GetLengthOfTaggedArray(array);
        GateRef index = HandlerBaseGetOffset(handlerInfo);
        Branch(Int32GreaterThanOrEqual(index, capacity), &indexMoreCapacity, &indexLessCapacity);
        Bind(&indexMoreCapacity);
        {
            NewObjectStubBuilder newBuilder(this);
            Branch(Int32Equal(capacity, Int32(0)), &capacityIsZero, &capacityNotZero);
            Bind(&capacityIsZero);
            {
                GateRef properties = newBuilder.NewTaggedArray(glue, Int32(JSObject::MIN_PROPERTIES_LENGTH));
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, properties, index, value);
                SetPropertiesArray(VariableType::JS_POINTER(), glue, receiver, properties);
                Jump(&exit);
            }
            Bind(&capacityNotZero);
            {
                GateRef inlinedProperties = GetInlinedPropertiesFromHClass(newHClass);
                GateRef maxNonInlinedFastPropsCapacity =
                                Int32Sub(Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY), inlinedProperties);
                GateRef newLen = ComputeNonInlinedFastPropsCapacity(glue, capacity, maxNonInlinedFastPropsCapacity);
                GateRef properties = newBuilder.CopyArray(glue, array, capacity, newLen);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, properties, index, value);
                SetPropertiesArray(VariableType::JS_POINTER(), glue, receiver, properties);
                Jump(&exit);
            }
        }
        Bind(&indexLessCapacity);
        {
            GateRef rep = HandlerBaseGetRep(handlerInfo);
            GateRef base = PtrAdd(array, IntPtr(TaggedArray::DATA_OFFSET));
            GateRef toIndex = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
            SetValueWithRep(glue, base, toIndex, value, rep, &repChange);
            Jump(&exit);
        }
        Bind(&repChange);
        {
            result = Hole();
            Jump(&exit);
        }
    }
    Bind(&handlerInfoIsInlinedProps);
    {
        result = StoreField(glue, receiver, value, handlerInfo, callback);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::StoreGlobal(GateRef glue, GateRef value, GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label cellIsInvalid(env);
    Label cellNotInvalid(env);
    Label cellIsAccessorData(env);
    Label cellIsNotAccessorData(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(IsInvalidPropertyBox(cell), &cellIsInvalid, &cellNotInvalid);
    Bind(&cellIsInvalid);
    {
        Jump(&exit);
    }
    Bind(&cellNotInvalid);
    Branch(IsAccessorPropertyBox(cell), &cellIsAccessorData, &cellIsNotAccessorData);
    Bind(&cellIsAccessorData);
    {
        Jump(&exit);
    }
    Bind(&cellIsNotAccessorData);
    {
        Store(VariableType::JS_ANY(), glue, cell, IntPtr(PropertyBox::VALUE_OFFSET), value);
        result = Undefined();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<typename DictionaryT>
GateRef StubBuilder::GetAttributesFromDictionary(GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
    Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
             Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef attributesIndex =
        Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_DETAILS_INDEX));
    auto attrValue = GetValueFromTaggedArray(elements, attributesIndex);
    return GetInt32OfTInt(attrValue);
}

template<typename DictionaryT>
GateRef StubBuilder::GetValueFromDictionary(GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
        Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_VALUE_INDEX));
    return GetValueFromTaggedArray(elements, valueIndex);
}

template<typename DictionaryT>
GateRef StubBuilder::GetKeyFromDictionary(GateRef elements, GateRef entry)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label ltZero(env);
    Label notLtZero(env);
    Label gtLength(env);
    Label notGtLength(env);
    GateRef dictionaryLength =
        Load(VariableType::INT32(), elements, IntPtr(TaggedArray::LENGTH_OFFSET));
    GateRef arrayIndex =
        Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    Branch(Int32LessThan(arrayIndex, Int32(0)), &ltZero, &notLtZero);
    Bind(&ltZero);
    Jump(&exit);
    Bind(&notLtZero);
    Branch(Int32GreaterThan(arrayIndex, dictionaryLength), &gtLength, &notGtLength);
    Bind(&gtLength);
    Jump(&exit);
    Bind(&notGtLength);
    result = GetValueFromTaggedArray(elements, arrayIndex);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

inline void StubBuilder::UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index,
                                                  GateRef value, GateRef attr)
{
    GateRef arrayIndex =
        Int32Add(Int32(NameDictionary::TABLE_HEADER_SIZE),
                 Int32Mul(index, Int32(NameDictionary::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, Int32(NameDictionary::ENTRY_VALUE_INDEX));
    GateRef attributesIndex =
        Int32Add(arrayIndex, Int32(NameDictionary::ENTRY_DETAILS_INDEX));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, valueIndex, value);
    GateRef attroffset =
        PtrMul(ZExtInt32ToPtr(attributesIndex), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(attroffset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(VariableType::INT64(), glue, elements, dataOffset, IntToTaggedInt(attr));
}

template<typename DictionaryT>
inline void StubBuilder::UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value)
{
    GateRef arrayIndex = Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
        Int32Mul(index, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef valueIndex = Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_VALUE_INDEX));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, valueIndex, value);
}

GateRef StubBuilder::GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSpecialIndexed(env);
        Label notSpecialIndexed(env);
        Branch(IsSpecialIndexedObj(jsType), &isSpecialIndexed, &notSpecialIndexed);
        Bind(&isSpecialIndexed);
        {
            // TypeArray
            Label isFastTypeArray(env);
            Label notFastTypeArray(env);
            Label notTypedArrayProto(env);
            Branch(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY))), &exit, &notTypedArrayProto);
            Bind(&notTypedArrayProto);
            Branch(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
            Bind(&isFastTypeArray);
            {
                TypedArrayStubBuilder typedArrayStubBuilder(this);
                result = typedArrayStubBuilder.FastGetPropertyByIndex(glue, *holder, index, jsType);
                Jump(&exit);
            }
            Bind(&notFastTypeArray);

            Label isSpecialContainer(env);
            Label notSpecialContainer(env);
            // Add SpecialContainer
            Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
            Bind(&isSpecialContainer);
            {
                result = GetContainerProperty(glue, *holder, index, jsType);
                Jump(&exit);
            }
            Bind(&notSpecialContainer);
            {
                result = Hole();
                Jump(&exit);
            }
        }
        Bind(&notSpecialIndexed);
        {
            GateRef elements = GetElementsArray(*holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                Branch(Int32UnsignedLessThan(index, GetLengthOfTaggedArray(elements)),
                       &lessThanLength, &notLessThanLength);
                Bind(&lessThanLength);
                {
                    DEFVARIABLE(value, VariableType::JS_ANY(), Hole());
                    Label notHole(env);
                    Label isHole(env);
                    value = GetTaggedValueWithElementsKind(*holder, index);
                    Branch(TaggedIsNotHole(*value), &notHole, &isHole);
                    Bind(&notHole);
                    {
                        result = *value;
                        Jump(&exit);
                    }
                    Bind(&isHole);
                    {
                        Jump(&loopExit);
                    }
                }
                Bind(&notLessThanLength);
                {
                    Jump(&loopExit);
                }
            }
            Bind(&isDictionaryElement);
            {
                GateRef entryA = FindElementFromNumberDictionary(glue, elements, index);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                Branch(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    GateRef attr = GetAttributesFromDictionary<NumberDictionary>(elements, entryA);
                    GateRef value = GetValueFromDictionary<NumberDictionary>(elements, entryA);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value, callback);
                        Jump(&exit);
                    }
                    Bind(&notAccessor);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetPropertyByValue(GateRef glue, GateRef receiver, GateRef keyValue, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(key, VariableType::JS_ANY(), keyValue);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(isInternal, VariableType::BOOL(), True());
    Label isNumberOrStringSymbol(env);
    Label notNumber(env);
    Label isStringOrSymbol(env);
    Label notStringOrSymbol(env);
    Label exit(env);

    Branch(TaggedIsNumber(*key), &isNumberOrStringSymbol, &notNumber);
    Bind(&notNumber);
    {
        Branch(TaggedIsStringOrSymbol(*key), &isNumberOrStringSymbol, &notStringOrSymbol);
        Bind(&notStringOrSymbol);
        {
            result = Hole();
            Jump(&exit);
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        GateRef index64 = TryToElementsIndex(glue, *key);
        Label validIndex(env);
        Label notValidIndex(env);
        Label greaterThanInt32Max(env);
        Label notGreaterThanInt32Max(env);
        Branch(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
        Bind(&greaterThanInt32Max);
        {
            Jump(&exit);
        }
        Bind(&notGreaterThanInt32Max);
        GateRef index = TruncInt64ToInt32(index64);
        Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = GetPropertyByIndex(glue, receiver, index, callback);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label notNumber1(env);
            Label getByName(env);
            Branch(TaggedIsNumber(*key), &exit, &notNumber1);
            Bind(&notNumber1);
            {
                Label isString(env);
                Label notString(env);
                Label isInternalString(env);
                Label notIntenalString(env);
                Branch(TaggedIsString(*key), &isString, &notString);
                Bind(&isString);
                {
                    Branch(IsInternalString(*key), &isInternalString, &notIntenalString);
                    Bind(&isInternalString);
                    Jump(&getByName);
                    Bind(&notIntenalString);
                    {
                        Label notFind(env);
                        Label find(env);
                        // if key can't find in stringtabele, key is not propertyname for a object
                        GateRef res = CallNGCRuntime(glue, RTSTUB_ID(TryGetInternString), { glue, *key });
                        Branch(TaggedIsHole(res), &notFind, &find);
                        Bind(&notFind);
                        {
                            isInternal = False();
                            Jump(&getByName);
                        }
                        Bind(&find);
                        {
                            key = res;
                            Jump(&getByName);
                        }
                    }
                }
                Bind(&notString);
                {
                    Jump(&getByName);
                }
            }
            Bind(&getByName);
            {
                result = GetPropertyByName(glue, receiver, *key, callback, *isInternal, true);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
                                       ProfileOperation callback, GateRef isInternal, bool canUseIsInternal)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Label findProperty(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSIndexObj(env);
        Label notSIndexObj(env);
        Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
        Bind(&isSIndexObj);
        {
            // TypeArray
            Label isFastTypeArray(env);
            Label notFastTypeArray(env);
            Branch(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
            Bind(&isFastTypeArray);
            {
                result = GetTypeArrayPropertyByName(glue, receiver, *holder, key, jsType);
                Label isNull(env);
                Label notNull(env);
                Branch(TaggedIsNull(*result), &isNull, &notNull);
                Bind(&isNull);
                {
                    result = Hole();
                    Jump(&exit);
                }
                Bind(&notNull);
                Branch(TaggedIsHole(*result), &notSIndexObj, &exit);
            }
            Bind(&notFastTypeArray);
            {
                result = Hole();
                Jump(&exit);
            }
        }
        Bind(&notSIndexObj);
        {
            if (canUseIsInternal) {
                Branch(isInternal, &findProperty, &loopExit);
            } else {
                Jump(&findProperty);
            }
            Label isDicMode(env);
            Label notDicMode(env);
            Bind(&findProperty);
            Branch(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                GateRef layOutInfo = GetLayoutFromHClass(hclass);
                GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
                // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                GateRef entryA = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum);
                Label hasEntry(env);
                Label noEntry(env);
                // if branch condition : entry != -1
                Branch(Int32NotEqual(entryA, Int32(-1)), &hasEntry, &noEntry);
                Bind(&hasEntry);
                {
                    // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                    GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entryA);
                    GateRef attr = GetInt32OfTInt(propAttr);
                    GateRef value = JSObjectGetProperty(*holder, hclass, attr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value, callback);
                        Jump(&exit);
                    }
                    Bind(&notAccessor);
                    {
                        Label notHole(env);
                        Branch(TaggedIsHole(value), &noEntry, &notHole);
                        Bind(&notHole);
                        {
                            result = value;
                            Jump(&exit);
                        }
                    }
                }
                Bind(&noEntry);
                {
                    Jump(&loopExit);
                }
            }
            Bind(&isDicMode);
            {
                GateRef array = GetPropertiesArray(*holder);
                // int entry = dict->FindEntry(key)
                GateRef entryB = FindEntryFromNameDictionary(glue, array, key);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                // if branch condition : entry != -1
                Branch(Int32NotEqual(entryB, Int32(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    // auto value = dict->GetValue(entry)
                    GateRef attr = GetAttributesFromDictionary<NameDictionary>(array, entryB);
                    // auto attr = dict->GetAttributes(entry)
                    GateRef value = GetValueFromDictionary<NameDictionary>(array, entryB);
                    Label isAccessor1(env);
                    Label notAccessor1(env);
                    Branch(IsAccessor(attr), &isAccessor1, &notAccessor1);
                    Bind(&isAccessor1);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value, callback);
                        Jump(&exit);
                    }
                    Bind(&notAccessor1);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef srcHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label isTS(env);
    Label isNotTS(env);
    env->SubCfgEntry(&entry);
    auto proto = GetPrototypeFromHClass(srcHClass);
    SetPrototypeToHClass(VariableType::JS_POINTER(), glue, dstHClass, proto);
    SetBitFieldToHClass(glue, dstHClass, GetBitFieldFromHClass(srcHClass));
    SetIsAllTaggedProp(glue, dstHClass, GetIsAllTaggedPropFromHClass(srcHClass));
    SetNumberOfPropsToHClass(glue, dstHClass, GetNumberOfPropsFromHClass(srcHClass));
    SetTransitionsToHClass(VariableType::INT64(), glue, dstHClass, Undefined());
    SetParentToHClass(VariableType::INT64(), glue, dstHClass, Undefined());
    SetProtoChangeDetailsToHClass(VariableType::INT64(), glue, dstHClass, Null());
    SetEnumCacheToHClass(VariableType::INT64(), glue, dstHClass, Null());
    SetLayoutToHClass(VariableType::JS_POINTER(), glue, dstHClass, GetLayoutFromHClass(srcHClass));
    Branch(IsTSHClass(srcHClass), &isTS, &isNotTS);
    Bind(&isTS);
    {
        SetIsTS(glue, dstHClass, False());
        Jump(&isNotTS);
    }
    Bind(&isNotTS);
    env->SubCfgExit();
    return;
}

void StubBuilder::TransitionForRepChange(GateRef glue, GateRef receiver, GateRef key, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    GateRef hclass = LoadHClass(receiver);
    GateRef type = GetObjectType(hclass);
    GateRef size = Int32Mul(GetInlinedPropsStartFromHClass(hclass),
                            Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
    GateRef newJshclass = CallRuntime(glue, RTSTUB_ID(NewEcmaHClass),
        { IntToTaggedInt(size), IntToTaggedInt(type),
          IntToTaggedInt(inlineProps) });
    CopyAllHClass(glue, newJshclass, hclass);
    CallRuntime(glue, RTSTUB_ID(CopyAndUpdateObjLayout),
                { hclass, newJshclass, key, IntToTaggedInt(attr) });
#if ECMASCRIPT_ENABLE_IC
    NotifyHClassChanged(glue, hclass, newJshclass);
#endif
    StoreHClass(glue, receiver, newJshclass);
    // Because we currently only supports Fast ElementsKind
    CallRuntime(glue, RTSTUB_ID(TryRestoreElementsKind), { receiver, newJshclass });
    env->SubCfgExit();
}

void StubBuilder::TransitToElementsKind(GateRef glue, GateRef receiver, GateRef value, GateRef kind)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    GateRef hclass = LoadHClass(receiver);
    GateRef elementsKind = GetElementsKindFromHClass(hclass);

    Label isNoneDefault(env);
    Branch(Int32Equal(elementsKind, Int32(static_cast<int32_t>(ElementsKind::GENERIC))), &exit, &isNoneDefault);
    Bind(&isNoneDefault);
    {
        GateRef newKind = TaggedToElementKind(value);
        newKind = Int32Or(newKind, kind);
        newKind = Int32Or(newKind, elementsKind);
        Label change(env);
        Branch(Int32Equal(elementsKind, newKind), &exit, &change);
        Bind(&change);
        {
            CallRuntime(glue, RTSTUB_ID(UpdateHClassForElementsKind), { receiver, newKind });
            CallRuntime(glue, RTSTUB_ID(MigrateArrayWithKind),
                        { receiver, IntToTaggedInt(elementsKind), IntToTaggedInt(newKind) });
            Jump(&exit);
        }
    }

    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::AddElementInternal(GateRef glue, GateRef receiver, GateRef index, GateRef value, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(kind, VariableType::INT32(), Int32(static_cast<int32_t>(ElementsKind::NONE)));
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isArray(env);
    Label notArray(env);
    Branch(IsJsArray(receiver), &isArray, &notArray);
    Bind(&isArray);
    {
        GateRef oldLen = GetArrayLength(receiver);
        Label indexGreaterOrEq(env);
        Branch(Int32GreaterThanOrEqual(index, oldLen), &indexGreaterOrEq, &notArray);
        Bind(&indexGreaterOrEq);
        {
            Label isArrLenWritable(env);
            Label notArrLenWritable(env);
            Branch(IsArrayLengthWritable(glue, receiver), &isArrLenWritable, &notArrLenWritable);
            Bind(&isArrLenWritable);
            {
                SetArrayLength(glue, receiver, Int32Add(index, Int32(1)));
                Label indexGreater(env);
                Branch(Int32GreaterThan(index, oldLen), &indexGreater, &notArray);
                Bind(&indexGreater);
                kind = Int32(static_cast<int32_t>(ElementsKind::HOLE));
                Jump(&notArray);
            }
            Bind(&notArrLenWritable);
            result = False();
            Jump(&exit);
        }
    }
    Bind(&notArray);
    {
        NotifyStableArrayElementsGuardians(glue, receiver);
        GateRef hclass = LoadHClass(receiver);
        GateRef elements = GetElementsArray(receiver);
        Label isDicMode(env);
        Label notDicMode(env);
        Branch(IsDictionaryElement(hclass), &isDicMode, &notDicMode);
        Bind(&isDicMode);
        {
            GateRef res = CallRuntime(glue, RTSTUB_ID(NumberDictionaryPut),
                { receiver, elements, IntToTaggedInt(index), value, IntToTaggedInt(attr), TaggedFalse() });
            SetElementsArray(VariableType::JS_POINTER(), glue, receiver, res);
            result = True();
            Jump(&exit);
        }
        Bind(&notDicMode);
        {
            GateRef capacity = GetLengthOfTaggedArray(elements);
            GateRef notDefault = BoolNot(IsDefaultAttribute(attr));
            Label indexGreaterLen(env);
            Label notGreaterLen(env);
            Branch(BoolOr(Int32GreaterThanOrEqual(index, capacity), notDefault), &indexGreaterLen, &notGreaterLen);
            Bind(&indexGreaterLen);
            {
                Label isTransToDict(env);
                Label notTransToDict(env);
                Branch(BoolOr(ShouldTransToDict(capacity, index), notDefault), &isTransToDict, &notTransToDict);
                Bind(&isTransToDict);
                {
                    GateRef res = CallRuntime(glue, RTSTUB_ID(NumberDictionaryPut),
                        { receiver, elements, IntToTaggedInt(index), value, IntToTaggedInt(attr), TaggedTrue() });
                    SetElementsArray(VariableType::JS_POINTER(), glue, receiver, res);
                    result = True();
                    Jump(&exit);
                }
                Bind(&notTransToDict);
                {
                    GrowElementsCapacity(glue, receiver, Int32Add(index, Int32(1)));
                    SetValueWithElementsKind(glue, receiver, value, index, Boolean(true), *kind);
                    result = True();
                    Jump(&exit);
                }
            }
            Bind(&notGreaterLen);
            {
                SetValueWithElementsKind(glue, receiver, value, index, Boolean(true), *kind);
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

GateRef StubBuilder::GrowElementsCapacity(GateRef glue, GateRef receiver, GateRef capacity)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(newElements, VariableType::JS_ANY(), Hole());
    NewObjectStubBuilder newBuilder(this);
    GateRef newCapacity = ComputeElementCapacity(capacity);
    GateRef elements = GetElementsArray(receiver);
    newElements = newBuilder.CopyArray(glue, elements, capacity, newCapacity);
    SetElementsArray(VariableType::JS_POINTER(), glue, receiver, *newElements);
    auto ret = *newElements;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ShouldTransToDict(GateRef capacity, GateRef index)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    Label isGreaterThanCapcity(env);
    Label notGreaterThanCapcity(env);
    Branch(Int32GreaterThanOrEqual(index, capacity), &isGreaterThanCapcity, &notGreaterThanCapcity);
    Bind(&isGreaterThanCapcity);
    {
        Label isLessThanMax(env);
        Label notLessThanMax(env);
        Branch(Int32LessThanOrEqual(Int32Sub(index, capacity),
                                    Int32(JSObject::MAX_GAP)), &isLessThanMax, &notLessThanMax);
        Bind(&isLessThanMax);
        {
            Label isLessThanInt32Max(env);
            Label notLessThanInt32Max(env);
            Branch(Int32LessThan(index, Int32(INT32_MAX)), &isLessThanInt32Max, &notLessThanInt32Max);
            Bind(&isLessThanInt32Max);
            {
                Label isLessThanMin(env);
                Label notLessThanMin(env);
                Branch(Int32LessThan(capacity, Int32(JSObject::MIN_GAP)), &isLessThanMin, &notLessThanMin);
                Bind(&isLessThanMin);
                {
                    result = False();
                    Jump(&exit);
                }
                Bind(&notLessThanMin);
                {
                    result = Int32GreaterThan(index, Int32Mul(capacity, Int32(JSObject::FAST_ELEMENTS_FACTOR)));
                    Jump(&exit);
                }
            }
            Bind(&notLessThanInt32Max);
            {
                result = True();
                Jump(&exit);
            }
        }
        Bind(&notLessThanMax);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&notGreaterThanCapcity);
    {
        result = False();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::NotifyStableArrayElementsGuardians(GateRef glue, GateRef receiver)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    GateRef guardiansOffset =
                IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(env->Is32Bit()));
    GateRef guardians = Load(VariableType::BOOL(), glue, guardiansOffset);
    Label isGuardians(env);
    Branch(Equal(guardians, True()), &isGuardians, &exit);
    Bind(&isGuardians);
    {
        GateRef hclass = LoadHClass(receiver);
        Label isProtoType(env);
        Branch(BoolOr(IsProtoTypeHClass(hclass), IsJsArray(receiver)), &isProtoType, &exit);
        Bind(&isProtoType);
        {
            Label isEnvProtoType(env);
            GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
            GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
            GateRef objectFunctionPrototype = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                                GlobalEnv::OBJECT_FUNCTION_PROTOTYPE_INDEX);
            GateRef arrayPrototype = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                       GlobalEnv::ARRAY_PROTOTYPE_INDEX);
            Branch(BoolOr(Equal(objectFunctionPrototype, receiver), Equal(arrayPrototype, receiver)),
                &isEnvProtoType, &exit);
            Bind(&isEnvProtoType);
            Store(VariableType::BOOL(), glue, glue, guardiansOffset, False());
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef StubBuilder::IsArrayLengthWritable(GateRef glue, GateRef receiver)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    GateRef hclass = LoadHClass(receiver);
    Label isDicMode(env);
    Label notDicMode(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
    Bind(&isDicMode);
    {
        GateRef array = GetPropertiesArray(receiver);
        GateRef lengthString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::LENGTH_STRING_INDEX);
        GateRef entry = FindEntryFromNameDictionary(glue, array, lengthString);
        Label notNegtiveOne(env);
        Label isNegtiveOne(env);
        Branch(Int32NotEqual(entry, Int32(-1)), &notNegtiveOne, &isNegtiveOne);
        Bind(&notNegtiveOne);
        {
            GateRef attr = GetAttributesFromDictionary<NameDictionary>(array, entry);
            result = IsWritable(attr);
            Jump(&exit);
        }
        Bind(&isNegtiveOne);
        {
            GateRef attr1 = Int32(PropertyAttributes::GetDefaultAttributes());
            result = IsWritable(attr1);
            Jump(&exit);
        }
    }
    Bind(&notDicMode);
    {
        GateRef layoutInfo = GetLayoutFromHClass(hclass);
        GateRef propAttr = GetPropAttrFromLayoutInfo(layoutInfo, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
        GateRef attr = GetInt32OfTInt(propAttr);
        result = IsWritable(attr);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FindTransitions(GateRef glue, GateRef receiver, GateRef hclass, GateRef key, GateRef metaData)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    GateRef transitionOffset = IntPtr(JSHClass::TRANSTIONS_OFFSET);
    GateRef transition = Load(VariableType::JS_POINTER(), hclass, transitionOffset);
    DEFVARIABLE(result, VariableType::JS_ANY(), transition);

    Label notUndefined(env);
    Branch(Equal(transition, Undefined()), &exit, &notUndefined);
    Bind(&notUndefined);
    {
        Label isWeak(env);
        Label notWeak(env);
        Branch(TaggedIsWeak(transition), &isWeak, &notWeak);
        Bind(&isWeak);
        {
            GateRef transitionHClass = LoadObjectFromWeakRef(transition);
            GateRef propNums = GetNumberOfPropsFromHClass(transitionHClass);
            GateRef last = Int32Sub(propNums, Int32(1));
            GateRef layoutInfo = GetLayoutFromHClass(transitionHClass);
            GateRef cachedKey = GetKeyFromLayoutInfo(layoutInfo, last);
            GateRef cachedAttr = GetInt32OfTInt(GetPropAttrFromLayoutInfo(layoutInfo, last));
            GateRef cachedMetaData = GetPropertyMetaDataFromAttr(cachedAttr);
            Label keyMatch(env);
            Label isMatch(env);
            Label notMatch(env);
            Branch(Equal(cachedKey, key), &keyMatch, &notMatch);
            Bind(&keyMatch);
            {
                Branch(Int32Equal(metaData, cachedMetaData), &isMatch, &notMatch);
                Bind(&isMatch);
                {
                    GateRef oldHClass = LoadHClass(receiver);
                    GateRef prototype = GetPrototypeFromHClass(oldHClass);
                    StorePrototype(glue, transitionHClass, prototype);
#if ECMASCRIPT_ENABLE_IC
                    NotifyHClassChanged(glue, hclass, transitionHClass);
#endif
                    StoreHClass(glue, receiver, transitionHClass);
                    // Because we currently only supports Fast ElementsKind
                    CallRuntime(glue, RTSTUB_ID(TryRestoreElementsKind), { receiver, transitionHClass });
                    Jump(&exit);
                }
            }
            Bind(&notMatch);
            {
                result = Undefined();
                Jump(&exit);
            }
        }
        Bind(&notWeak);
        {
            // need to find from dictionary
            GateRef entryA = FindEntryFromTransitionDictionary(glue, transition, key, metaData);
            Label isFound(env);
            Label notFound(env);
            Branch(Int32NotEqual(entryA, Int32(-1)), &isFound, &notFound);
            Bind(&isFound);
            auto value = GetValueFromDictionary<TransitionsDictionary>(transition, entryA);
            Label valueUndefined(env);
            Label valueNotUndefined(env);
            Branch(Int64NotEqual(value, Undefined()), &valueNotUndefined,
                &valueUndefined);
            Bind(&valueNotUndefined);
            {
                GateRef newHClass = LoadObjectFromWeakRef(value);
                result = newHClass;
                GateRef oldHClass = LoadHClass(receiver);
                GateRef prototype = GetPrototypeFromHClass(oldHClass);
                StorePrototype(glue, newHClass, prototype);
#if ECMASCRIPT_ENABLE_IC
                NotifyHClassChanged(glue, hclass, newHClass);
#endif
                StoreHClass(glue, receiver, newHClass);
                // Because we currently only supports Fast ElementsKind
                CallRuntime(glue, RTSTUB_ID(TryRestoreElementsKind), { receiver, newHClass });
                Jump(&exit);
                Bind(&notFound);
                result = Undefined();
                Jump(&exit);
            }
            Bind(&valueUndefined);
            {
                result = Undefined();
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value, bool useOwn,
    ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(returnValue, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label exit(env);
    Label ifEnd(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Label isJsCOWArray(env);
    Label isNotJsCOWArray(env);
    Label setElementsArray(env);
    if (!useOwn) {
        Jump(&loopHead);
        LoopBegin(&loopHead);
    }
    GateRef hclass = LoadHClass(*holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSpecialIndex(env);
    Label notSpecialIndex(env);
    Branch(IsSpecialIndexedObj(jsType), &isSpecialIndex, &notSpecialIndex);
    Bind(&isSpecialIndex);
    {
        // TypeArray
        Label isFastTypeArray(env);
        Label notFastTypeArray(env);
        Label checkIsOnPrototypeChain(env);
        Label notTypedArrayProto(env);
        Branch(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY))), &exit, &notTypedArrayProto);
        Bind(&notTypedArrayProto);
        Branch(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
        Bind(&isFastTypeArray);
        {
            Branch(Equal(*holder, receiver), &checkIsOnPrototypeChain, &exit);
            Bind(&checkIsOnPrototypeChain);
            {
                returnValue = CallRuntime(glue, RTSTUB_ID(SetTypeArrayPropertyByIndex),
                    { receiver, IntToTaggedInt(index), value, IntToTaggedInt(jsType)});
                Jump(&exit);
            }
        }
        Bind(&notFastTypeArray);
        returnValue = Hole();
        Jump(&exit);
    }
    Bind(&notSpecialIndex);
    {
        GateRef elements = GetElementsArray(*holder);
        Label isDictionaryElement(env);
        Label notDictionaryElement(env);
        Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
        Bind(&notDictionaryElement);
        {
            Label isReceiver(env);
            if (useOwn) {
                Branch(Equal(*holder, receiver), &isReceiver, &ifEnd);
            } else {
                Branch(Equal(*holder, receiver), &isReceiver, &afterLoop);
            }
            Bind(&isReceiver);
            {
                GateRef length = GetLengthOfTaggedArray(elements);
                Label inRange(env);
                if (useOwn) {
                    Branch(Int64LessThan(index, length), &inRange, &ifEnd);
                } else {
                    Branch(Int64LessThan(index, length), &inRange, &loopExit);
                }
                Bind(&inRange);
                {
                    GateRef value1 = GetTaggedValueWithElementsKind(*holder, index);
                    Label notHole(env);
                    if (useOwn) {
                        Branch(Int64NotEqual(value1, Hole()), &notHole, &ifEnd);
                    } else {
                        Branch(Int64NotEqual(value1, Hole()), &notHole, &loopExit);
                    }
                    Bind(&notHole);
                    {
                        Branch(IsJsCOWArray(*holder), &isJsCOWArray, &isNotJsCOWArray);
                        Bind(&isJsCOWArray);
                        {
                            CallRuntime(glue, RTSTUB_ID(CheckAndCopyArray), {*holder});
                            SetValueWithElementsKind(glue, *holder, value, index, Boolean(true),
                                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                            returnValue = Undefined();
                            Jump(&exit);
                        }
                        Bind(&isNotJsCOWArray);
                        {
                            Jump(&setElementsArray);
                        }
                        Bind(&setElementsArray);
                        {
                            SetValueWithElementsKind(glue, *holder, value, index, Boolean(true),
                                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                            returnValue = Undefined();
                            Jump(&exit);
                        }
                    }
                }
            }
        }
        Bind(&isDictionaryElement);
        {
            GateRef entryA = FindElementFromNumberDictionary(glue, elements, index);
            Label negtiveOne(env);
            Label notNegtiveOne(env);
            Branch(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, &negtiveOne);
            Bind(&notNegtiveOne);
            {
                GateRef attr = GetAttributesFromDictionary<NumberDictionary>(elements, entryA);
                Label isWritandConfig(env);
                Label notWritandConfig(env);
                Branch(BoolAnd(IsWritable(attr), IsConfigable(attr)), &isWritandConfig, &notWritandConfig);
                Bind(&isWritandConfig);
                {
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        GateRef accessor = GetValueFromDictionary<NumberDictionary>(elements, entryA);
                        Label shouldCall(env);
                        Branch(ShouldCallSetter(receiver, *holder, accessor, attr), &shouldCall, &notAccessor);
                        Bind(&shouldCall);
                        {
                            returnValue = CallSetterHelper(glue, receiver, accessor, value, callback);
                            Jump(&exit);
                        }
                    }
                    Bind(&notAccessor);
                    {
                        Label holdEqualsRecv(env);
                        if (useOwn) {
                            Branch(Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
                        } else {
                            Branch(Equal(*holder, receiver), &holdEqualsRecv, &afterLoop);
                        }
                        Bind(&holdEqualsRecv);
                        {
                            UpdateValueInDict<NumberDictionary>(glue, elements, entryA, value);
                            returnValue = Undefined();
                            Jump(&exit);
                        }
                    }
                }
                Bind(&notWritandConfig);
                {
                    returnValue = Hole();
                    Jump(&exit);
                }
            }
            Bind(&negtiveOne);
            returnValue = Hole();
            Jump(&exit);
        }
    }
    if (useOwn) {
        Bind(&ifEnd);
    } else {
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
    }
    Label isExtensible(env);
    Label notExtensible(env);
    Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
    Bind(&isExtensible);
    {
        Label success(env);
        Label failed(env);
        Branch(AddElementInternal(glue, receiver, index, value,
                                  Int32(PropertyAttributes::GetDefaultAttributes())), &success, &failed);
        Bind(&success);
        {
            returnValue = Undefined();
            Jump(&exit);
        }
        Bind(&failed);
        {
            returnValue = Exception();
            Jump(&exit);
        }
    }
    Bind(&notExtensible);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        returnValue = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *returnValue;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SetPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value,
    bool useOwn, GateRef isInternal, ProfileOperation callback, bool canUseIsInternal)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_POINTER(), receiver);
    DEFVARIABLE(receiverHoleEntry, VariableType::INT32(), Int32(-1));
    Label exit(env);
    Label ifEnd(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Label findProperty(env);
    if (!useOwn) {
        // a do-while loop
        Jump(&loopHead);
        LoopBegin(&loopHead);
    }
    // auto *hclass = holder.GetTaggedObject()->GetClass()
    // JSType jsType = hclass->GetObjectType()
    GateRef hclass = LoadHClass(*holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSIndexObj(env);
    Label notSIndexObj(env);
    // if branch condition : IsSpecialIndexedObj(jsType)
    Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
    Bind(&isSIndexObj);
    {
        Label isFastTypeArray(env);
        Label notFastTypeArray(env);
        Branch(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
        Bind(&isFastTypeArray);
        {
            result = SetTypeArrayPropertyByName(glue, receiver, *holder, key, value, jsType);
            Label isNull(env);
            Label notNull(env);
            Branch(TaggedIsNull(*result), &isNull, &notNull);
            Bind(&isNull);
            {
                result = Hole();
                Jump(&exit);
            }
            Bind(&notNull);
            Branch(TaggedIsHole(*result), &notSIndexObj, &exit);
        }
        Bind(&notFastTypeArray);

        Label isSpecialContainer(env);
        Label notSpecialContainer(env);
        // Add SpecialContainer
        Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
        Bind(&isSpecialContainer);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotSetPropertyOnContainer));
            CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
            result = Exception();
            Jump(&exit);
        }
        Bind(&notSpecialContainer);
        {
            result = Hole();
            Jump(&exit);
        }
    }
    Bind(&notSIndexObj);
    {
        if (canUseIsInternal) {
            if (useOwn) {
                Branch(isInternal, &findProperty, &ifEnd);
            } else {
                Branch(isInternal, &findProperty, &loopExit);
            }
        } else {
            Jump(&findProperty);
        }
        Label isDicMode(env);
        Label notDicMode(env);
        Bind(&findProperty);
        // if branch condition : LIKELY(!hclass->IsDictionaryMode())
        Branch(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
        Bind(&notDicMode);
        {
            // LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetAttributes().GetTaggedObject())
            GateRef layOutInfo = GetLayoutFromHClass(hclass);
            // int propsNumber = hclass->NumberOfPropsFromHClass()
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum);
            Label hasEntry(env);
            // if branch condition : entry != -1
            if (useOwn) {
                Branch(Int32NotEqual(entry, Int32(-1)), &hasEntry, &ifEnd);
            } else {
                Branch(Int32NotEqual(entry, Int32(-1)), &hasEntry, &loopExit);
            }
            Bind(&hasEntry);
            {
                // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entry);
                GateRef attr = GetInt32OfTInt(propAttr);
                Label isAccessor(env);
                Label notAccessor(env);
                Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                Bind(&isAccessor);
                {
                    // auto accessor = JSObject::Cast(holder)->GetProperty(hclass, attr)
                    GateRef accessor = JSObjectGetProperty(*holder, hclass, attr);
                    Label shouldCall(env);
                    // ShouldCallSetter(receiver, *holder, accessor, attr)
                    Branch(ShouldCallSetter(receiver, *holder, accessor, attr), &shouldCall, &notAccessor);
                    Bind(&shouldCall);
                    {
                        result = CallSetterHelper(glue, receiver, accessor, value, callback);
                        Jump(&exit);
                    }
                }
                Bind(&notAccessor);
                {
                    Label writable(env);
                    Label notWritable(env);
                    Branch(IsWritable(attr), &writable, &notWritable);
                    Bind(&notWritable);
                    {
                        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                        result = Exception();
                        Jump(&exit);
                    }
                    Bind(&writable);
                    {
                        Label isTS(env);
                        Label notTS(env);
                        Branch(IsTSHClass(hclass), &isTS, &notTS);
                        Bind(&isTS);
                        {
                            GateRef attrVal = JSObjectGetProperty(*holder, hclass, attr);
                            Label attrValIsHole(env);
                            Branch(TaggedIsHole(attrVal), &attrValIsHole, &notTS);
                            Bind(&attrValIsHole);
                            {
                                Label storeReceiverHoleEntry(env);
                                Label noNeedStore(env);
                                GateRef checkReceiverHoleEntry = Int32Equal(*receiverHoleEntry, Int32(-1));
                                GateRef checkHolderEqualsRecv = Equal(*holder, receiver);
                                Branch(BoolAnd(checkReceiverHoleEntry, checkHolderEqualsRecv),
                                    &storeReceiverHoleEntry, &noNeedStore);
                                Bind(&storeReceiverHoleEntry);
                                {
                                    receiverHoleEntry = entry;
                                    Jump(&noNeedStore);
                                }
                                Bind(&noNeedStore);
                                if (useOwn) {
                                    Jump(&ifEnd);
                                } else {
                                    Jump(&loopExit);
                                }
                            }
                        }
                        Bind(&notTS);
                        Label holdEqualsRecv(env);
                        if (useOwn) {
                            Branch(Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
                        } else {
                            Branch(Equal(*holder, receiver), &holdEqualsRecv, &afterLoop);
                        }
                        Bind(&holdEqualsRecv);
                        {
                            // JSObject::Cast(holder)->SetProperty(thread, hclass, attr, value)
                            // return JSTaggedValue::Undefined()
                            Label executeSetProp(env);
                            CheckUpdateSharedType(false, &result, glue, jsType, attr, value, &executeSetProp, &exit);
                            Bind(&executeSetProp);
                            JSObjectSetProperty(glue, *holder, hclass, attr, key, value);
                            ProfilerStubBuilder(env).UpdatePropAttrWithValue(
                                glue, layOutInfo, attr, entry, value, callback);
                            result = Undefined();
                            Jump(&exit);
                        }
                    }
                }
            }
        }
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(*holder);
            // int entry = dict->FindEntry(key)
            GateRef entry1 = FindEntryFromNameDictionary(glue, array, key);
            Label notNegtiveOne(env);
            // if branch condition : entry != -1
            if (useOwn) {
                Branch(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &ifEnd);
            } else {
                Branch(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &loopExit);
            }
            Bind(&notNegtiveOne);
            {
                // auto attr = dict->GetAttributes(entry)
                GateRef attr1 = GetAttributesFromDictionary<NameDictionary>(array, entry1);
                Label isAccessor1(env);
                Label notAccessor1(env);
                // if branch condition : UNLIKELY(attr.IsAccessor())
                Branch(IsAccessor(attr1), &isAccessor1, &notAccessor1);
                Bind(&isAccessor1);
                {
                    // auto accessor = dict->GetValue(entry)
                    GateRef accessor1 = GetValueFromDictionary<NameDictionary>(array, entry1);
                    Label shouldCall1(env);
                    Branch(ShouldCallSetter(receiver, *holder, accessor1, attr1), &shouldCall1, &notAccessor1);
                    Bind(&shouldCall1);
                    {
                        result = CallSetterHelper(glue, receiver, accessor1, value, callback);
                        Jump(&exit);
                    }
                }
                Bind(&notAccessor1);
                {
                    Label writable1(env);
                    Label notWritable1(env);
                    Branch(IsWritable(attr1), &writable1, &notWritable1);
                    Bind(&notWritable1);
                    {
                        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                        result = Exception();
                        Jump(&exit);
                    }
                    Bind(&writable1);
                    {
                        Label holdEqualsRecv1(env);
                        if (useOwn) {
                            Branch(Equal(*holder, receiver), &holdEqualsRecv1, &ifEnd);
                        } else {
                            Branch(Equal(*holder, receiver), &holdEqualsRecv1, &afterLoop);
                        }
                        Bind(&holdEqualsRecv1);
                        {
                            // dict->UpdateValue(thread, entry, value)
                            // return JSTaggedValue::Undefined()
                            Label executeSetProp(env);
                            CheckUpdateSharedType(true, &result, glue, jsType, attr1, value, &executeSetProp, &exit);
                            Bind(&executeSetProp);
                            UpdateValueInDict<NameDictionary>(glue, array, entry1, value);
                            result = Undefined();
                            Jump(&exit);
                        }
                    }
                }
            }
        }
    }
    if (useOwn) {
        Bind(&ifEnd);
    } else {
        Bind(&loopExit);
        {
            // holder = hclass->GetPrototype()
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            // loop condition for a do-while loop
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
    }
    Label holeEntryNotNegtiveOne(env);
    Label holeEntryIfEnd(env);
    Branch(Int32NotEqual(*receiverHoleEntry, Int32(-1)), &holeEntryNotNegtiveOne, &holeEntryIfEnd);
    Bind(&holeEntryNotNegtiveOne);
    {
        GateRef receiverHClass = LoadHClass(receiver);
        GateRef receiverLayoutInfo = GetLayoutFromHClass(receiverHClass);
        GateRef holePropAttr = GetPropAttrFromLayoutInfo(receiverLayoutInfo, *receiverHoleEntry);
        GateRef holeAttr = GetInt32OfTInt(holePropAttr);
        JSObjectSetProperty(glue, receiver, receiverHClass, holeAttr, key, value);
        ProfilerStubBuilder(env).UpdatePropAttrWithValue(
            glue, receiverLayoutInfo, holeAttr, *receiverHoleEntry, value, callback);
        result = Undefined();
        Jump(&exit);
    }
    Bind(&holeEntryIfEnd);

    Label extensible(env);
    Label inextensible(env);
    Branch(IsExtensible(receiver), &extensible, &inextensible);
    Bind(&inextensible);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result = Exception();
        Jump(&exit);
    }
    Bind(&extensible);
    {
        result = AddPropertyByName(glue, receiver, key, value,
            Int32(PropertyAttributes::GetDefaultAttributes()), callback);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SetPropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value, bool useOwn,
    ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label subEntry1(env);
    env->SubCfgEntry(&subEntry1);
    DEFVARIABLE(varKey, VariableType::JS_ANY(), key);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(isInternal, VariableType::BOOL(), True());
    Label isNumberOrStringSymbol(env);
    Label notNumber(env);
    Label isStringOrSymbol(env);
    Label notStringOrSymbol(env);
    Label exit(env);
    Branch(TaggedIsNumber(*varKey), &isNumberOrStringSymbol, &notNumber);
    Bind(&notNumber);
    {
        Branch(TaggedIsStringOrSymbol(*varKey), &isNumberOrStringSymbol, &notStringOrSymbol);
        Bind(&notStringOrSymbol);
        {
            result = Hole();
            Jump(&exit);
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        GateRef index64 = TryToElementsIndex(glue, *varKey);
        Label validIndex(env);
        Label notValidIndex(env);
        Label greaterThanInt32Max(env);
        Label notGreaterThanInt32Max(env);
        Branch(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
        Bind(&greaterThanInt32Max);
        {
            Jump(&exit);
        }
        Bind(&notGreaterThanInt32Max);
        GateRef index = TruncInt64ToInt32(index64);
        Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = SetPropertyByIndex(glue, receiver, index, value, useOwn);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label isNumber1(env);
            Label notNumber1(env);
            Label setByName(env);
            Branch(TaggedIsNumber(*varKey), &isNumber1, &notNumber1);
            Bind(&isNumber1);
            {
                result = Hole();
                Jump(&exit);
            }
            Label isString(env);
            Label notString(env);
            Bind(&notNumber1);
            {
                Label notIntenalString(env);
                Branch(TaggedIsString(*varKey), &isString, &notString);
                Bind(&isString);
                {
                    Branch(IsInternalString(*varKey), &setByName, &notIntenalString);
                    Bind(&notIntenalString);
                    {
                        Label notFind(env);
                        Label find(env);
                        GateRef res = CallNGCRuntime(glue, RTSTUB_ID(TryGetInternString), { glue, *varKey });
                        Branch(TaggedIsHole(res), &notFind, &find);
                        Bind(&notFind);
                        {
                            varKey = CallRuntime(glue, RTSTUB_ID(InsertStringToTable), { *varKey });
                            isInternal = False();
                            Jump(&setByName);
                        }
                        Bind(&find);
                        {
                            varKey = res;
                            Jump(&setByName);
                        }
                    }
                }
            }
            Bind(&notString);
            CheckDetectorName(glue, *varKey, &setByName, &exit);
            Bind(&setByName);
            {
                result = SetPropertyByName(glue, receiver, *varKey, value, useOwn,  *isInternal, callback, true);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isProtoType(env);
    Branch(IsProtoTypeHClass(oldHClass), &isProtoType, &exit);
    Bind(&isProtoType);
    {
        Label notEqualHClass(env);
        Branch(Equal(oldHClass, newHClass), &exit, &notEqualHClass);
        Bind(&notEqualHClass);
        {
            SetIsProtoTypeToHClass(glue, newHClass, True());
            CallRuntime(glue, RTSTUB_ID(NoticeThroughChainAndRefreshUser), { oldHClass, newHClass });
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef StubBuilder::GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());

    Label isDefaultLabel(env);
    Label noDefaultLabel(env);
    Branch(IsSpecialContainer(jsType), &noDefaultLabel, &isDefaultLabel);
    Bind(&noDefaultLabel);
    {
        result = JSAPIContainerGet(glue, receiver, index);
        Jump(&exit);
    }
    Bind(&isDefaultLabel);
    {
        Jump(&exit);
    }
    Bind(&exit);

    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastTypeOf(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    GateRef undefinedIndex = GetGlobalConstantOffset(ConstantIndex::UNDEFINED_STRING_INDEX);
    GateRef gConstUndefinedStr = Load(VariableType::JS_POINTER(), gConstAddr, undefinedIndex);
    DEFVARIABLE(result, VariableType::JS_POINTER(), gConstUndefinedStr);
    Label objIsTrue(env);
    Label objNotTrue(env);
    Label defaultLabel(env);
    GateRef gConstBooleanStr = Load(VariableType::JS_POINTER(), gConstAddr,
        GetGlobalConstantOffset(ConstantIndex::BOOLEAN_STRING_INDEX));
    Branch(TaggedIsTrue(obj), &objIsTrue, &objNotTrue);
    Bind(&objIsTrue);
    {
        result = gConstBooleanStr;
        Jump(&exit);
    }
    Bind(&objNotTrue);
    {
        Label objIsFalse(env);
        Label objNotFalse(env);
        Branch(TaggedIsFalse(obj), &objIsFalse, &objNotFalse);
        Bind(&objIsFalse);
        {
            result = gConstBooleanStr;
            Jump(&exit);
        }
        Bind(&objNotFalse);
        {
            Label objIsNull(env);
            Label objNotNull(env);
            Branch(TaggedIsNull(obj), &objIsNull, &objNotNull);
            Bind(&objIsNull);
            {
                result = Load(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantOffset(ConstantIndex::OBJECT_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNull);
            {
                Label objIsUndefined(env);
                Label objNotUndefined(env);
                Branch(TaggedIsUndefined(obj), &objIsUndefined, &objNotUndefined);
                Bind(&objIsUndefined);
                {
                    result = Load(VariableType::JS_POINTER(), gConstAddr,
                        GetGlobalConstantOffset(ConstantIndex::UNDEFINED_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotUndefined);
                Jump(&defaultLabel);
            }
        }
    }
    Bind(&defaultLabel);
    {
        Label objIsHeapObject(env);
        Label objNotHeapObject(env);
        Branch(TaggedIsHeapObject(obj), &objIsHeapObject, &objNotHeapObject);
        Bind(&objIsHeapObject);
        {
            Label objIsString(env);
            Label objNotString(env);
            Branch(IsString(obj), &objIsString, &objNotString);
            Bind(&objIsString);
            {
                result = Load(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantOffset(ConstantIndex::STRING_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotString);
            {
                Label objIsSymbol(env);
                Label objNotSymbol(env);
                Branch(IsSymbol(obj), &objIsSymbol, &objNotSymbol);
                Bind(&objIsSymbol);
                {
                    result = Load(VariableType::JS_POINTER(), gConstAddr,
                        GetGlobalConstantOffset(ConstantIndex::SYMBOL_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotSymbol);
                {
                    Label objIsCallable(env);
                    Label objNotCallable(env);
                    Branch(IsCallable(obj), &objIsCallable, &objNotCallable);
                    Bind(&objIsCallable);
                    {
                        result = Load(VariableType::JS_POINTER(), gConstAddr,
                            GetGlobalConstantOffset(ConstantIndex::FUNCTION_STRING_INDEX));
                        Jump(&exit);
                    }
                    Bind(&objNotCallable);
                    {
                        Label objIsBigInt(env);
                        Label objNotBigInt(env);
                        Branch(TaggedObjectIsBigInt(obj), &objIsBigInt, &objNotBigInt);
                        Bind(&objIsBigInt);
                        {
                            result = Load(VariableType::JS_POINTER(), gConstAddr,
                                GetGlobalConstantOffset(ConstantIndex::BIGINT_STRING_INDEX));
                            Jump(&exit);
                        }
                        Bind(&objNotBigInt);
                        {
                            result = Load(VariableType::JS_POINTER(), gConstAddr,
                                GetGlobalConstantOffset(ConstantIndex::OBJECT_STRING_INDEX));
                            Jump(&exit);
                        }
                    }
                }
            }
        }
        Bind(&objNotHeapObject);
        {
            Label objIsNum(env);
            Label objNotNum(env);
            Branch(TaggedIsNumber(obj), &objIsNum, &objNotNum);
            Bind(&objIsNum);
            {
                result = Load(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantOffset(ConstantIndex::NUMBER_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNum);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::InstanceOf(
    GateRef glue, GateRef object, GateRef target, GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);

    // 1.If Type(target) is not Object, throw a TypeError exception.
    Label targetIsHeapObject(env);
    Label targetIsEcmaObject(env);
    Label targetNotEcmaObject(env);
    Branch(TaggedIsHeapObject(target), &targetIsHeapObject, &targetNotEcmaObject);
    Bind(&targetIsHeapObject);
    Branch(TaggedObjectIsEcmaObject(target), &targetIsEcmaObject, &targetNotEcmaObject);
    Bind(&targetNotEcmaObject);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(TargetTypeNotObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result = Exception();
        Jump(&exit);
    }
    Bind(&targetIsEcmaObject);
    {
        // 2.Let instOfHandler be GetMethod(target, @@hasInstance).
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        GateRef hasInstanceSymbol = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                      GlobalEnv::HASINSTANCE_SYMBOL_INDEX);
        GateRef instof = GetMethod(glue, target, hasInstanceSymbol, profileTypeInfo, slotId);

        // 3.ReturnIfAbrupt(instOfHandler).
        Label isPendingException(env);
        Label noPendingException(env);
        Branch(HasPendingException(glue), &isPendingException, &noPendingException);
        Bind(&isPendingException);
        {
            result = Exception();
            Jump(&exit);
        }
        Bind(&noPendingException);

        // 4.If instOfHandler is not undefined, then
        Label instOfNotUndefined(env);
        Label instOfIsUndefined(env);
        Label fastPath(env);
        Label targetNotCallable(env);
        Branch(TaggedIsUndefined(instof), &instOfIsUndefined, &instOfNotUndefined);
        Bind(&instOfNotUndefined);
        {
            TryFastHasInstance(glue, instof, target, object, &fastPath, &exit, &result, callback);
        }
        Bind(&instOfIsUndefined);
        {
            // 5.If IsCallable(target) is false, throw a TypeError exception.
            Branch(IsCallable(target), &fastPath, &targetNotCallable);
            Bind(&targetNotCallable);
            {
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(InstanceOfErrorTargetNotCallable));
                CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                result = Exception();
                Jump(&exit);
            }
        }
        Bind(&fastPath);
        {
            // 6.Return ? OrdinaryHasInstance(target, object).
            result = OrdinaryHasInstance(glue, target, object);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::TryFastHasInstance(GateRef glue, GateRef instof, GateRef target, GateRef object, Label *fastPath,
                                     Label *exit, Variable *result, ProfileOperation callback)
{
    auto env = GetEnvironment();

    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef function = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::HASINSTANCE_FUNCTION_INDEX);

    Label slowPath(env);
    Label tryFastPath(env);
    GateRef isEqual = IntPtrEqual(instof, function);
    Branch(isEqual, &tryFastPath, &slowPath);
    Bind(&tryFastPath);
    Jump(fastPath);
    Bind(&slowPath);
    {
        GateRef retValue = JSCallDispatch(glue, instof, Int32(1), 0, Circuit::NullGate(),
                                          JSCallMode::CALL_SETTER, { target, object }, callback);
        result->WriteVariable(FastToBoolean(retValue));
        Jump(exit);
    }
}

GateRef StubBuilder::GetMethod(GateRef glue, GateRef obj, GateRef key, GateRef profileTypeInfo, GateRef slotId)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);

    StringIdInfo info;
    AccessObjectStubBuilder builder(this);
    GateRef value = builder.LoadObjByName(glue, obj, key, info, profileTypeInfo, slotId, ProfileOperation());

    Label isPendingException(env);
    Label noPendingException(env);
    Branch(HasPendingException(glue), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noPendingException);
    Label valueIsUndefinedOrNull(env);
    Label valueNotUndefinedOrNull(env);
    Branch(TaggedIsUndefinedOrNull(value), &valueIsUndefinedOrNull, &valueNotUndefinedOrNull);
    Bind(&valueIsUndefinedOrNull);
    {
        result = Undefined();
        Jump(&exit);
    }
    Bind(&valueNotUndefinedOrNull);
    {
        Label valueIsCallable(env);
        Label valueNotCallable(env);
        Label valueIsHeapObject(env);
        Branch(TaggedIsHeapObject(value), &valueIsHeapObject, &valueNotCallable);
        Bind(&valueIsHeapObject);
        Branch(IsCallable(value), &valueIsCallable, &valueNotCallable);
        Bind(&valueNotCallable);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(NonCallable));
            CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
            result = Exception();
            Jump(&exit);
        }
        Bind(&valueIsCallable);
        {
            result = value;
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastGetPropertyByName(GateRef glue, GateRef obj, GateRef key, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label checkResult(env);
    Label fastpath(env);
    Label slowpath(env);

    Branch(TaggedIsHeapObject(obj), &fastpath, &slowpath);
    Bind(&fastpath);
    {
        result = GetPropertyByName(glue, obj, key, callback, True());
        Branch(TaggedIsHole(*result), &slowpath, &exit);
    }
    Bind(&slowpath);
    {
        result = CallRuntime(glue, RTSTUB_ID(LoadICByName), { Undefined(), obj, key, IntToTaggedInt(Int32(0)) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastGetPropertyByIndex(GateRef glue, GateRef obj, GateRef index, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label fastPath(env);
    Label slowPath(env);

    Branch(TaggedIsHeapObject(obj), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        result = GetPropertyByIndex(glue, obj, index, callback);
        Label notHole(env);
        Branch(TaggedIsHole(*result), &slowPath, &exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
            { obj, IntToTaggedInt(index), TaggedFalse(), Undefined() });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::FastSetPropertyByName(GateRef glue, GateRef obj, GateRef key, GateRef value,
    ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(keyVar, VariableType::JS_ANY(), key);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(isInternal, VariableType::BOOL(), True());
    Label exit(env);
    Label fastPath(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(obj), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Label isString(env);
        Label getByName(env);
        Label isInternalString(env);
        Label notIntenalString(env);
        Branch(TaggedIsString(*keyVar), &isString, &getByName);
        Bind(&isString);
        {
            Branch(IsInternalString(*keyVar), &isInternalString, &notIntenalString);
            Bind(&isInternalString);
            Jump(&getByName);
            Bind(&notIntenalString);
            {
                Label notFind(env);
                Label find(env);
                GateRef res = CallNGCRuntime(glue, RTSTUB_ID(TryGetInternString), { glue, *keyVar });
                Branch(TaggedIsHole(res), &notFind, &find);
                Bind(&notFind);
                {
                    keyVar = CallRuntime(glue, RTSTUB_ID(InsertStringToTable), { key });
                    isInternal = False();
                    Jump(&getByName);
                }
                Bind(&find);
                {
                    keyVar = res;
                    Jump(&getByName);
                }
            }
        }
        Bind(&getByName);

        result = SetPropertyByName(glue, obj, *keyVar, value, false, *isInternal, callback, true);
        Label notHole(env);
        Branch(TaggedIsHole(*result), &slowPath, &exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(StoreICByValue), { obj, *keyVar, value, IntToTaggedInt(Int32(0)) });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::FastSetPropertyByIndex(GateRef glue, GateRef obj, GateRef index, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label fastPath(env);
    Label slowPath(env);

    Branch(TaggedIsHeapObject(obj), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        result = SetPropertyByIndex(glue, obj, index, value, false);
        Label notHole(env);
        Branch(TaggedIsHole(*result), &slowPath, &exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
            { obj, IntToTaggedInt(index), value });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::GetCtorPrototype(GateRef ctor)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(constructorPrototype, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label isHClass(env);
    Label isPrototype(env);
    Label isHeapObject(env);
    Label notHeapObject(env);

    GateRef ctorProtoOrHC = Load(VariableType::JS_POINTER(), ctor, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    Branch(TaggedIsHeapObject(ctorProtoOrHC), &isHeapObject, &notHeapObject);
    Bind(&notHeapObject);
    {
        // If go slow path, return hole.
        constructorPrototype = Hole();
        Jump(&exit);
    }
    Bind(&isHeapObject);
    Branch(IsJSHClass(ctorProtoOrHC), &isHClass, &isPrototype);
    Bind(&isHClass);
    {
        constructorPrototype = Load(VariableType::JS_POINTER(), ctorProtoOrHC, IntPtr(JSHClass::PROTOTYPE_OFFSET));
        Jump(&exit);
    }
    Bind(&isPrototype);
    {
        constructorPrototype = ctorProtoOrHC;
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *constructorPrototype;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::OrdinaryHasInstance(GateRef glue, GateRef target, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    DEFVARIABLE(object, VariableType::JS_ANY(), obj);

    // 1. If IsCallable(C) is false, return false.
    Label targetIsCallable(env);
    Label targetNotCallable(env);
    Branch(IsCallable(target), &targetIsCallable, &targetNotCallable);
    Bind(&targetNotCallable);
    {
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&targetIsCallable);
    {
        // 2. If C has a [[BoundTargetFunction]] internal slot, then
        //    a. Let BC be the value of C's [[BoundTargetFunction]] internal slot.
        //    b. Return InstanceofOperator(O,BC)  (see 12.9.4).
        Label targetIsBoundFunction(env);
        Label targetNotBoundFunction(env);
        Branch(IsBoundFunction(target), &targetIsBoundFunction, &targetNotBoundFunction);
        Bind(&targetIsBoundFunction);
        {
            GateRef boundTarget = Load(VariableType::JS_ANY(), target, IntPtr(JSBoundFunction::BOUND_TARGET_OFFSET));
            result = CallRuntime(glue, RTSTUB_ID(InstanceOf), { obj, boundTarget });
            Jump(&exit);
        }
        Bind(&targetNotBoundFunction);
        {
            // 3. If Type(O) is not Object, return false
            Label objIsHeapObject(env);
            Label objIsEcmaObject(env);
            Label objNotEcmaObject(env);
            Branch(TaggedIsHeapObject(obj), &objIsHeapObject, &objNotEcmaObject);
            Bind(&objIsHeapObject);
            Branch(TaggedObjectIsEcmaObject(obj), &objIsEcmaObject, &objNotEcmaObject);
            Bind(&objNotEcmaObject);
            {
                result = TaggedFalse();
                Jump(&exit);
            }
            Bind(&objIsEcmaObject);
            {
                // 4. Let P be Get(C, "prototype").
                Label getCtorProtoSlowPath(env);
                Label ctorIsJSFunction(env);
                Label gotCtorPrototype(env);
                DEFVARIABLE(constructorPrototype, VariableType::JS_ANY(), Undefined());
                Branch(IsJSFunction(target), &ctorIsJSFunction, &getCtorProtoSlowPath);
                Bind(&ctorIsJSFunction);
                {
                    Label getCtorProtoFastPath(env);
                    GateRef ctorProtoOrHC = Load(VariableType::JS_POINTER(), target,
                                                 IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));

                    Branch(TaggedIsHole(ctorProtoOrHC), &getCtorProtoSlowPath, &getCtorProtoFastPath);
                    Bind(&getCtorProtoFastPath);
                    {
                        constructorPrototype = GetCtorPrototype(target);
                        Branch(TaggedIsHole(*constructorPrototype), &getCtorProtoSlowPath, &gotCtorPrototype);
                    }
                }
                Bind(&getCtorProtoSlowPath);
                {
                    auto prototypeString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                                  ConstantIndex::PROTOTYPE_STRING_INDEX);
                    constructorPrototype = FastGetPropertyByName(glue, target, prototypeString, ProfileOperation());
                    Jump(&gotCtorPrototype);
                }
                Bind(&gotCtorPrototype);

                // 5. ReturnIfAbrupt(P).
                // no throw exception, so needn't return
                Label isPendingException(env);
                Label noPendingException(env);
                Branch(HasPendingException(glue), &isPendingException, &noPendingException);
                Bind(&isPendingException);
                {
                    result = Exception();
                    Jump(&exit);
                }
                Bind(&noPendingException);

                // 6. If Type(P) is not Object, throw a TypeError exception.
                Label constructorPrototypeIsHeapObject(env);
                Label constructorPrototypeIsEcmaObject(env);
                Label constructorPrototypeNotEcmaObject(env);
                Branch(TaggedIsHeapObject(*constructorPrototype), &constructorPrototypeIsHeapObject,
                    &constructorPrototypeNotEcmaObject);
                Bind(&constructorPrototypeIsHeapObject);
                Branch(TaggedObjectIsEcmaObject(*constructorPrototype), &constructorPrototypeIsEcmaObject,
                    &constructorPrototypeNotEcmaObject);
                Bind(&constructorPrototypeNotEcmaObject);
                {
                    GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(InstanceOfErrorTargetNotCallable));
                    CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                    result = Exception();
                    Jump(&exit);
                }
                Bind(&constructorPrototypeIsEcmaObject);
                {
                    // 7. Repeat
                    //    a.Let O be O.[[GetPrototypeOf]]().
                    //    b.ReturnIfAbrupt(O).
                    //    c.If O is null, return false.
                    //    d.If SameValue(P, O) is true, return true.
                    Label loopHead(env);
                    Label loopEnd(env);
                    Label afterLoop(env);
                    Label strictEqual1(env);
                    Label notStrictEqual1(env);
                    Label shouldReturn(env);
                    Label shouldContinue(env);

                    Branch(TaggedIsNull(*object), &afterLoop, &loopHead);
                    LoopBegin(&loopHead);
                    {
                        GateRef isEqual = SameValue(glue, *object, *constructorPrototype);

                        Branch(isEqual, &strictEqual1, &notStrictEqual1);
                        Bind(&strictEqual1);
                        {
                            result = TaggedTrue();
                            Jump(&exit);
                        }
                        Bind(&notStrictEqual1);
                        {
                            object = GetPrototype(glue, *object);

                            Branch(HasPendingException(glue), &shouldReturn, &shouldContinue);
                            Bind(&shouldReturn);
                            {
                                result = Exception();
                                Jump(&exit);
                            }
                        }
                        Bind(&shouldContinue);
                        Branch(TaggedIsNull(*object), &afterLoop, &loopEnd);
                    }
                    Bind(&loopEnd);
                    LoopEnd(&loopHead);
                    Bind(&afterLoop);
                    {
                        result = TaggedFalse();
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetPrototype(GateRef glue, GateRef object)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label objectIsHeapObject(env);
    Label objectIsEcmaObject(env);
    Label objectNotEcmaObject(env);

    Branch(TaggedIsHeapObject(object), &objectIsHeapObject, &objectNotEcmaObject);
    Bind(&objectIsHeapObject);
    Branch(TaggedObjectIsEcmaObject(object), &objectIsEcmaObject, &objectNotEcmaObject);
    Bind(&objectNotEcmaObject);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotGetNotEcmaObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        CallRuntime(glue, RTSTUB_ID(Dump), { object });
        result = Exception();
        Jump(&exit);
    }
    Bind(&objectIsEcmaObject);
    {
        Label objectIsJsProxy(env);
        Label objectNotIsJsProxy(env);
        Branch(IsJsProxy(object), &objectIsJsProxy, &objectNotIsJsProxy);
        Bind(&objectIsJsProxy);
        {
            result = CallRuntime(glue, RTSTUB_ID(CallGetPrototype), { object });
            Jump(&exit);
        }
        Bind(&objectNotIsJsProxy);
        {
            result = GetPrototypeFromHClass(LoadHClass(object));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SameValue(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0.0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0.0));
    Label strictEqual(env);
    Label stringEqualCheck(env);
    Label stringCompare(env);
    Label bigIntEqualCheck(env);
    Label numberEqualCheck1(env);

    Branch(Equal(left, right), &strictEqual, &numberEqualCheck1);
    Bind(&strictEqual);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&numberEqualCheck1);
    {
        Label leftIsNumber(env);
        Label leftIsNotNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &exit);
            Bind(&rightIsNumber);
            {
                Label numberEqualCheck2(env);
                Label leftIsInt(env);
                Label leftNotInt(env);
                Label getRight(env);
                Branch(TaggedIsInt(left), &leftIsInt, &leftNotInt);
                Bind(&leftIsInt);
                {
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    Jump(&getRight);
                }
                Bind(&leftNotInt);
                {
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&getRight);
                }
                Bind(&getRight);
                {
                    Label rightIsInt(env);
                    Label rightNotInt(env);
                    Branch(TaggedIsInt(right), &rightIsInt, &rightNotInt);
                    Bind(&rightIsInt);
                    {
                        doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                        Jump(&numberEqualCheck2);
                    }
                    Bind(&rightNotInt);
                    {
                        doubleRight = GetDoubleOfTDouble(right);
                        Jump(&numberEqualCheck2);
                    }
                }
                Bind(&numberEqualCheck2);
                {
                    Label boolAndCheck(env);
                    Label signbitCheck(env);
                    Branch(DoubleEqual(*doubleLeft, *doubleRight), &signbitCheck, &boolAndCheck);
                    Bind(&signbitCheck);
                    {
                        GateRef leftEncoding = CastDoubleToInt64(*doubleLeft);
                        GateRef RightEncoding = CastDoubleToInt64(*doubleRight);
                        Label leftIsMinusZero(env);
                        Label leftNotMinusZero(env);
                        Branch(Int64Equal(leftEncoding, Int64(base::MINUS_ZERO_BITS)),
                            &leftIsMinusZero, &leftNotMinusZero);
                        Bind(&leftIsMinusZero);
                        {
                            Label rightIsMinusZero(env);
                            Branch(Int64Equal(RightEncoding, Int64(base::MINUS_ZERO_BITS)), &rightIsMinusZero, &exit);
                            Bind(&rightIsMinusZero);
                            {
                                result = True();
                                Jump(&exit);
                            }
                        }
                        Bind(&leftNotMinusZero);
                        {
                            Label rightNotMinusZero(env);
                            Branch(Int64Equal(RightEncoding, Int64(base::MINUS_ZERO_BITS)), &exit, &rightNotMinusZero);
                            Bind(&rightNotMinusZero);
                            {
                                result = True();
                                Jump(&exit);
                            }
                        }
                    }
                    Bind(&boolAndCheck);
                    {
                        result = BoolAnd(DoubleIsNAN(*doubleLeft), DoubleIsNAN(*doubleRight));
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&leftIsNotNumber);
        Branch(TaggedIsNumber(right), &exit, &stringEqualCheck);
        Bind(&stringEqualCheck);
        Branch(BothAreString(left, right), &stringCompare, &bigIntEqualCheck);
        Bind(&stringCompare);
        {
            result = FastStringEqual(glue, left, right);
            Jump(&exit);
        }
        Bind(&bigIntEqualCheck);
        {
            Label leftIsBigInt(env);
            Label leftIsNotBigInt(env);
            Branch(TaggedIsBigInt(left), &leftIsBigInt, &exit);
            Bind(&leftIsBigInt);
            {
                Label rightIsBigInt(env);
                Branch(TaggedIsBigInt(right), &rightIsBigInt, &exit);
                Bind(&rightIsBigInt);
                result = CallNGCRuntime(glue, RTSTUB_ID(BigIntEquals), { left, right });
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SameValueZero(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0.0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0.0));
    Label strictEqual(env);
    Label stringEqualCheck(env);
    Label stringCompare(env);
    Label bigIntEqualCheck(env);
    Label numberEqualCheck1(env);

    Branch(Equal(left, right), &strictEqual, &numberEqualCheck1);
    Bind(&strictEqual);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&numberEqualCheck1);
    {
        Label leftIsNumber(env);
        Label leftIsNotNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &exit);
            Bind(&rightIsNumber);
            {
                Label numberEqualCheck2(env);
                Label leftIsInt(env);
                Label leftNotInt(env);
                Label getRight(env);
                Branch(TaggedIsInt(left), &leftIsInt, &leftNotInt);
                Bind(&leftIsInt);
                {
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    Jump(&getRight);
                }
                Bind(&leftNotInt);
                {
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&getRight);
                }
                Bind(&getRight);
                {
                    Label rightIsInt(env);
                    Label rightNotInt(env);
                    Branch(TaggedIsInt(right), &rightIsInt, &rightNotInt);
                    Bind(&rightIsInt);
                    {
                        doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                        Jump(&numberEqualCheck2);
                    }
                    Bind(&rightNotInt);
                    {
                        doubleRight = GetDoubleOfTDouble(right);
                        Jump(&numberEqualCheck2);
                    }
                }
                Bind(&numberEqualCheck2);
                {
                    Label nanCheck(env);
                    Label doubleEqual(env);
                    Branch(DoubleEqual(*doubleLeft, *doubleRight), &doubleEqual, &nanCheck);
                    Bind(&doubleEqual);
                    {
                        result = True();
                        Jump(&exit);
                    }
                    Bind(&nanCheck);
                    {
                        result = BoolAnd(DoubleIsNAN(*doubleLeft), DoubleIsNAN(*doubleRight));
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&leftIsNotNumber);
        Branch(TaggedIsNumber(right), &exit, &stringEqualCheck);
        Bind(&stringEqualCheck);
        Branch(BothAreString(left, right), &stringCompare, &bigIntEqualCheck);
        Bind(&stringCompare);
        {
            result = FastStringEqual(glue, left, right);
            Jump(&exit);
        }
        Bind(&bigIntEqualCheck);
        {
            Label leftIsBigInt(env);
            Label leftIsNotBigInt(env);
            Branch(TaggedIsBigInt(left), &leftIsBigInt, &exit);
            Bind(&leftIsBigInt);
            {
                Label rightIsBigInt(env);
                Branch(TaggedIsBigInt(right), &rightIsBigInt, &exit);
                Bind(&rightIsBigInt);
                result = CallNGCRuntime(glue, RTSTUB_ID(BigIntSameValueZero), { left, right });
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastStringEqual(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label hashcodeCompare(env);
    Label contentsCompare(env);
    Label lenEqualOneCheck(env);
    Label lenIsOne(env);
    Branch(Int32Equal(GetLengthFromString(left), GetLengthFromString(right)), &lenEqualOneCheck, &exit);
    Bind(&lenEqualOneCheck);
    Branch(Int32Equal(GetLengthFromString(left), Int32(1)), &lenIsOne, &hashcodeCompare);
    Bind(&lenIsOne);
    {
        Label leftFlattenFastPath(env);
        FlatStringStubBuilder leftFlat(this);
        leftFlat.FlattenString(glue, left, &leftFlattenFastPath);
        Bind(&leftFlattenFastPath);
        {
            Label rightFlattenFastPath(env);
            FlatStringStubBuilder rightFlat(this);
            rightFlat.FlattenString(glue, right, &rightFlattenFastPath);
            Bind(&rightFlattenFastPath);
            {
                BuiltinsStringStubBuilder stringBuilder(this);
                StringInfoGateRef leftStrInfoGate(&leftFlat);
                StringInfoGateRef rightStrInfoGate(&rightFlat);
                GateRef leftStrToInt = stringBuilder.StringAt(leftStrInfoGate, Int32(0));
                GateRef rightStrToInt = stringBuilder.StringAt(rightStrInfoGate, Int32(0));
                result = Equal(leftStrToInt, rightStrToInt);
                Jump(&exit);
            }
        }
    }

    Bind(&hashcodeCompare);
    Label leftNotNeg(env);
    GateRef leftHash = TryGetHashcodeFromString(left);
    GateRef rightHash = TryGetHashcodeFromString(right);
    Branch(Int64Equal(leftHash, Int64(-1)), &contentsCompare, &leftNotNeg);
    Bind(&leftNotNeg);
    {
        Label rightNotNeg(env);
        Branch(Int64Equal(rightHash, Int64(-1)), &contentsCompare, &rightNotNeg);
        Bind(&rightNotNeg);
        Branch(Int64Equal(leftHash, rightHash), &contentsCompare, &exit);
    }

    Bind(&contentsCompare);
    {
        GateRef stringEqual = CallRuntime(glue, RTSTUB_ID(StringEqual), { left, right });
        result = Equal(stringEqual, TaggedTrue());
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastStrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label strictEqual(env);
    Label leftIsNumber(env);
    Label leftIsNotNumber(env);
    Label sameVariableCheck(env);
    Label stringEqualCheck(env);
    Label stringCompare(env);
    Label bigIntEqualCheck(env);
    Label exit(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &exit);
        Bind(&rightIsNumber);
        {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0.0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0.0));
            DEFVARIABLE(curType, VariableType::INT32(), Int32(PGOSampleType::IntType()));
            Label leftIsInt(env);
            Label leftNotInt(env);
            Label getRight(env);
            Label numberEqualCheck(env);

            Branch(TaggedIsInt(left), &leftIsInt, &leftNotInt);
            Bind(&leftIsInt);
            {
                doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                Jump(&getRight);
            }
            Bind(&leftNotInt);
            {
                curType = Int32(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
                Jump(&getRight);
            }
            Bind(&getRight);
            {
                Label rightIsInt(env);
                Label rightNotInt(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightNotInt);
                Bind(&rightIsInt);
                {
                    GateRef type = Int32(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                    Jump(&numberEqualCheck);
                }
                Bind(&rightNotInt);
                {
                    GateRef type = Int32(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&numberEqualCheck);
                }
            }
            Bind(&numberEqualCheck);
            {
                Label doubleEqualCheck(env);
                Branch(BoolOr(DoubleIsNAN(*doubleLeft), DoubleIsNAN(*doubleRight)), &exit, &doubleEqualCheck);
                Bind(&doubleEqualCheck);
                {
                    result = DoubleEqual(*doubleLeft, *doubleRight);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&leftIsNotNumber);
    Branch(TaggedIsNumber(right), &exit, &sameVariableCheck);
    Bind(&sameVariableCheck);
    Branch(Equal(left, right), &strictEqual, &stringEqualCheck);
    Bind(&stringEqualCheck);
    Branch(BothAreString(left, right), &stringCompare, &bigIntEqualCheck);
    Bind(&stringCompare);
    {
        callback.ProfileOpType(Int32(PGOSampleType::StringType()));
        result = FastStringEqual(glue, left, right);
        Jump(&exit);
    }
    Bind(&bigIntEqualCheck);
    {
        Label leftIsBigInt(env);
        Label leftIsNotBigInt(env);
        Branch(TaggedIsBigInt(left), &leftIsBigInt, &exit);
        Bind(&leftIsBigInt);
        {
            Label rightIsBigInt(env);
            Branch(TaggedIsBigInt(right), &rightIsBigInt, &exit);
            Bind(&rightIsBigInt);
            callback.ProfileOpType(Int32(PGOSampleType::BigIntType()));
            result = CallNGCRuntime(glue, RTSTUB_ID(BigIntEquals), { left, right });
            Jump(&exit);
        }
    }
    Bind(&strictEqual);
    {
        callback.ProfileOpType(Int32(PGOSampleType::AnyType()));
        result = True();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label leftEqualRight(env);
    Label leftNotEqualRight(env);
    Label exit(env);
    Branch(Equal(left, right), &leftEqualRight, &leftNotEqualRight);
    Bind(&leftEqualRight);
    {
        Label leftIsDouble(env);
        Label leftNotDoubleOrLeftNotNan(env);
        Branch(TaggedIsDouble(left), &leftIsDouble, &leftNotDoubleOrLeftNotNan);
        Bind(&leftIsDouble);
        {
            callback.ProfileOpType(Int32(PGOSampleType::DoubleType()));
            GateRef doubleLeft = GetDoubleOfTDouble(left);
            Label leftIsNan(env);
            Label leftIsNotNan(env);
            Branch(DoubleIsNAN(doubleLeft), &leftIsNan, &leftIsNotNan);
            Bind(&leftIsNan);
            {
                result = TaggedFalse();
                Jump(&exit);
            }
            Bind(&leftIsNotNan);
            {
                result = TaggedTrue();
                Jump(&exit);
            }
        }
        Bind(&leftNotDoubleOrLeftNotNan);
        {
            // Collect the type of left value
            result = TaggedTrue();
            if (callback.IsEmpty()) {
                Jump(&exit);
            } else {
                Label leftIsInt(env);
                Label leftIsNotInt(env);
                Branch(TaggedIsInt(left), &leftIsInt, &leftIsNotInt);
                Bind(&leftIsInt);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::IntType()));
                    Jump(&exit);
                }
                Bind(&leftIsNotInt);
                {
                    Label leftIsString(env);
                    Label leftIsNotString(env);
                    Branch(TaggedIsString(left), &leftIsString, &leftIsNotString);
                    Bind(&leftIsString);
                    {
                        callback.ProfileOpType(Int32(PGOSampleType::StringType()));
                        Jump(&exit);
                    }
                    Bind(&leftIsNotString);
                    {
                        callback.ProfileOpType(Int32(PGOSampleType::AnyType()));
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&leftNotEqualRight);
    {
        Label leftIsNumber(env);
        Label leftNotNumberOrLeftNotIntOrRightNotInt(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrLeftNotIntOrRightNotInt);
        Bind(&leftIsNumber);
        {
            Label leftIsInt(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Branch(TaggedIsInt(right), &rightIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::IntType()));
                    result = TaggedFalse();
                    Jump(&exit);
                }
            }
        }
        Bind(&leftNotNumberOrLeftNotIntOrRightNotInt);
        {
            DEFVARIABLE(curType, VariableType::INT32(), Int32(PGOSampleType::None()));
            Label rightIsUndefinedOrNull(env);
            Label rightIsNotUndefinedOrNull(env);
            Branch(TaggedIsUndefinedOrNull(right), &rightIsUndefinedOrNull, &rightIsNotUndefinedOrNull);
            Bind(&rightIsUndefinedOrNull);
            {
                curType = Int32(PGOSampleType::UndefineOrNullType());
                Label leftIsHeapObject(env);
                Label leftNotHeapObject(env);
                Branch(TaggedIsHeapObject(left), &leftIsHeapObject, &leftNotHeapObject);
                Bind(&leftIsHeapObject);
                {
                    GateRef type = Int32(PGOSampleType::HeapObjectType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    result = TaggedFalse();
                    Jump(&exit);
                }
                Bind(&leftNotHeapObject);
                {
                    Label leftIsUndefinedOrNull(env);
                    Label leftIsNotUndefinedOrNull(env);
                    // if left is undefined or null, then result is true, otherwise result is false
                    Branch(TaggedIsUndefinedOrNull(left), &leftIsUndefinedOrNull, &leftIsNotUndefinedOrNull);
                    Bind(&leftIsUndefinedOrNull);
                    {
                        callback.ProfileOpType(*curType);
                        result = TaggedTrue();
                        Jump(&exit);
                    }
                    Bind(&leftIsNotUndefinedOrNull);
                    {
                        callback.ProfileOpType(Int32(PGOSampleType::AnyType()));
                        result = TaggedFalse();
                        Jump(&exit);
                    }
                }
            }
            Bind(&rightIsNotUndefinedOrNull);
            {
                Label leftIsUndefinedOrNull(env);
                Label leftIsNotUndefinedOrNull(env);
                Branch(TaggedIsUndefinedOrNull(right), &leftIsUndefinedOrNull, &leftIsNotUndefinedOrNull);
                // If left is undefined or null, result will always be false
                // because we can ensure that right is not null here.
                Bind(&leftIsUndefinedOrNull);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::AnyType()));
                    result = TaggedFalse();
                    Jump(&exit);
                }
                Bind(&leftIsNotUndefinedOrNull);
                {
                    Label leftIsBool(env);
                    Label leftNotBoolOrRightNotSpecial(env);
                    Branch(TaggedIsBoolean(left), &leftIsBool, &leftNotBoolOrRightNotSpecial);
                    Bind(&leftIsBool);
                    {
                        curType = Int32(PGOSampleType::BooleanType());
                        Label rightIsSpecial(env);
                        Branch(TaggedIsSpecial(right), &rightIsSpecial, &leftNotBoolOrRightNotSpecial);
                        Bind(&rightIsSpecial);
                        {
                            GateRef type = Int32(PGOSampleType::SpecialType());
                            COMBINE_TYPE_CALL_BACK(curType, type);
                            result = TaggedFalse();
                            Jump(&exit);
                        }
                    }
                    Bind(&leftNotBoolOrRightNotSpecial);
                    {
                        Label bothString(env);
                        Label eitherNotString(env);
                        Branch(BothAreString(left, right), &bothString, &eitherNotString);
                        Bind(&bothString);
                        {
                            callback.ProfileOpType(Int32(PGOSampleType::StringType()));
                            Label stringEqual(env);
                            Label stringNotEqual(env);
                            Branch(FastStringEqual(glue, left, right), &stringEqual, &stringNotEqual);
                            Bind(&stringEqual);
                            result = TaggedTrue();
                            Jump(&exit);
                            Bind(&stringNotEqual);
                            result = TaggedFalse();
                            Jump(&exit);
                        }
                        Bind(&eitherNotString);
                        callback.ProfileOpType(Int32(PGOSampleType::AnyType()));
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastToBoolean(GateRef value, bool flag)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);

    Label isSpecial(env);
    Label notSpecial(env);
    Label isNumber(env);
    Label isInt(env);
    Label isDouble(env);
    Label notNumber(env);
    Label notNan(env);
    Label isString(env);
    Label notString(env);
    Label isBigint(env);
    Label lengthIsOne(env);
    Label returnTrue(env);
    Label returnFalse(env);

    Branch(TaggedIsSpecial(value), &isSpecial, &notSpecial);
    Bind(&isSpecial);
    {
        Branch(TaggedIsTrue(value), &returnTrue, &returnFalse);
    }
    Bind(&notSpecial);
    {
        Branch(TaggedIsNumber(value), &isNumber, &notNumber);
        Bind(&notNumber);
        {
            Branch(IsString(value), &isString, &notString);
            Bind(&isString);
            {
                auto len = GetLengthFromString(value);
                Branch(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            Branch(TaggedObjectIsBigInt(value), &isBigint, &returnTrue);
            Bind(&isBigint);
            {
                auto len = Load(VariableType::INT32(), value, IntPtr(BigInt::LENGTH_OFFSET));
                Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data = PtrAdd(value, IntPtr(BigInt::DATA_OFFSET));
                    auto data0 = Load(VariableType::INT32(), data, Int32(0));
                    Branch(Int32Equal(data0, Int32(0)), &returnFalse, &returnTrue);
                }
            }
        }
        Bind(&isNumber);
        {
            Branch(TaggedIsInt(value), &isInt, &isDouble);
            Bind(&isInt);
            {
                auto intValue = GetInt32OfTInt(value);
                Branch(Int32Equal(intValue, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&isDouble);
            {
                auto doubleValue = GetDoubleOfTDouble(value);
                Branch(DoubleIsNAN(doubleValue), &returnFalse, &notNan);
                Bind(&notNan);
                Branch(DoubleEqual(doubleValue, Double(0.0)), &returnFalse, &returnTrue);
            }
        }
    }
    if (flag == 1) {
        Bind(&returnTrue);
        {
            result = TaggedTrue();
            Jump(&exit);
        }
        Bind(&returnFalse);
        {
            result = TaggedFalse();
            Jump(&exit);
        }
    } else {
        Bind(&returnFalse);
        {
            result = TaggedTrue();
            Jump(&exit);
        }
        Bind(&returnTrue);
        {
            result = TaggedFalse();
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastDiv(GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(curType, VariableType::INT32(), Int32(PGOSampleType::None()));
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Label leftIsNumberAndRightIsNumber(env);
    Label leftIsDoubleAndRightIsDouble(env);
    Label exit(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftNotInt(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftNotInt);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label bailout(env);
                Branch(TaggedIsInt(right), &rightIsInt, &bailout);
                Bind(&rightIsInt);
                {
                    result = FastIntDiv(left, right, &bailout, callback);
                    Jump(&exit);
                }
                Bind(&bailout);
                {
                    curType = Int32(PGOSampleType::IntOverFlowType());
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    Jump(&leftIsNumberAndRightIsNumber);
                }
            }
            Bind(&leftNotInt);
            {
                curType = Int32(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
                Jump(&leftIsNumberAndRightIsNumber);
            }
        }
    }
    Bind(&leftNotNumberOrRightNotNumber);
    {
        Jump(&exit);
    }
    Bind(&leftIsNumberAndRightIsNumber);
    {
        Label rightIsInt(env);
        Label rightNotInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &rightNotInt);
        Bind(&rightIsInt);
        {
            GateRef type = Int32(PGOSampleType::IntType());
            COMBINE_TYPE_CALL_BACK(curType, type);
            doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
            Jump(&leftIsDoubleAndRightIsDouble);
        }
        Bind(&rightNotInt);
        {
            GateRef type = Int32(PGOSampleType::DoubleType());
            COMBINE_TYPE_CALL_BACK(curType, type);
            doubleRight = GetDoubleOfTDouble(right);
            Jump(&leftIsDoubleAndRightIsDouble);
        }
    }
    Bind(&leftIsDoubleAndRightIsDouble);
    {
        Label rightIsZero(env);
        Label rightNotZero(env);
        Branch(DoubleEqual(*doubleRight, Double(0.0)), &rightIsZero, &rightNotZero);
        Bind(&rightIsZero);
        {
            Label leftIsZero(env);
            Label leftNotZero(env);
            Label leftIsZeroOrNan(env);
            Label leftNotZeroAndNotNan(env);
            Branch(DoubleEqual(*doubleLeft, Double(0.0)), &leftIsZero, &leftNotZero);
            Bind(&leftIsZero);
            {
                Jump(&leftIsZeroOrNan);
            }
            Bind(&leftNotZero);
            {
                Label leftIsNan(env);
                Branch(DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
                Bind(&leftIsNan);
                {
                    Jump(&leftIsZeroOrNan);
                }
            }
            Bind(&leftIsZeroOrNan);
            {
                result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
                Jump(&exit);
            }
            Bind(&leftNotZeroAndNotNan);
            {
                GateRef intLeftTmp = CastDoubleToInt64(*doubleLeft);
                GateRef intRightTmp = CastDoubleToInt64(*doubleRight);
                GateRef flagBit = Int64And(Int64Xor(intLeftTmp, intRightTmp), Int64(base::DOUBLE_SIGN_MASK));
                GateRef tmpResult = Int64Xor(flagBit, CastDoubleToInt64(Double(base::POSITIVE_INFINITY)));
                result = DoubleToTaggedDoublePtr(CastInt64ToFloat64(tmpResult));
                Jump(&exit);
            }
        }
        Bind(&rightNotZero);
        {
            result = DoubleToTaggedDoublePtr(DoubleDiv(*doubleLeft, *doubleRight));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode Op>
GateRef StubBuilder::FastBinaryOp(GateRef glue, GateRef left, GateRef right,
                                  const BinaryOperation& intOp,
                                  const BinaryOperation& floatOp,
                                  ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));

    Label exit(env);
    Label doFloatOp(env);
    Label doIntOp(env);
    Label leftIsNumber(env);
    Label rightIsNumber(env);
    Label leftIsIntRightIsDouble(env);
    Label rightIsInt(env);
    Label rightIsDouble(env);
    Label checkString(env);
    if (Op == OpCode::ADD) {
        Branch(TaggedIsNumber(left), &leftIsNumber, &checkString);
    } else {
        Branch(TaggedIsNumber(left), &leftIsNumber, &exit);
    }
    Bind(&leftIsNumber);
    {
        Branch(TaggedIsNumber(right), &rightIsNumber, &exit);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Branch(TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
                Bind(&leftIsIntRightIsDouble);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::NumberType()));
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&doFloatOp);
                }
            }
            Bind(&leftIsDouble);
            {
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::NumberType()));
                    doubleLeft = GetDoubleOfTDouble(left);
                    doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                    Jump(&doFloatOp);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::DoubleType()));
                    doubleLeft = GetDoubleOfTDouble(left);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&doFloatOp);
                }
            }
        }
    }
    if (Op == OpCode::ADD) {
        Bind(&checkString);
        {
            Label stringAdd(env);
            Label hasPendingException(env);
            GateRef bothString = BoolAnd(TaggedIsString(left), TaggedIsString(right));
            Branch(bothString, &stringAdd, &exit);
            Bind(&stringAdd);
            {
                callback.ProfileOpType(Int32(PGOSampleType::StringType()));
                BuiltinsStringStubBuilder builtinsStringStubBuilder(this);
                result = builtinsStringStubBuilder.StringConcat(glue, left, right);
                Branch(HasPendingException(glue), &hasPendingException, &exit);
                Bind(&hasPendingException);
                result = Exception();
                Jump(&exit);
            }
        }
    }
    Bind(&doIntOp);
    {
        result = intOp(env, left, right);
        Jump(&exit);
    }
    Bind(&doFloatOp);
    {
        result = floatOp(env, *doubleLeft, *doubleRight);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode Op>
GateRef StubBuilder::FastAddSubAndMul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto intOperation = [=](Environment *env, GateRef left, GateRef right) {
        Label entry(env);
        env->SubCfgEntry(&entry);
        DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
        Label exit(env);
        Label overflow(env);
        Label notOverflow(env);
        auto res = BinaryOpWithOverflow<Op, MachineType::I32>(GetInt32OfTInt(left), GetInt32OfTInt(right));
        GateRef condition = env->GetBuilder()->ExtractValue(MachineType::I1, res, Int32(1));
        Branch(condition, &overflow, &notOverflow);
        Bind(&overflow);
        {
            auto doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
            auto doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
            auto ret = BinaryOp<Op, MachineType::F64>(doubleLeft, doubleRight);
            result = DoubleToTaggedDoublePtr(ret);
            callback.ProfileOpType(Int32(PGOSampleType::IntOverFlowType()));
            Jump(&exit);
        }
        Bind(&notOverflow);
        {
            res = env->GetBuilder()->ExtractValue(MachineType::I32, res, Int32(0));
            if (Op == OpCode::MUL) {
                Label resultIsZero(env);
                Label returnNegativeZero(env);
                Label returnResult(env);
                Branch(Int32Equal(res, Int32(0)), &resultIsZero, &returnResult);
                Bind(&resultIsZero);
                GateRef leftNegative = Int32LessThan(GetInt32OfTInt(left), Int32(0));
                GateRef rightNegative = Int32LessThan(GetInt32OfTInt(right), Int32(0));
                Branch(BoolOr(leftNegative, rightNegative), &returnNegativeZero, &returnResult);
                Bind(&returnNegativeZero);
                result = DoubleToTaggedDoublePtr(Double(-0.0));
                callback.ProfileOpType(Int32(PGOSampleType::DoubleType()));
                Jump(&exit);
                Bind(&returnResult);
                result = IntToTaggedPtr(res);
                callback.ProfileOpType(Int32(PGOSampleType::IntType()));
                Jump(&exit);
            } else {
                result = IntToTaggedPtr(res);
                callback.ProfileOpType(Int32(PGOSampleType::IntType()));
                Jump(&exit);
            }
        }
        Bind(&exit);
        auto ret = *result;
        env->SubCfgExit();
        return ret;
    };
    auto floatOperation = [=]([[maybe_unused]] Environment *env, GateRef left, GateRef right) {
        auto res = BinaryOp<Op, MachineType::F64>(left, right);
        return DoubleToTaggedDoublePtr(res);
    };
    return FastBinaryOp<Op>(glue, left, right, intOperation, floatOperation, callback);
}

GateRef StubBuilder::FastIntDiv(GateRef left, GateRef right, Label *bailout, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(intResult, VariableType::INT32(), Int32(0));

    GateRef intLeft = GetInt32OfTInt(left);
    GateRef intRight = GetInt32OfTInt(right);
    Label exit(env);
    Label rightIsNotZero(env);
    Label leftIsIntMin(env);
    Label leftAndRightIsNotBoundary(env);
    Branch(Int32Equal(intRight, Int32(0)), bailout, &rightIsNotZero);
    Bind(&rightIsNotZero);
    Branch(Int32Equal(intLeft, Int32(INT_MIN)), &leftIsIntMin, &leftAndRightIsNotBoundary);
    Bind(&leftIsIntMin);
    Branch(Int32Equal(intRight, Int32(-1)), bailout, &leftAndRightIsNotBoundary);
    Bind(&leftAndRightIsNotBoundary);
    {
        Label leftIsZero(env);
        Label leftIsNotZero(env);
        Branch(Int32Equal(intLeft, Int32(0)), &leftIsZero, &leftIsNotZero);
        Bind(&leftIsZero);
        {
            Branch(Int32LessThan(intRight, Int32(0)), bailout, &leftIsNotZero);
        }
        Bind(&leftIsNotZero);
        {
            intResult = Int32Div(intLeft, intRight);
            GateRef truncated = Int32Mul(*intResult, intRight);
            Branch(Equal(intLeft, truncated), &exit, bailout);
        }
    }
    Bind(&exit);
    callback.ProfileOpType(Int32(PGOSampleType::IntType()));
    auto ret = IntToTaggedPtr(*intResult);
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastAdd(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    return FastAddSubAndMul<OpCode::ADD>(glue, left, right, callback);
}

GateRef StubBuilder::FastSub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    return FastAddSubAndMul<OpCode::SUB>(glue, left, right, callback);
}

GateRef StubBuilder::FastMul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    return FastAddSubAndMul<OpCode::MUL>(glue, left, right, callback);
}

GateRef StubBuilder::FastMod(GateRef glue, GateRef left, GateRef right, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(intLeft, VariableType::INT32(), Int32(0));
    DEFVARIABLE(intRight, VariableType::INT32(), Int32(0));
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
    Label leftIsInt(env);
    Label leftNotIntOrRightNotInt(env);
    Label exit(env);
    Branch(TaggedIsInt(left), &leftIsInt, &leftNotIntOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftNotIntOrRightNotInt);
        Bind(&rightIsInt);
        {
            intLeft = GetInt32OfTInt(left);
            intRight = GetInt32OfTInt(right);
            Label leftGreaterZero(env);
            Branch(Int32GreaterThanOrEqual(*intLeft, Int32(0)), &leftGreaterZero, &leftNotIntOrRightNotInt);
            Bind(&leftGreaterZero);
            {
                Label rightGreaterZero(env);
                Branch(Int32GreaterThan(*intRight, Int32(0)), &rightGreaterZero, &leftNotIntOrRightNotInt);
                Bind(&rightGreaterZero);
                {
                    callback.ProfileOpType(Int32(PGOSampleType::IntType()));
                    result = IntToTaggedPtr(Int32Mod(*intLeft, *intRight));
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&leftNotIntOrRightNotInt);
    {
        Label leftIsNumber(env);
        Label leftNotNumberOrRightNotNumber(env);
        Label leftIsNumberAndRightIsNumber(env);
        Label leftIsDoubleAndRightIsDouble(env);
        DEFVARIABLE(curType, VariableType::INT32(), Int32(PGOSampleType::None()));
        // less than 0 result should be double
        curType = Int32(PGOSampleType::DoubleType());
        Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
            Bind(&rightIsNumber);
            {
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    GateRef type = Int32(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    Jump(&leftIsNumberAndRightIsNumber);
                }
                Bind(&leftNotInt1);
                {
                    GateRef type = Int32(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleLeft = GetDoubleOfTDouble(left);
                    Jump(&leftIsNumberAndRightIsNumber);
                }
            }
        }
        Bind(&leftNotNumberOrRightNotNumber);
        {
            Jump(&exit);
        }
        Bind(&leftIsNumberAndRightIsNumber);
        {
            Label rightIsInt1(env);
            Label rightNotInt1(env);
            Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
            Bind(&rightIsInt1);
            {
                GateRef type = Int32(PGOSampleType::IntType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                Jump(&leftIsDoubleAndRightIsDouble);
            }
            Bind(&rightNotInt1);
            {
                GateRef type = Int32(PGOSampleType::DoubleType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = GetDoubleOfTDouble(right);
                Jump(&leftIsDoubleAndRightIsDouble);
            }
        }
        Bind(&leftIsDoubleAndRightIsDouble);
        {
            Label rightNotZero(env);
            Label rightIsZeroOrNanOrLeftIsNanOrInf(env);
            Label rightNotZeroAndNanAndLeftNotNanAndInf(env);
            Branch(DoubleEqual(*doubleRight, Double(0.0)), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotZero);
            Bind(&rightNotZero);
            {
                Label rightNotNan(env);
                Branch(DoubleIsNAN(*doubleRight), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotNan);
                Bind(&rightNotNan);
                {
                    Label leftNotNan(env);
                    Branch(DoubleIsNAN(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf, &leftNotNan);
                    Bind(&leftNotNan);
                    {
                        Branch(DoubleIsINF(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf,
                            &rightNotZeroAndNanAndLeftNotNanAndInf);
                    }
                }
            }
            Bind(&rightIsZeroOrNanOrLeftIsNanOrInf);
            {
                result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
                Jump(&exit);
            }
            Bind(&rightNotZeroAndNanAndLeftNotNanAndInf);
            {
                Label leftNotZero(env);
                Label leftIsZeroOrRightIsInf(env);
                Branch(DoubleEqual(*doubleLeft, Double(0.0)), &leftIsZeroOrRightIsInf, &leftNotZero);
                Bind(&leftNotZero);
                {
                    Label rightNotInf(env);
                    Branch(DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
                    Bind(&rightNotInf);
                    {
                        result = DoubleToTaggedDoublePtr(CallNGCRuntime(glue, RTSTUB_ID(FloatMod),
                            { *doubleLeft, *doubleRight }));
                        Jump(&exit);
                    }
                }
                Bind(&leftIsZeroOrRightIsInf);
                {
                    result = DoubleToTaggedDoublePtr(*doubleLeft);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef properties = GetPropertiesFromJSObject(receiver);
    GateRef entry = FindEntryFromNameDictionary(glue, properties, key);
    Label notNegtiveOne(env);
    Label exit(env);
    Branch(Int32NotEqual(entry, Int32(-1)), &notNegtiveOne, &exit);
    Bind(&notNegtiveOne);
    {
        result = GetValueFromGlobalDictionary(properties, entry);
        Label callGetter(env);
        Branch(TaggedIsAccessor(*result), &callGetter, &exit);
        Bind(&callGetter);
        {
            result = CallGetterHelper(glue, receiver, receiver, *result, callback);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetConstPoolFromFunction(GateRef jsFunc)
{
    return env_->GetBuilder()->GetConstPoolFromFunction(jsFunc);
}

GateRef StubBuilder::GetStringFromConstPool(GateRef glue, GateRef constpool, GateRef index)
{
    GateRef module = Circuit::NullGate();
    GateRef hirGate = Circuit::NullGate();
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, module, index, ConstPoolType::STRING);
}

GateRef StubBuilder::GetMethodFromConstPool(GateRef glue, GateRef constpool, GateRef module, GateRef index)
{
    GateRef hirGate = Circuit::NullGate();
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, module, index, ConstPoolType::METHOD);
}

GateRef StubBuilder::GetArrayLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module)
{
    GateRef hirGate = Circuit::NullGate();
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, module, index,
                                                      ConstPoolType::ARRAY_LITERAL);
}

GateRef StubBuilder::GetObjectLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module)
{
    GateRef hirGate = Circuit::NullGate();
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, module, index,
                                                      ConstPoolType::OBJECT_LITERAL);
}

GateRef StubBuilder::JSAPIContainerGet(GateRef glue, GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());

    GateRef lengthOffset = IntPtr(panda::ecmascript::JSAPIArrayList::LENGTH_OFFSET);
    GateRef length = GetInt32OfTInt(Load(VariableType::INT64(), receiver, lengthOffset));
    Label isVailedIndex(env);
    Label notValidIndex(env);
    Branch(BoolAnd(Int32GreaterThanOrEqual(index, Int32(0)),
        Int32UnsignedLessThan(index, length)), &isVailedIndex, &notValidIndex);
    Bind(&isVailedIndex);
    {
        GateRef elements = GetElementsArray(receiver);
        result = GetValueFromTaggedArray(elements, index);
        Jump(&exit);
    }
    Bind(&notValidIndex);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(GetPropertyOutOfBounds));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result = Exception();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetEnumCacheKind(GateRef glue, GateRef enumCache)
{
    return env_->GetBuilder()->GetEnumCacheKind(glue, enumCache);
}

GateRef StubBuilder::IsEnumCacheValid(GateRef receiver, GateRef cachedHclass, GateRef kind)
{
    return env_->GetBuilder()->IsEnumCacheValid(receiver, cachedHclass, kind);
}

GateRef StubBuilder::NeedCheckProperty(GateRef receiver)
{
    return env_->GetBuilder()->NeedCheckProperty(receiver);
}

GateRef StubBuilder::NextInternal(GateRef glue, GateRef iter)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label notFinish(env);
    Label notEnumCacheValid(env);
    Label fastGetKey(env);
    Label slowpath(env);

    GateRef index = GetIndexFromForInIterator(iter);
    GateRef length = GetLengthFromForInIterator(iter);
    Branch(Int32GreaterThanOrEqual(index, length), &exit, &notFinish);
    Bind(&notFinish);
    GateRef keys = GetKeysFromForInIterator(iter);
    GateRef receiver = GetObjectFromForInIterator(iter);
    GateRef cachedHclass = GetCachedHclassFromForInIterator(iter);
    GateRef kind = GetEnumCacheKind(glue, keys);
    Branch(IsEnumCacheValid(receiver, cachedHclass, kind), &fastGetKey, &notEnumCacheValid);
    Bind(&notEnumCacheValid);
    Branch(NeedCheckProperty(receiver), &slowpath, &fastGetKey);
    Bind(&fastGetKey);
    {
        result = GetValueFromTaggedArray(keys, index);
        IncreaseInteratorIndex(glue, iter, index);
        Jump(&exit);
    }
    Bind(&slowpath);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetNextPropNameSlowpath), { iter });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetFunctionPrototype(GateRef glue, size_t index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label isHeapObject(env);
    Label isJSHclass(env);

    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env_->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef func = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, index);
    GateRef protoOrHclass = Load(VariableType::JS_ANY(), func, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    result = protoOrHclass;
    Branch(TaggedIsHeapObject(protoOrHclass), &isHeapObject, &exit);
    Bind(&isHeapObject);
    Branch(IsJSHClass(protoOrHclass), &isJSHclass, &exit);
    Bind(&isJSHclass);
    {
        result = GetPrototypeFromHClass(protoOrHclass);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ToObject(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), obj);
    DEFVARIABLE(taggedId, VariableType::INT32(), Int32(0));
    Label isNumber(env);
    Label notNumber(env);
    Label isBoolean(env);
    Label notBoolean(env);
    Label isString(env);
    Label notString(env);
    Label isECMAObject(env);
    Label notIsECMAObject(env);
    Label isSymbol(env);
    Label notSymbol(env);
    Label isUndefined(env);
    Label notIsUndefined(env);
    Label isNull(env);
    Label notIsNull(env);
    Label isHole(env);
    Label notIsHole(env);
    Label isBigInt(env);
    Label notIsBigInt(env);
    Label throwError(env);
    Branch(IsEcmaObject(obj), &isECMAObject, &notIsECMAObject);
    Bind(&isECMAObject);
    {
        result = obj;
        Jump(&exit);
    }
    Bind(&notIsECMAObject);
    Branch(TaggedIsNumber(obj), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = NewJSPrimitiveRef(glue, GlobalEnv::NUMBER_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notNumber);
    Branch(TaggedIsBoolean(obj), &isBoolean, &notBoolean);
    Bind(&isBoolean);
    {
        result = NewJSPrimitiveRef(glue, GlobalEnv::BOOLEAN_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notBoolean);
    Branch(TaggedIsString(obj), &isString, &notString);
    Bind(&isString);
    {
        result = NewJSPrimitiveRef(glue, GlobalEnv::STRING_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notString);
    Branch(TaggedIsSymbol(obj), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        result = NewJSPrimitiveRef(glue, GlobalEnv::SYMBOL_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notSymbol);
    Branch(TaggedIsUndefined(obj), &isUndefined, &notIsUndefined);
    Bind(&isUndefined);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotUndefinedObject));
        Jump(&throwError);
    }
    Bind(&notIsUndefined);
    Branch(TaggedIsHole(obj), &isHole, &notIsHole);
    Bind(&isHole);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotHoleObject));
        Jump(&throwError);
    }
    Bind(&notIsHole);
    Branch(TaggedIsNull(obj), &isNull, &notIsNull);
    Bind(&isNull);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotNullObject));
        Jump(&throwError);
    }
    Bind(&notIsNull);
    Branch(TaggedIsBigInt(obj), &isBigInt, &notIsBigInt);
    Bind(&isBigInt);
    {
        result = NewJSPrimitiveRef(glue, GlobalEnv::BIGINT_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notIsBigInt);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotNullObject));
        Jump(&throwError);
    }
    Bind(&throwError);
    {
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(*taggedId) });
        result = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::NewJSPrimitiveRef(GateRef glue, size_t index, GateRef obj)
{
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env_->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef func = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, index);
    GateRef protoOrHclass = Load(VariableType::JS_ANY(), func, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(env_);
    GateRef newObj  = newBuilder.NewJSObject(glue, protoOrHclass);
    GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
    Store(VariableType::JS_ANY(), glue, newObj, valueOffset, obj);
    return newObj;
}

GateRef StubBuilder::DeletePropertyOrThrow(GateRef glue, GateRef obj, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    DEFVARIABLE(key, VariableType::JS_ANY(), value);
    Label toObject(env);
    Label isNotExceptiont(env);
    Label objectIsEcmaObject(env);
    Label objectIsHeapObject(env);
    GateRef object = ToObject(glue, obj);
    Branch(TaggedIsException(object), &exit, &isNotExceptiont);
    Bind(&isNotExceptiont);
    {
        Label deleteProper(env);
        Label notStringOrSymbol(env);
        Label notPrimitive(env);
        Branch(TaggedIsStringOrSymbol(value), &deleteProper, &notStringOrSymbol);
        Bind(&notStringOrSymbol);
        {
            Branch(TaggedIsNumber(value), &deleteProper, &notPrimitive);
            Bind(&notPrimitive);
            {
                key = CallRuntime(glue, RTSTUB_ID(ToPropertyKey), {value});
                Branch(TaggedIsException(*key), &exit, &deleteProper);
            }
        }
        Bind(&deleteProper);
        {
            result = DeleteProperty(glue, object, *key);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::DeleteProperty(GateRef glue, GateRef obj, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label notRegularJSObject(env);
    Label regularJSObjDeletePrototype(env);
    Branch(TaggedIsRegularObject(obj), &regularJSObjDeletePrototype, &notRegularJSObject);
    Bind(&regularJSObjDeletePrototype);
    {
        result = CallRuntime(glue, RTSTUB_ID(RegularJSObjDeletePrototype), { obj, value});
        Jump(&exit);
    }
    Bind(&notRegularJSObject);
    {
        result = CallRuntime(glue, RTSTUB_ID(CallJSObjDeletePrototype), { obj, value});
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ToPrototypeOrObj(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), obj);

    Label isNumber(env);
    Label notNumber(env);
    Label isBoolean(env);
    Label notBoolean(env);
    Label isString(env);
    Label notString(env);
    Label isSymbol(env);
    Label notSymbol(env);
    Label isBigInt(env);
    Branch(TaggedIsNumber(obj), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::NUMBER_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notNumber);
    Branch(TaggedIsBoolean(obj), &isBoolean, &notBoolean);
    Bind(&isBoolean);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::BOOLEAN_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notBoolean);
    Branch(TaggedIsString(obj), &isString, &notString);
    Bind(&isString);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::STRING_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notString);
    Branch(TaggedIsSymbol(obj), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::SYMBOL_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notSymbol);
    Branch(TaggedIsBigInt(obj), &isBigInt, &exit);
    Bind(&isBigInt);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::BIGINT_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsSpecialKeysObject(GateRef obj)
{
    return BoolOr(BoolOr(IsTypedArray(obj), IsModuleNamespace(obj)), ObjIsSpecialContainer(obj));
}

GateRef StubBuilder::IsSlowKeysObject(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());

    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = BoolOr(BoolOr(IsJSGlobalObject(obj), IsJsProxy(obj)), IsSpecialKeysObject(obj));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetNumberOfElements(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(numOfElements, VariableType::INT32(), Int32(0));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label isJSPrimitiveRef(env);
    Label isPrimitiveString(env);
    Label notPrimitiveString(env);
    Label isDictMode(env);
    Label notDictMode(env);

    Branch(IsJSPrimitiveRef(obj), &isJSPrimitiveRef, &notPrimitiveString);
    Bind(&isJSPrimitiveRef);
    GateRef value = Load(VariableType::JS_ANY(), obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
    Branch(TaggedIsString(value), &isPrimitiveString, &notPrimitiveString);
    Bind(&isPrimitiveString);
    {
        numOfElements = GetLengthFromString(value);
        Jump(&notPrimitiveString);
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
        GateRef elementsLen = GetLengthOfTaggedArray(elements);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int32UnsignedLessThan(*i, elementsLen), &iLessLength, &exit);
            Bind(&iLessLength);
            {
                GateRef element = GetValueFromTaggedArray(elements, *i);
                Branch(TaggedIsHole(element), &loopEnd, &notHole);
                Bind(&notHole);
                numOfElements = Int32Add(*numOfElements, Int32(1));
                Jump(&loopEnd);
            }
            Bind(&loopEnd);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&isDictMode);
    {
        GateRef entryCount = TaggedGetInt(
            GetValueFromTaggedArray(elements, Int32(TaggedHashTable<NumberDictionary>::NUMBER_OF_ENTRIES_INDEX)));
        numOfElements = Int32Add(*numOfElements, entryCount);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *numOfElements;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsSimpleEnumCacheValid(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    DEFVARIABLE(current, VariableType::JS_ANY(), Undefined());

    Label receiverHasNoElements(env);

    GateRef numOfElements = GetNumberOfElements(obj);
    Branch(Int32GreaterThan(numOfElements, Int32(0)), &exit, &receiverHasNoElements);
    Bind(&receiverHasNoElements);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Label currentHasNoElements(env);
        Label enumCacheIsUndefined(env);
        current = GetPrototypeFromHClass(LoadHClass(obj));
        Branch(TaggedIsHeapObject(*current), &loopHead, &afterLoop);
        LoopBegin(&loopHead);
        {
            GateRef numOfCurrentElements = GetNumberOfElements(*current);
            Branch(Int32GreaterThan(numOfCurrentElements, Int32(0)), &exit, &currentHasNoElements);
            Bind(&currentHasNoElements);
            GateRef hclass = LoadHClass(*current);
            GateRef protoEnumCache = GetEnumCacheFromHClass(hclass);
            Branch(TaggedIsUndefined(protoEnumCache), &enumCacheIsUndefined, &exit);
            Bind(&enumCacheIsUndefined);
            current = GetPrototypeFromHClass(hclass);
            Branch(TaggedIsHeapObject(*current), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsEnumCacheWithProtoChainInfoValid(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    DEFVARIABLE(current, VariableType::JS_ANY(), Undefined());

    Label receiverHasNoElements(env);
    Label prototypeIsEcmaObj(env);
    Label isProtoChangeMarker(env);
    Label protoNotChanged(env);

    GateRef numOfElements = GetNumberOfElements(obj);
    Branch(Int32GreaterThan(numOfElements, Int32(0)), &exit, &receiverHasNoElements);
    Bind(&receiverHasNoElements);
    GateRef prototype = GetPrototypeFromHClass(LoadHClass(obj));
    Branch(IsEcmaObject(prototype), &prototypeIsEcmaObj, &exit);
    Bind(&prototypeIsEcmaObj);
    GateRef protoChangeMarker = GetProtoChangeMarkerFromHClass(LoadHClass(prototype));
    Branch(TaggedIsProtoChangeMarker(protoChangeMarker), &isProtoChangeMarker, &exit);
    Bind(&isProtoChangeMarker);
    Branch(GetHasChanged(protoChangeMarker), &exit, &protoNotChanged);
    Bind(&protoNotChanged);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Label currentHasNoElements(env);
        current = prototype;
        Branch(TaggedIsHeapObject(*current), &loopHead, &afterLoop);
        LoopBegin(&loopHead);
        {
            GateRef numOfCurrentElements = GetNumberOfElements(*current);
            Branch(Int32GreaterThan(numOfCurrentElements, Int32(0)), &exit, &currentHasNoElements);
            Bind(&currentHasNoElements);
            current = GetPrototypeFromHClass(LoadHClass(*current));
            Branch(TaggedIsHeapObject(*current), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TryGetEnumCache(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label notSlowKeys(env);
    Label notDictionaryMode(env);
    Label checkSimpleEnumCache(env);
    Label notSimpleEnumCache(env);
    Label checkEnumCacheWithProtoChainInfo(env);
    Label enumCacheValid(env);

    Branch(IsSlowKeysObject(obj), &exit, &notSlowKeys);
    Bind(&notSlowKeys);
    GateRef hclass = LoadHClass(obj);
    Branch(IsDictionaryModeByHClass(hclass), &exit, &notDictionaryMode);
    Bind(&notDictionaryMode);
    GateRef enumCache = GetEnumCacheFromHClass(hclass);
    GateRef kind = GetEnumCacheKind(glue, enumCache);
    Branch(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::SIMPLE))),
           &checkSimpleEnumCache, &notSimpleEnumCache);
    Bind(&checkSimpleEnumCache);
    {
        Branch(IsSimpleEnumCacheValid(obj), &enumCacheValid, &exit);
    }
    Bind(&notSimpleEnumCache);
    Branch(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::PROTOCHAIN))),
           &checkEnumCacheWithProtoChainInfo, &exit);
    Bind(&checkEnumCacheWithProtoChainInfo);
    {
        Branch(IsEnumCacheWithProtoChainInfoValid(obj), &enumCacheValid, &exit);
    }
    Bind(&enumCacheValid);
    {
        result = enumCache;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::DoubleToInt(GateRef glue, GateRef x, size_t typeBits)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label overflow(env);

    GateRef xInt = ChangeFloat64ToInt32(x);
    DEFVARIABLE(result, VariableType::INT32(), xInt);

    if (env->IsAmd64()) {
        // 0x80000000: amd64 overflow return value
        Branch(Int32Equal(xInt, Int32(0x80000000)), &overflow, &exit);
    } else {
        GateRef xInt64 = CastDoubleToInt64(x);
        // exp = (u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_SIGNIFICAND_SIZE - DOUBLE_EXPONENT_BIAS
        GateRef exp = Int64And(xInt64, Int64(base::DOUBLE_EXPONENT_MASK));
        exp = TruncInt64ToInt32(Int64LSR(exp, Int64(base::DOUBLE_SIGNIFICAND_SIZE)));
        exp = Int32Sub(exp, Int32(base::DOUBLE_EXPONENT_BIAS));
        GateRef bits = Int32(typeBits - 1);
        // exp < 32 - 1
        Branch(Int32LessThan(exp, bits), &exit, &overflow);
    }
    Bind(&overflow);
    {
        result = CallNGCRuntime(glue, RTSTUB_ID(DoubleToInt), { x, IntPtr(typeBits) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::ReturnExceptionIfAbruptCompletion(GateRef glue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label hasPendingException(env);
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    Branch(TaggedIsNotHole(exception), &hasPendingException, &exit);
    Bind(&hasPendingException);
    Return(Exception());
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef StubBuilder::GetHashcodeFromString(GateRef glue, GateRef value)
{
    return env_->GetBuilder()->GetHashcodeFromString(glue, value);
}

GateRef StubBuilder::ConstructorCheck(GateRef glue, GateRef ctor, GateRef outPut, GateRef thisObj)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    Label isHeapObject(env);
    Label isEcmaObj(env);
    Label notEcmaObj(env);
    Branch(TaggedIsHeapObject(outPut), &isHeapObject, &notEcmaObj);
    Bind(&isHeapObject);
    Branch(TaggedObjectIsEcmaObject(outPut), &isEcmaObj, &notEcmaObj);
    Bind(&isEcmaObj);
    {
        result = outPut;
        Jump(&exit);
    }
    Bind(&notEcmaObj);
    {
        Label ctorIsBase(env);
        Label ctorNotBase(env);
        Branch(IsBase(ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            result = thisObj;
            Jump(&exit);
        }
        Bind(&ctorNotBase);
        {
            Label throwExeption(env);
            Label returnObj(env);
            Branch(TaggedIsUndefined(outPut), &returnObj, &throwExeption);
            Bind(&returnObj);
            result = thisObj;
            Jump(&exit);
            Bind(&throwExeption);
            {
                CallRuntime(glue, RTSTUB_ID(ThrowNonConstructorException), {});
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetIterator(GateRef glue, GateRef obj, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isPendingException(env);
    Label noPendingException(env);
    Label isHeapObject(env);
    Label objIsCallable(env);

    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef iteratorKey = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ITERATOR_SYMBOL_INDEX);
    result = FastGetPropertyByName(glue, obj, iteratorKey, ProfileOperation());
    Branch(HasPendingException(glue), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noPendingException);
    callback.ProfileGetIterator(*result);
    Branch(TaggedIsHeapObject(*result), &isHeapObject, &exit);
    Bind(&isHeapObject);
    Branch(IsCallable(*result), &objIsCallable, &exit);
    Bind(&objIsCallable);
    {
        result = JSCallDispatch(glue, *result, Int32(0), 0, Circuit::NullGate(),
                                JSCallMode::CALL_GETTER, { obj }, ProfileOperation());
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

bool StubBuilder::IsCallModeSupportPGO(JSCallMode mode)
{
    switch (mode) {
        case JSCallMode::CALL_ARG0:
        case JSCallMode::CALL_ARG1:
        case JSCallMode::CALL_ARG2:
        case JSCallMode::CALL_ARG3:
        case JSCallMode::CALL_WITH_ARGV:
        case JSCallMode::CALL_THIS_ARG0:
        case JSCallMode::CALL_THIS_ARG1:
        case JSCallMode::CALL_THIS_ARG2:
        case JSCallMode::CALL_THIS_ARG3:
        case JSCallMode::CALL_THIS_WITH_ARGV:
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::SUPER_CALL_WITH_ARGV:
        case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
            return true;
        case JSCallMode::DEPRECATED_CALL_ARG0:
        case JSCallMode::DEPRECATED_CALL_ARG1:
        case JSCallMode::DEPRECATED_CALL_ARG2:
        case JSCallMode::DEPRECATED_CALL_ARG3:
        case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
        case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_ENTRY:
        case JSCallMode::CALL_FROM_AOT:
        case JSCallMode::CALL_GENERATOR:
        case JSCallMode::CALL_GETTER:
        case JSCallMode::CALL_SETTER:
        case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
        case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            return false;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

GateRef StubBuilder::JSCallDispatch(GateRef glue, GateRef func, GateRef actualNumArgs, GateRef jumpSize,
                                    GateRef hotnessCounter, JSCallMode mode, std::initializer_list<GateRef> args,
                                    ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    // 1. call initialize
    Label funcIsHeapObject(env);
    Label funcIsCallable(env);
    Label funcNotCallable(env);
    // save pc
    SavePcIfNeeded(glue);
    GateRef bitfield = 0;
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue, RTSTUB_ID(StartCallTimer), { glue, func, False()});
#endif
    Branch(TaggedIsHeapObject(func), &funcIsHeapObject, &funcNotCallable);
    Bind(&funcIsHeapObject);
    GateRef hclass = LoadHClass(func);
    bitfield = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    Branch(IsCallableFromBitField(bitfield), &funcIsCallable, &funcNotCallable);
    Bind(&funcNotCallable);
    {
        CallRuntime(glue, RTSTUB_ID(ThrowNotCallableException), {});
        Jump(&exit);
    }
    Bind(&funcIsCallable);
    GateRef method = GetMethodFromJSFunction(func);
    GateRef callField = GetCallFieldFromMethod(method);
    GateRef isNativeMask = Int64(static_cast<uint64_t>(1) << MethodLiteral::IsNativeBit::START_BIT);

    // 2. call dispatch
    Label methodIsNative(env);
    Label methodNotNative(env);
    Branch(Int64NotEqual(Int64And(callField, isNativeMask), Int64(0)), &methodIsNative, &methodNotNative);
    auto data = std::begin(args);
    Label notFastBuiltinsArg0(env);
    Label notFastBuiltinsArg1(env);
    Label notFastBuiltinsArg2(env);
    Label notFastBuiltinsArg3(env);
    Label notFastBuiltins(env);
    // 3. call native
    Bind(&methodIsNative);
    {
        if (IsCallModeSupportPGO(mode)) {
            callback.ProfileNativeCall(func);
        }
        GateRef nativeCode = Load(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef newTarget = Undefined();
        GateRef thisValue = Undefined();
        GateRef numArgs = Int32Add(actualNumArgs, Int32(NUM_MANDATORY_JSFUNC_ARGS));
        switch (mode) {
            case JSCallMode::CALL_THIS_ARG0: {
                thisValue = data[0];
                CallFastPath(glue, nativeCode, func, thisValue, actualNumArgs, callField,
                    method, &notFastBuiltinsArg0, &exit, &result, args, mode);
                Bind(&notFastBuiltinsArg0);
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, thisValue });
                break;
            }
            case JSCallMode::CALL_ARG0:
            case JSCallMode::DEPRECATED_CALL_ARG0:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, thisValue });
                break;
            case JSCallMode::CALL_THIS_ARG1: {
                thisValue = data[1];
                CallFastPath(glue, nativeCode, func, thisValue, actualNumArgs, callField,
                    method, &notFastBuiltinsArg1, &exit, &result, args, mode);
                Bind(&notFastBuiltinsArg1);
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, thisValue, data[0]});
                break;
            }
            case JSCallMode::CALL_ARG1:
            case JSCallMode::DEPRECATED_CALL_ARG1:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, thisValue, data[0]});
                break;
            case JSCallMode::CALL_THIS_ARG2: {
                thisValue = data[2];
                CallFastPath(glue, nativeCode, func, thisValue, actualNumArgs, callField,
                    method, &notFastBuiltinsArg2, &exit, &result, args, mode);
                Bind(&notFastBuiltinsArg2);
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, thisValue, data[0], data[1] });
                break;
            }
            case JSCallMode::CALL_ARG2:
            case JSCallMode::DEPRECATED_CALL_ARG2:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, thisValue, data[0], data[1] });
                break;
            case JSCallMode::CALL_THIS_ARG3: {
                thisValue = data[3];
                CallFastPath(glue, nativeCode, func, thisValue, actualNumArgs, callField,
                    method, &notFastBuiltinsArg3, &exit, &result, args, mode);
                Bind(&notFastBuiltinsArg3);
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func,
                        newTarget, thisValue, data[0], data[1], data[2] }); // 2: args2
                break;
            }
            case JSCallMode::CALL_ARG3:
            case JSCallMode::DEPRECATED_CALL_ARG3:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func,
                      newTarget, thisValue, data[0], data[1], data[2] }); // 2: args2
                break;
            case JSCallMode::CALL_THIS_WITH_ARGV:
            case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
            case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV: {
                thisValue = data[2]; // 2: this input
                [[fallthrough]];
            }
            case JSCallMode::CALL_WITH_ARGV:
            case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallRangeAndDispatchNative),
                    { glue, nativeCode, func, thisValue, data[0], data[1] });
                break;
            case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV: {
                CallFastPath(glue, nativeCode, func, thisValue, actualNumArgs, callField,
                    method, &notFastBuiltins, &exit, &result, args, mode);
                Bind(&notFastBuiltins);
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallNewAndDispatchNative),
                    { glue, nativeCode, func, data[2], data[0], data[1] });
                break;
            }
            case JSCallMode::CALL_GETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, data[0] });
                break;
            case JSCallMode::CALL_SETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, data[0], data[1] });
                break;
            case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { nativeCode, glue, numArgs, func, newTarget, data[0], data[1], data[2], data[3] });
                break;
            case JSCallMode::SUPER_CALL_WITH_ARGV:
                result = CallRuntime(glue, RTSTUB_ID(SuperCall), { data[0], data[1], IntToTaggedInt(data[2]) });
                break;
            case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
                result = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), {data[0], data[1]});
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        Jump(&exit);
    }
    // 4. call nonNative
    Bind(&methodNotNative);

    if (IsCallModeSupportPGO(mode)) {
        callback.ProfileCall(func);
    }
    Label funcIsClassConstructor(env);
    Label funcNotClassConstructor(env);
    Label methodNotAot(env);
    if (!AssemblerModule::IsCallNew(mode)) {
        Branch(IsClassConstructorFromBitField(bitfield), &funcIsClassConstructor, &funcNotClassConstructor);
        Bind(&funcIsClassConstructor);
        {
            CallRuntime(glue, RTSTUB_ID(ThrowCallConstructorException), {});
            Jump(&exit);
        }
        Bind(&funcNotClassConstructor);
    } else {
        Branch(IsClassConstructorFromBitField(bitfield), &funcIsClassConstructor, &methodNotAot);
        Bind(&funcIsClassConstructor);
    }
    GateRef sp = 0;
    if (env->IsAsmInterp()) {
        sp = Argument(static_cast<size_t>(InterpreterHandlerInputs::SP));
    }
    Label methodisAot(env);
    Label methodIsFastCall(env);
    Label methodNotFastCall(env);
    Label fastCall(env);
    Label fastCallBridge(env);
    Label slowCall(env);
    Label slowCallBridge(env);
    {
        GateRef newTarget = Undefined();
        GateRef thisValue = Undefined();
        GateRef realNumArgs = Int64Add(ZExtInt32ToInt64(actualNumArgs), Int64(NUM_MANDATORY_JSFUNC_ARGS));
        Branch(JudgeAotAndFastCallWithMethod(method, CircuitBuilder::JudgeMethodType::HAS_AOT_FASTCALL),
            &methodIsFastCall, &methodNotFastCall);
        Bind(&methodIsFastCall);
        {
            GateRef expectedNum = Int64And(Int64LSR(callField, Int64(MethodLiteral::NumArgsBits::START_BIT)),
                Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
            GateRef expectedArgc = Int64Add(expectedNum, Int64(NUM_MANDATORY_JSFUNC_ARGS));
            Branch(Int64LessThanOrEqual(expectedArgc, realNumArgs), &fastCall, &fastCallBridge);
            Bind(&fastCall);
            {
                GateRef code = GetAotCodeAddr(func);
                switch (mode) {
                    case JSCallMode::CALL_THIS_ARG0:
                        thisValue = data[0];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG0:
                    case JSCallMode::DEPRECATED_CALL_ARG0:
                        result = FastCallOptimized(glue, code, { glue, func, thisValue});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG1:
                        thisValue = data[1];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG1:
                    case JSCallMode::DEPRECATED_CALL_ARG1:
                        result = FastCallOptimized(glue, code, { glue, func, thisValue, data[0] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG2:
                        thisValue = data[2];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG2:
                    case JSCallMode::DEPRECATED_CALL_ARG2:
                        result = FastCallOptimized(glue, code, { glue, func, thisValue, data[0], data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3:
                        thisValue = data[3];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG3:
                    case JSCallMode::DEPRECATED_CALL_ARG3:
                        result = FastCallOptimized(glue, code, { glue, func, thisValue, data[0], data[1], data[2] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_WITH_ARGV:
                    case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
                    case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
                        thisValue = data[2]; // 2: this input
                        [[fallthrough]];
                    case JSCallMode::CALL_WITH_ARGV:
                    case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgV),
                            { glue, func, thisValue, ZExtInt32ToInt64(actualNumArgs), data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
                    case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgV),
                            { glue, func, data[2], ZExtInt32ToInt64(actualNumArgs), data[1]});
                        result = ConstructorCheck(glue, func, *result, data[2]);  // 2: the second index
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_GETTER:
                        result = FastCallOptimized(glue, code, { glue, func, data[0] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_SETTER:
                        result = FastCallOptimized(glue, code, { glue, func, data[0], data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
                        result = FastCallOptimized(glue, code, { glue, func, data[0], data[1], data[2], data[3] });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCall), { data[0], data[1], IntToTaggedInt(data[2]) });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), {data[0], data[1]});
                        Jump(&exit);
                        break;
                    default:
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                }
            }
            Bind(&fastCallBridge);
            {
                switch (mode) {
                    case JSCallMode::CALL_THIS_ARG0:
                        thisValue = data[0];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG0:
                    case JSCallMode::DEPRECATED_CALL_ARG0:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG1:
                        thisValue = data[1];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG1:
                    case JSCallMode::DEPRECATED_CALL_ARG1:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue, data[0] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG2:
                        thisValue = data[2];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG2:
                    case JSCallMode::DEPRECATED_CALL_ARG2:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue, data[0], data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3:
                        thisValue = data[3];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG3:
                    case JSCallMode::DEPRECATED_CALL_ARG3:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue,
                            data[0], data[1], data[2] }); // 2: args2
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_WITH_ARGV:
                    case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
                    case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
                        thisValue = data[2]; // 2: this input
                        [[fallthrough]];
                    case JSCallMode::CALL_WITH_ARGV:
                    case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgVAndPushUndefined),
                            { glue, func, thisValue, ZExtInt32ToInt64(actualNumArgs), data[1], expectedNum });
                        Jump(&exit);
                        break;
                    case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
                    case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSFastCallWithArgVAndPushUndefined),
                            { glue, func, data[2], ZExtInt32ToInt64(actualNumArgs), data[1], expectedNum });
                        result = ConstructorCheck(glue, func, *result, data[2]);  // 2: the second index
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_GETTER:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, data[0]});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_SETTER:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, data[0], data[1]});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedFastCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, data[0], data[1], data[2], data[3] });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCall), { data[0], data[1], IntToTaggedInt(data[2]) });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), {data[0], data[1]});
                        Jump(&exit);
                        break;
                    default:
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                }
            }
        }

        Bind(&methodNotFastCall);
        Branch(JudgeAotAndFastCallWithMethod(method, CircuitBuilder::JudgeMethodType::HAS_AOT),
            &methodisAot, &methodNotAot);
        Bind(&methodisAot);
        {
            GateRef expectedNum = Int64And(Int64LSR(callField, Int64(MethodLiteral::NumArgsBits::START_BIT)),
                Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
            GateRef expectedArgc = Int64Add(expectedNum, Int64(NUM_MANDATORY_JSFUNC_ARGS));
            Branch(Int64LessThanOrEqual(expectedArgc, realNumArgs), &slowCall, &slowCallBridge);
            Bind(&slowCall);
            {
                GateRef code = GetAotCodeAddr(func);
                switch (mode) {
                    case JSCallMode::CALL_THIS_ARG0:
                        thisValue = data[0];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG0:
                    case JSCallMode::DEPRECATED_CALL_ARG0:
                        result = CallOptimized(glue, code, { glue, realNumArgs, func, newTarget, thisValue });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG1:
                        thisValue = data[1];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG1:
                    case JSCallMode::DEPRECATED_CALL_ARG1:
                        result = CallOptimized(glue, code, { glue, realNumArgs, func, newTarget, thisValue, data[0] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG2:
                        thisValue = data[2];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG2:
                    case JSCallMode::DEPRECATED_CALL_ARG2:
                        result = CallOptimized(glue, code,
                            { glue, realNumArgs, func, newTarget, thisValue, data[0], data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3:
                        thisValue = data[3];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG3:
                    case JSCallMode::DEPRECATED_CALL_ARG3:
                        result = CallOptimized(glue, code, { glue, realNumArgs, func, newTarget, thisValue,
                            data[0], data[1], data[2] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_WITH_ARGV:
                    case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
                    case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
                        thisValue = data[2]; // 2: this input
                        [[fallthrough]];
                    case JSCallMode::CALL_WITH_ARGV:
                    case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV),
                            { glue, ZExtInt32ToInt64(actualNumArgs), func, newTarget, thisValue, data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
                    case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV),
                            { glue, ZExtInt32ToInt64(actualNumArgs), func, func, data[2], data[1]});
                        result = ConstructorCheck(glue, func, *result, data[2]);  // 2: the second index
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_GETTER:
                        result = CallOptimized(glue, code, { glue, realNumArgs, func, newTarget, data[0] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_SETTER:
                        result = CallOptimized(glue, code, { glue, realNumArgs, func, newTarget, data[0], data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
                        result = CallOptimized(glue, code,
                            { glue, realNumArgs, func, newTarget, data[0], data[1], data[2], data[3] });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCall), { data[0], data[1], IntToTaggedInt(data[2]) });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), {data[0], data[1]});
                        Jump(&exit);
                        break;
                    default:
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                }
            }
            Bind(&slowCallBridge);
            {
                switch (mode) {
                    case JSCallMode::CALL_THIS_ARG0:
                        thisValue = data[0];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG0:
                    case JSCallMode::DEPRECATED_CALL_ARG0:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG1:
                        thisValue = data[1];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG1:
                    case JSCallMode::DEPRECATED_CALL_ARG1:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue, data[0] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG2:
                        thisValue = data[2];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG2:
                    case JSCallMode::DEPRECATED_CALL_ARG2:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue, data[0], data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3:
                        thisValue = data[3];
                        [[fallthrough]];
                    case JSCallMode::CALL_ARG3:
                    case JSCallMode::DEPRECATED_CALL_ARG3:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, thisValue,
                            data[0], data[1], data[2] }); // 2: args2
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_WITH_ARGV:
                    case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
                    case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
                        thisValue = data[2]; // 2: this input
                        [[fallthrough]];
                    case JSCallMode::CALL_WITH_ARGV:
                    case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgVAndPushUndefined),
                            { glue, ZExtInt32ToInt64(actualNumArgs), func, newTarget, thisValue, data[1] });
                        Jump(&exit);
                        break;
                    case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
                    case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                        result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgVAndPushUndefined),
                            { glue, ZExtInt32ToInt64(actualNumArgs), func, func, data[2], data[1]});
                        result = ConstructorCheck(glue, func, *result, data[2]);  // 2: the second index
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_GETTER:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, data[0]});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_SETTER:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, data[0], data[1]});
                        Jump(&exit);
                        break;
                    case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
                        result = CallNGCRuntime(glue, RTSTUB_ID(OptimizedCallAndPushUndefined),
                            { glue, realNumArgs, func, newTarget, data[0], data[1], data[2], data[3] });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCall), { data[0], data[1], IntToTaggedInt(data[2]) });
                        Jump(&exit);
                        break;
                    case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
                        result = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), {data[0], data[1]});
                        Jump(&exit);
                        break;
                    default:
                        LOG_ECMA(FATAL) << "this branch is unreachable";
                        UNREACHABLE();
                }
            }
        }

        Bind(&methodNotAot);
        if (jumpSize != 0) {
            SaveJumpSizeIfNeeded(glue, jumpSize);
        }
        SaveHotnessCounterIfNeeded(glue, sp, hotnessCounter, mode);
        switch (mode) {
            case JSCallMode::CALL_THIS_ARG0:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallThisArg0AndDispatch),
                    { glue, sp, func, method, callField, data[0] });
                Return();
                break;
            case JSCallMode::CALL_ARG0:
            case JSCallMode::DEPRECATED_CALL_ARG0:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArg0AndDispatch),
                    { glue, sp, func, method, callField });
                Return();
                break;
            case JSCallMode::CALL_THIS_ARG1:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallThisArg1AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1] });
                Return();
                break;
            case JSCallMode::CALL_ARG1:
            case JSCallMode::DEPRECATED_CALL_ARG1:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArg1AndDispatch),
                    { glue, sp, func, method, callField, data[0] });
                Return();
                break;
            case JSCallMode::CALL_THIS_ARG2:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallThisArgs2AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2] });
                Return();
                break;
            case JSCallMode::CALL_ARG2:
            case JSCallMode::DEPRECATED_CALL_ARG2:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgs2AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1] });
                Return();
                break;
            case JSCallMode::CALL_THIS_ARG3:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallThisArgs3AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2], data[3] });
                Return();
                break;
            case JSCallMode::CALL_ARG3:
            case JSCallMode::DEPRECATED_CALL_ARG3:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgs3AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2] }); // 2: args2
                Return();
                break;
            case JSCallMode::CALL_WITH_ARGV:
            case JSCallMode::DEPRECATED_CALL_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallRangeAndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1] });
                Return();
                break;
            case JSCallMode::CALL_THIS_WITH_ARGV:
            case JSCallMode::DEPRECATED_CALL_THIS_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallThisRangeAndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2] });
                Return();
                break;
            case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallNewAndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2] });
                Return();
                break;
            case JSCallMode::CALL_GETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(CallGetter),
                    { glue, func, method, callField, data[0] });
                Jump(&exit);
                break;
            case JSCallMode::CALL_SETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(CallSetter),
                    { glue, func, method, callField, data[1], data[0] });
                Jump(&exit);
                break;
            case JSCallMode::CALL_THIS_ARG3_WITH_RETURN:
                result = CallNGCRuntime(glue, RTSTUB_ID(CallContainersArgs3),
                    { glue, func, method, callField, data[1], data[2], data[3], data[0] });
                Jump(&exit);
                break;
            case JSCallMode::CALL_THIS_ARGV_WITH_RETURN:
                result = CallNGCRuntime(glue, RTSTUB_ID(CallReturnWithArgv),
                    { glue, func, method, callField, data[0], data[1], data[2] });
                Jump(&exit);
                break;
            case JSCallMode::SUPER_CALL_WITH_ARGV:
            case JSCallMode::SUPER_CALL_SPREAD_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushSuperCallAndDispatch),
                    { glue, sp, func, method, callField, data[2], data[3], data[4], data[5] });
                Return();
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::CallFastPath(GateRef glue, GateRef nativeCode, GateRef func, GateRef thisValue,
    GateRef actualNumArgs, GateRef callField, GateRef method, Label* notFastBuiltins, Label* exit, Variable* result,
    std::initializer_list<GateRef> args, JSCallMode mode)
{
    auto env = GetEnvironment();
    auto data = std::begin(args);
    Label isFastBuiltins(env);
    GateRef numArgs = ZExtInt32ToPtr(actualNumArgs);
    GateRef isFastBuiltinsMask = Int64(static_cast<uint64_t>(1) << MethodLiteral::IsFastBuiltinBit::START_BIT);
    Branch(Int64NotEqual(Int64And(callField, isFastBuiltinsMask), Int64(0)), &isFastBuiltins, notFastBuiltins);
    Bind(&isFastBuiltins);
    {
        GateRef builtinId = GetBuiltinId(method);
        GateRef ret;
        switch (mode) {
            case JSCallMode::CALL_THIS_ARG0:
                ret = DispatchBuiltins(glue, builtinId, { glue, nativeCode, func, Undefined(), thisValue, numArgs,
                    Undefined(), Undefined(), Undefined()});
                break;
            case JSCallMode::CALL_THIS_ARG1:
                ret = DispatchBuiltins(glue, builtinId, { glue, nativeCode, func, Undefined(),
                                                          thisValue, numArgs, data[0], Undefined(), Undefined() });
                break;
            case JSCallMode::CALL_THIS_ARG2:
                ret = DispatchBuiltins(glue, builtinId, { glue, nativeCode, func, Undefined(), thisValue,
                                                          numArgs, data[0], data[1], Undefined() });
                break;
            case JSCallMode::CALL_THIS_ARG3:
                ret = DispatchBuiltins(glue, builtinId, { glue, nativeCode, func, Undefined(), thisValue,
                                                          numArgs, data[0], data[1], data[2] });
                break;
            case JSCallMode::DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV:
            case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                ret = DispatchBuiltinsWithArgv(glue, builtinId, { glue, nativeCode, func, func, thisValue,
                                                                  numArgs, data[1] });
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        result->WriteVariable(ret);
        Jump(exit);
    }
    Bind(notFastBuiltins);
}

GateRef StubBuilder::TryStringOrSymbolToElementIndex(GateRef glue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));

    Label keyNotSymbol(env);
    Branch(IsSymbol(key), &exit, &keyNotSymbol);
    Bind(&keyNotSymbol);

    Label greatThanZero(env);
    Label inRange(env);
    Label flattenFastPath(env);
    auto len = GetLengthFromString(key);
    Branch(Int32Equal(len, Int32(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    Branch(Int32GreaterThan(len, Int32(MAX_ELEMENT_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        Label isUtf8(env);
        DEFVARIABLE(c, VariableType::INT32(), Int32(0));
        Branch(IsUtf16String(key), &exit, &isUtf8);
        Bind(&isUtf8);
        FlatStringStubBuilder thisFlat(this);
        thisFlat.FlattenString(glue, key, &flattenFastPath);
        Bind(&flattenFastPath);
        StringInfoGateRef stringInfoGate(&thisFlat);
        GateRef data = GetNormalStringData(stringInfoGate);
        c = ZExtInt8ToInt32(Load(VariableType::INT8(), data));
        Label isDigitZero(env);
        Label notDigitZero(env);
        Branch(Int32Equal(*c, Int32('0')), &isDigitZero, &notDigitZero);
        Bind(&isDigitZero);
        {
            Label lengthIsOne(env);
            Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &exit);
            Bind(&lengthIsOne);
            {
                result = Int32(0);
                Jump(&exit);
            }
        }
        Bind(&notDigitZero);
        {
            Label isDigit(env);
            Label notIsDigit(env);
            DEFVARIABLE(i, VariableType::INT32(), Int32(1));
            DEFVARIABLE(n, VariableType::INT64(), Int64Sub(SExtInt32ToInt64(*c), Int64('0')));

            Branch(IsDigit(*c), &isDigit, &notIsDigit);
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Bind(&isDigit);
            Branch(Int32UnsignedLessThan(*i, len), &loopHead, &afterLoop);
            LoopBegin(&loopHead);
            {
                c = ZExtInt8ToInt32(Load(VariableType::INT8(), data, ZExtInt32ToPtr(*i)));
                Label isDigit2(env);
                Label notDigit2(env);
                Branch(IsDigit(*c), &isDigit2, &notDigit2);
                Bind(&isDigit2);
                {
                    // 10 means the base of digit is 10.
                    n = Int64Add(Int64Mul(*n, Int64(10)),
                                 Int64Sub(SExtInt32ToInt64(*c), Int64('0')));
                    i = Int32Add(*i, Int32(1));
                    Branch(Int32UnsignedLessThan(*i, len), &loopEnd, &afterLoop);
                }
                Bind(&notDigit2);
                {
                    Label hasPoint(env);
                    Branch(Int32Equal(*c, Int32('.')), &hasPoint, &exit);
                    Bind(&hasPoint);
                    {
                        result = Int32(-2); // -2:return -2 means should goto slow path
                        Jump(&exit);
                    }
                }
            }
            Bind(&loopEnd);
            LoopEnd(&loopHead);
            Bind(&afterLoop);
            {
                Label lessThanMaxIndex(env);
                Branch(Int64LessThan(*n, Int64(JSObject::MAX_ELEMENT_INDEX)),
                       &lessThanMaxIndex, &exit);
                Bind(&lessThanMaxIndex);
                {
                    result = TruncInt64ToInt32(*n);
                    Jump(&exit);
                }
            }
            Bind(&notIsDigit);
            {
                Label isNegative(env);
                Branch(Int32Equal(*c, Int32('-')), &isNegative, &exit);
                Bind(&isNegative);
                {
                    result = Int32(-2); // -2:return -2 means should goto slow path
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder,
                                                GateRef key, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());

    Label notOnProtoChain(env);
    Branch(Int64NotEqual(receiver, holder), &exit, &notOnProtoChain);
    Bind(&notOnProtoChain);

    auto negativeZero = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::NEGATIVE_ZERO_STRING_INDEX);
    Label isNegativeZero(env);
    Label notNegativeZero(env);
    Branch(Equal(negativeZero, key), &isNegativeZero, &notNegativeZero);
    Bind(&isNegativeZero);
    {
        result = Undefined();
        Jump(&exit);
    }
    Bind(&notNegativeZero);
    {
        GateRef index = TryStringOrSymbolToElementIndex(glue, key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            TypedArrayStubBuilder typedArrayStubBuilder(this);
            result = typedArrayStubBuilder.FastGetPropertyByIndex(glue, holder, index, jsType);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label returnNull(env);
            Branch(Int32Equal(index, Int32(-2)), &returnNull, &exit); // -2:equal -2 means should goto slow path
            Bind(&returnNull);
            {
                result = Null();
                Jump(&exit);
            }
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SetTypeArrayPropertyByName(GateRef glue, GateRef receiver, GateRef holder, GateRef key,
                                                GateRef value, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label notOnProtoChain(env);
    Branch(Int64NotEqual(receiver, holder), &exit, &notOnProtoChain);
    Bind(&notOnProtoChain);

    auto negativeZero = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::NEGATIVE_ZERO_STRING_INDEX);
    Label isNegativeZero(env);
    Label notNegativeZero(env);
    Branch(Equal(negativeZero, key), &isNegativeZero, &notNegativeZero);
    Bind(&isNegativeZero);
    {
        Label isObj(env);
        Label notObj(env);
        Branch(IsEcmaObject(value), &isObj, &notObj);
        Bind(&isObj);
        {
            result = Null();
            Jump(&exit);
        }
        Bind(&notObj);
        result = Undefined();
        Jump(&exit);
    }
    Bind(&notNegativeZero);
    {
        GateRef index = TryStringOrSymbolToElementIndex(glue, key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = CallRuntime(glue, RTSTUB_ID(SetTypeArrayPropertyByIndex),
                { receiver, IntToTaggedInt(index), value, IntToTaggedInt(jsType) });
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label returnNull(env);
            Branch(Int32Equal(index, Int32(-2)), &returnNull, &exit); // -2:equal -2 means should goto slow path
            Bind(&returnNull);
            {
                result = Null();
                Jump(&exit);
            }
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::Assert(int messageId, int line, GateRef glue, GateRef condition, Label *nextLabel)
{
    auto env = GetEnvironment();
    Label ok(env);
    Label notOk(env);
    Branch(condition, &ok, &notOk);
    Bind(&ok);
    {
        Jump(nextLabel);
    }
    Bind(&notOk);
    {
        FatalPrint(glue, { Int32(messageId), Int32(line) });
        Jump(nextLabel);
    }
}

GateRef StubBuilder::GetNormalStringData(const StringInfoGateRef &stringInfoGate)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isConstantString(env);
    Label isLineString(env);
    Label isUtf8(env);
    Label isUtf16(env);
    DEFVARIABLE(result, VariableType::NATIVE_POINTER(), Undefined());
    Branch(IsConstantString(stringInfoGate.GetString()), &isConstantString, &isLineString);
    Bind(&isConstantString);
    {
        GateRef address = PtrAdd(stringInfoGate.GetString(), IntPtr(ConstantString::CONSTANT_DATA_OFFSET));
        result = PtrAdd(Load(VariableType::NATIVE_POINTER(), address, IntPtr(0)),
            ZExtInt32ToPtr(stringInfoGate.GetStartIndex()));
        Jump(&exit);
    }
    Bind(&isLineString);
    {
        GateRef data = ChangeTaggedPointerToInt64(
            PtrAdd(stringInfoGate.GetString(), IntPtr(LineEcmaString::DATA_OFFSET)));
        Branch(IsUtf8String(stringInfoGate.GetString()), &isUtf8, &isUtf16);
        Bind(&isUtf8);
        {
            result = PtrAdd(data, ZExtInt32ToPtr(stringInfoGate.GetStartIndex()));
            Jump(&exit);
        }
        Bind(&isUtf16);
        {
            GateRef offset = PtrMul(ZExtInt32ToPtr(stringInfoGate.GetStartIndex()), IntPtr(sizeof(uint16_t)));
            result = PtrAdd(data, offset);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ToNumber(GateRef glue, GateRef tagged)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isNumber(env);
    Label notNumber(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsNumber(tagged), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = tagged;
        Jump(&exit);
    }
    Bind(&notNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(ToNumber), { tagged });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ToLength(GateRef glue, GateRef target)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    Label exit(env);

    GateRef number = ToNumber(glue, target);
    Label isPendingException(env);
    Label noPendingException(env);
    Branch(HasPendingException(glue), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        Jump(&exit);
    }
    Bind(&noPendingException);
    {
        GateRef num = GetDoubleOfTNumber(number);
        Label targetLessThanZero(env);
        Label targetGreaterThanZero(env);
        Label targetLessThanSafeNumber(env);
        Label targetGreaterThanSafeNumber(env);
        Branch(DoubleLessThan(num, Double(0.0)), &targetLessThanZero, &targetGreaterThanZero);
        Bind(&targetLessThanZero);
        {
            res = DoubleToTaggedDoublePtr(Double(0.0));
            Jump(&exit);
        }
        Bind(&targetGreaterThanZero);
        Branch(DoubleGreaterThan(num, Double(SAFE_NUMBER)), &targetGreaterThanSafeNumber, &targetLessThanSafeNumber);
        Bind(&targetGreaterThanSafeNumber);
        {
            res = DoubleToTaggedDoublePtr(Double(SAFE_NUMBER));
            Jump(&exit);
        }
        Bind(&targetLessThanSafeNumber);
        {
            res = number;
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedGetNumber(GateRef x)
{
    auto env = GetEnvironment();
    Label subentry(env);
    Label exit(env);
    env->SubCfgEntry(&subentry);

    Label targetIsInt(env);
    Label targetIsDouble(env);
    DEFVALUE(number, env_, VariableType::FLOAT64(), Double(0));
    Branch(TaggedIsInt(x), &targetIsInt, &targetIsDouble);
    Bind(&targetIsInt);
    {
        number = ChangeInt32ToFloat64(TaggedGetInt(x));
        Jump(&exit);
    }
    Bind(&targetIsDouble);
    {
        number = GetDoubleOfTDouble(x);
        Jump(&exit);
    }
    Bind(&exit);
    GateRef ret = *number;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::NumberGetInt(GateRef glue, GateRef x)
{
    auto env = GetEnvironment();
    Label subentry(env);
    Label exit(env);
    env->SubCfgEntry(&subentry);

    Label targetIsInt(env);
    Label targetIsDouble(env);
    DEFVALUE(number, env_, VariableType::INT32(), Int32(0));
    Branch(TaggedIsInt(x), &targetIsInt, &targetIsDouble);
    Bind(&targetIsInt);
    {
        number = TaggedGetInt(x);
        Jump(&exit);
    }
    Bind(&targetIsDouble);
    {
        number = DoubleToInt(glue, GetDoubleOfTDouble(x));
        Jump(&exit);
    }
    Bind(&exit);
    GateRef ret = *number;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::HasStableElements(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label targetIsHeapObject(env);
    Label targetIsStableElements(env);

    Branch(TaggedIsHeapObject(obj), &targetIsHeapObject, &exit);
    Bind(&targetIsHeapObject);
    {
        GateRef jsHclass = LoadHClass(obj);
        Branch(IsStableElements(jsHclass), &targetIsStableElements, &exit);
        Bind(&targetIsStableElements);
        {
            GateRef guardiansOffset =
                IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(env->Is32Bit()));
            result = Load(VariableType::BOOL(), glue, guardiansOffset);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef StubBuilder::IsStableJSArguments(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label targetIsHeapObject(env);
    Label targetIsStableArguments(env);

    Branch(TaggedIsHeapObject(obj), &targetIsHeapObject, &exit);
    Bind(&targetIsHeapObject);
    {
        GateRef jsHclass = LoadHClass(obj);
        Branch(IsStableArguments(jsHclass), &targetIsStableArguments, &exit);
        Bind(&targetIsStableArguments);
        {
            GateRef guardiansOffset =
                IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(env->Is32Bit()));
            result = Load(VariableType::BOOL(), glue, guardiansOffset);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef StubBuilder::IsStableJSArray(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label targetIsHeapObject(env);
    Label targetIsStableArray(env);

    Branch(TaggedIsHeapObject(obj), &targetIsHeapObject, &exit);
    Bind(&targetIsHeapObject);
    {
        GateRef jsHclass = LoadHClass(obj);
        Branch(IsStableArray(jsHclass), &targetIsStableArray, &exit);
        Bind(&targetIsStableArray);
        {
            GateRef guardiansOffset =
                IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(env->Is32Bit()));
            GateRef guardians = Load(VariableType::BOOL(), glue, guardiansOffset);
            result.WriteVariable(guardians);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef StubBuilder::UpdateProfileTypeInfo(GateRef glue, GateRef jsFunc)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label needUpdate(env);
    Label exit(env);
    DEFVARIABLE(profileTypeInfo, VariableType::JS_POINTER(), GetProfileTypeInfo(jsFunc));
    Branch(TaggedIsUndefined(*profileTypeInfo), &needUpdate, &exit);
    Bind(&needUpdate);
    {
        profileTypeInfo = CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounter), { jsFunc });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *profileTypeInfo;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetFuncKind(GateRef method)
{
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), method, extraLiteralInfoOffset);

    GateRef kind = Int32And(Int32LSR(bitfield, Int32(Method::FunctionKindBits::START_BIT)),
                            Int32((1LU << Method::FunctionKindBits::SIZE) - 1));
    return kind;
}

GateRef StubBuilder::GetCallSpreadArgs(GateRef glue, GateRef array, ProfileOperation callBack)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), NullPtr());
    Label fastPath(env);
    Label noCopyPath(env);
    Label exit(env);

    GateRef itor = GetIterator(glue, array, callBack);
    GateRef iterHClass = LoadHClass(itor);
    GateRef isJSArrayIter = Int32Equal(GetObjectType(iterHClass),
                                       Int32(static_cast<int32_t>(JSType::JS_ARRAY_ITERATOR)));
    GateRef needCopy = BoolAnd(IsStableJSArray(glue, array), isJSArrayIter);
    Branch(needCopy, &fastPath, &noCopyPath);
    Bind(&fastPath);
    {
        // copy operation is omitted
        result = GetElementsArray(array);
        Jump(&exit);
    }
    Bind(&noCopyPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetCallSpreadArgs), { array });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CalArrayRelativePos(GateRef index, GateRef arrayLen)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label exit(env);
    Branch(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        GateRef tempBeginIndex = Int32Add(arrayLen, index);
        Label beginIndexLargeZero(env);
        Branch(Int32GreaterThan(tempBeginIndex, Int32(0)), &beginIndexLargeZero, &exit);
        Bind(&beginIndexLargeZero);
        {
            result = tempBeginIndex;
            Jump(&exit);
        }
    }
    Bind(&indexNotLessZero);
    {
        Label lessLen(env);
        Label largeLen(env);
        Branch(Int32LessThan(index, arrayLen), &lessLen, &largeLen);
        Bind(&lessLen);
        {
            result = index;
            Jump(&exit);
        }
        Bind(&largeLen);
        {
            result = arrayLen;
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::AppendSkipHole(GateRef glue, GateRef first, GateRef second, GateRef copyLength)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());

    GateRef firstLength = GetLengthOfTaggedArray(first);
    GateRef secondLength = GetLengthOfTaggedArray(second);
    NewObjectStubBuilder newBuilder(this);
    GateRef array = newBuilder.NewTaggedArray(glue, copyLength);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Label notHole(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32UnsignedLessThan(*index, firstLength), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            GateRef value = GetValueFromTaggedArray(first, *index);
            Branch(TaggedIsHole(value), &afterLoop, &notHole);
            Bind(&notHole);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, value);
            index = Int32Add(*index, Int32(1));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    {
        Label loopHead1(env);
        Label loopEnd1(env);
        Label storeValue1(env);
        Label notHole1(env);
        Jump(&loopHead1);
        LoopBegin(&loopHead1);
        {
            Branch(Int32UnsignedLessThan(*i, secondLength), &storeValue1, &exit);
            Bind(&storeValue1);
            {
                GateRef value1 = GetValueFromTaggedArray(second, *i);
                Branch(TaggedIsHole(value1), &exit, &notHole1);
                Bind(&notHole1);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, value1);
                i = Int32Add(*i, Int32(1));
                index = Int32Add(*index, Int32(1));
                Jump(&loopEnd1);
            }
        }
        Bind(&loopEnd1);
        LoopEnd(&loopHead1);
    }
    Bind(&exit);
    res = array;
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IntToEcmaString(GateRef glue, GateRef number)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(n, VariableType::INT32(), number);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Hole());

    GateRef isPositive = Int32GreaterThanOrEqual(number, Int32(0));
    GateRef isSingle = Int32LessThan(number, Int32(10));
    Label process(env);
    Label callRuntime(env);
    Label afterNew(env);
    Branch(BoolAnd(isPositive, isSingle), &process, &callRuntime);
    Bind(&process);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), true);
        Bind(&afterNew);
        n = Int32Add(Int32('0'), *n);
        GateRef dst = ChangeTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
        Store(VariableType::INT8(), glue, dst, IntPtr(0), TruncInt32ToInt8(*n));
        Jump(&exit);
    }
    Bind(&callRuntime);
    {
        result = CallRuntime(glue, RTSTUB_ID(IntToString), { IntToTaggedInt(*n) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetTaggedValueWithElementsKind(GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);

    GateRef hclass = LoadHClass(receiver);
    DEFVARIABLE(elementsKind, VariableType::INT32(), GetElementsKindFromHClass(hclass));
    Label isMutantTaggedArray(env);
    Label isNotMutantTaggedArray(env);
    GateRef elements = GetElementsArray(receiver);
    Branch(IsMutantTaggedArray(elements), &isMutantTaggedArray, &isNotMutantTaggedArray);
    Bind(&isNotMutantTaggedArray);
    {
        elementsKind = Int32(static_cast<int32_t>(ElementsKind::GENERIC));
        Jump(&isMutantTaggedArray);
    }
    Bind(&isMutantTaggedArray);
    GateRef rawValue = GetValueFromMutantTaggedArray(elements, index);
    Label isSpecialHole(env);
    Label isNotSpecialHole(env);
    Branch(Int64Equal(rawValue, SpecialHole()), &isSpecialHole, &isNotSpecialHole);
    Bind(&isSpecialHole);
    {
        Jump(&exit);
    }
    Bind(&isNotSpecialHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        GateRef elementsKindIntLowerBound = Int32GreaterThanOrEqual(*elementsKind,
                                                                    Int32(static_cast<int32_t>(ElementsKind::INT)));
        GateRef elementsKindIntUpperBound = Int32LessThanOrEqual(*elementsKind,
                                                                 Int32(static_cast<int32_t>(ElementsKind::HOLE_INT)));
        GateRef checkIntKind = BoolAnd(elementsKindIntLowerBound, elementsKindIntUpperBound);
        Branch(checkIntKind, &isInt, &isNotInt);
        Bind(&isInt);
        {
            result = Int64ToTaggedIntPtr(rawValue);
            Jump(&exit);
        }
        Bind(&isNotInt);
        {
            Label isNumber(env);
            Label isNotNumber(env);
            GateRef elementsKindNumberLB = Int32GreaterThanOrEqual(*elementsKind,
                                                                   Int32(static_cast<int32_t>(ElementsKind::NUMBER)));
            GateRef elementsKindNumberUB = Int32LessThanOrEqual(*elementsKind,
                                                                Int32(static_cast<int32_t>(ElementsKind::HOLE_NUMBER)));
            GateRef checkNumberKind = BoolAnd(elementsKindNumberLB, elementsKindNumberUB);
            Branch(checkNumberKind, &isNumber, &isNotNumber);
            Bind(&isNumber);
            {
                GateRef numberValue = CastInt64ToFloat64(rawValue);
                result = DoubleToTaggedDoublePtr(numberValue);
                Jump(&exit);
            }
            Bind(&isNotNumber);
            {
                result = Int64ToTaggedPtr(rawValue);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SetValueWithElementsKind(GateRef glue, GateRef receiver, GateRef rawValue,
                                              GateRef index, GateRef needTransition, GateRef extraKind)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::INT64(), SpecialHole());
    Label exit(env);

    Label transitElementsKind(env);
    Label finishTransition(env);
    Branch(needTransition, &transitElementsKind, &finishTransition);
    Bind(&transitElementsKind);
    {
        TransitToElementsKind(glue, receiver, rawValue, extraKind);
        Jump(&finishTransition);
    }
    Bind(&finishTransition);
    GateRef hclass = LoadHClass(receiver);
    DEFVARIABLE(elementsKind, VariableType::INT32(), GetElementsKindFromHClass(hclass));
    Label setValue(env);
    Label isMutantTaggedArray(env);
    Label isNotMutantTaggedArray(env);
    GateRef elements = GetElementsArray(receiver);
    Branch(IsMutantTaggedArray(elements), &isMutantTaggedArray, &isNotMutantTaggedArray);
    Bind(&isNotMutantTaggedArray);
    {
        elementsKind = Int32(static_cast<int32_t>(ElementsKind::GENERIC));
        Jump(&isMutantTaggedArray);
    }
    Bind(&isMutantTaggedArray);
    Label isHole(env);
    Label isNotHole(env);
    GateRef valueIsHole = TaggedIsHole(rawValue);
    GateRef elementsKindInNumbersLB = Int32GreaterThanOrEqual(*elementsKind,
                                                              Int32(static_cast<int32_t>(ElementsKind::HOLE)));
    GateRef elementsKindInNumbersUB = Int32LessThan(*elementsKind, Int32(static_cast<int32_t>(ElementsKind::STRING)));
    GateRef checkInNumersKind = BoolAnd(BoolAnd(valueIsHole, elementsKindInNumbersLB), elementsKindInNumbersUB);
    Branch(checkInNumersKind, &isHole, &isNotHole);
    Bind(&isHole);
    {
        Jump(&setValue);
    }
    Bind(&isNotHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        GateRef elementsKindIntLB = Int32GreaterThanOrEqual(*elementsKind,
                                                            Int32(static_cast<int32_t>(ElementsKind::INT)));
        GateRef elementsKindIntUB = Int32LessThanOrEqual(*elementsKind,
                                                         Int32(static_cast<int32_t>(ElementsKind::HOLE_INT)));
        GateRef checkIntKind = BoolAnd(elementsKindIntLB, elementsKindIntUB);
        Branch(checkIntKind, &isInt, &isNotInt);
        Bind(&isInt);
        {
            result = GetInt64OfTInt(rawValue);
            Jump(&setValue);
        }
        Bind(&isNotInt);
        {
            Label isNumber(env);
            Label isNotNumber(env);
            GateRef elementsKindNumberLB = Int32GreaterThanOrEqual(*elementsKind,
                                                                   Int32(static_cast<int32_t>(ElementsKind::NUMBER)));
            GateRef elementsKindNumberUB = Int32LessThanOrEqual(*elementsKind,
                                                                Int32(static_cast<int32_t>(ElementsKind::HOLE_NUMBER)));
            GateRef checkNumberKind = BoolAnd(elementsKindNumberLB, elementsKindNumberUB);
            Branch(checkNumberKind, &isNumber, &isNotNumber);
            Bind(&isNumber);
            {
                Label isNumberInt(env);
                Label isNotNumberInt(env);
                Branch(TaggedIsInt(rawValue), &isNumberInt, &isNotNumberInt);
                Bind(&isNumberInt);
                {
                    result = CastDoubleToInt64(GetDoubleOfTInt(rawValue));
                    Jump(&setValue);
                }
                Bind(&isNotNumberInt);
                {
                    result = CastDoubleToInt64(GetDoubleOfTDouble(rawValue));
                    Jump(&setValue);
                }
            }
            Bind(&isNotNumber);
            {
                result = ChangeTaggedPointerToInt64(rawValue);
                Jump(&setValue);
            }
        }
    }
    Bind(&setValue);
    Label storeToNormalArray(env);
    Label storeToMutantArray(env);
    Branch(TaggedIsHeapObject(rawValue), &storeToNormalArray, &storeToMutantArray);
    Bind(&storeToNormalArray);
    {
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, *result);
        Jump(&exit);
    }
    Bind(&storeToMutantArray);
    {
        SetValueToTaggedArray(VariableType::INT64(), glue, elements, index, *result);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
