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

#ifndef ECMASCRIPT_IC_IC_INFO_H
#define ECMASCRIPT_IC_IC_INFO_H

#include <optional>

#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class JSThread;
class ObjectOperator;
class ProfileTypeInfo;
/**
 *  ICInfo — base IC slot storage (extends TaggedArray).
 *  Each callsite uses 2 consecutive slots: e.g. slot[0] and slot[1].
 *
 *  NamedIC / Element IC (property name known at compile time, or element access):
 *  +-------------------+-------------------+
 *  |     slot[0]       |     slot[1]       |
 *  +-------------------+-------------------+
 *  | Undefined         | Undefined         |  UNINIT
 *  | Weak<HClass>      | Handler           |  MONO
 *  | TaggedArray       | Hole              |  POLY  (array of [WHC, Hdl] pairs)
 *  | Hole              | Hole              |  MEGA
 *  | Hole              | String            |  IC_MEGA (key for MegaICCache)
 *  +-------------------+-------------------+
 *
 *  Key IC (computed key, e.g. obj[key]):
 *  +-------------------+-------------------+
 *  |     slot[0]       |     slot[1]       |
 *  +-------------------+-------------------+
 *  | Undefined         | Undefined         |  UNINIT
 *  | String|Symbol     | TaggedArray[2]    |  MONO   (key, [WHC, Hdl])
 *  | String|Symbol     | TaggedArray[4..8] |  POLY   (key, [WHC, Hdl] pairs)
 *  | Hole              | Hole              |  MEGA
 *  | Hole              | String            |  IC_MEGA
 *  +-------------------+-------------------+
 *
 *  NamedGlobalIC (single-slot):      GlobalLoad/StoreIC (single-slot):
 *  +-------------------+             +-------------------+
 *  |     slot[0]       |             |     slot[0]       |
 *  +-------------------+             +-------------------+
 *  | Undefined         |  UNINIT     | Undefined         |  UNINIT
 *  | PropertyBox       |  MONO       | TaggedArray[2]    |  MONO  ([WKey, Hdl])
 *  | Hole              |  MEGA       | TaggedArray[4..8] |  POLY  ([WKey, Hdl] pairs)
 *  +-------------------+             | Hole              |  MEGA
 *                                    +-------------------+
 */
class ICInfo : public TaggedArray {
public:
    static inline JSHandle<ICInfo> SafeCast(JSHandle<JSTaggedValue> handle)
    {
        if (handle.GetTaggedValue().IsHeapObject() && handle->GetTaggedObject()->GetClass()->IsICInfo()) {
            return JSHandle<ICInfo>::Cast(handle);
        }

        return JSHandle<ICInfo>();
    }

    inline JSTaggedValue GetIcSlot(const JSThread *thread, uint32_t idx) const
    {
        return Get(thread, idx);
    }

    inline void SetIcSlot(const JSThread *thread, uint32_t idx, const JSTaggedValue &value)
    {
        Set(thread, idx, value);
    }

    inline uint32_t GetIcSlotLength() const
    {
        return GetLength();
    }

    void SetMultiIcSlotLocked(JSThread *thread, uint32_t firstIdx,
                              const JSTaggedValue &firstValue,
                              uint32_t secondIdx,
                              const JSTaggedValue &secondValue);

    CAST_CHECK(ICInfo, IsICInfo)
    DECL_VISIT_ARRAY(DATA_OFFSET, GetLength(), GetLength());
    DECL_DUMP()
};

// Note: using a get-populated IC for set (or vice versa) would corrupt memory or crash.
// So it needs a tag (slot[2]) that records whether this IC was first populated by a get or set miss. 
class NapiICInfo : public ICInfo {
public:
    static constexpr size_t IC_SLOT_COUNT = 2;
    static constexpr size_t LENGTH = IC_SLOT_COUNT + 1;

