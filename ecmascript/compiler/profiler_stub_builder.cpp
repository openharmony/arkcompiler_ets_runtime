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

#include "ecmascript/compiler/profiler_stub_builder.h"

#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/ic/profile_type_info.h"

namespace panda::ecmascript::kungfu {
void ProfilerStubBuilder::PGOProfiler(GateRef glue, GateRef pc, GateRef func, GateRef profileTypeInfo,
    const std::vector<GateRef> &values, SlotIDFormat format, OperationType type)
{
    switch (type) {
        case OperationType::CALL:
            ProfileCall(glue, pc, func, values[0], profileTypeInfo, format);
            break;
        case OperationType::NATIVE_CALL:
            ProfileNativeCall(glue, pc, func, values[0], profileTypeInfo, format);
            break;
        case OperationType::OPERATION_TYPE:
            ProfileOpType(glue, pc, func, profileTypeInfo, values[0], format);
            break;
        case OperationType::DEFINE_CLASS:
            ProfileDefineClass(glue, pc, func, values[0], profileTypeInfo, format);
            break;
        case OperationType::CREATE_OBJECT:
            ProfileCreateObject(glue, pc, func, values[0], profileTypeInfo, format);
            break;
        case OperationType::TRY_DUMP:
            TryDump(glue, func, profileTypeInfo);
            break;
        case OperationType::TRY_PREDUMP:
            TryPreDump(glue, func, profileTypeInfo);
            break;
        case OperationType::TRUE_BRANCH:
            ProfileBranch(glue, pc, func, profileTypeInfo, true);
            break;
        case OperationType::FALSE_BRANCH:
            ProfileBranch(glue, pc, func, profileTypeInfo, false);
            break;
        case OperationType::ITERATOR_FUNC_KIND:
            ProfileGetIterator(glue, pc, func, values[0], profileTypeInfo, format);
            break;
        default:
            break;
    }
}

void ProfilerStubBuilder::TryDump(GateRef glue, GateRef func, GateRef profileTypeInfo)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label updatePeriodCounter(env);
    Label exit(env);

