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

#ifndef ECMASCRIPT_PROPERTY_ATTRIBUTES_H
#define ECMASCRIPT_PROPERTY_ATTRIBUTES_H

#include "ecmascript/js_tagged_value.h"

#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
class PropertyDescriptor;

enum class Representation {
    NONE,
    INT,
    DOUBLE,
    TAGGED,
};

enum class TrackType : uint8_t {
    NONE = 0x0ULL,
    INT = 0x1ULL,
    DOUBLE = 0x1ULL << 1,
    NUMBER = INT | DOUBLE,
    TAGGED = 0x1ULL << 2
};
enum class SharedFieldType {
    NONE = 0,
    NUMBER = (1 << 0),
    STRING = (1 << 1),
    BOOLEAN = (1 << 2),
    SENDABLE = (1 << 3),
    BIG_INT = (1 << 4),
    GENERIC = (1 << 5),
    NULL_TYPE = (1 << 6),
    UNDEFINED = (1 << 7),
};

enum class PropertyBoxType {
    // Meaningful when a property cell does not contain the hole.
    UNDEFINED,     // The PREMONOMORPHIC of property cells.
    CONSTANT,      // Cell has been assigned only once.
    CONSTANTTYPE,  // Cell has been assigned only one type.
    MUTABLE,       // Cell will no longer be tracked as constant.

    // Meaningful when a property cell contains the hole.
    UNINITIALIZED = UNDEFINED,  // Cell has never been initialized.
    INVALIDATED = CONSTANT,     // Cell has been deleted, invalidated or never existed.
};

/**
 * [bitfield]
 *  Common | WritableField (bit 1)
 *         | EnumerableField (bit 2)
 *         | ConfigurableField (bit 3)
 *         | IsAccessorField (bit 4)
 *         | IsInlinedPropsField(bit 5)
 *         | RepresentationField(bit 6...7)
 *         --------------------------------
 *    Fast | OffsetField(bit 8...17)
 *         | TrackTypeField(bit 18...20)
 *         | SharedFieldTypeField(bit 21...28)
 *         | SortedIndexField(bit 29...38)
 *         | IsConstPropsField(bit 39)
 *         | IsNotHoleField(bit 40)
 *         -----------------------------
 *    Slow | PropertyBoxTypeField(bit 8...9)
 *         | DictionaryOrderField(bit 10...29)
 *         | SharedFieldTypeField(bit 30...37)
 */
