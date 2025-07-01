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

#ifndef ECMASCRIPT_VTABLE_H
#define ECMASCRIPT_VTABLE_H

#include "ecmascript/weak_vector.h"

namespace panda::ecmascript {
class VTable : public TaggedArray {
public:
    static constexpr uint32_t DEFAULT_SUPERS_CAPACITY = 4;

    enum TupleItem {
        NAME = 0,
        TYPE,
        OWNER,
        OFFSET,

        ITEM_NUM,
        ITEM_FIRST = NAME,
        ITEM_LAST = OFFSET,
    };

    class Tuple {
    public:
        JSHandle<JSTaggedValue> GetItem(TupleItem item) const
        {
            return items_[item];
        }
    private:
        // only allow VTable to create a instance of Tuple
        explicit Tuple(const CVector<JSHandle<JSTaggedValue>> &vec) : items_(std::move(vec)) {}
        CVector<JSHandle<JSTaggedValue>> items_;

        friend class VTable;
    };

    enum TypeKind {
        FUNCTION = 0,
        ACCESSOR,
        NORMAL
    };

    static constexpr uint32_t TUPLE_SIZE = 4;

    CAST_CHECK(VTable, IsTaggedArray);

    static Tuple CreateTuple(const JSThread *thread, JSTaggedValue phc,
                             const JSHandle<JSTaggedValue> &owner, uint32_t propIndex);

    static JSHandle<VTable> Copy(const JSThread *thread, const JSHandle<VTable> &vtable);

    uint32_t GetNumberOfTuples() const
    {
        return GetLength() / TUPLE_SIZE;
    }

    JSTaggedValue GetTupleItem(const JSThread *thread, uint32_t tupleIdx, TupleItem kind) const
    {
        return Get(thread, tupleIdx * TUPLE_SIZE + kind);
    }

    bool IsAccessor(const JSThread *thread, uint32_t tupleIdx) const
    {
        TypeKind type = static_cast<TypeKind>(GetTupleItem(thread, tupleIdx, TupleItem::TYPE).GetInt());
        return type == TypeKind::ACCESSOR;
    }

    Tuple GetTuple(const JSThread *thread, uint32_t tupleIdx) const;

    void SetByIndex(const JSThread *thread, uint32_t idx, const VTable::Tuple &tuple);

    void Trim(const JSThread *thread, uint32_t newLength);

    int GetTupleIndexByName(const JSThread *thread, JSTaggedValue val) const;

    bool Find(const JSThread *thread, JSTaggedValue val) const;

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_VTABLE_H