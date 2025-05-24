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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_BUFFER_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_BUFFER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::containers {
/**
 * High performance container interface in jsapi.
 * */
#define CONTAINER_BUFFER_CHECK(name)                                                                       \
    JSHandle<JSTaggedValue> self = GetThis(argv);                                                          \
    if (!self->IsJSAPIBuffer()) {                                                                          \
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIBuffer()) {             \
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());            \
        } else {                                                                                           \
            JSTaggedValue error =                                                                          \
                ContainerError::BusinessError(thread, BIND_ERROR, "The " #name " method cannot be bound"); \
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                   \
        }                                                                                                  \
    }

#define CONTAINER_BUFFER_ACCESSORS(name)                                                                             \
    JSTaggedValue ContainersBuffer::Write##name##BE(EcmaRuntimeCallInfo *argv)                                       \
    {                                                                                                                \
        ASSERT(argv != nullptr);                                                                                     \
        JSThread *thread = argv->GetThread();                                                                        \
        BUILTINS_API_TRACE(thread, Buffer, Write##name##BE);                                                         \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                                        \
        CONTAINER_BUFFER_CHECK(Write##name##BE)                                                                      \
        JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);                                                         \
        JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);                                                        \
        CHECK_NULL_OR_UNDEFINED(value); \
        uint32_t offsetIndex = 0;                                                                                    \
        if (offset->IsNumber()) {                                                                                    \
            if (IsNegetiveNumber(offset)) {                                                                          \
                std::ostringstream oss;                                                                              \
                oss << "The value of \"offset\" is out of range. It must be >= 0 and <= "                            \
                       "4294967296. Received value is: "                                                             \
                    << offset->GetNumber();                                                                          \
                JSTaggedValue error =                                                                                \
                    ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());                \
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                         \
            }                                                                                                        \
            if (UNLIKELY(offset->IsDouble())) {                                                                      \
                offsetIndex = static_cast<uint32_t>(offset->GetDouble());                                            \
            } else {                                                                                                 \
                offsetIndex = static_cast<uint32_t>(offset->GetInt());                                               \
            }                                                                                                        \
        }                                                                                                            \
        if (value->IsUndefined() || value->IsNull()) {                                                               \
            return JSTaggedValue(offsetIndex);                                                                       \
        }                                                                                                            \
        JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);                                    \
        auto ret = JSAPIFastBuffer::Write##name(thread, buffer, value, offsetIndex, false);                          \
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);                                                               \
        return ret;                                                                                                  \
    }                                                                                                                \
    JSTaggedValue ContainersBuffer::Write##name##LE(EcmaRuntimeCallInfo *argv)                                       \
    {                                                                                                                \
        ASSERT(argv != nullptr);                                                                                     \
        JSThread *thread = argv->GetThread();                                                                        \
        BUILTINS_API_TRACE(thread, Buffer, Write##name##LE);                                                         \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                                        \
        CONTAINER_BUFFER_CHECK(Write##name##LE)                                                                      \
        JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);                                                         \
        JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);                                                        \
        CHECK_NULL_OR_UNDEFINED(value); \
        uint32_t offsetIndex = 0;                                                                                    \
        if (offset->IsNumber()) {                                                                                    \
            if (IsNegetiveNumber(offset)) {                                                                          \
                std::ostringstream oss;                                                                              \
                oss << "The value of \"offset\" is out of range. It must be >= 0 and <= "                            \
                       "4294967296. Received value is: "                                                             \
                    << offset->GetNumber();                                                                          \
                JSTaggedValue error =                                                                                \
                    ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());                \
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                         \
            }                                                                                                        \
            if (UNLIKELY(offset->IsDouble())) {                                                                      \
                offsetIndex = static_cast<uint32_t>(offset->GetDouble());                                            \
            } else {                                                                                                 \
                offsetIndex = static_cast<uint32_t>(offset->GetInt());                                               \
            }                                                                                                        \
        }                                                                                                            \
        if (value->IsUndefined() || value->IsNull()) {                                                               \
            return JSTaggedValue(offsetIndex);                                                                       \
        }                                                                                                            \
        JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);                                    \
        auto ret = JSAPIFastBuffer::Write##name(thread, buffer, value, offsetIndex, true);                           \
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);                                                               \
        return ret;                                                                                                  \
    }                                                                                                                \
    JSTaggedValue ContainersBuffer::Read##name##BE(EcmaRuntimeCallInfo *argv)                                        \
    {                                                                                                                \
        ASSERT(argv != nullptr);                                                                                     \
        JSThread *thread = argv->GetThread();                                                                        \
        BUILTINS_API_TRACE(thread, Buffer, Read##name##BE);                                                          \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                                        \
        CONTAINER_BUFFER_CHECK(Read##name##BE)                                                                       \
        JSHandle<JSTaggedValue> offset = GetCallArg(argv, 0);                                                        \
        uint32_t offsetIndex = 0;                                                                                    \
        if (offset->IsNumber()) {                                                                                    \
            if (IsNegetiveNumber(offset)) {                                                                          \
                std::ostringstream oss;                                                                              \
                oss << "The value of \"offset\" is out of range. It must be >= 0 and <= "                            \
                       "4294967296. Received value is: "                                                             \
                    << offset->GetNumber();                                                                          \
                JSTaggedValue error =                                                                                \
                    ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());                \
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                         \
            }                                                                                                        \
            if (UNLIKELY(offset->IsDouble())) {                                                                      \
                offsetIndex = static_cast<uint32_t>(offset->GetDouble());                                            \
            } else {                                                                                                 \
                offsetIndex = static_cast<uint32_t>(offset->GetInt());                                               \
            }                                                                                                        \
        }                                                                                                            \
        JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);                                    \
        auto ret = JSAPIFastBuffer::Read##name(thread, buffer, offsetIndex, false);                                  \
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);                                                               \
        return ret;                                                                                                  \
    }                                                                                                                \
    JSTaggedValue ContainersBuffer::Read##name##LE(EcmaRuntimeCallInfo *argv)                                        \
    {                                                                                                                \
        ASSERT(argv != nullptr);                                                                                     \
        JSThread *thread = argv->GetThread();                                                                        \
        BUILTINS_API_TRACE(thread, Buffer, Read##name##LE);                                                          \
        [[maybe_unused]] EcmaHandleScope handleScope(thread);                                                        \
        CONTAINER_BUFFER_CHECK(Read##name##LE)                                                                       \
        JSHandle<JSTaggedValue> offset = GetCallArg(argv, 0);                                                        \
        uint32_t offsetIndex = 0;                                                                                    \
        if (offset->IsNumber()) {                                                                                    \
            if (IsNegetiveNumber(offset)) {                                                                          \
                std::ostringstream oss;                                                                              \
                oss << "The value of \"offset\" is out of range. It must be >= 0 and <= "                            \
                       "4294967296. Received value is: "                                                             \
                    << offset->GetNumber();                                                                          \
                JSTaggedValue error =                                                                                \
                    ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());                \
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                         \
            }                                                                                                        \
            if (UNLIKELY(offset->IsDouble())) {                                                                      \
                offsetIndex = static_cast<uint32_t>(offset->GetDouble());                                            \
            } else {                                                                                                 \
                offsetIndex = static_cast<uint32_t>(offset->GetInt());                                               \
            }                                                                                                        \
        }                                                                                                            \
        JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);                                    \
        auto ret = JSAPIFastBuffer::Read##name(thread, buffer, offsetIndex, true);                                   \
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);                                                               \
        return ret;                                                                                                  \
    }

