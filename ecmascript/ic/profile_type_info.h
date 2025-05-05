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

#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"

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

class ProfileTypeAccessorLockScope {
public:
    ProfileTypeAccessorLockScope(JSThread *thread)
    {
        if (thread->GetEcmaVM()->IsEnableFastJit() || thread->GetEcmaVM()->IsEnableBaselineJit()) {
            lockHolder_.emplace(thread->GetProfileTypeAccessorLock());
        }
    }

private:
    std::optional<LockHolder> lockHolder_;
};

/**
 *              ProfileTypeInfo
 *      +--------------------------------+
 *      |           ic slot              |
 *      |            .....               |
 *      +--------------------------------+
 *      |        64 bits jit osr         |
 *      |      tagged array address      |
 *      +--------------------------------+
 *      |    low 32 bits(PeriodCount)    |
 *      |    hight 32 bits(jit hotness)  |
 *      +--------------------------------+
 *      |    low 32 bits(osr hotness)    |
 *      | hight 32 bits(baseline hotness)|
 *      +--------------------------------+
 */
class ProfileTypeInfo : public TaggedArray {
public:
    static const uint32_t MAX_FUNC_CACHE_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t INVALID_SLOT_INDEX = 0xFF;
    static constexpr uint32_t MAX_SLOT_INDEX = 0xFFFF;
    static constexpr size_t BIT_FIELD_INDEX = 2;
    static constexpr size_t JIT_OSR_INDEX = 3;
    static constexpr size_t EXTRA_INFO_MAP_INDEX = 4;
    static constexpr size_t RESERVED_LENGTH = EXTRA_INFO_MAP_INDEX;
    static constexpr size_t INITIAL_PERIOD_INDEX = 0;
    static constexpr size_t PRE_DUMP_PERIOD_INDEX = 1;
    static constexpr size_t DUMP_PERIOD_INDEX = 2;
    static constexpr size_t BIG_METHOD_PERIOD_INDEX = 3;
    static constexpr size_t INITIAL_OSR_HOTNESS_THRESHOLD = 0;
    static constexpr size_t INITIAL_OSR_HOTNESS_CNT = 0;
    static constexpr size_t INITIAL_JIT_CALL_THRESHOLD = 0;
    static constexpr size_t INITIAL_JIT_CALL_CNT = 0;
    static constexpr uint16_t JIT_DISABLE_FLAG = 0xFFFF;
    static constexpr size_t JIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD = 4;  // 4 : 4 byte offset from bitfield
    static constexpr size_t JIT_CNT_OFFSET_FROM_THRESHOLD = 2;  // 2 : 2 byte offset from jit hotness threshold
    static constexpr size_t OSR_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD = 8;  // 8 : 8 byte offset from bitfield
    static constexpr size_t OSR_CNT_OFFSET_FROM_OSR_THRESHOLD = 2;  // 2 : 2 byte offset from osr hotness threshold
    static constexpr size_t BASELINEJIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD = 12; // 12: bytes offset from bitfield
    static constexpr size_t JIT_CALL_CNT_OFFSET_FROM_BITFIELD = 14;  // 14 : 14 byte offset from bitfield

