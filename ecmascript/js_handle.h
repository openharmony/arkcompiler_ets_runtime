/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSHANDLE_H
#define ECMASCRIPT_JSHANDLE_H

#include <type_traits>

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/barriers.h"
#include "common_interfaces/objects/readonly_handle.h"
/*
 * JSHandle: A JSHandle provides a reference to an object that survives relocation by the garbage collector.
 *
 * HandleStorage: Handles are only valid within a HandleScope. When a JSHandle is created for an object a cell is
 * allocated in the current HandleScope.
 *
 * HandleStorage: HandleStorage is the storage structure of the object pointer. GC will use the stored pointer as root
 * and update the stored value after the object is moved
 *
 *  JSHandle ---- HandleStorage -----  heap
 *    |               |               |
 * address-----> store: T*  ------> object
 *
 *    {
 *      EcmaHandleScope scope2(thread);
 *      JSHandle<T> jhandle(thread, obj4);
 *      JSHandle<T> jhandle(thread, obj5);
 *      JSHandle<T> jhandle(thread, obj6);
 *      JSHandle<T> jhandle(thread, obj7);
 *    }
 *
 *  // out of scope, The obj pointer in node will be free (obj7, obj6, obj5, obj4) and PopTopNode(top_node = prev_node)
 *
 *      |        |          |  obj5   |
 *      |        | scope2-> |  obj4   |
 *      |        |          |  obj3   |
 *      |  obj7  |          |  obj2   |
 *      |__obj6__| scope1-> |__obj1___|
 *       top_node --------->  prev_node------>nullptr
 *
 *  example:
 *      JSHandle<T> handle;
 *      {
 *          EcmaHandleScope(thread);
 *          JSHandle<T> jshandle(thread, T*);
 *          jshandle->method();  // to invoke method of T
 *          handle = jshandle;
 *      }
 *      handle->method(); // error! do not used handle out of scope
 */

namespace panda::test {
class JSHandleTest;
}  // namespace panda::test

namespace panda::ecmascript {
class TaggedArray;
class LinkedHashMap;
class LinkedHashSet;
class NameDictionary;

template <typename T>
class JSHandle {
public:
    inline JSHandle() : address_(reinterpret_cast<uintptr_t>(nullptr)) {}
    ~JSHandle() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(JSHandle);
    DEFAULT_COPY_SEMANTIC(JSHandle);

    JSHandle(const JSThread *thread, JSTaggedValue value)
    {
        address_ = EcmaHandleScope::NewHandle(const_cast<JSThread *>(thread), value.GetRawData());
    }

    JSHandle(const JSThread *thread, JSTaggedValue value, bool isPrimitive)
    {
        if (LIKELY(isPrimitive)) {
            address_ = EcmaHandleScope::NewPrimitiveHandle(
                const_cast<JSThread *>(thread), value.GetRawData());
            return;
        }
        address_ = EcmaHandleScope::NewHandle(const_cast<JSThread *>(thread), value.GetRawData());
    }

    JSHandle(const JSThread *thread, const TaggedObject *value)
    {
        address_ = EcmaHandleScope::NewHandle(const_cast<JSThread *>(thread), JSTaggedValue(value).GetRawData());
    }

    inline uintptr_t GetAddress() const
    {
        return address_;
    }

    template <typename S>
    explicit JSHandle(const JSHandle<S> &handle) : address_(handle.GetAddress()) {}

    template <typename S>
    inline static JSHandle<T> Cast(const JSHandle<S> &handle)
    {
        T::Cast(handle.GetTaggedValue().GetTaggedObject());
        return JSHandle<T>(handle.GetAddress());
    }

    inline JSTaggedValue GetTaggedValue() const
    {
        CHECK_NO_DEREF_HANDLE;
        if (GetAddress() == 0U) {
            return JSTaggedValue::Undefined();
        }
        return *reinterpret_cast<JSTaggedValue *>(GetAddress()); // NOLINT(clang-analyzer-core.NullDereference)
    }

    inline JSTaggedType GetTaggedType() const
    {
        CHECK_NO_DEREF_HANDLE;
        if (GetAddress() == 0U) {
            return JSTaggedValue::Undefined().GetRawData();
        }
        return *reinterpret_cast<JSTaggedType *>(GetAddress()); // NOLINT(clang-analyzer-core.NullDereference)
    }

