/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JSPANDAFILE_BYTECODE_INST_INSTRUCTION_H
#define ECMASCRIPT_JSPANDAFILE_BYTECODE_INST_INSTRUCTION_H

#include <cstdint>

#include "libpandabase/utils/bit_helpers.h"

#if !PANDA_TARGET_WINDOWS
#include "securec.h"
#endif


namespace panda::ecmascript {
class OldBytecodeInstBase {
public:
    OldBytecodeInstBase() = default;
    explicit OldBytecodeInstBase(const uint8_t *pc) : pc_ {pc} {}
    ~OldBytecodeInstBase() = default;

protected:
    const uint8_t *GetPointer(int32_t offset) const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return pc_ + offset;
    }

    const uint8_t *GetAddress() const
    {
        return pc_;
    }

    const uint8_t *GetAddress() volatile const
    {
        return pc_;
    }

    uint8_t GetPrimaryOpcode() const
    {
        return ReadByte(0);
    }

    uint8_t GetSecondaryOpcode() const
    {
        return ReadByte(1);
    }

    template <class T>
    T Read(size_t offset) const
    {
        using unaligned_type __attribute__((aligned(1))) = const T;
        return *reinterpret_cast<unaligned_type *>(GetPointer(offset));
    }

    void Write(uint32_t value, uint32_t offset, uint32_t width)
    {
        auto *dst = const_cast<uint8_t *>(GetPointer(offset));
        if (memcpy_s(dst, width, &value, width) != 0) {
            LOG(FATAL, PANDAFILE) << "Cannot write value : " << value << "at the dst offset : " << offset;
        }
    }

    uint8_t ReadByte(size_t offset) const
    {
        return Read<uint8_t>(offset);
    }

    template <class R, class S>
    inline auto ReadHelper(size_t byteoffset, size_t bytecount, size_t offset, size_t width) const
    {
        constexpr size_t BYTE_WIDTH = 8;

        size_t right_shift = offset % BYTE_WIDTH;

        S v = 0;
        for (size_t i = 0; i < bytecount; i++) {
            S mask = static_cast<S>(ReadByte(byteoffset + i)) << (i * BYTE_WIDTH);
            v |= mask;
        }

        v >>= right_shift;
        size_t left_shift = sizeof(R) * BYTE_WIDTH - width;

        // Do sign extension using arithmetic shift. It's implementation defined
        // so we check such behavior using static assert
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        static_assert((-1 >> 1) == -1);

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        return static_cast<R>(v << left_shift) >> left_shift;
    }

    template <size_t offset, size_t width, bool is_signed = false>
    inline auto Read() const
    {
        constexpr size_t BYTE_WIDTH = 8;
        constexpr size_t BYTE_OFFSET = offset / BYTE_WIDTH;
        constexpr size_t BYTE_OFFSET_END = (offset + width + BYTE_WIDTH - 1) / BYTE_WIDTH;
        constexpr size_t BYTE_COUNT = BYTE_OFFSET_END - BYTE_OFFSET;

        using storage_type = helpers::TypeHelperT<BYTE_COUNT * BYTE_WIDTH, false>;
        using return_type = helpers::TypeHelperT<width, is_signed>;

        return ReadHelper<return_type, storage_type>(BYTE_OFFSET, BYTE_COUNT, offset, width);
    }

    template <bool is_signed = false>
    inline auto Read64(size_t offset, size_t width) const
    {
        constexpr size_t BIT64 = 64;
        constexpr size_t BYTE_WIDTH = 8;

        ASSERT((offset % BYTE_WIDTH) + width <= BIT64);

        size_t byteoffset = offset / BYTE_WIDTH;
        size_t byteoffset_end = (offset + width + BYTE_WIDTH - 1) / BYTE_WIDTH;
        size_t bytecount = byteoffset_end - byteoffset;

        using storage_type = helpers::TypeHelperT<BIT64, false>;
        using return_type = helpers::TypeHelperT<BIT64, is_signed>;

        return ReadHelper<return_type, storage_type>(byteoffset, bytecount, offset, width);
    }

private:
    const uint8_t *pc_ {nullptr};
};

}  // panda::ecmascript

#endif  // ECMASCRIPT_JSPANDAFILE_BYTECODE_INST_OLD_INSTRUCTION_H