    Branch(IsProfileTypeInfoDumped(profileTypeInfo), &exit, &updatePeriodCounter);
    Bind(&updatePeriodCounter);
    {
        SetDumpPeriodIndex(glue, profileTypeInfo);
        CallRuntime(glue, RTSTUB_ID(PGODump), { func });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::TryPreDump(GateRef glue, GateRef func, GateRef profileTypeInfo)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label profiler(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &profiler);
    Bind(&profiler);
    {
        TryPreDumpInner(glue, func, profileTypeInfo);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileOpType(
    GateRef glue, GateRef pc, GateRef func, GateRef profileTypeInfo, GateRef type, SlotIDFormat format)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label exit(env);
    Label profiler(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &profiler);
    Bind(&profiler);
    {
        Label uninitialize(env);
        Label compareLabel(env);
        Label updateSlot(env);

        GateRef slotId = GetSlotID(pc, format);
        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        DEFVARIABLE(curType, VariableType::INT32(), type);
        DEFVARIABLE(curCount, VariableType::INT32(), Int32(0));
        Branch(TaggedIsInt(slotValue), &compareLabel, &uninitialize);
        Bind(&compareLabel);
        {
            GateRef oldSlotValue = TaggedGetInt(slotValue);
            GateRef oldType = Int32And(oldSlotValue, Int32(PGOSampleType::AnyType()));
            curType = Int32Or(oldType, type);
            curCount = Int32And(oldSlotValue, Int32(0xfffffc00));   // 0xfffffc00: count bits
            Branch(Int32Equal(oldType, *curType), &exit, &updateSlot);
        }
        Bind(&uninitialize);
        {
            // Slot maybe overflow.
            Branch(TaggedIsUndefined(slotValue), &updateSlot, &exit);
        }
        Bind(&updateSlot);
        {
            GateRef newSlotValue = Int32Or(*curCount, *curType);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(newSlotValue));
            TryPreDumpInner(glue, func, profileTypeInfo);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileDefineClass(
    GateRef glue, GateRef pc, GateRef func, GateRef constructor, GateRef profileTypeInfo, SlotIDFormat format)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label exit(env);
    Label profiler(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &profiler);
    Bind(&profiler);
    {
        GateRef slotId = GetSlotID(pc, format);
        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        Label updateSlot(env);
        Branch(TaggedIsUndefined(slotValue), &updateSlot, &exit);
        Bind(&updateSlot);
        auto weakCtor = env->GetBuilder()->CreateWeakRef(constructor);
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, weakCtor);
        TryPreDumpInner(glue, func, profileTypeInfo);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileCreateObject(
    GateRef glue, GateRef pc, GateRef func, GateRef newObj, GateRef profileTypeInfo, SlotIDFormat format)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    Label profiler(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &profiler);
    Bind(&profiler);
    {
        GateRef slotId = GetSlotID(pc, format);
        auto hclass = LoadHClass(newObj);
        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        Label isHeapObject(env);
        Label isWeak(env);
        Label uninitialized(env);
        Label updateSlot(env);
        Branch(TaggedIsHeapObject(slotValue), &isHeapObject, &uninitialized);
        Bind(&isHeapObject);
        {
            Branch(TaggedIsWeak(slotValue), &isWeak, &updateSlot);
        }
        Bind(&isWeak);
        {
            auto cachedHClass = LoadObjectFromWeakRef(slotValue);
            Branch(Equal(cachedHClass, hclass), &exit, &updateSlot);
        }
        Bind(&uninitialized);
        {
            Branch(TaggedIsUndefined(slotValue), &updateSlot, &exit);
        }
        Bind(&updateSlot);
        {
            auto weakCtor = env->GetBuilder()->CreateWeakRef(hclass);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, weakCtor);
            TryPreDumpInner(glue, func, profileTypeInfo);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileCall(
    GateRef glue, GateRef pc, GateRef func, GateRef target, GateRef profileTypeInfo, SlotIDFormat format)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label exit(env);
    Label slowpath(env);
    Label fastpath(env);

    Label targetIsFunction(env);
    Branch(IsJSFunction(target), &targetIsFunction, &exit);
    Bind(&targetIsFunction);
    {
        GateRef targetProfileInfo = GetProfileTypeInfo(target);
        Label targetNonHotness(env);
        Label IsCurrentHotness(env);
        Label currentIsHotness(env);
        Branch(TaggedIsUndefined(targetProfileInfo), &targetNonHotness, &IsCurrentHotness);
        Bind(&targetNonHotness);
        {
            CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounterWithProf), { target });
            Jump(&IsCurrentHotness);
        }
        Bind(&IsCurrentHotness);
        {
            Branch(TaggedIsUndefined(profileTypeInfo), &exit, &currentIsHotness);
        }
        Bind(&currentIsHotness);
        {
            GateRef slotId = GetSlotID(pc, format);
            GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
            Label isInt(env);
            Label uninitialized(env);
            Label updateSlot(env);
            Branch(TaggedIsInt(slotValue), &isInt, &uninitialized);
            Bind(&isInt);
            {
                Label change(env);
                Label resetSlot(env);
                GateRef oldSlotValue = TaggedGetInt(slotValue);
                GateRef methodId = env->GetBuilder()->GetMethodId(target);
                Branch(Int32Equal(oldSlotValue, TruncInt64ToInt32(methodId)), &exit, &change);
                Bind(&change);
                {
                    Branch(Int32Equal(oldSlotValue, Int32(0)), &exit, &resetSlot);
                }
                Bind(&resetSlot);
                {
                    GateRef nonType = IntToTaggedInt(Int32(0));
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, nonType);
                    TryPreDumpInner(glue, func, profileTypeInfo);
                    Jump(&exit);
                }
            }
            Bind(&uninitialized);
            {
                Branch(TaggedIsUndefined(slotValue), &updateSlot, &exit);
            }
            Bind(&updateSlot);
            {
                GateRef methodId = env->GetBuilder()->GetMethodId(target);
                GateRef methodIdValue = IntToTaggedInt(TruncInt64ToInt32(methodId));
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, methodIdValue);
                TryPreDumpInner(glue, func, profileTypeInfo);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef ProfilerStubBuilder::TryGetBuiltinFunctionId(GateRef glue, GateRef target)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(functionId, VariableType::INT32(), Int32(PGO_BUILTINS_STUB_ID(NONE)));
    DEFVARIABLE(maybeFunc, VariableType::JS_ANY(), Undefined());
    Label isArrayIterProtoNext(env);
    Label notArrayIterProtoNext(env);
    Label isMapIterProtoNext(env);
    Label notMapIterProtoNext(env);
    Label isSetIterProtoNext(env);
    Label notSetIterProtoNext(env);
    Label isStringIterProtoNext(env);

    maybeFunc = GetGlobalConstantValue(
        VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_ITERATOR_PROTO_NEXT_INDEX);
    Branch(Int64Equal(target, *maybeFunc), &isArrayIterProtoNext, &notArrayIterProtoNext);
    Bind(&isArrayIterProtoNext);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(ARRAY_ITERATOR_PROTO_NEXT));
        Jump(&exit);
    }
    Bind(&notArrayIterProtoNext);
    maybeFunc = GetGlobalConstantValue(
        VariableType::JS_ANY(), glue, ConstantIndex::MAP_ITERATOR_PROTO_NEXT_INDEX);
    Branch(Int64Equal(target, *maybeFunc), &isMapIterProtoNext, &notMapIterProtoNext);
    Bind(&isMapIterProtoNext);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(MAP_ITERATOR_PROTO_NEXT));
        Jump(&exit);
    }
    Bind(&notMapIterProtoNext);
    maybeFunc = GetGlobalConstantValue(
        VariableType::JS_ANY(), glue, ConstantIndex::SET_ITERATOR_PROTO_NEXT_INDEX);
    Branch(Int64Equal(target, *maybeFunc), &isSetIterProtoNext, &notSetIterProtoNext);
    Bind(&isSetIterProtoNext);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(SET_ITERATOR_PROTO_NEXT));
        Jump(&exit);
    }
    Bind(&notSetIterProtoNext);
    maybeFunc = GetGlobalConstantValue(
        VariableType::JS_ANY(), glue, ConstantIndex::STRING_ITERATOR_PROTO_NEXT_INDEX);
    Branch(Int64Equal(target, *maybeFunc), &isStringIterProtoNext, &exit);
    Bind(&isStringIterProtoNext);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(STRING_ITERATOR_PROTO_NEXT));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *functionId;
    env->SubCfgExit();
    return ret;
}

