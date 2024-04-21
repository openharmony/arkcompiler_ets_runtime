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

#ifndef ECMASCRIPT_IC_PROFILE_TYPE_INFO_H
#define ECMASCRIPT_IC_PROFILE_TYPE_INFO_H

#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
enum class ICKind : uint32_t {
    NamedLoadIC,
    NamedStoreIC,
    LoadIC,
    StoreIC,
    NamedGlobalLoadIC,
    NamedGlobalStoreIC,
    NamedGlobalTryLoadIC,
    NamedGlobalTryStoreIC,
    GlobalLoadIC,
    GlobalStoreIC,
};

static inline bool IsNamedGlobalIC(ICKind kind)
{
    return (kind == ICKind::NamedGlobalLoadIC) || (kind == ICKind::NamedGlobalStoreIC) ||
           (kind == ICKind::NamedGlobalTryLoadIC) || (kind == ICKind::NamedGlobalTryStoreIC);
}

static inline bool IsValueGlobalIC(ICKind kind)
{
    return (kind == ICKind::GlobalLoadIC) || (kind == ICKind::GlobalStoreIC);
}

static inline bool IsValueNormalIC(ICKind kind)
{
    return (kind == ICKind::LoadIC) || (kind == ICKind::StoreIC);
}

static inline bool IsValueIC(ICKind kind)
{
    return IsValueNormalIC(kind) || IsValueGlobalIC(kind);
}

static inline bool IsNamedNormalIC(ICKind kind)
{
    return (kind == ICKind::NamedLoadIC) || (kind == ICKind::NamedStoreIC);
}

static inline bool IsNamedIC(ICKind kind)
{
    return IsNamedNormalIC(kind) || IsNamedGlobalIC(kind);
}

static inline bool IsGlobalLoadIC(ICKind kind)
{
    return (kind == ICKind::NamedGlobalLoadIC) || (kind == ICKind::GlobalLoadIC) ||
           (kind == ICKind::NamedGlobalTryLoadIC);
}

static inline bool IsGlobalStoreIC(ICKind kind)
{
    return (kind == ICKind::NamedGlobalStoreIC) || (kind == ICKind::GlobalStoreIC) ||
           (kind == ICKind::NamedGlobalTryStoreIC);
}

static inline bool IsGlobalIC(ICKind kind)
{
    return IsValueGlobalIC(kind) || IsNamedGlobalIC(kind);
}

std::string ICKindToString(ICKind kind);

/*                  ProfileTypeInfo
 *      +--------------------------------+----
 *      |            cache               |
 *      |            .....               |
 *      +--------------------------------+----
 *      |    low 32bits(PeriodCount)     |
 *      |    hight 32bits(jit hotness)   |
 *      +--------------------------------+
 *      |    low 32bits(osr hotness)     |
 *      | hight 32bits(baseline hotness) |
 *      +--------------------------------+
 */
class ProfileTypeInfo : public TaggedArray {
public:
    static const uint32_t MAX_FUNC_CACHE_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t INVALID_SLOT_INDEX = 0xFF;
    static constexpr uint32_t MAX_SLOT_INDEX = 0xFFFF;
    static constexpr size_t BIT_FIELD_INDEX = 2;
    static constexpr size_t RESERVED_LENGTH = BIT_FIELD_INDEX;
    // 1 : one more slot for registering osr jit code array
    static constexpr size_t EXTRA_CACHE_SLOT_INDEX = RESERVED_LENGTH + 1;
    static constexpr size_t INITIAL_PEROID_INDEX = 0;
    static constexpr size_t INITIAL_OSR_HOTNESS_THRESHOLD = 0;
    static constexpr size_t INITIAL_OSR_HOTNESS_CNT = 0;
    static constexpr uint16_t JIT_DISABLE_FLAG = 0xFFFF;
    static constexpr size_t PRE_DUMP_PEROID_INDEX = 1;
    static constexpr size_t DUMP_PEROID_INDEX = 2;
    static constexpr size_t JIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD = 4;  // 4 : 4 byte offset from bitfield
    static constexpr size_t JIT_CNT_OFFSET_FROM_THRESHOLD = 2;  // 2 : 2 byte offset from jit hotness threshold
    static constexpr size_t OSR_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD = 8;  // 8 : 8 byte offset from bitfield
    static constexpr size_t OSR_CNT_OFFSET_FROM_OSR_THRESHOLD = 2;  // 2 : 2 byte offset from osr hotness threshold
    static constexpr size_t BASELINEJIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD = 12; // 12: bytes offset from bitfield

