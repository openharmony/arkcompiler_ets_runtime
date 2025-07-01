/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/js_finalization_registry.h"

#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript {
// -------------------------------CellRecordVector-----------------------------------
JSHandle<CellRecordVector> CellRecordVector::Append(const JSThread *thread, const JSHandle<CellRecordVector> &vector,
                                                    const JSHandle<JSTaggedValue> &value)
{
    JSHandle<WeakVector> oldVector(vector);
    JSHandle<WeakVector> newVector = WeakVector::FillOrAppend(thread, oldVector, value);
    return JSHandle<CellRecordVector>(newVector);
}

bool CellRecordVector::IsEmpty(const JSThread *thread)
{
    if (Empty()) {
        return true;
    }
    for (uint32_t i = 0; i < GetEnd(); i++) {
        JSTaggedValue value = Get(thread, i);
        if (!value.IsHole()) {
            return false;
        }
    }
    return true;
}

// ---------------------------JSFinalizationRegistry-----------------------------------
void JSFinalizationRegistry::Register(JSThread *thread, JSHandle<JSTaggedValue> target,
                                      JSHandle<JSTaggedValue> heldValue,
                                      JSHandle<JSTaggedValue> unregisterToken, JSHandle<JSFinalizationRegistry> obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CellRecord> cellRecord = factory->NewCellRecord();
    cellRecord->SetToWeakRefTarget(thread, target.GetTaggedValue());
    cellRecord->SetHeldValue(thread, heldValue);
    JSHandle<JSTaggedValue> cell(cellRecord);
    // If unregisterToken is undefined, we use vector to store
    // otherwise we use hash map to store to facilitate subsequent delete operations
    if (!unregisterToken->IsUndefined()) {
        JSHandle<LinkedHashMap> maybeUnregister(thread, obj->GetMaybeUnregister(thread));
        JSHandle<CellRecordVector> array(thread, JSTaggedValue::Undefined());
        if (maybeUnregister->Has(thread, unregisterToken.GetTaggedValue())) {
            array = JSHandle<CellRecordVector>(thread, maybeUnregister->Get(thread, unregisterToken.GetTaggedValue()));
        } else {
            array = JSHandle<CellRecordVector>(CellRecordVector::Create(thread));
        }
        array = CellRecordVector::Append(thread, array, cell);
        JSHandle<JSTaggedValue> arrayValue(array);
        maybeUnregister = LinkedHashMap::SetWeakRef(thread, maybeUnregister, unregisterToken, arrayValue);
        obj->SetMaybeUnregister(thread, maybeUnregister);
    } else {
        JSHandle<CellRecordVector> noUnregister(thread, obj->GetNoUnregister(thread));
        noUnregister = CellRecordVector::Append(thread, noUnregister, cell);
        obj->SetNoUnregister(thread, noUnregister);
    }
    JSFinalizationRegistry::AddFinRegLists(thread, obj);
}