void ProfilerStubBuilder::ProfileNativeCall(
    GateRef glue, GateRef pc, GateRef func, GateRef target, GateRef profileTypeInfo, SlotIDFormat format)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label exit(env);
    Label currentIsHot(env);

    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &currentIsHot);
    Bind(&currentIsHot);
    {
        Label updateSlot(env);
        Label initSlot(env);
        Label sameValueCheck(env);
        Label invalidate(env);

        GateRef slotId = GetSlotID(pc, format);
        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        GateRef newId = TryGetBuiltinFunctionId(glue, target);
        Branch(TaggedIsInt(slotValue), &updateSlot, &initSlot);
        Bind(&updateSlot);
        GateRef oldId = TaggedGetInt(slotValue);
        Branch(Int32Equal(oldId, Int32(PGO_BUILTINS_STUB_ID(NONE))), &exit, &sameValueCheck);
        Bind(&sameValueCheck);
        Branch(Int32Equal(oldId, newId), &exit, &invalidate);
        Bind(&invalidate);
        {
            GateRef invalidId = Int32(PGO_BUILTINS_STUB_ID(NONE));
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(invalidId));
            TryPreDumpInner(glue, func, profileTypeInfo);
            Jump(&exit);
        }
        Bind(&initSlot);
        {
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(newId));
            TryPreDumpInner(glue, func, profileTypeInfo);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef ProfilerStubBuilder::IsProfileTypeInfoDumped(GateRef profileTypeInfo, ProfileOperation callback)
{
    if (callback.IsEmpty()) {
        return Boolean(true);
    }
    return IsProfileTypeInfoDumped(profileTypeInfo);
}