    static JSHandle<NapiICInfo> SafeCast(JSHandle<JSTaggedValue> handle)
    {
        if (handle.GetTaggedValue().IsHeapObject() &&
            handle->GetTaggedObject()->GetClass()->GetObjectType() == JSType::IC_INFO &&
            TaggedArray::Cast(handle->GetTaggedObject())->GetLength() == LENGTH) {
            return JSHandle<NapiICInfo>(handle);
        }
        return JSHandle<NapiICInfo>();
    }

    bool CanLoadIC(const JSThread *thread) const
    {
        return !GetIcSlot(thread, 0).IsHole() && !IsSetIC();
    }

    bool CanStoreIC(const JSThread *thread) const
    {
        return !GetIcSlot(thread, 0).IsHole() && !IsGetIC();
    }

    // Scope-free Key IC probe. No allocation, no handles created.
    // Returns loaded value on IC hit, or Hole on miss.
    static JSTaggedValue TryLoadKeyIC(JSThread *thread, JSHandle<NapiICInfo> icInfo,
                                      JSTaggedValue obj, JSTaggedValue key);

    // Full load IC path with key interning and miss handling.
    // May allocate. Returns loaded value, or Hole if IC can't handle.
    static JSTaggedValue TryLoadICOrMiss(JSThread *thread, JSHandle<NapiICInfo> icInfo,
                                         JSHandle<JSTaggedValue> obj,
                                         JSMutableHandle<JSTaggedValue> &key, bool *hit);

    // Full store IC path with key interning and miss handling.
    // May allocate. Returns result (non-Hole = success), or Hole if IC can't handle.
    static JSTaggedValue TryStoreICOrMiss(JSThread *thread, JSHandle<NapiICInfo> icInfo,
                                          JSHandle<JSTaggedValue> obj,
                                          JSMutableHandle<JSTaggedValue> &key,
                                          JSHandle<JSTaggedValue> value, bool *hit);

    CAST_CHECK(NapiICInfo, IsICInfo)

private:
    static constexpr size_t IC_HANDLER_TYPE_INDEX = LENGTH - 1;
    enum ICHandlerType : uint8_t { UNDEFINED = 0, GET = 1, SET = 2 };

    static inline bool IsKeyIC(JSTaggedValue slot0)
    {
        return !slot0.IsWeak() && (slot0.IsString() || slot0.IsSymbol());
    }

    bool IsGetIC() const
    {
        auto handlerType = GetPrimitive(IC_HANDLER_TYPE_INDEX);
        return handlerType.IsInt() && handlerType.GetInt() == ICHandlerType::GET;
    }
    bool IsSetIC() const
    {
        auto handlerType = GetPrimitive(IC_HANDLER_TYPE_INDEX);
        return handlerType.IsInt() && handlerType.GetInt() == ICHandlerType::SET;
    }

    void MarkAsGetIC(const JSThread *thread) { Set(thread, IC_HANDLER_TYPE_INDEX, JSTaggedValue(ICHandlerType::GET)); }
    void MarkAsSetIC(const JSThread *thread) { Set(thread, IC_HANDLER_TYPE_INDEX, JSTaggedValue(ICHandlerType::SET)); }
    void ClearICKind(const JSThread *thread)
    {
        Set(thread, IC_HANDLER_TYPE_INDEX, JSTaggedValue(ICHandlerType::UNDEFINED));
    }
};

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

class IcAccessorLockScope {
public:
    IcAccessorLockScope(JSThread *thread);

private:
    std::optional<LockHolder> lockHolder_;
};

class IcAccessor {
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

    IcAccessor(JSThread* thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind);
    IcAccessor(JSThread* thread, JSHandle<ICInfo> icInfo, uint32_t slotId, ICKind kind);
    ~IcAccessor() = default;
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
    void SetAsMegaIfUndefined() const;
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
    JSHandle<ICInfo> icInfo_;
    uint32_t slotId_;
    ICKind kind_;
    bool enableICMega_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_INFO_H