bool JSFinalizationRegistry::Unregister(JSThread *thread, JSHandle<JSTaggedValue> UnregisterToken,
                                        JSHandle<JSFinalizationRegistry> obj)
{
    // Because we have stored the object that may be unregistered in the hash map when registering,
    // at this time we just need to find it in the map and delete it
    JSHandle<LinkedHashMap> maybeUnregister(thread, obj->GetMaybeUnregister(thread));
    int entry = maybeUnregister->FindElement(thread, UnregisterToken.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    maybeUnregister->RemoveEntry(thread, entry);
    JSHandle<LinkedHashMap> newMaybeUnregister = LinkedHashMap::Shrink(thread, maybeUnregister);
    obj->SetMaybeUnregister(thread, newMaybeUnregister);
    return true;
}

void JSFinalizationRegistry::CleanFinRegLists(JSThread *thread, JSHandle<JSFinalizationRegistry> obj)
{
    EcmaVM* vm = thread->GetEcmaVM();
    if (obj->GetPrev(thread).IsNull() && obj->GetNext(thread).IsNull()) {
        vm->SetFinRegLists(JSTaggedValue::Hole());
        return;
    }
    if (!obj->GetPrev(thread).IsNull()) {
        JSHandle<JSFinalizationRegistry> prev(thread, obj->GetPrev(thread));
        prev->SetNext(thread, obj->GetNext(thread));
    }
    if (!obj->GetNext(thread).IsNull()) {
        JSHandle<JSFinalizationRegistry> next(thread, obj->GetNext(thread));
        next->SetPrev(thread, obj->GetPrev(thread));
    } else {
        vm->SetFinRegLists(obj->GetPrev(thread));
    }
    obj->SetPrev(thread, JSTaggedValue::Null());
    obj->SetNext(thread, JSTaggedValue::Null());
}

void JSFinalizationRegistry::CheckAndCall(JSThread *thread)
{
    if (thread->GetCheckAndCallEnterState()) {
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CheckAndCallScope scope(thread);
    EcmaVM* vm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> prev(thread, vm->GetFinRegLists());

    if (prev->IsHole()) {
        return;
    }
    JSMutableHandle<JSTaggedValue> current(thread, prev.GetTaggedValue());
    JSMutableHandle<JSFinalizationRegistry> jsFinalizationRegistry(thread, current.GetTaggedValue());
    while (!current->IsNull()) {
        jsFinalizationRegistry.Update(current.GetTaggedValue());
        current.Update(jsFinalizationRegistry->GetPrev(thread));
        if (CleanupFinalizationRegistry(thread, jsFinalizationRegistry)) {
            // If the objects registered on the current JSFinalizationRegistry object have all been gc,
            // then we clean up this JSFinalizationRegistry object from the FinRegLists
            CleanFinRegLists(thread, jsFinalizationRegistry);
        }
    }
}

void DealCallBackOfMap(JSThread *thread, JSHandle<CellRecordVector> &cellVect,
    JSHandle<job::MicroJobQueue> &job, JSHandle<JSFunction> &func)
{
    if (!cellVect->Empty()) {
        uint32_t cellVectLen = cellVect->GetEnd();
        for (uint32_t i = 0; i < cellVectLen; ++i) {
            JSTaggedValue value = cellVect->Get(thread, i);
            if (value.IsHole()) {
                continue;
            }
            JSHandle<CellRecord> cellRecord(thread, value);
            // if WeakRefTarget have been gc, set callback to job and delete
            if (cellRecord->GetFromWeakRefTarget(thread).IsUndefined()) {
                JSHandle<TaggedArray> argv = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(1);
                argv->Set(thread, 0, cellRecord->GetHeldValue(thread));
                job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, func, argv);
                cellVect->Delete(thread, i);
            }
        }
    }
}

bool JSFinalizationRegistry::CleanupFinalizationRegistry(JSThread *thread, JSHandle<JSFinalizationRegistry> obj)
{
    // 1. Assert: finalizationRegistry has [[Cells]] and [[CleanupCallback]] internal slots.
    // 2. Let callback be finalizationRegistry.[[CleanupCallback]].
    // 3. While finalizationRegistry.[[Cells]] contains a Record cell such that cell.[[WeakRefTarget]] is empty,
    // an implementation may perform the following steps:
    //     a. Choose any such cell.
    //     b. Remove cell from finalizationRegistry.[[Cells]].
    //     c. Perform ? HostCallJobCallback(callback, undefined, « cell.[[HeldValue]] »).
    // 4. Return unused.
    ASSERT(obj->IsECMAObject());
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<job::MicroJobQueue> job = ecmaVm->GetMicroJobQueue();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSFunction> func(thread, obj->GetCleanupCallback(thread));
    JSHandle<CellRecordVector> noUnregister(thread, obj->GetNoUnregister(thread));
    if (!noUnregister->Empty()) {
        uint32_t noUnregisterLen = noUnregister->GetEnd();
        for (uint32_t i = 0; i < noUnregisterLen; ++i) {
            JSTaggedValue value = noUnregister->Get(thread, i);
            if (value.IsHole()) {
                continue;
            }
            JSHandle<CellRecord> cellRecord(thread, value);
            // if WeakRefTarget have been gc, set callback to job and delete
            if (cellRecord->GetFromWeakRefTarget(thread).IsUndefined()) {
                JSHandle<TaggedArray> argv = factory->NewTaggedArray(1);
                argv->Set(thread, 0, cellRecord->GetHeldValue(thread));
                job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, func, argv);
                noUnregister->Delete(thread, i);
            }
        }
    }
    JSMutableHandle<LinkedHashMap> maybeUnregister(thread, obj->GetMaybeUnregister(thread));
    int index = 0;
    int totalElements = maybeUnregister->NumberOfElements() + maybeUnregister->NumberOfDeletedElements();
    while (index < totalElements) {
        if (!maybeUnregister->GetKey(thread, index++).IsHole()) {
            JSHandle<CellRecordVector> cellVect(thread, maybeUnregister->GetValue(thread, index - 1));
            DealCallBackOfMap(thread, cellVect, job, func);
            if (!cellVect->Empty()) {
                continue;
            }
            maybeUnregister->RemoveEntry(thread, index - 1);
        }
    }
    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Shrink(thread, maybeUnregister);
    obj->SetMaybeUnregister(thread, newMap);
    // Determine whether the objects registered on the current JSFinalizationRegistry object
    // have all been gc
    int remainSize = newMap->NumberOfElements() + newMap->NumberOfDeletedElements();
    if (noUnregister->IsEmpty(thread) && remainSize == 0) {
        return true;
    }
    return false;
}

void JSFinalizationRegistry::AddFinRegLists(JSThread *thread, JSHandle<JSFinalizationRegistry> next)
{
    // If any of prev and next is not null, it means that the current object is already in the linked list,
    // ignore this addition
    if (!next->GetPrev(thread).IsNull() || !next->GetNext(thread).IsNull()) {
        return;
    }
    EcmaVM* vm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> lists(thread, vm->GetFinRegLists());
    if (!lists->IsHole()) {
        JSHandle<JSFinalizationRegistry> prev(lists);
        // Prevent the same object from being connected end to end,
        // which should not happen and will lead to an infinite loop
        if (JSTaggedValue::SameValue(thread, next.GetTaggedValue(), prev.GetTaggedValue())) {
            return;
        }
        prev->SetNext(thread, next);
        next->SetPrev(thread, prev);
    }
    vm->SetFinRegLists(next.GetTaggedValue());
}
} // namespace
