/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_DTOA_HELPER_H
#define ECMASCRIPT_BASE_DTOA_HELPER_H

#include <math.h>
#include <stdint.h>
#include <stdint.h>
#include <limits>

#include "ecmascript/common.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::base {

template <typename T>
class BufferVector {
public:
    BufferVector() : start_(NULL), length_(0) {}
    BufferVector(T* data, int length) : start_(data), length_(length)
    {
        ASSERT(length == 0 || (length > 0 && data != NULL));
    }
    int length() const { return length_; }
    T* start() const { return start_; }

    // Access individual vector elements - checks bounds in debug mode.
    T& operator[](int index) const
    {
        ASSERT(0 <= index && index < length_);
        return start_[index];
    }

    T& first() { return start_[0]; }

    T& last() { return start_[length_ - 1]; }

private:
    T* start_;
    int length_;
};

class UInt128 {
public:
    UInt128() : high_bits_(0), low_bits_(0) { }
    UInt128(uint64_t high, uint64_t low) : high_bits_(high), low_bits_(low) { }

    static constexpr int SHIFT_32BIT = 32;
    static constexpr int SHIFT_64BIT = 64;
    static constexpr int NEGATIVE_64BIT = -64;

    void Multiply(uint32_t multiplicand)
    {
        uint64_t accumulator = (low_bits_ & kMask32) * multiplicand;
        uint32_t part = static_cast<uint32_t>(accumulator & kMask32);
        accumulator >>= SHIFT_32BIT;
        accumulator = accumulator + (low_bits_ >> SHIFT_32BIT) * multiplicand;
        low_bits_ = (accumulator << SHIFT_32BIT) + part;
        accumulator >>= SHIFT_32BIT;
        accumulator = accumulator + (high_bits_ & kMask32) * multiplicand;
        part = static_cast<uint32_t>(accumulator & kMask32);
        accumulator >>= SHIFT_32BIT;
        accumulator = accumulator + (high_bits_ >> SHIFT_32BIT) * multiplicand;
        high_bits_ = (accumulator << SHIFT_32BIT) + part;
        ASSERT((accumulator >> SHIFT_32BIT) == 0);
    }

    void Shift(int shift_amount)
    {
        ASSERT(NEGATIVE_64BIT <= shift_amount && shift_amount <= SHIFT_64BIT);
        if (shift_amount == 0) {
            return;
        } else if (shift_amount == NEGATIVE_64BIT) {
            high_bits_ = low_bits_;
            low_bits_ = 0;
        } else if (shift_amount == SHIFT_64BIT) {
            low_bits_ = high_bits_;
            high_bits_ = 0;
        } else if (shift_amount <= 0) {
            high_bits_ <<= -shift_amount;
            high_bits_ += low_bits_ >> (SHIFT_64BIT + shift_amount);
            low_bits_ <<= -shift_amount;
        } else {
            low_bits_ >>= shift_amount;
            low_bits_ += high_bits_ << (SHIFT_64BIT - shift_amount);
            high_bits_ >>= shift_amount;
        }
    }

    // Modifies *this to *this MOD (2^power).
    // Returns *this DIV (2^power).
    int DivModPowerOf2(int power)
    {
        if (power >= SHIFT_64BIT) {
            int result = static_cast<int>(high_bits_ >> (power - SHIFT_64BIT));
            high_bits_ -= static_cast<uint64_t>(result) << (power - SHIFT_64BIT);
            return result;
        } else {
            uint64_t part_low = low_bits_ >> power;
            uint64_t part_high = high_bits_ << (SHIFT_64BIT - power);
            int result = static_cast<int>(part_low + part_high);
            high_bits_ = 0;
            low_bits_ -= part_low << power;
            return result;
        }
    }

    bool IsZero() const
    {
        return high_bits_ == 0 && low_bits_ == 0;
    }

