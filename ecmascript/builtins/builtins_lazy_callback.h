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
 
#ifndef ECMASCRIPT_BUILTINS_BUILTINS_LAZY_CALLBACK_H
#define ECMASCRIPT_BUILTINS_BUILTINS_LAZY_CALLBACK_H

#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ITERATE_TYPED_ARRAY(V) \
    V(Int8Array)               \
    V(Uint8Array)              \
    V(Uint8ClampedArray)       \
    V(Int16Array)              \
    V(Uint16Array)             \
    V(Int32Array)              \
    V(Uint32Array)             \
    V(Float32Array)            \
    V(Float64Array)            \
    V(BigInt64Array)           \
    V(BigUint64Array)

#define ITERATE_INTL(V)       \
    V(Locale)                 \
    V(DateTimeFormat)         \
    V(NumberFormat)           \
    V(RelativeTimeFormat)     \
    V(Collator)               \
    V(PluralRules)            \
    V(DisplayNames)           \
    V(Segmenter)              \
    V(Segments)               \
    V(ListFormat)             \

namespace panda::ecmascript::builtins {
class BuiltinsLazyCallback {
public:
    static JSTaggedValue Date(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Set(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Map(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue WeakMap(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue WeakSet(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue WeakRef(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue FinalizationRegistry(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue TypedArray(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Int8Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Uint8Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Uint8ClampedArray(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Int16Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Uint16Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Int32Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Uint32Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Float32Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Float64Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue BigInt64Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue BigUint64Array(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue ArrayBuffer(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue DataView(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue SharedArrayBuffer(JSThread *thread, const JSHandle<JSObject> &obj);
#ifdef ARK_SUPPORT_INTL
    static JSTaggedValue Locale(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue DateTimeFormat(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue NumberFormat(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue RelativeTimeFormat(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Collator(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue PluralRules(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue DisplayNames(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Segmenter(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue Segments(JSThread *thread, const JSHandle<JSObject> &obj);
    static JSTaggedValue ListFormat(JSThread *thread, const JSHandle<JSObject> &obj);
#endif

private:
    static void ResetLazyInternalAttr(JSThread *thread, const JSHandle<JSObject> &object, const char *key);
};
}   // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_LAZY_CALLBACK_H