GateRef ProfilerStubBuilder::UpdateTrackTypeInPropAttr(GateRef attr, GateRef value, ProfileOperation callback)
{
    if (callback.IsEmpty()) {
        return attr;
    }
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    GateRef oldTrackType = GetTrackTypeInPropAttr(attr);
    DEFVARIABLE(newTrackType, VariableType::INT32(), Int32(static_cast<int32_t>(TrackType::TAGGED)));
    DEFVARIABLE(result, VariableType::INT32(), attr);

    Label exit(env);
    Label judgeValue(env);
    Branch(Equal(oldTrackType, Int32(static_cast<int32_t>(TrackType::TAGGED))), &exit, &judgeValue);
    Bind(&judgeValue);
    {
        newTrackType = TaggedToTrackType(value);
        Label update(env);
        Label merge(env);
        Branch(Int32Equal(*newTrackType, Int32(static_cast<int32_t>(TrackType::TAGGED))), &update, &merge);
        Bind(&merge);
        {
            newTrackType = Int32Or(oldTrackType, *newTrackType);
            Branch(Int32Equal(oldTrackType, *newTrackType), &exit, &update);
        }
        Bind(&update);
        {
            result = SetTrackTypeInPropAttr(attr, *newTrackType);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void ProfilerStubBuilder::UpdatePropAttrIC(
    GateRef glue, GateRef receiver, GateRef value, GateRef handler, ProfileOperation callback)
{
    if (callback.IsEmpty()) {
        return;
    }
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label updateLayout(env);

    GateRef attrIndex = HandlerBaseGetAttrIndex(handler);
    GateRef hclass = LoadHClass(receiver);
    GateRef layout = GetLayoutFromHClass(hclass);
    GateRef propAttr = GetPropAttrFromLayoutInfo(layout, attrIndex);
    GateRef attr = GetInt32OfTInt(propAttr);
    GateRef newAttr = UpdateTrackTypeInPropAttr(attr, value, callback);
    Branch(Equal(attr, newAttr), &exit, &updateLayout);
    Bind(&updateLayout);
    {
        SetPropAttrToLayoutInfo(glue, layout, attrIndex, newAttr);
        callback.TryPreDump();
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::UpdatePropAttrWithValue(
    GateRef glue, GateRef layout, GateRef attr, GateRef attrIndex, GateRef value, ProfileOperation callback)
{
    if (callback.IsEmpty()) {
        return;
    }
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label updateLayout(env);
    GateRef newAttr = UpdateTrackTypeInPropAttr(attr, value, callback);
    Branch(Equal(attr, newAttr), &exit, &updateLayout);
    Bind(&updateLayout);
    {
        SetPropAttrToLayoutInfo(glue, layout, attrIndex, newAttr);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef ProfilerStubBuilder::TaggedToTrackType(GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(newTrackType, VariableType::INT32(), Int32(static_cast<int32_t>(TrackType::TAGGED)));
    Label exit(env);
    Label isInt(env);
    Label notInt(env);
    Branch(TaggedIsInt(value), &isInt, &notInt);
    Bind(&isInt);
    {
        newTrackType = Int32(static_cast<int32_t>(TrackType::INT));
        Jump(&exit);
    }
    Bind(&notInt);
    {
        Label isObject(env);
        Label isDouble(env);
        Branch(TaggedIsObject(value), &isObject, &isDouble);
        Bind(&isObject);
        {
            newTrackType = Int32(static_cast<int32_t>(TrackType::TAGGED));
            Jump(&exit);
        }
        Bind(&isDouble);
        {
            newTrackType = Int32(static_cast<int32_t>(TrackType::DOUBLE));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *newTrackType;
    env->SubCfgExit();
    return ret;
}

void ProfilerStubBuilder::ProfileBranch(GateRef glue, GateRef pc, GateRef func, GateRef profileTypeInfo, bool isTrue)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label profiler(env);
    Label hasSlot(env);
    Label currentIsTrue(env);
    Label currentIsFalse(env);
    Label genCurrentWeight(env);
    Label compareLabel(env);
    Label updateSlot(env);
    Label preProfile(env);
    Label needUpdate(env);
    Label exit(env);
    DEFVARIABLE(oldPrama, VariableType::INT32(), Int32(PGOSampleType::None()));
    DEFVARIABLE(newTrue, VariableType::INT32(), isTrue ? Int32(1) : Int32(0));
    DEFVARIABLE(newFalse, VariableType::INT32(), isTrue ? Int32(0) : Int32(1));

    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &profiler);
    Bind(&profiler);
    {
        GateRef slotId = ZExtInt8ToInt32(Load(VariableType::INT8(), pc, IntPtr(1)));
        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        Branch(TaggedIsHole(slotValue), &exit, &hasSlot);   // ishole -- isundefined
        Bind(&hasSlot);
        {
            Label uninitialized(env);
            Branch(TaggedIsInt(slotValue), &compareLabel, &uninitialized);
            Bind(&compareLabel);
            {
                GateRef oldSlotValue = TaggedGetInt(slotValue);
                GateRef oldTrue = Int32LSR(oldSlotValue, Int32(PGOSampleType::WEIGHT_TRUE_START_BIT));
                GateRef oldFalse = Int32LSR(oldSlotValue, Int32(PGOSampleType::WEIGHT_START_BIT));
                oldFalse = Int32And(oldFalse, Int32(PGOSampleType::WEIGHT_MASK));
                oldPrama = Int32And(oldSlotValue, Int32(PGOSampleType::AnyType()));
                auto condition = BoolAnd(Int32LessThan(oldTrue, Int32(PGOSampleType::WEIGHT_MASK)),
                    Int32LessThan(oldFalse, Int32(PGOSampleType::WEIGHT_MASK)));
                Branch(condition, &needUpdate, &exit);    // 2000: limit
                Bind(&needUpdate);
                {
                    newTrue = Int32Add(*newTrue, oldTrue);
                    newFalse = Int32Add(*newFalse, oldFalse);
                    Jump(&updateSlot);
                }
            }
            Bind(&uninitialized);
            {
                Branch(TaggedIsUndefined(slotValue), &updateSlot, &exit);
            }
            Bind(&updateSlot);
            {
                GateRef newSlotValue =
                    Int32Or(*oldPrama, Int32LSL(*newTrue, Int32(PGOSampleType::WEIGHT_TRUE_START_BIT)));
                newSlotValue = Int32Or(newSlotValue, Int32LSL(*newFalse, Int32(PGOSampleType::WEIGHT_START_BIT)));
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo,
                    slotId, IntToTaggedInt(newSlotValue));
                auto totalCount = Int32Add(*newTrue, *newFalse);
                auto mask = Int32(0x1FF);
                Label updateFinal(env);
                Branch(Int32Equal(Int32And(totalCount, mask), Int32(0)), &preProfile, &updateFinal);
                Bind(&updateFinal);
                {
                    auto isFinal = BoolOr(Int32Equal(*newTrue, Int32(PGOSampleType::WEIGHT_MASK)),
                        Int32Equal(*newFalse, Int32(PGOSampleType::WEIGHT_MASK)));
                    Branch(isFinal, &preProfile, &exit);
                }
            }
            Bind(&preProfile);
            {
                TryPreDumpInner(glue, func, profileTypeInfo);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::TryPreDumpInner(GateRef glue, GateRef func, GateRef profileTypeInfo)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label setPreDumpPeriodIndex(env);
    Label isInPredumpWorkList(env);
    Label addPredumpWorkList(env);
    Label exit(env);
    Branch(IsProfileTypeInfoPreDumped(profileTypeInfo), &exit, &setPreDumpPeriodIndex);
    Bind(&setPreDumpPeriodIndex);
    {
        SetPreDumpPeriodIndex(glue, profileTypeInfo);
        Jump(&addPredumpWorkList);
    }
    Bind(&addPredumpWorkList);
    {
        CallRuntime(glue, RTSTUB_ID(PGOPreDump), { func });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef ProfilerStubBuilder::GetIterationFunctionId(GateRef glue, GateRef iterator)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(functionId, VariableType::INT32(), Int32(PGO_BUILTINS_STUB_ID(NONE)));
    DEFVARIABLE(maybeFunc, VariableType::JS_ANY(), Undefined());
    Label isArrayProtoValues(env);
    Label notArrayProtoValues(env);
    Label isSetProtoValues(env);
    Label notSetProtoValues(env);
    Label isMapProtoEntries(env);
    Label notMapProtoEntries(env);
    Label isStringProtoIter(env);
    Label notStringProtoIter(env);
    Label isTypedArrayProtoValues(env);

    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    maybeFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_PROTO_VALUES_FUNCTION_INDEX);
    Branch(Int64Equal(iterator, *maybeFunc), &isArrayProtoValues, &notArrayProtoValues);
    Bind(&isArrayProtoValues);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(ARRAY_PROTO_ITERATOR));
        Jump(&exit);
    }
    Bind(&notArrayProtoValues);
    maybeFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::SET_PROTO_VALUES_FUNCTION_INDEX);
    Branch(Int64Equal(iterator, *maybeFunc), &isSetProtoValues, &notSetProtoValues);
    Bind(&isSetProtoValues);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(SET_PROTO_ITERATOR));
        Jump(&exit);
    }
    Bind(&notSetProtoValues);
    maybeFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::MAP_PROTO_ENTRIES_FUNCTION_INDEX);
    Branch(Int64Equal(iterator, *maybeFunc), &isMapProtoEntries, &notMapProtoEntries);
    Bind(&isMapProtoEntries);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(MAP_PROTO_ITERATOR));
        Jump(&exit);
    }
    Bind(&notMapProtoEntries);
    maybeFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::STRING_PROTO_ITER_FUNCTION_INDEX);
    Branch(Int64Equal(iterator, *maybeFunc), &isStringProtoIter, &notStringProtoIter);
    Bind(&isStringProtoIter);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(STRING_PROTO_ITERATOR));
        Jump(&exit);
    }
    Bind(&notStringProtoIter);
    maybeFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                  GlobalEnv::TYPED_ARRAY_PROTO_VALUES_FUNCTION_INDEX);
    Branch(Int64Equal(iterator, *maybeFunc), &isTypedArrayProtoValues, &exit);
    Bind(&isTypedArrayProtoValues);
    {
        functionId = Int32(PGO_BUILTINS_STUB_ID(TYPED_ARRAY_PROTO_ITERATOR));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *functionId;
    env->SubCfgExit();
    return ret;
}