    int BitAt(int position)
    {
        if (position >= SHIFT_64BIT) {
            return static_cast<int>((high_bits_ >> (position - SHIFT_64BIT)) & 1);
        } else {
            return static_cast<int>((low_bits_ >> position) & 1);
        }
    }

private:
    static const uint64_t kMask32 = 0xFFFFFFFF;
    uint64_t high_bits_;
    uint64_t low_bits_;
};

class DtoaHelper {
public:
    static const int kDoubleSignificandSize = 53;  // Includes the hidden bit.
    static const uint32_t kMaxUInt32 = 0xFFFFFFFF;
    static constexpr int CACHED_POWERS_OFFSET = 348;
    static constexpr double kD_1_LOG2_10 = 0.30102999566398114;  //1 / lg(10)
    static constexpr int kQ = -61;
    static constexpr int kIndex = 20;
    static constexpr int MIN_DECIMAL_EXPONENT = -348;
    static constexpr int EXPONENT_64 = 64;
    static constexpr int EXPONENT_128 = 128;
    static constexpr int NEGATIVE_128BIT = -128;
    static constexpr uint64_t POW10[] = { 1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL,
                                          10000000ULL, 100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL,
                                          1000000000000ULL, 10000000000000ULL, 100000000000000ULL, 1000000000000000ULL,
                                          10000000000000000ULL, 100000000000000000ULL, 1000000000000000000ULL,
                                          10000000000000000000ULL };

    static constexpr uint32_t TEN = 10;
    static constexpr uint32_t TEN2POW = 100;
    static constexpr uint32_t TEN3POW = 1000;
    static constexpr uint32_t TEN4POW = 10000;
    static constexpr uint32_t TEN5POW = 100000;
    static constexpr uint32_t TEN6POW = 1000000;
    static constexpr uint32_t TEN7POW = 10000000;
    static constexpr uint32_t TEN8POW = 100000000;
    static constexpr uint32_t TEN9POW = 1000000000;
    // DiyFp is a floating-point number type, consists of a uint64 significand and one integer exponent.
    struct DiyFp {
        DiyFp() : f(), e() {}
        DiyFp(uint64_t fp, int exp) : f(fp), e(exp) {}

        explicit DiyFp(double d)
        {
            union {
                double d;
                uint64_t u64;
            } u = { d };

            int biased_e = static_cast<int>((u.u64 & kDpExponentMask) >> kDpSignificandSize);
            uint64_t significand = (u.u64 & kDpSignificandMask);
            if (biased_e != 0) {
                f = significand + kDpHiddenBit;
                e = biased_e - kDpExponentBias;
            } else {
                f = significand;
                e = kDpMinExponent + 1;
            }
        }

        DiyFp operator - (const DiyFp &rhs) const
        {
            return DiyFp(f - rhs.f, e);
        }

        DiyFp operator*(const DiyFp &rhs) const
        {
            const uint64_t M32 = 0xFFFFFFFF;
            const uint64_t a = f >> 32;
            const uint64_t b = f & M32;
            const uint64_t c = rhs.f >> 32;
            const uint64_t d = rhs.f & M32;
            const uint64_t ac = a * c;
            const uint64_t bc = b * c;
            const uint64_t ad = a * d;
            const uint64_t bd = b * d;
            uint64_t tmp = (bd >> kInt32Bits) + (ad & M32) + (bc & M32);
            tmp += 1U << kRoundBits; // mult_round
            return DiyFp(ac + (ad >> kInt32Bits) + (bc >> kInt32Bits) + (tmp >> kInt32Bits), e + rhs.e + kInt64Bits);
        }

        DiyFp Normalize() const
        {
            DiyFp res = *this;
            while (!(res.f & kDpHiddenBit)) {
                res.f <<= 1;
                res.e--;
            }
            res.f <<= (kDiySignificandSize - kDpSignificandSize - 1);
            res.e = res.e - (kDiySignificandSize - kDpSignificandSize - 1);
            return res;
        }

