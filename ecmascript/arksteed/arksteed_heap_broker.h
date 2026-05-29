/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_HEAP_BROKER_H
#define ECMASCRIPT_ARKSTEED_HEAP_BROKER_H

#include <array>
#include <optional>

#include "ecmascript/arksteed/arksteed_heap_ref.h"
#include "ecmascript/arksteed/arksteed_processed_feedback.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {

class ArkSteedFeedbackReader;

enum class ArkSteedBrokerMode : uint8_t {
    DISABLED,
    SERIALIZING,
    SERIALIZED,
    RETIRED,
};

class ArkSteedHeapBroker {
public:
    class SerializingScope {
    public:
        SerializingScope(const ArkSteedHeapBroker *broker, CString message) : broker_(broker)
        {
            if (!broker_->SerializingAllowed()) {
                lock_.emplace(broker_->env_, message);
                broker_->StartSerializing();
                ownsSerializing_ = true;
            }
        }

        ~SerializingScope()
        {
            if (ownsSerializing_) {
                broker_->StopSerializing();
            }
        }

        NO_COPY_SEMANTIC(SerializingScope);
        NO_MOVE_SEMANTIC(SerializingScope);

    private:
        const ArkSteedHeapBroker *broker_ {nullptr};
        std::optional<Jit::JitLockHolder> lock_ {};
        bool ownsSerializing_ {false};
    };

    ArkSteedHeapBroker(JSThread *compilerThread, JitCompilationEnv *env)
        : compilerThread_(compilerThread), env_(env)
    {}

    void StartSerializing() const
    {
        mode_ = ArkSteedBrokerMode::SERIALIZING;
    }

    void StopSerializing() const
    {
        mode_ = ArkSteedBrokerMode::SERIALIZED;
    }

    void Retire() const
    {
        mode_ = ArkSteedBrokerMode::RETIRED;
    }

    ArkSteedBrokerMode Mode() const
    {
        return mode_;
    }

    bool SerializingAllowed() const
    {
        return mode_ == ArkSteedBrokerMode::SERIALIZING;
    }

    bool TryGetProfileTypeInfo(ProfileTypeInfo **profileTypeArray) const
    {
        if (!SerializingAllowed()) {
            return false;
        }
        auto profileTypeInfo = env_->GetProfileTypeInfo();
        JSTaggedValue profileTypeInfoValue = GetRawHandleValue(profileTypeInfo);
        if (profileTypeInfoValue.IsUndefined() || !profileTypeInfoValue.IsTaggedArray()) {
            return false;
        }

        *profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfoValue.GetTaggedObject());
        return true;
    }

    ArkSteedObjectRef MakeObjectRef(JSTaggedValue value) const
    {
        return MakeStableHeapRef(value, false);
    }

    ArkSteedHClassRef MakeHClassRef(JSTaggedValue value) const
    {
        return MakeStableHeapRef(value, false);
    }

    ArkSteedHClassRef MakeWeakHClassRef(JSTaggedValue weakValue) const
    {
        return MakeStableHeapRef(JSTaggedValue(weakValue.GetWeakReferent()), false);
    }

    ArkSteedProtoCellRef MakeProtoCellRef(JSTaggedValue value) const
    {
        return MakeStableHeapRef(value, false);
    }

    ArkSteedHandlerRef MakeHandlerRef(JSTaggedValue value) const
    {
        return MakeStableHeapRef(value, true);
    }

    ArkSteedNameRef MakeNameRef(JSTaggedValue value) const
    {
        return MakeStableHeapRef(value, false);
    }

    bool TryGetNameFromConstantPool(uint16_t cpIdx, ArkSteedNameRef *name) const
    {
        if (!SerializingAllowed()) {
            return false;
        }
        uint32_t methodOffset = env_->GetMethodLiteral()->GetMethodId().GetOffset();
        JSTaggedValue nameValue = env_->GetStringFromConstantPool(methodOffset, cpIdx, false);
        if (nameValue.IsUndefined()) {
            return false;
        }
        *name = MakeNameRef(nameValue);
        return name->IsSafeForCompile();
    }

    JSThread *GetCompilerThread() const
    {
        return compilerThread_;
    }

    bool GetFeedbackForNamedAccess(const ArkSteedFeedbackReader &reader, int slotIndex,
                                   NamedAccessFeedback *feedback) const;

    bool TryGetCachedNamedAccessFeedback(AccessFeedbackSource source, NamedAccessFeedback *feedback) const
    {
        for (const auto &entry : namedAccessFeedbackCache_) {
            if (entry.valid && IsSameFeedbackSource(entry.source, source)) {
                *feedback = entry.feedback;
                return true;
            }
        }
        return false;
    }

    void CacheNamedAccessFeedback(const NamedAccessFeedback &feedback) const
    {
        uint32_t index = feedback.base.source.slotId % FEEDBACK_CACHE_SIZE;
        namedAccessFeedbackCache_[index] = {true, feedback.base.source, feedback};
    }

private:
    static constexpr uint32_t FEEDBACK_CACHE_SIZE = 16;

    struct NamedAccessFeedbackCacheEntry {
        bool valid {false};
        AccessFeedbackSource source {};
        NamedAccessFeedback feedback {};
    };

    static bool IsSameFeedbackSource(AccessFeedbackSource left, AccessFeedbackSource right)
    {
        return left.slotId == right.slotId && left.isPoly == right.isPoly && left.slotKind == right.slotKind;
    }

    template <typename T>
    static JSTaggedValue GetRawHandleValue(const JSHandle<T> &handle)
    {
        uintptr_t address = handle.GetAddress();
        if (address == 0U) {
            return JSTaggedValue::Undefined();
        }
        return JSTaggedValue(*reinterpret_cast<JSTaggedType *>(address));
    }

    ArkSteedHeapRef MakeStableHeapRef(JSTaggedValue value, bool allowPrimitive) const
    {
        if (!SerializingAllowed()) {
            return ArkSteedHeapRef();
        }
        if (!value.IsHeapObject()) {
            return allowPrimitive ? ArkSteedHeapRef(value, true) : ArkSteedHeapRef();
        }
        JSHandle<JSTaggedValue> handle = env_->NewJSHandle(value);
        return ArkSteedHeapRef(handle);
    }

    JSThread *compilerThread_ {nullptr};
    JitCompilationEnv *env_ {nullptr};
    mutable ArkSteedBrokerMode mode_ {ArkSteedBrokerMode::SERIALIZED};
    mutable std::array<NamedAccessFeedbackCacheEntry, FEEDBACK_CACHE_SIZE> namedAccessFeedbackCache_ {};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_HEAP_BROKER_H
