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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_H
#define ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

// List of functions in Promise, excluding the '@@' propertiex.
// V(name, func, length, stubIndex)
// where BuiltinsPromise::func refers to the native implementation of Promise[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_PROMISE_FUNCTIONS(V)                \
    /* Promise.all ( iterable ) */                  \
    V("all",        All,        1, INVALID)         \
    /* Promise.allSettled ( iterable ) */           \
    V("allSettled", AllSettled, 1, INVALID)         \
    /* Promise.any ( iterable ) */                  \
    V("any",        Any,        1, INVALID)         \
    /* Promise.race ( iterable ) */                 \
    V("race",       Race,       1, INVALID)         \
    /* Promise.reject ( r ) */                      \
    V("reject",     Reject,     1, INVALID)         \
    /* Promise.resolve ( x ) */                     \
    V("resolve",    Resolve,    1, INVALID)

// List of functions in Promise.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsPromise::func refers to the native implementation of Promise.prototype[name].
#define BUILTIN_PROMISE_PROTOTYPE_FUNCTIONS(V)                  \
    /* Promise.prototype.catch ( onRejected ) */                \
    V("catch",   Catch,   1, INVALID)                           \
    /* Promise.prototype.finally ( onFinally ) */               \
    V("finally", Finally, 1, INVALID)                           \
    /* Promise.prototype.then ( onFulfilled, onRejected ) */    \
    V("then",    Then,    2, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsPromise : public base::BuiltinsBase {
public:
    // 25.4.3.1 Promise ( executor )
    static JSTaggedValue PromiseConstructor(EcmaRuntimeCallInfo *argv);
    // 25.4.4.1 Promise.all ( iterable )
    static JSTaggedValue All(EcmaRuntimeCallInfo *argv);
    // 25.4.4.3 Promise.race ( iterable )
    static JSTaggedValue Race(EcmaRuntimeCallInfo *argv);
    // 25.4.4.4 Promise.reject ( r )
    static JSTaggedValue Reject(EcmaRuntimeCallInfo *argv);
    // 25.4.4.5 Promise.resolve ( x )
    static JSTaggedValue Resolve(EcmaRuntimeCallInfo *argv);
    // 25.4.4.6 get Promise [ @@species ]
    static JSTaggedValue GetSpecies(EcmaRuntimeCallInfo *argv);
    // 25.4.5.1 Promise.prototype.catch ( onRejected )
    static JSTaggedValue Catch(EcmaRuntimeCallInfo *argv);
    // 25.4.5.3 Promise.prototype.then ( onFulfilled , onRejected )
    static JSTaggedValue Then(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue PerformPromiseThen(JSThread *thread, const JSHandle<JSPromise> &promise,
                                            const JSHandle<JSTaggedValue> &onFulfilled,
                                            const JSHandle<JSTaggedValue> &onRejected,
                                            const JSHandle<PromiseCapability> &capability);

    static JSTaggedValue Any(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue AllSettled(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Finally(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetPromiseResolve(JSThread *thread, JSHandle<JSTaggedValue> promiseConstructor);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetPromiseFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(PROMISE_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetPromisePrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(PROMISE_PROTOTYPE_FUNCTIONS);
    }

private:
#define BUILTIN_PROMISE_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsPromise::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array PROMISE_FUNCTIONS  = {
        BUILTIN_PROMISE_FUNCTIONS(BUILTIN_PROMISE_FUNCTION_ENTRY)
    };
    static constexpr std::array PROMISE_PROTOTYPE_FUNCTIONS = {
        BUILTIN_PROMISE_PROTOTYPE_FUNCTIONS(BUILTIN_PROMISE_FUNCTION_ENTRY)
    };
#undef BUILTIN_PROMISE_FUNCTION_ENTRY

    static JSTaggedValue PerformPromiseAll(JSThread *thread,
                                           const JSHandle<PromiseIteratorRecord> &itRecord,
                                           const JSHandle<JSTaggedValue> &ctor,
                                           const JSHandle<PromiseCapability> &capa);

    static JSHandle<CompletionRecord> PerformPromiseRace(JSThread *thread,
                                                         const JSHandle<PromiseIteratorRecord> &iteratorRecord,
                                                         const JSHandle<PromiseCapability> &capability,
                                                         const JSHandle<JSTaggedValue> &constructor);

    static JSHandle<CompletionRecord> PerformPromiseAllSettled(JSThread *thread,
                                                               const JSHandle<PromiseIteratorRecord> &iterRecord,
                                                               const JSHandle<JSTaggedValue> &constructor,
                                                               const JSHandle<PromiseCapability> &resultCapa,
                                                               const JSHandle<JSTaggedValue> &promiseResolve);

    static JSHandle<CompletionRecord> PerformPromiseAny(JSThread *thread,
                                                        const JSHandle<PromiseIteratorRecord> &iteratorRecord,
                                                        const JSHandle<JSTaggedValue> &constructor,
                                                        const JSHandle<PromiseCapability> &resultCapability,
                                                        const JSHandle<JSTaggedValue> &promiseResolve);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_H