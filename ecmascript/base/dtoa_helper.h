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
namespace panda::ecmascript::base::dtoa {
class DtoaHelper {
public:
    static constexpr int CACHED_POWERS_OFFSET = 348;
    static constexpr double kD_1_LOG2_10 = 0.30102999566398114;  //1 / lg(10)
    static constexpr int kQ = -61;
    static constexpr int kIndex = 20;
    static constexpr int MIN_DECIMAL_EXPONENT = -348;
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
        unsigned index = static_cast<unsigned>((k >> 3) + 1); // 3: parameter
        *K = -(MIN_DECIMAL_EXPONENT + static_cast<int>(index << 3)); // 3: parameter
        return GetCachedPowerByIndex(index);
    }

    static DiyFp GetCachedPowerByIndex(size_t index);
    static void GrisuRound(char* buffer, int len, uint64_t delta, uint64_t rest, uint64_t tenKappa, uint64_t distance);
    static int CountDecimalDigit32(uint32_t n);
    static void DigitGen(const DiyFp& W, const DiyFp& Mp, uint64_t delta, char* buffer, int* len, int* K);
    static void Grisu(double value, char* buffer, int* length, int* K);
    static void Dtoa(double value, char* buffer, int* point, int* length);
};
}
#endif