#define CONTAINER_FASTBUFFER_PROTOTYPE_FUNCTIONS(V)     \
    V("toString", ToString, 3, INVALID)                 \
    V("write", Write, 3, INVALID)                       \
    V("writeUIntBE", WriteUIntBE, 3, INVALID)           \
    V("writeUIntLE", WriteUIntLE, 3, INVALID)           \
    V("writeIntBE", WriteIntBE, 3, INVALID)             \
    V("writeIntLE", WriteIntLE, 3, INVALID)             \
    V("writeUInt8", WriteUInt8, 2, INVALID)             \
    V("writeUInt16BE", WriteUInt16BE, 2, INVALID)       \
    V("writeUInt16LE", WriteUInt16LE, 2, INVALID)       \
    V("writeUInt32BE", WriteUInt32BE, 2, INVALID)       \
    V("writeUInt32LE", WriteUInt32LE, 2, INVALID)       \
    V("writeInt8", WriteInt8, 2, INVALID)               \
    V("writeInt16BE", WriteInt16BE, 2, INVALID)         \
    V("writeInt16LE", WriteInt16LE, 2, INVALID)         \
    V("writeInt32BE", WriteInt32BE, 2, INVALID)         \
    V("writeInt32LE", WriteInt32LE, 2, INVALID)         \
    V("writeFloatBE", WriteFloat32BE, 2, INVALID)       \
    V("writeDoubleBE", WriteFloat64BE, 2, INVALID)      \
    V("writeFloatLE", WriteFloat32LE, 2, INVALID)       \
    V("writeDoubleLE", WriteFloat64LE, 2, INVALID)      \
    V("writeBigUInt64BE", WriteBigUInt64BE, 2, INVALID) \
    V("writeBigUInt64LE", WriteBigUInt64LE, 2, INVALID) \
    V("writeBigInt64BE", WriteBigInt64BE, 2, INVALID)   \
    V("writeBigInt64LE", WriteBigInt64LE, 2, INVALID)   \
    V("readUIntBE", ReadUIntBE, 2, INVALID)             \
    V("readUIntLE", ReadUIntLE, 2, INVALID)             \
    V("readIntBE", ReadIntBE, 2, INVALID)               \
    V("readIntLE", ReadIntLE, 2, INVALID)               \
    V("readInt8", ReadInt8, 1, INVALID)                 \
    V("readUInt8", ReadUInt8, 1, INVALID)               \
    V("readUInt16BE", ReadUInt16BE, 1, INVALID)         \
    V("readUInt16LE", ReadUInt16LE, 1, INVALID)         \
    V("readUInt32BE", ReadUInt32BE, 1, INVALID)         \
    V("readUInt32LE", ReadUInt32LE, 1, INVALID)         \
    V("readInt16BE", ReadInt16BE, 1, INVALID)           \
    V("readInt16LE", ReadInt16LE, 1, INVALID)           \
    V("readInt32BE", ReadInt32BE, 1, INVALID)           \
    V("readInt32LE", ReadInt32LE, 1, INVALID)           \
    V("readFloatBE", ReadFloat32BE, 1, INVALID)         \
    V("readFloatLE", ReadFloat32LE, 1, INVALID)         \
    V("readDoubleBE", ReadFloat64BE, 1, INVALID)        \
    V("readDoubleLE", ReadFloat64LE, 1, INVALID)        \
    V("readBigUInt64BE", ReadBigUInt64BE, 1, INVALID)   \
    V("readBigUInt64LE", ReadBigUInt64LE, 1, INVALID)   \
    V("readBigInt64BE", ReadBigInt64BE, 1, INVALID)     \
    V("readBigInt64LE", ReadBigInt64LE, 1, INVALID)     \
    V("entries", Entries, 0, INVALID)                   \
    V("keys", Keys, 0, INVALID)                         \
    V("values", Values, 0, INVALID)                     \
    V("copy", Copy, 4, INVALID)                         \
    V("fill", Fill, 4, INVALID)                         \
    V("includes", Includes, 3, INVALID)                 \
    V("indexOf", IndexOf, 3, INVALID)                   \
    V("lastIndexOf", LastIndexOf, 3, INVALID)           \
    V("compare", Compare, 5, INVALID)                   \
    V("equals", Equals, 1, INVALID)

