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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_REGLIST_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_REGLIST_H

#include <sstream>

#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

// RegListBase - Bitmask-based register list
template <typename RegisterT>
class RegListBase {
public:
    // 64: max registers for uint64_t bitmask
    static_assert(RegisterT::NUM_REGISTERS <= 64, "RegListBase supports at most 64 registers");

    using storage_t = std::conditional_t<RegisterT::NUM_REGISTERS <= 16, uint16_t, // 16: fits in uint16_t
        std::conditional_t<RegisterT::NUM_REGISTERS <= 32, uint32_t, uint64_t>>;   // 32: fits in uint32_t

    constexpr RegListBase() : bits_(0) {}

    constexpr RegListBase(std::initializer_list<RegisterT> regs) : bits_(0)
    {
        for (RegisterT reg : regs) {
            Set(reg);
        }
    }

    constexpr void Set(RegisterT reg)
    {
        bits_ |= (storage_t{1} << reg.Code());
    }

    void Clear(RegisterT reg)
    {
        bits_ &= ~(storage_t{1} << reg.Code());
    }

    constexpr bool Has(RegisterT reg) const
    {
        return (bits_ & (storage_t{1} << reg.Code())) != 0;
    }

    void Clear(const RegListBase &other)
    {
        bits_ &= ~other.bits_;
    }

    constexpr bool IsEmpty() const
    {
        return bits_ == 0;
    }

    void Reset()
    {
        bits_ = 0;
    }

    constexpr uint32_t Count() const
    {
        storage_t bits = bits_;
        uint32_t count = 0;
        while (bits) {
            count += bits & 1;
            bits >>= 1;
        }
        return count;
    }

    constexpr RegListBase operator&(const RegListBase &other) const
    {
        return RegListBase(bits_ & other.bits_);
    }

    constexpr RegListBase operator|(const RegListBase &other) const
    {
        return RegListBase(bits_ | other.bits_);
    }

    constexpr RegListBase operator^(const RegListBase &other) const
    {
        return RegListBase(bits_ ^ other.bits_);
    }

    constexpr RegListBase operator-(const RegListBase &other) const
    {
        return RegListBase(bits_ & ~other.bits_);
    }

    constexpr RegListBase operator|(RegisterT reg) const
    {
        RegListBase result = *this;
        result.Set(reg);
        return result;
    }

    constexpr RegListBase operator-(RegisterT reg) const
    {
        RegListBase result = *this;
        result.Clear(reg);
        return result;
    }

    constexpr RegListBase &operator&=(const RegListBase &other)
    {
        bits_ &= other.bits_;
        return *this;
    }

    constexpr RegListBase &operator|=(const RegListBase &other)
    {
        bits_ |= other.bits_;
        return *this;
    }

    constexpr RegListBase &operator^=(const RegListBase &other)
    {
        bits_ ^= other.bits_;
        return *this;
    }

    constexpr bool operator==(const RegListBase &other) const
    {
        return bits_ == other.bits_;
    }

    constexpr bool operator!=(const RegListBase &other) const
    {
        return bits_ != other.bits_;
    }

    constexpr storage_t Bits() const
    {
        return bits_;
    }

    constexpr RegisterT First() const
    {
        ASSERT(!IsEmpty());
        int firstCode = __builtin_ctzll(bits_);
        return RegisterT::FromCode(firstCode);
    }

    constexpr RegisterT Last() const
    {
        ASSERT(!IsEmpty());
        int lastCode = 8 * sizeof(bits_) - 1 - __builtin_clzll(bits_);  // 8: bits per byte
        return RegisterT::FromCode(lastCode);
    }

    constexpr RegisterT PopFirst()
    {
        RegisterT reg = First();
        Clear(reg);
        return reg;
    }

    std::string Dump() const;

    static constexpr RegListBase FromBits(storage_t bits)
    {
        return RegListBase(bits);
    }

    class Iterator {
    public:
        constexpr Iterator() : remaining_() {}
        explicit constexpr Iterator(RegListBase remaining) : remaining_(remaining) {}

        constexpr bool operator!=(const Iterator &other) const
        {
            return remaining_ != other.remaining_;
        }

        constexpr bool operator==(const Iterator &other) const
        {
            return remaining_ == other.remaining_;
        }

        constexpr Iterator &operator++()
        {
            RegisterT reg = remaining_.First();
            remaining_.Clear(reg);
            return *this;
        }

        constexpr RegisterT operator*() const
        {
            return remaining_.First();
        }

    private:
        RegListBase remaining_;
    };

    constexpr Iterator begin() const
    {
        return Iterator(*this);
    }
    constexpr Iterator end() const
    {
        return Iterator();
    }

private:
    explicit constexpr RegListBase(storage_t bits) : bits_(bits) {}

    storage_t bits_ = 0;
};

struct SpillInfo {
    uint32_t slotIndex;
    VertexId freedAtPosition;
    bool doubleSlot;

    SpillInfo(uint32_t index, VertexId pos, bool isDouble)
        : slotIndex(index), freedAtPosition(pos), doubleSlot(isDouble)
    {}
};

struct SpillLocations {
    int top = 0;
    std::vector<SpillInfo> freeSlots;
};

// -------- Implementation --------

template <typename RegisterT>
std::string RegListBase<RegisterT>::Dump() const
{
    std::ostringstream out;
    out << '{';
    bool first = true;
    for (int i = 0; i < RegisterT::NUM_REGISTERS; i++) {
        if (bits_ & (1u << i)) {
            first ? (void)(first = false) : (void)(out << ", ");
            out << i;
        }
    }
    out << '}';
    return out.str();
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_REGLIST_H
