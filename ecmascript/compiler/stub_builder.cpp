/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_proxy_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_collection_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/object_operator_stub_builder.h"
#include "ecmascript/compiler/profiler_stub_builder.h"
#include "ecmascript/elements.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/marker_cell.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/transitions_dictionary.h"
#include "common_components/heap/allocator/region_desc.h"
#include "objects/base_state_word.h"

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

void StubBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel,
                        uint32_t trueWeight, uint32_t falseWeight, const char *comment)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = env_->GetBuilder()->Branch(currentControl, condition, trueWeight, falseWeight, comment);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = env_->GetBuilder()->IfTrue(ifBranch);
    trueLabel->AppendPredecessor(env_->GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = env_->GetBuilder()->IfFalse(ifBranch);
    falseLabel->AppendPredecessor(env_->GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_->SetCurrentLabel(nullptr);
}

template <class LabelPtrGetter>
void StubBuilder::SwitchGeneric(GateRef index, Label *defaultLabel, Span<const int64_t> keysValue,
                                LabelPtrGetter getIthLabelFn)
{
    static_assert(std::is_invocable_r_v<Label*, LabelPtrGetter, size_t>, "Invalid call signature.");
    size_t numberOfKeys = keysValue.Size();
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = env_->GetBuilder()->SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (size_t i = 0; i < numberOfKeys; i++) {
        GateRef switchCase = env_->GetBuilder()->SwitchCase(switchBranch, keysValue[i]);
        Label *curLabel = std::invoke(getIthLabelFn, i);
        curLabel->AppendPredecessor(currentLabel);
        curLabel->MergeControl(switchCase);
    }

    GateRef defaultCase = env_->GetBuilder()->DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_->SetCurrentLabel(nullptr);
}

void StubBuilder::Switch(GateRef index, Label *defaultLabel,
                         const int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    return SwitchGeneric(index, defaultLabel, {keysValue, numberOfKeys}, [keysLabel](size_t i) {
        return &keysLabel[i];
    });
}

void StubBuilder::Switch(GateRef index, Label *defaultLabel,
                         const int64_t *keysValue, Label *const *keysLabel, int numberOfKeys)
{
    return SwitchGeneric(index, defaultLabel, {keysValue, numberOfKeys}, [keysLabel](size_t i) {
        return keysLabel[i];
    });
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

GateRef StubBuilder::CheckSuspend(GateRef glue)
{
    GateRef stateAndFlagsOffset = IntPtr(JSThread::GlueData::GetStateAndFlagsOffset(env_->IsArch32Bit()));
    GateRef stateAndFlags = LoadPrimitive(VariableType::INT16(), glue, stateAndFlagsOffset);
    return Int32And(ZExtInt16ToInt32(stateAndFlags), Int32(ThreadFlag::SUSPEND_REQUEST));
}

GateRef StubBuilder::CheckSuspendForCMCGC(GateRef glue)
{
    GateRef threadHolderOffset = IntPtr(JSThread::GlueData::GetThreadHolderOffset(env_->IsArch32Bit()));
    GateRef threadHolder = LoadPrimitive(VariableType::NATIVE_POINTER(), glue, threadHolderOffset);
    GateRef mutatorBase = LoadPrimitive(VariableType::NATIVE_POINTER(), threadHolder, IntPtr(ThreadHolder::GetMutatorBaseOffset()));
    GateRef safepointActive = LoadPrimitive(VariableType::INT32(), mutatorBase, IntPtr(common::MutatorBase::GetSafepointActiveOffset()));
    return safepointActive;
}

void StubBuilder::LoopEndWithCheckSafePoint(Label *loopHead, Environment *env, GateRef glue)
{
    Label loopEnd(env);
    Label needSuspend(env);
    Label checkSuspendForCMCGC(env);
    Label checkSuspend(env);
    BRANCH_UNLIKELY(LoadPrimitive(
        VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &checkSuspendForCMCGC, &checkSuspend);
    Bind(&checkSuspend);
    BRANCH_UNLIKELY(Int32Equal(Int32(ThreadFlag::SUSPEND_REQUEST), CheckSuspend(glue)), &needSuspend, &loopEnd);
    Bind(&checkSuspendForCMCGC);
    BRANCH_UNLIKELY(Int32Equal(Int32(ThreadFlag::SUSPEND_REQUEST), CheckSuspendForCMCGC(glue)), &needSuspend, &loopEnd);
    Bind(&needSuspend);
    {
        CallRuntime(glue, RTSTUB_ID(CheckSafePoint), {});
        Jump(&loopEnd);
    }
    Bind(&loopEnd);
    LoopEnd(loopHead);
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

void StubBuilder::MatchFieldType(
    GateRef glue, GateRef fieldType, GateRef value, Label *executeSetProp, Label *typeMismatch)
{
    auto *env = GetEnvironment();
    Label valueIsUndefined(env);
    Label checkNumber(env);
    Label isNumber(env);
    Label checkBoolean(env);
    Label isBoolean(env);
    Label checkString(env);
    Label isString(env);
    Label checkJSShared(env);
    Label isJSShared(env);
    Label checkBigInt(env);
    Label isBigInt(env);
    Label checkNoneOrGeneric(env);
    Label isNoneOrGeneric(env);
    Label checkNull(env);
    Label isNull(env);
    Label checkUndefined(env);
    Label isUndefined(env);
    Label exit(env);
    Label mismatch(env);
    Label checkMatch(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    GateRef checkType = TaggedIsUndefined(value);
    BRANCH(checkType, &valueIsUndefined, &checkNumber);
    Bind(&valueIsUndefined);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&checkNumber);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::NUMBER))), Int32(0)))
            .And(TaggedIsNumber(value))
            .Done();
        BRANCH(checkType, &isNumber, &checkBoolean);
        Bind(&isNumber);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkBoolean);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::BOOLEAN))), Int32(0)))
            .And(TaggedIsBoolean(value))
            .Done();
        BRANCH(checkType, &isBoolean, &checkString);
        Bind(&isBoolean);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkString);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::STRING))), Int32(0)))
            .And(LogicOrBuilder(env).Or(TaggedIsString(glue, value)).Or(TaggedIsNull(value)).Done())
            .Done();
        BRANCH(checkType, &isString, &checkJSShared);
        Bind(&isString);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkJSShared);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::SENDABLE))), Int32(0)))
            .And(LogicOrBuilder(env).Or(TaggedIsSharedObj(glue, value)).Or(TaggedIsNull(value)).Done())
            .Done();
        BRANCH(checkType, &isJSShared, &checkBigInt);
        Bind(&isJSShared);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkBigInt);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::BIG_INT))), Int32(0)))
            .And(TaggedIsBigInt(glue, value))
            .Done();
        BRANCH(checkType, &isBigInt, &checkNoneOrGeneric);
        Bind(&isBigInt);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkNoneOrGeneric);
    {
        GateRef fieldTypeCheck = LogicOrBuilder(env)
            .Or(Equal(fieldType, Int32(static_cast<int32_t>(SharedFieldType::NONE))))
            .Or(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::GENERIC))), Int32(0)))
            .Done();
        checkType = LogicAndBuilder(env)
            .And(fieldTypeCheck)
            .And(LogicOrBuilder(env).Or(BoolNot(TaggedIsHeapObject(value))).Or(TaggedIsSharedObj(glue, value)).Done())
            .Done();
        BRANCH(checkType, &isNoneOrGeneric, &checkNull);
        Bind(&isNoneOrGeneric);
        {
            // (none || generic) && (jsShared || !heapObject)
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkNull);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::NULL_TYPE))), Int32(0)))
            .And(TaggedIsNull(value))
            .Done();
        BRANCH(checkType, &isNull, &checkUndefined);
        Bind(&isNull);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&checkUndefined);
    {
        checkType = LogicAndBuilder(env)
            .And(Int32NotEqual(Int32And(fieldType, Int32(static_cast<int32_t>(SharedFieldType::UNDEFINED))), Int32(0)))
            .And(TaggedIsUndefined(value))
            .Done();
        BRANCH(checkType, &isUndefined, &exit);
        Bind(&isUndefined);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    Branch(BoolNot(*result), &mismatch, &checkMatch);
    Bind(&mismatch);
    {
        CallRuntime(glue, RTSTUB_ID(MismatchError), {IntToTaggedInt(fieldType), value});
        Jump(&checkMatch);
    }
    Bind(&checkMatch);
    BRANCH(*result, executeSetProp, typeMismatch);
}

// FindElementWithCache in ecmascript/layout_info-inl.h
GateRef StubBuilder::FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hclass,
    GateRef key, GateRef propsNum, GateRef hir)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    Label exit(env);
    Label notExceedUpper(env);
    Label exceedUpper(env);
    // 9 : Builtins Object properties number is nine
    constexpr int32_t maxPropsNum = 9;
    BRANCH(Int32LessThanOrEqual(propsNum, Int32(maxPropsNum)), &notExceedUpper, &exceedUpper);
    Bind(&notExceedUpper);
    {
        Label labels[maxPropsNum] = {Label(env), Label(env), Label(env), Label(env), Label(env), Label(env), Label(env),
                                     Label(env), Label(env)};
        Label notFount(env);
        GateRef elementAddr = GetPropertiesAddrFromLayoutInfo(layoutInfo);
        Jump(&labels[0]);
        for (int32_t idx = 0; idx < maxPropsNum; idx++) {
            bool isLast = (idx == maxPropsNum - 1);
            Label check(env);
            Bind(&labels[idx]);
            {
                BRANCH_LIKELY(Int32LessThan(Int32(idx), propsNum), &check, &notFount);
                // Not real "likely", just to make the code layout of labels and check block tightly.
            }
            Bind(&check);
            {
                result = Int32(idx);
                GateRef keyInProperty = Load(VariableType::JS_ANY(), glue, elementAddr,
                                             PtrMul(ZExtInt32ToPtr(Int32(idx)),
                                                    IntPtr(sizeof(panda::ecmascript::Properties))));
                if (!isLast) {
                    BRANCH_UNLIKELY(Equal(keyInProperty, key), &exit, &labels[idx + 1]);
                    // Not real "unlikely", just to make the code layout of labels and check block tightly.
                } else {
                    BRANCH(Equal(keyInProperty, key), &exit, &notFount);
                }
            }
        }
        Bind(&notFount);
        result = Int32(-1);
        Jump(&exit);
    }
    Bind(&exceedUpper);
    Label find(env);
    Label notFind(env);
    Label setCache(env);
    GateRef cache = GetPropertiesCache(glue);
    GateRef index = GetIndexFromPropertiesCache(glue, cache, hclass, key, hir);
    BRANCH(Int32Equal(index, Int32(PropertiesCache::NOT_FOUND)), &notFind, &find);
    Bind(&notFind);
    {
        result = BinarySearch(glue, layoutInfo, key, propsNum, hir);
        BRANCH(Int32Equal(*result, Int32(PropertiesCache::NOT_FOUND)), &exit, &setCache);
        Bind(&setCache);
        SetToPropertiesCache(glue, cache, hclass, key, *result, hir);
        Jump(&exit);
    }
    Bind(&find);
    {
        result = index;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetIndexFromPropertiesCache(GateRef glue, GateRef cache, GateRef cls, GateRef key, GateRef hir)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(PropertiesCache::NOT_FOUND));

    Label exit(env);
    Label find(env);
    GateRef hash = HashFromHclassAndKey(glue, cls, key, hir);
    GateRef prop =
        PtrAdd(cache, PtrMul(ZExtInt32ToPtr(hash), IntPtr(PropertiesCache::PropertyKey::GetPropertyKeySize())));
    size_t propHclassOffset = PropertiesCache::PropertyKey::GetHclassOffset();
    size_t propKeyOffset = PropertiesCache::PropertyKey::GetKeyOffset();
    BRANCH(LogicAndBuilder(env)
        .And(IntPtrEqual(cls, Load(VariableType::JS_POINTER(), glue, prop, IntPtr(propHclassOffset))))
        .And(IntPtrEqual(key, Load(VariableType::JS_ANY(), glue, prop, IntPtr(propKeyOffset))))
        .Done(), &find, &exit);
    Bind(&find);
    {
        result = LoadPrimitive(VariableType::INT32(), prop, IntPtr(PropertiesCache::PropertyKey::GetResultsOffset()));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetHandlerFromMegaICCache(GateRef glue, GateRef cache, GateRef cls, GateRef key)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label find(env);
    GateRef hash = HashFromHclassAndStringKey(glue, cls, key);

    GateRef prop = PtrAdd(cache, PtrMul(ZExtInt32ToPtr(hash), IntPtr(MegaICCache::PropertyKey::GetPropertyKeySize())));
    GateRef propHclass = Load(VariableType::JS_POINTER(), glue, prop, IntPtr(MegaICCache::PropertyKey::GetHclassOffset()));
    GateRef propKey = Load(VariableType::JS_ANY(), glue, prop, IntPtr(MegaICCache::PropertyKey::GetKeyOffset()));

    GateRef hclassIsEqual = IntPtrEqual(cls, propHclass);
    GateRef keyIsEqual = IntPtrEqual(key, propKey);
    IncMegaProbeCount(glue);
    BRANCH(BitAnd(hclassIsEqual, keyIsEqual), &find, &exit);
    Bind(&find);
    {
        result = Load(VariableType::JS_ANY(), glue, prop, IntPtr(MegaICCache::PropertyKey::GetResultsOffset()));
        IncMegaHitCount(glue);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}


GateRef StubBuilder::BinarySearch(GateRef glue, GateRef layoutInfo, GateRef key, GateRef propsNum, GateRef hir)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(low, VariableType::INT32(), Int32(0));
    Label exit(env);
    GateRef elements = GetExtraLengthOfTaggedArray(layoutInfo);
    DEFVARIABLE(high, VariableType::INT32(), Int32Sub(elements, Int32(1)));
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(mid, VariableType::INT32(), Int32(-1));

    GateRef keyHash = GetKeyHashCode(glue, key, hir);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label midGreaterKey(env);
    Label midnotGreaterKey(env);
    Label midLessKey(env);
    Label midEqualKey(env);
    Label next(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32LessThanOrEqual(*low, *high), &next, &exit);
        Bind(&next);
        mid = Int32Add(*low, Int32Div(Int32Sub(*high, *low), Int32(2)));  // 2: half
        GateRef midKey = GetSortedKey(glue, layoutInfo, *mid);
        GateRef midHash = GetKeyHashCode(glue, midKey, hir);
        BRANCH(Int32UnsignedGreaterThan(midHash, keyHash), &midGreaterKey, &midnotGreaterKey);
        Bind(&midGreaterKey);
        {
            high = Int32Sub(*mid, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&midnotGreaterKey);
        {
            BRANCH(Int32UnsignedLessThan(midHash, keyHash), &midLessKey, &midEqualKey);
            Bind(&midLessKey);
            {
                low = Int32Add(*mid, Int32(1));
                Jump(&loopEnd);
            }
            Bind(&midEqualKey);
            {
                Label retIndex(env);
                Label nextLoop(env);
                DEFVARIABLE(sortIndex, VariableType::INT32(), GetSortedIndex(glue, layoutInfo, *mid));
                DEFVARIABLE(currentKey, VariableType::JS_ANY(), midKey);
                BRANCH(IntPtrEqual(midKey, key), &retIndex, &nextLoop);
                Bind(&retIndex);
                {
                    Label retSortIndex(env);
                    BRANCH(Int32LessThan(*sortIndex, propsNum), &retSortIndex, &exit);
                    Bind(&retSortIndex);
                    {
                        result = *sortIndex;
                        Jump(&exit);
                    }
                }
                Bind(&nextLoop);
                {
                    DEFVARIABLE(midLeft, VariableType::INT32(), *mid);
                    DEFVARIABLE(midRight, VariableType::INT32(), *mid);
                    Label loopHead1(env);
                    Label loopEnd1(env);
                    Label afterLoop1(env);
                    Label nextCount(env);
                    Jump(&loopHead1);
                    LoopBegin(&loopHead1);
                    {
                        BRANCH(Int32GreaterThanOrEqual(Int32Sub(*midLeft, Int32(1)), Int32(0)),
                            &nextCount, &afterLoop1);
                        Bind(&nextCount);
                        {
                            Label hashEqual(env);
                            midLeft = Int32Sub(*midLeft, Int32(1));
                            sortIndex = GetSortedIndex(glue, layoutInfo, *midLeft);
                            currentKey = GetKey(glue, layoutInfo, *sortIndex);
                            BRANCH(Int32Equal(GetKeyHashCode(glue, *currentKey, hir), keyHash), &hashEqual,
                                   &afterLoop1);
                            Bind(&hashEqual);
                            {
                                Label retIndex1(env);
                                BRANCH(IntPtrEqual(*currentKey, key), &retIndex1, &loopEnd1);
                                Bind(&retIndex1);
                                {
                                    Label retSortIndex(env);
                                    BRANCH(Int32LessThan(*sortIndex, propsNum), &retSortIndex, &exit);
                                    Bind(&retSortIndex);
                                    {
                                        result = *sortIndex;
                                        Jump(&exit);
                                    }
                                }
                            }
                        }
                        Bind(&loopEnd1);
                        {
                            LoopEnd(&loopHead1);
                        }
                    }
                    Bind(&afterLoop1);
                    {
                        Label loopHead2(env);
                        Label loopEnd2(env);
                        Label nextCount1(env);
                        Jump(&loopHead2);
                        LoopBegin(&loopHead2);
                        {
                            BRANCH(Int32LessThan(Int32Add(*midRight, Int32(1)), elements), &nextCount1, &exit);
                            Bind(&nextCount1);
                            {
                                Label hashEqual(env);
                                midRight = Int32Add(*midRight, Int32(1));
                                sortIndex = GetSortedIndex(glue, layoutInfo, *midRight);
                                currentKey = GetKey(glue, layoutInfo, *sortIndex);
                                BRANCH(Int32Equal(GetKeyHashCode(glue, *currentKey, hir), keyHash), &hashEqual, &exit);
                                Bind(&hashEqual);
                                {
                                    Label retIndex2(env);
                                    BRANCH(IntPtrEqual(*currentKey, key), &retIndex2, &loopEnd2);
                                    Bind(&retIndex2);
                                    {
                                        Label retSortIndex(env);
                                        BRANCH(Int32LessThan(*sortIndex, propsNum), &retSortIndex, &exit);
                                        Bind(&retSortIndex);
                                        {
                                            result = *sortIndex;
                                            Jump(&exit);
                                        }
                                    }
                                }
                            }
                            Bind(&loopEnd2);
                            {
                                LoopEnd(&loopHead2);
                            }
                        }
                    }
                }
            }
        }
    }

    Bind(&loopEnd);
    {
        LoopEnd(&loopHead);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetKeyHashCode(GateRef glue, GateRef key, GateRef hir)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));

    Label exit(env);
    Label isString(env);
    Label isSymblo(env);
    BRANCH(TaggedIsString(glue, key), &isString, &isSymblo);
    Bind(&isString);
    {
        result = GetHashcodeFromString(glue, key, hir);
        Jump(&exit);
    }
    Bind(&isSymblo);
    {
        result = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), key,
            IntPtr(JSSymbol::HASHFIELD_OFFSET)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// JSObject::CreateDataProperty
GateRef StubBuilder::CreateDataProperty(GateRef glue, GateRef obj, GateRef propKey, GateRef value)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    Label next(env);
    Label objIsShared(env);
    Label objIsNotShared(env);
    Label isHole(env);
    Label notHole(env);
    Label hasPendingException(env);

    DEFVARIABLE(result, VariableType::BOOL(), True());
    GateRef SCheckModelIsCHECK = Boolean(true);
    auto flag = DefinePropertyByValue(glue, obj, propKey, value, SCheckModelIsCHECK, ProfileOperation());
    BRANCH(HasPendingException(glue), &hasPendingException, &next);
    Bind(&hasPendingException);
    {
        result = False();
        Jump(&exit);
    }
    Bind(&next);
    {
        BRANCH(TaggedIsHole(flag), &isHole, &notHole);
        Bind(&isHole);
        {
            GateRef temp = CallRuntime(glue, RTSTUB_ID(DefineOwnProperty), {obj, propKey, value});
            result = TaggedIsTrue(temp);
            Jump(&exit);
        }
        Bind(&notHole);
        {
            result = BoolNot(TaggedIsException(flag));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CreateDataPropertyOrThrow(GateRef glue, GateRef obj, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    Label newThrow(env);
    Label isThrow(env);

    DEFVARIABLE(result, VariableType::BOOL(), True());

    CanNotConvertNotValidObject(glue, obj);
    IsNotPropertyKey(TaggedIsPropertyKey(glue, key));

    result = CreateDataProperty(glue, obj, key, value);
    BRANCH(*result, &exit, &isThrow);

    Bind(&isThrow);
    {
        BRANCH(HasPendingException(glue), &exit, &newThrow);
        Bind(&newThrow);
        {
            GateRef msgIntId = Int32(GET_MESSAGE_STRING_ID(CreateDataPropertyFailed));
            CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(msgIntId)});
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::DefineField(GateRef glue, GateRef obj, GateRef propKey, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label next(env);
    Label notObj(env);
    Label newThrow(env);
    Label isObj(env);
    Label hasPendingException(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::JS_ANY(), Undefined());
    BRANCH(IsEcmaObject(glue, obj), &isObj, &notObj);
    Bind(&isObj);
    {
#if ENABLE_NEXT_OPTIMIZATION
        key = ToPropertyKey(glue, propKey);
#else
        key = CallRuntime(glue, RTSTUB_ID(ToPropertyKey), {propKey});
#endif
        BRANCH(HasPendingException(glue), &hasPendingException, &next);
    }
    Bind(&next);
    {
        CreateDataPropertyOrThrow(glue, obj, *key, value);
        BRANCH(HasPendingException(glue), &hasPendingException, &exit);
    }
    Bind(&notObj);
    {
        BRANCH(HasPendingException(glue), &hasPendingException, &newThrow);
    }
    Bind(&hasPendingException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&newThrow);
    {
        GateRef msgIntId = Int32(GET_MESSAGE_STRING_ID(DefineFieldField));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(msgIntId)});
        result = Exception();
        Jump(&exit);
    }

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
    GateRef capacity = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), elements,
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
    GateRef element = GetKeyFromDictionary<NumberDictionary>(glue, elements, *entry);
    Label isHole(env);
    Label notHole(env);
    BRANCH(TaggedIsHole(element), &isHole, &notHole);
    Bind(&isHole);
    Jump(&loopEnd);
    Bind(&notHole);
    Label isUndefined(env);
    Label notUndefined(env);
    BRANCH(TaggedIsUndefined(element), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    result = Int32(-1);
    Jump(&exit);
    Bind(&notUndefined);
    Label isMatch(env);
    Label notMatch(env);
    BRANCH(Int32Equal(index, GetInt32OfTInt(element)), &isMatch, &notMatch);
    Bind(&isMatch);
    result = *entry;
    Jump(&exit);
    Bind(&notMatch);
    Jump(&loopEnd);
    Bind(&loopEnd);
    entry = GetNextPositionForHash(*entry, *count, capacity);
    count = Int32Add(*count, Int32(1));
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// int TaggedHashTable<Derived>::FindEntry(const JSTaggedValue &key) in tagged_hash_table.h
template<typename HashTableT>
GateRef StubBuilder::FindEntryFromHashTable(GateRef glue, GateRef elements, GateRef key, GateRef hir)
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
    GateRef capacity = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), elements,
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
    BRANCH(IsSymbol(glue, key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), key,
            IntPtr(JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        BRANCH(IsString(glue, key), &isString, &notString);
        Bind(&isString);
        {
            hash = GetHashcodeFromString(glue, key, hir);
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
        GateRef element = GetKeyFromDictionary<HashTableT>(glue, elements, *entry);
        Label isHole(env);
        Label notHole(env);
        BRANCH(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                BRANCH(TaggedIsUndefined(element), &isUndefined, &notUndefined);
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
                        BRANCH(Equal(key, element), &isMatch, &notMatch);
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
            LoopEndWithCheckSafePoint(&loopHead, env, glue);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr)
{
    return BitAnd(Equal(element, key), Int32Equal(metaData, attr));
}

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
    GateRef capacity = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), elements,
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
    BRANCH(IsSymbol(glue, key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), key,
            IntPtr(panda::ecmascript::JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        BRANCH(IsString(glue, key), &isString, &notString);
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
        GateRef element = GetKeyFromDictionary<TransitionsDictionary>(glue, elements, *entry);
        Label isHole(env);
        Label notHole(env);
        BRANCH(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                BRANCH(TaggedIsUndefined(element), &isUndefined, &notUndefined);
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
                        BRANCH(IsMatchInTransitionDictionary(element, key, metaData,
                            // metaData is int32 type
                            TruncInt64ToInt32(GetAttributesFromDictionary<TransitionsDictionary>(glue, elements, *entry))),
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
            LoopEndWithCheckSafePoint(&loopHead, env, glue);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// JSObject::HasProperty
GateRef StubBuilder::JSObjectHasProperty(GateRef glue, GateRef obj, GateRef key, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label checkHolder(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());

    IsNotPropertyKey(TaggedIsPropertyKey(glue, key));

    ObjectOperatorStubBuilder opStubBuilder(this, GetCurrentGlobalEnv());

    ObjectOperatorStubBuilder::ObjectOperatorOptions opOptions;
    opOptions.lookupKind = ObjectOperatorStubBuilder::LookupKind::KIND_HAS_PROPERTY;

    ObjectOperatorStubBuilder::ObjectOperatorResult opResult;
    opStubBuilder.InitializeOperatorResults(opResult);
    opResult.holder->WriteVariable(obj);

    opStubBuilder.StartLookup<ObjectOperatorStubBuilder::StartLookupType::NO_RECEIVER>(
        glue, key, &checkHolder, opOptions, opResult, hir);

    // if holder is JSProxy, op's lookup will return. This will be handled by CallRuntime.
    Bind(&checkHolder);
    {
        Label isJSProxy(env);
        Label isFound(env);
        BRANCH(TaggedIsJSProxy(glue, opResult.GetHolder()), &isJSProxy, &isFound);
        Bind(&isJSProxy);
        {
            result = CallRuntime(glue, RTSTUB_ID(JSProxyHasProperty), {opResult.GetHolder(), key});
            Jump(&exit);
        }
        Bind(&isFound);
        {
            result = BooleanToTaggedBooleanPtr(opStubBuilder.IsFound(opResult.metaData));
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    opStubBuilder.FinalizeOperatorResults(opResult);
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::JSObjectGetPropertyWithRep(GateRef glue, GateRef obj, GateRef hclass, GateRef attr)
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
    BRANCH(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            result = GetPropertyInlinedProps(glue, obj, hclass, attrOffset);
            Jump(&post);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array =
                Load(VariableType::JS_ANY(), glue, obj, IntPtr(JSObject::PROPERTIES_OFFSET));
            result = GetValueFromTaggedArray(glue, array, Int32Sub(attrOffset,
                GetInlinedPropertiesFromHClass(hclass)));
            Jump(&post);
        }
    }
    Bind(&post);
    {
        Label nonDoubleToTagged(env);
        Label doubleToTagged(env);
        BRANCH(IsDoubleRepInPropAttr(rep), &doubleToTagged, &nonDoubleToTagged);
        Bind(&doubleToTagged);
        {
            result = TaggedPtrToTaggedDoublePtr(*result);
            Jump(&exit);
        }
        Bind(&nonDoubleToTagged);
        {
            Label intToTagged(env);
            BRANCH(IsIntRepInPropAttr(rep), &intToTagged, &exit);
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
    BRANCH(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
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
            GateRef array = Load(VariableType::JS_POINTER(), glue, obj,
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
    GateRef propertiesStep = LoadPrimitive(VariableType::INT32(), glue,
        IntPtr(JSThread::GlueData::GetPropertiesGrowStepOffset(env->Is32Bit())));
    GateRef newL = Int32Add(oldLength, propertiesStep);
    Label reachMax(env);
    Label notReachMax(env);
    BRANCH(Int32GreaterThan(newL, maxNonInlinedFastPropsCapacity), &reachMax, &notReachMax);
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
#if ENABLE_NEXT_OPTIMIZATION
GateRef StubBuilder::ComputeElementCapacity(GateRef oldLength)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef newL = Int32Add(oldLength, Int32LSR(oldLength, Int32(1)));
    // Handle small array edge cases: for small arrays, 1.5 times expansion may not provide sufficient growth,
    // so adding a fixed value (like 16) avoids frequent capacity expansion in the short term.
    newL = Int32Add(newL, Int32(16));
    result = newL;
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
#else
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
    BRANCH(Int32GreaterThan(newL, Int32(JSObject::MIN_ELEMENTS_LENGTH)), &reachMin, &notReachMin);
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
#endif
GateRef StubBuilder::CallGetterHelper(
    GateRef glue, GateRef receiver, GateRef holder, GateRef accessor, ProfileOperation callback, GateRef hir)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isInternal(env);
    Label notInternal(env);
    BRANCH(IsAccessorInternal(glue, accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label arrayLength(env);
        Label tryContinue(env);
        auto lengthAccessor = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
        BRANCH(Equal(accessor, lengthAccessor), &arrayLength, &tryContinue);
        Bind(&arrayLength);
        {
            auto length = LoadPrimitive(VariableType::INT32(), holder, IntPtr(JSArray::LENGTH_OFFSET));
            // TaggedInt supports up to INT32_MAX.
            // If length is greater than Int32_MAX, needs to be converted to TaggedDouble.
            auto condition = Int32UnsignedGreaterThan(length, Int32(INT32_MAX));
            Label overflow(env);
            Label notOverflow(env);
            BRANCH(condition, &overflow, &notOverflow);
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
        auto getter = Load(VariableType::JS_ANY(), glue, accessor,
                           IntPtr(AccessorData::GETTER_OFFSET));
        Label objIsUndefined(env);
        Label objNotUndefined(env);
        Label callExit(env);
        BRANCH(TaggedIsUndefined(getter), &objIsUndefined, &objNotUndefined);
        // if getter is undefined, return undefiend
        Bind(&objIsUndefined);
        {
            result = Undefined();
            Jump(&exit);
        }
        Bind(&objNotUndefined);
        {
            DEFVARIABLE(tmpResult, VariableType::JS_ANY(), Exception());
            JSCallArgs callArgs(JSCallMode::CALL_GETTER);
            callArgs.callGetterArgs = { receiver };
            CallStubBuilder callBuilder(this, glue, getter, Int32(0), 0, &tmpResult, Circuit::NullGate(), callArgs,
                callback, true, hir);
            if (env->IsBaselineBuiltin()) {
                callBuilder.JSCallDispatchForBaseline(&callExit);
                Bind(&callExit);
            } else {
                tmpResult = callBuilder.JSCallDispatch();
            }
            Label noPendingException(env);
            BRANCH(HasPendingException(glue), &exit, &noPendingException);
            Bind(&noPendingException);
            {
                result = *tmpResult;
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
    GateRef glue, GateRef receiver, GateRef accessor, GateRef value, ProfileOperation callback, bool mayThrow)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isInternal(env);
    Label notInternal(env);
    BRANCH(IsAccessorInternal(glue, accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        if (mayThrow) {
            result = CallRuntime(glue, RTSTUB_ID(CallInternalSetter), {receiver, accessor, value});
        } else {
            result = CallRuntime(glue, RTSTUB_ID(CallInternalSetterNoThrow), {receiver, accessor, value});
        }
        Jump(&exit);
    }
    Bind(&notInternal);
    {
        auto setter = Load(VariableType::JS_ANY(), glue, accessor, IntPtr(AccessorData::SETTER_OFFSET));
        Label objIsUndefined(env);
        Label objNotUndefined(env);
        Label callExit(env);
        BRANCH(TaggedIsUndefined(setter), &objIsUndefined, &objNotUndefined);
        Bind(&objIsUndefined);
        {
            if (mayThrow) {
                CallRuntime(glue, RTSTUB_ID(ThrowSetterIsUndefinedException), {});
                result = Exception();
            } else {
                result = TaggedFalse();
            }
            Jump(&exit);
        }
        Bind(&objNotUndefined);
        {
            DEFVARIABLE(tmpResult, VariableType::JS_ANY(), Exception());
            JSCallArgs callArgs(JSCallMode::CALL_SETTER);
            callArgs.callSetterArgs = { receiver, value };
            CallStubBuilder callBuilder(this, glue, setter, Int32(1), 0, &tmpResult, Circuit::NullGate(), callArgs,
                callback);
            if (env->IsBaselineBuiltin()) {
                callBuilder.JSCallDispatchForBaseline(&callExit);
                Bind(&callExit);
            } else {
                tmpResult = callBuilder.JSCallDispatch();
            }
            Label noPendingException(env);
            BRANCH(HasPendingException(glue), &exit, &noPendingException);
            Bind(&noPendingException);
            {
                if (mayThrow) {
                    result = *tmpResult;
                } else {
                    result = Undefined();
                }
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ShouldCallSetter(GateRef glue, GateRef receiver, GateRef holder, GateRef accessor, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    Label isInternal(env);
    Label notInternal(env);
    BRANCH(IsAccessorInternal(glue, accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label receiverEqualsHolder(env);
        Label receiverNotEqualsHolder(env);
        BRANCH(Equal(receiver, holder), &receiverEqualsHolder, &receiverNotEqualsHolder);
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

void StubBuilder::JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    GateRef hclass = LoadHClass(glue, receiver);
    GateRef metaData = GetPropertyMetaDataFromAttr(attr);
    GateRef newClass = FindTransitions(glue, hclass, key, metaData, value);
    Label findHClass(env);
    Label notFindHClass(env);
    BRANCH(Equal(newClass, Undefined()), &notFindHClass, &findHClass);
    Bind(&findHClass);
    {
        GateRef isAOTHClass = IsAOTHClass(newClass);
        Label setPrototype(env);
        Label endSetPrototypeCheck(env);
        Branch(isAOTHClass, &setPrototype, &endSetPrototypeCheck);
        Bind(&setPrototype);
        {
            GateRef prototype = GetPrototypeFromHClass(glue, hclass);
            StorePrototype(glue, newClass, prototype);
            Jump(&endSetPrototypeCheck);
        }
        Bind(&endSetPrototypeCheck);
        GateRef oldKind = GetElementsKindFromHClass(LoadHClass(glue, receiver));
        RestoreElementsKindToGeneric(glue, newClass);
        TransitionHClass(glue, receiver, newClass);
#if ECMASCRIPT_ENABLE_IC
        Label needUpdateAOTHClass(env);
        Label normalNotify(env);
        Label endUpdate(env);
        GateRef updateCondition = LogicAndBuilder(env).And(isAOTHClass).And(IsPrototypeHClass(newClass)).Done();
        Branch(updateCondition, &needUpdateAOTHClass, &normalNotify);
        Bind(&needUpdateAOTHClass);
        {
            TryMigrateToGenericKindForJSObject(glue, receiver, oldKind);
            CallRuntime(glue, RTSTUB_ID(UpdateAOTHClass),
                        { hclass, newClass, key });
            Jump(&endUpdate);
        }
        Bind(&normalNotify);
        {
            // Because we currently only supports Fast ElementsKind
            TryMigrateToGenericKindForJSObject(glue, receiver, oldKind);
            NotifyHClassChanged(glue, hclass, newClass);
            Jump(&endUpdate);
        }
        Bind(&endUpdate);
#else
        // Because we currently only supports Fast ElementsKind
        CallRuntime(glue, RTSTUB_ID(TryRestoreElementsKind), { receiver, newClass });
#endif
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
                    { hclass, newJshclass, key, Int64ToTaggedInt(attr) });
#if ECMASCRIPT_ENABLE_IC
        NotifyHClassChanged(glue, hclass, newJshclass);
#endif
        // Because we currently only supports Fast ElementsKind
        RestoreElementsKindToGeneric(glue, newJshclass);
        TransitionHClass(glue, receiver, newJshclass);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
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
    GateRef hclass = LoadHClass(glue, receiver);
    // 0x111 : default attribute for property: writable, enumerable, configurable
    DEFVARIABLE(attr, VariableType::INT64(), propertyAttributes);
    GateRef numberOfProps = GetNumberOfPropsFromHClass(hclass);
    GateRef inlinedProperties = GetInlinedPropertiesFromHClass(hclass);
    Label hasUnusedInProps(env);
    Label noUnusedInProps(env);
    Label afterInPropsCon(env);
    BRANCH(Int32UnsignedLessThan(numberOfProps, inlinedProperties), &hasUnusedInProps, &noUnusedInProps);
    {
        Bind(&noUnusedInProps);
        Jump(&afterInPropsCon);
        Bind(&hasUnusedInProps);
        {
            attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
            attr = SetIsInlinePropsFieldInPropAttr(*attr, Int32(1)); // 1: set inInlineProps true
            attr = SetTaggedRepInPropAttr(*attr);
            attr = ProfilerStubBuilder(env).UpdateTrackTypeInPropAttr(*attr, value, callback);
            GateRef oldKind = GetElementsKindFromHClass(LoadHClass(glue, receiver));
            JSHClassAddProperty(glue, receiver, key, *attr, value);
            TryMigrateToGenericKindForJSObject(glue, receiver, oldKind);
            GateRef newHclass = LoadHClass(glue, receiver);
            GateRef newLayoutInfo = GetLayoutFromHClass(glue, newHclass);
            GateRef offset = GetInlinedPropOffsetFromHClass(hclass, numberOfProps);
            attr = GetPropAttrFromLayoutInfo(glue, newLayoutInfo, numberOfProps);
            SetValueWithAttr(glue, receiver, offset, key, value, *attr);
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&afterInPropsCon);
    DEFVARIABLE(array, VariableType::JS_POINTER(), GetPropertiesArray(glue, receiver));
    DEFVARIABLE(length, VariableType::INT32(), GetLengthOfTaggedArray(*array));
    Label lenIsZero(env);
    Label lenNotZero(env);
    Label afterLenCon(env);
    BRANCH(Int32Equal(*length, Int32(0)), &lenIsZero, &lenNotZero);
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
    BRANCH(IsDictionaryMode(glue, *array), &isDictMode, &notDictMode);
    {
        Bind(&isDictMode);
        {
            GateRef res = CallRuntime(glue, RTSTUB_ID(NameDictPutIfAbsent),
                                      {receiver, *array, key, value, Int64ToTaggedInt(*attr), TaggedFalse()});
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
            BRANCH(Int32GreaterThanOrEqual(numberOfProps, Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY)),
                &ChangeToDict, &notChangeToDict);
            {
                Bind(&ChangeToDict);
                {
                    attr = SetDictionaryOrderFieldInPropAttr(*attr,
                        Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY));
                    GateRef res = CallRuntime(glue, RTSTUB_ID(NameDictPutIfAbsent),
                        { receiver, *array, key, value, Int64ToTaggedInt(*attr), TaggedTrue() });
                    Label isPendingException(env);
                    Label noPendingException(env);
                    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
                    Bind(&isPendingException);
                    {
                        result = Exception();
                        Jump(&exit);
                    }
                    Bind(&noPendingException);
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
            BRANCH(Int32Equal(*length, outProps), &isArrayFull, &arrayNotFull);
            {
                Bind(&isArrayFull);
                {
                    GateRef maxNonInlinedFastPropsCapacity =
                        Int32Sub(Int32(PropertyAttributes::MAX_FAST_PROPS_CAPACITY), inlinedProperties);
                    GateRef capacity = ComputeNonInlinedFastPropsCapacity(glue, *length,
                        maxNonInlinedFastPropsCapacity);
                    NewObjectStubBuilder newBuilder(this, glue);
                    array = newBuilder.CopyArray(glue, *array, *length, capacity);
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
                GateRef oldKind = GetElementsKindFromHClass(LoadHClass(glue, receiver));
                JSHClassAddProperty(glue, receiver, key, *attr, value);
                TryMigrateToGenericKindForJSObject(glue, receiver, oldKind);
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

    BRANCH(TaggedIsInt(value), &isInt, &notInt);
    Bind(&isInt);
    {
        resultRep = Int64(static_cast<int32_t>(Representation::INT));
        Jump(&exit);
    }
    Bind(&notInt);
    {
        Label isDouble(env);
        Label notDouble(env);
        BRANCH(TaggedIsDouble(value), &isDouble, &notDouble);
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

GateRef StubBuilder::TaggedToElementKind(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::INT32(), Int32(Elements::ToUint(ElementsKind::TAGGED)));
    Label isHole(env);
    Label isNotHole(env);
    BRANCH(TaggedIsHole(value), &isHole, &isNotHole);
    Bind(&isHole);
    {
        result = Int32(Elements::ToUint(ElementsKind::HOLE));
        Jump(&exit);
    }
    Bind(&isNotHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        BRANCH(TaggedIsInt(value), &isInt, &isNotInt);
        Bind(&isInt);
        {
            result = Int32(Elements::ToUint(ElementsKind::INT));
            Jump(&exit);
        }
        Bind(&isNotInt);
        {
            Label isObject(env);
            Label isDouble(env);
            BRANCH(TaggedIsObject(value), &isObject, &isDouble);
            Bind(&isDouble);
            {
                result = Int32(Elements::ToUint(ElementsKind::NUMBER));
                Jump(&exit);
            }
            Bind(&isObject);
            {
                Label isHeapObject(env);
                BRANCH(TaggedIsHeapObject(value), &isHeapObject, &exit);
                Bind(&isHeapObject);
                {
                    Label isString(env);
                    Label isNonString(env);
                    BRANCH(TaggedIsString(glue, value), &isString, &isNonString);
                    Bind(&isString);
                    {
                        result = Int32(Elements::ToUint(ElementsKind::STRING));
                        Jump(&exit);
                    }
                    Bind(&isNonString);
                    {
                        result = Int32(Elements::ToUint(ElementsKind::OBJECT));
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

void StubBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value,
                        MemoryAttribute mAttr)
{
    if (!env_->IsAsmInterp()) {
        env_->GetBuilder()->Store(type, glue, base, offset, value, mAttr);
    } else {
        auto depend = env_->GetCurrentLabel()->GetDepend();
        auto bit = LoadStoreAccessor::ToValue(mAttr);
        GateRef result = env_->GetCircuit()->NewGate(
            env_->GetCircuit()->Store(bit), MachineType::NOVALUE,
            { depend, glue, base, offset, value, value }, type.GetGateType());
        env_->GetCurrentLabel()->SetDepend(result);
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
    BRANCH(IsDoubleRepInPropAttr(rep), &repIsDouble, &repIsNonDouble);
    Bind(&repIsDouble);
    {
        Label valueIsInt(env);
        Label valueIsNotInt(env);
        BRANCH(TaggedIsInt(value), &valueIsInt, &valueIsNotInt);
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
            BRANCH(TaggedIsObject(value), &valueIsObject, &valueIsDouble);
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
        BRANCH(IsIntRepInPropAttr(rep), &repIsInt, &repIsTagged);
        Bind(&repIsInt);
        {
            Label valueIsInt(env);
            Label valueIsNotInt(env);
            BRANCH(TaggedIsInt(value), &valueIsInt, &valueIsNotInt);
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

void StubBuilder::VerifyBarrier(GateRef glue, GateRef obj, [[maybe_unused]] GateRef offset, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label checkNext(env);
    BRANCH_UNLIKELY(LoadPrimitive(
        VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &exit, &checkNext);
    Bind(&checkNext);
    // ObjectAddressToRange function may cause obj is not an object. GC may not mark this obj.
    GateRef objectRegion = ObjectAddressToRange(obj);
    GateRef valueRegion = ObjectAddressToRange(value);
    Label fatal(env);
    Label noFatal(env);
    int msgId = GET_MESSAGE_STRING_ID(SharedObjectRefersLocalObject);
    BRANCH(BitAnd(InSharedHeap(objectRegion), BoolNot(InSharedHeap(valueRegion))), &fatal, &exit);
    Bind(&fatal);
    {
        FatalPrint(glue, {Int32(msgId)});
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::GetCMCRegionRSet(GateRef obj)
{
    GateRef metaDataAddr = IntPtrAnd(TaggedCastToIntPtr(obj),
                                     IntPtr(~static_cast<uint64_t>(common::RegionDesc::DEFAULT_REGION_UNIT_MASK)));
    GateRef regionRSet = LoadPrimitive(VariableType::NATIVE_POINTER(), metaDataAddr,
                                       IntPtr(common::RegionDesc::REGION_RSET_IN_INLINED_METADATA_OFFSET));
    return regionRSet;
}

GateRef StubBuilder::GetCMCRegionType(GateRef obj)
{

    GateRef metaDataAddr = IntPtrAnd(TaggedCastToIntPtr(obj), IntPtr(~common::RegionDesc::DEFAULT_REGION_UNIT_MASK));
    GateRef regionDescAddr = LoadPrimitive(VariableType::INT8(), metaDataAddr,
                                   IntPtr(common::RegionDesc::REGION_TYPE_IN_INLINED_METADATA_OFFSET));
    return regionDescAddr;
}

GateRef StubBuilder::CMCIsInYoungSpace(GateRef regionType)
{
    auto env = GetEnvironment();
    GateRef ret = LogicOrBuilder(env)
        .Or(Int8Equal(regionType, Int8(static_cast<int8_t>(common::RegionDesc::RegionType::THREAD_LOCAL_REGION))))
        .Or(Int8Equal(regionType, Int8(static_cast<int8_t>(common::RegionDesc::RegionType::RECENT_FULL_REGION))))
        .Or(Int8Equal(regionType, Int8(static_cast<int8_t>(common::RegionDesc::RegionType::FROM_REGION))))
        .Done();
    return ret;
}

GateRef StubBuilder::IsOldToYoung(GateRef objRegionType, GateRef valueRegionType)
{
    auto env = GetEnvironment();
    GateRef isOldToYoung = LogicAndBuilder(env)
        .And(BoolNot(CMCIsInYoungSpace(objRegionType)))
        .And(CMCIsInYoungSpace(valueRegionType)).Done();
    return isOldToYoung;
}

GateRef StubBuilder::GetGCPhase(GateRef glue)
{
    GateRef gcPhase = LoadPrimitive(VariableType::INT8(), glue,
        Int64(JSThread::GlueData::GetSharedGCStateBitFieldOffset(false) +
        JSThread::CMCGCPhaseBits::START_BIT / BITS_PER_BYTE));
    return gcPhase;
}

GateRef StubBuilder::GetGCReason(GateRef glue)
{
    GateRef gcReason = LoadPrimitive(VariableType::INT32(), glue,
        Int64(JSThread::GlueData::GetSharedGCStateBitFieldOffset(false) +
        JSThread::CMCGCReasonBits::START_BIT / BITS_PER_BYTE));
    return gcReason;
}

void StubBuilder::MarkRSetCardTable(GateRef obj, Label *exit)
{
    auto env = GetEnvironment();
    Label markBit(env);
    GateRef regionBase = IntPtrAnd(TaggedCastToIntPtr(obj),
        IntPtr(~static_cast<uint64_t>(common::RegionDesc::DEFAULT_REGION_UNIT_MASK)));
    GateRef objOffset = PtrSub(TaggedCastToIntPtr(obj), regionBase);
    GateRef rset = GetCMCRegionRSet(obj);
    GateRef cardIdx = IntPtrDiv(IntPtrDiv(objOffset, IntPtr(common::kMarkedBytesPerBit)),
                                IntPtr(common::kBitsPerWord));
    GateRef headMaskBitStart = IntPtrMod(IntPtrDiv(objOffset, IntPtr(common::kMarkedBytesPerBit)),
                                            IntPtr(common::kBitsPerWord));
    GateRef headMaskBits = Int64LSL(Int64(1), headMaskBitStart);
    GateRef cardOffset = PtrMul(cardIdx, IntPtr(common::kBytesPerWord));
    GateRef cardTable = PtrAdd(rset, IntPtr(common::RegionRSet::CARD_TABLE_DATA_OFFSET));
    GateRef card = LoadPrimitive(VariableType::INT64(), cardTable, cardOffset);
    GateRef isMarked = Int64NotEqual(Int64And(card, headMaskBits), Int64(0));
    BRANCH_NO_WEIGHT(isMarked, exit, &markBit);
    Bind(&markBit);
    {
        Int64FetchOr(PtrAdd(cardTable, cardOffset), headMaskBits, MemoryAttribute::Default());
        Jump(exit);
    }
}

GateRef StubBuilder::ShouldGetGCReason(GateRef gcPhase)
{
    auto env = GetEnvironment();
    GateRef shouldGetGCReason = LogicOrBuilder(env)
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_ENUM)))
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_MARK)))
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_POST_MARK)))
        .Done();
    return shouldGetGCReason;
}

GateRef StubBuilder::ShouldProcessSATB(GateRef gcPhase)
{
    auto env = GetEnvironment();
    GateRef shouldProcessSATB = LogicOrBuilder(env)
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_ENUM)))
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_MARK)))
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_FINAL_MARK)))
        .Or(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_REMARK_SATB)))
        .Done();
    return shouldProcessSATB;
}

GateRef StubBuilder::ShouldUpdateRememberSet(GateRef glue, GateRef gcPhase)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label checkOldToYoung(env);
    Label exit(env);
    Label notMarkRSet(env);
    Label notIdlePhase(env);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    BRANCH(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_IDLE)), &exit, &notIdlePhase);
    Bind(&notIdlePhase);
    GateRef gcReason = GetGCReason(glue);
    Label reasonNotYoung(env);
    BRANCH(Int32Equal(gcReason, Int32(common::GCReason::GC_REASON_YOUNG)), &exit, &reasonNotYoung);
    Bind(&reasonNotYoung);
    GateRef shouldGetGCReason = ShouldGetGCReason(gcPhase);
    BRANCH(BoolNot(shouldGetGCReason), &exit, &notMarkRSet);
    Bind(&notMarkRSet);
    result = False();
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::CMCSetValueWithBarrier(GateRef glue, GateRef obj, [[maybe_unused]]GateRef offset, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label checkOldToYoung(env);
    Label markRSet(env);
    Label notMarkRSet(env);
    GateRef gcPhase = GetGCPhase(glue);
    BRANCH(ShouldUpdateRememberSet(glue, gcPhase), &checkOldToYoung, &notMarkRSet);
    Bind(&checkOldToYoung);
    {
        GateRef objRegionType = GetCMCRegionType(obj);
        GateRef valueRegionType = GetCMCRegionType(value);
        GateRef isOldToYoung = IsOldToYoung(objRegionType, valueRegionType);
        BRANCH_UNLIKELY(isOldToYoung, &markRSet, &notMarkRSet);
        Bind(&markRSet);
        MarkRSetCardTable(obj, &notMarkRSet);
    }
    Bind(&notMarkRSet);
    Label markInBuffer(env);
    GateRef shouldProcessSATB = ShouldProcessSATB(gcPhase);
    BRANCH_UNLIKELY(shouldProcessSATB, &markInBuffer, &exit);
    Bind(&markInBuffer);
    {
        CallNGCRuntime(glue, RTSTUB_ID(MarkInBuffer), {glue, value});
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::CMCArrayCopyWriteBarrier(GateRef glue, GateRef dstObj, GateRef src, GateRef dst, GateRef count)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label iLessLength(env);
    Label isTaggedObject(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label markRSet(env);
    Label notMarkRSet(env);
    GateRef objRegionType = GetCMCRegionType(dstObj);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef gcPhase = GetGCPhase(glue);
    Label checkOldToYoung(env);
    Label objInOld(env);
    BRANCH(BoolNot(CMCIsInYoungSpace(objRegionType)), &objInOld, &notMarkRSet);
    Bind(&objInOld);
    BRANCH(ShouldUpdateRememberSet(glue, gcPhase), &checkOldToYoung, &notMarkRSet);
    Bind(&checkOldToYoung);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*i, count), &iLessLength, &notMarkRSet);
        Bind(&iLessLength);
        GateRef offset = PtrMul(ZExtInt32ToPtr(*i), IntPtr(JSTaggedValue::TaggedTypeSize()));
        GateRef ref = LoadPrimitive(VariableType::JS_ANY(), src, offset);
        BRANCH(TaggedIsHeapObject(ref), &isTaggedObject, &loopEnd);
        Bind(&isTaggedObject);
        GateRef refRegionType = GetCMCRegionType(ref);
        BRANCH(CMCIsInYoungSpace(refRegionType), &markRSet, &loopEnd);
        Bind(&markRSet);
        MarkRSetCardTable(dstObj, &notMarkRSet);
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
    }
    Bind(&notMarkRSet);
    Label markInBuffer(env);
    GateRef shouldProcessSATB = ShouldProcessSATB(gcPhase);
    BRANCH_UNLIKELY(shouldProcessSATB, &markInBuffer, &exit);
    Bind(&markInBuffer);
    CallNGCRuntime(glue, RTSTUB_ID(BatchMarkInBuffer), {glue, TaggedCastToIntPtr(src), count});
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::CMCArrayCopyWriteBarrierSameArray(GateRef glue, GateRef dstObj, GateRef src, GateRef dst, GateRef count)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label iLessLength(env);
    Label isTaggedObject(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label markRSet(env);
    Label notMarkRSet(env);
    Label notIdlePhase(env);
    GateRef objRegionType = GetCMCRegionType(dstObj);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef gcPhase = GetGCPhase(glue);
    BRANCH(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_IDLE)), &exit, &notIdlePhase);
    Bind(&notIdlePhase);
    {
        Label checkOldToYoung(env);
        Label objInOld(env);
        BRANCH(BoolNot(CMCIsInYoungSpace(objRegionType)), &objInOld, &notMarkRSet);
        Bind(&objInOld);
        BRANCH(ShouldUpdateRememberSet(glue, gcPhase), &checkOldToYoung, &notMarkRSet);
        Bind(&checkOldToYoung);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH(Int32UnsignedLessThan(*i, count), &iLessLength, &notMarkRSet);
            Bind(&iLessLength);
            GateRef offset = PtrMul(ZExtInt32ToPtr(*i), IntPtr(JSTaggedValue::TaggedTypeSize()));
            GateRef ref = LoadPrimitive(VariableType::JS_ANY(), src, offset);
            BRANCH(TaggedIsHeapObject(ref), &isTaggedObject, &loopEnd);
            Bind(&isTaggedObject);
            GateRef refRegionType = GetCMCRegionType(ref);
            BRANCH(CMCIsInYoungSpace(refRegionType), &markRSet, &loopEnd);
            Bind(&markRSet);
            MarkRSetCardTable(dstObj, &notMarkRSet);
            Bind(&loopEnd);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&loopHead);
        }
        Bind(&notMarkRSet);
        Label markInBuffer(env);
        GateRef shouldProcessSATB = ShouldProcessSATB(gcPhase);
        BRANCH_UNLIKELY(shouldProcessSATB, &markInBuffer, &exit);
        Bind(&markInBuffer);
        CallNGCRuntime(glue, RTSTUB_ID(BatchMarkInBuffer), {glue, TaggedCastToIntPtr(src), count});
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value,
                                      MemoryAttribute::ShareFlag share)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label callRuntime(env);
    Label checkNext(env);
    BRANCH_LIKELY(LoadPrimitive(
        VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &callRuntime, &checkNext);
    Bind(&callRuntime);
    {
        CMCSetValueWithBarrier(glue, obj, offset, value);
        Jump(&exit);
    }
    Bind(&checkNext);
    // ObjectAddressToRange function may cause obj is not an object. GC may not mark this obj.
    GateRef objectRegion = ObjectAddressToRange(obj);
    GateRef valueRegion = ObjectAddressToRange(value);
#ifndef NDEBUG
    Label fatal(env);
    Label noFatal(env);
    int msgId = GET_MESSAGE_STRING_ID(SharedObjectRefersLocalObject);
    if (share == MemoryAttribute::SHARED) {
        BRANCH(BoolNot(InSharedHeap(valueRegion)), &fatal, &noFatal);
        msgId = GET_MESSAGE_STRING_ID(ValueIsNonSObject);
    }
    if (share == MemoryAttribute::NON_SHARE) {
        BRANCH(InSharedHeap(objectRegion), &fatal, &noFatal);
    }
    if (share == MemoryAttribute::UNKNOWN) {
        BRANCH(BitAnd(InSharedHeap(objectRegion), BoolNot(InSharedHeap(valueRegion))), &fatal, &noFatal);
    }
    Bind(&fatal);
    {
        FatalPrint(glue, {Int32(msgId)});
        Jump(&exit);
    }
    Bind(&noFatal);
#endif
    switch (share) {
        case MemoryAttribute::SHARED: {
            SetSValueWithBarrier(glue, obj, offset, value, objectRegion, valueRegion);
            Jump(&exit);
            break;
        }
        case MemoryAttribute::NON_SHARE: {
            SetNonSValueWithBarrier(glue, obj, offset, value, objectRegion, valueRegion);
            Jump(&exit);
            break;
        }
        case MemoryAttribute::UNKNOWN: {
            Label valueIsShared(env);
            Label valueIsNotShared(env);
            BRANCH_UNLIKELY(InSharedHeap(valueRegion), &valueIsShared, &valueIsNotShared);
            Bind(&valueIsShared);
            {
                SetSValueWithBarrier(glue, obj, offset, value, objectRegion, valueRegion);
                Jump(&exit);
            }
            Bind(&valueIsNotShared);
            {
                SetNonSValueWithBarrier(glue, obj, offset, value, objectRegion, valueRegion);
                Jump(&exit);
            }
            break;
        }
        default:
            UNREACHABLE();
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::SetSValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value, GateRef objectRegion,
                                       GateRef valueRegion)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label needBarrier(env);
    GateRef valueRegionInSweepableShare = InSharedSweepableSpace(valueRegion);
    BRANCH(valueRegionInSweepableShare, &needBarrier, &exit);
    Bind(&needBarrier);
    {
        Label updateLocalToShareRSet(env);
        Label checkBarrierForSharedValue(env);
        GateRef objectNotInShare = BoolNot(InSharedHeap(objectRegion));
        BRANCH(objectNotInShare, &updateLocalToShareRSet, &checkBarrierForSharedValue);
        Bind(&updateLocalToShareRSet);
        {
            Label callSharedBarrier(env);
            Label storeToSharedRSet(env);
            GateRef loadOffset = IntPtr(Region::PackedData::GetLocalToShareSetOffset(env_->Is32Bit()));
            auto localToShareSet = LoadPrimitive(VariableType::NATIVE_POINTER(), objectRegion, loadOffset);
            BRANCH(IntPtrEqual(localToShareSet, IntPtr(0)), &callSharedBarrier, &storeToSharedRSet);
            Bind(&storeToSharedRSet);
            {
                GateRef slotAddr = PtrAdd(TaggedCastToIntPtr(obj), offset);
                // (slotAddr - this) >> TAGGED_TYPE_SIZE_LOG
                GateRef bitOffsetPtr = IntPtrLSR(PtrSub(slotAddr, objectRegion), IntPtr(TAGGED_TYPE_SIZE_LOG));
                GateRef bitOffset = TruncPtrToInt32(bitOffsetPtr);
                GateRef bitPerWordLog2 = Int32(GCBitset::BIT_PER_WORD_LOG2);
                GateRef bytePerWord = Int32(GCBitset::BYTE_PER_WORD);
                // bitOffset >> BIT_PER_WORD_LOG2
                GateRef index = Int32LSR(bitOffset, bitPerWordLog2);
                GateRef byteIndex = Int32Mul(index, bytePerWord);
                // bitset_[index] |= mask;
                GateRef bitsetData = PtrAdd(localToShareSet, IntPtr(RememberedSet::GCBITSET_DATA_OFFSET));
                GateRef oldsetValue = LoadPrimitive(VariableType::INT32(), bitsetData, byteIndex);
                GateRef mask = GetBitMask(bitOffset);
                GateRef flag = Int32And(oldsetValue, mask);
                // Load the bit using relaxed memory order.
                // If the bit is set, do nothing (local->shared barrier is done).
                Label needSet(env);
                BRANCH(Int32NotEqual(flag, Int32(0)), &checkBarrierForSharedValue, &needSet);
                Bind(&needSet);
                {
                    GateRef newMapValue = Int32Or(oldsetValue, mask);
                    Store(VariableType::INT32(), glue, bitsetData, byteIndex, newMapValue);
                    Jump(&checkBarrierForSharedValue);
                }
            }
            Bind(&callSharedBarrier);
            {
                CallNGCRuntime(glue, RTSTUB_ID(InsertLocalToShareRSet), { glue, obj, offset });
                Jump(&checkBarrierForSharedValue);
            }
        }
        Bind(&checkBarrierForSharedValue);
        {
            Label sharedMarking(env);
            bool isArch32 = GetEnvironment()->Is32Bit();
            GateRef stateBitField = LoadPrimitive(VariableType::INT64(), glue,
                                         Int64(JSThread::GlueData::GetSharedGCStateBitFieldOffset(isArch32)));
            GateRef state = Int64And(stateBitField, Int64(JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK));
            BRANCH(Int64Equal(state, Int64(static_cast<int64_t>(SharedMarkStatus::READY_TO_CONCURRENT_MARK))),
                   &exit, &sharedMarking);

            Bind(&sharedMarking);
            CallNGCRuntime(glue, RTSTUB_ID(SharedGCMarkingBarrier), {glue, obj, offset, value});
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::SetNonSValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value,
                                          GateRef objectRegion, GateRef valueRegion)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label checkMarkStatus(env);
    Label isOldToYoung(env);
    GateRef objectNotInYoung = BoolNot(InYoungGeneration(objectRegion));
    GateRef valueRegionInYoung = InYoungGeneration(valueRegion);
    BRANCH_UNLIKELY(BitAnd(objectNotInYoung, valueRegionInYoung), &isOldToYoung, &checkMarkStatus);

    Bind(&isOldToYoung);
    {
        GateRef loadOffset = IntPtr(Region::PackedData::GetOldToNewSetOffset(env_->Is32Bit()));
        auto oldToNewSet = LoadPrimitive(VariableType::NATIVE_POINTER(), objectRegion, loadOffset);
        Label isNullPtr(env);
        Label notNullPtr(env);
        BRANCH(IntPtrEuqal(oldToNewSet, IntPtr(0)), &isNullPtr, &notNullPtr);
        Bind(&notNullPtr);
        {
            GateRef slotAddr = PtrAdd(TaggedCastToIntPtr(obj), offset);
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
            GateRef oldsetValue = LoadPrimitive(VariableType::INT32(), bitsetData, byteIndex);
            GateRef newmapValue = Int32Or(oldsetValue, GetBitMask(bitOffset));

            Store(VariableType::INT32(), glue, bitsetData, byteIndex, newmapValue);
            Jump(&checkMarkStatus);
        }
        Bind(&isNullPtr);
        {
            CallNGCRuntime(glue, RTSTUB_ID(InsertOldToNewRSet), { glue, obj, offset });
            Jump(&checkMarkStatus);
        }
    }
    Bind(&checkMarkStatus);
    {
        Label marking(env);
        bool isArch32 = GetEnvironment()->Is32Bit();
        GateRef stateBitField = LoadPrimitive(VariableType::INT64(), glue,
                                     Int64(JSThread::GlueData::GetGCStateBitFieldOffset(isArch32)));
        GateRef state = Int64And(stateBitField, Int64(JSThread::CONCURRENT_MARKING_BITFIELD_MASK));
        BRANCH_LIKELY(Int64Equal(state, Int64(static_cast<int64_t>(MarkStatus::READY_TO_MARK))), &exit, &marking);

        Bind(&marking);
        {
            // Check fresh region, and directly mark value instead of call runtime.
            CallNGCRuntime(glue, RTSTUB_ID(MarkingBarrier), {glue, obj, offset, value});
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::GetValueWithBarrier(GateRef glue, GateRef addr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label notHeapObject(env);
    Label isHeapObject(env);
    Label exit(env);

    // TODO: should be atomic
    GateRef value = LoadZeroOffsetPrimitive(VariableType::JS_ANY(), addr);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());

    BRANCH(BoolNot(TaggedIsHeapObject(value)), &notHeapObject, &isHeapObject);
    Bind(&notHeapObject);
    {
        result = value;
        Jump(&exit);
    }
    Bind(&isHeapObject);
    {
        Label isHeapAddress(env);
        Label notHeapAddress(env);
        BRANCH(IsHeapAddress(glue, value), &isHeapAddress, &notHeapAddress);
        Bind(&notHeapAddress);
        {
            result = value;
            Jump(&exit);
        }
        Bind(&isHeapAddress);
        {
            result = FastReadBarrier(glue, addr, value);
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::FastReadBarrier(GateRef glue, GateRef addr, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), value);

    GateRef intValue = ChangeTaggedPointerToInt64(value);
    GateRef regionType = GetCMCRegionType(value);
    Label isFromeSpace(env);
    GateRef fromType = Int8(static_cast<int8_t>(common::RegionDesc::RegionType::FROM_REGION));
    Branch(Int8Equal(regionType, fromType), &isFromeSpace, &exit);
    Bind(&isFromeSpace);
    {
        GateRef weakMask = Int64And(intValue, Int64(JSTaggedValue::TAG_WEAK));
        GateRef obj = Int64And(intValue, Int64(~JSTaggedValue::TAG_WEAK));

        GateRef forwardedAddr = LoadPrimitive(VariableType::INT64(), obj, IntPtr(0));
        GateRef forwardState =
            Int64LSR(forwardedAddr, Int64(common::BaseStateWord::PADDING_WIDTH + common::BaseStateWord::FORWARD_WIDTH));
        Label forwarded(env);
        Label notForwarded(env);
        Branch(Int64Equal(forwardState, Int64(static_cast<int64_t>(common::BaseStateWord::ForwardState::FORWARDED))),
            &forwarded, &notForwarded);
        Bind(&forwarded);
        {
            result = Int64ToTaggedPtr(Int64Or(Int64And(forwardedAddr, Int64(TaggedStateWord::ADDRESS_MASK)), weakMask));
            Jump(&exit);
        }
        Bind(&notForwarded);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(ReadBarrier), { glue, addr });
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsHeapAddress(GateRef glue, GateRef value)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef heapStartAddr = LoadPrimitive(
        VariableType::NATIVE_POINTER(), glue, IntPtr(JSThread::GlueData::GetHeapStartAddrOffset(isArch32)));
    GateRef heapCurrentEnd = LoadPrimitive(
        VariableType::NATIVE_POINTER(), glue, IntPtr(JSThread::GlueData::GetHeapCurrentEndOffset(isArch32)));
    auto IntPtrValue =  ChangeTaggedPointerToInt64(value);
    return BitAnd(IntPtrGreaterThanOrEqual(IntPtrValue, heapStartAddr), IntPtrLessThan(IntPtrValue, heapCurrentEnd));
}

GateRef StubBuilder::TaggedIsBigInt(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(glue, obj)),
                            Int32(static_cast<int32_t>(JSType::BIGINT)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedIsPropertyBox(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef type = GetObjectType(LoadHClass(glue, obj));
        result = Int32Equal(type, Int32(static_cast<int32_t>(JSType::PROPERTY_BOX)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedIsAccessor(GateRef glue, GateRef x)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(TaggedIsHeapObject(x), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef type = GetObjectType(LoadHClass(glue, x));
        result = BitOr(Int32Equal(type, Int32(static_cast<int32_t>(JSType::ACCESSOR_DATA))),
                        Int32Equal(type, Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedIsInternalAccessor(GateRef glue, GateRef x)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(TaggedIsHeapObject(x), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef type = GetObjectType(LoadHClass(glue, x));
        result = Int32Equal(type, Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR)));
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
    GateRef len = LoadPrimitive(VariableType::INT32(), string, IntPtr(BaseString::LENGTH_AND_FLAGS_OFFSET));
    return Int32Equal(
        Int32And(len, Int32((1 << BaseString::CompressedStatusBit::SIZE) - 1)),
        Int32(BaseString::STRING_UNCOMPRESSED));
}

GateRef StubBuilder::IsUtf8String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = LoadPrimitive(VariableType::INT32(), string, IntPtr(BaseString::LENGTH_AND_FLAGS_OFFSET));
    return Int32Equal(
        Int32And(len, Int32((1 << BaseString::CompressedStatusBit::SIZE) - 1)),
        Int32(BaseString::STRING_COMPRESSED));
}

GateRef StubBuilder::IsDigit(GateRef ch)
{
    return BitAnd(Int32LessThanOrEqual(ch, Int32('9')),
        Int32GreaterThanOrEqual(ch, Int32('0')));
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
    BRANCH(TaggedIsInt(key), &isKeyInt, &notKeyInt);
    Bind(&isKeyInt);
    {
        resultKey = GetInt64OfTInt(key);
        Jump(&exit);
    }
    Bind(&notKeyInt);
    {
        Label isString(env);
        Label notString(env);
        BRANCH(TaggedIsString(glue, key), &isString, &notString);
        Bind(&isString);
        {
            BuiltinsStringStubBuilder stringStub(this, GetCurrentGlobalEnv());
            resultKey = stringStub.StringToUint(glue, key, JSObject::MAX_ELEMENT_INDEX - 1);
            Jump(&exit);
        }
        Bind(&notString);
        {
            Label isDouble(env);
            BRANCH(TaggedIsDouble(key), &isDouble, &exit);
            Bind(&isDouble);
            {
                GateRef number = GetDoubleOfTDouble(key);
                GateRef integer = ChangeFloat64ToInt32(number);
                Label isEqual(env);
                BRANCH(DoubleEqual(number, ChangeInt32ToFloat64(integer)), &isEqual, &exit);
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
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef globalRecord = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::GLOBAL_RECORD);
    GateRef recordEntry = FindEntryFromHashTable<NameDictionary>(glue, globalRecord, key);
    Label foundInGlobalRecord(env);
    BRANCH(Int32NotEqual(recordEntry, Int32(-1)), &foundInGlobalRecord, &exit);
    Bind(&foundInGlobalRecord);
    {
        result = GetBoxFromGlobalDictionary(glue, globalRecord, recordEntry);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadFromField(GateRef glue, GateRef receiver, GateRef handlerInfo)
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
#if ENABLE_NEXT_OPTIMIZATION
    BRANCH_LIKELY(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
#else
    BRANCH(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
#endif
    Bind(&handlerInfoIsInlinedProps);
    {
        result = Load(VariableType::JS_ANY(), glue, receiver, PtrMul(ZExtInt32ToPtr(index),
            IntPtr(JSTaggedValue::TaggedTypeSize())));
        Jump(&handlerPost);
    }
    Bind(&handlerInfoNotInlinedProps);
    {
        result = GetValueFromTaggedArray(glue, GetPropertiesArray(glue, receiver), index);
        Jump(&handlerPost);
    }
    Bind(&handlerPost);
    {
        Label nonDoubleToTagged(env);
        Label doubleToTagged(env);
        GateRef rep = HandlerBaseGetRep(handlerInfo);
#if ENABLE_NEXT_OPTIMIZATION
        BRANCH_UNLIKELY(IsDoubleRepInPropAttr(rep), &doubleToTagged, &nonDoubleToTagged);
#else
        BRANCH(IsDoubleRepInPropAttr(rep), &doubleToTagged, &nonDoubleToTagged);
#endif
        Bind(&doubleToTagged);
        {
            result = TaggedPtrToTaggedDoublePtr(*result);
            Jump(&exit);
        }
        Bind(&nonDoubleToTagged);
        {
            Label intToTagged(env);
#if ENABLE_NEXT_OPTIMIZATION
            BRANCH_UNLIKELY(IsIntRepInPropAttr(rep), &intToTagged, &exit);
#else
            BRANCH(IsIntRepInPropAttr(rep), &intToTagged, &exit);
#endif
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

GateRef StubBuilder::LoadGlobal(GateRef glue, GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label cellNotInvalid(env);
    Label cellNotAccessor(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    BRANCH(IsInvalidPropertyBox(glue, cell), &exit, &cellNotInvalid);
    Bind(&cellNotInvalid);
    {
        BRANCH(IsAccessorPropertyBox(glue, cell), &exit, &cellNotAccessor);
        Bind(&cellNotAccessor);
        {
            result = GetValueFromPropertyBox(glue, cell);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CheckPolyHClass(GateRef glue, GateRef cachedValue, GateRef hclass)
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
    BRANCH(TaggedIsWeak(cachedValue), &exit, &cachedValueNotWeak);
    Bind(&cachedValueNotWeak);
    {
        Label isTaggedArray(env);
        Branch(IsTaggedArray(glue, cachedValue), &isTaggedArray, &exit);
        Bind(&isTaggedArray);
        {
            GateRef length = GetLengthOfTaggedArray(cachedValue);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32UnsignedLessThan(*i, length), &iLessLength, &exit);
                Bind(&iLessLength);
                {
                    GateRef element = GetValueFromTaggedArray(glue, cachedValue, *i);
                    BRANCH(Equal(LoadObjectFromWeakRef(element), hclass), &hasHclass, &loopEnd);
                    Bind(&hasHclass);
                    result = GetValueFromTaggedArray(glue, cachedValue, Int32Add(*i, Int32(1)));
                    Jump(&exit);
                }
                Bind(&loopEnd);
                i = Int32Add(*i, Int32(2)); // 2 means one ic, two slot
                LoopEnd(&loopHead);
            }
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
    Label handleInfoIsFound(env);
    Label handleInfoIsNotFound(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerInfoIsNonExist(env);
    Label handlerInfoExist(env);
    Label handlerInfoIsPrimitive(env);
    Label handlerInfoNotPrimitive(env);
    Label handlerInfoNotStringOrNumber(env);
    Label handlerInfoIsStringLength(env);
    Label handlerInfoNotStringLength(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label cellHasChanged(env);
    Label cellNotFoundHasChanged(env);
    Label cellNotFoundNotChanged(env);
    Label cellNotUndefined(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(holder, VariableType::JS_ANY(), argHolder);
    DEFVARIABLE(handler, VariableType::JS_ANY(), argHandler);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = GetInt64OfTInt(*handler);
#if ENABLE_NEXT_OPTIMIZATION
            BRANCH_LIKELY(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
#else
            BRANCH(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
#endif
            Bind(&handlerInfoIsField);
            {
                result = LoadFromField(glue, *holder, handlerInfo);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                BRANCH(BitOr(IsStringElement(handlerInfo), IsNumberHandler(handlerInfo)),
                    &handlerInfoIsPrimitive, &handlerInfoNotStringOrNumber);
                Bind(&handlerInfoNotStringOrNumber);
                {
                    BRANCH(IsBooleanHandler(handlerInfo), &handlerInfoIsPrimitive, &handlerInfoNotPrimitive);
                }
                Bind(&handlerInfoIsPrimitive);
                {
                    result = LoadFromField(glue, *holder, handlerInfo);
                    Jump(&exit);
                }
                Bind(&handlerInfoNotPrimitive);
                {
                    BRANCH(IsNonExist(handlerInfo), &handlerInfoIsNonExist, &handlerInfoExist);
                    // For the special "Not Found" case we may generate ic by "LoadHandler::LoadProperty".
                    // In this situation, you can trust ic without ChangeMarker.
                    Bind(&handlerInfoIsNonExist);
                    {
                        result = Undefined();
                        Jump(&exit);
                    }
                    Bind(&handlerInfoExist);
                    {
                        BRANCH(IsStringLength(handlerInfo), &handlerInfoIsStringLength, &handlerInfoNotStringLength);
                        Bind(&handlerInfoNotStringLength);
                        {
                            GateRef accessor = LoadFromField(glue, *holder, handlerInfo);
                            // The getter may involve nested calls, so it is better to end (or return) early.
                            EndTraceLoad(glue);
                            result = CallGetterHelper(glue, receiver, *holder, accessor, callback);
                            StartTraceLoadGetter(glue);
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
        BRANCH_LIKELY(TaggedIsPrototypeHandler(glue, *handler), &handlerIsPrototypeHandler, &handlerNotPrototypeHandler);
        Bind(&handlerIsPrototypeHandler);
        {
            GateRef cellValue = GetPrototypeHandlerProtoCell(glue, *handler);
            BRANCH_UNLIKELY(TaggedIsUndefined(cellValue), &loopEnd, &cellNotUndefined);
            Bind(&cellNotUndefined);
            BRANCH(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            Bind(&cellHasChanged);
            {
                result = Hole();
                Jump(&exit);
            }
            Bind(&loopEnd);
            holder = GetPrototypeHandlerHolder(glue, *handler);
            BRANCH(Equal(*holder, Undefined()), &handleInfoIsNotFound, &handleInfoIsFound);
            Bind(&handleInfoIsFound);
            {
            handler = GetPrototypeHandlerHandlerInfo(glue, *handler);
            LoopEndWithCheckSafePoint(&loopHead, env, glue);
            }

            // For "Not Found" case (holder equals Undefined()),
            // we should ensure that both GetNotFoundHasChanged() and GetHasChanged() return false.
            Bind(&handleInfoIsNotFound);
            {
                BRANCH(GetNotFoundHasChanged(cellValue), &cellNotFoundHasChanged, &cellNotFoundNotChanged);
                Bind(&cellNotFoundHasChanged);
                {
                    result = Hole();
                    Jump(&exit);
                }
                Bind(&cellNotFoundNotChanged);
                {
                    result = Undefined();
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&handlerNotPrototypeHandler);
    {
    result = LoadGlobal(glue, *handler);
    Jump(&exit);
    }
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
    BRANCH(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    BRANCH(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        GateRef elements = GetElementsArray(glue, receiver);
        BRANCH(Int32LessThanOrEqual(GetLengthOfTaggedArray(elements), index), &lengthLessIndex, &lengthNotLessIndex);
        Bind(&lengthLessIndex);
        Jump(&exit);
        Bind(&lengthNotLessIndex);
        result = GetTaggedValueWithElementsKind(glue, receiver, index);
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
    BRANCH(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    BRANCH(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        FlatStringStubBuilder thisFlat(this);
        thisFlat.FlattenString(glue, receiver, &flattenFastPath);
        Bind(&flattenFastPath);
        BRANCH(Int32LessThanOrEqual(GetLengthFromString(receiver), index), &lengthLessIndex, &lengthNotLessIndex);
        Bind(&lengthLessIndex);
        Jump(&exit);
        Bind(&lengthNotLessIndex);
        BuiltinsStringStubBuilder stringBuilder(this, GetCurrentGlobalEnv());
        StringInfoGateRef stringInfoGate(&thisFlat);
        result = stringBuilder.CreateFromEcmaString(glue, index, stringInfoGate);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedArraySetValue(GateRef glue, GateRef receiver, GateRef value, GateRef index, GateRef capacity)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label indexGreaterLen(env);
    Label storeElement(env);
    Label isTransToDict(env);
    Label notTransToDict(env);
    Label exit(env);
    BRANCH(Int32UnsignedGreaterThanOrEqual(index, capacity), &indexGreaterLen, &storeElement);
    Bind(&indexGreaterLen);
    {
        BRANCH(ShouldTransToDict(capacity, index), &isTransToDict, &notTransToDict);
        Bind(&isTransToDict);
        result = Hole();
        Jump(&exit);
        Bind(&notTransToDict);
        GrowElementsCapacity(glue, receiver, Int32Add(index, Int32(1)));
        Jump(&storeElement);
    }
    Bind(&storeElement);
    {
        SetValueWithElementsKind(glue, receiver, value, index, Boolean(true),
            Int32(Elements::ToUint(ElementsKind::NONE)));
        result = Undefined();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ICStoreElement(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef handler,
                                    bool updateHandler, GateRef profileTypeInfo, GateRef slotId)
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
    Label storeElement(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label greaterThanInt32Max(env);
    Label notGreaterThanInt32Max(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(varHandler, VariableType::JS_ANY(), handler);
    GateRef index64 = TryToElementsIndex(glue, key);
    BRANCH_UNLIKELY(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    BRANCH_UNLIKELY(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        DEFVARIABLE(isOnPrototype, VariableType::BOOL(), False());
        Jump(&loopHead);
        LoopBegin(&loopHead);
        BRANCH(TaggedIsInt(*varHandler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = GetInt64OfTInt(*varHandler);
            BRANCH(IsTypedArrayElement(handlerInfo), &handlerInfoIsTypedArray, &handerInfoNotTypedArray);
            Bind(&handlerInfoIsTypedArray);
            {
                GateRef hclass = LoadHClass(glue, receiver);
                GateRef jsType = GetObjectType(hclass);
                BuiltinsTypedArrayStubBuilder typedArrayBuilder(this, GetCurrentGlobalEnv());
                result = typedArrayBuilder.StoreTypedArrayElement(glue, receiver, index64, value, jsType);
                Jump(&exit);
            }
            Bind(&handerInfoNotTypedArray);
            BRANCH(HandlerBaseIsJSArray(handlerInfo), &handerInfoIsJSArray, &handerInfoNotJSArray);
            Bind(&handerInfoIsJSArray);
            {
                BRANCH(IsJsCOWArray(glue, receiver), &isJsCOWArray, &isNotJsCOWArray);
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
                    BRANCH(Int32UnsignedGreaterThanOrEqual(index, oldLength), &indexGreaterLength, &handerInfoNotJSArray);
                    Bind(&indexGreaterLength);
                    Store(VariableType::INT32(), glue, receiver,
                        IntPtr(panda::ecmascript::JSArray::LENGTH_OFFSET),
                        Int32Add(index, Int32(1)));
                    if (updateHandler) {
                        Label update(env);
                        Label setObject(env);
                        Label setPrototype(env);
                        GateRef oldHandler = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);
                        BRANCH(Equal(oldHandler, Hole()), &handerInfoNotJSArray, &update);
                        Bind(&update);
                        {
                            handler = Int64ToTaggedInt(UpdateSOutOfBoundsForHandler(handlerInfo));
                            BRANCH(Equal(*isOnPrototype, False()), &setObject, &setPrototype);
                            Bind(&setObject);
                            {
                                SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, handler);
                                Jump(&handerInfoNotJSArray);
                            }
                            Bind(&setPrototype);
                            {
                                SetPrototypeHandlerHandlerInfo(glue, oldHandler, handler);
                            }
                        }
                    }
                }
                Jump(&handerInfoNotJSArray);
            }
            Bind(&handerInfoNotJSArray);
            {
                GateRef elements = GetElementsArray(glue, receiver);
                GateRef capacity = GetLengthOfTaggedArray(elements);
                BRANCH(Int32UnsignedGreaterThanOrEqual(index, capacity), &indexGreaterCapacity, &storeElement);
                Bind(&indexGreaterCapacity);
                {
                    result = TaggedArraySetValue(glue, receiver, value, index, capacity);
                    Label transition(env);
                    BRANCH(TaggedIsHole(*result), &exit, &transition);
                    Bind(&transition);
                    {
                        Label hole(env);
                        DEFVARIABLE(kind, VariableType::INT32(), Int32(Elements::ToUint(ElementsKind::NONE)));
                        BRANCH(Int32UnsignedGreaterThan(index, capacity), &hole, &exit);
                        Bind(&hole);
                        {
                            kind = Int32(Elements::ToUint(ElementsKind::HOLE));
                            SetValueWithElementsKind(glue, receiver, value, index, Boolean(true), *kind);
                            Jump(&exit);
                        }
                    }
                }
                Bind(&storeElement);
                {
                    SetValueWithElementsKind(glue, receiver, value, index, Boolean(true),
                                             Int32(Elements::ToUint(ElementsKind::NONE)));
                    result = Undefined();
                    Jump(&exit);
                }
            }
        }
        Bind(&handlerNotInt);
        {
            Label handlerIsPrototypeHandler(env);
            Label handlerNotPrototypeHandler(env);
            Label handlerIsTransWithProtoHandler(env);
            Label handlerIsTransitionHandler(env);

            BRANCH(TaggedIsPrototypeHandler(glue, *varHandler), &handlerIsPrototypeHandler,
                   &handlerNotPrototypeHandler);
            Bind(&handlerIsPrototypeHandler);
            {
                // If the handler is a prototypehandler, we need to check protochangemarker.
                Label cellNotNullOrUndefined(env);
                Label getPrototypeHandlerHandlerInfo(env);
                GateRef cellValue = GetPrototypeHandlerProtoCell(glue, *varHandler);
                BRANCH(TaggedIsUndefinedOrNull(cellValue), &exit, &cellNotNullOrUndefined);

                Bind(&cellNotNullOrUndefined);
                BRANCH(GetHasChanged(cellValue), &exit, &getPrototypeHandlerHandlerInfo);
                Bind(&getPrototypeHandlerHandlerInfo);
                {
                    isOnPrototype = True();
                    varHandler = GetPrototypeHandlerHandlerInfo(glue, *varHandler);
                    Jump(&loopEnd);
                }
            }

            Bind(&handlerNotPrototypeHandler);
            {
                BRANCH(TaggedIsTransWithProtoHandler(glue, *varHandler), &handlerIsTransWithProtoHandler,
                       &handlerIsTransitionHandler);
                Bind(&handlerIsTransWithProtoHandler);
                {
                    // If the handler is a transwithprotohandler, we need to check protochangemarker.
                    Label cellNotNullOrUndefined(env);
                    Label getTransWithProtoHandlerInfo(env);
                    GateRef cellValue = GetTransWithProtoHandlerProtoCell(glue, *varHandler);
                    BRANCH(TaggedIsUndefinedOrNull(cellValue), &exit, &cellNotNullOrUndefined);

                    Bind(&cellNotNullOrUndefined);
                    BRANCH(GetHasChanged(cellValue), &exit, &getTransWithProtoHandlerInfo);
                    Bind(&getTransWithProtoHandlerInfo);
                    {
                        isOnPrototype = True();
                        varHandler = GetTransWithProtoHandlerInfo(glue, *varHandler);
                        Jump(&loopEnd);
                    }
                }
                Bind(&handlerIsTransitionHandler);
                {
                    // If the handler is a transitionhandler, we just get handlerinfo.
                    varHandler = GetTransitionHandlerInfo(glue, *varHandler);
                    Jump(&loopEnd);
                }
            }

            Bind(&loopEnd);
            {
                LoopEndWithCheckSafePoint(&loopHead, env, glue);
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
    GateRef result = LoadPrimitive(VariableType::INT32(), object, lengthOffset);
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
    Label isShared(env);
    Label notShared(env);
    Label sharedObjectStoreBarrier(env);
    Label prepareIntHandlerLoop(env);
    Label handlerIsTransitionHandler(env);
    Label handlerNotTransitionHandler(env);
    Label handlerIsTransWithProtoHandler(env);
    Label handlerNotTransWithProtoHandler(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label handlerIsPropertyBox(env);
    Label handlerNotPropertyBox(env);
    Label handlerIsStoreAOTHandler(env);
    Label handlerNotStoreAOTHandler(env);
    Label aotHandlerInfoIsField(env);
    Label aotHandlerInfoNotField(env);
    Label cellHasChanged(env);
    Label cellNotChanged(env);
    Label cellNotUndefined(env);
    Label aotCellNotChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label JumpLoopHead(env);
    Label cellNotNull(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(holder, VariableType::JS_ANY(), argHolder);
    DEFVARIABLE(handler, VariableType::JS_ANY(), argHandler);
    DEFVARIABLE(actualValue, VariableType::JS_ANY(), value);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = GetInt64OfTInt(*handler);
            BRANCH(IsNonSharedStoreField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                result = StoreField(glue, receiver, *actualValue, handlerInfo, callback);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                BRANCH(IsStoreShared(handlerInfo), &isShared, &notShared);
                Bind(&isShared);
                {
                    BRANCH(HandlerBaseIsAccessor(handlerInfo), &prepareIntHandlerLoop, &sharedObjectStoreBarrier);
                    Bind(&sharedObjectStoreBarrier);
                    {
                        GateRef field = GetFieldTypeFromHandler(handlerInfo);
                        SharedObjectStoreBarrierWithTypeCheck(&result, glue, field, value, &actualValue,
                            &prepareIntHandlerLoop, &exit);
                    }
                    Bind(&prepareIntHandlerLoop);
                    {
                        handler = IntToTaggedPtr(ClearSharedStoreKind(handlerInfo));
                        Jump(&JumpLoopHead);
                    }
                }
                Bind(&notShared);
                GateRef accessor = LoadFromField(glue, *holder, handlerInfo);
                result = CallSetterHelper(glue, receiver, accessor, *actualValue, callback);
                Jump(&exit);
            }
        }
        Bind(&handlerNotInt);
        {
            BRANCH(TaggedIsTransitionHandler(glue, *handler), &handlerIsTransitionHandler, &handlerNotTransitionHandler);
            Bind(&handlerIsTransitionHandler);
            {
                result = StoreWithTransition(glue, receiver, *actualValue, *handler, callback);
                Jump(&exit);
            }
            Bind(&handlerNotTransitionHandler);
            {
                BRANCH(TaggedIsTransWithProtoHandler(glue, *handler), &handlerIsTransWithProtoHandler,
                    &handlerNotTransWithProtoHandler);
                Bind(&handlerIsTransWithProtoHandler);
                {
                    GateRef cellValue = GetTransWithProtoHandlerProtoCell(glue, *handler);
                    BRANCH(GetHasChanged(cellValue), &cellHasChanged, &cellNotChanged);
                    Bind(&cellNotChanged);
                    {
                        result = StoreWithTransition(glue, receiver, *actualValue, *handler, callback, true);
                        Jump(&exit);
                    }
                }
                Bind(&handlerNotTransWithProtoHandler);
                {
                    BRANCH(TaggedIsPrototypeHandler(glue, *handler), &handlerIsPrototypeHandler, &handlerNotPrototypeHandler);
                    Bind(&handlerNotPrototypeHandler);
                    {
                        BRANCH(TaggedIsPropertyBox(glue, *handler), &handlerIsPropertyBox, &handlerNotPropertyBox);
                        Bind(&handlerIsPropertyBox);
                        StoreGlobal(glue, *actualValue, *handler);
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&handlerIsPrototypeHandler);
        {
            GateRef cellValue = GetPrototypeHandlerProtoCell(glue, *handler);
            BRANCH(TaggedIsUndefined(cellValue), &loopEnd, &cellNotUndefined);
            Bind(&cellNotUndefined);
            BRANCH(TaggedIsNull(cellValue), &cellHasChanged, &cellNotNull);
            Bind(&cellNotNull);
            {
                BRANCH(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            }
            Bind(&loopEnd);
            {
                holder = GetPrototypeHandlerHolder(glue, *handler);
                handler = GetPrototypeHandlerHandlerInfo(glue, *handler);
                Jump(&JumpLoopHead);
            }
        }
        Bind(&handlerNotPropertyBox);
        {
            BRANCH(TaggedIsStoreAOTHandler(glue, *handler), &handlerIsStoreAOTHandler, &handlerNotStoreAOTHandler);
            Bind(&handlerIsStoreAOTHandler);
            {
                GateRef cellValue = GetStoreAOTHandlerProtoCell(glue, *handler);
                BRANCH(GetHasChanged(cellValue), &cellHasChanged, &aotCellNotChanged);
                Bind(&aotCellNotChanged);
                {
                    holder = GetStoreAOTHandlerHolder(glue, *handler);
                    handler = GetStoreAOTHandlerHandlerInfo(glue, *handler);
                    GateRef handlerInfo = GetInt64OfTInt(*handler);
                    BRANCH(IsField(handlerInfo), &aotHandlerInfoIsField, &aotHandlerInfoNotField);
                    Bind(&aotHandlerInfoIsField);
                    {
                        result = StoreField(glue, receiver, *actualValue, handlerInfo, callback);
                        Jump(&exit);
                    }
                    Bind(&aotHandlerInfoNotField);
                    {
                        GateRef accessor = LoadFromField(glue, *holder, handlerInfo);
                        result = CallSetterHelper(glue, receiver, accessor, *actualValue, callback);
                        Jump(&exit);
                    }
                }
            }
            Bind(&handlerNotStoreAOTHandler);
            Jump(&exit);
        }
        Bind(&cellHasChanged);
        {
            result = Hole();
            Jump(&exit);
        }
        Bind(&JumpLoopHead);
        {
            LoopEndWithCheckSafePoint(&loopHead, env, glue);
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
    BRANCH(HandlerBaseIsInlinedProperty(handler), &handlerIsInlinedProperty, &handlerNotInlinedProperty);
    Bind(&handlerIsInlinedProperty);
    {
        GateRef toOffset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
        SetValueWithRep(glue, receiver, toOffset, value, rep, &repChange);
        Jump(&exit);
    }
    Bind(&handlerNotInlinedProperty);
    {
        GateRef array = GetPropertiesArray(glue, receiver);
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
    Label isPrototype(env);
    Label notPrototype(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef newHClass;
    GateRef handlerInfo;
    if (withPrototype) {
        newHClass = GetTransWithProtoHClass(glue, handler);
        handlerInfo = GetInt64OfTInt(GetTransWithProtoHandlerInfo(glue, handler));
    } else {
        newHClass = GetTransitionHClass(glue, handler);
        handlerInfo = GetInt64OfTInt(GetTransitionHandlerInfo(glue, handler));
    }

    GateRef oldHClass = LoadHClass(glue, receiver);
    GateRef prototype = GetPrototypeFromHClass(glue, oldHClass);
    StorePrototype(glue, newHClass, prototype);
    BRANCH(IsPrototypeHClass(newHClass), &isPrototype, &notPrototype);
    Bind(&isPrototype);
    {
        SetProtoChangeDetailsToHClass(VariableType::INT64(), glue, newHClass, GetProtoChangeDetails(glue, oldHClass));
        Jump(&notPrototype);
    }
    Bind(&notPrototype);
    // Because we currently only supports Fast ElementsKind
    GateRef oldKind = GetElementsKindFromHClass(LoadHClass(glue, receiver));
    RestoreElementsKindToGeneric(glue, newHClass);
    TransitionHClass(glue, receiver, newHClass);
    TryMigrateToGenericKindForJSObject(glue, receiver, oldKind);
    BRANCH(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoNotInlinedProps);
    {
        ProfilerStubBuilder(env).UpdatePropAttrIC(glue, receiver, value, handlerInfo, callback);
        Label repChange(env);
        GateRef array = GetPropertiesArray(glue, receiver);
        GateRef capacity = GetLengthOfTaggedArray(array);
        GateRef index = HandlerBaseGetOffset(handlerInfo);
        BRANCH(Int32GreaterThanOrEqual(index, capacity), &indexMoreCapacity, &indexLessCapacity);
        Bind(&indexMoreCapacity);
        {
            NewObjectStubBuilder newBuilder(this);
            BRANCH(Int32Equal(capacity, Int32(0)), &capacityIsZero, &capacityNotZero);
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
            GateRef toIndex = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
            Label isCMCGC(env);
            Label notCMCGC(env);
            BRANCH_UNLIKELY(LoadPrimitive(
                VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
                &isCMCGC, &notCMCGC);
            Bind(&isCMCGC);
            {
                GateRef offset = PtrAdd(toIndex, IntPtr(TaggedArray::DATA_OFFSET));
                SetValueWithRep(glue, array, offset, value, rep, &repChange);
                Jump(&exit);
            }
            Bind(&notCMCGC);
            {
                GateRef base = PtrAdd(array, IntPtr(TaggedArray::DATA_OFFSET));
                SetValueWithRep(glue, base, toIndex, value, rep, &repChange);
                Jump(&exit);
            }
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
    Label cellNotInvalid(env);
    Label cellIsNotAccessorData(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    BRANCH(IsInvalidPropertyBox(glue, cell), &exit, &cellNotInvalid);
    Bind(&cellNotInvalid);
    {
        BRANCH(IsAccessorPropertyBox(glue, cell), &exit, &cellIsNotAccessorData);
        Bind(&cellIsNotAccessorData);
        {
            Store(VariableType::JS_ANY(), glue, cell, IntPtr(PropertyBox::VALUE_OFFSET), value);
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<typename DictionaryT>
GateRef StubBuilder::GetAttributesFromDictionary(GateRef glue, GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
    Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
             Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef attributesIndex =
        Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_DETAILS_INDEX));
    auto attrValue = GetValueFromTaggedArray(glue, elements, attributesIndex);
    return GetInt64OfTInt(attrValue);
}

template<typename DictionaryT>
GateRef StubBuilder::GetValueFromDictionary(GateRef glue, GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
        Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_VALUE_INDEX));
    return GetValueFromTaggedArray(glue, elements, valueIndex);
}

template<typename DictionaryT>
GateRef StubBuilder::GetKeyFromDictionary(GateRef glue, GateRef elements, GateRef entry)
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
        LoadPrimitive(VariableType::INT32(), elements, IntPtr(TaggedArray::LENGTH_OFFSET));
    GateRef arrayIndex =
        Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    BRANCH(Int32LessThan(arrayIndex, Int32(0)), &ltZero, &notLtZero);
    Bind(&ltZero);
    Jump(&exit);
    Bind(&notLtZero);
    BRANCH(Int32GreaterThan(arrayIndex, dictionaryLength), &gtLength, &notGtLength);
    Bind(&gtLength);
    Jump(&exit);
    Bind(&notGtLength);
    result = GetValueFromTaggedArray(glue, elements, arrayIndex);
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
    Store(VariableType::INT64(), glue, elements, dataOffset, Int64ToTaggedInt(attr));
}

template<typename DictionaryT>
inline void StubBuilder::UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value)
{
    GateRef arrayIndex = Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
        Int32Mul(index, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef valueIndex = Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_VALUE_INDEX));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, valueIndex, value);
}

GateRef StubBuilder::GetPropertyByIndex(GateRef glue, GateRef receiver,
                                        GateRef index, ProfileOperation callback, GateRef hir)
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
        GateRef hclass = LoadHClass(glue, *holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSpecialIndexed(env);
        Label notSpecialIndexed(env);
        BRANCH(IsSpecialIndexedObj(jsType), &isSpecialIndexed, &notSpecialIndexed);
        Bind(&isSpecialIndexed);
        {
            // TypeArray
            Label isFastTypeArray(env);
            Label notFastTypeArray(env);
            Label notTypedArrayProto(env);
            BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY))), &exit, &notTypedArrayProto);
            Bind(&notTypedArrayProto);
            BRANCH(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
            Bind(&isFastTypeArray);
            {
                BuiltinsTypedArrayStubBuilder typedArrayStubBuilder(this, GetCurrentGlobalEnv());
                result = typedArrayStubBuilder.FastGetPropertyByIndex(glue, *holder, index, jsType);
                Jump(&exit);
            }
            Bind(&notFastTypeArray);

            Label isSpecialContainer(env);
            Label notSpecialContainer(env);
            // Add SpecialContainer
            BRANCH(IsArrayListOrVector(jsType), &isSpecialContainer, &notSpecialContainer);
            Bind(&isSpecialContainer);
            {
                result = JSAPIContainerGet(glue, *holder, index);
                Jump(&exit);
            }
            Bind(&notSpecialContainer);

            Label isString(env);
            Label notString(env);
            Label getSubString(env);
            BRANCH(TaggedIsString(glue, *holder), &isString, &notString);
            Bind(&isString);
            {
                GateRef length = GetLengthFromString(*holder);
                BRANCH(Int32LessThan(index, length), &getSubString, &notString);
                Bind(&getSubString);
                Label flattenFastPath(env);
                BuiltinsStringStubBuilder stringBuilder(this, GetCurrentGlobalEnv());
                FlatStringStubBuilder thisFlat(this);
                thisFlat.FlattenString(glue, *holder, &flattenFastPath);
                Bind(&flattenFastPath);
                StringInfoGateRef stringInfoGate(&thisFlat);
                result = stringBuilder.FastSubString(glue, *holder, index, Int32(1), stringInfoGate);
                Jump(&exit);
            }
            Bind(&notString);
            {
                result = Hole();
                Jump(&exit);
            }
        }
        Bind(&notSpecialIndexed);
        {
            GateRef elements = GetElementsArray(glue, *holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            BRANCH(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                BRANCH(Int32UnsignedLessThan(index, GetLengthOfTaggedArray(elements)),
                       &lessThanLength, &notLessThanLength);
                Bind(&lessThanLength);
                {
                    DEFVARIABLE(value, VariableType::JS_ANY(), Hole());
                    Label notHole(env);
                    Label isHole(env);
                    value = GetTaggedValueWithElementsKind(glue, *holder, index);
                    BRANCH(TaggedIsNotHole(*value), &notHole, &isHole);
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
                BRANCH(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    GateRef attr = GetAttributesFromDictionary<NumberDictionary>(glue, elements, entryA);
                    GateRef value = GetValueFromDictionary<NumberDictionary>(glue, elements, entryA);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    BRANCH(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value, callback, hir);
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
                holder = GetPrototypeFromHClass(glue, LoadHClass(glue, *holder));
                BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
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

GateRef StubBuilder::GetPropertyByValue(
    GateRef glue, GateRef target, GateRef propKey, GateRef receiver, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(key, VariableType::JS_ANY(), propKey);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(isInternal, VariableType::BOOL(), True());
    Label isPropertyKey(env);
    Label exit(env);

    BRANCH(TaggedIsPropertyKey(glue, *key), &isPropertyKey, &exit);

    Bind(&isPropertyKey);
    {
        GateRef index64 = TryToElementsIndex(glue, *key);
        Label validIndex(env);
        Label notValidIndex(env);
        Label greaterThanInt32Max(env);
        Label notGreaterThanInt32Max(env);
        BRANCH(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
        Bind(&greaterThanInt32Max);
        {
            Jump(&exit);
        }
        Bind(&notGreaterThanInt32Max);
        GateRef index = TruncInt64ToInt32(index64);
        BRANCH(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = GetPropertyByIndex(glue, target, index, callback);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label notNumber1(env);
            Label getByName(env);
            BRANCH(TaggedIsNumber(*key), &exit, &notNumber1);
            Bind(&notNumber1);
            {
                Label isString(env);
                Label notString(env);
                Label isInternalString(env);
                Label notIntenalString(env);
                BRANCH(TaggedIsString(glue, *key), &isString, &notString);
                Bind(&isString);
                {
                    BRANCH(IsInternalString(*key), &isInternalString, &notIntenalString);
                    Bind(&isInternalString);
                    Jump(&getByName);
                    Bind(&notIntenalString);
                    {
                        Label notFind(env);
                        Label find(env);
                        // if key can't find in stringtabele, key is not propertyname for a object
                        GateRef res = CallRuntime(glue, RTSTUB_ID(TryGetInternString), { *key });
                        BRANCH(TaggedIsHole(res), &notFind, &find);
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
                result = GetPropertyByName(glue, target, *key, receiver, callback, *isInternal, true);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetPropertyByName(GateRef glue,
                                       GateRef target,
                                       GateRef propKey,
                                       GateRef receiver,
                                       ProfileOperation callback,
                                       GateRef isInternal,
                                       bool canUseIsInternal,
                                       GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), target);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Label findProperty(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(glue, *holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSIndexObj(env);
        Label notSIndexObj(env);
        BRANCH(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
        Bind(&isSIndexObj);
        {
            // TypeArray
            Label isFastTypeArray(env);
            Label notFastTypeArray(env);
            BRANCH(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
            Bind(&isFastTypeArray);
            {
                result = GetTypeArrayPropertyByName(glue, target, *holder, propKey, jsType);
                Label isNull(env);
                Label notNull(env);
                BRANCH(TaggedIsNull(*result), &isNull, &notNull);
                Bind(&isNull);
                {
                    result = Hole();
                    Jump(&exit);
                }
                Bind(&notNull);
                BRANCH(TaggedIsHole(*result), &notSIndexObj, &exit);
            }
            Bind(&notFastTypeArray);

            Label isString(env);
            Label notString(env);
            Label notJsPrimitiveRef(env);
            auto holderValue = *holder;
            BRANCH(LogicAndBuilder(env).And(TaggedIsString(glue, holderValue)).And(TaggedIsString(glue, propKey))
                .Done(), &isString, &notString);
            Bind(&isString);
            {
                Label getStringLength(env);
                Label getStringPrototype(env);
                GateRef lengthString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                              ConstantIndex::LENGTH_STRING_INDEX);
                BRANCH(FastStringEqual(glue, propKey, lengthString), &getStringLength, &getStringPrototype);
                Bind(&getStringLength);
                {
                    result = IntToTaggedPtr(GetLengthFromString(*holder));
                    Jump(&exit);
                }
                Bind(&getStringPrototype);
                {
                    GateRef globalEnv = GetCurrentGlobalEnv();
                    GateRef stringPrototype = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                                GlobalEnv::STRING_PROTOTYPE_INDEX);
                    holder = stringPrototype;
                    BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
                }
            }
            Bind(&notString);
            BRANCH(IsJSPrimitiveRef(glue, *holder), &notSIndexObj, &notJsPrimitiveRef);
            Bind(&notJsPrimitiveRef);  // not string prototype etc.
            {
#if ENABLE_NEXT_OPTIMIZATION
                Label isJsProxy(env);
                Label notJsProxy(env);
                BRANCH(IsJSProxy(jsType), &isJsProxy, &notJsProxy);
                Bind(&isJsProxy);
                {
                    result = CallCommonStub(glue, CommonStubCSigns::JSProxyGetProperty,
                                            {glue, *holder, propKey, target, GetCurrentGlobalEnv()}, hir);
                    Label isPendingException(env);
                    Label noPendingException(env);
                    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
                    Bind(&isPendingException);
                    {
                        result = Exception();
                        Jump(&exit);
                    }
                    Bind(&noPendingException);
                    Jump(&exit);
                }
                Bind(&notJsProxy);
                {
                    result = Hole();
                    Jump(&exit);
                }
#else
                result = Hole();
                Jump(&exit);
#endif
            }
        }
        Bind(&notSIndexObj);
        {
            if (canUseIsInternal) {
                BRANCH(isInternal, &findProperty, &loopExit);
            } else {
                Jump(&findProperty);
            }
            Label isDicMode(env);
            Label notDicMode(env);
            Bind(&findProperty);
            BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                GateRef layOutInfo = GetLayoutFromHClass(glue, hclass);
                GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
                // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                GateRef entryA = FindElementWithCache(glue, layOutInfo, hclass, propKey, propsNum, hir);
                Label hasEntry(env);
                Label noEntry(env);
                // if branch condition : entry != -1
                BRANCH(Int32NotEqual(entryA, Int32(-1)), &hasEntry, &noEntry);
                Bind(&hasEntry);
                {
                    // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                    GateRef attr = GetPropAttrFromLayoutInfo(glue, layOutInfo, entryA);
                    GateRef value = JSObjectGetPropertyWithRep(glue, *holder, hclass, attr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    BRANCH(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        // The getter may involve nested calls, so it is better to end (or return) early
                        EndTraceLoad(glue);
                        if (receiver == Circuit::NullGate()) {
                            result = CallGetterHelper(glue, target, *holder, value, callback, hir);
                        } else {
                            result = CallGetterHelper(glue, receiver, *holder, value, callback, hir);
                        }
                        StartTraceLoadGetter(glue);
                        Jump(&exit);
                    }
                    Bind(&notAccessor);
                    {
                        Label notHole(env);
                        BRANCH(TaggedIsHole(value), &noEntry, &notHole);
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
                GateRef array = GetPropertiesArray(glue, *holder);
                // int entry = dict->FindEntry(key)
                GateRef entryB = FindEntryFromHashTable<NameDictionary>(glue, array, propKey, hir);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                // if branch condition : entry != -1
                BRANCH(Int32NotEqual(entryB, Int32(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    // auto value = dict->GetValue(entry)
                    GateRef attr = GetAttributesFromDictionary<NameDictionary>(glue, array, entryB);
                    // auto attr = dict->GetAttributes(entry)
                    GateRef value = GetValueFromDictionary<NameDictionary>(glue, array, entryB);
                    Label isAccessor1(env);
                    Label notAccessor1(env);
                    BRANCH(IsAccessor(attr), &isAccessor1, &notAccessor1);
                    Bind(&isAccessor1);
                    {
                        // The getter may involve nested calls, so it is better to end (or return) early
                        EndTraceLoad(glue);
                        if (receiver == Circuit::NullGate()) {
                            result = CallGetterHelper(glue, target, *holder, value, callback, hir);
                        } else {
                            result = CallGetterHelper(glue, receiver, *holder, value, callback, hir);
                        }
                        StartTraceLoadGetter(glue);
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
                holder = GetPrototypeFromHClass(glue, LoadHClass(glue, *holder));
                BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
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

void StubBuilder::CallGetterIfAccessor(GateRef glue, GateRef holder, Variable *value, Variable *attr,
                                       Label *isFoundData, Label *isFoundAccessor)
{
    auto env = GetEnvironment();
    Label isAccessor(env);
    GateRef attrVal = attr->ReadVariable();
    GateRef valueVal = value->ReadVariable();
    BRANCH(IsAccessor(attrVal), &isAccessor, isFoundData);
    Bind(&isAccessor);
    {
        Label isAccessorInternal(env);
        BRANCH_UNLIKELY(IsAccessorInternal(glue, valueVal), &isAccessorInternal, isFoundAccessor);
        Bind(&isAccessorInternal);
        {
            value->WriteVariable(CallGetterHelper(glue, holder, holder, valueVal, ProfileOperation()));
            Jump(isFoundData);
        }
    }
}

void StubBuilder::TryGetOwnProperty(GateRef glue, GateRef holder, GateRef key, GateRef hir,
                                    Variable *rValue, Variable *rAttr,
                                    Label *isFoundData, Label *isFoundAccessor, Label *notFound, Label *callRuntime)
{
    auto env = GetEnvironment();
    Label exit(env);
    Label findProperty(env);
    Label found(env);
    GateRef hclass = LoadHClass(glue, holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSIndexObj(env);
    Label notSIndexObj(env);
    BRANCH(IsSpecialIndexedObj(jsType), callRuntime, &notSIndexObj);
    Bind(&notSIndexObj);
    {
        Label isDicMode(env);
        Label notDicMode(env);
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
        Bind(&notDicMode);
        {
            GateRef layOutInfo = GetLayoutFromHClass(glue, hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
            GateRef entryA = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum, hir);
            Label hasEntry(env);
            // if branch condition : entry != -1
            BRANCH(Int32NotEqual(entryA, Int32(-1)), &hasEntry, notFound);
            Bind(&hasEntry);
            {
                // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                GateRef attr = GetPropAttrFromLayoutInfo(glue, layOutInfo, entryA);
                GateRef value = JSObjectGetPropertyWithRep(glue, holder, hclass, attr);
                Label notHole(env);
                BRANCH(TaggedIsHole(value), notFound, &notHole);
                Bind(&notHole);
                {
                    rValue->WriteVariable(value);
                    rAttr->WriteVariable(attr);
                    Jump(&found);
                }
            }
        }
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(glue, holder);
            // int entry = dict->FindEntry(key)
            GateRef entryB = FindEntryFromHashTable<NameDictionary>(glue, array, key, hir);
            Label notNegtiveOne(env);
            // if branch condition : entry != -1
            BRANCH(Int32NotEqual(entryB, Int32(-1)), &notNegtiveOne, notFound);
            Bind(&notNegtiveOne);
            {
                // auto value = dict->GetValue(entry)
                rAttr->WriteVariable(GetAttributesFromDictionary<NameDictionary>(glue, array, entryB));
                // auto attr = dict->GetAttributes(entry)
                rValue->WriteVariable(GetValueFromDictionary<NameDictionary>(glue, array, entryB));
                Jump(&found);
            }
        }
        Bind(&found);
        {
            CallGetterIfAccessor(glue, holder, rValue, rAttr, isFoundData, isFoundAccessor);
        }
    }
}

void StubBuilder::CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef srcHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label isAOT(env);
    Label isNotAOT(env);
    env->SubCfgEntry(&entry);
    auto proto = GetPrototypeFromHClass(glue, srcHClass);
    SetPrototypeToHClass(VariableType::JS_POINTER(), glue, dstHClass, proto);
    GateRef bitField = GetBitFieldFromHClass(srcHClass);
    GateRef afterSetStableBitField = SetIsStableToBitField(bitField, Int32(1));
    SetBitFieldToHClass(glue, dstHClass, afterSetStableBitField);
    SetIsAllTaggedProp(glue, dstHClass, GetIsAllTaggedPropFromHClass(srcHClass));
    SetNumberOfPropsToHClass(glue, dstHClass, GetNumberOfPropsFromHClass(srcHClass));
    SetTransitionsToHClass(VariableType::INT64(), glue, dstHClass, Undefined());
    SetParentToHClass(VariableType::INT64(), glue, dstHClass, Undefined());
    SetProtoChangeDetailsToHClass(VariableType::INT64(), glue, dstHClass, Null());
    SetEnumCacheToHClass(VariableType::INT64(), glue, dstHClass, Null());
    SetDependentInfosToHClass(VariableType::INT64(), glue, dstHClass, Undefined());
    SetLayoutToHClass(VariableType::JS_POINTER(),
                      glue,
                      dstHClass,
                      GetLayoutFromHClass(glue, srcHClass),
                      MemoryAttribute::NeedBarrierAndAtomic());
    BRANCH(IsAOTHClass(srcHClass), &isAOT, &isNotAOT);
    Bind(&isAOT);
    {
        SetIsAOT(glue, dstHClass, False());
        Jump(&isNotAOT);
    }
    Bind(&isNotAOT);
    env->SubCfgExit();
    return;
}

void StubBuilder::TransitionForRepChange(GateRef glue, GateRef receiver, GateRef key, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    GateRef hclass = LoadHClass(glue, receiver);
    GateRef type = GetObjectType(hclass);
    GateRef size = Int32Mul(GetInlinedPropsStartFromHClass(hclass),
                            Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
    GateRef newJshclass = CallRuntime(glue, RTSTUB_ID(NewEcmaHClass),
        { IntToTaggedInt(size), IntToTaggedInt(type),
          IntToTaggedInt(inlineProps) });
    CopyAllHClass(glue, newJshclass, hclass);
    CallRuntime(glue, RTSTUB_ID(CopyAndUpdateObjLayout),
                { hclass, newJshclass, key, Int64ToTaggedInt(attr) });
#if ECMASCRIPT_ENABLE_IC
    NotifyHClassChanged(glue, hclass, newJshclass);
#endif
    // Because we currently only supports Fast ElementsKind
    GateRef oldKind = GetElementsKindFromHClass(LoadHClass(glue, receiver));
    RestoreElementsKindToGeneric(glue, newJshclass);
    TransitionHClass(glue, receiver, newJshclass);
    TryMigrateToGenericKindForJSObject(glue, receiver, oldKind);
    env->SubCfgExit();
}

void StubBuilder::TransitToElementsKind(GateRef glue, GateRef receiver, GateRef value, GateRef kind)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    GateRef hclass = LoadHClass(glue, receiver);
    GateRef elementsKind = GetElementsKindFromHClass(hclass);

    Label isNoneDefault(env);
    BRANCH(Int32Equal(elementsKind, Int32(Elements::ToUint(ElementsKind::GENERIC))), &exit, &isNoneDefault);
    Bind(&isNoneDefault);
    {
        GateRef newKind = TaggedToElementKind(glue, value);
        newKind = Int32Or(newKind, kind);
        newKind = Int32Or(newKind, elementsKind);
        Label change(env);
        BRANCH(Int32Equal(elementsKind, newKind), &exit, &change);
        Bind(&change);
        {
            CallRuntime(glue, RTSTUB_ID(UpdateHClassForElementsKind), { receiver, newKind });
            MigrateArrayWithKind(glue, receiver, elementsKind, newKind);
            Jump(&exit);
        }
    }

    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::TryMigrateToGenericKindForJSObject(GateRef glue, GateRef receiver, GateRef oldKind)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    Label isJSArray(env);
    BRANCH(TaggedIsJSArray(glue, receiver), &isJSArray, &exit);
    Bind(&isJSArray);
    {
        Label elementsIsMutantTaggedArray(env);
        GateRef elements = GetElementsArray(glue, receiver);
        BRANCH(IsMutantTaggedArray(glue, elements), &elementsIsMutantTaggedArray, &exit);
        Bind(&elementsIsMutantTaggedArray);
        {
            MigrateArrayWithKind(glue, receiver, oldKind, Int32(Elements::ToUint(ElementsKind::GENERIC)));
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
    DEFVARIABLE(kind, VariableType::INT32(), Int32(Elements::ToUint(ElementsKind::NONE)));
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isArray(env);
    Label notArray(env);
    BRANCH(IsJsArray(glue, receiver), &isArray, &notArray);
    Bind(&isArray);
    {
        GateRef oldLen = GetArrayLength(receiver);
        Label indexGreaterOrEq(env);
        BRANCH(Int32GreaterThanOrEqual(index, oldLen), &indexGreaterOrEq, &notArray);
        Bind(&indexGreaterOrEq);
        {
            Label isArrLenWritable(env);
            Label notArrLenWritable(env);
            BRANCH(IsArrayLengthWritable(glue, receiver), &isArrLenWritable, &notArrLenWritable);
            Bind(&isArrLenWritable);
            {
                SetArrayLength(glue, receiver, Int32Add(index, Int32(1)));
                Label indexGreater(env);
                BRANCH(Int32GreaterThan(index, oldLen), &indexGreater, &notArray);
                Bind(&indexGreater);
                kind = Int32(Elements::ToUint(ElementsKind::HOLE));
                Jump(&notArray);
            }
            Bind(&notArrLenWritable);
            result = False();
            Jump(&exit);
        }
    }
    Bind(&notArray);
    {
        NotifyArrayPrototypeChangedGuardians(glue, receiver);
        GateRef hclass = LoadHClass(glue, receiver);
        GateRef elements = GetElementsArray(glue, receiver);
        Label isDicMode(env);
        Label notDicMode(env);
        BRANCH(IsDictionaryElement(hclass), &isDicMode, &notDicMode);
        Bind(&isDicMode);
        {
            GateRef res = CallRuntime(glue, RTSTUB_ID(NumberDictionaryPut),
                { receiver, elements, IntToTaggedInt(index), value, Int64ToTaggedInt(attr), TaggedFalse() });
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
            BRANCH(BitOr(notDefault, Int32GreaterThanOrEqual(index, capacity)), &indexGreaterLen, &notGreaterLen);
            Bind(&indexGreaterLen);
            {
                Label isTransToDict(env);
                Label notTransToDict(env);
                BRANCH(LogicOrBuilder(env).Or(notDefault).Or(ShouldTransToDict(capacity, index)).Done(),
                    &isTransToDict, &notTransToDict);
                Bind(&isTransToDict);
                {
                    GateRef res = CallRuntime(glue, RTSTUB_ID(NumberDictionaryPut),
                        { receiver, elements, IntToTaggedInt(index), value, Int64ToTaggedInt(attr), TaggedTrue() });
                    Label isPendingException(env);
                    Label noPendingException(env);
                    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
                    Bind(&isPendingException);
                    {
                        result = False();
                        Jump(&exit);
                    }
                    Bind(&noPendingException);
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
    GateRef elements = GetElementsArray(glue, receiver);
    newElements = newBuilder.ExtendArrayWithOptimizationCheck(glue, elements, newCapacity);
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
    BRANCH(Int32GreaterThanOrEqual(index, capacity), &isGreaterThanCapcity, &notGreaterThanCapcity);
    Bind(&isGreaterThanCapcity);
    {
        Label isLessThanMax(env);
        Label notLessThanMax(env);
        BRANCH(Int32LessThanOrEqual(Int32Sub(index, capacity),
            Int32(JSObject::MAX_GAP)), &isLessThanMax, &notLessThanMax);
        Bind(&isLessThanMax);
        {
            Label isLessThanInt32Max(env);
            Label notLessThanInt32Max(env);
            BRANCH(Int32LessThan(index, Int32(INT32_MAX)), &isLessThanInt32Max, &notLessThanInt32Max);
            Bind(&isLessThanInt32Max);
            {
                Label isLessThanMin(env);
                Label notLessThanMin(env);
                BRANCH(Int32LessThan(capacity, Int32(JSObject::MIN_GAP)), &isLessThanMin, &notLessThanMin);
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

void StubBuilder::NotifyArrayPrototypeChangedGuardians(GateRef glue, GateRef receiver)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef guardians = GetArrayElementsGuardians(globalEnv);

    Label isGuardians(env);
    BRANCH(Equal(guardians, True()), &isGuardians, &exit);
    Bind(&isGuardians);
    {
        GateRef hclass = LoadHClass(glue, receiver);
        Label isPrototype(env);
        BRANCH(BitOr(IsPrototypeHClass(hclass), IsJsArray(glue, receiver)), &isPrototype, &exit);
        Bind(&isPrototype);
        {
            Label isEnvPrototype(env);
            GateRef isEnvPrototypeCheck = LogicOrBuilder(env)
                .Or(Equal(GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                            GlobalEnv::OBJECT_FUNCTION_PROTOTYPE_INDEX), receiver))
                .Or(Equal(GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                            GlobalEnv::ARRAY_PROTOTYPE_INDEX), receiver))
                .Done();
            BRANCH(isEnvPrototypeCheck, &isEnvPrototype, &exit);
            Bind(&isEnvPrototype);
            CallRuntime(glue, RTSTUB_ID(NotifyArrayPrototypeChanged), {});
            SetArrayElementsGuardians(glue, globalEnv, False());
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
    GateRef hclass = LoadHClass(glue, receiver);
    Label isDicMode(env);
    Label notDicMode(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH_UNLIKELY(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
    Bind(&isDicMode);
    {
        GateRef array = GetPropertiesArray(glue, receiver);
        GateRef lengthString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::LENGTH_STRING_INDEX);
        GateRef entry = CallCommonStub(glue, CommonStubCSigns::FindEntryFromNameDictionary,
                                       {glue, array, lengthString});
        Label notNegtiveOne(env);
        Label isNegtiveOne(env);
        BRANCH(Int32NotEqual(entry, Int32(-1)), &notNegtiveOne, &isNegtiveOne);
        Bind(&notNegtiveOne);
        {
            GateRef attr = GetAttributesFromDictionary<NameDictionary>(glue, array, entry);
            result = IsWritable(attr);
            Jump(&exit);
        }
        Bind(&isNegtiveOne);
        {
            GateRef attr1 = Int64(PropertyAttributes::GetDefaultAttributes());
            result = IsWritable(attr1);
            Jump(&exit);
        }
    }
    Bind(&notDicMode);
    {
        result = IsArrayLengthWritableForNonDictMode(glue, receiver);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsArrayLengthWritableForNonDictMode(GateRef glue, GateRef receiver)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    GateRef hclass = LoadHClass(glue, receiver);
    GateRef layoutInfo = GetLayoutFromHClass(glue, hclass);
    GateRef attr = GetPropAttrFromLayoutInfo(glue, layoutInfo, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    result = IsWritable(attr);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CheckHClassForRep(GateRef glue, GateRef hclass, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label isAOTHClass(env);
    DEFVARIABLE(result, VariableType::BOOL(), Boolean(true));
    Branch(IsAOTHClass(hclass), &isAOTHClass, &exit);
    Bind(&isAOTHClass);
    {
        GateRef propNums = GetNumberOfPropsFromHClass(hclass);
        GateRef last = Int32Sub(propNums, Int32(1));
        GateRef layoutInfo = GetLayoutFromHClass(glue, hclass);
        GateRef cachedAttr = GetPropAttrFromLayoutInfo(glue, layoutInfo, last);
        GateRef lastRep = GetRepInPropAttr(cachedAttr);
        Label repIsInt(env);
        Label repIsNotInt(env);
        Label repIsDouble(env);
        Branch(Equal(lastRep, Int32(static_cast<int32_t>(Representation::INT))), &repIsInt, &repIsNotInt);
        Bind(&repIsInt);
        {
            GateRef valueRep = TranslateToRep(value);
            Label valueRepIsNotInt(env);
            Branch(Equal(valueRep, Int32(static_cast<int32_t>(Representation::INT))), &exit, &valueRepIsNotInt);
            Bind(&valueRepIsNotInt);
            {
                result = Boolean(false);
                Jump(&exit);
            }
        }
        Bind(&repIsNotInt);
        {
            Branch(Equal(lastRep, Int32(static_cast<int32_t>(Representation::DOUBLE))), &repIsDouble, &exit);
            Bind(&repIsDouble);
            GateRef valueRep = TranslateToRep(value);
            Label valueRepIsNotInt(env);
            Branch(Equal(valueRep, Int32(static_cast<int32_t>(Representation::INT))), &exit, &valueRepIsNotInt);
            Bind(&valueRepIsNotInt);
            {
                Label valueRepIsNotDouble(env);
                Branch(Equal(valueRep, Int32(static_cast<int32_t>(Representation::DOUBLE))), &exit,
                    &valueRepIsNotDouble);
                Bind(&valueRepIsNotDouble);
                {
                    result = Boolean(false);
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

GateRef StubBuilder::FindTransitions(GateRef glue, GateRef hclass, GateRef key, GateRef metaData, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    GateRef transition = GetTransitionsFromHClass(glue, hclass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label notUndefined(env);
    BRANCH(Equal(transition, Undefined()), &exit, &notUndefined);
    Bind(&notUndefined);
    {
        Label isWeak(env);
        Label notWeak(env);
        BRANCH(TaggedIsWeak(transition), &isWeak, &notWeak);
        Bind(&isWeak);
        {
            GateRef transitionHClass = LoadObjectFromWeakRef(transition);
            GateRef propNums = GetNumberOfPropsFromHClass(transitionHClass);
            GateRef last = Int32Sub(propNums, Int32(1));
            GateRef layoutInfo = GetLayoutFromHClass(glue, transitionHClass);
            GateRef cachedKey = GetKeyFromLayoutInfo(glue, layoutInfo, last);
            GateRef cachedAttr = GetPropAttrFromLayoutInfo(glue, layoutInfo, last);
            GateRef cachedMetaData = GetPropertyMetaDataFromAttr(cachedAttr);
            Label keyMatch(env);
            Label isMatch(env);
            Label repMatch(env);
            BRANCH(Equal(cachedKey, key), &keyMatch, &exit);
            Bind(&keyMatch);
            {
                BRANCH(Int32Equal(metaData, cachedMetaData), &isMatch, &exit);
                Bind(&isMatch);
                {
                    BRANCH(CheckHClassForRep(glue, transitionHClass, value), &repMatch, &exit);
                    Bind(&repMatch);
                    result = transitionHClass;
                    Jump(&exit);
                }
            }
        }
        Bind(&notWeak);
        {
            // need to find from dictionary
            GateRef entryA = FindEntryFromTransitionDictionary(glue, transition, key, metaData);
            Label isFound(env);
            BRANCH(Int32NotEqual(entryA, Int32(-1)), &isFound, &exit);
            Bind(&isFound);
            auto cachedValue = GetValueFromDictionary<TransitionsDictionary>(glue, transition, entryA);
            Label valueNotUndefined(env);
            Label repMatch(env);
            BRANCH(Int64NotEqual(cachedValue, Undefined()), &valueNotUndefined, &exit);
            Bind(&valueNotUndefined);
            {
                GateRef newHClass = LoadObjectFromWeakRef(cachedValue);
                BRANCH(CheckHClassForRep(glue, newHClass, value), &repMatch, &exit);
                Bind(&repMatch);
                result = newHClass;
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
                                        ProfileOperation callback, bool defineSemantics, bool mayThrow)
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
    if (!useOwn && !defineSemantics) {
        Jump(&loopHead);
        LoopBegin(&loopHead);
    }
    GateRef hclass = LoadHClass(glue, *holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSpecialIndex(env);
    Label notSpecialIndex(env);
    BRANCH(IsSpecialIndexedObj(jsType), &isSpecialIndex, &notSpecialIndex);
    Bind(&isSpecialIndex);
    {
        // TypeArray
        Label isFastTypeArray(env);
        Label notFastTypeArray(env);
        Label checkIsOnPrototypeChain(env);
        Label notTypedArrayProto(env);
        BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY))), &exit, &notTypedArrayProto);
        Bind(&notTypedArrayProto);
        BRANCH(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
        Bind(&isFastTypeArray);
        {
            BRANCH(Equal(*holder, receiver), &checkIsOnPrototypeChain, &exit);
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
        GateRef elements = GetElementsArray(glue, *holder);
        Label isDictionaryElement(env);
        Label notDictionaryElement(env);
        BRANCH(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
        Bind(&notDictionaryElement);
        {
            Label isReceiver(env);
            if (useOwn) {
                BRANCH(Equal(*holder, receiver), &isReceiver, &ifEnd);
            } else {
                BRANCH(Equal(*holder, receiver), &isReceiver, &afterLoop);
            }
            Bind(&isReceiver);
            {
                GateRef length = GetLengthOfTaggedArray(elements);
                Label inRange(env);
                if (useOwn) {
                    BRANCH(Int64LessThan(index, length), &inRange, &ifEnd);
                } else {
                    BRANCH(Int64LessThan(index, length), &inRange, &loopExit);
                }
                Bind(&inRange);
                {
                    GateRef value1 = GetTaggedValueWithElementsKind(glue, *holder, index);
                    Label notHole(env);
                    if (useOwn) {
                        BRANCH(Int64NotEqual(value1, Hole()), &notHole, &ifEnd);
                    } else {
                        BRANCH(Int64NotEqual(value1, Hole()), &notHole, &loopExit);
                    }
                    Bind(&notHole);
                    {
                        BRANCH(IsJsCOWArray(glue, *holder), &isJsCOWArray, &isNotJsCOWArray);
                        Bind(&isJsCOWArray);
                        {
                            CallRuntime(glue, RTSTUB_ID(CheckAndCopyArray), {*holder});
                            SetValueWithElementsKind(glue, *holder, value, index, Boolean(true),
                                                     Int32(Elements::ToUint(ElementsKind::NONE)));
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
                                                     Int32(Elements::ToUint(ElementsKind::NONE)));
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
            BRANCH(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, &negtiveOne);
            Bind(&notNegtiveOne);
            {
                GateRef attr = GetAttributesFromDictionary<NumberDictionary>(glue, elements, entryA);
                Label isWritandConfig(env);
                Label notWritandConfig(env);
                BRANCH(BitOr(LogicAndBuilder(env).And(IsWritable(attr)).And(IsConfigable(attr)).Done(),
                    IsJSShared(glue, *holder)), &isWritandConfig, &notWritandConfig);
                Bind(&isWritandConfig);
                {
                    Label isAccessor(env);
                    Label notAccessor(env);
                    BRANCH(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    if (defineSemantics) {
                        Jump(&exit);
                    } else {
                        GateRef accessor = GetValueFromDictionary<NumberDictionary>(glue, elements, entryA);
                        Label shouldCall(env);
                        BRANCH(ShouldCallSetter(glue, receiver, *holder, accessor, attr), &shouldCall, &notAccessor);
                        Bind(&shouldCall);
                        {
                            returnValue = CallSetterHelper(glue, receiver, accessor, value, callback, mayThrow);
                            Jump(&exit);
                        }
                    }
                    Bind(&notAccessor);
                    {
                        Label holdEqualsRecv(env);
                        if (useOwn) {
                            BRANCH(Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
                        } else {
                            BRANCH(Equal(*holder, receiver), &holdEqualsRecv, &afterLoop);
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
            holder = GetPrototypeFromHClass(glue, LoadHClass(glue, *holder));
            BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
        Bind(&afterLoop);
    }
    Label isExtensible(env);
    Label notExtensible(env);
    Label throwNotExtensible(env);
    BRANCH(IsExtensible(glue, receiver), &isExtensible, &notExtensible);
    Bind(&notExtensible);
    {
        // fixme(hzzhouzebin) this makes SharedArray's frozen no sense.
        BRANCH(IsJsSArray(glue, receiver), &isExtensible, &throwNotExtensible);
    }
    Bind(&isExtensible);
    {
        Label success(env);
        Label failed(env);
        BRANCH(AddElementInternal(glue, receiver, index, value, Int64(PropertyAttributes::GetDefaultAttributes())),
               &success, &failed);
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
    Bind(&throwNotExtensible);
    {
        if (mayThrow) {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensibleByIndex));
            CallRuntime(glue, RTSTUB_ID(ThrowTypeErrorWithParam), {IntToTaggedInt(taggedId), IntToTaggedInt(index)});
            returnValue = Exception();
        } else {
            returnValue = TaggedFalse();
        }
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *returnValue;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::DefinePropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value)
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
    GateRef hclass = LoadHClass(glue, *holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSpecialIndex(env);
    Label notSpecialIndex(env);
    BRANCH(IsSpecialIndexedObj(jsType), &isSpecialIndex, &notSpecialIndex);
    Bind(&isSpecialIndex);
    {
        Label isFastTypeArray(env);
        Label notFastTypeArray(env);
        Label checkIsOnPrototypeChain(env);
        Label notTypedArrayProto(env);
        BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY))), &exit, &notTypedArrayProto);
        Bind(&notTypedArrayProto);
        BRANCH(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
        Bind(&isFastTypeArray);
        {
            BRANCH(Equal(*holder, receiver), &checkIsOnPrototypeChain, &exit);
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
        GateRef elements = GetElementsArray(glue, *holder);
        Label isDictionaryElement(env);
        Label notDictionaryElement(env);
        BRANCH(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
        Bind(&notDictionaryElement);
        {
            Label isReceiver(env);
            BRANCH(Equal(*holder, receiver), &isReceiver, &ifEnd);
            Bind(&isReceiver);
            {
                GateRef length = GetLengthOfTaggedArray(elements);
                Label inRange(env);
                BRANCH(Int64LessThan(index, length), &inRange, &ifEnd);
                Bind(&inRange);
                {
                    GateRef value1 = GetTaggedValueWithElementsKind(glue, *holder, index);
                    Label notHole(env);
                    BRANCH(Int64NotEqual(value1, Hole()), &notHole, &ifEnd);
                    Bind(&notHole);
                    {
                        BRANCH(IsJsCOWArray(glue, *holder), &isJsCOWArray, &isNotJsCOWArray);
                        Bind(&isJsCOWArray);
                        {
                            CallRuntime(glue, RTSTUB_ID(CheckAndCopyArray), {*holder});
                            SetValueWithElementsKind(glue, *holder, value, index, Boolean(true),
                                                     Int32(Elements::ToUint(ElementsKind::NONE)));
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
                                                     Int32(Elements::ToUint(ElementsKind::NONE)));
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
            BRANCH(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, &negtiveOne);
            Bind(&notNegtiveOne);
            {
                GateRef attr = GetAttributesFromDictionary<NumberDictionary>(glue, elements, entryA);
                Label isWritandConfig(env);
                Label notWritandConfig(env);
                BRANCH(BitOr(LogicAndBuilder(env).And(IsWritable(attr)).And(IsConfigable(attr)).Done(),
                    IsJSShared(glue, *holder)), &isWritandConfig, &notWritandConfig);
                Bind(&isWritandConfig);
                {
                    Label notAccessor(env);
                    BRANCH(IsAccessor(attr), &exit, &notAccessor);
                    Bind(&notAccessor);
                    {
                        Label holdEqualsRecv(env);
                        BRANCH(Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
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
    Bind(&ifEnd);
    Label isExtensible(env);
    Label notExtensible(env);
    Label throwNotExtensible(env);
    BRANCH(IsExtensible(glue, receiver), &isExtensible, &notExtensible);
    Bind(&notExtensible);
    {
        BRANCH(IsJsSArray(glue, receiver), &isExtensible, &throwNotExtensible);
    }
    Bind(&isExtensible);
    {
        Label success(env);
        Label failed(env);
        BRANCH(AddElementInternal(glue, receiver, index, value, Int64(PropertyAttributes::GetDefaultAttributes())),
               &success, &failed);
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
    Bind(&throwNotExtensible);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensibleByIndex));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeErrorWithParam), {IntToTaggedInt(taggedId), IntToTaggedInt(index)});
        returnValue = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *returnValue;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ThrowTypeErrorInextensiableAddProperty(GateRef glue, GateRef key)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label isString(env);
    Label notString(env);
    BRANCH(TaggedIsString(glue, key), &isString, &notString);
    Bind(&isString);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensibleByName));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeErrorWithParam), {IntToTaggedInt(taggedId), key});
        Jump(&exit);
    }
    Bind(&notString);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = Exception();
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SetPropertyByName(GateRef glue,
                                       GateRef receiver,
                                       GateRef key,
                                       GateRef value,
                                       bool useOwn,
                                       GateRef isInternal,
                                       ProfileOperation callback,
                                       bool canUseIsInternal,
                                       bool defineSemantics,
                                       bool mayThrow)
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
        Jump(&loopHead);
        LoopBegin(&loopHead);
    }
    GateRef hclass = LoadHClass(glue, *holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSIndexObj(env);
    Label notSIndexObj(env);
    BRANCH(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
    Bind(&isSIndexObj);
    {
        Label isFastTypeArray(env);
        Label notFastTypeArray(env);
        BRANCH(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
        Bind(&isFastTypeArray);
        {
            result = SetTypeArrayPropertyByName(glue, receiver, *holder, key, value, jsType);
            Label isNull(env);
            Label notNull(env);
            BRANCH(TaggedIsNull(*result), &isNull, &notNull);
            Bind(&isNull);
            {
                result = Hole();
                Jump(&exit);
            }
            Bind(&notNull);
            BRANCH(TaggedIsHole(*result), &notSIndexObj, &exit);
        }
        Bind(&notFastTypeArray);

        Label isSpecialContainer(env);
        Label notSpecialContainer(env);
        BRANCH(IsArrayListOrVector(jsType), &isSpecialContainer, &notSpecialContainer);
        Bind(&isSpecialContainer);
        {
            if (mayThrow) {
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotSetPropertyOnContainer));
                CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
                result = Exception();
            } else {
                result = TaggedFalse();
            }
            Jump(&exit);
        }
        Bind(&notSpecialContainer);
        {
#if ENABLE_NEXT_OPTIMIZATION
            Label isJsProxy(env);
            Label notJsProxy(env);
            BRANCH(IsJSProxy(jsType), &isJsProxy, &notJsProxy);
            Bind(&isJsProxy);
            {
                if (defineSemantics) {
                    Jump(&exit);
                } else {
                    Label returnException(env);
                    Label noPendingException(env);
                    if (mayThrow) {
                        result = CallCommonStub(glue, CommonStubCSigns::JSProxySetProperty,
                                                {glue, *holder, key, value, receiver, GetCurrentGlobalEnv()});
                    } else {
                        result = CallCommonStub(glue, CommonStubCSigns::JSProxySetPropertyNoThrow,
                                                {glue, *holder, key, value, receiver, GetCurrentGlobalEnv()});
                    }
                    BRANCH(HasPendingException(glue), &returnException, &exit);
                    Bind(&returnException);
                    {
                        result = Exception();
                        Jump(&exit);
                    }
                }
            }
            Bind(&notJsProxy);
            {
                result = Hole();
                Jump(&exit);
            }
#else
            result = Hole();
            Jump(&exit);
#endif
        }
    }
    Bind(&notSIndexObj);
    {
        if (canUseIsInternal) {
            if (useOwn) {
                BRANCH(isInternal, &findProperty, &ifEnd);
            } else {
                BRANCH(isInternal, &findProperty, &loopExit);
            }
        } else {
            Jump(&findProperty);
        }
        Label isDicMode(env);
        Label notDicMode(env);
        Bind(&findProperty);
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
        Bind(&notDicMode);
        {
            GateRef layOutInfo = GetLayoutFromHClass(glue, hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum);
            Label hasEntry(env);
            if (useOwn || defineSemantics) {
                BRANCH(Int32NotEqual(entry, Int32(-1)), &hasEntry, &ifEnd);
            } else {
                BRANCH(Int32NotEqual(entry, Int32(-1)), &hasEntry, &loopExit);
            }
            Bind(&hasEntry);
            {
                GateRef attr = GetPropAttrFromLayoutInfo(glue, layOutInfo, entry);
                Label isAccessor(env);
                Label notAccessor(env);
                BRANCH(IsAccessor(attr), &isAccessor, &notAccessor);
                Bind(&isAccessor);
                if (defineSemantics) {
                    Jump(&exit);
                } else {
                    GateRef accessor = JSObjectGetPropertyWithRep(glue, *holder, hclass, attr);
                    Label shouldCall(env);
                    BRANCH(ShouldCallSetter(glue, receiver, *holder, accessor, attr), &shouldCall, &notAccessor);
                    Bind(&shouldCall);
                    {
                        result = CallSetterHelper(glue, receiver, accessor, value, callback, mayThrow);
                        Jump(&exit);
                    }
                }
                Bind(&notAccessor);
                {
                    Label writable(env);
                    Label notWritable(env);
                    BRANCH(IsWritable(attr), &writable, &notWritable);
                    Bind(&notWritable);
                    if (defineSemantics) {
                        Jump(&exit);
                    } else {
                        if (mayThrow) {
                            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                            CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
                            result = Exception();
                        } else {
                            result = TaggedFalse();
                        }
                        Jump(&exit);
                    }
                    Bind(&writable);
                    {
                        Label isAOT(env);
                        Label notAOT(env);
                        BRANCH(IsAOTHClass(hclass), &isAOT, &notAOT);
                        Bind(&isAOT);
                        {
                            GateRef attrVal = JSObjectGetPropertyWithRep(glue, *holder, hclass, attr);
                            Label attrValIsHole(env);
                            BRANCH(TaggedIsHole(attrVal), &attrValIsHole, &notAOT);
                            Bind(&attrValIsHole);
                            {
                                Label storeReceiverHoleEntry(env);
                                Label noNeedStore(env);
                                GateRef checkReceiverHoleEntry = Int32Equal(*receiverHoleEntry, Int32(-1));
                                GateRef checkHolderEqualsRecv = Equal(*holder, receiver);
                                BRANCH(BitAnd(checkReceiverHoleEntry, checkHolderEqualsRecv),
                                    &storeReceiverHoleEntry, &noNeedStore);
                                Bind(&storeReceiverHoleEntry);
                                {
                                    receiverHoleEntry = entry;
                                    Jump(&noNeedStore);
                                }
                                Bind(&noNeedStore);
                                if (useOwn || defineSemantics) {
                                    Jump(&ifEnd);
                                } else {
                                    Jump(&loopExit);
                                }
                            }
                        }
                        Bind(&notAOT);
                        Label holdEqualsRecv(env);
                        if (useOwn || defineSemantics) {
                            BRANCH(Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
                        } else {
                            BRANCH(Equal(*holder, receiver), &holdEqualsRecv, &afterLoop);
                        }
                        Bind(&holdEqualsRecv);
                        {
                            Label isJSShared(env);
                            Label executeSetProp(env);
                            BRANCH(IsJSShared(glue, receiver), &isJSShared, &executeSetProp);
                            Bind(&isJSShared);
                            {
                                DEFVARIABLE(actualValue, VariableType::JS_ANY(), value);
                                Label executeSharedSetProp(env);
                                SharedObjectStoreBarrierWithTypeCheck(false, &result, glue, attr, value, &actualValue,
                                    &executeSharedSetProp, &exit);
                                Bind(&executeSharedSetProp);
                                {
                                    JSObjectSetProperty(glue, *holder, hclass, attr, key, *actualValue);
                                    ProfilerStubBuilder(env).UpdatePropAttrWithValue(glue, receiver, attr,
                                        *actualValue, callback);
                                    result = Undefined();
                                    Jump(&exit);
                                }
                            }
                            Bind(&executeSetProp);
                            {
                                JSObjectSetProperty(glue, *holder, hclass, attr, key, value);
                                ProfilerStubBuilder(env).UpdatePropAttrWithValue(glue, receiver, attr, value,
                                    callback);
                                result = Undefined();
                                Jump(&exit);
                            }
                        }
                    }
                }
            }
        }
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(glue, *holder);
            GateRef entry1 = FindEntryFromHashTable<NameDictionary>(glue, array, key);
            Label notNegtiveOne(env);
            if (useOwn || defineSemantics) {
                BRANCH(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &ifEnd);
            } else {
                BRANCH(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &loopExit);
            }
            Bind(&notNegtiveOne);
            {
                GateRef attr1 = GetAttributesFromDictionary<NameDictionary>(glue, array, entry1);
                Label isAccessor1(env);
                Label notAccessor1(env);
                BRANCH(IsAccessor(attr1), &isAccessor1, &notAccessor1);
                Bind(&isAccessor1);
                if (defineSemantics) {
                    Jump(&exit);
                } else {
                    GateRef accessor1 = GetValueFromDictionary<NameDictionary>(glue, array, entry1);
                    Label shouldCall1(env);
                    BRANCH(ShouldCallSetter(glue, receiver, *holder, accessor1, attr1), &shouldCall1, &notAccessor1);
                    Bind(&shouldCall1);
                    {
                        result = CallSetterHelper(glue, receiver, accessor1, value, callback, mayThrow);
                        Jump(&exit);
                    }
                }
                Bind(&notAccessor1);
                {
                    Label writable1(env);
                    Label notWritable1(env);
                    BRANCH(IsWritable(attr1), &writable1, &notWritable1);
                    Bind(&notWritable1);
                    if (defineSemantics) {
                        Jump(&exit);
                    } else {
                        if (mayThrow) {
                            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                            CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
                            result = Exception();
                        } else {
                            result = TaggedFalse();
                        }
                        Jump(&exit);
                    }
                    Bind(&writable1);
                    {
                        Label holdEqualsRecv1(env);
                        if (useOwn) {
                            BRANCH(Equal(*holder, receiver), &holdEqualsRecv1, &ifEnd);
                        } else {
                            BRANCH(Equal(*holder, receiver), &holdEqualsRecv1, &afterLoop);
                        }
                        Bind(&holdEqualsRecv1);
                        {
                            Label isJSShared(env);
                            Label executeSetProp(env);
                            BRANCH(IsJSShared(glue, receiver), &isJSShared, &executeSetProp);
                            Bind(&isJSShared);
                            {
                                DEFVARIABLE(actualValue, VariableType::JS_ANY(), value);
                                Label executeSharedSetProp(env);
                                SharedObjectStoreBarrierWithTypeCheck(true, &result, glue, attr1, value, &actualValue,
                                    &executeSharedSetProp, &exit);
                                Bind(&executeSharedSetProp);
                                {
                                    UpdateValueInDict<NameDictionary>(glue, array, entry1, *actualValue);
                                    result = Undefined();
                                    Jump(&exit);
                                }
                            }
                            Bind(&executeSetProp);
                            {
                                UpdateValueInDict<NameDictionary>(glue, array, entry1, value);
                                result = Undefined();
                                Jump(&exit);
                            }
                        }
                    }
                }
            }
        }
    }
    if (useOwn || defineSemantics) {
        Bind(&ifEnd);
    } else {
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(glue, LoadHClass(glue, *holder));
            BRANCH(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
        Bind(&afterLoop);
    }
    Label holeEntryNotNegtiveOne(env);
    Label holeEntryIfEnd(env);
    BRANCH(Int32NotEqual(*receiverHoleEntry, Int32(-1)), &holeEntryNotNegtiveOne, &holeEntryIfEnd);
    Bind(&holeEntryNotNegtiveOne);
    {
        GateRef receiverHClass = LoadHClass(glue, receiver);
        GateRef receiverLayoutInfo = GetLayoutFromHClass(glue, receiverHClass);
        GateRef holeAttr = GetPropAttrFromLayoutInfo(glue, receiverLayoutInfo, *receiverHoleEntry);
        JSObjectSetProperty(glue, receiver, receiverHClass, holeAttr, key, value);
        ProfilerStubBuilder(env).UpdatePropAttrWithValue(glue, receiver, holeAttr, value, callback);
        result = Undefined();
        Jump(&exit);
    }
    Bind(&holeEntryIfEnd);

    Label extensible(env);
    Label inextensible(env);
    BRANCH(IsExtensible(glue, receiver), &extensible, &inextensible);
    Bind(&inextensible);
    {
        if (mayThrow) {
            result = ThrowTypeErrorInextensiableAddProperty(glue, key);
        } else {
            result = TaggedFalse();
        }
        Jump(&exit);
    }
    Bind(&extensible);
    {
        result = AddPropertyByName(glue, receiver, key, value,
            Int64(PropertyAttributes::GetDefaultAttributes()), callback);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::DefinePropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value,
    GateRef isInternal, GateRef SCheckModelIsCHECK, ProfileOperation callback)
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

    GateRef hclass = LoadHClass(glue, *holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSIndexObj(env);
    Label notSIndexObj(env);
    BRANCH(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
    Bind(&isSIndexObj);
    {
        Label isFastTypeArray(env);
        Label notFastTypeArray(env);
        BRANCH(IsFastTypeArray(jsType), &isFastTypeArray, &notFastTypeArray);
        Bind(&isFastTypeArray);
        {
            result = SetTypeArrayPropertyByName(glue, receiver, *holder, key, value, jsType);
            Label isNull(env);
            Label notNull(env);
            BRANCH(TaggedIsNull(*result), &isNull, &notNull);
            Bind(&isNull);
            {
                result = Hole();
                Jump(&exit);
            }
            Bind(&notNull);
            BRANCH(TaggedIsHole(*result), &notSIndexObj, &exit);
        }
        Bind(&notFastTypeArray);

        Label isSpecialContainer(env);
        Label notSpecialContainer(env);
        BRANCH(IsArrayListOrVector(jsType), &isSpecialContainer, &notSpecialContainer);
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
        BRANCH(isInternal, &findProperty, &ifEnd);
        Label isDicMode(env);
        Label notDicMode(env);
        Bind(&findProperty);
        BRANCH(IsDictionaryModeByHClass(hclass), &isDicMode, &notDicMode);
        Bind(&notDicMode);
        {
            GateRef layOutInfo = GetLayoutFromHClass(glue, hclass);
            GateRef propsNum = GetNumberOfPropsFromHClass(hclass);
            GateRef entry = FindElementWithCache(glue, layOutInfo, hclass, key, propsNum);
            Label hasEntry(env);
            BRANCH(Int32NotEqual(entry, Int32(-1)), &hasEntry, &ifEnd);
            Bind(&hasEntry);
            {
                GateRef attr = GetPropAttrFromLayoutInfo(glue, layOutInfo, entry);
                Label isAccessor(env);
                Label notAccessor(env);
                Label isSCheckModelIsCHECK1(env);
                Label isNotSCheckModelIsCHECK1(env);
                BRANCH(IsAccessor(attr), &isAccessor, &notAccessor);
                Bind(&isAccessor);
                BRANCH(SCheckModelIsCHECK, &isSCheckModelIsCHECK1, &isNotSCheckModelIsCHECK1);
                Bind(&isSCheckModelIsCHECK1);
                {
                    Jump(&exit);
                }
                Bind(&isNotSCheckModelIsCHECK1);
                GateRef accessor = JSObjectGetPropertyWithRep(glue, *holder, hclass, attr);
                Label shouldCall(env);
                BRANCH(ShouldCallSetter(glue, receiver, *holder, accessor, attr), &shouldCall, &notAccessor);
                Bind(&shouldCall);
                {
                    result = CallSetterHelper(glue, receiver, accessor, value, callback);
                    Jump(&exit);
                }
                Bind(&notAccessor);
                {
                    Label writable(env);
                    Label notWritable(env);
                    Label isSCheckModelIsCHECK2(env);
                    Label isNotSCheckModelIsCHECK2(env);
                    BRANCH(IsWritable(attr), &writable, &notWritable);
                    Bind(&notWritable);
                    BRANCH(SCheckModelIsCHECK, &isSCheckModelIsCHECK2, &isNotSCheckModelIsCHECK2);
                    Bind(&isSCheckModelIsCHECK2);
                    {
                        Jump(&exit);
                    }
                    Bind(&isNotSCheckModelIsCHECK2);
                    GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                    CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                    result = Exception();
                    Jump(&exit);
                    Bind(&writable);
                    {
                        Label isAOT(env);
                        Label notAOT(env);
                        BRANCH(IsAOTHClass(hclass), &isAOT, &notAOT);
                        Bind(&isAOT);
                        {
                            GateRef attrVal = JSObjectGetPropertyWithRep(glue, *holder, hclass, attr);
                            Label attrValIsHole(env);
                            BRANCH(TaggedIsHole(attrVal), &attrValIsHole, &notAOT);
                            Bind(&attrValIsHole);
                            {
                                Label storeReceiverHoleEntry(env);
                                Label noNeedStore(env);
                                GateRef checkReceiverHoleEntry = Int32Equal(*receiverHoleEntry, Int32(-1));
                                GateRef checkHolderEqualsRecv = Equal(*holder, receiver);
                                BRANCH(BitAnd(checkReceiverHoleEntry, checkHolderEqualsRecv),
                                    &storeReceiverHoleEntry, &ifEnd);
                                Bind(&storeReceiverHoleEntry);
                                {
                                    receiverHoleEntry = entry;
                                    Jump(&ifEnd);
                                }
                            }
                        }
                        Bind(&notAOT);
                        Label holdEqualsRecv(env);
                        BRANCH(Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
                        Bind(&holdEqualsRecv);
                        {
                            Label isJSShared(env);
                            Label executeSetProp(env);
                            BRANCH(IsJSShared(glue, receiver), &isJSShared, &executeSetProp);
                            Bind(&isJSShared);
                            {
                                DEFVARIABLE(actualValue, VariableType::JS_ANY(), value);
                                Label executeSharedSetProp(env);
                                SharedObjectStoreBarrierWithTypeCheck(false, &result, glue, attr, value, &actualValue,
                                    &executeSharedSetProp, &exit, SCheckModelIsCHECK);
                                Bind(&executeSharedSetProp);
                                {
                                    JSObjectSetProperty(glue, *holder, hclass, attr, key, *actualValue);
                                    ProfilerStubBuilder(env).UpdatePropAttrWithValue(glue, receiver, attr,
                                        *actualValue, callback);
                                    result = Undefined();
                                    Jump(&exit);
                                }
                            }
                            Bind(&executeSetProp);
                            {
                                JSObjectSetProperty(glue, *holder, hclass, attr, key, value);
                                ProfilerStubBuilder(env).UpdatePropAttrWithValue(glue, receiver, attr, value,
                                    callback);
                                result = Undefined();
                                Jump(&exit);
                            }
                        }
                    }
                }
            }
        }
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(glue, *holder);
            GateRef entry1 = FindEntryFromHashTable<NameDictionary>(glue, array, key);
            Label notNegtiveOne(env);
            BRANCH(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &ifEnd);
            Bind(&notNegtiveOne);
            {
                GateRef attr1 = GetAttributesFromDictionary<NameDictionary>(glue, array, entry1);
                Label isAccessor1(env);
                Label notAccessor1(env);
                Label isSCheckModelIsCHECK3(env);
                Label isNotSCheckModelIsCHECK3(env);
                BRANCH(IsAccessor(attr1), &isAccessor1, &notAccessor1);
                Bind(&isAccessor1);
                BRANCH(SCheckModelIsCHECK, &isSCheckModelIsCHECK3, &isNotSCheckModelIsCHECK3);
                Bind(&isSCheckModelIsCHECK3);
                {
                    Jump(&exit);
                }
                Bind(&isNotSCheckModelIsCHECK3);
                GateRef accessor1 = GetValueFromDictionary<NameDictionary>(glue, array, entry1);
                Label shouldCall1(env);
                BRANCH(ShouldCallSetter(glue, receiver, *holder, accessor1, attr1), &shouldCall1, &notAccessor1);
                Bind(&shouldCall1);
                {
                    result = CallSetterHelper(glue, receiver, accessor1, value, callback);
                    Jump(&exit);
                }
                Bind(&notAccessor1);
                {
                    Label writable1(env);
                    Label notWritable1(env);
                    Label isSCheckModelIsCHECK4(env);
                    Label isNotSCheckModelIsCHECK4(env);
                    BRANCH(IsWritable(attr1), &writable1, &notWritable1);
                    Bind(&notWritable1);
                    BRANCH(SCheckModelIsCHECK, &isSCheckModelIsCHECK4, &isNotSCheckModelIsCHECK4);
                    Bind(&isSCheckModelIsCHECK4);
                    {
                        Jump(&exit);
                    }
                    Bind(&isNotSCheckModelIsCHECK4);
                    GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                    CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                    result = Exception();
                    Jump(&exit);
                    Bind(&writable1);
                    {
                        Label holdEqualsRecv1(env);
                        BRANCH(Equal(*holder, receiver), &holdEqualsRecv1, &ifEnd);
                        Bind(&holdEqualsRecv1);
                        {
                            Label isJSShared(env);
                            Label executeSetProp(env);
                            BRANCH(IsJSShared(glue, receiver), &isJSShared, &executeSetProp);
                            Bind(&isJSShared);
                            {
                                DEFVARIABLE(actualValue, VariableType::JS_ANY(), value);
                                Label executeSharedSetProp(env);
                                SharedObjectStoreBarrierWithTypeCheck(true, &result, glue, attr1, value, &actualValue,
                                    &executeSharedSetProp, &exit, SCheckModelIsCHECK);
                                Bind(&executeSharedSetProp);
                                {
                                    UpdateValueInDict<NameDictionary>(glue, array, entry1, *actualValue);
                                    result = Undefined();
                                    Jump(&exit);
                                }
                            }
                            Bind(&executeSetProp);
                            {
                                UpdateValueInDict<NameDictionary>(glue, array, entry1, value);
                                result = Undefined();
                                Jump(&exit);
                            }
                        }
                    }
                }
            }
        }
    }
    Bind(&ifEnd);
    Label holeEntryNotNegtiveOne(env);
    Label holeEntryIfEnd(env);
    BRANCH(Int32NotEqual(*receiverHoleEntry, Int32(-1)), &holeEntryNotNegtiveOne, &holeEntryIfEnd);
    Bind(&holeEntryNotNegtiveOne);
    {
        GateRef receiverHClass = LoadHClass(glue, receiver);
        GateRef receiverLayoutInfo = GetLayoutFromHClass(glue, receiverHClass);
        GateRef holeAttr = GetPropAttrFromLayoutInfo(glue, receiverLayoutInfo, *receiverHoleEntry);
        JSObjectSetProperty(glue, receiver, receiverHClass, holeAttr, key, value);
        ProfilerStubBuilder(env).UpdatePropAttrWithValue(glue, receiver, holeAttr, value, callback);
        result = Undefined();
        Jump(&exit);
    }
    Bind(&holeEntryIfEnd);

    Label extensible(env);
    Label inextensible(env);
    BRANCH(IsExtensible(glue, receiver), &extensible, &inextensible);
    Bind(&inextensible);
    {
        result = ThrowTypeErrorInextensiableAddProperty(glue, key);
        Jump(&exit);
    }
    Bind(&extensible);
    {
        result = AddPropertyByName(glue, receiver, key, value,
            Int64(PropertyAttributes::GetDefaultAttributes()), callback);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

/**
    Ruturn value: Hole means try fastPath fail, caller should go slow path, undefined is try fastPath succsee.
    only when mayThrow, will return TaggedFalse, it means some step has exception but not throw in
    thread, same as C interpreter SetProperty
    In ANY situation, exception means that an error must have occurred and been thrown on the thread.
 */

GateRef StubBuilder::SetPropertyByValue(GateRef glue,
                                        GateRef receiver,
                                        GateRef key,
                                        GateRef value,
                                        bool useOwn,
                                        ProfileOperation callback,
                                        bool defineSemantics,
                                        bool mayThrow)
{
    auto env = GetEnvironment();
    Label subEntry1(env);
    env->SubCfgEntry(&subEntry1);
    DEFVARIABLE(varKey, VariableType::JS_ANY(), key);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(isInternal, VariableType::BOOL(), True());
    Label isPropertyKey(env);
    Label exit(env);

    BRANCH(TaggedIsPropertyKey(glue, *varKey), &isPropertyKey, &exit);

    Bind(&isPropertyKey);
    {
        GateRef index64 = TryToElementsIndex(glue, *varKey);
        Label validIndex(env);
        Label notValidIndex(env);
        Label greaterThanInt32Max(env);
        Label notGreaterThanInt32Max(env);
        BRANCH(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
        Bind(&greaterThanInt32Max);
        {
            Jump(&exit);
        }
        Bind(&notGreaterThanInt32Max);
        GateRef index = TruncInt64ToInt32(index64);
        BRANCH(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = SetPropertyByIndex(glue, receiver, index, value, useOwn, callback, defineSemantics, mayThrow);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label isNumber1(env);
            Label notNumber1(env);
            Label setByName(env);
            BRANCH(TaggedIsNumber(*varKey), &isNumber1, &notNumber1);
            Bind(&isNumber1);
            {
                result = Hole();
                Jump(&exit);
            }
            Label isString(env);
            Label checkDetector(env);
            Bind(&notNumber1);
            {
                Label notIntenalString(env);
                BRANCH(TaggedIsString(glue, *varKey), &isString, &checkDetector);
                Bind(&isString);
                {
                    BRANCH(IsInternalString(*varKey), &setByName, &notIntenalString);
                    Bind(&notIntenalString);
                    {
                    #if ENABLE_NEXT_OPTIMIZATION
                        GateRef res = CallRuntime(glue, RTSTUB_ID(GetOrInternStringFromHashTrieTable), { *varKey });
                        varKey = res;
                        Jump(&checkDetector);
                    #else
                        Label notFind(env);
                        Label find(env);
                        GateRef res = CallRuntime(glue, RTSTUB_ID(TryGetInternString), { *varKey });
                        BRANCH(TaggedIsHole(res), &notFind, &find);
                        Bind(&notFind);
                        {
                            varKey = CallRuntime(glue, RTSTUB_ID(InsertStringToTable), { *varKey });
                            isInternal = False();
                            Jump(&checkDetector);
                        }
                        Bind(&find);
                        {
                            varKey = res;
                            Jump(&checkDetector);
                        }
                    #endif
                    }
                }
            }
            Bind(&checkDetector);
            CheckDetectorName(glue, *varKey, &setByName, &exit);
            Bind(&setByName);
            {
                result = SetPropertyByName(
                    glue, receiver, *varKey, value, useOwn, *isInternal, callback, true, defineSemantics, mayThrow);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// ObjectFastOperator::SetPropertyByValue
GateRef StubBuilder::DefinePropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value,
    GateRef SCheckModelIsCHECK, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label subEntry1(env);
    env->SubCfgEntry(&subEntry1);
    DEFVARIABLE(varKey, VariableType::JS_ANY(), key);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(isInternal, VariableType::BOOL(), True());
    Label isPropertyKey(env);
    Label exit(env);

    BRANCH(TaggedIsPropertyKey(glue, *varKey), &isPropertyKey, &exit);

    Bind(&isPropertyKey);
    {
        GateRef index64 = TryToElementsIndex(glue, *varKey);
        Label validIndex(env);
        Label notValidIndex(env);
        Label greaterThanInt32Max(env);
        Label notGreaterThanInt32Max(env);
        BRANCH(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
        Bind(&greaterThanInt32Max);
        {
            Jump(&exit);
        }
        Bind(&notGreaterThanInt32Max);
        GateRef index = TruncInt64ToInt32(index64);
        BRANCH(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = DefinePropertyByIndex(glue, receiver, index, value);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label isNumber1(env);
            Label notNumber1(env);
            Label setByName(env);
            BRANCH(TaggedIsNumber(*varKey), &isNumber1, &notNumber1);
            Bind(&isNumber1);
            {
                result = Hole();
                Jump(&exit);
            }
            Label isString(env);
            Label checkDetector(env);
            Bind(&notNumber1);
            {
                Label notIntenalString(env);
                BRANCH(TaggedIsString(glue, *varKey), &isString, &checkDetector);
                Bind(&isString);
                {
                    BRANCH(IsInternalString(*varKey), &setByName, &notIntenalString);
                    Bind(&notIntenalString);
                    {
                    #if ENABLE_NEXT_OPTIMIZATION
                        GateRef res = CallRuntime(glue, RTSTUB_ID(GetOrInternStringFromHashTrieTable), { *varKey });
                        varKey = res;
                        Jump(&checkDetector);
                    #else
                        Label notFind(env);
                        Label find(env);
                        GateRef res = CallRuntime(glue, RTSTUB_ID(TryGetInternString), { *varKey });
                        BRANCH(TaggedIsHole(res), &notFind, &find);
                        Bind(&notFind);
                        {
                            varKey = CallRuntime(glue, RTSTUB_ID(InsertStringToTable), { *varKey });
                            isInternal = False();
                            Jump(&checkDetector);
                        }
                        Bind(&find);
                        {
                            varKey = res;
                            Jump(&checkDetector);
                        }
                    #endif
                    }
                }
            }
            Bind(&checkDetector);
            CheckDetectorName(glue, *varKey, &setByName, &exit);
            Bind(&setByName);
            {
                result = DefinePropertyByName(glue, receiver, *varKey, value, *isInternal,
                    SCheckModelIsCHECK, callback);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value)
{
    GateRef valueIndex = Int32Add(index, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, object, valueIndex, value);
}

void StubBuilder::NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isPrototype(env);
    BRANCH(IsPrototypeHClass(oldHClass), &isPrototype, &exit);
    Bind(&isPrototype);
    {
        Label notEqualHClass(env);
        BRANCH(Equal(oldHClass, newHClass), &exit, &notEqualHClass);
        Bind(&notEqualHClass);
        {
            CallRuntime(glue, RTSTUB_ID(NoticeThroughChainAndRefreshUser), { oldHClass, newHClass });
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef StubBuilder::FastTypeOf(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    GateRef gConstAddr = LoadPrimitive(VariableType::JS_ANY(), glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    GateRef undefinedIndex = GetGlobalConstantOffset(ConstantIndex::UNDEFINED_STRING_INDEX);
    GateRef gConstUndefinedStr = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr, undefinedIndex);
    DEFVARIABLE(result, VariableType::JS_POINTER(), gConstUndefinedStr);
    Label objIsTrue(env);
    Label objNotTrue(env);
    Label defaultLabel(env);
    GateRef gConstBooleanStr = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
        GetGlobalConstantOffset(ConstantIndex::BOOLEAN_STRING_INDEX));
    BRANCH(TaggedIsTrue(obj), &objIsTrue, &objNotTrue);
    Bind(&objIsTrue);
    {
        result = gConstBooleanStr;
        Jump(&exit);
    }
    Bind(&objNotTrue);
    {
        Label objIsFalse(env);
        Label objNotFalse(env);
        BRANCH(TaggedIsFalse(obj), &objIsFalse, &objNotFalse);
        Bind(&objIsFalse);
        {
            result = gConstBooleanStr;
            Jump(&exit);
        }
        Bind(&objNotFalse);
        {
            Label objIsNull(env);
            Label objNotNull(env);
            BRANCH(TaggedIsNull(obj), &objIsNull, &objNotNull);
            Bind(&objIsNull);
            {
                result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantOffset(ConstantIndex::OBJECT_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNull);
            {
                Label objIsUndefined(env);
                Label objNotUndefined(env);
                BRANCH(TaggedIsUndefined(obj), &objIsUndefined, &objNotUndefined);
                Bind(&objIsUndefined);
                {
                    result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
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
        BRANCH(TaggedIsHeapObject(obj), &objIsHeapObject, &objNotHeapObject);
        Bind(&objIsHeapObject);
        {
            Label objIsString(env);
            Label objNotString(env);
            BRANCH(IsString(glue, obj), &objIsString, &objNotString);
            Bind(&objIsString);
            {
                result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantOffset(ConstantIndex::STRING_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotString);
            {
                Label objIsSymbol(env);
                Label objNotSymbol(env);
                BRANCH_UNLIKELY(IsSymbol(glue, obj), &objIsSymbol, &objNotSymbol);
                Bind(&objIsSymbol);
                {
                    result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                        GetGlobalConstantOffset(ConstantIndex::SYMBOL_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotSymbol);
                {
                    Label objIsCallable(env);
                    Label objNotCallable(env);
                    BRANCH_UNLIKELY(IsCallable(glue, obj), &objIsCallable, &objNotCallable);
                    Bind(&objIsCallable);
                    {
                        result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                            GetGlobalConstantOffset(ConstantIndex::FUNCTION_STRING_INDEX));
                        Jump(&exit);
                    }
                    Bind(&objNotCallable);
                    {
                        Label objIsBigInt(env);
                        Label objNotBigInt(env);
                        BRANCH_UNLIKELY(TaggedObjectIsBigInt(glue, obj), &objIsBigInt, &objNotBigInt);
                        Bind(&objIsBigInt);
                        {
                            result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                                GetGlobalConstantOffset(ConstantIndex::BIGINT_STRING_INDEX));
                            Jump(&exit);
                        }
                        Bind(&objNotBigInt);
                        {
                            Label objIsNativeModuleFailureInfo(env);
                            Label objNotNativeModuleFailureInfo(env);
                            BRANCH_UNLIKELY(IsNativeModuleFailureInfo(glue, obj), &objIsNativeModuleFailureInfo,
                                &objNotNativeModuleFailureInfo);
                            Bind(&objIsNativeModuleFailureInfo);
                            {
                                result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                                    GetGlobalConstantOffset(ConstantIndex::NATIVE_MODULE_FAILURE_INFO_STRING_INDEX));
                                Jump(&exit);
                            }
                            Bind(&objNotNativeModuleFailureInfo);
                            {
                                result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
                                    GetGlobalConstantOffset(ConstantIndex::OBJECT_STRING_INDEX));
                                Jump(&exit);
                            }
                        }
                    }
                }
            }
        }
        Bind(&objNotHeapObject);
        {
            Label objIsNum(env);
            Label objNotNum(env);
            BRANCH(TaggedIsNumber(obj), &objIsNum, &objNotNum);
            Bind(&objIsNum);
            {
                result = LoadPrimitive(VariableType::JS_POINTER(), gConstAddr,
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
    BRANCH(TaggedIsHeapObject(target), &targetIsHeapObject, &targetNotEcmaObject);
    Bind(&targetIsHeapObject);
    BRANCH(TaggedObjectIsEcmaObject(glue, target), &targetIsEcmaObject, &targetNotEcmaObject);
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
        GateRef globalEnv = GetCurrentGlobalEnv();
        GateRef hasInstanceSymbol = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                      GlobalEnv::HASINSTANCE_SYMBOL_INDEX);
        GateRef instof = GetMethod(glue, target, hasInstanceSymbol, profileTypeInfo, slotId);

        // 3.ReturnIfAbrupt(instOfHandler).
        Label isPendingException(env);
        Label noPendingException(env);
        BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
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
        BRANCH(TaggedIsUndefined(instof), &instOfIsUndefined, &instOfNotUndefined);
        Bind(&instOfNotUndefined);
        {
            TryFastHasInstance(glue, instof, target, object, &fastPath, &exit, &result, callback);
        }
        Bind(&instOfIsUndefined);
        {
            // 5.If IsCallable(glue, target) is false, throw a TypeError exception.
            BRANCH(IsCallable(glue, target), &fastPath, &targetNotCallable);
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

    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef function = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                         GlobalEnv::HASINSTANCE_FUNCTION_INDEX);

    Label slowPath(env);
    Label tryFastPath(env);
    Label callExit(env);
    GateRef isEqual = IntPtrEqual(instof, function);
    BRANCH(isEqual, &tryFastPath, &slowPath);
    Bind(&tryFastPath);
    Jump(fastPath);
    Bind(&slowPath);
    {
        JSCallArgs callArgs(JSCallMode::CALL_SETTER);
        callArgs.callSetterArgs = { target, object };
        if (env->IsBaselineBuiltin()) {
            DEFVARIABLE(callRes, VariableType::JS_ANY(), Undefined());
            CallStubBuilder callBuilder(this, glue, instof, Int32(1), 0, &callRes, Circuit::NullGate(), callArgs,
                callback);
            callBuilder.JSCallDispatchForBaseline(&callExit);
            Bind(&callExit);
            result->WriteVariable(FastToBoolean(glue, *callRes));
        } else {
            CallStubBuilder callBuilder(this, glue, instof, Int32(1), 0, nullptr, Circuit::NullGate(), callArgs,
                callback);
            GateRef retValue = callBuilder.JSCallDispatch();
            result->WriteVariable(FastToBoolean(glue, retValue));
        }
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
    AccessObjectStubBuilder builder(this, GetCurrentGlobalEnv());
    GateRef value = builder.LoadObjByName(glue, obj, key, info, profileTypeInfo, slotId, ProfileOperation());

    Label isPendingException(env);
    Label noPendingException(env);
    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noPendingException);
    Label valueIsUndefinedOrNull(env);
    Label valueNotUndefinedOrNull(env);
    BRANCH(TaggedIsUndefinedOrNull(value), &valueIsUndefinedOrNull, &valueNotUndefinedOrNull);
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
        BRANCH(TaggedIsHeapObject(value), &valueIsHeapObject, &valueNotCallable);
        Bind(&valueIsHeapObject);
        BRANCH(IsCallable(glue, value), &valueIsCallable, &valueNotCallable);
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

GateRef StubBuilder::FastGetPropertyByName(GateRef glue, GateRef obj, GateRef key,
                                           ProfileOperation callback, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label checkResult(env);
    Label fastpath(env);
    Label slowpath(env);

    BRANCH(TaggedIsHeapObject(obj), &fastpath, &slowpath);
    Bind(&fastpath);
    {
        result = GetPropertyByName(glue, obj, key, Circuit::NullGate(), callback, True(), false, hir);
        BRANCH(TaggedIsHole(*result), &slowpath, &exit);
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

GateRef StubBuilder::FastGetPropertyByIndex(GateRef glue, GateRef obj,
                                            GateRef index, ProfileOperation callback, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label fastPath(env);
    Label slowPath(env);

    BRANCH(TaggedIsHeapObject(obj), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        result = GetPropertyByIndex(glue, obj, index, callback, hir);
        Label notHole(env);
        BRANCH(TaggedIsHole(*result), &slowPath, &exit);
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
    BRANCH(TaggedIsHeapObject(obj), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Label isString(env);
        Label getByName(env);
        Label isInternalString(env);
        Label notIntenalString(env);
        BRANCH(TaggedIsString(glue, *keyVar), &isString, &getByName);
        Bind(&isString);
        {
            BRANCH(IsInternalString(*keyVar), &isInternalString, &notIntenalString);
            Bind(&isInternalString);
            Jump(&getByName);
            Bind(&notIntenalString);
            {
            #if ENABLE_NEXT_OPTIMIZATION
                GateRef res = CallRuntime(glue, RTSTUB_ID(GetOrInternStringFromHashTrieTable), { *keyVar });
                keyVar = res;
                Jump(&getByName);
            #else
                Label notFind(env);
                Label find(env);
                GateRef res = CallRuntime(glue, RTSTUB_ID(TryGetInternString), { *keyVar });
                BRANCH(TaggedIsHole(res), &notFind, &find);
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
            #endif
            }
        }
        Bind(&getByName);

        result = SetPropertyByName(glue, obj, *keyVar, value, false, *isInternal, callback, true);
        Label notHole(env);
        BRANCH(TaggedIsHole(*result), &slowPath, &exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(StObjByValue), { obj, *keyVar, value });
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

    BRANCH(TaggedIsHeapObject(obj), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        result = SetPropertyByIndex(glue, obj, index, value, false);
        Label notHole(env);
        BRANCH(TaggedIsHole(*result), &slowPath, &exit);
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

GateRef StubBuilder::GetCtorPrototype(GateRef glue, GateRef ctor)
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

    GateRef ctorProtoOrHC = Load(VariableType::JS_POINTER(), glue, ctor, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(TaggedIsHeapObject(ctorProtoOrHC), &isHeapObject, &notHeapObject);
    Bind(&notHeapObject);
    {
        // If go slow path, return hole.
        constructorPrototype = Hole();
        Jump(&exit);
    }
    Bind(&isHeapObject);
    BRANCH(IsJSHClass(glue, ctorProtoOrHC), &isHClass, &isPrototype);
    Bind(&isHClass);
    {
        constructorPrototype = GetPrototypeFromHClass(glue, ctorProtoOrHC);
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

GateRef StubBuilder::HasFunctionPrototype(GateRef glue, GateRef ctor)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    Label exit(env);
    Label isHole(env);

    GateRef ctorProtoOrHC = Load(VariableType::JS_POINTER(), glue, ctor, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(TaggedIsHole(ctorProtoOrHC), &isHole, &exit);
    Bind(&isHole);
    {
        result = False();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
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

    // 1. If IsCallable(glue, C) is false, return false.
    Label targetIsCallable(env);
    Label targetNotCallable(env);
    BRANCH(IsCallable(glue, target), &targetIsCallable, &targetNotCallable);
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
        BRANCH(IsBoundFunction(glue, target), &targetIsBoundFunction, &targetNotBoundFunction);
        Bind(&targetIsBoundFunction);
        {
            GateRef boundTarget = Load(VariableType::JS_ANY(), glue, target, IntPtr(JSBoundFunction::BOUND_TARGET_OFFSET));
            result = CallRuntime(glue, RTSTUB_ID(InstanceOf), { obj, boundTarget });
            Jump(&exit);
        }
        Bind(&targetNotBoundFunction);
        {
            // 3. If Type(O) is not Object, return false
            Label objIsHeapObject(env);
            Label objIsEcmaObject(env);
            Label objNotEcmaObject(env);
            BRANCH(TaggedIsHeapObject(obj), &objIsHeapObject, &objNotEcmaObject);
            Bind(&objIsHeapObject);
            BRANCH(TaggedObjectIsEcmaObject(glue, obj), &objIsEcmaObject, &objNotEcmaObject);
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
                BRANCH(IsJSFunction(glue, target), &ctorIsJSFunction, &getCtorProtoSlowPath);
                Bind(&ctorIsJSFunction);
                {
                    Label getCtorProtoFastPath(env);
                    GateRef ctorProtoOrHC = Load(VariableType::JS_POINTER(), glue, target,
                                                 IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));

                    BRANCH(TaggedIsHole(ctorProtoOrHC), &getCtorProtoSlowPath, &getCtorProtoFastPath);
                    Bind(&getCtorProtoFastPath);
                    {
                        constructorPrototype = GetCtorPrototype(glue, target);
                        BRANCH(TaggedIsHole(*constructorPrototype), &getCtorProtoSlowPath, &gotCtorPrototype);
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
                BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
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
                BRANCH(TaggedIsHeapObject(*constructorPrototype), &constructorPrototypeIsHeapObject,
                    &constructorPrototypeNotEcmaObject);
                Bind(&constructorPrototypeIsHeapObject);
                BRANCH(TaggedObjectIsEcmaObject(glue, *constructorPrototype), &constructorPrototypeIsEcmaObject,
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

                    BRANCH(TaggedIsNull(*object), &afterLoop, &loopHead);
                    LoopBegin(&loopHead);
                    {
                        GateRef isEqual = SameValue(glue, *object, *constructorPrototype);

                        BRANCH(isEqual, &strictEqual1, &notStrictEqual1);
                        Bind(&strictEqual1);
                        {
                            result = TaggedTrue();
                            Jump(&exit);
                        }
                        Bind(&notStrictEqual1);
                        {
                            object = GetPrototype(glue, *object);

                            BRANCH(HasPendingException(glue), &shouldReturn, &shouldContinue);
                            Bind(&shouldReturn);
                            {
                                result = Exception();
                                Jump(&exit);
                            }
                        }
                        Bind(&shouldContinue);
                        BRANCH(TaggedIsNull(*object), &afterLoop, &loopEnd);
                    }
                    Bind(&loopEnd);
                    LoopEndWithCheckSafePoint(&loopHead, env, glue);
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

    BRANCH(Equal(left, right), &strictEqual, &numberEqualCheck1);
    Bind(&strictEqual);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&numberEqualCheck1);
    {
        Label leftIsNumber(env);
        Label leftIsNotNumber(env);
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &exit);
            Bind(&rightIsNumber);
            {
                Label numberEqualCheck2(env);
                Label leftIsInt(env);
                Label leftNotInt(env);
                Label getRight(env);

                BRANCH(TaggedIsInt(left), &leftIsInt, &leftNotInt);
                Bind(&leftIsInt);
                {
                    Label fastPath(env);
                    Label slowPath(env);
                    BRANCH(TaggedIsInt(right), &fastPath, &slowPath);
                    Bind(&fastPath);
                    {
                        result = False();
                        Jump(&exit);
                    }
                    Bind(&slowPath);
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&numberEqualCheck2);
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
                    BRANCH(TaggedIsInt(right), &rightIsInt, &rightNotInt);
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
                    BRANCH(DoubleEqual(*doubleLeft, *doubleRight), &signbitCheck, &boolAndCheck);
                    Bind(&signbitCheck);
                    {
                        GateRef leftEncoding = CastDoubleToInt64(*doubleLeft);
                        GateRef RightEncoding = CastDoubleToInt64(*doubleRight);
                        Label leftIsMinusZero(env);
                        Label leftNotMinusZero(env);
                        BRANCH(Int64Equal(leftEncoding, Int64(base::MINUS_ZERO_BITS)),
                            &leftIsMinusZero, &leftNotMinusZero);
                        Bind(&leftIsMinusZero);
                        {
                            Label rightIsMinusZero(env);
                            BRANCH(Int64Equal(RightEncoding, Int64(base::MINUS_ZERO_BITS)), &rightIsMinusZero, &exit);
                            Bind(&rightIsMinusZero);
                            {
                                result = True();
                                Jump(&exit);
                            }
                        }
                        Bind(&leftNotMinusZero);
                        {
                            Label rightNotMinusZero(env);
                            BRANCH(Int64Equal(RightEncoding, Int64(base::MINUS_ZERO_BITS)), &exit, &rightNotMinusZero);
                            Bind(&rightNotMinusZero);
                            {
                                result = True();
                                Jump(&exit);
                            }
                        }
                    }
                    Bind(&boolAndCheck);
                    {
                        GateRef doubleLeftVal = *doubleLeft;
                        GateRef doubleRightVal = *doubleRight;
                        result = LogicAndBuilder(env).And(DoubleIsNAN(doubleLeftVal))
                            .And(DoubleIsNAN(doubleRightVal)).Done();
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&leftIsNotNumber);
        BRANCH(TaggedIsNumber(right), &exit, &stringEqualCheck);
        Bind(&stringEqualCheck);
        BRANCH(BothAreString(glue, left, right), &stringCompare, &bigIntEqualCheck);
        Bind(&stringCompare);
        {
            result = FastStringEqual(glue, left, right);
            Jump(&exit);
        }
        Bind(&bigIntEqualCheck);
        {
            Label leftIsBigInt(env);
            Label leftIsNotBigInt(env);
            BRANCH(TaggedIsBigInt(glue, left), &leftIsBigInt, &exit);
            Bind(&leftIsBigInt);
            {
                Label rightIsBigInt(env);
                BRANCH(TaggedIsBigInt(glue, right), &rightIsBigInt, &exit);
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

    BRANCH(Equal(left, right), &strictEqual, &numberEqualCheck1);
    Bind(&strictEqual);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&numberEqualCheck1);
    {
        Label leftIsNumber(env);
        Label leftIsNotNumber(env);
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &exit);
            Bind(&rightIsNumber);
            {
                Label numberEqualCheck2(env);
                Label leftIsInt(env);
                Label leftNotInt(env);
                Label getRight(env);
                BRANCH(TaggedIsInt(left), &leftIsInt, &leftNotInt);
                Bind(&leftIsInt);
                {
                    Label fastPath(env);
                    Label slowPath(env);
                    BRANCH(TaggedIsInt(right), &fastPath, &slowPath);
                    Bind(&fastPath);
                    {
                        result = False();
                        Jump(&exit);
                    }
                    Bind(&slowPath);
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&numberEqualCheck2);
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
                    BRANCH(TaggedIsInt(right), &rightIsInt, &rightNotInt);
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
                    BRANCH(DoubleEqual(*doubleLeft, *doubleRight), &doubleEqual, &nanCheck);
                    Bind(&doubleEqual);
                    {
                        result = True();
                        Jump(&exit);
                    }
                    Bind(&nanCheck);
                    {
                        GateRef doubleLeftVal = *doubleLeft;
                        GateRef doubleRightVal = *doubleRight;
                        result = LogicAndBuilder(env).And(DoubleIsNAN(doubleLeftVal))
                            .And(DoubleIsNAN(doubleRightVal)).Done();
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&leftIsNotNumber);
        BRANCH(TaggedIsNumber(right), &exit, &stringEqualCheck);
        Bind(&stringEqualCheck);
        BRANCH(BothAreString(glue, left, right), &stringCompare, &bigIntEqualCheck);
        Bind(&stringCompare);
        {
            result = FastStringEqual(glue, left, right);
            Jump(&exit);
        }
        Bind(&bigIntEqualCheck);
        {
            Label leftIsBigInt(env);
            Label leftIsNotBigInt(env);
            BRANCH(TaggedIsBigInt(glue, left), &leftIsBigInt, &exit);
            Bind(&leftIsBigInt);
            {
                Label rightIsBigInt(env);
                BRANCH(TaggedIsBigInt(glue, right), &rightIsBigInt, &exit);
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
    BRANCH(Int32Equal(GetLengthFromString(left), GetLengthFromString(right)), &lenEqualOneCheck, &exit);
    Bind(&lenEqualOneCheck);
    BRANCH(Int32Equal(GetLengthFromString(left), Int32(1)), &lenIsOne, &hashcodeCompare);
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
                BuiltinsStringStubBuilder stringBuilder(this, GetCurrentGlobalEnv());
                StringInfoGateRef leftStrInfoGate(&leftFlat);
                StringInfoGateRef rightStrInfoGate(&rightFlat);
                GateRef leftStrToInt = stringBuilder.StringAt(glue, leftStrInfoGate, Int32(0));
                GateRef rightStrToInt = stringBuilder.StringAt(glue, rightStrInfoGate, Int32(0));
                result = Equal(leftStrToInt, rightStrToInt);
                Jump(&exit);
            }
        }
    }

    Bind(&hashcodeCompare);
    Label leftNotNeg(env);
    GateRef leftHash = TryGetHashcodeFromString(left);
    GateRef rightHash = TryGetHashcodeFromString(right);
    BRANCH(Int64Equal(leftHash, Int64(-1)), &contentsCompare, &leftNotNeg);
    Bind(&leftNotNeg);
    {
        Label rightNotNeg(env);
        BRANCH(Int64Equal(rightHash, Int64(-1)), &contentsCompare, &rightNotNeg);
        Bind(&rightNotNeg);
        BRANCH(Int64Equal(leftHash, rightHash), &contentsCompare, &exit);
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

// init is -1 if leftLength  < rightLength;
//          0 if leftLength == rightLength;
//         +1 if leftLength  > rightLength.
GateRef StubBuilder::StringCompareContents(GateRef glue, GateRef left, GateRef right, GateRef init, GateRef minLength)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label loopHead(env);
    Label loopEnd(env);
    Label loopBody(env);
    Label leftFlattenFastPath(env);
    DEFVARIABLE(result, VariableType::INT32(), init);

    FlatStringStubBuilder leftFlat(this);
    leftFlat.FlattenString(glue, left, &leftFlattenFastPath);
    Bind(&leftFlattenFastPath);

    Label rightFlattenFastPath(env);
    FlatStringStubBuilder rightFlat(this);
    rightFlat.FlattenString(glue, right, &rightFlattenFastPath);
    Bind(&rightFlattenFastPath);

    StringInfoGateRef leftStrInfoGate(&leftFlat);
    StringInfoGateRef rightStrInfoGate(&rightFlat);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef isBothUtf8 = LogicAndBuilder(env).And(IsUtf8String(leftStrInfoGate.GetString()))
                                             .And(IsUtf8String(rightStrInfoGate.GetString())).Done();
    Label bothUtf8(env);
    Label slowCompare(env);
    BRANCH_LIKELY(isBothUtf8, &bothUtf8, &slowCompare);
    Bind(&bothUtf8);
    {
        GateRef leftData = GetNormalStringData(glue, leftStrInfoGate);
        GateRef rightData = GetNormalStringData(glue, rightStrInfoGate);
        Label utf8LoopHead(env);
        Label utf8LoopEnd(env);
        Label utf8LoopBody(env);
        Jump(&utf8LoopHead);
        LoopBegin(&utf8LoopHead);
        {
            BRANCH(Int32UnsignedLessThan(*i, minLength), &utf8LoopBody, &exit);
            Bind(&utf8LoopBody);
            {
                GateRef leftChar = LoadPrimitive(VariableType::INT8(), leftData, *i);
                GateRef rightChar = LoadPrimitive(VariableType::INT8(), rightData, *i);
                Label notEqual(env);
                BRANCH_NO_WEIGHT(Int8Equal(leftChar, rightChar), &utf8LoopEnd, &notEqual);
                Bind(&notEqual);
                {
                    Label leftIsLess(env);
                    Label rightIsLess(env);
                    BRANCH_NO_WEIGHT(Int32LessThan(ZExtInt8ToInt32(leftChar), ZExtInt8ToInt32(rightChar)), &leftIsLess, &rightIsLess);
                    Bind(&leftIsLess);
                    {
                        result = Int32(-1);
                        Jump(&exit);
                    }
                    Bind(&rightIsLess);
                    {
                        result = Int32(1);
                        Jump(&exit);
                    }
                }
            }
            Bind(&utf8LoopEnd);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&utf8LoopHead);
        }
    }
    Bind(&slowCompare);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32UnsignedLessThan(*i, minLength), &loopBody, &exit);
        Bind(&loopBody);
        {
            BuiltinsStringStubBuilder stringBuilder(this, GetCurrentGlobalEnv());
            GateRef leftStrToInt = stringBuilder.StringAt(glue, leftStrInfoGate, *i);
            GateRef rightStrToInt = stringBuilder.StringAt(glue, rightStrInfoGate, *i);
            Label notEqual(env);
            BRANCH_NO_WEIGHT(Int32Equal(leftStrToInt, rightStrToInt), &loopEnd, &notEqual);
            Bind(&notEqual);
            {
                Label leftIsLess(env);
                Label rightIsLess(env);
                BRANCH_NO_WEIGHT(Int32UnsignedLessThan(leftStrToInt, rightStrToInt), &leftIsLess, &rightIsLess);
                Bind(&leftIsLess);
                {
                    result = Int32(-1);
                    Jump(&exit);
                }
                Bind(&rightIsLess);
                {
                    result = Int32(1);
                    Jump(&exit);
                }
            }
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

GateRef StubBuilder::FastStringEqualWithoutRTStub(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label hashcodeCompare(env);
    Label contentsCompare(env);
    Label lenIsOne(env);

    GateRef leftLen = GetLengthFromString(left);
    GateRef rightLen = GetLengthFromString(right);
    BRANCH(Int32Equal(leftLen, rightLen), &hashcodeCompare, &exit);
    Bind(&hashcodeCompare);
    Label leftNotNeg(env);
    GateRef leftHash = TryGetHashcodeFromString(left);
    GateRef rightHash = TryGetHashcodeFromString(right);
    BRANCH(Int64Equal(leftHash, Int64(-1)), &contentsCompare, &leftNotNeg);
    Bind(&leftNotNeg);
    {
        Label rightNotNeg(env);
        BRANCH(Int64Equal(rightHash, Int64(-1)), &contentsCompare, &rightNotNeg);
        Bind(&rightNotNeg);
        BRANCH(Int64Equal(leftHash, rightHash), &contentsCompare, &exit);
    }

    Bind(&contentsCompare);
    {
        GateRef compareResult = StringCompareContents(glue, left, right, Int32(0), Int32Min(leftLen, rightLen));
        result = Equal(Int32(0), compareResult);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::StringCompare(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label compareContent(env);
    Label compareLength(env);
    GateRef leftLength = GetLengthFromString(left);
    GateRef rightLength = GetLengthFromString(right);
    DEFVARIABLE(minLength, VariableType::INT32(), leftLength);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    BRANCH_NO_WEIGHT(Int32Equal(leftLength, rightLength), &compareContent, &compareLength);
    Bind(&compareLength);
    {
        Label rightLengthIsLess(env);
        Label leftLengthIsLess(env);
        BRANCH(Int32GreaterThan(leftLength, rightLength), &rightLengthIsLess, &leftLengthIsLess);
        Bind(&rightLengthIsLess);
        {
            result = Int32(1);
            minLength = rightLength;
            Jump(&compareContent);
        }
        Bind(&leftLengthIsLess);
        {
            result = Int32(-1);
            minLength = leftLength;
            Jump(&compareContent);
        }
    }

    Bind(&compareContent);
    result = StringCompareContents(glue, left, right, *result, *minLength);
    Jump(&exit);

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
    Label leftIsNumber(env);
    Label leftIsNotNumber(env);
    Label sameVariableCheck(env);
    Label stringEqualCheck(env);
    Label stringCompare(env);
    Label updataPGOTypeWithInternString(env);
    Label bigIntEqualCheck(env);
    Label undefinedCheck(env);
    Label exit(env);
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &exit);
        Bind(&rightIsNumber);
        {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0.0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0.0));
            DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::IntType()));
            Label leftIsInt(env);
            Label leftNotInt(env);
            Label getRight(env);
            Label numberEqualCheck(env);

            BRANCH(TaggedIsInt(left), &leftIsInt, &leftNotInt);
            Bind(&leftIsInt);
            {
                doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                Jump(&getRight);
            }
            Bind(&leftNotInt);
            {
                curType = TaggedInt(PGOSampleType::DoubleType());
                doubleLeft = GetDoubleOfTDouble(left);
                Jump(&getRight);
            }
            Bind(&getRight);
            {
                Label rightIsInt(env);
                Label rightNotInt(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightNotInt);
                Bind(&rightIsInt);
                {
                    GateRef type = TaggedInt(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                    Jump(&numberEqualCheck);
                }
                Bind(&rightNotInt);
                {
                    GateRef type = TaggedInt(PGOSampleType::DoubleType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&numberEqualCheck);
                }
            }
            Bind(&numberEqualCheck);
            {
                Label doubleEqualCheck(env);
                BRANCH(BitOr(DoubleIsNAN(*doubleLeft), DoubleIsNAN(*doubleRight)), &exit, &doubleEqualCheck);
                Bind(&doubleEqualCheck);
                {
                    result = DoubleEqual(*doubleLeft, *doubleRight);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&leftIsNotNumber);
    BRANCH(TaggedIsNumber(right), &exit, &sameVariableCheck);
    Bind(&sameVariableCheck);
    {
        Label ifSameVariable(env);
        BRANCH(Equal(left, right), &ifSameVariable, &stringEqualCheck);
        Bind(&ifSameVariable);
        {
            result = True();
            if (!callback.IsEmpty()) {
                Label bothAreString(env);
                Label updataPGOTypeWithAny(env);
                BRANCH(TaggedIsString(glue, left), &bothAreString, &updataPGOTypeWithAny);
                Bind(&bothAreString);
                {
                    Label updataPGOTypeWithString(env);
                    BRANCH(IsInternalString(left), &updataPGOTypeWithInternString, &updataPGOTypeWithString);
                    Bind(&updataPGOTypeWithString);
                    {
                        callback.ProfileOpType(TaggedInt(PGOSampleType::StringType()));
                        Jump(&exit);
                    }
                }
                Bind(&updataPGOTypeWithAny);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
                    Jump(&exit);
                }
            } else {
                Jump(&exit);
            }
        }
    }
    Bind(&stringEqualCheck);
    BRANCH(BothAreString(glue, left, right), &stringCompare, &bigIntEqualCheck);
    Bind(&stringCompare);
    {
        Label executeFastStringEqual(env);
        BRANCH(LogicAndBuilder(env).And(IsInternalString(left)).And(IsInternalString(right)).Done(),
               // if not same variable and both sides are intern strings,
               // then the comparison result of the strings must be false.
               &updataPGOTypeWithInternString, &executeFastStringEqual);
        Bind(&executeFastStringEqual);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::StringType()));
            result = FastStringEqual(glue, left, right);
            Jump(&exit);
        }
    }
    Bind(&bigIntEqualCheck);
    {
        Label leftIsBigInt(env);
        Label leftIsNotBigInt(env);
        BRANCH(TaggedIsBigInt(glue, left), &leftIsBigInt, &undefinedCheck);
        Bind(&leftIsBigInt);
        {
            Label rightIsBigInt(env);
            BRANCH(TaggedIsBigInt(glue, right), &rightIsBigInt, &exit);
            Bind(&rightIsBigInt);
            callback.ProfileOpType(TaggedInt(PGOSampleType::BigIntType()));
            result = CallNGCRuntime(glue, RTSTUB_ID(BigIntEquals), { left, right });
            Jump(&exit);
        }
    }
    Bind(&updataPGOTypeWithInternString);
    {
        callback.ProfileOpType(TaggedInt(PGOSampleType::InternStringType()));
        Jump(&exit);
    }
    Bind(&undefinedCheck);
    {
        if (!callback.IsEmpty()) {
            Label updateProfileOpTypeWithAny(env);
            BRANCH(TaggedIsUndefined(left), &updateProfileOpTypeWithAny, &exit);
            Bind(&updateProfileOpTypeWithAny);
            callback.ProfileOpType(TaggedInt(PGOSampleType::UndefinedOrNullType()));
        }
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
    BRANCH(Equal(left, right), &leftEqualRight, &leftNotEqualRight);
    Bind(&leftEqualRight);
    {
        Label leftIsDouble(env);
        Label leftNotDoubleOrLeftNotNan(env);
        BRANCH(TaggedIsDouble(left), &leftIsDouble, &leftNotDoubleOrLeftNotNan);
        Bind(&leftIsDouble);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
            GateRef doubleLeft = GetDoubleOfTDouble(left);
            Label leftIsNan(env);
            Label leftIsNotNan(env);
            BRANCH(DoubleIsNAN(doubleLeft), &leftIsNan, &leftIsNotNan);
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
                BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsNotInt);
                Bind(&leftIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    Jump(&exit);
                }
                Bind(&leftIsNotInt);
                {
                    Label leftIsString(env);
                    Label leftIsNotString(env);
                    BRANCH(TaggedIsString(glue, left), &leftIsString, &leftIsNotString);
                    Bind(&leftIsString);
                    {
                        callback.ProfileOpType(TaggedInt(PGOSampleType::StringType()));
                        Jump(&exit);
                    }
                    Bind(&leftIsNotString);
                    {
                        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrLeftNotIntOrRightNotInt);
        Bind(&leftIsNumber);
        {
            Label leftIsInt(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                    result = TaggedFalse();
                    Jump(&exit);
                }
            }
        }
        Bind(&leftNotNumberOrLeftNotIntOrRightNotInt);
        {
            DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::None()));
            Label rightIsUndefinedOrNull(env);
            Label rightIsNotUndefinedOrNull(env);
            BRANCH(TaggedIsUndefinedOrNull(right), &rightIsUndefinedOrNull, &rightIsNotUndefinedOrNull);
            Bind(&rightIsUndefinedOrNull);
            {
                curType = TaggedInt(PGOSampleType::UndefinedOrNullType());
                Label leftIsHeapObject(env);
                Label leftNotHeapObject(env);
                BRANCH(TaggedIsHeapObject(left), &leftIsHeapObject, &leftNotHeapObject);
                Bind(&leftIsHeapObject);
                {
                    GateRef type = TaggedInt(PGOSampleType::HeapObjectType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    result = TaggedFalse();
                    Jump(&exit);
                }
                Bind(&leftNotHeapObject);
                {
                    Label leftIsUndefinedOrNull(env);
                    Label leftIsNotUndefinedOrNull(env);
                    // if left is undefined or null, then result is true, otherwise result is false
                    BRANCH(TaggedIsUndefinedOrNull(left), &leftIsUndefinedOrNull, &leftIsNotUndefinedOrNull);
                    Bind(&leftIsUndefinedOrNull);
                    {
                        callback.ProfileOpType(*curType);
                        result = TaggedTrue();
                        Jump(&exit);
                    }
                    Bind(&leftIsNotUndefinedOrNull);
                    {
                        callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
                        result = TaggedFalse();
                        Jump(&exit);
                    }
                }
            }
            Bind(&rightIsNotUndefinedOrNull);
            {
                Label bothString(env);
                Label eitherNotString(env);
                Label isHeapObject(env);
                Label notHeapObject(env);
                BRANCH(BitAnd(TaggedIsHeapObject(left), TaggedIsHeapObject(right)), &isHeapObject, &notHeapObject);
                Bind(&isHeapObject);
                {
                    Label typeEqual(env);
                    GateRef leftType = GetObjectType(LoadHClass(glue, left));
                    GateRef rightType = GetObjectType(LoadHClass(glue, right));
                    BRANCH(Equal(leftType, rightType), &typeEqual, &notHeapObject);
                    Bind(&typeEqual);
                    {
                        Label eitherNotString1(env);
                        Label bothBigInt(env);
                        Label eitherNotBigInt(env);
                        BRANCH(BothAreString(glue, left, right), &bothString, &eitherNotString1);
                        Bind(&eitherNotString1);
                        BRANCH(BitAnd(TaggedIsBigInt(glue, left),TaggedIsBigInt(glue, right)), &bothBigInt, &eitherNotBigInt);
                        Bind(&bothBigInt);
                        {
                            callback.ProfileOpType(TaggedInt(PGOSampleType::BigIntType()));
                            result =BooleanToTaggedBooleanPtr(CallNGCRuntime(glue,
                                                                RTSTUB_ID(BigIntEquals), {left, right}));
                            Jump(&exit);
                        }
                        Bind(&eitherNotBigInt);
                        {
                            callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
                            result = TaggedFalse();
                            Jump(&exit);
                        }
                    }
                }
                Bind(&notHeapObject);
                Label leftIsUndefinedOrNull(env);
                Label leftIsNotUndefinedOrNull(env);
                BRANCH(TaggedIsUndefinedOrNull(right), &leftIsUndefinedOrNull, &leftIsNotUndefinedOrNull);
                // If left is undefined or null, result will always be false
                // because we can ensure that right is not null here.
                Bind(&leftIsUndefinedOrNull);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
                    result = TaggedFalse();
                    Jump(&exit);
                }
                Bind(&leftIsNotUndefinedOrNull);
                {
                    Label leftIsBool(env);
                    Label leftNotBoolOrRightNotSpecial(env);
                    BRANCH(TaggedIsBoolean(left), &leftIsBool, &leftNotBoolOrRightNotSpecial);
                    Bind(&leftIsBool);
                    {
                        curType = TaggedInt(PGOSampleType::BooleanType());
                        Label rightIsSpecial(env);
                        BRANCH(TaggedIsSpecial(right), &rightIsSpecial, &leftNotBoolOrRightNotSpecial);
                        Bind(&rightIsSpecial);
                        {
                            GateRef type = TaggedInt(PGOSampleType::SpecialType());
                            COMBINE_TYPE_CALL_BACK(curType, type);
                            result = TaggedFalse();
                            Jump(&exit);
                        }
                    }
                    Bind(&leftNotBoolOrRightNotSpecial);
                    {
                        BRANCH(BothAreString(glue, left, right), &bothString, &eitherNotString);
                    }
                    Bind(&bothString);
                    {
                        callback.ProfileOpType(TaggedInt(PGOSampleType::StringType()));
                        Label stringEqual(env);
                        Label stringNotEqual(env);
                        BRANCH(FastStringEqual(glue, left, right), &stringEqual, &stringNotEqual);
                        Bind(&stringEqual);
                        result = TaggedTrue();
                        Jump(&exit);
                        Bind(&stringNotEqual);
                        result = TaggedFalse();
                        Jump(&exit);
                    }
                    Bind(&eitherNotString);
                    callback.ProfileOpType(TaggedInt(PGOSampleType::AnyType()));
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

GateRef StubBuilder::FastToBoolean(GateRef glue, GateRef value, bool flag)
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

    BRANCH(TaggedIsSpecial(value), &isSpecial, &notSpecial);
    Bind(&isSpecial);
    {
        BRANCH(TaggedIsTrue(value), &returnTrue, &returnFalse);
    }
    Bind(&notSpecial);
    {
        BRANCH(TaggedIsNumber(value), &isNumber, &notNumber);
        Bind(&notNumber);
        {
            BRANCH(IsString(glue, value), &isString, &notString);
            Bind(&isString);
            {
                auto len = GetLengthFromString(value);
                BRANCH(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            BRANCH(TaggedObjectIsBigInt(glue, value), &isBigint, &returnTrue);
            Bind(&isBigint);
            {
                auto len = LoadPrimitive(VariableType::INT32(), value, IntPtr(BigInt::LENGTH_OFFSET));
                BRANCH(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data = PtrAdd(value, IntPtr(BigInt::DATA_OFFSET));
                    auto data0 = LoadPrimitive(VariableType::INT32(), data, Int32(0));
                    BRANCH(Int32Equal(data0, Int32(0)), &returnFalse, &returnTrue);
                }
            }
        }
        Bind(&isNumber);
        {
            BRANCH(TaggedIsInt(value), &isInt, &isDouble);
            Bind(&isInt);
            {
                auto intValue = GetInt32OfTInt(value);
                BRANCH(Int32Equal(intValue, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&isDouble);
            {
                auto doubleValue = GetDoubleOfTDouble(value);
                BRANCH(DoubleIsNAN(doubleValue), &returnFalse, &notNan);
                Bind(&notNan);
                BRANCH(DoubleEqual(doubleValue, Double(0.0)), &returnFalse, &returnTrue);
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

GateRef StubBuilder::FastToBooleanBaseline(GateRef glue, GateRef value, bool flag)
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
            Branch(IsString(glue, value), &isString, &notString);
            Bind(&isString);
            {
                auto len = GetLengthFromString(value);
                Branch(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            Branch(TaggedObjectIsBigInt(glue, value), &isBigint, &returnTrue);
            Bind(&isBigint);
            {
                auto len = LoadPrimitive(VariableType::INT32(), value, IntPtr(BigInt::LENGTH_OFFSET));
                Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data = PtrAdd(value, IntPtr(BigInt::DATA_OFFSET));
                    auto data0 = LoadPrimitive(VariableType::INT32(), data, Int32(0));
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

GateRef StubBuilder::FastToBooleanWithProfile(GateRef glue, GateRef value, ProfileOperation callback, bool flag)
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
    Label isTrue(env);
    Label isNotTrue(env);
    Label isFalse(env);
    Label isNotFalse(env);
    Label isUndefinedOrNull(env);
    Label isHole(env);
    Label isNotBigInt(env);

    BRANCH(TaggedIsSpecial(value), &isSpecial, &notSpecial);
    Bind(&isSpecial);
    {
        BRANCH(TaggedIsTrue(value), &isTrue, &isNotTrue);
        Bind(&isTrue);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::BooleanType()));
            Jump(&returnTrue);
        }
        Bind(&isNotTrue);
        {
            BRANCH(TaggedIsFalse(value), &isFalse, &isNotFalse);
            Bind(&isFalse);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::BooleanType()));
                Jump(&returnFalse);
            }
            Bind(&isNotFalse);
            BRANCH(TaggedIsUndefinedOrNull(value), &isUndefinedOrNull, &isHole);
            Bind(&isUndefinedOrNull);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::UndefinedOrNullType()));
                Jump(&returnFalse);
            }
            Bind(&isHole);
            {
                // value will never be hole, this branch shouldn't be entered
                callback.ProfileOpType(TaggedInt(PGOSampleType::SpecialType()));
                Jump(&returnFalse);
            }
        }
    }
    Bind(&notSpecial);
    {
        BRANCH(TaggedIsNumber(value), &isNumber, &notNumber);
        Bind(&notNumber);
        {
            BRANCH(IsString(glue, value), &isString, &notString);
            Bind(&isString);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::StringType()));
                auto len = GetLengthFromString(value);
                BRANCH(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            BRANCH(TaggedObjectIsBigInt(glue, value), &isBigint, &isNotBigInt);
            Bind(&isBigint);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::BigIntType()));
                auto len = LoadPrimitive(VariableType::INT32(), value, IntPtr(BigInt::LENGTH_OFFSET));
                BRANCH(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data = PtrAdd(value, IntPtr(BigInt::DATA_OFFSET));
                    auto data0 = LoadPrimitive(VariableType::INT32(), data, Int32(0));
                    BRANCH(Int32Equal(data0, Int32(0)), &returnFalse, &returnTrue);
                }
            }
            Bind(&isNotBigInt);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::HeapObjectType()));
                Jump(&returnTrue);
            }
        }
        Bind(&isNumber);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
            BRANCH(TaggedIsInt(value), &isInt, &isDouble);
            Bind(&isInt);
            {
                auto intValue = GetInt32OfTInt(value);
                BRANCH(Int32Equal(intValue, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&isDouble);
            {
                auto doubleValue = GetDoubleOfTDouble(value);
                BRANCH(DoubleIsNAN(doubleValue), &returnFalse, &notNan);
                Bind(&notNan);
                BRANCH(DoubleEqual(doubleValue, Double(0.0)), &returnFalse, &returnTrue);
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

GateRef StubBuilder::FastToBooleanWithProfileBaseline(GateRef glue, GateRef value, ProfileOperation callback, bool flag)
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
    Label isTrue(env);
    Label isNotTrue(env);
    Label isFalse(env);
    Label isNotFalse(env);
    Label isUndefinedOrNull(env);

    Branch(TaggedIsSpecial(value), &isSpecial, &notSpecial);
    Bind(&isSpecial);
    {
        Branch(TaggedIsTrue(value), &isTrue, &isNotTrue);
        Bind(&isTrue);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::BooleanType()));
            Jump(&returnTrue);
        }
        Bind(&isNotTrue);
        {
            BRANCH(TaggedIsFalse(value), &isFalse, &isNotFalse);
            Bind(&isFalse);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::BooleanType()));
                Jump(&returnFalse);
            }
            Bind(&isNotFalse);
            BRANCH(TaggedIsUndefinedOrNull(value), &isUndefinedOrNull, &returnFalse);
            Bind(&isUndefinedOrNull);
            {
                callback.ProfileOpType(TaggedInt(PGOSampleType::UndefinedOrNullType()));
                Jump(&returnFalse);
            }
        }
    }
    Bind(&notSpecial);
    {
        Branch(TaggedIsNumber(value), &isNumber, &notNumber);
        Bind(&notNumber);
        {
            Branch(IsString(glue, value), &isString, &notString);
            Bind(&isString);
            {
                auto len = GetLengthFromString(value);
                Branch(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            Branch(TaggedObjectIsBigInt(glue, value), &isBigint, &returnTrue);
            Bind(&isBigint);
            {
                auto len = LoadPrimitive(VariableType::INT32(), value, IntPtr(BigInt::LENGTH_OFFSET));
                Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data = PtrAdd(value, IntPtr(BigInt::DATA_OFFSET));
                    auto data0 = LoadPrimitive(VariableType::INT32(), data, Int32(0));
                    Branch(Int32Equal(data0, Int32(0)), &returnFalse, &returnTrue);
                }
            }
        }
        Bind(&isNumber);
        {
            callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
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
    DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::None()));
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Label leftIsNumberAndRightIsNumber(env);
    Label leftIsDoubleAndRightIsDouble(env);
    Label exit(env);
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftNotInt(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftNotInt);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label bailout(env);
                BRANCH(TaggedIsInt(right), &rightIsInt, &bailout);
                Bind(&rightIsInt);
                {
                    result = FastIntDiv(left, right, &bailout, callback);
                    Jump(&exit);
                }
                Bind(&bailout);
                {
                    curType = TaggedInt(PGOSampleType::IntOverFlowType());
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    Jump(&leftIsNumberAndRightIsNumber);
                }
            }
            Bind(&leftNotInt);
            {
                curType = TaggedInt(PGOSampleType::DoubleType());
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
        BRANCH(TaggedIsInt(right), &rightIsInt, &rightNotInt);
        Bind(&rightIsInt);
        {
            GateRef type = TaggedInt(PGOSampleType::IntType());
            COMBINE_TYPE_CALL_BACK(curType, type);
            doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
            Jump(&leftIsDoubleAndRightIsDouble);
        }
        Bind(&rightNotInt);
        {
            GateRef type = TaggedInt(PGOSampleType::DoubleType());
            COMBINE_TYPE_CALL_BACK(curType, type);
            doubleRight = GetDoubleOfTDouble(right);
            Jump(&leftIsDoubleAndRightIsDouble);
        }
    }
    Bind(&leftIsDoubleAndRightIsDouble);
    {
        Label rightIsZero(env);
        Label rightNotZero(env);
        BRANCH(DoubleEqual(*doubleRight, Double(0.0)), &rightIsZero, &rightNotZero);
        Bind(&rightIsZero);
        {
            Label leftIsZero(env);
            Label leftNotZero(env);
            Label leftIsZeroOrNan(env);
            Label leftNotZeroAndNotNan(env);
            BRANCH(DoubleEqual(*doubleLeft, Double(0.0)), &leftIsZero, &leftNotZero);
            Bind(&leftIsZero);
            {
                Jump(&leftIsZeroOrNan);
            }
            Bind(&leftNotZero);
            {
                Label leftIsNan(env);
                BRANCH(DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
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

GateRef StubBuilder::NumberOperation(Environment *env, GateRef left, GateRef right,
                                     const BinaryOperation& intOp,
                                     const BinaryOperation& floatOp,
                                     ProfileOperation callback)
{
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
    Label exit(env);
    Label doFloatOp(env);
    Label doIntOp(env);
    Label leftIsNumber(env);
    Label leftIsIntRightIsDouble(env);
    Label rightIsDouble(env);
    Label rightIsInt(env);
    Label rightIsNumber(env);
    BRANCH(TaggedIsNumber(left), &leftIsNumber, &exit);
    Bind(&leftIsNumber);
    {
        BRANCH(TaggedIsNumber(right), &rightIsNumber, &exit);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            BRANCH(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                BRANCH(TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
                Bind(&leftIsIntRightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&doFloatOp);
                }
            }
            Bind(&leftIsDouble);
            {
                BRANCH(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::NumberType()));
                    doubleLeft = GetDoubleOfTDouble(left);
                    doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                    Jump(&doFloatOp);
                }
                Bind(&rightIsDouble);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                    doubleLeft = GetDoubleOfTDouble(left);
                    doubleRight = GetDoubleOfTDouble(right);
                    Jump(&doFloatOp);
                }
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

GateRef StubBuilder::TryStringAdd(Environment *env, GateRef glue, GateRef left, GateRef right,
                                  const BinaryOperation& intOp,
                                  const BinaryOperation& floatOp,
                                  ProfileOperation callback)
{
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label leftIsNotSpecial(env);
    Label leftIsNotString(env);
    Label leftIsString(env);
    Label rightIsNotSpecial(env);
    Label rightIsNotString(env);
    Label rightIsString(env);
    Label stringLeftAddNumberRight(env);
    Label numberLeftAddStringRight(env);
    Label stringLeftAddStringRight(env);
    Label notStringAdd(env);
    BRANCH(TaggedIsString(glue, left), &leftIsString, &leftIsNotString);
    Bind(&leftIsString);
    {
        BRANCH(TaggedIsString(glue, right), &stringLeftAddStringRight, &rightIsNotString);
        Bind(&rightIsNotString);
        {
            BRANCH(TaggedIsSpecial(right), &notStringAdd, &rightIsNotSpecial);
            Bind(&rightIsNotSpecial);
            {
                BRANCH(TaggedIsNumber(right), &stringLeftAddNumberRight, &notStringAdd);
            }
        }
    }
    Bind(&leftIsNotString);
    {
        BRANCH(TaggedIsString(glue, right), &rightIsString, &notStringAdd);
        Bind(&rightIsString);
        {
            BRANCH(TaggedIsSpecial(left), &notStringAdd, &leftIsNotSpecial);
            Bind(&leftIsNotSpecial);
            {
                BRANCH(TaggedIsNumber(left), &numberLeftAddStringRight, &notStringAdd);
            }
        }
    }
    Bind(&stringLeftAddNumberRight);
    {
        Label hasPendingException(env);
        // NOTICE-PGO: support string and number
        callback.ProfileOpType(TaggedInt(PGOSampleType::NumberOrStringType()));
        BuiltinsStringStubBuilder builtinsStringStubBuilder(this, GetCurrentGlobalEnv());
        result = builtinsStringStubBuilder.StringConcat(glue, left, NumberToString(glue, right));
        BRANCH(HasPendingException(glue), &hasPendingException, &exit);
        Bind(&hasPendingException);
        result = Exception();
        Jump(&exit);
    }
    Bind(&numberLeftAddStringRight);
    {
        Label hasPendingException(env);
        // NOTICE-PGO: support string and number
        callback.ProfileOpType(TaggedInt(PGOSampleType::NumberOrStringType()));
        BuiltinsStringStubBuilder builtinsStringStubBuilder(this, GetCurrentGlobalEnv());
        result = builtinsStringStubBuilder.StringConcat(glue, NumberToString(glue, left), right);
        BRANCH(HasPendingException(glue), &hasPendingException, &exit);
        Bind(&hasPendingException);
        result = Exception();
        Jump(&exit);
    }
    Bind(&stringLeftAddStringRight);
    {
        Label hasPendingException(env);
        callback.ProfileOpType(TaggedInt(PGOSampleType::StringType()));
        BuiltinsStringStubBuilder builtinsStringStubBuilder(this, GetCurrentGlobalEnv());
        result = builtinsStringStubBuilder.StringConcat(glue, left, right);
        BRANCH(HasPendingException(glue), &hasPendingException, &exit);
        Bind(&hasPendingException);
        result = Exception();
        Jump(&exit);
    }
    Bind(&notStringAdd);
    {
        result = NumberOperation(env, left, right, intOp, floatOp, callback);
        Jump(&exit);
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

    if (Op == OpCode::ADD) { // Try string Add
        result = TryStringAdd(env, glue, left, right, intOp, floatOp, callback);
    } else {
        result = NumberOperation(env, left, right, intOp, floatOp, callback);
    }
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
        BRANCH(condition, &overflow, &notOverflow);
        Bind(&overflow);
        {
            auto doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
            auto doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
            auto ret = BinaryOp<Op, MachineType::F64>(doubleLeft, doubleRight);
            result = DoubleToTaggedDoublePtr(ret);
            callback.ProfileOpType(TaggedInt(PGOSampleType::IntOverFlowType()));
            Jump(&exit);
        }
        Bind(&notOverflow);
        {
            res = env->GetBuilder()->ExtractValue(MachineType::I32, res, Int32(0));
            if (Op == OpCode::MUL) {
                Label resultIsZero(env);
                Label returnNegativeZero(env);
                Label returnResult(env);
                BRANCH(Int32Equal(res, Int32(0)), &resultIsZero, &returnResult);
                Bind(&resultIsZero);
                BRANCH(LogicOrBuilder(env).Or(Int32LessThan(GetInt32OfTInt(left), Int32(0)))
                    .Or(Int32LessThan(GetInt32OfTInt(right), Int32(0))).Done(),
                    &returnNegativeZero, &returnResult);
                Bind(&returnNegativeZero);
                result = DoubleToTaggedDoublePtr(Double(-0.0));
                callback.ProfileOpType(TaggedInt(PGOSampleType::DoubleType()));
                Jump(&exit);
                Bind(&returnResult);
                result = IntToTaggedPtr(res);
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
                Jump(&exit);
            } else {
                result = IntToTaggedPtr(res);
                callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
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
    BRANCH(Int32Equal(intRight, Int32(0)), bailout, &rightIsNotZero);
    Bind(&rightIsNotZero);
    BRANCH(Int32Equal(intLeft, Int32(INT_MIN)), &leftIsIntMin, &leftAndRightIsNotBoundary);
    Bind(&leftIsIntMin);
    BRANCH(Int32Equal(intRight, Int32(-1)), bailout, &leftAndRightIsNotBoundary);
    Bind(&leftAndRightIsNotBoundary);
    {
        Label leftIsZero(env);
        Label leftIsNotZero(env);
        BRANCH(Int32Equal(intLeft, Int32(0)), &leftIsZero, &leftIsNotZero);
        Bind(&leftIsZero);
        {
            BRANCH(Int32LessThan(intRight, Int32(0)), bailout, &leftIsNotZero);
        }
        Bind(&leftIsNotZero);
        {
            intResult = Int32Div(intLeft, intRight);
            GateRef truncated = Int32Mul(*intResult, intRight);
            BRANCH(Equal(intLeft, truncated), &exit, bailout);
        }
    }
    Bind(&exit);
    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
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
    BRANCH(TaggedIsInt(left), &leftIsInt, &leftNotIntOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        BRANCH(TaggedIsInt(right), &rightIsInt, &leftNotIntOrRightNotInt);
        Bind(&rightIsInt);
        {
            intLeft = GetInt32OfTInt(left);
            intRight = GetInt32OfTInt(right);
            Label leftGreaterZero(env);
            BRANCH(Int32GreaterThanOrEqual(*intLeft, Int32(0)), &leftGreaterZero, &leftNotIntOrRightNotInt);
            Bind(&leftGreaterZero);
            {
                Label rightGreaterZero(env);
                BRANCH(Int32GreaterThan(*intRight, Int32(0)), &rightGreaterZero, &leftNotIntOrRightNotInt);
                Bind(&rightGreaterZero);
                {
                    callback.ProfileOpType(TaggedInt(PGOSampleType::IntType()));
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
        DEFVARIABLE(curType, VariableType::INT64(), TaggedInt(PGOSampleType::None()));
        // less than 0 result should be double
        curType = TaggedInt(PGOSampleType::DoubleType());
        BRANCH(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            BRANCH(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
            Bind(&rightIsNumber);
            {
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                BRANCH(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::IntType());
                    COMBINE_TYPE_CALL_BACK(curType, type);
                    doubleLeft = ChangeInt32ToFloat64(GetInt32OfTInt(left));
                    Jump(&leftIsNumberAndRightIsNumber);
                }
                Bind(&leftNotInt1);
                {
                    GateRef type = TaggedInt(PGOSampleType::DoubleType());
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
            BRANCH(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
            Bind(&rightIsInt1);
            {
                GateRef type = TaggedInt(PGOSampleType::IntType());
                COMBINE_TYPE_CALL_BACK(curType, type);
                doubleRight = ChangeInt32ToFloat64(GetInt32OfTInt(right));
                Jump(&leftIsDoubleAndRightIsDouble);
            }
            Bind(&rightNotInt1);
            {
                GateRef type = TaggedInt(PGOSampleType::DoubleType());
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
            BRANCH(DoubleEqual(*doubleRight, Double(0.0)), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotZero);
            Bind(&rightNotZero);
            {
                Label rightNotNan(env);
                BRANCH(DoubleIsNAN(*doubleRight), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotNan);
                Bind(&rightNotNan);
                {
                    Label leftNotNan(env);
                    BRANCH(DoubleIsNAN(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf, &leftNotNan);
                    Bind(&leftNotNan);
                    {
                        BRANCH(DoubleIsINF(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf,
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
                BRANCH(DoubleEqual(*doubleLeft, Double(0.0)), &leftIsZeroOrRightIsInf, &leftNotZero);
                Bind(&leftNotZero);
                {
                    Label rightNotInf(env);
                    BRANCH(DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
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
    GateRef properties = GetPropertiesFromJSObject(glue, receiver);
    GateRef entry = FindEntryFromHashTable<NameDictionary>(glue, properties, key);
    Label notNegtiveOne(env);
    Label exit(env);
    BRANCH(Int32NotEqual(entry, Int32(-1)), &notNegtiveOne, &exit);
    Bind(&notNegtiveOne);
    {
        result = GetValueFromGlobalDictionary(glue, properties, entry);
        Label callGetter(env);
        BRANCH(TaggedIsAccessor(glue, *result), &callGetter, &exit);
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

GateRef StubBuilder::GetConstPoolFromFunction(GateRef glue, GateRef jsFunc)
{
    return env_->GetBuilder()->GetConstPoolFromFunction(glue, jsFunc);
}

GateRef StubBuilder::GetSharedConstpoolFromMethod(GateRef glue, GateRef method)
{
    return env_->GetBuilder()->GetSharedConstpoolFromMethod(glue, method);
}

GateRef StubBuilder::GetStringFromConstPool(GateRef glue, GateRef constpool, GateRef index)
{
    GateRef module = Circuit::NullGate();
    GateRef hirGate = Circuit::NullGate();
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, Circuit::NullGate(), module, index,
                                                      ConstPoolType::STRING);
}

GateRef StubBuilder::GetMethodFromConstPool(GateRef glue, GateRef constpool, GateRef index)
{
    GateRef module = Circuit::NullGate();
    GateRef hirGate = Circuit::NullGate();
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, Circuit::NullGate(), module, index,
                                                      ConstPoolType::METHOD);
}

GateRef StubBuilder::GetArrayLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module)
{
    GateRef hirGate = Circuit::NullGate();
    GateRef unsharedConstPool = env_->GetBuilder()->GetUnsharedConstpoolFromGlue(glue, constpool);
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, unsharedConstPool, module, index,
                                                      ConstPoolType::ARRAY_LITERAL);
}

GateRef StubBuilder::GetObjectLiteralFromConstPool(GateRef glue, GateRef constpool, GateRef index, GateRef module)
{
    GateRef hirGate = Circuit::NullGate();
    GateRef unsharedConstPool = env_->GetBuilder()->GetUnsharedConstpoolFromGlue(glue, constpool);
    return env_->GetBuilder()->GetObjectFromConstPool(glue, hirGate, constpool, unsharedConstPool, module, index,
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
    GateRef length = GetInt32OfTInt(LoadPrimitive(VariableType::INT64(), receiver, lengthOffset));
    Label isVailedIndex(env);
    Label notValidIndex(env);
    BRANCH(BitAnd(Int32GreaterThanOrEqual(index, Int32(0)),
        Int32UnsignedLessThan(index, length)), &isVailedIndex, &notValidIndex);
    Bind(&isVailedIndex);
    {
        GateRef elements = GetElementsArray(glue, receiver);
        result = GetValueFromTaggedArray(glue, elements, index);
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

GateRef StubBuilder::GetEnumCacheKindFromEnumCache(GateRef enumCache)
{
    return env_->GetBuilder()->GetEnumCacheKindFromEnumCache(enumCache);
}

GateRef StubBuilder::GetEnumCacheOwnFromEnumCache(GateRef glue, GateRef enumCache)
{
    return env_->GetBuilder()->GetEnumCacheOwnFromEnumCache(glue, enumCache);
}

GateRef StubBuilder::GetEnumCacheAllFromEnumCache(GateRef glue, GateRef enumCache)
{
    return env_->GetBuilder()->GetEnumCacheAllFromEnumCache(glue, enumCache);
}

GateRef StubBuilder::GetProtoChainInfoEnumCacheFromEnumCache(GateRef glue, GateRef enumCache)
{
    return env_->GetBuilder()->GetProtoChainInfoEnumCacheFromEnumCache(glue, enumCache);
}

GateRef StubBuilder::IsEnumCacheValid(GateRef glue, GateRef receiver, GateRef cachedHclass, GateRef kind)
{
    return env_->GetBuilder()->IsEnumCacheValid(glue, receiver, cachedHclass, kind);
}

GateRef StubBuilder::NeedCheckProperty(GateRef glue, GateRef receiver)
{
    return env_->GetBuilder()->NeedCheckProperty(glue, receiver);
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
    BRANCH(Int32GreaterThanOrEqual(index, length), &exit, &notFinish);
    Bind(&notFinish);
    GateRef keys = GetKeysFromForInIterator(glue, iter);
    GateRef receiver = GetObjectFromForInIterator(glue, iter);
    GateRef cachedHclass = GetCachedHClassFromForInIterator(glue, iter);
    GateRef kind = GetCacheKindFromForInIterator(iter);
    BRANCH(IsEnumCacheValid(glue, receiver, cachedHclass, kind), &fastGetKey, &notEnumCacheValid);
    Bind(&notEnumCacheValid);
    BRANCH(NeedCheckProperty(glue, receiver), &slowpath, &fastGetKey);
    Bind(&fastGetKey);
    {
        result = GetValueFromTaggedArray(glue, keys, index);
        IncreaseIteratorIndex(glue, iter, index);
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

GateRef StubBuilder::GetOrCreateEnumCacheFromHClass(GateRef glue, GateRef hClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label isEnumCache(env);
    Label notEnumCache(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef enumCache = GetEnumCacheFromHClass(glue, hClass);
    BRANCH(TaggedIsEnumCache(glue, enumCache), &isEnumCache, &notEnumCache);
    Bind(&isEnumCache);
    {
        result = enumCache;
        Jump(&exit);
    }
    Bind(&notEnumCache);
    {
        NewObjectStubBuilder newBuilder(this);
        GateRef enumCacheNew = newBuilder.NewEnumCache(glue);
        SetEnumCacheToHClass(VariableType::JS_POINTER(), glue, hClass, enumCacheNew);
        result = enumCacheNew;
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

    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef func = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, index);
    GateRef protoOrHclass = Load(VariableType::JS_ANY(), glue, func, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    result = protoOrHclass;
    BRANCH(TaggedIsHeapObject(protoOrHclass), &isHeapObject, &exit);
    Bind(&isHeapObject);
    BRANCH(IsJSHClass(glue, protoOrHclass), &isJSHclass, &exit);
    Bind(&isJSHclass);
    {
        result = GetPrototypeFromHClass(glue, protoOrHclass);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
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
    GateRef object = ToObject(glue, GetCurrentGlobalEnv(), obj);
    BRANCH(TaggedIsException(object), &exit, &isNotExceptiont);
    Bind(&isNotExceptiont);
    {
        Label deleteProper(env);
#if ENABLE_NEXT_OPTIMIZATION
        key = ToPropertyKey(glue, value);
#else
        key = CallRuntime(glue, RTSTUB_ID(ToPropertyKey), {value});
#endif
        BRANCH(HasPendingException(glue), &exit, &deleteProper);
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
    BRANCH(TaggedIsRegularObject(glue, obj), &regularJSObjDeletePrototype, &notRegularJSObject);
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
    Label isNotEcmaObject(env);
    Label isNumber(env);
    Label notNumber(env);
    Label isBoolean(env);
    Label notBoolean(env);
    Label isString(env);
    Label notString(env);
    Label isSymbol(env);
    Label notSymbol(env);
    Label isBigInt(env);
    BRANCH(IsEcmaObject(glue, obj), &exit, &isNotEcmaObject);
    Bind(&isNotEcmaObject);
    BRANCH(TaggedIsNumber(obj), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::NUMBER_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notNumber);
    BRANCH(TaggedIsBoolean(obj), &isBoolean, &notBoolean);
    Bind(&isBoolean);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::BOOLEAN_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notBoolean);
    BRANCH(TaggedIsString(glue, obj), &isString, &notString);
    Bind(&isString);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::STRING_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notString);
    BRANCH(TaggedIsSymbol(glue, obj), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        result = GetFunctionPrototype(glue, GlobalEnv::SYMBOL_FUNCTION_INDEX);
        Jump(&exit);
    }
    Bind(&notSymbol);
    BRANCH(TaggedIsBigInt(glue, obj), &isBigInt, &exit);
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

GateRef StubBuilder::ToPropertyKey(GateRef glue, GateRef tagged)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label notPrimitive(env);
    Label hasPendingException(env);
    Label checkSymbol(env);
    Label castKey(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), tagged);

    BRANCH(TaggedIsPropertyKey(glue, tagged), &exit, &notPrimitive);

    Bind(&notPrimitive);
    {
        result = ToPrimitive(glue, tagged, PreferredPrimitiveType::PREFER_STRING);
        BRANCH(HasPendingException(glue), &hasPendingException, &checkSymbol);
    }

    Bind(&checkSymbol);
    {
        BRANCH(TaggedIsSymbol(glue, *result), &exit, &castKey);
    }

    Bind(&castKey);
    {
        result = JSTaggedValueToString(glue, *result);
        BRANCH(HasPendingException(glue), &hasPendingException, &exit);
    }

    Bind(&hasPendingException);
    {
        result = Exception();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::TaggedIsPropertyKey(GateRef glue, GateRef obj)
{
    return LogicOrBuilder(env_).Or(TaggedIsStringOrSymbol(glue, obj)).Or(TaggedIsNumber(obj)).Done();
}

// JSTaggedValue::HasProperty (O, P)
GateRef StubBuilder::HasProperty(GateRef glue, GateRef obj, GateRef key, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label slowpath(env);
    Label isJSProxy(env);
    Label notJSProxy(env);
    Label isTypedArray(env);
    Label notTypedArray(env);
    Label isModuleNamespace(env);
    Label notModuleNamespace(env);
    Label isSpecialContainer(env);
    Label defaultObj(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());

    BRANCH(IsJsProxy(glue, obj), &isJSProxy, &notJSProxy);
    Bind(&isJSProxy);
    {
        result = CallRuntime(glue, RTSTUB_ID(JSProxyHasProperty), {obj, key});
        Jump(&exit);
    }

    Bind(&notJSProxy);
    BRANCH(BitOr(IsTypedArray(glue, obj), IsSharedTypedArray(glue, obj)), &isTypedArray, &notTypedArray);
    Bind(&isTypedArray);
    {
        result = CallRuntime(glue, RTSTUB_ID(JSTypedArrayHasProperty), {obj, key});
        Jump(&exit);
    }

    Bind(&notTypedArray);
    BRANCH(IsModuleNamespace(glue, obj), &isModuleNamespace, &notModuleNamespace);
    Bind(&isModuleNamespace);
    {
        result = CallRuntime(glue, RTSTUB_ID(ModuleNamespaceHasProperty), {obj, key});
        Jump(&exit);
    }

    Bind(&notModuleNamespace);
    BRANCH(IsSpecialContainer(glue, obj), &isSpecialContainer, &defaultObj);
    Bind(&isSpecialContainer);
    {
        Jump(&slowpath);
    }

    Bind(&defaultObj);
    {
        result = JSObjectHasProperty(glue, obj, key, hir);
        Jump(&exit);
    }

    Bind(&slowpath);
    {
        result = CallRuntime(glue, RTSTUB_ID(HasProperty), {obj, key});
        Jump(&exit);
    }

    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef StubBuilder::IsIn(GateRef glue, GateRef prop, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Label checkProperty(env);
    Label isPendingException(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
    DEFVARIABLE(propKey, VariableType::JS_ANY(), Undefined());

    BRANCH(IsEcmaObject(glue, obj), &isEcmaObject, &notEcmaObject);

    Bind(&notEcmaObject);
    {
        auto taggedId = Int32(GET_MESSAGE_STRING_ID(InOperatorOnNonObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
        result = Exception();
        Jump(&exit);
    }

    Bind(&isEcmaObject);
    {
        propKey = ToPropertyKey(glue, prop);
        BRANCH(HasPendingException(glue), &isPendingException, &checkProperty);
    }

    Bind(&checkProperty);
    {
        result = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                { glue, obj, *propKey, GetCurrentGlobalEnv() });
        BRANCH(HasPendingException(glue), &isPendingException, &exit);
    }

    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::IsSpecialKeysObject(GateRef glue, GateRef obj)
{
    return LogicOrBuilder(env_).Or(IsTypedArray(glue, obj)).Or(IsModuleNamespace(glue, obj)).Or(IsSpecialContainer(glue, obj)).Done();
}

GateRef StubBuilder::IsSlowKeysObject(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());

    Label isHeapObject(env);
    BRANCH(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = LogicOrBuilder(env).Or(IsJSGlobalObject(glue, obj)).Or(IsJsProxy(glue, obj)).Or(IsSpecialKeysObject(glue, obj)).Done();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetNumberOfElements(GateRef glue, GateRef obj)
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

    BRANCH(IsJSPrimitiveRef(glue, obj), &isJSPrimitiveRef, &notPrimitiveString);
    Bind(&isJSPrimitiveRef);
    GateRef value = Load(VariableType::JS_ANY(), glue, obj, IntPtr(JSPrimitiveRef::VALUE_OFFSET));
    BRANCH(TaggedIsString(glue, value), &isPrimitiveString, &notPrimitiveString);
    Bind(&isPrimitiveString);
    {
        numOfElements = GetLengthFromString(value);
        Jump(&notPrimitiveString);
    }
    Bind(&notPrimitiveString);
    GateRef elements = GetElementsArray(glue, obj);
    BRANCH(IsDictionaryMode(glue, elements), &isDictMode, &notDictMode);
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
            BRANCH(Int32UnsignedLessThan(*i, elementsLen), &iLessLength, &exit);
            Bind(&iLessLength);
            {
                GateRef element = GetTaggedValueWithElementsKind(glue, obj, *i);
                BRANCH(TaggedIsHole(element), &loopEnd, &notHole);
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
            GetValueFromTaggedArray(glue, elements, Int32(TaggedHashTable<NumberDictionary>::NUMBER_OF_ENTRIES_INDEX)));
        numOfElements = Int32Add(*numOfElements, entryCount);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *numOfElements;
    env->SubCfgExit();
    return ret;
}

// When receiver and receiver's prototype chain have no elements
// and ProtoChainInfoEnumCache of receiver's proto == JSTaggedValue::Undefined,
// the SimpleEnumCache valid.
GateRef StubBuilder::IsSimpleEnumCacheValid(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    DEFVARIABLE(current, VariableType::JS_ANY(), Undefined());

    Label receiverHasNoElements(env);
    Label protoIsHeapObject(env);
    Label afterLoop(env);
    Label isProtoEnumCache(env);
    Label isKeyUndefined(env);

    // Check no elements on self.
    GateRef numOfElements = GetNumberOfElements(glue, obj);
    BRANCH(Int32GreaterThan(numOfElements, Int32(0)), &exit, &receiverHasNoElements);

    Bind(&receiverHasNoElements);
    // Since current isn't a heapObject, the receiver's proto chain has no keys.
    current = GetPrototypeFromHClass(glue, LoadHClass(glue, obj));
    BRANCH_LIKELY(TaggedIsHeapObject(*current), &protoIsHeapObject, &afterLoop);

    // Check protoChainInfoEnumCache of receiver's proto == JSTaggedValue::Undefined.
    Bind(&protoIsHeapObject);
    {
        GateRef protoHClass = LoadHClass(glue, *current);
        GateRef enumCacheProto = GetEnumCacheFromHClass(glue, protoHClass);
        BRANCH(TaggedIsEnumCache(glue, enumCacheProto), &isProtoEnumCache, &exit);
        Bind(&isProtoEnumCache);
        GateRef keyProto = GetProtoChainInfoEnumCacheFromEnumCache(glue, enumCacheProto);
        BRANCH(TaggedIsUndefined(keyProto), &isKeyUndefined, &exit);
    }

    // Check no elements on prototype chain.
    Bind(&isKeyUndefined);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label currentHasNoElements(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            GateRef numOfCurrentElements = GetNumberOfElements(glue, *current);
            BRANCH(Int32GreaterThan(numOfCurrentElements, Int32(0)), &exit, &currentHasNoElements);
            Bind(&currentHasNoElements);
            GateRef hclass = LoadHClass(glue, *current);
            current = GetPrototypeFromHClass(glue, hclass);
            BRANCH(TaggedIsHeapObject(*current), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
    }

    Bind(&afterLoop);
    {
        result = True();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// When receiver and receiver's prototype chain have no elements,
// and receiver.proto.EnumCacheAll == receiver.ProtoChainInfoEnumCache,
// the ProtoChainEnumCache valid.
GateRef StubBuilder::IsProtoChainCacheValid(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    DEFVARIABLE(current, VariableType::JS_ANY(), Undefined());

    Label receiverHasNoElements(env);
    Label prototypeIsHeapObj(env);
    Label isEnumCache(env);
    Label isCacheEqual(env);

    // Check no elements on self.
    GateRef numOfElements = GetNumberOfElements(glue, obj);
    BRANCH(Int32GreaterThan(numOfElements, Int32(0)), &exit, &receiverHasNoElements);

    Bind(&receiverHasNoElements);
    GateRef hClass = LoadHClass(glue, obj);
    GateRef prototype = GetPrototypeFromHClass(glue, hClass);
    BRANCH(TaggedIsHeapObject(prototype), &prototypeIsHeapObj, &exit);

    // Check receiver.proto.EnumCacheAll == receiver.ProtoChainInfoEnumCache.
    Bind(&prototypeIsHeapObj);
    GateRef protoHClass = LoadHClass(glue, prototype);
    GateRef enumCacheOwn = GetEnumCacheFromHClass(glue, hClass);
    GateRef enumCacheProto = GetEnumCacheFromHClass(glue, protoHClass);
    GateRef checkEnumCache = LogicAndBuilder(env)
                             .And(TaggedIsEnumCache(glue, enumCacheOwn))
                             .And(TaggedIsEnumCache(glue, enumCacheProto))
                             .Done();
    BRANCH(checkEnumCache, &isEnumCache, &exit);
    Bind(&isEnumCache);
    GateRef keyOwn = GetProtoChainInfoEnumCacheFromEnumCache(glue, enumCacheOwn);
    GateRef keyProto = GetEnumCacheAllFromEnumCache(glue, enumCacheProto);
    GateRef checkKey = LogicAndBuilder(env)
                       .And(Equal(keyOwn, keyProto))
                       .And(TaggedIsNotNull(keyOwn))
                       .Done();
    BRANCH(checkKey, &isCacheEqual, &exit);

    // Check no elements on prototype chain.
    Bind(&isCacheEqual);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Label currentHasNoElements(env);
        current = prototype;
        BRANCH(TaggedIsHeapObject(*current), &loopHead, &afterLoop);
        LoopBegin(&loopHead);
        {
            GateRef numOfCurrentElements = GetNumberOfElements(glue, *current);
            BRANCH(Int32GreaterThan(numOfCurrentElements, Int32(0)), &exit, &currentHasNoElements);
            Bind(&currentHasNoElements);
            current = GetPrototypeFromHClass(glue, LoadHClass(glue, *current));
            BRANCH(TaggedIsHeapObject(*current), &loopEnd, &afterLoop);
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

    Label isEnumCache(env);
    Label notSlowKeys(env);
    Label notDictionaryMode(env);
    Label checkSimpleEnumCache(env);
    Label notSimpleEnumCache(env);
    Label checkEnumCacheWithProtoChainInfo(env);
    Label enumCacheValid(env);

    BRANCH(IsSlowKeysObject(glue, obj), &exit, &notSlowKeys);
    Bind(&notSlowKeys);
    GateRef hclass = LoadHClass(glue, obj);
    BRANCH(IsDictionaryModeByHClass(hclass), &exit, &notDictionaryMode);
    Bind(&notDictionaryMode);
    GateRef enumCache = GetEnumCacheFromHClass(glue, hclass);
    BRANCH(TaggedIsEnumCache(glue, enumCache), &isEnumCache, &exit);
    Bind(&isEnumCache);
    GateRef kind = GetEnumCacheKindFromEnumCache(enumCache);
    BRANCH(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::SIMPLE))),
           &checkSimpleEnumCache, &notSimpleEnumCache);
    Bind(&checkSimpleEnumCache);
    {
        BRANCH(IsSimpleEnumCacheValid(glue, obj), &enumCacheValid, &exit);
    }
    Bind(&notSimpleEnumCache);
    {
        BRANCH(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::PROTOCHAIN))),
            &checkEnumCacheWithProtoChainInfo, &exit);
    }
    Bind(&checkEnumCacheWithProtoChainInfo);
    {
        BRANCH(IsProtoChainCacheValid(glue, obj), &enumCacheValid, &exit);
    }
    Bind(&enumCacheValid);
    {
        result = GetEnumCacheAllFromEnumCache(glue, enumCache);
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
        BRANCH(Int32Equal(xInt, Int32(0x80000000)), &overflow, &exit);
    } else {
        GateRef xInt64 = CastDoubleToInt64(x);
        // exp = (u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_SIGNIFICAND_SIZE - DOUBLE_EXPONENT_BIAS
        GateRef exp = Int64And(xInt64, Int64(base::DOUBLE_EXPONENT_MASK));
        exp = TruncInt64ToInt32(Int64LSR(exp, Int64(base::DOUBLE_SIGNIFICAND_SIZE)));
        exp = Int32Sub(exp, Int32(base::DOUBLE_EXPONENT_BIAS));
        GateRef bits = Int32(typeBits - 1);
        // exp < 32 - 1
        BRANCH(Int32LessThan(exp, bits), &exit, &overflow);
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
    BRANCH(HasPendingException(glue), &hasPendingException, &exit);
    Bind(&hasPendingException);
    Return(Exception());
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef StubBuilder::CalcHashcodeForInt(GateRef value)
{
    return env_->GetBuilder()->CalcHashcodeForInt(value);
}

GateRef StubBuilder::CanDoubleRepresentInt(GateRef exp, GateRef expBits, GateRef fractionBits)
{
    return LogicOrBuilder(env_)
        .Or(Int64Equal(expBits, Int64(base::DOUBLE_EXPONENT_MASK)))
        .Or(BitAnd(Int64Equal(expBits, Int64(0)), Int64NotEqual(fractionBits, Int64(0))))
        .Or(Int64LessThan(exp, Int64(0)))
        .Or(Int64GreaterThanOrEqual(exp, Int64(31U)))
        .Or(Int64NotEqual(Int64And(Int64LSL(fractionBits, exp), Int64(base::DOUBLE_SIGNIFICAND_MASK)),
                          Int64(0)))
        .Done();
}

void StubBuilder::CalcHashcodeForDouble(GateRef x, Variable *res, Label *exit)
{
    auto env = GetEnvironment();
    GateRef xInt64 = Int64Sub(ChangeTaggedPointerToInt64(x), Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    GateRef fractionBits = Int64And(xInt64, Int64(base::DOUBLE_SIGNIFICAND_MASK));
    GateRef expBits = Int64And(xInt64, Int64(base::DOUBLE_EXPONENT_MASK));
    GateRef isZero = BitAnd(
        Int64Equal(expBits, Int64(0)),
        Int64Equal(fractionBits, Int64(0)));
    Label zero(env);
    Label nonZero(env);

    BRANCH(isZero, &zero, &nonZero);
    Bind(&nonZero);
    {
        DEFVARIABLE(value, VariableType::JS_ANY(), x);
        // exp = (u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_SIGNIFICAND_SIZE - DOUBLE_EXPONENT_BIAS
        GateRef exp = Int64Sub(
            Int64LSR(expBits, Int64(base::DOUBLE_SIGNIFICAND_SIZE)),
            Int64(base::DOUBLE_EXPONENT_BIAS));
        Label convertToInt(env);
        Label calcHash(env);
        BRANCH(CanDoubleRepresentInt(exp, expBits, fractionBits), &calcHash, &convertToInt);
        Bind(&convertToInt);
        {
            *res = ChangeFloat64ToInt32(CastInt64ToFloat64(xInt64));
            Jump(exit);
        }
        Bind(&calcHash);
        {
            *res = env_->GetBuilder()->CalcHashcodeForInt(*value);
            Jump(exit);
        }
    }

    Bind(&zero);
    *res = Int32(0);
    Jump(exit);
}

GateRef StubBuilder::GetHash(GateRef glue, GateRef object)
{
    auto env = GetEnvironment();
    Label subentry(env);
    Label isHeapObject(env);
    Label exit(env);
    env->SubCfgEntry(&subentry);
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    GateRef value = Load(VariableType::JS_ANY(), glue, object, hashOffset);
    DEFVARIABLE(res, VariableType::INT32(), GetInt32OfTInt(value));
    BRANCH(TaggedIsHeapObject(value), &isHeapObject, &exit);

    Bind(&isHeapObject);
    {
        Label isTaggedArray(env);
        Label notTaggedArray(env);
        BRANCH(IsTaggedArray(glue, value), &isTaggedArray, &notTaggedArray);
        Bind(&isTaggedArray);
        GateRef extlen = GetExtraLengthOfTaggedArray(value);
        GateRef index = Int32Add(Int32(ECMAObject::HASH_INDEX), extlen);
        res = GetInt32OfTInt(GetValueFromTaggedArray(glue, value, index));
        Jump(&exit);
        Bind(&notTaggedArray);
        res = Int32(0);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::SetHash(GateRef glue, GateRef object, GateRef hash)
{
    auto env = GetEnvironment();
    Label subentry(env);
    Label isHeapObject(env);
    Label notHeapObject(env);
    Label exit(env);
    env->SubCfgEntry(&subentry);
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    GateRef value = Load(VariableType::JS_ANY(), glue, object, hashOffset);
    DEFVARIABLE(res, VariableType::JS_ANY(), object);
    BRANCH(TaggedIsHeapObject(value), &isHeapObject, &notHeapObject);

    Bind(&isHeapObject);
    {
        Label isTaggedArray(env);
        Label notTaggedArray(env);
        BRANCH(IsTaggedArray(glue, value), &isTaggedArray, &notTaggedArray);
        Bind(&isTaggedArray);
        {
            GateRef extlen = GetExtraLengthOfTaggedArray(value);
            GateRef index = Int32Add(Int32(ECMAObject::HASH_INDEX), extlen);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, value, index, IntToTaggedInt(hash));
            Jump(&exit);
        }
        Bind(&notTaggedArray);
        {
            Label isNativePointer(env);
            Label notNativePointer(env);
            BRANCH(IsNativePointer(glue, value), &isNativePointer, &notNativePointer);
            Bind(&isNativePointer);
            {
                NewObjectStubBuilder newBuilder(this);
                GateRef array = newBuilder.NewTaggedArray(glue, Int32(ECMAObject::RESOLVED_MAX_SIZE));
                SetExtraLengthOfTaggedArray(glue, array, Int32(0));
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, array,
                                      Int32(ECMAObject::HASH_INDEX), IntToTaggedInt(hash));
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, array,
                                      Int32(ECMAObject::FUNCTION_EXTRA_INDEX), value);
                Store(VariableType::JS_ANY(), glue, object, hashOffset, array);
                Jump(&exit);
            }
            Bind(&notNativePointer);
            FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
            Jump(&exit);
        }
    }
    Bind(&notHeapObject);
    {
        Store(VariableType::JS_ANY(), glue, object, hashOffset, IntToTaggedInt(hash), MemoryAttribute::NoBarrier());
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::CalcHashcodeForObject(GateRef glue, GateRef value, Variable *res, Label *exit)
{
    auto env = GetEnvironment();

    GateRef hash = GetHash(glue, value);
    *res = hash;
    Label calcHash(env);
    BRANCH(Int32Equal(**res, Int32(0)), &calcHash, exit);
    Bind(&calcHash);
    GateRef offset = IntPtr(JSThread::GlueData::GetRandomStatePtrOffset(env_->Is32Bit()));
    GateRef randomStatePtr = LoadPrimitive(VariableType::NATIVE_POINTER(), glue, offset);
    GateRef randomState = LoadPrimitive(VariableType::INT64(), randomStatePtr, IntPtr(0));
    GateRef k1 = Int64Xor(randomState, Int64LSR(randomState, Int64(base::RIGHT12)));
    GateRef k2 = Int64Xor(k1, Int64LSL(k1, Int64(base::LEFT25)));
    GateRef k3 = Int64Xor(k2, Int64LSR(k2, Int64(base::RIGHT27)));
    Store(VariableType::INT64(), glue, randomStatePtr, IntPtr(0), k3);
    GateRef k4 = Int64Mul(k3, Int64(base::GET_MULTIPLY));
    GateRef k5 = Int64LSR(k4, Int64(base::INT64_BITS - base::INT32_BITS));
    GateRef k6 = Int32And(TruncInt64ToInt32(k5), Int32(INT32_MAX));
    SetHash(glue, value, k6);
    *res = k6;
    Jump(exit);
}

GateRef StubBuilder::GetHashcodeFromString(GateRef glue, GateRef value, GateRef hir)
{
    return env_->GetBuilder()->GetHashcodeFromString(glue, value, hir);
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
    BRANCH(TaggedIsHeapObject(outPut), &isHeapObject, &notEcmaObj);
    Bind(&isHeapObject);
    BRANCH(TaggedObjectIsEcmaObject(glue, outPut), &isEcmaObj, &notEcmaObj);
    Bind(&isEcmaObj);
    {
        result = outPut;
        Jump(&exit);
    }
    Bind(&notEcmaObj);
    {
        Label ctorIsBase(env);
        Label ctorNotBase(env);
        BRANCH(IsBase(glue, ctor), &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            result = thisObj;
            Jump(&exit);
        }
        Bind(&ctorNotBase);
        {
            Label throwExeption(env);
            Label returnObj(env);
            BRANCH(TaggedIsUndefined(outPut), &returnObj, &throwExeption);
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

GateRef StubBuilder::CalIteratorKey(GateRef glue)
{
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef iteratorKey = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ITERATOR_SYMBOL_INDEX);
    return iteratorKey;
}

void StubBuilder::FuncOrHClassCompare(GateRef glue, GateRef funcOrHClass,
                                      Label *match, Label *slowPath, size_t index)
{
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef globalRecord = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, index);
    BRANCH(Equal(globalRecord, funcOrHClass), match, slowPath);
}

GateRef StubBuilder::IsDetectorInvalid(GateRef glue, size_t indexDetector)
{
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef value = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, indexDetector);
    GateRef bitFieldOffset = IntPtr(MarkerCell::BIT_FIELD_OFFSET);
    GateRef bitField = LoadPrimitive(VariableType::INT32(), value, bitFieldOffset);
    GateRef mask = Int32(1LLU << (MarkerCell::IS_DETECTOR_INVALID_BITS - 1));
    return Int32NotEqual(Int32And(bitField, mask), Int32(0));
}

void StubBuilder::HClassCompareAndCheckDetector(GateRef glue, GateRef hclass,
                                                Label *match, Label *slowPath,
                                                size_t indexHClass, bool isMap)
{
    auto env = GetEnvironment();
    Label matchHClass(env);
    FuncOrHClassCompare(glue, hclass, &matchHClass, slowPath, indexHClass);
    Bind(&matchHClass);
    GateRef globalEnv = GetCurrentGlobalEnv();
    if (isMap) {
        BRANCH(GetMapIteratorDetector(globalEnv), slowPath, match);
    } else {
        BRANCH(GetSetIteratorDetector(globalEnv), slowPath, match);
    }
}

void StubBuilder::GetIteratorResult(GateRef glue, Variable *result, GateRef obj,
                                    Label *isPendingException, Label *noPendingException)
{
    GateRef iteratorKey = CalIteratorKey(glue);
    *result = FastGetPropertyByName(glue, obj, iteratorKey, ProfileOperation());
    BRANCH(HasPendingException(glue), isPendingException, noPendingException);
}

// If the jsType of the obj is JS_ARRAY and
// its elementsKind is not GENERIC or it's hclass == GENERIC array's ihc
// the obj doesn't have symbol.iterator within itself.
// So when the Array.prototype[symbol.iterator] remains unchanged, we call FastPath.
void StubBuilder::TryFastGetArrayIterator(GateRef glue, GateRef hclass, GateRef jsType,
                                          Label *slowPath2, Label *matchArray)
{
    auto env = GetEnvironment();
    Label arrayDetectorValid(env);
    Label tryArray(env);
    BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_ARRAY))), &tryArray, slowPath2);
    Bind(&tryArray);
    {
        GateRef globalEnv = GetCurrentGlobalEnv();
        BRANCH(GetArrayIteratorDetector(globalEnv), slowPath2, &arrayDetectorValid);
        Bind(&arrayDetectorValid);
        {
            BuiltinsArrayStubBuilder arrayStubBuilder(this, globalEnv);
            arrayStubBuilder.ElementsKindHclassCompare(glue, hclass, matchArray, slowPath2);
        }
    }
}

void StubBuilder::TryFastGetIterator(GateRef glue, GateRef obj, GateRef hclass,
                                     Variable &result, Label *slowPath, Label *exit,
                                     Label *isPendingException)
{
    auto env = GetEnvironment();
    Label matchMap(env);
    Label notmatchMap(env);
    Label matchSet(env);
    Label notmatchSet(env);
    Label tryArray(env);
    Label matchArray(env);
    Label isMap(env);
    Label isNotMap(env);
    Label isSet(env);
    Label isNotSet(env);
    Label isArray(env);
    Label noPendingException(env);
    Label slowPath2(env);

    // When the symbol.iterator method remains unmodified
    // it is used to quickly process instances of Map, Set whose hclass == Map/Set's ihc.
    // In this situation we don't need to perform FastGetPropertyByName and CallRuntime.
    HClassCompareAndCheckDetector(glue, hclass, &matchMap, &notmatchMap, GlobalEnv::MAP_CLASS_INDEX, true);
    Bind(&notmatchMap);
    HClassCompareAndCheckDetector(glue, hclass, &matchSet, &notmatchSet, GlobalEnv::SET_CLASS_INDEX, false);
    Bind(&notmatchSet);

    GateRef jsType = GetObjectType(hclass);
    TryFastGetArrayIterator(glue, hclass, jsType, &slowPath2, &matchArray);

    Bind(&slowPath2);
    GetIteratorResult(glue, &result, obj, isPendingException, &noPendingException);
    // Mainly solve the situation with inheritance
    // and the symbol.iterator method remains unmodified.
    // In this situation we need to perform FastGetPropertyByName but needn't CallRuntime.
    Bind(&noPendingException);
    {
        BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_MAP))), &isMap, &isNotMap);
        Bind(&isMap);
        {
            FuncOrHClassCompare(glue, *result, &matchMap, slowPath, GlobalEnv::MAP_PROTO_ENTRIES_FUNCTION_INDEX);
        }
        Bind(&isNotMap);
        BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_SET))), &isSet, &isNotSet);
        Bind(&isSet);
        {
            FuncOrHClassCompare(glue, *result, &matchSet, slowPath, GlobalEnv::SET_PROTO_VALUES_FUNCTION_INDEX);
        }
        Bind(&isNotSet);
        BRANCH(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_ARRAY))), &isArray, slowPath);
        Bind(&isArray);
        {
            FuncOrHClassCompare(glue, *result, &matchArray, slowPath, GlobalEnv::ARRAY_PROTO_VALUES_FUNCTION_INDEX);
        }
    }

    Bind(&matchMap);
    {
        BuiltinsCollectionStubBuilder<JSMap> collectionStubBuilder(this, glue, obj, Int32(0), GetCurrentGlobalEnv());
        collectionStubBuilder.Entries(&result, exit, slowPath);
    }
    Bind(&matchSet);
    {
        BuiltinsCollectionStubBuilder<JSSet> collectionStubBuilder(this, glue, obj, Int32(0), GetCurrentGlobalEnv());
        collectionStubBuilder.Values(&result, exit, slowPath);
    }
    Bind(&matchArray);
    {
        BuiltinsArrayStubBuilder arrayStubBuilder(this, GetCurrentGlobalEnv());
        arrayStubBuilder.Values(glue, obj, Int32(0), &result, exit, slowPath);
    }
}

#if ENABLE_NEXT_OPTIMIZATION
GateRef StubBuilder::GetIterator(GateRef glue, GateRef obj, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    DEFVARIABLE(taggedId, VariableType::INT32(), Int32(GET_MESSAGE_STRING_ID(ObjIsNotCallable)));

    Label isHeapObject(env);
    Label objIsHeapObject(env);
    Label objIsCallable(env);
    Label throwError(env);
    Label callExit(env);
    Label slowPath(env);
    Label slowPath3(env);
    Label isPendingException(env);

    BRANCH(TaggedIsHeapObject(obj), &objIsHeapObject, &slowPath3);
    Bind(&objIsHeapObject);
    GateRef hclass = LoadHClass(glue, obj);
    TryFastGetIterator(glue, obj, hclass, result, &slowPath, &exit, &isPendingException);

    Bind(&slowPath3);
    GetIteratorResult(glue, &result, obj, &isPendingException, &slowPath);

    Bind(&slowPath);
    callback.ProfileGetIterator(*result);
    BRANCH(TaggedIsHeapObject(*result), &isHeapObject, &throwError);
    Bind(&isHeapObject);
    BRANCH(IsCallable(glue, *result), &objIsCallable, &throwError);
    Bind(&objIsCallable);
    {
        JSCallArgs callArgs(JSCallMode::CALL_GETTER);
        callArgs.callGetterArgs = { obj };
        CallStubBuilder callBuilder(this, glue, *result, Int32(0), 0, &result, Circuit::NullGate(), callArgs,
            ProfileOperation());
        if (env->IsBaselineBuiltin()) {
            callBuilder.JSCallDispatchForBaseline(&callExit);
            Bind(&callExit);
            Jump(&exit);
        } else {
            result = callBuilder.JSCallDispatch();
            Label modifyErrorInfo(env);
            BRANCH(TaggedIsHeapObject(*result), &exit, &modifyErrorInfo);
            Bind(&modifyErrorInfo);
            taggedId = Int32(GET_MESSAGE_STRING_ID(IterNotObject));
            Jump(&throwError);
        }
    }
    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
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
#else
GateRef StubBuilder::GetIterator(GateRef glue, GateRef obj, ProfileOperation callback)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    DEFVARIABLE(taggedId, VariableType::INT32(), Int32(GET_MESSAGE_STRING_ID(ObjIsNotCallable)));

    Label isPendingException(env);
    Label noPendingException(env);
    Label isHeapObject(env);
    Label objIsCallable(env);
    Label throwError(env);
    Label callExit(env);

    GateRef iteratorKey = CalIteratorKey(glue);
    result = FastGetPropertyByName(glue, obj, iteratorKey, ProfileOperation());
    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
    Bind(&isPendingException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noPendingException);
    callback.ProfileGetIterator(*result);
    BRANCH(TaggedIsHeapObject(*result), &isHeapObject, &throwError);
    Bind(&isHeapObject);
    BRANCH(IsCallable(glue, *result), &objIsCallable, &throwError);
    Bind(&objIsCallable);
    {
        JSCallArgs callArgs(JSCallMode::CALL_GETTER);
        callArgs.callGetterArgs = { obj };
        CallStubBuilder callBuilder(this, glue, *result, Int32(0), 0, &result, Circuit::NullGate(), callArgs,
            ProfileOperation());
        if (env->IsBaselineBuiltin()) {
            callBuilder.JSCallDispatchForBaseline(&callExit);
            Bind(&callExit);
            Jump(&exit);
        } else {
            result = callBuilder.JSCallDispatch();
            Label modifyErrorInfo(env);
            BRANCH(TaggedIsHeapObject(*result), &exit, &modifyErrorInfo);
            Bind(&modifyErrorInfo);
            taggedId = Int32(GET_MESSAGE_STRING_ID(IterNotObject));
            Jump(&throwError);
        }
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
#endif

GateRef StubBuilder::TryStringOrSymbolToElementIndex(GateRef glue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));

    Label keyNotSymbol(env);
    BRANCH(IsSymbol(glue, key), &exit, &keyNotSymbol);
    Bind(&keyNotSymbol);

    Label greatThanZero(env);
    Label inRange(env);
    Label flattenFastPath(env);
    auto len = GetLengthFromString(key);
    BRANCH(Int32Equal(len, Int32(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    BRANCH(Int32GreaterThan(len, Int32(MAX_ELEMENT_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        Label isUtf8(env);
        DEFVARIABLE(c, VariableType::INT32(), Int32(0));
        BRANCH(IsUtf16String(key), &exit, &isUtf8);
        Bind(&isUtf8);
        FlatStringStubBuilder thisFlat(this);
        thisFlat.FlattenString(glue, key, &flattenFastPath);
        Bind(&flattenFastPath);
        StringInfoGateRef stringInfoGate(&thisFlat);
        GateRef data = GetNormalStringData(glue, stringInfoGate);
        c = ZExtInt8ToInt32(LoadZeroOffsetPrimitive(VariableType::INT8(), data));
        Label isDigitZero(env);
        Label notDigitZero(env);
        BRANCH(Int32Equal(*c, Int32('0')), &isDigitZero, &notDigitZero);
        Bind(&isDigitZero);
        {
            Label lengthIsOne(env);
            BRANCH(Int32Equal(len, Int32(1)), &lengthIsOne, &exit);
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

            BRANCH(IsDigit(*c), &isDigit, &notIsDigit);
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Bind(&isDigit);
            BRANCH(Int32UnsignedLessThan(*i, len), &loopHead, &afterLoop);
            LoopBegin(&loopHead);
            {
                c = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), data, ZExtInt32ToPtr(*i)));
                Label isDigit2(env);
                Label notDigit2(env);
                BRANCH(IsDigit(*c), &isDigit2, &notDigit2);
                Bind(&isDigit2);
                {
                    // 10 means the base of digit is 10.
                    n = Int64Add(Int64Mul(*n, Int64(10)),
                                 Int64Sub(SExtInt32ToInt64(*c), Int64('0')));
                    i = Int32Add(*i, Int32(1));
                    BRANCH(Int32UnsignedLessThan(*i, len), &loopEnd, &afterLoop);
                }
                Bind(&notDigit2);
                {
                    Label hasPoint(env);
                    BRANCH(Int32Equal(*c, Int32('.')), &hasPoint, &exit);
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
                BRANCH(Int64LessThan(*n, Int64(JSObject::MAX_ELEMENT_INDEX)),
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
                BRANCH(Int32Equal(*c, Int32('-')), &isNegative, &exit);
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
    BRANCH(Int64NotEqual(receiver, holder), &exit, &notOnProtoChain);
    Bind(&notOnProtoChain);

    auto negativeZero = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::NEGATIVE_ZERO_STRING_INDEX);
    Label isNegativeZero(env);
    Label notNegativeZero(env);
    BRANCH(Equal(negativeZero, key), &isNegativeZero, &notNegativeZero);
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
        BRANCH(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            BuiltinsTypedArrayStubBuilder typedArrayStubBuilder(this, GetCurrentGlobalEnv());
            result = typedArrayStubBuilder.FastGetPropertyByIndex(glue, holder, index, jsType);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label returnNull(env);
            BRANCH(Int32Equal(index, Int32(-2)), &returnNull, &exit); // -2:equal -2 means should goto slow path
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
    BRANCH(Int64NotEqual(receiver, holder), &exit, &notOnProtoChain);
    Bind(&notOnProtoChain);

    auto negativeZero = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::NEGATIVE_ZERO_STRING_INDEX);
    Label isNegativeZero(env);
    Label notNegativeZero(env);
    BRANCH(Equal(negativeZero, key), &isNegativeZero, &notNegativeZero);
    Bind(&isNegativeZero);
    {
        Label isObj(env);
        Label notObj(env);
        BRANCH(IsEcmaObject(glue, value), &isObj, &notObj);
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
        BRANCH(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = CallRuntime(glue, RTSTUB_ID(SetTypeArrayPropertyByIndex),
                { receiver, IntToTaggedInt(index), value, IntToTaggedInt(jsType) });
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label returnNull(env);
            BRANCH(Int32Equal(index, Int32(-2)), &returnNull, &exit); // -2:equal -2 means should goto slow path
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
    BRANCH(condition, &ok, &notOk);
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

GateRef StubBuilder::GetNormalStringData([[maybe_unused]] GateRef glue, const StringInfoGateRef &stringInfoGate)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isUtf8(env);
    Label isUtf16(env);
    DEFVARIABLE(result, VariableType::NATIVE_POINTER(), Undefined());
    GateRef data = ChangeTaggedPointerToInt64(
        PtrAdd(stringInfoGate.GetString(), IntPtr(LineString::DATA_OFFSET)));
    BRANCH(IsUtf8String(stringInfoGate.GetString()), &isUtf8, &isUtf16);
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
    Label isUndefinedOrNull(env);
    Label notUndefinedOrNull(env);
    Label defaultLabel(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    BRANCH(TaggedIsNumber(tagged), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = tagged;
        Jump(&exit);
    }
    Bind(&notNumber);
    {
        GateRef flag = LogicOrBuilder(env)
            .Or(TaggedIsUndefined(tagged))
            .Or(TaggedIsHole(tagged))
            .Done();
        BRANCH(flag, &isUndefinedOrNull, &notUndefinedOrNull);
        Bind(&isUndefinedOrNull);
        {
            result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
            Jump(&exit);
        }
        Bind(&notUndefinedOrNull);
        {
            Label isTrue(env);
            Label notTrue(env);
            BRANCH(TaggedIsTrue(tagged), &isTrue, &notTrue);
            Bind(&isTrue);
            {
                result = DoubleToTaggedDoublePtr(Double(1));
                Jump(&exit);
            }
            Bind(&notTrue);
            {
                Label isFlaseOrNull(env);
                GateRef flag1 = LogicOrBuilder(env)
                    .Or(TaggedIsFalse(tagged))
                    .Or(TaggedIsNull(tagged))
                    .Done();
                BRANCH(flag1, &isFlaseOrNull, &defaultLabel);
                Bind(&isFlaseOrNull);
                {
                    result = DoubleToTaggedDoublePtr(Double(0));
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&defaultLabel);
    {
        result = CallRuntime(glue, RTSTUB_ID(ToNumber), { tagged });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ToIndex(GateRef glue, GateRef tagged)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isInt(env);
    Label notInt(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    BRANCH(TaggedIsInt(tagged), &isInt, &notInt);
    Bind(&isInt);
    {
        Label lessThanZero(env);
        Label greaterOrEqualZero(env);
        BRANCH(Int32GreaterThanOrEqual(TaggedGetInt(tagged), Int32(0)), &greaterOrEqualZero, &notInt);
        Bind(&greaterOrEqualZero);
        {
            result = tagged;
            Jump(&exit);
        }
    }
    Bind(&notInt);
    {
        Label isUndef(env);
        Label notUndef(env);
        BRANCH(TaggedIsUndefined(tagged), &isUndef, &notUndef);
        Bind(&isUndef);
        {
            result = IntToTaggedPtr(Int32(0));
            Jump(&exit);
        }
        Bind(&notUndef);
        {
            result = CallRuntime(glue, RTSTUB_ID(ToIndex), { tagged });
            Jump(&exit);
        }
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
    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
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
        BRANCH(DoubleLessThan(num, Double(0.0)), &targetLessThanZero, &targetGreaterThanZero);
        Bind(&targetLessThanZero);
        {
            res = DoubleToTaggedDoublePtr(Double(0.0));
            Jump(&exit);
        }
        Bind(&targetGreaterThanZero);
        BRANCH(DoubleGreaterThan(num, Double(SAFE_NUMBER)), &targetGreaterThanSafeNumber, &targetLessThanSafeNumber);
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
    BRANCH(TaggedIsInt(x), &targetIsInt, &targetIsDouble);
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
    BRANCH(TaggedIsInt(x), &targetIsInt, &targetIsDouble);
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

GateRef StubBuilder::IsStableJSArguments(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label targetIsHeapObject(env);
    Label targetIsStableArguments(env);

    BRANCH(TaggedIsHeapObject(obj), &targetIsHeapObject, &exit);
    Bind(&targetIsHeapObject);
    {
        GateRef jsHclass = LoadHClass(glue, obj);
        BRANCH(IsStableArguments(jsHclass), &targetIsStableArguments, &exit);
        Bind(&targetIsStableArguments);
        {
            GateRef globalEnv = GetCurrentGlobalEnv();
            result = GetArrayElementsGuardians(globalEnv);
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
    BRANCH_LIKELY(TaggedIsHeapObject(obj), &targetIsHeapObject, &exit);
    Bind(&targetIsHeapObject);
    {
        GateRef jsHclass = LoadHClass(glue, obj);
        BRANCH_LIKELY(IsStableArray(jsHclass), &targetIsStableArray, &exit);
        Bind(&targetIsStableArray);
        {
            Label isPrototypeNotModified(env);
            BRANCH_UNLIKELY(IsJSArrayPrototypeModified(jsHclass), &exit, &isPrototypeNotModified);
            Bind(&isPrototypeNotModified);
            {
                GateRef globalEnv = GetCurrentGlobalEnv();
                GateRef guardians = GetArrayElementsGuardians(globalEnv);
                result.WriteVariable(guardians);
                Jump(&exit);
            }
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
    DEFVARIABLE(profileTypeInfo, VariableType::JS_POINTER(), GetProfileTypeInfo(glue, jsFunc));
    BRANCH(TaggedIsUndefined(*profileTypeInfo), &needUpdate, &exit);
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
    GateRef bitfield = LoadPrimitive(VariableType::INT32(), method, extraLiteralInfoOffset);

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
    Label noException(env);
    Label isException(env);

    GateRef itor = GetIterator(glue, array, callBack);
    BRANCH(TaggedIsException(itor), &isException, &noException);
    Bind(&isException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&noException);
    GateRef needCopy = LogicAndBuilder(env)
        .And(Int32Equal(GetObjectType(LoadHClass(glue, itor)), Int32(static_cast<int32_t>(JSType::JS_ARRAY_ITERATOR))))
        .And(IsStableJSArray(glue, array))
        .Done();
    BRANCH(needCopy, &fastPath, &noCopyPath);
    Bind(&fastPath);
    {
        // copy operation is omitted
        result = CopyJSArrayToTaggedArrayArgs(glue, array);
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
    BRANCH(Int32LessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        GateRef tempBeginIndex = Int32Add(arrayLen, index);
        Label beginIndexLargeZero(env);
        BRANCH(Int32GreaterThan(tempBeginIndex, Int32(0)), &beginIndexLargeZero, &exit);
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
        BRANCH(Int32LessThan(index, arrayLen), &lessLen, &largeLen);
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
        BRANCH(Int32UnsignedLessThan(*index, firstLength), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            GateRef value = GetValueFromTaggedArray(glue, first, *index);
            BRANCH(TaggedIsHole(value), &afterLoop, &notHole);
            Bind(&notHole);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, value);
            index = Int32Add(*index, Int32(1));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&afterLoop);
    {
        Label loopHead1(env);
        Label loopEnd1(env);
        Label storeValue1(env);
        Label notHole1(env);
        Jump(&loopHead1);
        LoopBegin(&loopHead1);
        {
            BRANCH(Int32UnsignedLessThan(*i, secondLength), &storeValue1, &exit);
            Bind(&storeValue1);
            {
                GateRef value1 = GetValueFromTaggedArray(glue, second, *i);
                BRANCH(TaggedIsHole(value1), &exit, &notHole1);
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

GateRef StubBuilder::ToCharCode(GateRef number)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), number);

    Label lessThanTen(env);
    Label notLessThanTen(env);
    BRANCH(Int32LessThan(number, Int32(10)), &lessThanTen, &notLessThanTen); // 10: means number
    Bind(&lessThanTen);
    {
        result = Int32Add(Int32('0'), *result);
        Jump(&exit);
    }
    Bind(&notLessThanTen);
    {
        result = Int32Sub(*result, Int32(10)); // 10: means number
        result = Int32Add(Int32('a'), *result);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
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
    BRANCH(BitAnd(isPositive, isSingle), &process, &callRuntime);
    Bind(&process);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), true);
        Bind(&afterNew);
        n = Int32Add(Int32('0'), *n);
        GateRef dst = ChangeTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
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

GateRef StubBuilder::NumberToString(GateRef glue, GateRef number)
{
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    res = CallRuntime(glue, RTSTUB_ID(NumberToString), { number });
    return *res;
}

void StubBuilder::RestoreElementsKindToGeneric(GateRef glue, GateRef jsHClass)
{
    GateRef newKind = Int32(Elements::ToUint(ElementsKind::GENERIC));
    SetElementsKindToJSHClass(glue, jsHClass, newKind);
}

GateRef StubBuilder::GetTaggedValueWithElementsKind(GateRef glue, GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label enableMutantArray(env);
    Label disableMutantArray(env);
    Label isMutantTaggedArray(env);
    Label isNotMutantTaggedArray(env);
    GateRef elements = GetElementsArray(glue, receiver);
    BRANCH_UNLIKELY(IsEnableMutantArray(glue), &enableMutantArray, &disableMutantArray);
    Bind(&disableMutantArray);
    {
        result = GetValueFromTaggedArray(glue, elements, index);
        Jump(&exit);
    }
    Bind(&enableMutantArray);
    GateRef hclass = LoadHClass(glue, receiver);
    DEFVARIABLE(elementsKind, VariableType::INT32(), GetElementsKindFromHClass(hclass));
    BRANCH(IsMutantTaggedArray(glue, elements), &isMutantTaggedArray, &isNotMutantTaggedArray);
    Bind(&isNotMutantTaggedArray);
    {
        elementsKind = Int32(Elements::ToUint(ElementsKind::GENERIC));
        Jump(&isMutantTaggedArray);
    }
    Bind(&isMutantTaggedArray);
    GateRef rawValue = GetValueFromMutantTaggedArray(elements, index);
    Label isSpecialHole(env);
    Label isNotSpecialHole(env);
    BRANCH(Int64Equal(rawValue, SpecialHole()), &isSpecialHole, &isNotSpecialHole);
    Bind(&isSpecialHole);
    {
        Jump(&exit);
    }
    Bind(&isNotSpecialHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        GateRef elementsKindIntLowerBound = Int32GreaterThanOrEqual(*elementsKind,
                                                                    Int32(Elements::ToUint(ElementsKind::INT)));
        GateRef elementsKindIntUpperBound = Int32LessThanOrEqual(*elementsKind,
                                                                 Int32(Elements::ToUint(ElementsKind::HOLE_INT)));
        GateRef checkIntKind = BitAnd(elementsKindIntLowerBound, elementsKindIntUpperBound);
        BRANCH(checkIntKind, &isInt, &isNotInt);
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
                                                                   Int32(Elements::ToUint(ElementsKind::NUMBER)));
            GateRef elementsKindNumberUB = Int32LessThanOrEqual(*elementsKind,
                                                                Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER)));
            GateRef checkNumberKind = BitAnd(elementsKindNumberLB, elementsKindNumberUB);
            BRANCH(checkNumberKind, &isNumber, &isNotNumber);
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

GateRef StubBuilder::ConvertTaggedValueWithElementsKind([[maybe_unused]] GateRef glue, GateRef value, GateRef  extraKind)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label isHole(env);
    Label isNotHole(env);
    GateRef valueIsHole = TaggedIsHole(value);
    GateRef elementsKindInNumbersLB = Int32GreaterThanOrEqual(extraKind,
                                                              Int32(Elements::ToUint(ElementsKind::HOLE)));
    GateRef elementsKindInNumbersUB = Int32LessThan(extraKind, Int32(Elements::ToUint(ElementsKind::STRING)));
    GateRef checkInNumersKind = LogicAndBuilder(env)
        .And(valueIsHole)
        .And(elementsKindInNumbersLB)
        .And(elementsKindInNumbersUB).Done();
    BRANCH(checkInNumersKind, &isHole, &isNotHole);
    Bind(&isHole);
    {
        Jump(&exit);
    }
    Bind(&isNotHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        GateRef elementsKindIntLB = Int32GreaterThanOrEqual(extraKind,
                                                            Int32(Elements::ToUint(ElementsKind::INT)));
        GateRef elementsKindIntUB = Int32LessThanOrEqual(extraKind,
                                                         Int32(Elements::ToUint(ElementsKind::HOLE_INT)));
        GateRef checkIntKind = LogicAndBuilder(env)
            .And(elementsKindIntLB)
            .And(elementsKindIntUB)
            .Done();
        BRANCH(checkIntKind, &isInt, &isNotInt);
        Bind(&isInt);
        {
            result = Int64ToTaggedPtr(GetInt64OfTInt(value));
            Jump(&exit);
        }
        Bind(&isNotInt);
        {
            Label isNumber(env);
            Label isNotNumber(env);
            GateRef elementsKindNumberLB = Int32GreaterThanOrEqual(extraKind,
                                                                   Int32(Elements::ToUint(ElementsKind::NUMBER)));
            GateRef elementsKindNumberUB = Int32LessThanOrEqual(extraKind,
                                                                Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER)));
            GateRef checkNumberKind = LogicAndBuilder(env)
                .And(elementsKindNumberLB)
                .And(elementsKindNumberUB)
                .Done();
            BRANCH(checkNumberKind, &isNumber, &isNotNumber);
            Bind(&isNumber);
            {
                Label isNumberInt(env);
                Label isNotNumberInt(env);
                BRANCH(TaggedIsInt(value), &isNumberInt, &isNotNumberInt);
                Bind(&isNumberInt);
                {
                    result = Int64ToTaggedPtr(CastDoubleToInt64(GetDoubleOfTInt(value)));
                    Jump(&exit);
                }
                Bind(&isNotNumberInt);
                {
                    result = Int64ToTaggedPtr(CastDoubleToInt64(GetDoubleOfTDouble(value)));
                    Jump(&exit);
                }
            }
            Bind(&isNotNumber);
            {
                result = value;
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
    BRANCH(needTransition, &transitElementsKind, &finishTransition);
    Bind(&transitElementsKind);
    {
        TransitToElementsKind(glue, receiver, rawValue, extraKind);
        Jump(&finishTransition);
    }
    Bind(&finishTransition);
    GateRef elements = GetElementsArray(glue, receiver);
    Label enableMutantArray(env);
    Label disableMutantArray(env);
    BRANCH_UNLIKELY(IsEnableMutantArray(glue), &enableMutantArray, &disableMutantArray);
    Bind(&disableMutantArray);
    {
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, rawValue);
        Jump(&exit);
    }
    Bind(&enableMutantArray);
    Label setValue(env);
    Label isMutantTaggedArray(env);
    Label isNotMutantTaggedArray(env);
    GateRef hclass = LoadHClass(glue, receiver);
    DEFVARIABLE(elementsKind, VariableType::INT32(), GetElementsKindFromHClass(hclass));
    BRANCH(IsMutantTaggedArray(glue, elements), &isMutantTaggedArray, &isNotMutantTaggedArray);
    Bind(&isNotMutantTaggedArray);
    {
        elementsKind = Int32(Elements::ToUint(ElementsKind::GENERIC));
        Jump(&isMutantTaggedArray);
    }
    Bind(&isMutantTaggedArray);
    Label isHole(env);
    Label isNotHole(env);
    GateRef elementsKindVal = *elementsKind;
    GateRef checkInNumersKind = LogicAndBuilder(env)
        .And(TaggedIsHole(rawValue))
        .And(Int32GreaterThanOrEqual(elementsKindVal, Int32(Elements::ToUint(ElementsKind::HOLE))))
        .And(Int32LessThan(elementsKindVal, Int32(Elements::ToUint(ElementsKind::STRING))))
        .Done();
    BRANCH(checkInNumersKind, &isHole, &isNotHole);
    Bind(&isHole);
    {
        Jump(&setValue);
    }
    Bind(&isNotHole);
    {
        Label isInt(env);
        Label isNotInt(env);
        GateRef elementsKindIntLB = Int32GreaterThanOrEqual(*elementsKind,
                                                            Int32(Elements::ToUint(ElementsKind::INT)));
        GateRef elementsKindIntUB = Int32LessThanOrEqual(*elementsKind,
                                                         Int32(Elements::ToUint(ElementsKind::HOLE_INT)));
        GateRef checkIntKind = BitAnd(elementsKindIntLB, elementsKindIntUB);
        BRANCH(checkIntKind, &isInt, &isNotInt);
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
                                                                   Int32(Elements::ToUint(ElementsKind::NUMBER)));
            GateRef elementsKindNumberUB = Int32LessThanOrEqual(*elementsKind,
                                                                Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER)));
            GateRef checkNumberKind = BitAnd(elementsKindNumberLB, elementsKindNumberUB);
            BRANCH(checkNumberKind, &isNumber, &isNotNumber);
            Bind(&isNumber);
            {
                Label isNumberInt(env);
                Label isNotNumberInt(env);
                BRANCH(TaggedIsInt(rawValue), &isNumberInt, &isNotNumberInt);
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
    BRANCH(TaggedIsHeapObject(rawValue), &storeToNormalArray, &storeToMutantArray);
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

void StubBuilder::FastSetValueWithElementsKind(GateRef glue, GateRef receiver, GateRef elements, GateRef rawValue,
                                               GateRef index, ElementsKind kind, bool needTransition)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label exit(env);
    if (needTransition) {
        TransitToElementsKind(glue, receiver, rawValue, Int32(Elements::ToUint(ElementsKind::NONE)));
    }
    if (kind == ElementsKind::INT || kind == ElementsKind::NUMBER) {
        SetValueToTaggedArray(VariableType::INT64(), glue, elements, index, rawValue);
        Jump(&exit);
    } else {
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, rawValue);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::CopyJSArrayToTaggedArrayArgs(GateRef glue, GateRef srcObj)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);

    Label isMutantTaggedArray(env);
    result = GetElementsArray(glue, srcObj);
    BRANCH(IsMutantTaggedArray(glue, *result), &isMutantTaggedArray, &exit);
    Bind(&isMutantTaggedArray);
    {
        GateRef argvLength = GetLengthOfTaggedArray(*result);
        NewObjectStubBuilder newBuilder(this);
        GateRef argv = newBuilder.NewTaggedArray(glue, argvLength);
        DEFVARIABLE(index, VariableType::INT32(), Int32(0));
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Label storeValue(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH(Int32UnsignedLessThan(*index, argvLength), &storeValue, &afterLoop);
            Bind(&storeValue);
            {
                GateRef value = GetTaggedValueWithElementsKind(glue, srcObj, *index);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argv, *index, value);
                index = Int32Add(*index, Int32(1));
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = argv;
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::MigrateArrayWithKind(GateRef glue, GateRef object, GateRef oldKind, GateRef newKind)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label exit(env);

    Label mutantArrayOn(env);
    BRANCH(IsEnableMutantArray(glue), &mutantArrayOn, &exit);
    Bind(&mutantArrayOn);

    DEFVARIABLE(newElements, VariableType::JS_ANY(), Undefined());
    Label doMigration(env);
    Label migrateFromInt(env);
    Label migrateOtherKinds(env);

    GateRef noNeedMigration = LogicOrBuilder(env)
        .Or(Int32Equal(oldKind, newKind))
        .Or(BitAnd(Int32Equal(oldKind, Int32(Elements::ToUint(ElementsKind::INT))),
                   Int32Equal(newKind, Int32(Elements::ToUint(ElementsKind::HOLE_INT)))))
        .Or(BitAnd(Int32Equal(oldKind, Int32(Elements::ToUint(ElementsKind::NUMBER))),
                   Int32Equal(newKind, Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER)))))
        .Done();
    BRANCH(noNeedMigration, &exit, &doMigration);
    Bind(&doMigration);
    GateRef needCOW = IsJsCOWArray(glue, object);
    BRANCH(ElementsKindIsIntOrHoleInt(oldKind), &migrateFromInt, &migrateOtherKinds);
    Bind(&migrateFromInt);
    {
        Label migrateToHeapValuesFromInt(env);
        Label migrateToRawValuesFromInt(env);
        Label migrateToNumbersFromInt(env);
        BRANCH(ElementsKindIsHeapKind(newKind), &migrateToHeapValuesFromInt, &migrateToRawValuesFromInt);
        Bind(&migrateToHeapValuesFromInt);
        {
            newElements = MigrateFromRawValueToHeapValues(glue, object, needCOW, True());
            SetElementsArray(VariableType::JS_ANY(), glue, object, *newElements);
            Jump(&exit);
        }
        Bind(&migrateToRawValuesFromInt);
        {
            BRANCH(ElementsKindIsNumOrHoleNum(newKind), &migrateToNumbersFromInt, &exit);
            Bind(&migrateToNumbersFromInt);
            {
                MigrateFromHoleIntToHoleNumber(glue, object);
                Jump(&exit);
            }
        }
    }
    Bind(&migrateOtherKinds);
    {
        Label migrateFromNumber(env);
        Label migrateToHeapValuesFromNum(env);
        Label migrateToRawValuesFromNum(env);
        Label migrateToIntFromNum(env);
        Label migrateToRawValueFromTagged(env);
        BRANCH(ElementsKindIsNumOrHoleNum(oldKind), &migrateFromNumber, &migrateToRawValueFromTagged);
        Bind(&migrateFromNumber);
        {
            BRANCH(ElementsKindIsHeapKind(newKind), &migrateToHeapValuesFromNum, &migrateToRawValuesFromNum);
            Bind(&migrateToHeapValuesFromNum);
            {
                Label migrateToTaggedFromNum(env);
                BRANCH(ElementsKindIsHeapKind(newKind), &migrateToTaggedFromNum, &exit);
                Bind(&migrateToTaggedFromNum);
                {
                    newElements = MigrateFromRawValueToHeapValues(glue, object, needCOW, False());
                    SetElementsArray(VariableType::JS_ANY(), glue, object, *newElements);
                    Jump(&exit);
                }
            }
            Bind(&migrateToRawValuesFromNum);
            {
                BRANCH(ElementsKindIsIntOrHoleInt(newKind), &migrateToIntFromNum, &exit);
                Bind(&migrateToIntFromNum);
                {
                    MigrateFromHoleNumberToHoleInt(glue, object);
                    Jump(&exit);
                }
            }
        }
        Bind(&migrateToRawValueFromTagged);
        {
            Label migrateToIntFromTagged(env);
            Label migrateToOthersFromTagged(env);
            BRANCH(ElementsKindIsIntOrHoleInt(newKind), &migrateToIntFromTagged, &migrateToOthersFromTagged);
            Bind(&migrateToIntFromTagged);
            {
                newElements = MigrateFromHeapValueToRawValue(glue, object, needCOW, True());
                SetElementsArray(VariableType::JS_ANY(), glue, object, *newElements);
                Jump(&exit);
            }
            Bind(&migrateToOthersFromTagged);
            {
                Label migrateToNumFromTagged(env);
                BRANCH(ElementsKindIsNumOrHoleNum(newKind), &migrateToNumFromTagged, &exit);
                Bind(&migrateToNumFromTagged);
                {
                    newElements = MigrateFromHeapValueToRawValue(glue, object, needCOW, False());
                    SetElementsArray(VariableType::JS_ANY(), glue, object, *newElements);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::MigrateFromRawValueToHeapValues(GateRef glue, GateRef object, GateRef needCOW, GateRef isIntKind)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(newElements, VariableType::JS_ANY(), Undefined());
    Label exit(env);

    GateRef elements = GetElementsArray(glue, object);
    GateRef length = GetLengthOfTaggedArray(elements);
    Label createCOW(env);
    Label createNormal(env);
    Label finishElementsInit(env);
    BRANCH(needCOW, &createCOW, &createNormal);
    Bind(&createCOW);
    {
        newElements = CallRuntime(glue, RTSTUB_ID(NewCOWTaggedArray), { IntToTaggedPtr(length) });
        Jump(&finishElementsInit);
    }
    Bind(&createNormal);
    {
        newElements = CallRuntime(glue, RTSTUB_ID(NewTaggedArray), { IntToTaggedPtr(length) });
        Jump(&finishElementsInit);
    }
    Bind(&finishElementsInit);

    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label storeHole(env);
        Label storeNormalValue(env);
        Label finishStore(env);
        BRANCH(Int32UnsignedLessThan(*index, length), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            Label rawValueIsInt(env);
            Label rawValueIsNumber(env);
            GateRef value = GetValueFromMutantTaggedArray(elements, *index);
            BRANCH(ValueIsSpecialHole(value), &storeHole, &storeNormalValue);
            Bind(&storeHole);
            {
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *newElements, *index, Hole());
                Jump(&finishStore);
            }
            Bind(&storeNormalValue);
            {
                BRANCH(isIntKind, &rawValueIsInt, &rawValueIsNumber);
                Bind(&rawValueIsInt);
                {
                    GateRef convertedInt = Int64ToTaggedIntPtr(value);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, *newElements, *index, convertedInt);
                    Jump(&finishStore);
                }
                Bind(&rawValueIsNumber);
                {
                    GateRef tmpDouble = CastInt64ToFloat64(value);
                    GateRef convertedDouble = DoubleToTaggedDoublePtr(tmpDouble);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, *newElements, *index, convertedDouble);
                    Jump(&finishStore);
                }
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
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *newElements;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::MigrateFromHeapValueToRawValue(GateRef glue, GateRef object, GateRef needCOW, GateRef isIntKind)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(newElements, VariableType::JS_ANY(), Undefined());
    Label exit(env);

    GateRef elements = GetElementsArray(glue, object);
    GateRef length = GetLengthOfTaggedArray(elements);
    Label createCOW(env);
    Label createNormal(env);
    Label finishElementsInit(env);
    BRANCH(needCOW, &createCOW, &createNormal);
    Bind(&createCOW);
    {
        newElements = CallRuntime(glue, RTSTUB_ID(NewCOWMutantTaggedArray), { IntToTaggedPtr(length) });
        Jump(&finishElementsInit);
    }
    Bind(&createNormal);
    {
        newElements = CallRuntime(glue, RTSTUB_ID(NewMutantTaggedArray), { IntToTaggedPtr(length) });
        Jump(&finishElementsInit);
    }
    Bind(&finishElementsInit);

    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label storeSpecialHole(env);
        Label storeNormalValue(env);
        Label finishStore(env);
        BRANCH(Int32UnsignedLessThan(*index, length), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            Label convertToInt(env);
            Label convertToDouble(env);
            GateRef value = GetValueFromTaggedArray(glue, elements, *index);
            BRANCH(TaggedIsHole(value), &storeSpecialHole, &storeNormalValue);
            Bind(&storeSpecialHole);
            {
                SetValueToTaggedArray(VariableType::INT64(), glue, *newElements, *index, SpecialHole());
                Jump(&finishStore);
            }
            Bind(&storeNormalValue);
            {
                Label valueIsInt(env);
                Label valueIsDouble(env);
                BRANCH(isIntKind, &convertToInt, &convertToDouble);
                Bind(&convertToInt);
                {
                    GateRef convertedInt = GetInt64OfTInt(value);
                    SetValueToTaggedArray(VariableType::INT64(), glue, *newElements, *index, convertedInt);
                    Jump(&finishStore);
                }
                Bind(&convertToDouble);
                {
                    BRANCH(TaggedIsInt(value), &valueIsInt, &valueIsDouble);
                    Bind(&valueIsInt);
                    {
                        GateRef convertedDoubleFromTInt = CastDoubleToInt64(GetDoubleOfTInt(value));
                        SetValueToTaggedArray(VariableType::INT64(), glue, *newElements, *index,
                                              convertedDoubleFromTInt);
                        Jump(&finishStore);
                    }
                    Bind(&valueIsDouble);
                    {
                        GateRef convertedDoubleFromTDouble = CastDoubleToInt64(GetDoubleOfTDouble(value));
                        SetValueToTaggedArray(VariableType::INT64(), glue, *newElements, *index,
                                              convertedDoubleFromTDouble);
                        Jump(&finishStore);
                    }
                }
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
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *newElements;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::MigrateFromHoleIntToHoleNumber(GateRef glue, GateRef object)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label exit(env);

    GateRef elements = GetElementsArray(glue, object);
    GateRef length = GetLengthOfTaggedArray(elements);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label storeNormalValue(env);
        Label finishStore(env);
        BRANCH(Int32UnsignedLessThan(*index, length), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            GateRef value = GetValueFromMutantTaggedArray(elements, *index);
            BRANCH(ValueIsSpecialHole(value), &finishStore, &storeNormalValue);
            Bind(&storeNormalValue);
            {
                GateRef intVal = TruncInt64ToInt32(value);
                GateRef convertedValue = CastDoubleToInt64(ChangeInt32ToFloat64(intVal));
                SetValueToTaggedArray(VariableType::INT64(), glue, elements, *index,
                                      convertedValue);
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
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::MigrateFromHoleNumberToHoleInt(GateRef glue, GateRef object)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label exit(env);

    GateRef elements = GetElementsArray(glue, object);
    GateRef length = GetLengthOfTaggedArray(elements);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label storeValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label storeNormalValue(env);
        Label finishStore(env);
        BRANCH(Int32UnsignedLessThan(*index, length), &storeValue, &afterLoop);
        Bind(&storeValue);
        {
            GateRef value = GetValueFromMutantTaggedArray(elements, *index);
            BRANCH(ValueIsSpecialHole(value), &finishStore, &storeNormalValue);
            Bind(&storeNormalValue);
            {
                GateRef doubleVal = CastInt64ToFloat64(value);
                GateRef convertedValue = SExtInt32ToInt64(ChangeFloat64ToInt32(doubleVal));
                SetValueToTaggedArray(VariableType::INT64(), glue, elements, *index,
                                      convertedValue);
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
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::IsDetachedBuffer(GateRef glue, GateRef buffer)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(IsByteArray(glue, buffer), &isByteArray, &notByteArray);
    Bind(&isByteArray);
    {
        Jump(&exit);
    }
    Bind(&notByteArray);
    {
        GateRef dataSlot = GetArrayBufferData(glue, buffer);
        BRANCH(TaggedIsNull(dataSlot), &isNull, &exit);
        Bind(&isNull);
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

GateRef StubBuilder::DefineFunc(GateRef glue, GateRef constpool, GateRef index, FunctionKind targetKind)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(ihc, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(val, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label isHeapObject(env);
    Label afterAOTLiteral(env);
    Label tryGetAOTIhc(env);
    //AOT ihc infos always in unshareConstpool
    //If is runing on AOT,unshareConstpool is definitely not a hole
    //So wo can skip if unshareConstpool is hole
    GateRef unsharedConstpool = GetUnsharedConstpoolFromGlue(glue, constpool);
    BRANCH(TaggedIsHole(unsharedConstpool), &afterAOTLiteral, &tryGetAOTIhc);
    Bind(&tryGetAOTIhc);
    {
        val = GetValueFromTaggedArray(glue, unsharedConstpool, index);
        BRANCH(TaggedIsHeapObject(*val), &isHeapObject, &afterAOTLiteral);
        {
            Bind(&isHeapObject);
            Label isAOTLiteral(env);
            BRANCH(IsAOTLiteralInfo(glue, *val), &isAOTLiteral, &afterAOTLiteral);
            {
                Bind(&isAOTLiteral);
                {
                    ihc = GetIhcFromAOTLiteralInfo(glue, *val);
                    Jump(&afterAOTLiteral);
                }
            }
        }
    }
    Bind(&afterAOTLiteral);
    GateRef method = GetMethodFromConstPool(glue, constpool, index);
    Label isSendableFunc(env);
    Label isNotSendableFunc(env);
    Label afterDealWithCompiledStatus(env);
    BRANCH(IsSendableFunction(method), &isSendableFunc, &isNotSendableFunc);
    Bind(&isSendableFunc);
    {
        GateRef globalEnv = GetCurrentGlobalEnv();
        NewObjectStubBuilder newBuilder(this, globalEnv);
        result = newBuilder.NewJSSendableFunction(glue, method, targetKind);
        Jump(&afterDealWithCompiledStatus);
    }
    Bind(&isNotSendableFunc);
    {
        GateRef globalEnv = GetCurrentGlobalEnv();
        NewObjectStubBuilder newBuilder(this, globalEnv);
        result = newBuilder.NewJSFunction(glue, method, targetKind);
        Jump(&afterDealWithCompiledStatus);
    }
    Bind(&afterDealWithCompiledStatus);

    Label ihcNotUndefined(env);
    BRANCH(TaggedIsUndefined(*ihc), &exit, &ihcNotUndefined);
    Bind(&ihcNotUndefined);
    {
        CallRuntime(glue, RTSTUB_ID(AOTEnableProtoChangeMarker), {*result, *ihc, *val});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::SetCompiledCodeFlagToFunctionFromMethod(GateRef glue, GateRef function, GateRef method)
{
    // set compiled code & fast call flag
    GateRef isFastCall = IsFastCall(method);
    GateRef compiledCodeField = Int32LSL(Int32(1U), Int32(JSFunctionBase::IsCompiledCodeBit::START_BIT));
    GateRef compiledCodeFlag = Int32Or(compiledCodeField, Int32LSL(ZExtInt1ToInt32(isFastCall),
        Int32(JSFunctionBase::IsFastCallBit::START_BIT)));
    SetCompiledCodeFlagToFunction(glue, function, compiledCodeFlag);
}

void StubBuilder::UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                                      GateRef profileTypeInfo, GateRef slotId)
{
    Label subEntry(env_);
    env_->SubCfgEntry(&subEntry);

    Label profileTypeInfoNotUndefined(env_);
    Label slotValueUpdate(env_);
    Label slotValueNotUndefined(env_);
    Label profileTypeInfoEnd(env_);
    NewObjectStubBuilder newBuilder(env_);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &profileTypeInfoEnd, &profileTypeInfoNotUndefined);
    Bind(&profileTypeInfoNotUndefined);
    {
        GateRef slotValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);
        BRANCH(TaggedIsUndefined(slotValue), &slotValueUpdate, &slotValueNotUndefined);
        Bind(&slotValueUpdate);
        {
            GateRef newProfileTypeInfoCell = newBuilder.NewProfileTypeInfoCell(glue, Undefined());
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, newProfileTypeInfoCell);
            SetRawProfileTypeInfoToFunction(glue, function, newProfileTypeInfoCell);
            Jump(&profileTypeInfoEnd);
        }
        Bind(&slotValueNotUndefined);
        {
            UpdateProfileTypeInfoCellType(glue, slotValue);
            SetRawProfileTypeInfoToFunction(glue, function, slotValue);
            TryToJitReuseCompiledFunc(glue, function, slotValue);
            Jump(&profileTypeInfoEnd);
        }
    }
    Bind(&profileTypeInfoEnd);

    env_->SubCfgExit();
}

GateRef StubBuilder::Loadlocalmodulevar(GateRef glue, GateRef index, GateRef module)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label notSendableFunctionModule(env);
    Label sendableFunctionModule(env);
    Label exit(env);
    BRANCH_UNLIKELY(IsSendableFunctionModule(glue, module), &sendableFunctionModule, &notSendableFunctionModule);
    Bind(&sendableFunctionModule);
    {
        result = CallRuntime(glue, RTSTUB_ID(LdLocalModuleVarByIndexWithModule),
                             {Int8ToTaggedInt(index), module});
        Jump(&exit);
    }
    Bind(&notSendableFunctionModule);
    {
        GateRef dictionary = GetNameDictionary(glue, module);
        Label dataIsNotUndefined(env);
        BRANCH_UNLIKELY(TaggedIsUndefined(dictionary), &exit, &dataIsNotUndefined);
        Bind(&dataIsNotUndefined);
        {
            result = GetValueFromTaggedArray(glue, dictionary, ZExtInt8ToInt32(index));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::ModuleEnvMustBeValid(GateRef glue, GateRef curEnv)
{
    GateRef objectType = GetObjectType(LoadHClass(glue, curEnv));
    [[maybe_unused]] GateRef isTaggedArray = LogicOrBuilder(env_)
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::TAGGED_ARRAY))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::TAGGED_DICTIONARY))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::LEXICAL_ENV))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::SENDABLE_ENV))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::CONSTANT_POOL))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::PROFILE_TYPE_INFO))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::AOT_LITERAL_INFO))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::VTABLE))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::COW_TAGGED_ARRAY))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::MUTANT_TAGGED_ARRAY))))
        .Or(Int32Equal(objectType, Int32(static_cast<int>(JSType::COW_MUTANT_TAGGED_ARRAY))))
        .Done();
    ASSERT(isTaggedArray);
}

GateRef StubBuilder::SearchFromModuleCache(GateRef glue, GateRef moduleName)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label isContainModule(env);
    Label exit(env);
    GateRef moduleObj = GetCjsModuleFunction(glue);
    GateRef cacheName = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::CJS_CACHE_INDEX);
    GateRef moduleCache = FastGetPropertyByName(glue, moduleObj, cacheName, ProfileOperation());
    GateRef containsModuleIndex = FindEntryFromHashTable<CjsModuleCache>(glue, moduleCache, moduleName);
    BRANCH(Int32NotEqual(containsModuleIndex, Int32(-1)), &isContainModule, &exit);

    Bind(&isContainModule);
    {
        GateRef cachedModule = GetValueFromDictionary<CjsModuleCache>(glue, moduleCache, containsModuleIndex);
        GateRef exportsName = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::CJS_EXPORTS_INDEX);
        result = FastGetPropertyByName(glue, cachedModule, exportsName, ProfileOperation());
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetNativeOrCjsExports(GateRef glue, GateRef resolvedModule)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(exports, VariableType::JS_ANY(), Undefined());
    Label isNativeModule(env);
    Label judgeCjsModule(env);
    Label exit(env);
    BRANCH(IsNativeModule(resolvedModule), &isNativeModule, &judgeCjsModule);

    Bind(&isNativeModule);
    {
        Label notJSObject(env);
        exports = GetModuleValueByIndex(glue, resolvedModule, Int32(0), TaggedFalse());
        BRANCH(IsJSObject(glue, *exports), &exit, &notJSObject);

        Bind(&notJSObject);
        {
            CallNGCRuntime(glue, RTSTUB_ID(LoadNativeModuleFailed), {resolvedModule});
            Jump(&exit);
        }
    }

    Bind(&judgeCjsModule);
    {
        Label isCjsModule(env);
        BRANCH(IsCjsModule(resolvedModule), &isCjsModule, &exit);

        Bind(&isCjsModule);
        Label exportsIsHole(env);
        GateRef moduleName = CallRuntime(glue, RTSTUB_ID(GetModuleName), {resolvedModule});
        exports = SearchFromModuleCache(glue, moduleName);
        BRANCH(TaggedIsHole(*exports), &exportsIsHole, &exit);

        Bind(&exportsIsHole);
        {
            exports = CallRuntime(glue, RTSTUB_ID(ThrowExportsIsHole), {resolvedModule});
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *exports;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetValueFromExportObject(GateRef glue, GateRef exports, GateRef index)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label isUndefinedIdx(env);
    Label validIdx(env);
    Label checkResultIsAccessor(env);
    Label exit(env);
    BRANCH(Int32Equal(index, Int32(SourceTextModule::UNDEFINED_INDEX)), &isUndefinedIdx, &validIdx);

    Bind(&isUndefinedIdx);
    {
        result = exports;
        Jump(&exit);
    }

    Bind(&validIdx);
    {
        Label isDictionaryMode(env);
        Label notDictionaryMode(env);
        GateRef properties = GetPropertiesFromJSObject(glue, exports);
        BRANCH(IsDictionaryMode(glue, properties), &isDictionaryMode, &notDictionaryMode);

        Bind(&isDictionaryMode);
        {
            result = GetValueFromDictionary<NameDictionary>(glue, properties, index);
            Jump(&checkResultIsAccessor);
        }

        Bind(&notDictionaryMode);
        {
            GateRef hClass = LoadHClass(glue, exports);
            GateRef layoutInfo = GetLayoutFromHClass(glue, hClass);
            GateRef attr = GetAttr(glue, layoutInfo, index);
            result = JSObjectGetPropertyWithRep(glue, exports, hClass, attr);
            Jump(&checkResultIsAccessor);
        }
    }

    Bind(&checkResultIsAccessor);
    {
        Label isAccessor(env);
        BRANCH_UNLIKELY(TaggedIsAccessor(glue, *result), &isAccessor, &exit);

        Bind(&isAccessor);
        {
            result = CallGetterHelper(glue, exports, exports, *result, ProfileOperation());
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetNativeOrCjsModuleValue(GateRef glue, GateRef module, GateRef index)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label hasException(env);
    Label noException(env);
    Label exit(env);
    GateRef exports = GetNativeOrCjsExports(glue, module);
    BRANCH(HasPendingException(glue), &hasException, &noException);

    Bind(&hasException);
    {
        result = Exception();
        Jump(&exit);
    }

    Bind(&noException);
    {
        result = GetValueFromExportObject(glue, exports, index);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetModuleValueByIndex(GateRef glue, GateRef module, GateRef index, GateRef isThrow)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label isUndefined(env);
    Label notUndefined(env);
    Label exit(env);
    GateRef dictionary = GetNameDictionary(glue, module);
    BRANCH_UNLIKELY(TaggedIsUndefined(dictionary), &isUndefined, &notUndefined);

    Bind(&notUndefined);
    {
        result = GetValueFromTaggedArray(glue, dictionary, index);
        Jump(&exit);
    }

    Bind(&isUndefined);
    {
        result = CallRuntime(glue, RTSTUB_ID(CheckAndThrowModuleError), {module, isThrow});
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetModuleValue(GateRef glue, GateRef module, GateRef index)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label isNativeOrCjsModule(env);
    Label notNativeOrCjsModule(env);
    Label exit(env);
    GateRef checkNativeOrCjsModule = BitOr(IsNativeModule(module), IsCjsModule(module));
    BRANCH(checkNativeOrCjsModule, &isNativeOrCjsModule, &notNativeOrCjsModule);

    Bind(&isNativeOrCjsModule);
    {
        result = GetNativeOrCjsModuleValue(glue, module, index);
        Jump(&exit);
    }

    Bind(&notNativeOrCjsModule);
    {
        result = GetModuleValueByIndex(glue, module, index, TaggedFalse());
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetNativeOrCjsModuleValueByName(GateRef glue, GateRef module, GateRef bindingName)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    GateRef exports = GetNativeOrCjsExports(glue, module);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label hasPendingException(env);
    Label noPendingException(env);
    Label exit(env);
    BRANCH(HasPendingException(glue), &hasPendingException, &noPendingException);

    Bind(&hasPendingException);
    {
        result = Exception();
        Jump(&exit);
    }

    Bind(&noPendingException);
    {
        GateRef defaultString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::DEFAULT_INDEX);
        Label isSameVal(env);
        Label notSameVal(env);
        BRANCH_UNLIKELY(SameValue(glue, bindingName, defaultString), &isSameVal, &notSameVal);

        Bind(&isSameVal);
        {
            result = exports;
            Jump(&exit);
        }

        Bind(&notSameVal);
        {
            result = FastGetPropertyByName(glue, exports, bindingName, ProfileOperation());
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ResolveElementOfObject(GateRef glue, GateRef hClass, GateRef exportName,
                                            GateRef module, GateRef layOutInfo)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label validIdx(env);
    Label invalidIdx(env);
    Label exit(env);
    GateRef propsNum = GetNumberOfPropsFromHClass(hClass);
    GateRef idx = FindElementWithCache(glue, layOutInfo, hClass, exportName, propsNum);
    BRANCH(Int32NotEqual(idx, Int32(-1)), &validIdx, &invalidIdx);

    Bind(&validIdx);
    {
        result = CallRuntime(glue, RTSTUB_ID(NewResolvedIndexBindingRecord), {module, IntToTaggedInt(idx)});
        Jump(&exit);
    }

    Bind(&invalidIdx);
    {
        result = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::UNDEFINED_INDEX);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ResolveExportObject(GateRef glue, GateRef module, GateRef exports, GateRef exportName)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label defaultCallNewBindingRecord(env);
    Label notSameVal(env);
    Label exportsIsHeapObj(env);
    Label notNativeModuleFailureInfo(env);
    Label exit(env);
    GateRef defaultString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::DEFAULT_INDEX);
    BRANCH(SameValue(glue, exportName, defaultString), &defaultCallNewBindingRecord, &notSameVal);

    Bind(&notSameVal);
    BRANCH(TaggedIsHeapObject(exports), &exportsIsHeapObj, &notNativeModuleFailureInfo);

    Bind(&exportsIsHeapObj);
    BRANCH(IsNativeModuleFailureInfo(glue, exports), &defaultCallNewBindingRecord, &notNativeModuleFailureInfo);

    Bind(&notNativeModuleFailureInfo);
    Label exportsIsJSObject(env);
    Label exportsNotJSObject(env);
    BRANCH(IsJSObject(glue, exports), &exportsIsJSObject, &exportsNotJSObject);

    Bind(&exportsIsJSObject);
    {
        Label isDictionaryMode(env);
        Label notDictionaryMode(env);
        Label checkResultIsUndefined(env);
        GateRef properties = GetPropertiesFromJSObject(glue, exports);
        BRANCH(IsDictionaryMode(glue, properties), &isDictionaryMode, &notDictionaryMode);

        Bind(&isDictionaryMode);
        {
            Label validEntry(env);
            GateRef entry = FindEntryFromHashTable<NameDictionary>(glue, properties, exportName);
            BRANCH(Int32NotEqual(entry, Int32(-1)), &validEntry, &checkResultIsUndefined);

            Bind(&validEntry);
            result = CallRuntime(glue, RTSTUB_ID(NewResolvedIndexBindingRecord), {module, IntToTaggedInt(entry)});
            Jump(&checkResultIsUndefined);
        }

        Bind(&notDictionaryMode);
        {
            Label notEqualZero(env);
            GateRef hClass = LoadHClass(glue, exports);
            GateRef layoutInfo = GetLayoutFromHClass(glue, hClass);
            GateRef eleNum = GetNumberOfPropsFromHClass(hClass);
            BRANCH(Int32NotEqual(eleNum, Int32(0)), &notEqualZero, &checkResultIsUndefined);

            Bind(&notEqualZero);
            result = ResolveElementOfObject(glue, hClass, exportName, module, layoutInfo);
            Jump(&checkResultIsUndefined);
        }

        Bind(&checkResultIsUndefined);
        BRANCH(TaggedIsUndefined(*result), &exportsNotJSObject, &exit);
    }

    Bind(&defaultCallNewBindingRecord);
    {
        result = CallRuntime(glue, RTSTUB_ID(NewResolvedIndexBindingRecord), {module, IntToTaggedInt(Int32(-1))});
        Jump(&exit);
    }

    Bind(&exportsNotJSObject);
    {
        result = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::NULL_INDEX);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::UpdateBindingAndGetModuleValue(GateRef glue, GateRef module, GateRef requiredModule,
                                                    GateRef index, GateRef bindingName)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label hasException(env);
    Label noException(env);
    Label exit(env);
    GateRef curModuleEnv = GetCurrentModuleEnv(glue, module);
    ModuleEnvMustBeDefined(curModuleEnv);
    GateRef exports = GetNativeOrCjsExports(glue, requiredModule);
    BRANCH(HasPendingException(glue), &hasException, &noException);

    Bind(&noException);
    {
        Label isNullOrString(env);
        Label notNullOrString(env);
        GateRef resolution = ResolveExportObject(glue, requiredModule, exports, bindingName);
        BRANCH(BitOr(TaggedIsNull(resolution), TaggedIsString(glue, resolution)), &isNullOrString, &notNullOrString);

        Bind(&isNullOrString);
        {
            result = CallRuntime(glue, RTSTUB_ID(HandleResolutionIsNullOrString),
                                 {module, requiredModule, bindingName, resolution});
            Jump(&exit);
        }

        Bind(&notNullOrString);
        {
            CheckIsResolvedIndexBinding(glue, resolution);
            SetIsUpdatedFromResolvedBindingOfResolvedIndexBinding(glue, resolution, True());
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, curModuleEnv, index, resolution);
            result = GetValueFromExportObject(glue, exports, GetIdxOfResolvedIndexBinding(resolution));
            Jump(&exit);
        }
    }

    Bind(&hasException);
    {
        result = Exception();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetResolvedRecordIndexBindingModule(GateRef glue, GateRef module, GateRef resolvedBinding)
{
    GateRef recordName = GetModuleRecord(glue, resolvedBinding);
    RecordNameMustBeString(glue, recordName);
    GateRef moduleManager = GetModuleManager(glue);
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetResolvedRecordIndexBindingModule),
                                 {module, resolvedBinding, moduleManager, recordName});
    return result;
}

GateRef StubBuilder::GetResolvedRecordBindingModule(GateRef glue, GateRef module, GateRef resolvedBinding)
{
    GateRef recordName = GetModuleRecord(glue, resolvedBinding);
    RecordNameMustBeString(glue, recordName);
    GateRef moduleManager = GetModuleManager(glue);
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetResolvedRecordBindingModule),
                                 {module, moduleManager, recordName});
    return result;
}

GateRef StubBuilder::LoadExternalmodulevar(GateRef glue, GateRef index, GateRef curModule)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label notSendableFunctionModule(env);
    Label moduleUndefined(env);
    Label moduleIsdefined(env);
    Label moduleEnvUndefined(env);
    Label moduleEnvIsdefined(env);
    Label isSendableFunctionModule(env);
    Label isNullPtr(env);
    Label notNullPtr(env);
    Label resolvedBindingIsHeapObj(env);
    Label misstakenResolvedBinding(env);
    Label judgeResolvedBinding(env);
    Label judgeResolvedRecordIndexBinding(env);
    Label judgeResolvedRecordBinding(env);
    Label exit(env);
    BRANCH(IsSendableFunctionModule(glue, curModule), &isSendableFunctionModule, &notSendableFunctionModule);

    Bind(&notSendableFunctionModule);
    BRANCH_UNLIKELY(TaggedIsUndefined(curModule), &moduleUndefined, &moduleIsdefined);

    Bind(&moduleIsdefined);
    GateRef curModuleEnv = GetCurrentModuleEnv(glue, curModule);
    BRANCH(TaggedIsUndefined(curModuleEnv), &moduleEnvUndefined, &moduleEnvIsdefined);

    Bind(&moduleEnvIsdefined);
    ModuleEnvMustBeValid(glue, curModuleEnv);
    GateRef resolvedBinding = GetValueFromTaggedArray(glue, curModuleEnv, index);
    BRANCH_LIKELY(IntPtrEuqal(GetModuleLogger(glue), IntPtr(0)), &isNullPtr, &notNullPtr);

    Bind(&isNullPtr);
    BRANCH(TaggedIsHeapObject(resolvedBinding), &resolvedBindingIsHeapObj, &misstakenResolvedBinding);

    Bind(&resolvedBindingIsHeapObj);
    {
        Label isResolvedIndexBinding(env);
        BRANCH(IsResolvedIndexBinding(glue, resolvedBinding), &isResolvedIndexBinding, &judgeResolvedBinding);

        Bind(&isResolvedIndexBinding);
        {
            Label isLdEndExecPatchMain(env);
            Label notLdEndExecPatchMain(env);
            Label notHole(env);
            DEFVARIABLE(resolvedModule, VariableType::JS_ANY(), Hole());
            resolvedModule = GetResolveModuleFromResolvedIndexBinding(glue, resolvedBinding);
            ResolvedModuleMustBeSourceTextModule(glue, *resolvedModule);
            GateRef idxOfResolvedBinding = GetIdxOfResolvedIndexBinding(resolvedBinding);
            BRANCH(IsLdEndExecPatchMain(glue), &isLdEndExecPatchMain, &notLdEndExecPatchMain);

            Bind(&isLdEndExecPatchMain);
            GateRef resolvedModuleOfHotReload = CallNGCRuntime(glue, RTSTUB_ID(FindPatchModule),
                                                               {glue, *resolvedModule});
            BRANCH(TaggedIsHole(resolvedModuleOfHotReload), &notLdEndExecPatchMain, &notHole);

            Bind(&notLdEndExecPatchMain);
            Label isSharedModule(env);
            Label notSharedModule(env);
            BRANCH(IsSharedModule(*resolvedModule), &isSharedModule, &notSharedModule);
            Bind(&isSharedModule);
            resolvedModule = CallNGCRuntime(glue, RTSTUB_ID(UpdateSharedModule), {glue, *resolvedModule});
            Jump(&notSharedModule);
            Bind(&notSharedModule);
            result = GetModuleValue(glue, *resolvedModule, idxOfResolvedBinding);
            Jump(&exit);

            Bind(&notHole);
            result = GetModuleValue(glue, resolvedModuleOfHotReload, idxOfResolvedBinding);
            Jump(&exit);
        }
    }

    Bind(&judgeResolvedBinding);
    {
        Label isResolvedBinding(env);
        BRANCH(IsResolvedBinding(glue, resolvedBinding), &isResolvedBinding, &judgeResolvedRecordIndexBinding);

        Bind(&isResolvedBinding);
        {
            GateRef resolvedModule = GetResolveModuleFromResolvedBinding(glue, resolvedBinding);
            ResolvedModuleMustBeSourceTextModule(glue, resolvedModule);
            Label isNativeOrCjsModule(env);
            GateRef checkNativeOrCjsModule = BitOr(IsNativeModule(resolvedModule), IsCjsModule(resolvedModule));
            BRANCH(checkNativeOrCjsModule, &isNativeOrCjsModule, &misstakenResolvedBinding);

            Bind(&isNativeOrCjsModule);
            {
                result = UpdateBindingAndGetModuleValue(glue, curModule, resolvedModule, index,
                                                        GetBindingName(glue, resolvedBinding));
                Jump(&exit);
            }
        }
    }

    Bind(&judgeResolvedRecordIndexBinding);
    {
        Label isResolvedRecordIndexBinding(env);
        BRANCH(IsResolvedRecordIndexBinding(glue, resolvedBinding), &isResolvedRecordIndexBinding,
               &judgeResolvedRecordBinding);

        Bind(&isResolvedRecordIndexBinding);
        {
            GateRef resolvedModule = GetResolvedRecordIndexBindingModule(glue, curModule, resolvedBinding);
            GateRef idxOfResolvedRecordIndexBinding = GetIdxOfResolvedRecordIndexBinding(resolvedBinding);
            result = GetModuleValue(glue, resolvedModule, idxOfResolvedRecordIndexBinding);
            Jump(&exit);
        }
    }

    Bind(&judgeResolvedRecordBinding);
    {
        Label isResolvedRecordBinding(env);
        BRANCH(IsResolvedRecordBinding(glue, resolvedBinding), &isResolvedRecordBinding, &misstakenResolvedBinding);
        Bind(&isResolvedRecordBinding);
        {
            GateRef resolvedModule = GetResolvedRecordBindingModule(glue, curModule, resolvedBinding);
            result = GetNativeOrCjsModuleValueByName(glue, resolvedModule, GetBindingName(glue, resolvedBinding));
            Jump(&exit);
        }
    }

    Bind(&isSendableFunctionModule);
    {
        result = CallRuntime(glue, RTSTUB_ID(LdExternalModuleVarByIndexWithModule),
                             {IntToTaggedInt(index), curModule});
        Jump(&exit);
    }

    Bind(&moduleUndefined);
    {
        FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(CurrentModuleUndefined))});
        Jump(&exit);
    }

    Bind(&moduleEnvUndefined);
    {
        result = Undefined();
        Jump(&exit);
    }

    Bind(&notNullPtr);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetModuleValueOuterInternal),
                             {curModule, IntToTaggedInt(index)});
        Jump(&exit);
    }

    Bind(&misstakenResolvedBinding);
    {
        CallNGCRuntime(glue, RTSTUB_ID(FatalPrintMisstakenResolvedBinding), {index, curModule});
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::LoadModuleNamespaceByIndex(GateRef glue, GateRef index, GateRef module)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label moduleUndefined(env);
    Label moduleNotUndefined(env);
    Label notSendableFunctionModule(env);
    Label requestedModulesNotUndefined(env);
    Label requiredModuleIsHeapObj(env);
    Label requiredModuleIsSourceTextModule(env);
    Label isNullPtr(env);
    Label isNativeModule(env);
    Label dictionaryNotUndefined(env);
    Label notNativeModule(env);
    Label isCjsModule(env);
    Label notCjsModule(env);
    Label namespaceNotUndefined(env);
    Label slowPath(env);
    Label exit(env);
    BRANCH_UNLIKELY(TaggedIsUndefined(module), &moduleUndefined, &moduleNotUndefined);
    Bind(&moduleUndefined);
    {
        FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(CurrentModuleUndefined))});
        Jump(&exit);
    }
    Bind(&moduleNotUndefined);
    BRANCH_UNLIKELY(IsSendableFunctionModule(glue, module), &slowPath, &notSendableFunctionModule);
    Bind(&notSendableFunctionModule);
    {
        GateRef requestedModules = GetRequestedModules(glue, module);
        BRANCH_UNLIKELY(TaggedIsUndefined(requestedModules), &slowPath, &requestedModulesNotUndefined);
        Bind(&requestedModulesNotUndefined);
        GateRef requiredModule = GetValueFromTaggedArray(glue, requestedModules, index);
        BRANCH_LIKELY(TaggedIsHeapObject(requiredModule), &requiredModuleIsHeapObj, &slowPath);
        Bind(&requiredModuleIsHeapObj);
        BRANCH_LIKELY(IsSourceTextModule(glue, requiredModule), &requiredModuleIsSourceTextModule, &slowPath);
        Bind(&requiredModuleIsSourceTextModule);
        BRANCH_LIKELY(IntPtrEuqal(GetModuleLogger(glue), IntPtr(0)), &isNullPtr, &slowPath);
        Bind(&isNullPtr);
        BRANCH(IsNativeModule(requiredModule), &isNativeModule, &notNativeModule);
        Bind(&isNativeModule);
        {
            GateRef dictionary = GetNameDictionary(glue, requiredModule);
            BRANCH_UNLIKELY(TaggedIsUndefined(dictionary), &slowPath, &dictionaryNotUndefined);
            Bind(&dictionaryNotUndefined);
            result = GetValueFromTaggedArray(glue, dictionary, Int32(0));
            Jump(&exit);
        }
        Bind(&notNativeModule);
        BRANCH(IsCjsModule(requiredModule), &isCjsModule, &notCjsModule);
        Bind(&isCjsModule);
        {
            // CommonJS module involves CString, jump to slowPath here
            Jump(&slowPath);
        }
        Bind(&notCjsModule);
        {
            // requiredModule is ESM
            GateRef moduleNamespace = GetNamespaceFromSourceTextModule(glue, requiredModule);
            BRANCH_UNLIKELY(TaggedIsUndefined(moduleNamespace), &slowPath, &namespaceNotUndefined);
            Bind(&namespaceNotUndefined);
            result = moduleNamespace;
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetModuleNamespaceByIndex), { IntToTaggedInt(index) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// Used for jit machine code reusing of inner functions have the same method to improve performance.
void StubBuilder::TryToJitReuseCompiledFunc(GateRef glue, GateRef jsFunc, GateRef profileTypeInfoCell)
{
    Label subEntry(env_);
    env_->SubCfgEntry(&subEntry);

    Label machineCodeIsNotHole(env_);
    Label exitPoint(env_);
    Label hasNotDisable(env_);
    GateRef weakMachineCode = Load(VariableType::JS_ANY(), glue, profileTypeInfoCell,
                                   IntPtr(ProfileTypeInfoCell::MACHINE_CODE_OFFSET));
    BRANCH(TaggedIsHole(weakMachineCode), &exitPoint, &machineCodeIsNotHole);
    Bind(&machineCodeIsNotHole);
    {
        Label hasProfileTypeInfo(env_);
        GateRef profileTypeInfo = Load(VariableType::JS_ANY(), glue, profileTypeInfoCell,
                                       IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
        BRANCH(TaggedIsUndefined(profileTypeInfo), &exitPoint, &hasProfileTypeInfo);
        Bind(&hasProfileTypeInfo);
        {
            GateRef jitHotnessThreshold = ProfilerStubBuilder(env_).GetJitHotnessThreshold(profileTypeInfo);
            BRANCH(Int32Equal(jitHotnessThreshold, Int32(ProfileTypeInfo::JIT_DISABLE_FLAG)), &exitPoint, &hasNotDisable);
            Bind(&hasNotDisable);
            {
                Label machineCodeIsUndefine(env_);
                Label machineCodeIsNotUndefine(env_);
                BRANCH(TaggedIsUndefined(weakMachineCode), &machineCodeIsUndefine, &machineCodeIsNotUndefine);
                Bind(&machineCodeIsUndefine);
                {
                    ProfilerStubBuilder(env_).SetJitHotnessCnt(glue, profileTypeInfo, Int16(0));
                    Store(VariableType::JS_POINTER(), glue, profileTypeInfoCell,
                        IntPtr(ProfileTypeInfoCell::MACHINE_CODE_OFFSET), Hole());
                    Jump(&exitPoint);
                }
                Bind(&machineCodeIsNotUndefine);
                {
                    GateRef machineCode = TaggedCastToIntPtr(RemoveTaggedWeakTag(weakMachineCode));
                    GateRef codeAddr = Load(VariableType::NATIVE_POINTER(), glue, machineCode,
                                            IntPtr(MachineCode::FUNCADDR_OFFSET));
                    ASSERT(IntPtrNotEqual(codeAddr, IntPtr(0)));
                    GateRef isFastCall = GetIsFastCall(machineCode);
                    SetCompiledFuncEntry(glue, jsFunc, codeAddr, ZExtInt1ToInt32(isFastCall));
                    SetMachineCodeToFunction(glue, jsFunc, machineCode);
                    Jump(&exitPoint);
                }
            }
        }
    }
    Bind(&exitPoint);
    env_->SubCfgExit();
}

GateRef StubBuilder::GetArgumentsElements(GateRef glue, GateRef argvTaggedArray, GateRef argv)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::NATIVE_POINTER(), NullPtr());
    DEFVARIABLE(argvVar, VariableType::NATIVE_POINTER(), argv);

    Label calcArgv(env);
    Label hasArgv(env);
    Label argvTaggedArrayUndef(env);
    Label argvTaggedArrayDef(env);

    BRANCH(TaggedIsUndefined(argvTaggedArray), &argvTaggedArrayUndef, &argvTaggedArrayDef);
    Bind(&argvTaggedArrayUndef);

    BRANCH(Equal(*argvVar, IntPtr(0)), &calcArgv, &hasArgv);
    Bind(&calcArgv);
    argvVar = CallNGCRuntime(glue, RTSTUB_ID(GetActualArgvNoGC), { glue });
    Jump(&hasArgv);

    Bind(&argvTaggedArrayDef);
    argvVar = PtrAdd(TaggedCastToIntPtr(argvTaggedArray), IntPtr(TaggedArray::DATA_OFFSET));
    Jump(&hasArgv);

    Bind(&hasArgv);
    result = PtrAdd(*argvVar, IntPtr(NUM_MANDATORY_JSFUNC_ARGS * 8)); // 8: ptr size
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ComputeTaggedArrayElementKind(GateRef glue, GateRef array, GateRef offset, GateRef end)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef elements = GetElementsArray(glue, array);
    GateRef kind = GetElementsKindFromHClass(LoadHClass(glue, array));
    Label fastCompute(env);
    Label slowCompute(env);
    GateRef checkType = LogicOrBuilder(env)
                        .Or(Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::NONE))))
                        .Or(Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::INT))))
                        .Or(Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::STRING))))
                        .Or(Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::OBJECT))))
                        .Or(Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::HOLE))))
                        .Done();
    BRANCH(checkType, &fastCompute, &slowCompute);
    Bind(&fastCompute);
    {
        result = kind;
        Jump(&exit);
    }
    Bind(&slowCompute);
    Label loopHead(env);
    Label loopEnd(env);
    Label doLoop(env);
    Label loopExit(env);
    DEFVARIABLE(i, VariableType::INT64(), offset);
    GateRef generic = Int32(Elements::ToUint(ElementsKind::GENERIC));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef checkType2 = BitAnd(Int64LessThan(*i, end), Int32LessThan(*result, generic));
        BRANCH(checkType2, &doLoop, &loopExit);
        Bind(&doLoop);
        GateRef value = GetValueFromTaggedArray(glue, elements, *i);
        result = Int32Or(TaggedToElementKind(glue, value), *result);
        i = Int64Add(*i, Int64(1));
        Jump(&loopEnd);
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetElementsKindHClass(GateRef glue, GateRef elementKind)
{
    auto env = GetEnvironment();
    GateRef offset = PtrMul(ZExtInt32ToPtr(elementKind), IntPtr(sizeof(ElementsHClassEntries::Entry)));
    GateRef arrayHClassIndexesOff = IntPtr(JSThread::GlueData::GetArrayHClassIndexesIndexOffset(env->Is32Bit()));
    GateRef arrayIndexes = PtrAdd(glue, arrayHClassIndexesOff);
    GateRef elementIdx = LoadPrimitive(VariableType::INT16(), arrayIndexes, offset);
    GateRef currentEnv = GetCurrentGlobalEnv();
    GateRef elementOffset = PtrAdd(IntPtr(GlobalEnv::HEADER_SIZE),
                                   PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), ZExtInt16ToPtr(elementIdx)));
    return Load(VariableType::JS_ANY(), glue, currentEnv, elementOffset);
}

GateRef StubBuilder::NeedBarrier(GateRef kind){
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    GateRef isInt = LogicAndBuilder(env)
                    .And(Int32GreaterThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::INT))))
                    .And(Int32LessThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::HOLE_INT))))
                    .Done();
    GateRef isNumber = LogicAndBuilder(env)
                       .And(Int32GreaterThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::NUMBER))))
                       .And(Int32LessThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER))))
                       .Done();
    GateRef check = LogicOrBuilder(env).Or(isInt).Or(isNumber)
                    .Or(Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::HOLE)))).Done();
    result = BoolNot(check);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::StartTraceLoadValueDetail([[maybe_unused]] GateRef glue, [[maybe_unused]] GateRef receiver,
    [[maybe_unused]] GateRef profileTypeInfo, [[maybe_unused]] GateRef slotId, [[maybe_unused]] GateRef key)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD_VALUE
    CallRuntime(glue, RTSTUB_ID(TraceLoadValueDetail), {receiver, profileTypeInfo, slotId, key});
#endif
}


void StubBuilder::StartTraceLoadValueSlowPath([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD_VALUE
    CallRuntime(glue, RTSTUB_ID(TraceLoadValueSlowPath), {});
#endif
}

void StubBuilder::EndTraceLoadValue([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD_VALUE
    CallRuntime(glue, RTSTUB_ID(TraceLoadValueEnd), {});
#endif
}

void StubBuilder::StartTraceCallDetail([[maybe_unused]] GateRef glue, [[maybe_unused]] GateRef profileTypeInfo,
                                       [[maybe_unused]] GateRef slotId)
{
#if ECMASCRIPT_ENABLE_TRACE_CALL
    CallRuntime(glue, RTSTUB_ID(TraceCallDetail), {profileTypeInfo, slotId});
#endif
}

void StubBuilder::EndTraceCall([[maybe_unused]] GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_CALL
    CallRuntime(glue, RTSTUB_ID(TraceLoadValueEnd), {});
#endif
}

void StubBuilder::StartTraceLoadDetail([[maybe_unused]] GateRef glue, [[maybe_unused]] GateRef receiver,
                                       [[maybe_unused]] GateRef profileTypeInfo, [[maybe_unused]] GateRef slotId)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    CallRuntime(glue, RTSTUB_ID(TraceLoadDetail), {receiver, profileTypeInfo, slotId});
#endif
}

void StubBuilder::StartTraceLoadGetter([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    CallRuntime(glue, RTSTUB_ID(TraceLoadGetter), {});
#endif
}

void StubBuilder::StartTraceLoadSlowPath([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    CallRuntime(glue, RTSTUB_ID(TraceLoadSlowPath), {});
#endif
}

void StubBuilder::EndTraceLoad([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    CallRuntime(glue, RTSTUB_ID(TraceLoadEnd), {});
#endif
}

void StubBuilder::StartTraceStoreDetail([[maybe_unused]] GateRef glue, [[maybe_unused]] GateRef receiver,
                                       [[maybe_unused]] GateRef profileTypeInfo, [[maybe_unused]] GateRef slotId)
{
#if ECMASCRIPT_ENABLE_TRACE_STORE
    CallRuntime(glue, RTSTUB_ID(TraceStoreDetail), {receiver, profileTypeInfo, slotId});
#endif
}

void StubBuilder::StartTraceStoreFastPath([[maybe_unused]] GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_STORE
    CallRuntime(glue, RTSTUB_ID(TraceStoreFastPath), {});
#endif
}

void StubBuilder::StartTraceStoreSlowPath([[maybe_unused]] GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_STORE
    CallRuntime(glue, RTSTUB_ID(TraceStoreSlowPath), {});
#endif
}

void StubBuilder::EndTraceStore([[maybe_unused]] GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_STORE
    CallRuntime(glue, RTSTUB_ID(TraceStoreEnd), {});
#endif
}

void StubBuilder::StartTraceDefineFunc([[maybe_unused]] GateRef glue, [[maybe_unused]] GateRef methodId,
                                       [[maybe_unused]] GateRef profileTypeInfo, [[maybe_unused]] GateRef slotId)
{
#if ECMASCRIPT_ENABLE_TRACE_DEFINEFUNC
    CallRuntime(glue, RTSTUB_ID(TraceDefineFunc), {methodId, profileTypeInfo, IntToTaggedInt(slotId)});
#endif
}
void StubBuilder::EndTraceDefineFunc([[maybe_unused]] GateRef glue)
{
#if ECMASCRIPT_ENABLE_TRACE_DEFINEFUNC
    CallRuntime(glue, RTSTUB_ID(TraceDefineFuncEnd), {});
#endif
}

void StubBuilder::UpdateProfileTypeInfoAsMega(GateRef glue, GateRef profileTypeInfo, GateRef slotId)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label pgoOn(env);
    Label pgoOff(env);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &pgoOff, &pgoOn);
    Bind(&pgoOn);
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, Hole(), MemoryAttribute::NoBarrier());
    Jump(&pgoOff);
    Bind(&pgoOff);
    env->SubCfgExit();
}
GateRef StubBuilder::JSTaggedValueToString(GateRef glue, GateRef val, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(value, VariableType::JS_ANY(), val);
    Label exit(env);
    Label notString(env);
    Label isSpecial(env);
    Label notSpecial(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(TaggedIsString(glue, *value), &exit, &notString);
        Bind(&notString);
        {
            BRANCH(TaggedIsSpecial(*value), &isSpecial, &notSpecial);
            Bind(&isSpecial);
            {
                value = SpecialToString(glue, *value);
                Jump(&exit);
            }
            Bind(&notSpecial);
            {
                Label numberBigIntPointer(env);
                Label notNumberBigIntPointer(env);
                GateRef checkValue = *value;
                GateRef checkType = LogicOrBuilder(env)
                                    .Or(TaggedIsNumber(checkValue))
                                    .Or(TaggedIsBigInt(glue, checkValue))
                                    .Or(TaggedIsNativePointer(glue, checkValue))
                                    .Done();
                BRANCH(checkType, &numberBigIntPointer, &notNumberBigIntPointer);
                Bind(&numberBigIntPointer);
                {
                    value = CallRuntime(glue, RTSTUB_ID(NumberBigIntNativePointerToString), { *value });
                    Jump(&exit);
                }
                Bind(&notNumberBigIntPointer);
                {
                    Label isEcmaObject1(env);
                    Label notEcmaObject1(env);
                    BRANCH(IsEcmaObject(glue, *value), &isEcmaObject1, &notEcmaObject1);
                    Bind(&isEcmaObject1);
                    {
                        value = ToPrimitive(glue, *value, PreferredPrimitiveType::PREFER_STRING, hir);
                        Label hasException(env);
                        BRANCH(HasPendingException(glue), &hasException, &loopEnd);
                        Bind(&hasException);
                        {
                            value = GetGlobalConstantValue(VariableType::JS_POINTER(),
                                                           glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
                            Jump(&exit);
                        }
                    }
                    Bind(&notEcmaObject1);
                    {
                        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertIllageValueToString));
                        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                        value = GetGlobalConstantValue(VariableType::JS_POINTER(),
                                                       glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&exit);
    auto ret = *value;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::SpecialToString(GateRef glue, GateRef specialVal)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label exit(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    Label labelBuffer[5] = { Label(env), Label(env), Label(env), Label(env), Label(env) };
    Label defaultLabel(env);
    Switch(ChangeTaggedPointerToInt64(specialVal), &defaultLabel, SPECIAL_VALUE, labelBuffer, SPECIAL_VALUE_NUM);
    for (int i = 0; i < SPECIAL_VALUE_NUM; i++) {
        Bind(&labelBuffer[i]);
        value = GetGlobalConstantValue(VariableType::JS_ANY(), glue, SPECIAL_STRING_INDEX[i]);
        Jump(&exit);
    }
    Bind(&defaultLabel);
    {
        FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable))});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *value;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::ToPrimitive(GateRef glue, GateRef value, PreferredPrimitiveType type, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label isEcmaObject(env);
    Label exit(env);
    Label hasException(env);
    Label notHasException(env);
    Label notHasException1(env);
    Label notHasException2(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), value);
    BRANCH(IsEcmaObject(glue, value), &isEcmaObject, &exit);
    Bind(&isEcmaObject);
    {
        Label isUndefined(env);
        Label notUndefined(env);
        GateRef globalEnv = GetCurrentGlobalEnv();
        GateRef primitiveKey = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                 GlobalEnv::TOPRIMITIVE_SYMBOL_INDEX);
        GateRef name = FastGetPropertyByName(glue, value, primitiveKey, ProfileOperation(), hir);
        BRANCH(HasPendingException(glue), &hasException, &notHasException);
        Bind(&notHasException);
        GateRef exoticToprim = CallFunction(glue, name);
        BRANCH(HasPendingException(glue), &hasException, &notHasException1);
        Bind(&notHasException1);
        BRANCH(TaggedIsUndefined(exoticToprim), &isUndefined, &notUndefined);
        Bind(&notUndefined);
        {
            GateRef typeValue = GetPrimitiveTypeString(glue, type);
            DEFVARIABLE(tmpResult, VariableType::JS_ANY(), Undefined());
            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG2_WITH_RETURN);
            callArgs.callThisArg2WithReturnArgs = {value, typeValue, Undefined()};
            CallStubBuilder callBuilder(this, glue, exoticToprim, Int32(2), 0, &tmpResult, Circuit::NullGate(),
                                        callArgs, ProfileOperation(), true, hir);
            Label callExit(env);
            if (env->IsBaselineBuiltin()) {
                callBuilder.JSCallDispatchForBaseline(&callExit);
                Bind(&callExit);
            } else {
                tmpResult = callBuilder.JSCallDispatch();
            }
            BRANCH(HasPendingException(glue), &hasException, &notHasException2);
            Bind(&notHasException2);
            Label isEcmaObject1(env);
            Label notEcmaObject1(env);
            BRANCH(IsEcmaObject(glue, *tmpResult), &isEcmaObject1, &notEcmaObject1);
            Bind(&isEcmaObject1);
            {
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertObjectToPrimitiveValue));
                CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
                Jump(&hasException);
            }
            Bind(&notEcmaObject1);
            {
                result = *tmpResult;
                Jump(&exit);
            }
        }
        Bind(&isUndefined);
        {
            Label numberPreference(env);
            Label defaultPreference(env);
            BRANCH(Int32Equal(Int32(static_cast<uint8_t>(type)),
                Int32(static_cast<uint8_t>(PreferredPrimitiveType::NO_PREFERENCE))),
                &numberPreference, &defaultPreference);
            Bind(&numberPreference);
            {
                result = OrdinaryToPrimitive(glue, value, PreferredPrimitiveType::PREFER_NUMBER, hir);
                Jump(&exit);
            }
            Bind(&defaultPreference);
            {
                result = OrdinaryToPrimitive(glue, value, type, hir);
                Jump(&exit);
            }
        }
    }
    Bind(&hasException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::GetPrimitiveTypeString(GateRef glue, PreferredPrimitiveType type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(typeValue, VariableType::JS_ANY(),
        GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
        ConstantIndex::STRING_STRING_INDEX));
    Label labelBuffer[2] = {Label(env), Label(env)};
    Label labelDefault(env);
    int64_t valueBuffer[2] = {
        static_cast<int64_t>(PreferredPrimitiveType::NO_PREFERENCE),
        static_cast<int64_t>(PreferredPrimitiveType::PREFER_NUMBER),
    };
    Switch(Int64(static_cast<int64_t>(type)), &labelDefault, valueBuffer, labelBuffer, 2);
    Bind(&labelBuffer[0]);
    {
        typeValue = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                           ConstantIndex::DEFAULT_INDEX);
        Jump(&labelDefault);
    }
    Bind(&labelBuffer[1]);
    {
        typeValue = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                           ConstantIndex::NUMBER_STRING_INDEX);
        Jump(&labelDefault);
    }
    Bind(&labelDefault);
    auto ret = *typeValue;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::OrdinaryToPrimitive(GateRef glue, GateRef value, PreferredPrimitiveType type, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label isEcmaObject(env);
    Label exit(env);
    Label hasException(env);
    Label notHasException1(env);
    Label notHasException2(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    BRANCH(IsEcmaObject(glue, value), &isEcmaObject, &exit);
    Bind(&isEcmaObject);
    DEFVARIABLE(valType, VariableType::INT32(), Int32(static_cast<uint8_t>(type)));
    GateRef numberInt32 = Int32(static_cast<uint8_t>(PreferredPrimitiveType::PREFER_NUMBER));
    GateRef stringInt32 = Int32(static_cast<uint8_t>(PreferredPrimitiveType::PREFER_STRING));
    GateRef len = Int32(2);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        DEFVARIABLE(keyString, VariableType::JS_ANY(), Undefined());
        BRANCH(Int32LessThan(*i, len), &next, &loopExit);
        Bind(&next);
        Label toString(env);
        Label valueOf(env);
        Label checkExit(env);
        BRANCH(Int32Equal(*valType, stringInt32), &toString, &valueOf);
        Bind(&toString);
        {
            keyString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                               ConstantIndex::TO_STRING_STRING_INDEX);
            valType = numberInt32;
            Jump(&checkExit);
        }
        Bind(&valueOf);
        {
            keyString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                               ConstantIndex::VALUE_OF_STRING_INDEX);
            valType = stringInt32;
            Jump(&checkExit);
        }
        Bind(&checkExit);
        GateRef entryfunc = FastGetPropertyByName(glue, value, *keyString, ProfileOperation(), hir);
        BRANCH(HasPendingException(glue), &hasException, &notHasException1);
        Bind(&notHasException1);
        Label isCallable1(env);
        BRANCH(TaggedIsCallable(glue, entryfunc), &isCallable1, &loopEnd);
        Bind(&isCallable1);
        {
            DEFVARIABLE(tmpResult, VariableType::JS_ANY(), Undefined());
            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG2_WITH_RETURN);
            callArgs.callThisArg2WithReturnArgs = { value, Undefined(), Undefined() };
            CallStubBuilder callBuilder(this, glue, entryfunc, Int32(2), 0, &tmpResult, Circuit::NullGate(),
                                        callArgs, ProfileOperation(), true, hir);
            Label callExit(env);
            if (env->IsBaselineBuiltin()) {
                callBuilder.JSCallDispatchForBaseline(&callExit);
                Bind(&callExit);
            } else {
                tmpResult = callBuilder.JSCallDispatch();
            }
            BRANCH(HasPendingException(glue), &hasException, &notHasException2);
            Bind(&notHasException2);
            Label notEcmaObject1(env);
            BRANCH(IsEcmaObject(glue, *tmpResult), &loopEnd, &notEcmaObject1);
            Bind(&notEcmaObject1);
            {
                result = *tmpResult;
                Jump(&exit);
            }
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertIllageValueToPrimitive));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result = Undefined();
        Jump(&exit);
    }
    Bind(&hasException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef StubBuilder::CallFunction(GateRef glue, GateRef func)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label undefinedOrNull(env);
    Label notUndefinedAndNull(env);
    Label notCallable(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), func);
    BRANCH(TaggedIsUndefinedOrNull(func), &undefinedOrNull, &notUndefinedAndNull);
    Bind(&undefinedOrNull);
    {
        result = Undefined();
        Jump(&exit);
    }
    Bind(&notUndefinedAndNull);
    BRANCH(TaggedIsCallable(glue, func), &exit, &notCallable);
    Bind(&notCallable);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ObjIsNotCallable));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void StubBuilder::ArrayCopy(GateRef glue, GateRef srcObj, GateRef srcAddr, GateRef dstObj,
                            GateRef dstAddr, GateRef taggedValueCount, GateRef needBarrier,
                            CopyKind copyKind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isEnableCMCGC(env);
    Label notCMCGC(env);
    BRANCH_UNLIKELY(LoadPrimitive(
        VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &isEnableCMCGC, &notCMCGC);

    Bind(&isEnableCMCGC);
    {
        CallNGCRuntime(glue, RTSTUB_ID(CopyObjectPrimitive),
            {glue, TaggedCastToIntPtr(dstObj), TaggedCastToIntPtr(dstAddr), TaggedCastToIntPtr(srcAddr), taggedValueCount});
        Label handleBarrier(env);
        BRANCH_NO_WEIGHT(needBarrier, &handleBarrier, &exit);
        Bind(&handleBarrier);
        {
            if (copyKind == SameArray) {
                CMCArrayCopyWriteBarrierSameArray(glue, dstObj, srcAddr, dstAddr, taggedValueCount);
            } else {
                CMCArrayCopyWriteBarrier(glue, dstObj, srcAddr, dstAddr, taggedValueCount);
            }
            Jump(&exit);
        }
    }
    Bind(&notCMCGC);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ObjectCopy),
            {glue, TaggedCastToIntPtr(dstObj), TaggedCastToIntPtr(dstAddr), TaggedCastToIntPtr(srcAddr), taggedValueCount});
        Label handleBarrier(env);
        BRANCH_NO_WEIGHT(needBarrier, &handleBarrier, &exit);
        Bind(&handleBarrier);
        {
            if (copyKind == SameArray) {
                CallCommonStub(glue, CommonStubCSigns::MoveBarrierInRegion,
                    {glue, TaggedCastToIntPtr(dstObj), TaggedCastToIntPtr(dstAddr), taggedValueCount,
                    TaggedCastToIntPtr(srcAddr)});
            } else {
                ASSERT(copyKind == DifferentArray);
                CallCommonStub(glue, CommonStubCSigns::MoveBarrierCrossRegion,
                    {glue, TaggedCastToIntPtr(dstObj), TaggedCastToIntPtr(dstAddr), taggedValueCount,
                    TaggedCastToIntPtr(srcAddr), TaggedCastToIntPtr(srcObj)});
            }
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void StubBuilder::ArrayCopyAndHoleToUndefined(GateRef glue, GateRef srcObj, GateRef srcAddr, GateRef dstObj,
                                              GateRef dstAddr, GateRef length, GateRef needBarrier)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label loopExit(env);
    Label exit(env);
    Label begin(env);
    Label body(env);
    Label handleBarrier(env);
    Label endLoop(env);
    GateRef dstOff = PtrSub(TaggedCastToIntPtr(dstAddr), TaggedCastToIntPtr(dstObj));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Jump(&begin);
    LoopBegin(&begin);
    {
        BRANCH_LIKELY(Int32UnsignedLessThan(*index, length), &body, &loopExit);
        Bind(&body);
        {
            GateRef offset = PtrMul(ZExtInt32ToPtr(*index), IntPtr(JSTaggedValue::TaggedTypeSize()));
            GateRef value = Load(VariableType::JS_ANY(), glue, srcAddr, offset);

            Label isHole(env);
            Label isNotHole(env);
            BRANCH_UNLIKELY(TaggedIsHole(value), &isHole, &isNotHole);
            Bind(&isHole);
            {
                Store(VariableType::JS_ANY(), glue, dstObj, PtrAdd(dstOff, offset), Undefined(),
                      MemoryAttribute::NoBarrier());
                Jump(&endLoop);
            }
            Bind(&isNotHole);
            Store(VariableType::JS_ANY(), glue, dstObj, PtrAdd(dstOff, offset), value, MemoryAttribute::NoBarrier());
            Jump(&endLoop);
        }
    }
    Bind(&endLoop);
    index = Int32Add(*index, Int32(1));
    LoopEnd(&begin);
    Bind(&loopExit);
    Label checkNext(env);
    BRANCH_NO_WEIGHT(needBarrier, &checkNext, &exit);
    Bind(&checkNext);
    BRANCH_UNLIKELY(LoadPrimitive(
        VariableType::BOOL(), glue, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &exit, &handleBarrier);
    Bind(&handleBarrier);
    {
        CallCommonStub(glue, CommonStubCSigns::MoveBarrierCrossRegion,
                       {glue, TaggedCastToIntPtr(dstObj), TaggedCastToIntPtr(dstAddr), length,
                       TaggedCastToIntPtr(srcAddr), TaggedCastToIntPtr(srcObj)});

        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef StubBuilder::Int64BitReverse(GateRef x)
{
    return env_->GetBuilder()->Int64Rev(x);
}

GateRef StubBuilder::Int32BitReverse(GateRef x)
{
    return env_->GetBuilder()->Int32Rev(x);
}

GateRef StubBuilder::Int16BitReverse(GateRef x)
{
    return env_->GetBuilder()->Int16Rev(x);
}

GateRef StubBuilder::Int8BitReverse(GateRef x)
{
    return env_->GetBuilder()->Int8Rev(x);
}

int64_t StubBuilder::SPECIAL_VALUE[SPECIAL_VALUE_NUM] = {
    static_cast<int64_t>(JSTaggedValue::VALUE_UNDEFINED),
    static_cast<int64_t>(JSTaggedValue::VALUE_NULL),
    static_cast<int64_t>(JSTaggedValue::VALUE_TRUE),
    static_cast<int64_t>(JSTaggedValue::VALUE_FALSE),
    static_cast<int64_t>(JSTaggedValue::VALUE_HOLE)
};

ConstantIndex StubBuilder::SPECIAL_STRING_INDEX[SPECIAL_VALUE_NUM] = {
    ConstantIndex::UNDEFINED_STRING_INDEX,
    ConstantIndex::NULL_STRING_INDEX,
    ConstantIndex::TRUE_STRING_INDEX,
    ConstantIndex::FALSE_STRING_INDEX,
    ConstantIndex::EMPTY_STRING_OBJECT_INDEX
};


GateRef StubBuilder::ThreeInt64Min(GateRef first, GateRef second, GateRef third)
{
    return env_->GetBuilder()->ThreeInt64Min(first, second, third);
}

GateRef StubBuilder::GetCurrentGlobalEnv(GateRef glue, GateRef currentEnv)
{
    auto env0 = GetEnvironment();
    {
        ASM_ASSERT(GET_MESSAGE_STRING_ID(CurrentEnvIsInvalid),
            LogicAndBuilder(env0).And(TaggedIsHeapObject(currentEnv))
                                 .And(LogicOrBuilder(env0).Or(IsGlobalEnv(glue, currentEnv))
                                      .Or(IsLexicalEnv(glue, currentEnv))
                                      .Or(IsSFunctionEnv(glue, currentEnv)).Done())
                                 .Done());
    }
    Label entry(env0);
    env0->SubCfgEntry(&entry);
    Label fromGlue(env0);
    Label exit(env0);
    DEFVARIABLE(globalEnv, VariableType::JS_ANY(), Undefined());

    globalEnv = GetValueFromTaggedArray(glue, currentEnv, Int32(BaseEnv::GLOBAL_ENV_INDEX));
    BRANCH_UNLIKELY(TaggedIsHole(*globalEnv), &fromGlue, &exit);
    Bind(&fromGlue);
    {
        globalEnv = GetGlobalEnv(glue);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *globalEnv;
    env0->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
