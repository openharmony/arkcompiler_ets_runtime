/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_gc.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsGc::GetFreeHeapSize(EcmaRuntimeCallInfo *info)
{
    auto *heap = info->GetThread()->GetEcmaVM()->GetHeap();
    auto size = heap->GetHeapLimitSize() - heap->GetHeapObjectSize();
    return JSTaggedValue(static_cast<int64_t>(size));
}

JSTaggedValue BuiltinsGc::GetReservedHeapSize(EcmaRuntimeCallInfo *info)
{
    auto *heap = info->GetThread()->GetEcmaVM()->GetHeap();
    return JSTaggedValue(static_cast<int64_t>(heap->GetHeapLimitSize()));
}

JSTaggedValue BuiltinsGc::GetUsedHeapSize(EcmaRuntimeCallInfo *info)
{
    auto *heap = info->GetThread()->GetEcmaVM()->GetHeap();
    return JSTaggedValue(static_cast<int64_t>(heap->GetHeapObjectSize()));
}

JSTaggedValue BuiltinsGc::GetObjectAddress(EcmaRuntimeCallInfo *info)
{
    JSHandle<JSTaggedValue> h = GetCallArg(info, 0);
    return JSTaggedValue(reinterpret_cast<int64_t>(h->GetHeapObject()));
}

JSTaggedValue BuiltinsGc::GetObjectSpaceType(EcmaRuntimeCallInfo *info)
{
    JSHandle<JSTaggedValue> h = GetCallArg(info, 0);
    auto *region = Region::ObjectAddressToRange(h->GetHeapObject());
    return JSTaggedValue(region->GetSpaceType());
}

