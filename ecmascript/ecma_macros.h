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

#ifndef ECMASCRIPT_ECMA_MACROS_H
#define ECMASCRIPT_ECMA_MACROS_H

#include "ecmascript/common.h"
#include "ecmascript/log_wrapper.h"

#if defined(ENABLE_BYTRACE)
    #include "hitrace_meter.h"
#endif

#if defined(__cplusplus)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)

#define OPTIONAL_LOG(vm, level) LOG_ECMA_IF(vm->IsOptionalLogEnabled(), level)
#define OPTIONAL_LOG_COMPILER(level) LOG_ECMA_IF(IsLogEnabled(), level)

#if !defined(ENABLE_BYTRACE)
    #define ECMA_BYTRACE_NAME(level, tag, name, customArgs)
    #define ECMA_BYTRACE_START_TRACE(level, tag, msg)
    #define ECMA_BYTRACE_FINISH_TRACE(level, tag)
    #define ECMA_BYTRACE_COUNT_TRACE(level, tag, name, count)
#else
    #define ECMA_BYTRACE_NAME(level, tag, name, customArgs) HITRACE_METER_NAME_EX(level, tag, name, customArgs)
    #define ECMA_BYTRACE_START_TRACE(level, tag, msg) StartTraceEx(level, tag, msg)
    #define ECMA_BYTRACE_FINISH_TRACE(level, tag) FinishTraceEx(level, tag)
    #define ECMA_BYTRACE_COUNT_TRACE(level, tag, name, count) CountTraceEx(level, tag, name, count)
#endif

#if defined(ENABLE_HITRACE)
    #define ENQUEUE_JOB_HITRACE(pendingJob, queueType) job::EnqueueJobScope hitraceScope(pendingJob, queueType)
    #define EXECUTE_JOB_HITRACE(pendingJob) job::ExecuteJobScope hitraceScope(pendingJob)
    #define ENQUEUE_JOB_TRACE(thread, pendingJob) job::EnqueueJobTrace enqueueJobTrace(thread, pendingJob)
    #define EXECUTE_JOB_TRACE(thread, pendingJob) job::ExecuteJobTrace executeJobTrace(thread, pendingJob)
#else
    #define ENQUEUE_JOB_HITRACE(pendingJob, queueType)
    #define EXECUTE_JOB_HITRACE(pendingJob)
    #define ENQUEUE_JOB_TRACE(thread, pendingJob)
    #define EXECUTE_JOB_TRACE(thread, pendingJob)
#endif