void ProfilerStubBuilder::ProfileGetIterator(
    GateRef glue, GateRef pc, GateRef func, GateRef iterator, GateRef profileTypeInfo, SlotIDFormat format)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label exit(env);
    Label profiler(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &exit, &profiler);
    Bind(&profiler);
    {
        Label updateSlot(env);
        Label initSlot(env);
        Label sameValueCheck(env);
        Label invalidate(env);

        GateRef slotId = GetSlotID(pc, format);
        GateRef slotValue = GetValueFromTaggedArray(profileTypeInfo, slotId);
        GateRef newIterKind = GetIterationFunctionId(glue, iterator);
        Branch(TaggedIsInt(slotValue), &updateSlot, &initSlot);
        Bind(&updateSlot);
        GateRef oldIterKind = TaggedGetInt(slotValue);
        Branch(Int32Equal(oldIterKind, Int32(PGO_BUILTINS_STUB_ID(NONE))),
            &exit, &sameValueCheck);
        Bind(&sameValueCheck);
        Branch(Int32Equal(oldIterKind, newIterKind), &exit, &invalidate);
        Bind(&invalidate);
        {
            GateRef invalidKind = Int32(PGO_BUILTINS_STUB_ID(NONE));
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(invalidKind));
            TryPreDumpInner(glue, func, profileTypeInfo);
            Jump(&exit);
        }
        Bind(&initSlot);
        {
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(newIterKind));
            TryPreDumpInner(glue, func, profileTypeInfo);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef ProfilerStubBuilder::GetSlotID(GateRef pc, SlotIDFormat format)
{
    if (format == SlotIDFormat::IMM16) {
        auto hight = Load(VariableType::INT8(), pc, IntPtr(2)); // 2 : skip 1 byte of bytecode
        hight = Int16LSL(ZExtInt8ToInt16(hight), Int16(8)); // 8 : set as high 8 bits
        auto low = Load(VariableType::INT8(), pc, IntPtr(1));
        auto result = Int16Add(hight, ZExtInt8ToInt16(low));
        return ZExtInt16ToInt32(result);
    }
    return ZExtInt8ToInt32(Load(VariableType::INT8(), pc, IntPtr(1)));
}

