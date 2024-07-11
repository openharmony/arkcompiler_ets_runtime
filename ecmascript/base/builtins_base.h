/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_BUILTINS_BASE_H
#define ECMASCRIPT_BASE_BUILTINS_BASE_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class JSArray;
namespace base {
class BuiltinConstantEntry {
public:
    constexpr BuiltinConstantEntry(std::string_view name, JSTaggedValue value)
        : name_(name), rawTaggedValue_(value.GetRawData()) {}

    static constexpr BuiltinConstantEntry Create(std::string_view name, JSTaggedValue value)
    {
        return BuiltinConstantEntry(name, value);
    }

    constexpr std::string_view GetName() const
    {
        return name_;
    }

    constexpr JSTaggedValue GetTaggedValue() const
    {
        return JSTaggedValue(rawTaggedValue_);
    }

private:
    std::string_view name_;
    JSTaggedType rawTaggedValue_;
};

class BuiltinsPropertyConfig {
public:
    constexpr BuiltinsPropertyConfig(std::string_view name, bool isAccessor, bool writable,
        bool enumerable, bool configurable)
        : name_(name), isAccessor_(isAccessor), writable_(writable),
          enumerable_(enumerable), configurable_(configurable) {}

    constexpr std::string_view GetName() const
    {
        return name_;
    }

    constexpr bool GetIsAccessor() const
    {
        return isAccessor_;
    }

    constexpr bool GetWritable() const
    {
        return writable_;
    }

    constexpr bool GetEnumerable() const
    {
        return enumerable_;
    }

    constexpr bool GetConfigurable() const
    {
        return configurable_;
    }

private:
    std::string_view name_;
    bool isAccessor_ {false};
    bool writable_ {false};
    bool enumerable_ {false};
    bool configurable_ {false};
};

class BuiltinFunctionEntry {
public:
    static constexpr int LENGTH_BITS_SIZE = 8;
    static constexpr int BUILTIN_ID_BITS_SIZE = 16;
    // Assures the bits are enough to represent all builtin stubs.
    static_assert(kungfu::BuiltinsStubCSigns::NUM_OF_BUILTINS_STUBS <= (1u << BUILTIN_ID_BITS_SIZE));

    using LengthBits = panda::BitField<int, 0, LENGTH_BITS_SIZE>;
    using BuiltinIdBits = LengthBits::NextField<kungfu::BuiltinsStubCSigns::ID, BUILTIN_ID_BITS_SIZE>;
    using IsConstructorBit = BuiltinIdBits::NextFlag;
    using IsAccessorBit = IsConstructorBit::NextFlag;

    template <class... BitFieldArgs>
    static constexpr BuiltinFunctionEntry Create(std::string_view name, EcmaEntrypoint entrypoint,
                                                 int length, kungfu::BuiltinsStubCSigns::ID builtinId)
    {
        static_assert((std::is_same_v<typename BitFieldArgs::ValueType, bool> && ...),
                      "Only 1-bit fields are available in BitFieldArgs");
        uint64_t bitfield = 0;
        bitfield |= LengthBits::Encode(length);
        bitfield |= BuiltinIdBits::Encode(builtinId);
        // Traverses BitFieldArgs (IsConstructorBit, IsAccessorBit, etc.)
        ((bitfield |= BitFieldArgs::Encode(true)), ...);
        return BuiltinFunctionEntry(name, entrypoint, bitfield);
    }

    constexpr std::string_view GetName() const
    {
        return name_;
    }

    constexpr EcmaEntrypoint GetEntrypoint() const
    {
        return entrypoint_;
    }

    constexpr int GetLength() const
    {
        return LengthBits::Decode(bitfield_);
    }

    constexpr kungfu::BuiltinsStubCSigns::ID GetBuiltinStubId() const
    {
        return BuiltinIdBits::Decode(bitfield_);
    }

    constexpr bool IsConstructor() const
    {
        return IsConstructorBit::Decode(bitfield_);
    }

    constexpr bool IsAccessor() const
    {
        return IsAccessorBit::Decode(bitfield_);
    }

private:
    std::string_view name_;
    EcmaEntrypoint entrypoint_;
    uint64_t bitfield_;

    constexpr BuiltinFunctionEntry(std::string_view name, EcmaEntrypoint entrypoint, uint64_t bitfield)
        : name_(name), entrypoint_(entrypoint), bitfield_(bitfield) {}
};

class BuiltinsBase {
public:
    enum ArgsPosition : uint32_t { FIRST = 0, SECOND, THIRD, FOURTH, FIFTH };
    static JSHandle<TaggedArray> GetArgsArray(EcmaRuntimeCallInfo *msg);
    static inline JSHandle<JSTaggedValue> GetConstructor(EcmaRuntimeCallInfo *msg)
    {
        return msg->GetFunction();
    }

    static inline JSHandle<JSTaggedValue> GetThis(EcmaRuntimeCallInfo *msg)
    {
        return msg->GetThis();
    }

    static inline JSHandle<JSTaggedValue> GetNewTarget(EcmaRuntimeCallInfo *msg)
    {
        return msg->GetNewTarget();
    }

    static inline JSHandle<JSTaggedValue> GetCallArg(EcmaRuntimeCallInfo *msg, uint32_t position)
    {
        if (position >= msg->GetArgsNumber()) {
            JSThread *thread = msg->GetThread();
            return thread->GlobalConstants()->GetHandledUndefined();
        }
        return msg->GetCallArg(position);
    }

    static inline JSTaggedValue GetTaggedInt(int32_t value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedInt64(int64_t value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedDouble(double value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedBoolean(bool value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedString(JSThread *thread, const char *str)
    {
        return thread->GetEcmaVM()->GetFactory()->NewFromASCII(str).GetTaggedValue();
    }
};
}  // namespace base
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_BASE_BUILTINS_BASE_H
