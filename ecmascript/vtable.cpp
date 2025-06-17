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

#include "ecmascript/vtable.h"
#include "ecmascript/layout_info-inl.h"

namespace panda::ecmascript {
VTable::Tuple VTable::CreateTuple(const JSThread *thread, JSTaggedValue phc,
                                  const JSHandle<JSTaggedValue> &owner, uint32_t propIndex)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *phcPoint = JSHClass::Cast(phc.GetTaggedObject());
    LayoutInfo *layoutInfo = LayoutInfo::Cast(phcPoint->GetLayout(thread).GetTaggedObject());
    JSHandle<JSTaggedValue> name(thread, layoutInfo->GetKey(thread, propIndex));

    // get type
    JSTaggedValue typeVal;
    PropertyAttributes attr = layoutInfo->GetAttr(thread, propIndex);
    if (attr.IsAccessor()) {
        typeVal = JSTaggedValue(VTable::TypeKind::ACCESSOR);
    } else {
        typeVal = JSTaggedValue(VTable::TypeKind::FUNCTION);
    }
    JSHandle<JSTaggedValue> type(thread, typeVal);

    // get offset
    uint32_t propsNumber = phcPoint->NumberOfProps();
    int entry = layoutInfo->FindElementWithCache(thread, phcPoint, name.GetTaggedValue(), propsNumber);
    ASSERT(entry != -1);
    uint32_t offsetInt = phcPoint->GetInlinedPropertiesOffset(static_cast<uint32_t>(entry));
    JSHandle<JSTaggedValue> offset(thread, JSTaggedValue(offsetInt));

    CVector<JSHandle<JSTaggedValue>> vec {name, type, owner, offset};
    return VTable::Tuple(vec);
}

VTable::Tuple VTable::GetTuple(const JSThread *thread, uint32_t tupleIdx) const
{
    CVector<JSHandle<JSTaggedValue>> vec;
    for (uint32_t loc = 0; loc < ITEM_NUM; ++loc) {
        JSTaggedValue val = Get(thread, tupleIdx * TUPLE_SIZE + loc);
        vec.emplace_back(JSHandle<JSTaggedValue>(thread, val));
    }
    return Tuple(vec);
}

void VTable::SetByIndex(const JSThread *thread, uint32_t idx, const VTable::Tuple &tuple)
{
    uint32_t beginIdx = idx * TUPLE_SIZE;
    for (uint32_t i = 0; i < TUPLE_SIZE; ++i) {
        uint32_t currIdx = beginIdx + i;
        JSTaggedValue val = tuple.GetItem(TupleItem(i)).GetTaggedValue();
        Set(thread, currIdx, val);
    }
}

void VTable::Trim(const JSThread *thread, uint32_t newLength)
{
    TaggedArray::Trim(thread, newLength * VTable::TUPLE_SIZE);
}

int VTable::GetTupleIndexByName(const JSThread *thread, JSTaggedValue name) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = GetNumberOfTuples();
    if (len == 0) {
        return -1;
    }

    [[maybe_unused]] EcmaString *str = EcmaString::Cast(name.GetTaggedObject());
    ASSERT_PRINT(EcmaStringAccessor(str).IsInternString(), "The name of the property is not an intern string");

    for (uint32_t index = 0; index < len; ++index) {
        JSTaggedValue nameVal = GetTupleItem(thread, index, VTable::TupleItem::NAME);
        if (nameVal == name) {
            return index;
        }
    }

    return -1;
}

bool VTable::Find(const JSThread *thread, JSTaggedValue name) const
{
    return GetTupleIndexByName(thread, name) != -1;
}

JSHandle<VTable> VTable::Copy(const JSThread *thread, const JSHandle<VTable> &vtable)
{
    uint32_t length = vtable->GetLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> copyVtable = factory->CopyArray(JSHandle<TaggedArray>(vtable), length, length);
    return JSHandle<VTable>(copyVtable);
}
}  // namespace panda::ecmascript