    static ProfileTypeInfo *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<ProfileTypeInfo *>(object);
    }

    static size_t ComputeSize(uint32_t cacheSize)
    {
        if (cacheSize == INVALID_SLOT_INDEX) {
            // used as hole.
            ++cacheSize;
        }
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), cacheSize + EXTRA_CACHE_SLOT_INDEX);
    }

    inline uint32_t GetCacheLength() const
    {
        return GetLength() - RESERVED_LENGTH;
    }

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t capacity, uint32_t extraLength = 0)
    {
        ASSERT(initValue.IsSpecial());
        bool needHole {false};
        if (capacity == INVALID_SLOT_INDEX) {
            // used as hole.
            ++capacity;
            needHole = true;
        }
        SetLength(capacity + EXTRA_CACHE_SLOT_INDEX);
        SetExtraLength(extraLength);
        for (uint32_t i = 0; i < capacity; i++) {
            size_t offset = JSTaggedValue::TaggedTypeSize() * i;
            Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, initValue.GetRawData());
        }
        if (needHole) {
            Barriers::SetPrimitive<JSTaggedType>(GetData(), INVALID_SLOT_INDEX * JSTaggedValue::TaggedTypeSize(),
                                                 JSTaggedValue::Hole().GetRawData());
        }
        // the last of the cache is used to save osr jit code array
        Barriers::SetPrimitive<JSTaggedType>(GetData(), capacity * JSTaggedValue::TaggedTypeSize(),
                                             JSTaggedValue::Undefined().GetRawData());
        SetPeriodIndex(INITIAL_PEROID_INDEX);
        SetJitHotnessThreshold(JIT_DISABLE_FLAG);
        SetOsrHotnessThreshold(INITIAL_OSR_HOTNESS_THRESHOLD);
        SetOsrHotnessCnt(INITIAL_OSR_HOTNESS_CNT);
    }

    void SetPreDumpPeriodIndex()
    {
        SetPeriodIndex(PRE_DUMP_PEROID_INDEX);
    }

    bool IsProfileTypeInfoPreDumped() const
    {
        return GetPeroidIndex() == PRE_DUMP_PEROID_INDEX;
    }

    uint16_t GetJitHotnessThreshold() const
    {
        return Barriers::GetValue<uint16_t>(GetData(), GetJitHotnessThresholdBitfieldOffset());
    }

    void SetJitHotnessThreshold(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetJitHotnessThresholdBitfieldOffset(), count);
    }

    uint16_t GetOsrHotnessThreshold() const
    {
        return Barriers::GetValue<uint16_t>(GetData(), GetOsrHotnessThresholdBitfieldOffset());
    }

    void SetOsrHotnessThreshold(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetOsrHotnessThresholdBitfieldOffset(), count);
    }

    uint16_t GetBaselineJitHotnessThreshold() const
    {
        return Barriers::GetValue<uint16_t>(GetData(), GetBaselineJitHotnessThresholdBitfiledOffset());
    }

    void SetBaselineJitHotnessThreshold(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetBaselineJitHotnessThresholdBitfiledOffset(), count);
    }

    uint16_t GetJitHotnessCnt() const
    {
        return Barriers::GetValue<uint16_t>(GetData(), GetJitHotnessCntBitfieldOffset());
    }

    void SetJitHotnessCnt(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetJitHotnessCntBitfieldOffset(), count);
    }

    void SetOsrHotnessCnt(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetOsrHotnessCntBitfieldOffset(), count);
    }

    DECL_VISIT_ARRAY(DATA_OFFSET, GetCacheLength(), GetCacheLength());

    DECL_DUMP()

private:
    uint32_t GetPeroidIndex() const
    {
        return Barriers::GetValue<uint32_t>(GetData(), GetBitfieldOffset());
    }

    void SetPeriodIndex(uint32_t count)
    {
        Barriers::SetPrimitive(GetData(), GetBitfieldOffset(), count);
    }

    inline size_t GetBitfieldOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - BIT_FIELD_INDEX);
    }

    // jit hotness(16bits) + count(16bits)
    inline size_t GetJitHotnessThresholdBitfieldOffset() const
    {
        return GetBitfieldOffset() + JIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD;
    }

    inline size_t GetJitHotnessCntBitfieldOffset() const
    {
        return GetJitHotnessThresholdBitfieldOffset() + JIT_CNT_OFFSET_FROM_THRESHOLD;
    }

    // osr hotness(16bits) + count(16bits)
    inline size_t GetOsrHotnessThresholdBitfieldOffset() const
    {
        return GetBitfieldOffset() + OSR_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD;
    }

    // baselinejit hotness(16bits)
    inline size_t GetBaselineJitHotnessThresholdBitfiledOffset() const
    {
        return GetBitfieldOffset() + BASELINEJIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD;
    }

    inline size_t GetOsrHotnessCntBitfieldOffset() const
    {
        return GetOsrHotnessThresholdBitfieldOffset() + OSR_CNT_OFFSET_FROM_OSR_THRESHOLD;
    }
};

class ProfileTypeAccessor {
public:
    static constexpr size_t CACHE_MAX_LEN = 8;
    static constexpr size_t MONO_CASE_NUM = 2;
    static constexpr size_t POLY_CASE_NUM = 4;

    enum ICState {
        UNINIT,
        MONO,
        POLY,
        MEGA,
    };

    ProfileTypeAccessor(JSThread* thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
        : thread_(thread), profileTypeInfo_(profileTypeInfo), slotId_(slotId), kind_(kind)
    {
    }
    ~ProfileTypeAccessor() = default;

    ICState GetICState() const;
    static std::string ICStateToString(ICState state);
    void AddHandlerWithoutKey(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler) const;
    void AddElementHandler(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler) const;
    void AddHandlerWithKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> hclass,
                           JSHandle<JSTaggedValue> handler) const;
    void AddGlobalHandlerKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> handler) const;
    void AddGlobalRecordHandler(JSHandle<JSTaggedValue> handler) const;

    JSTaggedValue GetWeakRef(JSTaggedValue value) const
    {
        return JSTaggedValue(value.CreateAndGetWeakRef());
    }

    JSTaggedValue GetRefFromWeak(const JSTaggedValue &value) const
    {
        return JSTaggedValue(value.GetWeakReferent());
    }
    void SetAsMega() const;

    ICKind GetKind() const
    {
        return kind_;
    }

    bool IsICSlotValid() const
    {
        return slotId_ + 1 < profileTypeInfo_->GetLength(); // slotId_ + 1 need to be valid
    }

private:
    JSThread* thread_;
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    uint32_t slotId_;
    ICKind kind_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_PROFILE_TYPE_INFO_H
