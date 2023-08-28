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

#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/ic/profile_type_info.h"

namespace panda::ecmascript::kungfu {
void ProfilerStubBuilder::PGOProfiler(GateRef glue, GateRef pc, GateRef func, GateRef profileTypeInfo,
    const std::vector<GateRef> &values, OperationType type)
{
    switch (type) {
        case OperationType::CALL:
            ProfileCall(glue, pc, func, values[0]);
            break;
        case OperationType::OPERATION_TYPE:
            ProfileOpType(glue, pc, func, profileTypeInfo, values[0]);
            break;
        case OperationType::DEFINE_CLASS:
            ProfileDefineClass(glue, pc, func, values[0]);
            break;
        case OperationType::CREATE_OBJECT:
            ProfileCreateObject(glue, pc, func, values[0]);
            break;
        case OperationType::STORE_LAYOUT:
            ProfileObjLayout(glue, pc, func, values[0], Int32(1));
            break;
        case OperationType::LOAD_LAYOUT:
            ProfileObjLayout(glue, pc, func, values[0], Int32(0));
            break;
        case OperationType::INDEX:
            ProfileObjIndex(glue, pc, func, values[0]);
            break;
        case OperationType::TRUE_BRANCH:
            ProfileBranch(glue, pc, func, profileTypeInfo, true);
            break;
        case OperationType::FALSE_BRANCH:
            ProfileBranch(glue, pc, func, profileTypeInfo, false);
            break;
        default:
            break;
    }
}

void ProfilerStubBuilder::ProfileOpType(GateRef glue, GateRef pc, GateRef func, GateRef profileTypeInfo, GateRef type)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label exit(env);
    Label profiler(env);
    Label slowpath(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &slowpath, &profiler);
    Bind(&slowpath);
    {
        GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef firstPC = Load(VariableType::NATIVE_POINTER(), method,
            IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
        CallNGCRuntime(glue, RTSTUB_ID(ProfileOpType), { glue, func, offset, type });
        Jump(&exit);
    }
    Bind(&profiler);
    {
        Label uninitialize(env);
        Label compareLabel(env);
        Label updateSlot(env);
        Label updateProfile(env);

        GateRef slotId = ZExtInt8ToInt32(Load(VariableType::INT8(), pc, IntPtr(1)));
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
            Branch(TaggedIsUndefined(slotValue), &updateSlot, &updateProfile);
        }
        Bind(&updateSlot);
        {
            GateRef newSlotValue = Int32Or(*curCount, *curType);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(newSlotValue));
            Jump(&updateProfile);
        }
        Bind(&updateProfile);
        {
            GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
            GateRef firstPC =
                Load(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
            GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
            CallNGCRuntime(glue, RTSTUB_ID(ProfileOpType), { glue, func, offset, *curType });
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileDefineClass(GateRef glue, GateRef pc, GateRef func, GateRef constructor)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef firstPC =
        Load(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
    CallNGCRuntime(glue, RTSTUB_ID(ProfileDefineClass), { glue, func, offset, constructor });

    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileCreateObject(GateRef glue, GateRef pc, GateRef func, GateRef newObj)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(traceId, VariableType::INT32(), Int32(0));
    Label isArray(env);
    Label profile(env);
    Label calculateTraceId(env);
    Branch(TaggedIsJSArray(newObj), &isArray, &calculateTraceId);
    Bind(&isArray);
    {
        GateRef traceIdOffset = IntPtr(JSArray::TRACE_INDEX_OFFSET);
        traceId = Load(VariableType::INT32(), newObj, traceIdOffset);
        Label uninitialize(env);
        Branch(Int32GreaterThan(*traceId, Int32(0)), &exit, &uninitialize);
        Bind(&uninitialize);
        {
            auto pfAddr = LoadPfHeaderFromConstPool(func);
            traceId = TruncPtrToInt32(PtrSub(pc, pfAddr));
            Store(VariableType::INT32(), glue, newObj, traceIdOffset, *traceId);
            Jump(&profile);
        }
    }
    Bind(&calculateTraceId);
    {
        auto pfAddr = LoadPfHeaderFromConstPool(func);
        traceId = TruncPtrToInt32(PtrSub(pc, pfAddr));
        Jump(&profile);
    }
    Bind(&profile);
    {
        GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef firstPC =
            Load(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
        CallNGCRuntime(glue, RTSTUB_ID(ProfileCreateObject), { glue, func, offset, newObj, *traceId });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileObjLayout(GateRef glue, GateRef pc, GateRef func, GateRef object, GateRef store)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label isHeap(env);
    Label exit(env);
    Branch(TaggedIsHeapObject(object), &isHeap, &exit);
    Bind(&isHeap);
    {
        GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef firstPC =
            Load(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
        CallNGCRuntime(glue, RTSTUB_ID(ProfileObjLayout), { glue, func, offset, object, store });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileObjIndex(GateRef glue, GateRef pc, GateRef func, GateRef object)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label isHeap(env);
    Label exit(env);
    Branch(TaggedIsHeapObject(object), &isHeap, &exit);
    Bind(&isHeap);
    {
        GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef firstPC =
            Load(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
        GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
        CallNGCRuntime(glue, RTSTUB_ID(ProfileObjIndex), { glue, func, offset, object });
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::ProfileCall(GateRef glue, GateRef pc, GateRef func, GateRef target)
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
        Label nonHotness(env);
        Branch(TaggedIsUndefined(targetProfileInfo), &nonHotness, &exit);
        Bind(&nonHotness);
        {
            GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
            GateRef firstPC =
                Load(VariableType::NATIVE_POINTER(), method, IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
            GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
            CallNGCRuntime(glue, RTSTUB_ID(ProfileCall), { glue, func, target, offset, Int32(1)});
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
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

void ProfilerStubBuilder::ProfileObjLayoutOrIndex(GateRef glue, GateRef receiver, GateRef key, GateRef isStore,
    ProfileOperation callback)
{
    if (callback.IsEmpty()) {
        return;
    }
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    GateRef index64 = TryToElementsIndex(glue, key);
    Label validIndex(env);
    Label profileIndex(env);
    Label profileLayout(env);
    Label greaterThanInt32Max(env);
    Label notGreaterThanInt32Max(env);
    Branch(Int64GreaterThanOrEqual(index64, Int64(INT32_MAX)), &greaterThanInt32Max, &notGreaterThanInt32Max);
    Bind(&greaterThanInt32Max);
    {
        Jump(&exit);
    }
    Bind(&notGreaterThanInt32Max);
    GateRef index = TruncInt64ToInt32(index64);
    Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &profileLayout);
    Bind(&validIndex);
    {
        Branch(IsTypedArray(receiver), &profileIndex, &profileLayout);
        Bind(&profileIndex);
        {
            callback.ProfileObjIndex(receiver);
            Jump(&exit);
        }
    }
    Bind(&profileLayout);
    {
        Label store(env);
        Label load(env);
        Branch(isStore, &store, &load);
        Bind(&store);
        {
            callback.ProfileObjLayoutByStore(receiver);
            Jump(&exit);
        }
        Bind(&load);
        {
            callback.ProfileObjLayoutByLoad(receiver);
            Jump(&exit);
        }
    }

    Bind(&exit);
    env->SubCfgExit();
}

void ProfilerStubBuilder::UpdatePropAttrIC(
    GateRef glue, GateRef receiver, GateRef value, GateRef handler, ProfileOperation callback)
{
    if (callback.IsEmpty()) {
        return;
    }
    GateRef attrIndex = HandlerBaseGetAttrIndex(handler);
    GateRef hclass = LoadHClass(receiver);
    GateRef layout = GetLayoutFromHClass(hclass);
    GateRef propAttr = GetPropAttrFromLayoutInfo(layout, attrIndex);
    GateRef attr = GetInt32OfTInt(propAttr);
    UpdatePropAttrWithValue(glue, receiver, layout, attr, attrIndex, value, callback);
}

void ProfilerStubBuilder::UpdatePropAttrWithValue(GateRef glue, GateRef receiver, GateRef layout, GateRef attr,
    GateRef attrIndex, GateRef value, ProfileOperation callback)
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
        callback.ProfileObjLayoutByStore(receiver);
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
    Label updateProfile(env);
    Label needUpdate(env);
    Label exit(env);
    DEFVARIABLE(oldType, VariableType::INT32(), Int32(PGOSampleType::None()));
    DEFVARIABLE(newType, VariableType::INT32(), Int32(PGOSampleType::NormalBranch()));
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
            Branch(TaggedIsInt(slotValue), &compareLabel, &updateSlot);
            Bind(&compareLabel);
            {
                GateRef oldSlotValue = TaggedGetInt(slotValue);
                GateRef oldTrue = Int32LSR(oldSlotValue, Int32(21));    // 21: trueWeight shift bit
                GateRef oldFalse = Int32LSR(oldSlotValue, Int32(10));   // 10: falstWeight shift bit
                oldFalse = Int32And(oldFalse, Int32(0x7ff));   // 0xffff: weight bits
                oldPrama = Int32And(oldSlotValue, Int32(PGOSampleType::AnyType()));
                Branch(Int32LessThan(Int32Add(oldTrue, oldFalse), Int32(2000)), &needUpdate, &exit);    // 2000: limit
                Bind(&needUpdate);
                {
                    oldType = GetBranchTypeFromWeight(oldTrue, oldFalse);
                    newTrue = Int32Add(*newTrue, oldTrue);
                    newFalse = Int32Add(*newFalse, oldFalse);
                    newType = GetBranchTypeFromWeight(*newTrue, *newFalse);
                    Jump(&updateSlot);
                }
            }
            Bind(&updateSlot);
            {
                GateRef newSlotValue = Int32Or(*oldPrama, Int32LSL(*newTrue, Int32(21))); // 21: trueWeight shift bit
                newSlotValue = Int32Or(newSlotValue, Int32LSL(*newFalse, Int32(10))); // 10: trueWeight shift bit

                SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo,
                    slotId, IntToTaggedInt(newSlotValue));
                Branch(Int32Equal(*oldType, *newType), &exit, &updateProfile);
            }
            Bind(&updateProfile);
            {
                GateRef method = Load(VariableType::JS_ANY(), func, IntPtr(JSFunctionBase::METHOD_OFFSET));
                GateRef firstPC = Load(VariableType::NATIVE_POINTER(), method,
                    IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
                GateRef offset = TruncPtrToInt32(PtrSub(pc, firstPC));
                CallNGCRuntime(glue, RTSTUB_ID(ProfileOpType), { glue, func, offset, *newType });
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef ProfilerStubBuilder::GetBranchTypeFromWeight(GateRef trueWeight, GateRef falseWeight)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(curType, VariableType::INT32(), Int32(PGOSampleType::NormalBranch()));
    Label trueBranch(env);
    Label strongTrue(env);
    Label notStrongTrue(env);
    Label notTrueBranch(env);
    Label falseBranch(env);
    Label strongFalse(env);
    Label notStrongFalse(env);
    Branch(Int32GreaterThan(trueWeight, Int32Mul(falseWeight, Int32(BranchWeight::WEAK_WEIGHT))),
        &trueBranch, &notTrueBranch);
    Bind(&trueBranch);
    {
        Branch(Int32GreaterThan(trueWeight, Int32Mul(falseWeight, Int32(BranchWeight::STRONG_WEIGHT))),
            &strongTrue, &notStrongTrue);
        Bind(&strongTrue);
        {
            curType = Int32(PGOSampleType::StrongLikely());
            Jump(&exit);
        }
        Bind(&notStrongTrue);
        {
            curType = Int32(PGOSampleType::Likely());
            Jump(&exit);
        }
    }
    Bind(&notTrueBranch);
    {
        Branch(Int32GreaterThan(falseWeight, Int32Mul(trueWeight, Int32(BranchWeight::WEAK_WEIGHT))),
            &falseBranch, &exit);
        Bind(&falseBranch);
        {
            Branch(Int32GreaterThan(falseWeight, Int32Mul(trueWeight, Int32(BranchWeight::STRONG_WEIGHT))),
                &strongFalse, &notStrongFalse);
            Bind(&strongFalse);
            {
                curType = Int32(PGOSampleType::StrongUnLikely());
                Jump(&exit);
            }
            Bind(&notStrongFalse);
            {
                curType = Int32(PGOSampleType::Unlikely());
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto result = *curType;
    env->SubCfgExit();
    return result;
}
} // namespace panda::ecmascript::kungfu