GateRef ProfilerStubBuilder::GetBitFieldOffsetFromProfileTypeInfo(GateRef profileTypeInfo)
{
    auto length = GetLengthOfTaggedArray(profileTypeInfo);
    auto index = Int32Sub(length, Int32(ProfileTypeInfo::BIT_FIELD_INDEX));
    auto indexOffset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    return PtrAdd(indexOffset, IntPtr(TaggedArray::DATA_OFFSET));
}

GateRef ProfilerStubBuilder::IsProfileTypeInfoDumped(GateRef profileTypeInfo)
{
    GateRef periodCounterOffset = GetBitFieldOffsetFromProfileTypeInfo(profileTypeInfo);
    GateRef count = Load(VariableType::INT32(), profileTypeInfo, periodCounterOffset);
    return Int32Equal(count, Int32(ProfileTypeInfo::DUMP_PEROID_INDEX));
}

GateRef ProfilerStubBuilder::IsProfileTypeInfoPreDumped(GateRef profileTypeInfo)
{
    GateRef periodCounterOffset = GetBitFieldOffsetFromProfileTypeInfo(profileTypeInfo);
    GateRef count = Load(VariableType::INT32(), profileTypeInfo, periodCounterOffset);
    return Int32Equal(count, Int32(ProfileTypeInfo::PRE_DUMP_PEROID_INDEX));
}

void ProfilerStubBuilder::SetDumpPeriodIndex(GateRef glue, GateRef profileTypeInfo)
{
    GateRef periodCounterOffset = GetBitFieldOffsetFromProfileTypeInfo(profileTypeInfo);
    GateRef newCount = Int32(ProfileTypeInfo::DUMP_PEROID_INDEX);
    Store(VariableType::INT32(), glue, profileTypeInfo, periodCounterOffset, newCount);
}

void ProfilerStubBuilder::SetPreDumpPeriodIndex(GateRef glue, GateRef profileTypeInfo)
{
    GateRef periodCounterOffset = GetBitFieldOffsetFromProfileTypeInfo(profileTypeInfo);
    GateRef newCount = Int32(ProfileTypeInfo::PRE_DUMP_PEROID_INDEX);
    Store(VariableType::INT32(), glue, profileTypeInfo, periodCounterOffset, newCount);
}
} // namespace panda::ecmascript::kungfu