JSTaggedValue BuiltinsGc::RegisterNativeAllocation(EcmaRuntimeCallInfo *info)
{
    JSHandle<JSTaggedValue> h = GetCallArg(info, 0);
    auto *heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    auto size = h->GetInt();
    if (size < 0) {
        auto *thread = info->GetThread();
        THROW_RANGE_ERROR_AND_RETURN(thread, "The value must be non negative", JSTaggedValue::Exception());
    }
    heap->IncreaseNativeBindingSize(size);
    heap->TryTriggerFullMarkOrGCByNativeSize();
    WaitAndHandleConcurrentMarkingFinished(heap);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGc::RegisterNativeFree(EcmaRuntimeCallInfo *info)
{
    JSHandle<JSTaggedValue> h = GetCallArg(info, 0);
    auto *heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    auto size = h->GetInt();
    if (size < 0) {
        auto *thread = info->GetThread();
        THROW_RANGE_ERROR_AND_RETURN(thread, "The value must be non negative", JSTaggedValue::Exception());
    }
    auto allocated = heap->GetNativeBindingSize();
    heap->DecreaseNativeBindingSize(std::min(allocated, static_cast<size_t>(size)));
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGc::ClearWeakRefForTest(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    if (!((thread)->GetEcmaVM()->GetJSOptions().IsOpenArkTools())) {
        return JSTaggedValue::Undefined();
    }
    EcmaVM::ClearKeptObjects(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGc::WaitForFinishGC(EcmaRuntimeCallInfo *info)
{
    auto *heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    auto gcId = GetCallArg(info, 0)->GetInt();
    if (gcId <= 0) {
        return JSTaggedValue::Undefined();
    }

    WaitAndHandleConcurrentMarkingFinished(heap);

#ifndef NDEBUG
    heap->EnableTriggerCollectionOnNewObject();
#endif
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGc::StartGC(EcmaRuntimeCallInfo *info)
{
    static int counter = 0;
    auto *thread = info->GetThread();
    auto *heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    auto cause = StringToGcType(thread, GetCallArg(info, 0).GetTaggedValue());
    auto runGcInPlace = GetCallArg(info, 2).GetTaggedValue().ToBoolean();

    if (cause == GC_TYPE_LAST) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Invalid GC trigger type", JSTaggedValue::Exception());
    }
    switch (cause) {
        case SHARED_GC:
        case SHARED_PARTIAL_GC:
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::EXTERNAL_TRIGGER>(thread);
            return JSTaggedValue(0);
        case SHARED_FULL_GC:
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::EXTERNAL_TRIGGER>(
                thread);
            return JSTaggedValue(0);
        case APPSPAWN_SHARED_FULL_GC:
            SharedHeap::GetInstance()
                ->CollectGarbage<TriggerGCType::APPSPAWN_SHARED_FULL_GC, GCReason::EXTERNAL_TRIGGER>(thread);
            return JSTaggedValue(0);
        default:
            break;
    }
    if (cause != OLD_GC) {
        // except OLD_GC all run in place implicitly
        heap->CollectGarbage(cause, GCReason::EXTERNAL_TRIGGER);
        return JSTaggedValue(0);
    }

    heap->SetMarkType(MarkType::MARK_FULL);
    heap->TriggerConcurrentMarking(MarkReason::EXTERNAL_TRIGGER);

    if (heap->GetConcurrentMarker()->IsTriggeredConcurrentMark()) {
        JSHandle<JSTaggedValue> hCallback = GetCallArg(info, 1);
        if (!hCallback->IsUndefinedOrNull()) {
            if (!hCallback->IsJSFunction()) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "Invalid GC callback", JSTaggedValue::Exception());
            }

            auto undefined = thread->GlobalConstants()->GetHandledUndefined();
            auto *calleeInfo = EcmaInterpreter::NewRuntimeCallInfo(thread, hCallback, info->GetThis(), undefined, 0);
            JSFunction::Call(calleeInfo);
        }
    }

    if (runGcInPlace) {
        WaitAndHandleConcurrentMarkingFinished(heap);
        return JSTaggedValue(0);
    }

#ifndef NDEBUG
    heap->DisableTriggerCollectionOnNewObject();
#endif
    return JSTaggedValue(++counter);
}

void BuiltinsGc::WaitAndHandleConcurrentMarkingFinished(Heap *heap)
{
    if (heap->GetConcurrentMarker()->IsTriggeredConcurrentMark()) {
        heap->WaitConcurrentMarkingFinished();
        heap->GetConcurrentMarker()->HandleMarkingFinished(GCReason::EXTERNAL_TRIGGER);
    }
}

JSTaggedValue BuiltinsGc::AllocateArrayObject(EcmaRuntimeCallInfo *info)
{
    auto *thread = info->GetThread();
    auto *factory = thread->GetEcmaVM()->GetFactory();
    ASSERT(thread != nullptr);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    int64_t sizeInBytes = 0;

    if (GetCallArg(info, 0)->IsInt()) {
        sizeInBytes = GetCallArg(info, 0)->GetInt();
    } else if (GetCallArg(info, 0)->IsDouble()) {
        sizeInBytes = GetCallArg(info, 0)->GetDouble();
    } else {
        auto err = thread->GetEcmaVM()->GetFactory()->GetJSError(ErrorType::TYPE_ERROR, "The value must be an integer");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, err.GetTaggedValue(), JSTaggedValue::Exception());
    }

    if (sizeInBytes < 0) {
        auto err = thread->GetEcmaVM()->GetFactory()->GetJSError(ErrorType::TYPE_ERROR, "The value must be positive");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, err.GetTaggedValue(), JSTaggedValue::Exception());
    }

    sizeInBytes = RoundUp(sizeInBytes, sizeof(TaggedType)) - JSArray::SIZE;
    if (sizeInBytes < 0) {
        sizeInBytes = 0;
    }

    uint32_t numElements = static_cast<uint32_t>(sizeInBytes / sizeof(TaggedType));
    auto array = factory->NewJSArray();

    if (numElements > 0) {
        auto elements = factory->NewTaggedArray(numElements);
        if (elements.IsEmpty()) {
            return JSTaggedValue::Exception();
        }

        array->SetElements(thread, elements);
        array->SetArrayLength(thread, numElements);
    }

    return array.GetTaggedValue();
}

TriggerGCType BuiltinsGc::StringToGcType(JSThread *thread, JSTaggedValue cause)
{
    static_assert(GC_TYPE_LAST == 9, "Update this method after TrigerGCType change");
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetYoungGcCause(), cause)) {
        return YOUNG_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetOldGcCause(), cause)) {
        return OLD_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetFullGcCause(), cause)) {
        return FULL_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetAppSpawnFullGcCause(), cause)) {
        return APPSPAWN_FULL_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetSharedGcCause(), cause)) {
        return SHARED_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetSharedPartialGcCause(), cause)) {
        return SHARED_PARTIAL_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetSharedFullGcCause(), cause)) {
        return SHARED_FULL_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetAppSpawnSharedFullGcCause(), cause)) {
        return APPSPAWN_SHARED_FULL_GC;
    }
    if (JSTaggedValue::StrictEqual(thread->GlobalConstants()->GetUnifiedGcCause(), cause)) {
        return UNIFIED_GC;
    }
    return GC_TYPE_LAST;
}
}  // namespace panda::ecmascript::builtins