class PropertyAttributes {
public:
    PropertyAttributes() = default;
    ~PropertyAttributes() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PropertyAttributes);
    DEFAULT_COPY_SEMANTIC(PropertyAttributes);

    explicit PropertyAttributes(uint64_t v) : value_(v) {}
    explicit PropertyAttributes(JSTaggedValue v) : value_(JSTaggedValue::UnwrapToUint64(v)) {}
    explicit PropertyAttributes(const PropertyDescriptor &desc);

    static constexpr uint32_t DICTIONARY_ORDER_NUM = 20;
    static constexpr uint32_t MAX_FAST_PROPS_CAPACITY_LOG2 = 10;
    static constexpr uint32_t REPRESENTATION_NUM = 2;
    static constexpr uint32_t TRACK_TYPE_NUM = 3;
    static constexpr uint32_t FIELD_TYPE_NUM = 8;
    static constexpr uint32_t MAX_FAST_PROPS_CAPACITY = (1U << MAX_FAST_PROPS_CAPACITY_LOG2) - 1;
    static constexpr uint32_t MAX_LITERAL_HCLASS_CACHE_SIZE = 63;
    static constexpr uint32_t MAX_LITERAL_HCLASS_CACHE_BIT = 6;
    static constexpr unsigned BITS_PER_BYTE = 8;

    static constexpr uint32_t MAX_BIT_SIZE = 48;
    static constexpr int INITIAL_PROPERTY_INDEX = 0;

    using PropertyMetaDataField = BitField<int32_t, 0, 4>;  // 4: property metaData field occupies 4 bits
    using AttributesField = BitField<int32_t, 0, 4>;        // 4: attributes field occupies 4 bits
    using DefaultAttributesField = BitField<int32_t, 0, 3>; // 3: default attributes field occupies 3 bits
    using WritableField = BitField<bool, 0, 1>;         // 1: writable field occupies 1 bits
    using EnumerableField = WritableField::NextFlag;
    using ConfigurableField = EnumerableField::NextFlag;
    using IsAccessorField = ConfigurableField::NextFlag; // 4

    using IsInlinedPropsField = PropertyMetaDataField::NextFlag;                                    // 5
    using RepresentationField = IsInlinedPropsField::NextField<Representation, REPRESENTATION_NUM>; // 2: 2 bits, 6-7
    using CommonLastBitField = RepresentationField;
    // For flags required for both fast mode and slow mode, need to be added before CommonLastBitField

    // ---------------------------------------------------------------------------------------------
    // only for fast mode
    using FastModeStartField = CommonLastBitField;
    static_assert(FastModeStartField::START_BIT == CommonLastBitField::START_BIT);
    static_assert(FastModeStartField::SIZE == CommonLastBitField::SIZE);
    using OffsetField = FastModeStartField::NextField<uint32_t, MAX_FAST_PROPS_CAPACITY_LOG2>; // 17
    using TrackTypeField = OffsetField::NextField<TrackType, TRACK_TYPE_NUM>;     // 20: 3 bits
    static_assert(TrackTypeField::END_BIT <= sizeof(uint32_t) * BITS_PER_BYTE, "Invalid");

    // normal attr should include SharedFieldTypeField when set to layout
    static constexpr uint32_t NORMAL_ATTR_BITS = 28;
    using NormalAttrField = BitField<uint32_t, 0, NORMAL_ATTR_BITS>;
    using SharedFieldTypeField = TrackTypeField::NextField<SharedFieldType, FIELD_TYPE_NUM>; // 28: 8 bits
    using SortedIndexField = SharedFieldTypeField::NextField<uint32_t, MAX_FAST_PROPS_CAPACITY_LOG2>; // 38: 10 bits
    using IsConstPropsField = SortedIndexField::NextFlag;                              // 39
    using IsNotHoleField = IsConstPropsField::NextFlag;                                // 40
    using IsPGODumpedField = IsNotHoleField::NextFlag;                                      // 41
    using FastModeLastField = IsPGODumpedField;
    static_assert(
        FastModeLastField::START_BIT + FastModeLastField::SIZE <= MAX_BIT_SIZE, "Invalid");

    // ---------------------------------------------------------------------------------------------
    // only for dictionary mode, include global
    using DictModeStartField = CommonLastBitField;
    static_assert(DictModeStartField::START_BIT == CommonLastBitField::START_BIT);
    static_assert(DictModeStartField::SIZE == CommonLastBitField::SIZE);
    using PropertyBoxTypeField = DictModeStartField::NextField<PropertyBoxType, 2>;               // 2: 2 bits, 8-9
    using DictionaryOrderField = PropertyBoxTypeField::NextField<uint32_t, DICTIONARY_ORDER_NUM>; // 29
    using DictSharedFieldTypeField = DictionaryOrderField::NextField<SharedFieldType, FIELD_TYPE_NUM>;
    using DictModeLastField = DictSharedFieldTypeField;

    static_assert(
        DictModeLastField::START_BIT + DictModeLastField::SIZE <= MAX_BIT_SIZE, "Invalid");

    inline int32_t GetPropertyMetaData() const
    {
        return PropertyMetaDataField::Get(value_);
    }

    static PropertyAttributes Default()
    {
        return PropertyAttributes(GetDefaultAttributes());
    }

    static PropertyAttributes Default(bool w, bool e, bool c, bool isAccessor = false)
    {
        uint64_t value = WritableField::Encode(w) | EnumerableField::Encode(e) | ConfigurableField::Encode(c) |
                         IsAccessorField::Encode(isAccessor);
        return PropertyAttributes(value);
    }

    static PropertyAttributes DefaultAccessor(bool w, bool e, bool c)
    {
        uint64_t value = WritableField::Encode(w) | EnumerableField::Encode(e) | ConfigurableField::Encode(c) |
                         IsAccessorField::Encode(true);
        return PropertyAttributes(value);
    }

    inline void SetDefaultAttributes()
    {
        AttributesField::Set<uint64_t>(DefaultAttributesField::Mask(), &value_);
    }

    static inline int32_t GetDefaultAttributes()
    {
        return DefaultAttributesField::Mask();
    }
    // JSShared should not update tracktype.
    bool UpdateTrackType(JSTaggedValue value)
    {
        TrackType oldType = GetTrackType();
        if (oldType == TrackType::TAGGED) {
            return false;
        }

        TrackType newType = TrackType::TAGGED;
        if (value.IsInt()) {
            newType = static_cast<TrackType>(static_cast<uint8_t>(TrackType::INT) | static_cast<uint8_t>(oldType));
        } else if (value.IsDouble()) {
            newType = static_cast<TrackType>(static_cast<uint8_t>(TrackType::DOUBLE) | static_cast<uint8_t>(oldType));
        }

        if (oldType != newType) {
            SetTrackType(newType);
            return true;
        }
        return false;
    }

    inline bool IsDefaultAttributes() const
    {
        return AttributesField::Get(value_) == static_cast<int32_t>(DefaultAttributesField::Mask());
    }

    inline void SetNoneAttributes()
    {
        AttributesField::Set<uint64_t>(0U, &value_);
    }

    inline bool IsNoneAttributes() const
    {
        return AttributesField::Get(value_) == 0;
    }

    inline void SetWritable(bool flag)
    {
        WritableField::Set<uint64_t>(flag, &value_);
    }
    inline bool IsWritable() const
    {
        return WritableField::Get(value_);
    }
    inline void SetEnumerable(bool flag)
    {
        EnumerableField::Set<uint64_t>(flag, &value_);
    }
    inline bool IsEnumerable() const
    {
        return EnumerableField::Get(value_);
    }
    inline void SetConfigurable(bool flag)
    {
        ConfigurableField::Set<uint64_t>(flag, &value_);
    }
    inline bool IsConfigurable() const
    {
        return ConfigurableField::Get(value_);
    }

    inline void SetIsAccessor(bool flag)
    {
        IsAccessorField::Set<uint64_t>(flag, &value_);
    }

    inline bool IsAccessor() const
    {
        return IsAccessorField::Get(value_);
    }

    inline void SetIsInlinedProps(bool flag)
    {
        IsInlinedPropsField::Set<uint64_t>(flag, &value_);
    }

    inline bool IsInlinedProps() const
    {
        return IsInlinedPropsField::Get(value_);
    }

    inline void SetIsConstProps(bool flag)
    {
        IsConstPropsField::Set<uint64_t>(flag, &value_);
    }

    inline bool IsConstProps() const
    {
        return IsConstPropsField::Get(value_);
    }

    inline void SetIsNotHole(bool flag)
    {
        IsNotHoleField::Set<uint64_t>(flag, &value_);
    }

    inline bool IsNotHole() const
    {
        return IsNotHoleField::Get(value_);
    }

    inline bool IsTaggedRep() const
    {
        return !IsDoubleRep() && !IsIntRep();
    }

    inline bool IsDoubleRep() const
    {
        auto rep = GetRepresentation();
        return rep == Representation::DOUBLE;
    }

    inline bool IsIntRep() const
    {
        auto rep = GetRepresentation();
        return rep == Representation::INT;
    }

    inline void SetRepresentation(Representation representation)
    {
        RepresentationField::Set<uint64_t>(representation, &value_);
    }

    inline Representation GetRepresentation() const
    {
        return RepresentationField::Get(value_);
    }

    inline TrackType GetTrackType() const
    {
        return TrackTypeField::Get(value_);
    }

    inline void SetTrackType(TrackType type)
    {
        TrackTypeField::Set<uint64_t>(type, &value_);
    }

    inline bool IsPGODumped()
    {
        return IsPGODumpedField::Get(value_);
    }

    inline void SetIsPGODumped(bool isDumped)
    {
        IsPGODumpedField::Set<uint64_t>(isDumped, &value_);
    }

    inline SharedFieldType GetSharedFieldType() const
    {
        return SharedFieldTypeField::Get(value_);
    }

    inline void SetSharedFieldType(SharedFieldType fieldType)
    {
        SharedFieldTypeField::Set<uint64_t>(fieldType, &value_);
    }

    inline void SetDictSharedFieldType(SharedFieldType fieldType)
    {
        DictSharedFieldTypeField::Set<uint64_t>(fieldType, &value_);
    }

    inline SharedFieldType GetDictSharedFieldType() const
    {
        return DictSharedFieldTypeField::Get(value_);
    }

    inline void SetDictionaryOrder(uint32_t order)
    {
        DictionaryOrderField::Set<uint64_t>(order, &value_);
    }
    inline uint32_t GetDictionaryOrder() const
    {
        return DictionaryOrderField::Get(value_);
    }

    inline void SetOffset(uint32_t offset)
    {
        OffsetField::Set<uint64_t>(offset, &value_);
    }

    inline uint32_t GetOffset() const
    {
        return OffsetField::Get(value_);
    }

    inline void SetSortedIndex(uint32_t sortedIndex)
    {
        SortedIndexField::Set<uint64_t>(sortedIndex, &value_);
    }
    inline uint32_t GetSortedIndex() const
    {
        return SortedIndexField::Get(value_);
    }

    inline void SetNormalAttr(uint32_t normalAttr)
    {
        NormalAttrField::Set<uint64_t>(normalAttr, &value_);
    }

    inline uint32_t GetNormalAttr() const
    {
        return NormalAttrField::Get(value_);
    }

    inline JSTaggedValue GetNormalTagged() const
    {
        return JSTaggedValue::WrapUint64(GetNormalAttr());
    }

    inline uint64_t GetValue() const
    {
        return value_;
    }

    inline void SetBoxType(PropertyBoxType cellType)
    {
        PropertyBoxTypeField::Set<uint64_t>(cellType, &value_);
    }

    inline PropertyBoxType GetBoxType() const
    {
        return PropertyBoxTypeField::Get(value_);
    }

    static inline Representation TranslateToRep(JSTaggedValue value)
    {
        if (value.IsInt()) {
            return Representation::INT;
        } else if (value.IsDouble()) {
            return Representation::DOUBLE;
        }
        return Representation::TAGGED;
    }

    inline static bool IsValidIndex(int index)
    {
        return DictionaryOrderField::IsValid(index);
    }

    inline JSTaggedValue GetTaggedValue() const
    {
        return JSTaggedValue::WrapUint64(value_);
    }

private:
    uint64_t value_{0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PROPERTY_ATTRIBUTES_H