    static ProfileTypeInfo *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<ProfileTypeInfo *>(object);
    }

    static size_t ComputeSize(uint32_t icSlotSize)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), AdjustSlotSize(icSlotSize) + RESERVED_LENGTH);
    }

    inline uint32_t GetIcSlotLength() const
    {
        return GetLength() - RESERVED_LENGTH;
    }

    inline uint32_t GetIcSlotToOsrLength() const
    {
        return GetLength() - BIT_FIELD_INDEX;
    }

    static inline uint32_t AdjustSlotSize(uint32_t icSlotSize)
    {
        // if ic slot size is 0xff comes from frontend, it means the actual size is 0x100.
        // 0xff is a invalid slot index, which value is hole.
        if (icSlotSize == INVALID_SLOT_INDEX) {
            ++icSlotSize;
        }
        return icSlotSize;
    }

    inline void SetPrimitiveOfSlot(JSTaggedValue initValue, uint32_t icSlotSize)
    {
        for (uint32_t i = 0; i < icSlotSize; i++) {
            size_t offset = JSTaggedValue::TaggedTypeSize() * i;
            if (i == INVALID_SLOT_INDEX) {
                Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, JSTaggedValue::Hole().GetRawData());
            } else {
                Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, initValue.GetRawData());
            }
        }
    }

    inline void SetSpecialValue()
    {
        SetPeriodIndex(INITIAL_PERIOD_INDEX);
        SetJitHotnessThreshold(JIT_DISABLE_FLAG);
        SetJitHotnessCnt(0);
        SetBaselineJitHotnessThreshold(JIT_DISABLE_FLAG);
        SetOsrHotnessThreshold(INITIAL_OSR_HOTNESS_THRESHOLD);
        SetOsrHotnessCnt(INITIAL_OSR_HOTNESS_CNT);
        SetJitCallThreshold(INITIAL_JIT_CALL_THRESHOLD);
    }

    inline void InitializeExtraInfoMap()
    {
        // the last-1 of the cache is used to save extra info map
        Barriers::SetPrimitive<JSTaggedType>(
            GetData(), GetIcSlotLength() * JSTaggedValue::TaggedTypeSize(),
            JSTaggedValue::Undefined().GetRawData());
    }

    inline void InitializeJitOsr()
    {
        // the last of the cache is used to save osr jit tagged array
        Barriers::SetPrimitive<JSTaggedType>(
            GetData(), (GetIcSlotLength() + 1) * JSTaggedValue::TaggedTypeSize(),
            JSTaggedValue::Undefined().GetRawData());
    }

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t icSlotSize, uint32_t extraLength = 0)
    {
        ASSERT(initValue.IsSpecial());
        icSlotSize = AdjustSlotSize(icSlotSize);
        SetLength(icSlotSize + RESERVED_LENGTH);
        SetExtraLength(extraLength);
        SetPrimitiveOfSlot(initValue, icSlotSize);
        InitializeExtraInfoMap();
        InitializeJitOsr();
        SetSpecialValue();
    }

    void SetPreDumpPeriodIndex()
    {
        SetPeriodIndex(PRE_DUMP_PERIOD_INDEX);
    }

    bool IsProfileTypeInfoPreDumped() const
    {
        return GetPeriodIndex() == PRE_DUMP_PERIOD_INDEX;
    }

    void SetBigMethodPeriodIndex()
    {
        SetPeriodIndex(BIG_METHOD_PERIOD_INDEX);
    }

    bool IsProfileTypeInfoWithBigMethod() const
    {
        return GetPeriodIndex() == BIG_METHOD_PERIOD_INDEX;
    }

    JSTaggedValue GetExtraInfoMap() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetExtraInfoMapOffset()));
    }

    void SetExtraInfoMap(const JSThread *thread, JSHandle<NumberDictionary> extraInfoMap)
    {
        Barriers::SetObject<true>(thread, GetData(), GetExtraInfoMapOffset(),
                                  extraInfoMap.GetTaggedValue().GetRawData());
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
        return Barriers::GetValue<uint16_t>(GetData(), GetBaselineJitHotnessThresholdBitfieldOffset());
    }

    void SetBaselineJitHotnessThreshold(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetBaselineJitHotnessThresholdBitfieldOffset(), count);
    }

    void SetJitCallThreshold(uint16_t count)
    {
        Barriers::SetPrimitive(GetData(), GetJitCallCntBitfieldOffset(), count);
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

    inline JSTaggedValue GetIcSlot(uint32_t idx) const
    {
        ASSERT(idx < GetIcSlotLength());
        return TaggedArray::Get(idx);
    }

    inline void SetIcSlot(const JSThread* thread, uint32_t idx, const JSTaggedValue& value)
    {
        ASSERT(idx < GetIcSlotLength());
        TaggedArray::Set(thread, idx, value);
    }

    inline void SetMultiIcSlotLocked(JSThread* thread, uint32_t firstIdx, const JSTaggedValue& firstValue,
        uint32_t secondIdx, const JSTaggedValue& secondValue)
    {
        ProfileTypeAccessorLockScope accessorLockScope(thread);
        ASSERT(firstIdx < GetIcSlotLength());
        ASSERT(secondIdx < GetIcSlotLength());
        TaggedArray::Set(thread, firstIdx, firstValue);
        TaggedArray::Set(thread, secondIdx, secondValue);
    }

    static JSHandle<NumberDictionary> CreateOrGetExtraInfoMap(const JSThread *thread,
                                                              JSHandle<ProfileTypeInfo> profileTypeInfo)
    {
        if (profileTypeInfo->GetExtraInfoMap().IsUndefined()) {
            JSHandle<NumberDictionary> dictJShandle = NumberDictionary::Create(thread);
            profileTypeInfo->SetExtraInfoMap(thread, dictJShandle);
            return dictJShandle;
        }
        JSHandle<NumberDictionary> dictJShandle(thread, profileTypeInfo->GetExtraInfoMap());
        return dictJShandle;
    }

    static void UpdateExtraInfoMap(const JSThread *thread, JSHandle<NumberDictionary> dictJShandle,
                            JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> receiverHClassHandle,
                            JSHandle<JSTaggedValue> holderHClassHandle,
                            JSHandle<ProfileTypeInfo> profileTypeInfo)
    {
        JSHandle<JSTaggedValue> info(thread->GetEcmaVM()->GetFactory()->NewExtraProfileTypeInfo());
        JSHandle<ExtraProfileTypeInfo> infoHandle(info);
        infoHandle->SetReceiver(thread, receiverHClassHandle.GetTaggedValue().CreateAndGetWeakRef());
        infoHandle->SetHolder(thread, holderHClassHandle.GetTaggedValue());
        JSHandle<NumberDictionary> dict = NumberDictionary::PutIfAbsent(thread,
                                                                        dictJShandle,
                                                                        key,
                                                                        info,
                                                                        PropertyAttributes::Default());
        profileTypeInfo->SetExtraInfoMap(thread, dict);
    }

    DECL_VISIT_ARRAY(DATA_OFFSET, GetIcSlotToOsrLength(), GetIcSlotToOsrLength());

    DECL_DUMP()

private:
    uint32_t GetPeriodIndex() const
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

    inline size_t GetExtraInfoMapOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - EXTRA_INFO_MAP_INDEX);
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
    inline size_t GetBaselineJitHotnessThresholdBitfieldOffset() const
    {
        return GetBitfieldOffset() + BASELINEJIT_HOTNESS_THRESHOLD_OFFSET_FROM_BITFIELD;
    }

    inline size_t GetOsrHotnessCntBitfieldOffset() const
    {
        return GetOsrHotnessThresholdBitfieldOffset() + OSR_CNT_OFFSET_FROM_OSR_THRESHOLD;
    }

    // jit call count(16bits)
    inline size_t GetJitCallCntBitfieldOffset() const
    {
        return GetBitfieldOffset() + JIT_CALL_CNT_OFFSET_FROM_BITFIELD;
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
        IC_MEGA,
        MEGA,
    };

