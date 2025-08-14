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

#ifndef ECMASCRIPT_NAPI_JSNAPI_HELPER_H
#define ECMASCRIPT_NAPI_JSNAPI_HELPER_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/napi/include/jsnapi.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_VALUE_IF_ABRUPT(thread, value)                 \
    do {                                                      \
        if (thread->HasPendingException()) {                  \
            LOG_FULL(WARN) << "occur exception need return";  \
            return value;                                     \
        }                                                     \
    } while (false)

#if ECMASCRIPT_ENABLE_NAPI_SPECIAL_CHECK
#define LOG_IF_SPECIAL(handleValue, level)                                                       \
    do {                                                                                         \
        LOG_ECMA(DEBUG) << "Enable napi special check";                                           \
        if ((handleValue).GetTaggedValue().IsSpecial()) {                                        \
            LOG_FULL(level) << "Special value " << (handleValue).GetTaggedType() << " checked!"; \
        }                                                                                        \
    } while (false)
#else
#define LOG_IF_SPECIAL(handleValue, level) static_cast<void>(0)
#endif

#define CROSS_THREAD_CHECK(vm)                                                                  \
    [[maybe_unused]] JSThread *thread = (vm)->GetJSThread()

#define CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, returnVal)                              \
    CROSS_THREAD_CHECK(vm);                                                                      \
    do {                                                                                         \
        if (thread->HasPendingException()) {                                                     \
            LOG_ECMA(ERROR) << "Pending exception before " << __FUNCTION__ << " called in line:" \
                            << __LINE__ << ", exception details as follows:";                    \
            JSNApi::PrintExceptionInfo(vm);                                                      \
            return returnVal;                                                                    \
        }                                                                                        \
    } while (false)

#define CROSS_THREAD_AND_EXCEPTION_CHECK(vm)                                                      \
    CROSS_THREAD_CHECK(vm);                                                                       \
    do {                                                                                          \
        if (thread->HasPendingException()) {                                                      \
            LOG_ECMA(ERROR) << "Pending exception before " << __FUNCTION__ << " called, in line:" \
                            << __LINE__ << ", exception details as follows:";                     \
            JSNApi::PrintExceptionInfo(vm);                                                       \
            return;                                                                               \
        }                                                                                         \
    } while (false)

#define SINGLE_THREAD_CHECK_WITH_RETURN(vm, result)                                              \
    [[maybe_unused]] JSThread *thread = (vm)->GetJSThreadNoCheck();                              \
    do {                                                                                         \
        if (thread->HasPendingException()) {                                                     \
            LOG_ECMA(DEBUG) << "Pending exception before " << __FUNCTION__ << " called in line:" \
                            << __LINE__ << ", exception details as follows:";                    \
            JSNApi::PrintExceptionInfo(vm);                                                      \
            return result;                                                                       \
        }                                                                                        \
        if (!(vm)->CheckSingleThread()) {                                                        \
            LOG_ECMA(DEBUG) << "cross thread to call " << __FUNCTION__ << "is not supported";    \
            return result;                                                                       \
        }                                                                                        \
    } while (false)

#define DCHECK_SPECIAL_VALUE(jsValueRef)                                                     \
    do {                                                                                     \
        auto val = reinterpret_cast<JSTaggedValue *>(jsValueRef);                            \
        if (UNLIKELY(val->IsSpecial())) {                                                    \
            LOG_FULL(WARN) << "JSNApi special value:0x" << val->GetRawData() << " checked"; \
            return;                                                                          \
        }                                                                                    \
    } while (false)

