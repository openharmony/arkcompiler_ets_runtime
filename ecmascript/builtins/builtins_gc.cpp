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
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap-inl.h"

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
    if (heap->GetConcurrentMarker()->IsTriggeredConcurrentMark()) {
        heap->WaitConcurrentMarkingFinished();
        heap->GetConcurrentMarker()->HandleMarkingFinished();
    }
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

JSTaggedValue BuiltinsGc::WaitForFinishGC(EcmaRuntimeCallInfo *info)
{
    auto *heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    heap->WaitAllTasksFinished();
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGc::StartGC(EcmaRuntimeCallInfo *info)
{
    auto *heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::EXTERNAL_TRIGGER);
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::builtins