#if ECMASCRIPT_ENABLE_TRACE_LOAD
    enum MegaState {
        NONE,
        NOTFOUND_MEGA,
        DICT_MEGA,
    };
#endif

    ProfileTypeAccessor(JSThread* thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
        : thread_(thread), profileTypeInfo_(profileTypeInfo), slotId_(slotId), kind_(kind)
    {
        enableICMega_ = thread_->GetEcmaVM()->GetJSOptions().IsEnableMegaIC();
    }
    ~ProfileTypeAccessor() = default;
    ICState GetMegaState() const;
    ICState GetICState() const;
    static std::string ICStateToString(ICState state);
    void AddHandlerWithoutKey(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler,
                              JSHandle<JSTaggedValue> keyForMegaIC = JSHandle<JSTaggedValue>(),
                              MegaICCache::MegaICKind kind = MegaICCache::MegaICKind::None) const;
    void AddWithoutKeyPoly(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler, uint32_t index,
                           JSTaggedValue profileData, JSHandle<JSTaggedValue> keyForMegaIC = JSHandle<JSTaggedValue>(),
                           MegaICCache::MegaICKind kind = MegaICCache::MegaICKind::None) const;

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
    void SetAsMegaForTraceSlowMode(ObjectOperator& op) const;
    void SetAsMegaForTrace(JSTaggedValue value) const;

    ICKind GetKind() const
    {
        return kind_;
    }

    uint32_t GetSlotId() const
    {
        return slotId_;
    }

private:
    JSThread* thread_;
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    uint32_t slotId_;
    ICKind kind_;
    bool enableICMega_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_PROFILE_TYPE_INFO_H
