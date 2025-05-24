/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SATTIC_MODULE_PROXY_HANDLER_H
#define ECMASCRIPT_SATTIC_MODULE_PROXY_HANDLER_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
class StaticModuleProxyHandler {
public:
    static JSHandle<JSProxy> CreateStaticModuleProxyHandler(JSThread *thread,
        const JSHandle<JSTaggedValue> exportObject);
    // [[SetPrototypeOf]]
    static JSTaggedValue SetPrototype(EcmaRuntimeCallInfo *argv);

    // [[GetPrototypeOf]]
    static JSTaggedValue GetPrototype(EcmaRuntimeCallInfo *argv);

    // [[PreventExtensions]]
    static JSTaggedValue PreventExtensions(EcmaRuntimeCallInfo *argv);
    // [[GetOwnProperty]]
    static JSTaggedValue GetOwnProperty(EcmaRuntimeCallInfo *argv);

    static bool GetOwnPropertyInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);
    // [[DefineOwnProperty]] ( P, Desc )
    static JSTaggedValue DefineOwnProperty(EcmaRuntimeCallInfo *argv);
    // [[HasProperty]]
    static JSTaggedValue HasProperty(EcmaRuntimeCallInfo *argv);
    // [[Get]] ( P, Receiver )
    static JSTaggedValue GetProperty(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetPropertyInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key);
    // [[Set]] ( P, V, Receiver )
    static JSTaggedValue SetProperty(EcmaRuntimeCallInfo *argv);
    // [[Delete]] ( P )
    static JSTaggedValue DeleteProperty(EcmaRuntimeCallInfo *argv);
    // [[OwnPropertyKeys]]
    static JSTaggedValue OwnPropertyKeys(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue OwnEnumPropertyKeys(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue IsExtensible(EcmaRuntimeCallInfo *argv);

private:
    static int const FIRST = 0;
    static int const SECOND = 1;
    static int const THIRD = 2;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SATTIC_MODULE_PROXY_HANDLER_H
