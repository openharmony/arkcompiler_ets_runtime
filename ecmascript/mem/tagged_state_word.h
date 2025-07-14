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

#ifndef ECMASCRIPT_TAGGED_OBJECT_STATE_WORD_H
#define ECMASCRIPT_TAGGED_OBJECT_STATE_WORD_H

#include "common_interfaces/base/common.h"
#include "common_interfaces/objects/base_state_word.h"
#include <atomic>

using ClassWordType = uint64_t;

namespace panda::ecmascript {
class TaggedObject;

class TaggedStateWord {
public:
    static constexpr size_t ADDRESS_WIDTH = 48;
    static constexpr uint64_t ADDRESS_MASK = (0x1ULL << ADDRESS_WIDTH) - 1;

    // Little endian
    struct GCStateWord {
        common::StateWordType address_   : ADDRESS_WIDTH;
        common::StateWordType padding_   : 12;
        common::StateWordType remainded_ : 4;
    };

    struct ClassStateWord {
        ClassWordType class_ : ADDRESS_WIDTH;
        ClassWordType padding_ : 12;
        ClassWordType remainded_ : 4;
    };

    inline void SynchronizedSetGCStateWord(common::StateWordType address, common::StateWordType padding = 0,
                                           common::StateWordType remainded = 0)
    {
        GCStateWord newState;
        newState.address_ = address;
        newState.padding_ = padding;
        newState.remainded_ = remainded;
        SynchronizedSetGCStateWord(newState);
    }
    inline void SetClass(ClassWordType cls)
    {
        class_.class_ = cls;
    }

    inline void SynchronizedSetClass(ClassWordType cls)
    {
        reinterpret_cast<volatile std::atomic<uint32_t> *>(&class_)->store(
            static_cast<uint32_t>(cls), std::memory_order_release);
    }

    inline uintptr_t GetClass() const
    {
        return static_cast<uintptr_t>(class_.class_);
    }

    inline uintptr_t SynchronizedGetClass() const
    {
        return static_cast<uintptr_t>(
                   reinterpret_cast<volatile std::atomic<uint32_t> *>(reinterpret_cast<uintptr_t>(&class_))
                       ->load(std::memory_order_acquire)) +
               BASE_ADDRESS;
    }

    inline void SetForwardingAddress(uintptr_t fwdPtr)
    {
        state_.address_ = fwdPtr;
    }

    inline uintptr_t GetForwardingAddress() const
    {
        return state_.address_;
    }

    static constexpr int STATE_WORD_OFFSET = sizeof(ClassWordType);
    static PUBLIC_API uintptr_t BASE_ADDRESS;

private:
    inline void SynchronizedSetGCStateWord(GCStateWord state)
    {
        reinterpret_cast<volatile std::atomic<GCStateWord> *>(&state_)->store(state, std::memory_order_release);
    }

    union {
        GCStateWord state_;
        ClassStateWord class_;
    };

    friend class TaggedObject;
};
static_assert(sizeof(TaggedStateWord) == sizeof(uint64_t), "Excepts 8 bytes");
static_assert(common::BaseStateWord::PADDING_WIDTH == 60, "Excepts 60 bits");
static_assert(common::BaseStateWord::FORWARD_WIDTH == 2, "Excepts 2 bits");
static_assert(common::BaseStateWord::LANGUAGE_WIDTH == 2, "Excepts 2 bits");
}  //  namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_OBJECT_STATE_WORD_H
