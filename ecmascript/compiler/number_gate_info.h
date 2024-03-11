/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_NUMBER_GATE_INFO_H
#define ECMASCRIPT_NUMBER_GATE_INFO_H
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_typed_array.h"

namespace panda::ecmascript::kungfu {

enum class TypeInfo {
    NONE,
    INT1,
    INT32,
    UINT32,
    FLOAT64,
    TAGGED,
    CHAR,
    HOLE_INT,
    HOLE_DOUBLE,
};

class UseInfo {
public:
    UseInfo(uint8_t tag) : tag_(tag) {}
    static constexpr uint8_t UNUSED = 0;
    static constexpr uint8_t BOOL = 1 << 0;
    static constexpr uint8_t INT32 = 1 << 1;
    static constexpr uint8_t FLOAT64 = 1 << 2;
    static constexpr uint8_t NATIVE = BOOL | INT32 | FLOAT64;
    static constexpr uint8_t TAGGED = 1 << 3;
    bool AddUse(const UseInfo &UseInfo)
    {
        uint8_t oldTag = tag_;
        tag_ |= UseInfo.tag_;
        return oldTag != tag_;
    }
    bool UsedAsBool() const
    {
        return ((tag_ & BOOL) == BOOL);
    }
    bool UsedAsFloat64() const
    {
        return ((tag_ & FLOAT64) == FLOAT64);
    }
    bool UsedAsNative() const
    {
        return ((tag_ & NATIVE) != 0);
    }
    bool UsedAsTagged() const
    {
        return ((tag_ & TAGGED) != 0);
    }
    static UseInfo UnUsed()
    {
        return UseInfo(UNUSED);
    }
    static UseInfo BoolUse()
    {
        return UseInfo(BOOL);
    }
    static UseInfo Int32Use()
    {
        return UseInfo(INT32);
    }
    static UseInfo Float64Use()
    {
        return UseInfo(FLOAT64);
    }
    static UseInfo TaggedUse()
    {
        return UseInfo(TAGGED);
    }
private:
    uint8_t tag_ {0};
};

class RangeInfo {
public:
    RangeInfo() {}
    RangeInfo(int32_t value) : min_(value), max_(value) {}
    RangeInfo(int32_t min, int32_t max)
    {
        if (min == max) {
            min_ = max_ = min;
        } else {
            auto it = std::upper_bound(rangeBounds_.begin(), rangeBounds_.end(), min);
            ASSERT(it != rangeBounds_.begin());
            it--;
            min_ = *it;
            max_ = *std::lower_bound(rangeBounds_.begin(), rangeBounds_.end(), max);
        }
    }

    static constexpr int32_t UINT30_MAX = 0x3fffffff;
    static constexpr int32_t TYPED_ARRAY_ONHEAP_MAX = JSTypedArray::MAX_ONHEAP_LENGTH;
    static constexpr int32_t UINT18_MAX = (1 << 18) - 1;
    static const inline std::vector<int32_t> rangeBounds_ = {INT32_MIN, INT32_MIN + 1,
        -UINT18_MAX, -TYPED_ARRAY_ONHEAP_MAX, -1, 0, 1, TYPED_ARRAY_ONHEAP_MAX - 1,
        TYPED_ARRAY_ONHEAP_MAX, TYPED_ARRAY_ONHEAP_MAX + 1, TYPED_ARRAY_ONHEAP_MAX * 3,
        UINT18_MAX, UINT30_MAX, UINT30_MAX + 1, INT32_MAX - 1, INT32_MAX };

    static RangeInfo NONE()
    {
        return RangeInfo(INT32_MAX, INT32_MIN);
    }

    static RangeInfo ANY()
    {
        return RangeInfo(INT32_MIN, INT32_MAX);
    }

    int32_t GetMin() const
    {
        return min_;
    }

    int32_t GetMax() const
    {
        return max_;
    }

    RangeInfo operator~() const
    {
        return RangeInfo(~ max_, ~ min_);
    }

    RangeInfo Union(const RangeInfo &rhs) const
    {
        return RangeInfo(std::min(min_, rhs.min_), std::max(max_, rhs.max_));
    }

    RangeInfo intersection(const RangeInfo &rhs) const
    {
        return RangeInfo(std::max(min_, rhs.min_), std::min(max_, rhs.max_));
    }

    bool MaybeAddOverflow(const RangeInfo &rhs) const
    {
        return (rhs.max_ > 0) && (max_ > INT32_MAX - rhs.max_);
    }

    bool MaybeAddUnderflow(const RangeInfo &rhs) const
    {
        return (rhs.min_ < 0) && (min_ < INT32_MIN - rhs.min_);
    }

    bool MaybeAddOverflowOrUnderflow(const RangeInfo &rhs) const
    {
        return MaybeAddOverflow(rhs) || MaybeAddUnderflow(rhs);
    }

    RangeInfo operator+ (const RangeInfo &rhs) const
    {
        ASSERT(min_ <= max_ && rhs.min_ <= rhs.max_);
        int32_t nmax = MaybeAddOverflow(rhs) ? INT32_MAX : max_ + rhs.max_;
        int32_t nmin = MaybeAddUnderflow(rhs) ? INT32_MIN : min_ + rhs.min_;
        return RangeInfo(nmin, nmax);
    }

    RangeInfo operator% (const RangeInfo &rhs) const
    {
        ASSERT(min_ <= max_ && rhs.min_ <= rhs.max_);
        RangeInfo result = RangeInfo(0, 0);
        int32_t nmax = std::max(std::abs(rhs.min_), std::abs(rhs.max_));
        if (nmax == 0) {
            return ANY();
        }
        if (max_ > 0) result = result.Union(RangeInfo(0, nmax - 1));
        if (min_ < 0) result = result.Union(RangeInfo(-nmax + 1, 0));
        return result;
    }