class ContainersBuffer : public base::BuiltinsBase {
public:
    static JSTaggedValue BufferConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Fill(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Copy(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Compare(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Equals(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Write(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue WriteIntBE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue WriteIntLE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue WriteUIntBE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue WriteUIntLE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReadIntBE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReadIntLE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReadUIntBE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReadUIntLE(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue WriteInt8(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue WriteUInt8(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReadUInt8(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReadInt8(EcmaRuntimeCallInfo *argv);

#define DECL_CONTAINER_BUFFER_ACCESSORS(name)                        \
    static JSTaggedValue Write##name##BE(EcmaRuntimeCallInfo *argv); \
    static JSTaggedValue Write##name##LE(EcmaRuntimeCallInfo *argv); \
    static JSTaggedValue Read##name##BE(EcmaRuntimeCallInfo *argv);  \
    static JSTaggedValue Read##name##LE(EcmaRuntimeCallInfo *argv);
    DECL_CONTAINER_BUFFER_ACCESSORS(UInt8)
    DECL_CONTAINER_BUFFER_ACCESSORS(UInt16)
    DECL_CONTAINER_BUFFER_ACCESSORS(UInt32)
    DECL_CONTAINER_BUFFER_ACCESSORS(BigInt64)
    DECL_CONTAINER_BUFFER_ACCESSORS(Int8)
    DECL_CONTAINER_BUFFER_ACCESSORS(Int16)
    DECL_CONTAINER_BUFFER_ACCESSORS(Int32)
    DECL_CONTAINER_BUFFER_ACCESSORS(BigUInt64)
    DECL_CONTAINER_BUFFER_ACCESSORS(Float32)
    DECL_CONTAINER_BUFFER_ACCESSORS(Float64)
#undef DECL_CONTAINER_BUFFER_ACCESSORS

    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetByteOffset(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetArrayBuffer(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetFastBufferPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(FASTBUFFER_PROTOTYPE_FUNCTIONS);
    }

private:
#define CONTAINER_FASTBUFFER_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersBuffer::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array FASTBUFFER_PROTOTYPE_FUNCTIONS = {
        CONTAINER_FASTBUFFER_PROTOTYPE_FUNCTIONS(CONTAINER_FASTBUFFER_FUNCTION_ENTRY)};
#undef CONTAINER_FASTBUFFER_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_BUFFER_H