        DiyFp NormalizeBoundary() const
        {
            DiyFp res = *this;
            while (!(res.f & (kDpHiddenBit << 1))) {
                res.f <<= 1;
                res.e--;
            }
            res.f <<= (kDiySignificandSize - kDpSignificandSize - 2); // 2: parameter
            res.e = res.e - (kDiySignificandSize - kDpSignificandSize - 2); // 2: parameter
            return res;
        }

        void NormalizedBoundaries(DiyFp *minus, DiyFp *plus) const
        {
            DiyFp pl = DiyFp((f << 1) + 1, e - 1).NormalizeBoundary();
            DiyFp mi = (f == kDpHiddenBit) ? DiyFp((f << 2) - 1, e - 2) : DiyFp((f << 1) - 1, e - 1); // 2: parameter
            mi.f <<= mi.e - pl.e;
            mi.e = pl.e;
            *plus = pl;
            *minus = mi;
        }

        static const int kInt64Bits = 64;
        static const int kInt32Bits = 32;
        static const int kRoundBits = 31;
        static const int kDiySignificandSize = 64;
        static const int kDpSignificandSize = 52;
        static const int kDpExponentBias = 0x3FF + kDpSignificandSize;
        static const int kDpMaxExponent = 0x7FF - kDpExponentBias;
        static const int kDpMinExponent = -kDpExponentBias;
        static const int kDpDenormalExponent = -kDpExponentBias + 1;
        static const uint64_t kDpExponentMask =
            (static_cast<uint64_t>(0x7FF00000) << 32) | static_cast<uint64_t>(0x00000000);
        static const uint64_t kDpSignificandMask =
            (static_cast<uint64_t>(0x000FFFFF) << 32) | static_cast<uint64_t>(0xFFFFFFFF);
        static const uint64_t kDpHiddenBit =
            (static_cast<uint64_t>(0x00100000) << 32) | static_cast<uint64_t>(0x00000000);

        uint64_t f;
        int e;
    };

    static DiyFp GetCachedPower(int e, int *K)
    {
        // dk must be positive, so can do ceiling in positive
        double dk = (kQ - e) * kD_1_LOG2_10 + CACHED_POWERS_OFFSET - 1;
        int k = static_cast<int>(dk);
        if (dk - k > 0.0) {
            k++;
        }
        unsigned index = static_cast<unsigned>((static_cast<unsigned>(k) >> 3) + 1); // 3: parameter
        *K = -(MIN_DECIMAL_EXPONENT + static_cast<int>(index << 3)); // 3: parameter
        return GetCachedPowerByIndex(index);
    }

    static DiyFp GetCachedPowerByIndex(size_t index);
    static void GrisuRound(char* buffer, int len, uint64_t delta, uint64_t rest, uint64_t tenKappa, uint64_t distance);
    static int CountDecimalDigit32(uint32_t n);
    static void DigitGen(const DiyFp& W, const DiyFp& Mp, uint64_t delta, char* buffer, int* len, int* K);
    static void Grisu(double value, char* buffer, int* length, int* K);
    static void Dtoa(double value, char* buffer, int* point, int* length);
    static void FillDigits32FixedLength(uint32_t number, int requested_length, BufferVector<char> buffer, int* length);
    static void FillDigits32(uint32_t number, BufferVector<char> buffer, int* length);
    static void FillDigits64FixedLength(uint64_t number, [[maybe_unused]] int requested_length,
                                        BufferVector<char> buffer, int* length);
    static void FillDigits64(uint64_t number, BufferVector<char> buffer, int* length);
    static void RoundUp(BufferVector<char> buffer, int* length, int* decimal_point);
    static void FillFractionals(uint64_t fractionals, int exponent, int fractional_count, BufferVector<char> buffer,
                                int* length, int* decimal_point);
    static void TrimZeros(BufferVector<char> buffer, int* length, int* decimal_point);
    static bool FixedDtoa(double v, int fractional_count, BufferVector<char> buffer, int* length, int* decimal_point);
};
}
#endif