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

#ifndef ECMASCRIPT_JSPROXY_H
#define ECMASCRIPT_JSPROXY_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_object.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class JSProxy final : public ECMAObject {
public:
    CAST_CHECK(JSProxy, IsJSProxy);

    // ES6 9.5.15 ProxyCreate(target, handler)
    static JSHandle<JSProxy> ProxyCreate(JSThread *thread, const JSHandle<JSTaggedValue> &target,
                                         const JSHandle<JSTaggedValue> &handler);
    // ES6 9.5.1 [[GetPrototypeOf]] ( )
    static JSTaggedValue GetPrototype(JSThread *thread, const JSHandle<JSProxy> &proxy);
    // ES6 9.5.2 [[SetPrototypeOf]] (V)
    static bool SetPrototype(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &proto);
    // ES6 9.5.3 [[IsExtensible]] ( )
    static bool IsExtensible(JSThread *thread, const JSHandle<JSProxy> &proxy);
    // ES6 9.5.4 [[PreventExtensions]] ( )
    static bool PreventExtensions(JSThread *thread, const JSHandle<JSProxy> &proxy);
    // ES6 9.5.5 [[GetOwnProperty]] (P)
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);
    // ES6 9.5.6 [[DefineOwnProperty]] (P, Desc)
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                                  const PropertyDescriptor &desc);
    // ES6 9.5.7 [[HasProperty]] (P)
    static bool HasProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key);
    // ES6 9.5.8 [[Get]] (P, Receiver)
    static inline OperationResult GetProperty(JSThread *thread, const JSHandle<JSProxy> &proxy,
                                              const JSHandle<JSTaggedValue> &key)
    {
        return GetProperty(thread, proxy, key, JSHandle<JSTaggedValue>::Cast(proxy));
    }
    static OperationResult CheckGetTrapResult(JSThread *thread, const JSHandle<JSTaggedValue> &targetHandle,
                                              const JSHandle<JSTaggedValue> &key,
                                              const JSHandle<JSTaggedValue> &resultHandle);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSProxy> &proxy,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);
    // ES6 9.5.9 [[Set]] ( P, V, Receiver)
    static inline bool SetProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value, bool mayThrow = false)
    {
        return SetProperty(thread, proxy, key, value, JSHandle<JSTaggedValue>::Cast(proxy), mayThrow);
    }
    static bool CheckSetTrapResult(JSThread *thread, const JSHandle<JSTaggedValue> &targetHandle,
                                   const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value);
    static bool SetProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver,
                            bool mayThrow = false);
    // ES6 9.5.10 [[Delete]] (P)
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key);

    // ES6 9.5.12 [[OwnPropertyKeys]] ()
    static JSHandle<TaggedArray> OwnPropertyKeys(JSThread *thread, const JSHandle<JSProxy> &proxy);

    static JSHandle<TaggedArray> GetAllPropertyKeys(JSThread *thread, const JSHandle<JSProxy> &proxy, uint32_t filter);

    void SetCallable(bool callable) const
    {
        GetClass()->SetCallable(callable);
    }

    void SetConstructor(bool constructor) const
    {
        GetClass()->SetConstructor(constructor);
    }

    JSHandle<JSTaggedValue> GetSourceTarget(JSThread *thread) const;

    // ES6 9.5.13 [[Call]] (thisArgument, argumentsList)
    static JSTaggedValue CallInternal(EcmaRuntimeCallInfo *info);
    // ES6 9.5.14 [[Construct]] ( argumentsList, newTarget)
    static JSTaggedValue ConstructInternal(EcmaRuntimeCallInfo *info);

    bool PUBLIC_API IsArray(JSThread *thread) const;

    static constexpr size_t TARGET_OFFSET = ECMAObject::SIZE;
    ACCESSORS(Target, TARGET_OFFSET, HANDLER_OFFSET)
    ACCESSORS(Handler, HANDLER_OFFSET, METHOD_OFFSET)
    ACCESSORS(Method, METHOD_OFFSET, PRIVATE_FIELD_OFFSET)
    ACCESSORS(PrivateField, PRIVATE_FIELD_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t IS_REVOKED_BITS = 1;
    FIRST_BIT_FIELD(BitField, IsRevoked, bool, IS_REVOKED_BITS)

    DECL_DUMP()

    DECL_VISIT_OBJECT(TARGET_OFFSET, BIT_FIELD_OFFSET)
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPROXY_H