/* Note: We can't statically decide the element type is a primitive or heap object, especially for */
/*       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.          */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VALUE(addr, offset) Barriers::GetTaggedValue((addr), (offset))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VALUE_WITH_BARRIER(thread, addr, offset, value)                          \
    if ((value).IsHeapObject()) {                                                    \
        Barriers::SetObject<true>(thread, addr, offset, (value).GetRawData());       \
    } else {                                                                         \
        Barriers::SetPrimitive<JSTaggedType>(addr, offset, (value).GetRawData());    \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VALUE_PRIMITIVE(addr, offset, value) \
    Barriers::SetPrimitive<JSTaggedType>(this, offset, (value).GetRawData())

#if !defined(NDEBUG)
#define FIELD_ACCESS_CHECK(needCheck, name, check)                                \
    if constexpr (needCheck) {                                                    \
        if (g_isEnableCMCGC && check()) {                                         \
            LOG_FULL(FATAL) << #name" field can't be used under check: "#check;   \
            UNREACHABLE();                                                        \
        };                                                                        \
    }
#else
    #define FIELD_ACCESS_CHECK(needCheck, name, check)
#endif

#define DUMMY_FUNC() false

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_WITH_DCHECK_BASE(name, offset, endOffset, needCheck, check)                                 \
    static constexpr size_t endOffset = (offset) + JSTaggedValue::TaggedTypeSize();                           \
    JSTaggedValue Get##name() const                                                                           \
    {                                                                                                         \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                           \
        /* Note: We can't statically decide the element type is a primitive or heap object, especially for */ \
        /*       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.          */ \
        return JSTaggedValue(Barriers::GetTaggedValue(this, offset));                                         \
    }                                                                                                         \
    template<BarrierMode mode = WRITE_BARRIER, typename T>                                                    \
    void Set##name(const JSThread *thread, JSHandle<T> value)                                                 \
    {                                                                                                         \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                           \
        if constexpr (mode == WRITE_BARRIER) {                                                                \
            if (value.GetTaggedValue().IsHeapObject()) {                                                      \
                Barriers::SetObject<true>(thread, this, offset, value.GetTaggedValue().GetRawData());         \
            } else {                                                                                          \
                Barriers::SetPrimitive<JSTaggedType>(this, offset, value.GetTaggedValue().GetRawData());      \
            }                                                                                                 \
        } else {                                                                                              \
            static_assert(mode == SKIP_BARRIER);                                                              \
            Barriers::SetPrimitive<JSTaggedType>(this, offset, value.GetTaggedValue().GetRawData());          \
        }                                                                                                     \
    }                                                                                                         \
    template<BarrierMode mode = WRITE_BARRIER>                                                                \
    void Set##name(const JSThread *thread, JSTaggedValue value)                                               \
    {                                                                                                         \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                           \
        if constexpr (mode == WRITE_BARRIER) {                                                                \
            if (value.IsHeapObject()) {                                                                       \
                Barriers::SetObject<true>(thread, this, offset, value.GetRawData());                          \
            } else {                                                                                          \
                Barriers::SetPrimitive<JSTaggedType>(this, offset, value.GetRawData());                       \
            }                                                                                                 \
        } else {                                                                                              \
            static_assert(mode == SKIP_BARRIER);                                                              \
            Barriers::SetPrimitive<JSTaggedType>(this, offset, value.GetRawData());                           \
        }                                                                                                     \
    }

#define ACCESSORS(name, offset, endOffset)                                                                    \
    ACCESSORS_WITH_DCHECK_BASE(name, offset, endOffset, false, DUMMY_FUNC)

#define ACCESSORS_DCHECK(name, offset, endOffset, check)                                                      \
    ACCESSORS_WITH_DCHECK_BASE(name, offset, endOffset, true, check)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(name, type, offset, endOffset)                                \
    static constexpr size_t endOffset = (offset) + sizeof(type);                                             \
    inline type Get##name() const                                                                            \
    {                                                                                                        \
        return reinterpret_cast<volatile std::atomic<type> *>(ToUintPtr(this) + offset)                      \
        ->load(std::memory_order_acquire);                                                                   \
    }                                                                                                        \
    inline void Set##name(type value)                                                                        \
    {                                                                                                        \
        reinterpret_cast<volatile std::atomic<type> *>(ToUintPtr(this) + offset)                             \
        ->store(value, std::memory_order_release);                                                           \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_SYNCHRONIZED_WITH_DCHECK_BASE(name, offset, endOffset, needCheck, check)                            \
    static constexpr size_t endOffset = (offset) + JSTaggedValue::TaggedTypeSize();                                   \
    JSTaggedValue Get##name() const                                                                                   \
    {                                                                                                                 \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                                   \
        /* Note: We can't statically decide the element type is a primitive or heap object, especially for */         \
        /*       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.          */         \
        /*       Synchronized means it will restrain the store and load in atomic.                         */         \
        return JSTaggedValue(Barriers::GetTaggedValueAtomic(this, offset));                                           \
    }                                                                                                                 \
    template<typename T>                                                                                              \
    void Set##name(const JSThread *thread, JSHandle<T> value)                                                         \
    {                                                                                                                 \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                                   \
        bool isPrimitive = !value.GetTaggedValue().IsHeapObject();                                                    \
        Barriers::SynchronizedSetObject(thread, this, offset, value.GetTaggedValue().GetRawData(), isPrimitive);      \
    }                                                                                                                 \
    void Set##name(const JSThread *thread, JSTaggedValue value)                                                       \
    {                                                                                                                 \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                                   \
        bool isPrimitive = !value.IsHeapObject();                                                                     \
        Barriers::SynchronizedSetObject(thread, this, offset, value.GetRawData(), isPrimitive);                       \
    }

#define ACCESSORS_SYNCHRONIZED(name, offset, endOffset)                                                               \
    ACCESSORS_SYNCHRONIZED_WITH_DCHECK_BASE(name, offset, endOffset, false, DUMMY_FUNC)

#define ACCESSORS_SYNCHRONIZED_DCHECK(name, offset, endOffset, check)                                                 \
    ACCESSORS_SYNCHRONIZED_WITH_DCHECK_BASE(name, offset, endOffset, true, check)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFINE_ALIGN_SIZE(offset) \
    static constexpr size_t SIZE = ((offset) + sizeof(JSTaggedType) - 1U) & (~(sizeof(JSTaggedType) - 1U))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_FIXED_SIZE_FIELD_DCHECK_BASE(name, type, sizeType, offset, endOffset, needCheck, check)              \
    static_assert(sizeof(type) <= sizeof(sizeType));                                                                   \
    static constexpr size_t endOffset = (offset) + sizeof(sizeType);                                                   \
    inline void Set##name(type value)                                                                                  \
    {                                                                                                                  \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                                    \
        Barriers::SetPrimitive<type>(this, offset, value);                                                             \
    }                                                                                                                  \
    inline type Get##name() const                                                                                      \
    {                                                                                                                  \
        FIELD_ACCESS_CHECK(needCheck, name, check);                                                                    \
        return Barriers::GetValue<type>(this, offset);                                                                 \
    }

#define ACCESSORS_FIXED_SIZE_FIELD(name, type, sizeType, offset, endOffset)                                            \
    ACCESSORS_FIXED_SIZE_FIELD_DCHECK_BASE(name, type, sizeType, offset, endOffset, false, DUMMY_FUNC)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_NATIVE_FIELD(name, type, offset, endOffset) \
    ACCESSORS_FIXED_SIZE_FIELD(name, type *, type *, offset, endOffset)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_PRIMITIVE_FIELD(name, type, offset, endOffset) \
    ACCESSORS_FIXED_SIZE_FIELD(name, type, type, offset, endOffset)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_PRIMITIVE_FIELD_DCHECK(name, type, offset, endOffset, check)                                         \
    ACCESSORS_FIXED_SIZE_FIELD_DCHECK_BASE(name, type, type, offset, endOffset, true, check)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_BIT_FIELD(name, offset, endOffset)                        \
    ACCESSORS_FIXED_SIZE_FIELD(name, uint32_t, uint32_t, offset, endOffset) \
    inline void Clear##name()                                               \
    {                                                                       \
        Set##name(0UL);                                                     \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_GET_BIT_FIELD(bitFieldName, name, type)                    \
    inline type Get##name() const                                      \
    {                                                                  \
        return name##Bits::Decode(Get##bitFieldName());                \
    }                                                                  \
    inline void Set##name(type t)                                      \
    {                                                                  \
        Set##bitFieldName(name##Bits::Update(Get##bitFieldName(), t)); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_PRIMITIVE_FIELD_ATOMIC(name, type, offset)                                                     \
    inline void AtomicSet##name(type value)                                                                      \
    {                                                                                                            \
        volatile auto *atomicField = reinterpret_cast<volatile std::atomic<type> *>(ToUintPtr(this) + (offset)); \
        atomicField->store(value, std::memory_order_release);                                                    \
    }                                                                                                            \
    inline type AtomicGet##name() const                                                                          \
    {                                                                                                            \
        volatile auto *atomicField = reinterpret_cast<volatile std::atomic<type> *>(ToUintPtr(this) + (offset)); \
        return atomicField->load(std::memory_order_acquire);                                                     \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_PRIMITIVE_FIELD_HAS_ATOMIC_INTERFACE(name, type, offset, endOffset) \
    ACCESSORS_PRIMITIVE_FIELD(name, type, offset, endOffset)                          \
    ACCESSORS_PRIMITIVE_FIELD_ATOMIC(name, type, offset)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FIRST_BIT_FIELD(bitFieldName, name, type, bits) \
    using name##Bits = BitField<type, 0, bits>;         \
    SET_GET_BIT_FIELD(bitFieldName, name, type)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NEXT_BIT_FIELD(bitFieldName, name, type, bits, lastName) \
    using name##Bits = lastName##Bits::NextField<type, bits>;    \
    SET_GET_BIT_FIELD(bitFieldName, name, type)

#if !defined(NDEBUG)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DASSERT(cond) assert(cond)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DASSERT_PRINT(cond, message)                     \
    if (auto cond_val = (cond); UNLIKELY(!(cond_val))) { \
        std::cerr << (message) << std::endl;             \
        ASSERT(#cond &&cond_val);                        \
    }
#else                                                      // NDEBUG
#define DASSERT(cond) static_cast<void>(0)                 // NOLINT(cppcoreguidelines-macro-usage)
#define DASSERT_PRINT(cond, message) static_cast<void>(0)  // NOLINT(cppcoreguidelines-macro-usage)
#endif                                                     // !NDEBUG

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RASSERT(cond) assert(cond)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RASSERT_PRINT(cond, message)                   \
    if (auto cond_val = cond; UNLIKELY(!(cond_val))) { \
        std::cerr << message << std::endl;             \
        RASSERT(#cond &&cond_val);                     \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_IF_ABRUPT_COMPLETION(thread)    \
    do {                                       \
        if ((thread)->HasPendingException()) { \
            return;                            \
        }                                      \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, value) \
    do {                                                 \
        if ((thread)->HasPendingException()) {           \
            return (value);                              \
        }                                                \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, value, flagsStr) \
    do {                                                                            \
        if ((thread)->HasPendingException()) {                                      \
            delete[] flagsStr;                                                      \
            return (value);                                                         \
        }                                                                           \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread) \
    do {                                              \
        if ((thread)->HasPendingException()) {        \
            return JSTaggedValue::Exception();        \
        }                                             \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, value)       \
    do {                                                        \
        if ((thread)->HasPendingException()) {                  \
            ArrayJoinStack::Pop(thread, value);                 \
            return JSTaggedValue::Exception();                  \
        }                                                       \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_HANDLE_IF_ABRUPT_COMPLETION(type, thread)               \
    do {                                                               \
        if ((thread)->HasPendingException()) {                         \
            return JSHandle<type>(thread, JSTaggedValue::Exception()); \
        }                                                              \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_NO_ABRUPT_COMPLETION(thread) ASSERT(!(thread)->HasPendingException());

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_DATE_VALUE(name, code, isLocal)                                                       \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                          \
    {                                                                                             \
        ASSERT(argv);                                                                             \
        JSThread *thread = argv->GetThread();                                                     \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                     \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                              \
        if (!msg->IsDate()) {                                                                     \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception()); \
        }                                                                                         \
        JSHandle<JSDate> jsDate(msg);                                                             \
        JSTaggedValue result = jsDate->SetDateValue(argv, code, isLocal);                         \
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);                                            \
        jsDate->SetTimeValue(thread, result);                                                     \
        return result;                                                                            \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DATE_TO_STRING(name)                                                                      \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                          \
    {                                                                                             \
        ASSERT(argv);                                                                             \
        JSThread *thread = argv->GetThread();                                                     \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                     \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                              \
        if (!msg->IsDate()) {                                                                     \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception()); \
        }                                                                                         \
        if (std::isnan(JSDate::Cast(msg->GetTaggedObject())->GetTimeValue().GetDouble())) {       \
            THROW_RANGE_ERROR_AND_RETURN(thread, "range error", JSTaggedValue::Exception());      \
        }                                                                                         \
        return JSDate::Cast(msg->GetTaggedObject())->name(thread);                                \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DATE_STRING(name)                                                                                          \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                                           \
    {                                                                                                              \
        ASSERT(argv);                                                                                              \
        JSThread *thread = argv->GetThread();                                                                      \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                                      \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                                               \
        if (!msg->IsDate()) {                                                                                      \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());                  \
        }                                                                                                          \
        if (std::isnan(JSDate::Cast(msg->GetTaggedObject())->GetTimeValue().GetDouble())) {                        \
            return thread->GetEcmaVM()->GetFactory()->NewFromASCII("Invalid Date").GetTaggedValue();               \
        }                                                                                                          \
        return JSDate::Cast(msg->GetTaggedObject())->name(thread);                                                 \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_DATE_VALUE(name, code, isLocal)                                                                 \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                                    \
    {                                                                                                       \
        ASSERT(argv);                                                                                       \
        JSThread *thread = argv->GetThread();                                                               \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                               \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                                        \
        if (!msg->IsDate()) {                                                                               \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());           \
        }                                                                                                   \
        JSHandle<JSDate> jsDate(msg);                                                                       \
        double result = jsDate->GetDateValue(thread, jsDate->GetTimeValue().GetDouble(), code, isLocal);    \
        return GetTaggedDouble(result);                                                                     \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_AND_RETURN(thread, error)       \
    do {                                                \
        if (!(thread)->HasPendingException()) {         \
            (thread)->SetException(error);              \
        }                                               \
        return;                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_ERROR(thread, type, message)                                                              \
    do {                                                                                                \
        if ((thread)->HasPendingException()) {                                                          \
            return;                                                                                     \
        }                                                                                               \
        ObjectFactory *_factory = (thread)->GetEcmaVM()->GetFactory();                                  \
        JSHandle<JSObject> _error = _factory->GetJSError(type, message, StackCheck::NO);                \
        (thread)->SetException(_error.GetTaggedValue());                                                \
        return;                                                                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_UNCATCHABLE_ERROR(thread, type, message)                                                  \
    do {                                                                                                \
        EcmaVM *_ecmaVm = (thread)->GetEcmaVM();                                                        \
        ObjectFactory *_factory = _ecmaVm->GetFactory();                                                \
        JSHandle<JSObject> _error = _factory->GetJSError(type, message, StackCheck::NO);                \
        (thread)->SetException(_error.GetTaggedValue());                                                \
        _ecmaVm->HandleUncatchableError();                                                              \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, value) \
    do {                                                       \
        if (!(thread)->HasPendingException()) {                \
            (thread)->SetException(error);                     \
        }                                                      \
        return (value);                                        \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, errorType, type, message)                                 \
    do {                                                                                                    \
        if ((thread)->HasPendingException()) {                                                              \
            return JSHandle<type>(thread, JSTaggedValue::Exception());                                      \
        }                                                                                                   \
        ObjectFactory *_factory = (thread)->GetEcmaVM()->GetFactory();                                      \
        JSHandle<JSObject> _error = _factory->GetJSError(errorType, message, StackCheck::NO);               \
        (thread)->SetException(_error.GetTaggedValue());                                                    \
        return JSHandle<type>(thread, JSTaggedValue::Exception());                                          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, errorType, message, value)                        \
    do {                                                                                                    \
        if ((thread)->HasPendingException()) {                                                              \
            return (value);                                                                                 \
        }                                                                                                   \
        ObjectFactory *_factory = (thread)->GetEcmaVM()->GetFactory();                                      \
        JSHandle<JSObject> _error = _factory->GetJSError(errorType, message, ecmascript::StackCheck::NO);   \
        (thread)->SetException(_error.GetTaggedValue());                                                    \
        return (value);                                                                                     \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_MODULE_NOT_FOUND_ERROR_WITH_RETURN_VALUE(thread, requestStr, currentRecord, value)            \
    do {                                                                                                    \
        if ((thread)->HasPendingException()) {                                                              \
            return (value);                                                                                 \
        }                                                                                                   \
        ObjectFactory *_factory = (thread)->GetEcmaVM()->GetFactory();                                      \
        CString normalizeStr = ModulePathHelper::ReformatPath(currentRecord);                               \
        CString msg =  "Cannot find module '" + requestStr + "' imported from '" + normalizeStr + "'.";     \
        JSHandle<JSObject> _error = _factory->GetJSError(ErrorType::REFERENCE_ERROR,                        \
                                                         msg.c_str(), StackCheck::NO);                      \
        (thread)->SetException(_error.GetTaggedValue());                                                    \
        return (value);                                                                                     \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_TYPE_ERROR_AND_RETURN(thread, message, value)                                      \
    THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::TYPE_ERROR, message, value)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_RANGE_ERROR_AND_RETURN(thread, message, value)                                     \
    THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::RANGE_ERROR, message, value)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_URI_ERROR_AND_RETURN(thread, message, value)                                       \
    THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::URI_ERROR, message, value)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_SYNTAX_ERROR_AND_RETURN(thread, message, value)                                    \
    THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::SYNTAX_ERROR, message, value)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_REFERENCE_ERROR_AND_RETURN(thread, message, value)                                 \
    THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::REFERENCE_ERROR, message, value)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_TYPE_ERROR(thread, message)               \
    THROW_ERROR(thread, ErrorType::TYPE_ERROR, message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_OOM_ERROR(thread, message)                \
    THROW_UNCATCHABLE_ERROR(thread, ErrorType::OOM_ERROR, message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_TERMINATION_ERROR(thread, message)               \
    THROW_ERROR(thread, ErrorType::TERMINATION_ERROR, message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_STACK_BEFORE_THROW_IF_ASM(thread)                                                   \
    do {                                                                                           \
        if ((thread)->IsAsmInterpreter()) {                                                        \
            FrameIterator it(const_cast<JSTaggedType *>((thread)->GetCurrentSPFrame()), (thread)); \
            it.Advance();                                                                          \
            (thread)->SetCurrentSPFrame(it.GetSp());                                               \
        }                                                                                          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_REJECT_PROMISE_IF_ABRUPT(thread, value, capability)                                 \
    do {                                                                                           \
        const GlobalEnvConstants *globalConst = (thread)->GlobalConstants();                       \
        if ((value).GetTaggedValue().IsCompletionRecord()) {                                       \
            JSHandle<CompletionRecord> record = JSHandle<CompletionRecord>::Cast(value);           \
            if (record->IsThrow()) {                                                               \
                JSHandle<JSTaggedValue> reject(thread, (capability)->GetReject());                 \
                JSHandle<JSTaggedValue> undefine = globalConst->GetHandledUndefined();             \
                EcmaRuntimeCallInfo *info =                                                        \
                    EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefine, undefine, 1);    \
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());             \
                info->SetCallArg(record->GetValue());                                              \
                JSTaggedValue res = JSFunction::Call(info);                                        \
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, res);                                    \
                return (capability)->GetPromise();                                                 \
            }                                                                                      \
        }                                                                                          \
        if ((thread)->HasPendingException()) {                                                     \
            (thread)->ClearException();                                                            \
            JSHandle<JSTaggedValue> reject(thread, (capability)->GetReject());                     \
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();                \
            EcmaRuntimeCallInfo *info =                                                            \
                EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);      \
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());                 \
            info->SetCallArg(value.GetTaggedValue());                                              \
            JSTaggedValue res = JSFunction::Call(info);                                            \
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, res);                                        \
            return (capability)->GetPromise();                                                     \
        }                                                                                          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_COMPLETION_IF_ABRUPT(thread, value)                                \
    do {                                                                          \
        if ((thread)->HasPendingException()) {                                    \
            JSHandle<CompletionRecord> completionRecord =                         \
                factory->NewCompletionRecord(CompletionRecordType::THROW, value); \
            return (completionRecord);                                            \
        }                                                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_COMPLETION_VALUE_IF_ABRUPT(thread, value)                          \
    do {                                                                          \
        if ((thread)->HasPendingException()) {                                    \
            JSHandle<CompletionRecord> completionRecord =                         \
                factory->NewCompletionRecord(CompletionRecordType::THROW, value); \
            return (completionRecord).GetTaggedValue();                           \
        }                                                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_DUMP()                                  \
    void Dump(std::ostream &os) const DUMP_API_ATTR; \
    void Dump() const DUMP_API_ATTR                  \
    {                                                \
        Dump(std::cout);                             \
    }                                                \
    void DumpForSnapshot(std::vector<Reference> &vec) const;

#endif  // defined(__cplusplus)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_CAST(TYPE)                           \
    static TYPE *Cast(TaggedObject *object)       \
    {                                             \
        ASSERT(JSTaggedValue(object).Is##TYPE()); \
        return reinterpret_cast<TYPE *>(object);  \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_ARRAY(BEGIN_OFFSET, REF_LENGTH, LENGTH)                                                \
    template <VisitType visitType, class DerivedVisitor>                                                  \
    void VisitRangeSlot(BaseObjectVisitor<DerivedVisitor> &visitor)                                       \
    {                                                                                                     \
        ArrayBodyIterator<visitType, (BEGIN_OFFSET)>::IterateBody(this, visitor, (REF_LENGTH), (LENGTH)); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_OBJECT(BEGIN_OFFSET, END_OFFSET)                                                       \
    template <VisitType visitType, class DerivedVisitor>                                                  \
    void VisitRangeSlot(BaseObjectVisitor<DerivedVisitor> &visitor)                                       \
    {                                                                                                     \
        ObjectBodyIterator<visitType, (BEGIN_OFFSET), (END_OFFSET), SIZE>::IterateRefBody(this, visitor); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_PRIMITIVE_OBJECT()                                               \
    template <VisitType visitType, class DerivedVisitor>                            \
    void VisitRangeSlot(BaseObjectVisitor<DerivedVisitor> &visitor)                 \
    {                                                                               \
        PrimitiveObjectBodyIterator<visitType, SIZE>::IterateBody(this, visitor);   \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_NATIVE_FIELD(BEGIN_OFFSET, END_OFFSET)                                                    \
    template <VisitType visitType, class DerivedVisitor>                                                     \
    void VisitRangeSlot(BaseObjectVisitor<DerivedVisitor> &visitor)                                          \
    {                                                                                                        \
        ObjectBodyIterator<visitType, (BEGIN_OFFSET), (END_OFFSET), SIZE>::IterateNativeBody(this, visitor); \
    }                                                                                                        \

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_OBJECT_FOR_JS_OBJECT(PARENTCLASS, BEGIN_OFFSET, END_OFFSET)                         \
    template <VisitType visitType, class DerivedVisitor>                                               \
    void VisitRangeSlot(BaseObjectVisitor<DerivedVisitor> &visitor)                                    \
    {                                                                                                  \
        VisitObjects<visitType>(visitor);                                                              \
        JSObjectBodyIterator<visitType, SIZE>::IterateBody(this, visitor);                             \
    }                                                                                                  \
    template <VisitType visitType, class DerivedVisitor>                                               \
    void VisitObjects(BaseObjectVisitor<DerivedVisitor> &visitor)                                      \
    {                                                                                                  \
        PARENTCLASS::VisitObjects<visitType>(visitor);                                                 \
        static constexpr size_t PARENT_SIZE = PARENTCLASS::SIZE;                                       \
        ObjectBodyIterator<visitType, (BEGIN_OFFSET),                                                  \
            (END_OFFSET), SIZE, PARENT_SIZE>::IterateDerivedRefBody(this, visitor);                    \
    }

#if ECMASCRIPT_ENABLE_CAST_CHECK
    #define CAST_CHECK(CAST_TYPE, CHECK_METHOD)                                                 \
        static inline CAST_TYPE *Cast(TaggedObject *object)                                     \
        {                                                                                       \
            if (!JSTaggedValue(object).CHECK_METHOD()) {                                        \
                LOG_ECMA(ERROR) << "cast failed, dump: " << JSTaggedValue(object);              \
                std::abort();                                                                   \
            }                                                                                   \
            return static_cast<CAST_TYPE *>(object);                                            \
        }                                                                                       \
        static inline const CAST_TYPE *ConstCast(const TaggedObject *object)                    \
        {                                                                                       \
            if (!JSTaggedValue(object).CHECK_METHOD()) {                                        \
                LOG_ECMA(ERROR) << "cast failed, dump: " << JSTaggedValue(object);              \
                std::abort();                                                                   \
            }                                                                                   \
            return static_cast<const CAST_TYPE *>(object);                                      \
        }                                                                                       \
        static inline CAST_TYPE *Cast(JSTaggedValue value)                                      \
        {                                                                                       \
            if (!value.CHECK_METHOD()) {                                                        \
                LOG_ECMA(ERROR) << "cast failed, dump: " << value;                              \
                std::abort();                                                                   \
            }                                                                                   \
            return static_cast<CAST_TYPE *>(value.GetTaggedObject());                           \
        }
#else
    #define CAST_CHECK(CAST_TYPE, CHECK_METHOD)                                                   \
        static inline CAST_TYPE *Cast(TaggedObject *object)                                       \
        {                                                                                         \
            ASSERT(JSTaggedValue(object).CHECK_METHOD());                                         \
            return static_cast<CAST_TYPE *>(object);                                              \
        }                                                                                         \
        static const inline CAST_TYPE *ConstCast(const TaggedObject *object)                      \
        {                                                                                         \
            ASSERT(JSTaggedValue(object).CHECK_METHOD());                                         \
            return static_cast<const CAST_TYPE *>(object);                                        \
        }                                                                                         \
        static inline CAST_TYPE *Cast(JSTaggedValue value)                                        \
        {                                                                                         \
            ASSERT(value.CHECK_METHOD());                                                         \
            return static_cast<CAST_TYPE *>(value.GetTaggedObject());                             \
        }
#endif

#define CAST_NO_CHECK(CAST_TYPE)                                                              \
    static inline CAST_TYPE *Cast(TaggedObject *object)                                       \
    {                                                                                         \
        return static_cast<CAST_TYPE *>(object);                                              \
    }                                                                                         \
    static const inline CAST_TYPE *ConstCast(const TaggedObject *object)                      \
    {                                                                                         \
        return static_cast<const CAST_TYPE *>(object);                                        \
    }

#define CHECK_OBJECT_SIZE(size)                                                        \
    if ((size) == 0) {                                                                 \
        LOG_FULL(FATAL) << __func__ << ":" << __LINE__ << " objectSize is " << (size); \
    }

#define CHECK_REGION_END(begin, end)                                                                  \
    if ((begin) > (end)) {                                                                            \
        LOG_FULL(FATAL) << __func__ << ":" << __LINE__ << " begin: " << (begin) << " end: " << (end); \
    }

#define CHECK_JS_THREAD(vm)                                                           \
    if (!(vm)->GetJSThread()->IsCrossThreadExecutionEnable()) {                       \
        ASSERT((vm)->GetJSThread()->GetThreadId() == JSThread::GetCurrentThreadId()); \
    }
#define CHECK_DAEMON_THREAD()                                                               \
    ASSERT(os::thread::GetCurrentThreadId() == DaemonThread::GetInstance()->GetThreadId())

#if !defined(NDEBUG)
#define STACK_ASSERT_SCOPE(thread) [[maybe_unused]] StackAssertScope stackAssertScope = StackAssertScope(thread)
#else
#define STACK_ASSERT_SCOPE(thread) static_cast<void>(0)
#endif

#if !defined(NDEBUG)
#define BUILTINS_ENTRY_DEBUG_LOG() LOG_BUILTINS(DEBUG) << "Builtins C++ " << __func__
#else
#define BUILTINS_ENTRY_DEBUG_LOG() static_cast<void>(0)
#endif

#if defined(ARK_NOT_SUPPORT_INTL_GLOBAL)
#define ARK_SUPPORT_INTL_RETURN_STR(msg) "Please use import intl lib "#msg
#define ARK_SUPPORT_INTL_RETURN(thread, message)                            \
    THROW_TYPE_ERROR(thread, ARK_SUPPORT_INTL_RETURN_STR(message))
#define ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, message)                                                        \
    THROW_TYPE_ERROR_AND_RETURN(thread,                                                                         \
        ARK_SUPPORT_INTL_RETURN_STR(message), JSTaggedValue::Exception())
#else
#define ARK_SUPPORT_INTL_RETURN(thread, message) static_cast<void>(0)
#define ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, message) static_cast<void>(0)
#endif

#define STACK_LIMIT_CHECK(thread, retVal)     \
    do {                                      \
        if ((thread)->DoStackLimitCheck()) {  \
            return (retVal);                  \
        }                                     \
    } while (0)

#define STACK_LIMIT_CHECK_VOID(thread)        \
    do {                                      \
        if ((thread)->DoStackLimitCheck()) {  \
            return;                           \
        }                                     \
    } while (0)

#define CHECK_SLOTID_BREAK(slotId)        \
    if ((slotId) == 0xff) {               \
        break;                            \
    }

#define CHECK_INPUT_NULLPTR(ptr, msg)     \
    if ((ptr) == nullptr) {               \
        LOG_FULL(FATAL) << (msg);         \
        UNREACHABLE();                    \
    }

#endif  // ECMASCRIPT_ECMA_MACROS_H
