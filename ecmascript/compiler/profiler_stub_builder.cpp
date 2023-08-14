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
        Branch(TaggedIsInt(slotValue), &compareLabel, &uninitialize);
        Bind(&compareLabel);
        {
            GateRef oldSlotValue = TaggedGetInt(slotValue);
            curType = Int32Or(oldSlotValue, type);
            Branch(Int32Equal(oldSlotValue, *curType), &exit, &updateSlot);
        }
        Bind(&uninitialize);
        {
            Branch(TaggedIsUndefined(slotValue), &updateSlot, &updateProfile);
        }
        Bind(&updateSlot);
        {
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, IntToTaggedInt(*curType));
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
} // namespace panda::ecmascript::kungfu