#define DCHECK_SPECIAL_VALUE_WITH_RETURN(jsValueRef, retValue)                               \
    do {                                                                                     \
        auto val = reinterpret_cast<JSTaggedValue *>(jsValueRef);                            \
        if (UNLIKELY(val->IsSpecial())) {                                                    \
            LOG_FULL(WARN) << "JSNApi special value:0x" << val->GetRawData() << " checked"; \
            return (retValue);                                                               \
        }                                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_ARRAY_ALL(V) \
    V(Int8Array)           \
    V(Uint8Array)          \
    V(Uint8ClampedArray)   \
    V(Int16Array)          \
    V(Uint16Array)         \
    V(Int32Array)          \
    V(Uint32Array)         \
    V(Float32Array)        \
    V(Float64Array)        \
    V(BigInt64Array)       \
    V(BigUint64Array)

#define SENDABLE_TYPED_ARRAY_ALL(V) \
    V(SharedInt8Array)     \
    V(SharedUint8Array)    \
    V(SharedInt16Array)    \
    V(SharedUint16Array)   \
    V(SharedInt32Array)    \
    V(SharedUint32Array)   \
    V(SharedUint8ClampedArray)   \
    V(SharedFloat32Array)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXCEPTION_ERROR_ALL(V)         \
    V(Error, ERROR)                    \
    V(RangeError, RANGE_ERROR)         \
    V(SyntaxError, SYNTAX_ERROR)       \
    V(ReferenceError, REFERENCE_ERROR) \
    V(TypeError, TYPE_ERROR)           \
    V(AggregateError, AGGREGATE_ERROR) \
    V(EvalError, EVAL_ERROR)           \
    V(OOMError, OOM_ERROR)             \
    V(TerminationError, TERMINATION_ERROR)

namespace panda {
using NativeFinalize = void (*)(EcmaVM *);
class JSNApiHelper {
public:
    template<typename T>
    static inline Local<T> ToLocal(ecmascript::JSHandle<ecmascript::JSTaggedValue> from)
    {
        return Local<T>(from.GetAddress());
    }

    static inline ecmascript::JSTaggedValue ToJSTaggedValue(JSValueRef *from)
    {
        ASSERT(from != nullptr);
        return *reinterpret_cast<ecmascript::JSTaggedValue *>(from);
    }

    static inline ecmascript::JSMutableHandle<ecmascript::JSTaggedValue> ToJSMutableHandle(Local<JSValueRef> from)
    {
        ASSERT(!from.IsEmpty());
        return ecmascript::JSMutableHandle<ecmascript::JSTaggedValue>(reinterpret_cast<uintptr_t>(*from));
    }

    static inline ecmascript::JSHandle<ecmascript::JSTaggedValue> ToJSHandle(Local<JSValueRef> from)
    {
        ASSERT(!from.IsEmpty());
        return ecmascript::JSHandle<ecmascript::JSTaggedValue>(reinterpret_cast<uintptr_t>(*from));
    }

    static inline ecmascript::JSHandle<ecmascript::JSTaggedValue> ToJSHandle(JSValueRef *from)
    {
        ASSERT(from != nullptr);
        return ecmascript::JSHandle<ecmascript::JSTaggedValue>(reinterpret_cast<uintptr_t>(from));
    }
};

class NativeReferenceHelper {
public:
    NativeReferenceHelper(EcmaVM *vm, Global<ObjectRef> obj, NativeFinalize callback)
        :  vm_(vm), obj_(obj), callback_(callback) {}
    ~NativeReferenceHelper()
    {
        obj_.FreeGlobalHandleAddr();
    }
    static void FreeGlobalCallBack(void* ref)
    {
        auto that = reinterpret_cast<NativeReferenceHelper*>(ref);
        that->obj_.FreeGlobalHandleAddr();
    }

    static void NativeFinalizeCallBack(void* ref)
    {
        auto that = reinterpret_cast<NativeReferenceHelper*>(ref);
        if (that->callback_ != nullptr) {
            that->callback_(that->vm_);
        }
        that->callback_ = nullptr;
    }

    void SetWeakCallback()
    {
        obj_.SetWeakCallback(this, FreeGlobalCallBack, NativeFinalizeCallBack);
    }

private:
    EcmaVM *vm_;
    Global<ObjectRef> obj_;
    NativeFinalize callback_ = nullptr;
};

class Callback {
public:
    static ecmascript::JSTaggedValue RegisterCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo);
};
}  // namespace panda
#endif  // ECMASCRIPT_NAPI_JSNAPI_HELPER_H