    inline T *operator*() const
    {
        return T::Cast(GetTaggedValue().GetTaggedObject());
    }

    inline T *operator->() const
    {
        return T::Cast(GetTaggedValue().GetTaggedObject());
    }

    inline bool operator==(const JSHandle<T> &other) const
    {
        return GetTaggedType() == other.GetTaggedType();
    }

    inline bool operator!=(const JSHandle<T> &other) const
    {
        return GetTaggedType() != other.GetTaggedType();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == 0U;
    }

    template <typename R>
    R *GetObject() const
    {
        return reinterpret_cast<R *>(GetTaggedValue().GetTaggedObject());
    }

    inline explicit JSHandle(uintptr_t slot) : address_(slot)
    {
        if (!std::is_convertible<T *, JSTaggedValue *>::value) {
            ASSERT(slot != 0);
            if ((*reinterpret_cast<JSTaggedValue *>(slot)).IsHeapObject()) {
                T::Cast((*reinterpret_cast<JSTaggedValue *>(slot)).GetTaggedObject());
            }
        }
    }

    void Dump() const DUMP_API_ATTR
    {
        GetTaggedValue().D();
    }

    template <typename R>
    operator ReadOnlyHandle<R>()
    {
        return ReadOnlyHandle<R>(address_);
    }

    template <typename R>
    operator const ReadOnlyHandle<R>() const
    {
        return ReadOnlyHandle<R>(address_);
    }
private:
    inline explicit JSHandle(const JSTaggedType *slot) : address_(reinterpret_cast<uintptr_t>(slot)) {}
    inline explicit JSHandle(const T *const *slot) : address_(reinterpret_cast<uintptr_t>(slot)) {}

    uintptr_t address_;  // NOLINT(misc-non-private-member-variables-in-classes)
    friend class EcmaVM;
    friend class GlobalEnv;
    friend class JSHandleTest;
    friend class GlobalHandleCollection;
    friend class RuntimeStubs;
};

template <>
inline JSTaggedValue *JSHandle<JSTaggedValue>::operator->() const
{
    Barriers::UpdateSlot(reinterpret_cast<JSTaggedValue*>(GetAddress()), 0);
    return reinterpret_cast<JSTaggedValue *>(GetAddress());
}

template <>
inline JSTaggedValue *JSHandle<JSTaggedValue>::operator*() const
{
    Barriers::UpdateSlot(reinterpret_cast<JSTaggedValue*>(GetAddress()), 0);
    return reinterpret_cast<JSTaggedValue *>(GetAddress());
}

template <typename T>
class JSMutableHandle : public JSHandle<T> {
public:
    JSMutableHandle() = default;
    ~JSMutableHandle() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(JSMutableHandle);
    DEFAULT_COPY_SEMANTIC(JSMutableHandle);

    JSMutableHandle(const JSThread *thread, JSTaggedValue value) : JSHandle<T>(thread, value) {}
    JSMutableHandle(const JSThread *thread, const TaggedArray *value) : JSHandle<T>(thread, value) {}
    template <typename S>
    JSMutableHandle(const JSThread *thread, const JSHandle<S> &handle)
        : JSHandle<T>(thread, handle.GetTaggedValue())
    {
    }
    inline explicit JSMutableHandle(uintptr_t slot) : JSHandle<T>(slot)
    {
    }

    template <typename S>
    inline static JSMutableHandle<T> Cast(const JSMutableHandle<S> &handle)
    {
        JSHandle<T>::Cast(handle);
        return JSMutableHandle<T>(handle.GetAddress());
    }

    void Update(JSTaggedValue value)
    {
        auto addr = reinterpret_cast<JSTaggedValue *>(this->GetAddress());
        ASSERT(addr != nullptr);
        *addr = value;
    }

    template <typename S>
    void Update(const JSHandle<S> &handle)
    {
        auto addr = reinterpret_cast<JSTaggedValue *>(this->GetAddress());
        *addr = handle.GetTaggedValue();
    }
};

template<typename>
struct IsJSHandle : std::false_type {};

template<typename Value>
struct IsJSHandle<JSHandle<Value>> : std::true_type {};

template<typename Value>
struct IsJSHandle<JSMutableHandle<Value>> : std::true_type {};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSHANDLE_H