    bool MaybeZero() const
    {
        return min_ <= 0 && max_ >= 0;
    }

    bool MaybeNegative() const
    {
        return min_ < 0;
    }

    RangeInfo operator* (const RangeInfo &rhs) const
    {
        ASSERT(min_ <= max_ && rhs.min_ <= rhs.max_);
        int32_t nmax = GetMaxMulResult(rhs);
        int32_t nmin = GetMinMulResult(rhs);
        return RangeInfo(nmin, nmax);
    }

    int32_t GetMaxMulResult(const RangeInfo &rhs) const
    {
        return std::max({ TryMul(min_, rhs.min_), TryMul(min_, rhs.max_),
            TryMul(max_, rhs.min_), TryMul(max_, rhs.max_) });
    }

    int32_t GetMinMulResult(const RangeInfo &rhs) const
    {
        return std::min({ TryMul(min_, rhs.min_), TryMul(min_, rhs.max_),
            TryMul(max_, rhs.min_), TryMul(max_, rhs.max_) });
    }

    int32_t TryMul(int32_t lhs, int32_t rhs) const
    {
        if (MaybeMulOverflow(lhs, rhs)) {
            return INT32_MAX;
        }
        if (MaybeMulUnderflow(lhs, rhs)) {
            return INT32_MIN;
        }
        return lhs * rhs;
    }

    bool MaybeMulOverflowOrUnderflow(const RangeInfo &rhs) const
    {
        return MaybeMulOverflow(rhs) || MaybeMulUnderflow(rhs);
    }

    bool MaybeMulUnderflow(const RangeInfo &rhs) const
    {
        return MaybeMulUnderflow(min_, rhs.max_) || MaybeMulUnderflow(max_, rhs.min_);
    }

    bool MaybeMulOverflow(const RangeInfo &rhs) const
    {
        return MaybeMulOverflow(max_, rhs.max_) || MaybeMulOverflow(min_, rhs.min_);
    }

    bool MaybeMulUnderflow(int32_t lhs, int32_t rhs) const
    {
        return (lhs > 0 && rhs < 0 && rhs < INT32_MIN / lhs) || (lhs < 0 && rhs > 0 && lhs < INT32_MIN / rhs);
    }

    bool MaybeMulOverflow(int32_t lhs, int32_t rhs) const
    {
        return (lhs > 0 && rhs > 0 && lhs > INT32_MAX / rhs) || (lhs < 0 && rhs < 0 && lhs < INT32_MAX / rhs);
    }

    bool MaybeSubOverflow(const RangeInfo &rhs) const
    {
        return (rhs.min_ < 0) && (max_ > INT32_MAX + rhs.min_);
    }

    bool MaybeSubUnderflow(const RangeInfo &rhs) const
    {
        return (rhs.max_ > 0) && (min_ < INT32_MIN + rhs.max_);
    }

    bool MaybeSubOverflowOrUnderflow(const RangeInfo &rhs) const
    {
        return MaybeSubOverflow(rhs) || MaybeSubUnderflow(rhs);
    }

    RangeInfo operator- (const RangeInfo &rhs) const
    {
        ASSERT(min_ <= max_ && rhs.min_ <= rhs.max_);
        int32_t nmax = MaybeSubOverflow(rhs) ? INT32_MAX : max_ - rhs.min_;
        int32_t nmin = MaybeSubUnderflow(rhs) ? INT32_MIN : min_ - rhs.max_;
        return RangeInfo(nmin, nmax);
    }

    bool MaybeShrOverflow(const RangeInfo &rhs) const
    {
        if (rhs.max_ != rhs.min_) {
            return true;
        }
        return ((static_cast<uint32_t>(rhs.max_) & 0x1f) == 0) && (min_< 0);   // 0x1f : shift bits
    }

    RangeInfo SHR(const RangeInfo &rhs) const
    {
        ASSERT(min_ <= max_);
        ASSERT(rhs.max_ == rhs.min_);
        if (MaybeShrOverflow(rhs)) {
            // assume no overflow occurs since overflow will lead to deopt
            return RangeInfo(0, std::max(0, GetMax()));
        }
        int32_t shift = rhs.max_ & 0x1f;    // 0x1f : shift bits
        uint32_t tempMin = bit_cast<uint32_t>((max_ >= 0) ? std::max(0, min_) : min_);
        uint32_t tempMax = bit_cast<uint32_t>((min_ < 0) ? std::min(-1, max_) : max_);
        int32_t nmin = bit_cast<int32_t>(tempMin >> shift);
        int32_t nmax = bit_cast<int32_t>(tempMax >> shift);
        return RangeInfo(nmin, nmax);
    }

    RangeInfo ASHR(const RangeInfo &rhs) const
    {
        ASSERT(min_ <= max_);
        ASSERT(rhs.max_ == rhs.min_);
        int32_t shift = rhs.max_ & 0x1f;    // 0x1f : shift bits
        int32_t nmin = min_ >> shift;
        int32_t nmax = max_ >> shift;
        return RangeInfo(nmin, nmax);
    }

    bool operator== (const RangeInfo &rhs) const
    {
        return (min_ == rhs.min_) && (max_ == rhs.max_);
    }

    bool operator!= (const RangeInfo &rhs) const
    {
        return (min_ != rhs.min_) || (max_ != rhs.max_);
    }

    bool IsNone() const
    {
        return (min_ == INT32_MAX) && (max_ == INT32_MIN);
    }

private:
    int32_t min_ {INT32_MIN};
    int32_t max_ {INT32_MAX};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NUMBER_GATE_INFO_H
