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

#ifndef ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H
#define ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H

#include "ecmascript/mem/tagged_object.h"

#include <atomic>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript {
inline void TaggedObject::SetClassWithoutBarrier(JSHClass *hclass)
{
    class_ = reinterpret_cast<MarkWordType>(hclass);
}

inline void TaggedObject::SetClass(const JSThread *thread, JSHClass *hclass)
{
    Barriers::SetObject<true>(thread, this, 0, JSTaggedValue(hclass).GetRawData());
}

inline void TaggedObject::SetClass(const JSThread *thread, JSHandle<JSHClass> hclass)
{
    SetClass(thread, *hclass);
}

inline void TaggedObject::SynchronizedSetClass(const JSThread *thread, JSHClass *hclass)
{
    Barriers::SynchronizedSetClass(thread, this, JSTaggedValue(hclass).GetRawData());
}

inline JSHClass *TaggedObject::SynchronizedGetClass() const
{
    return reinterpret_cast<JSHClass *>(
        reinterpret_cast<volatile std::atomic<MarkWordType> *>(ToUintPtr(this))->load(std::memory_order_acquire));
}
}  //  namespace panda::ecmascript

#endif  // ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H
