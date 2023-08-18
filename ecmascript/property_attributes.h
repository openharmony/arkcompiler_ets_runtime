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
 *         | SortedIndexField(bit 21...30)
 *         | IsConstPropsField(bit 31)
 *         | IsNotHoleField(bit 32)
 *         -----------------------------
 *    Slow | PropertyBoxTypeField(bit 8...9)
 *         | DictionaryOrderField(bit 10...29)
 */
class PropertyAttributes {
public:
    PropertyAttributes() = default;
    ~PropertyAttributes() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PropertyAttributes);
    DEFAULT_COPY_SEMANTIC(PropertyAttributes);

    explicit PropertyAttributes(uint32_t v) : value_(v) {}
    explicit PropertyAttributes(int32_t v) : value_(static_cast<uint32_t>(v)) {}
    explicit PropertyAttributes(JSTaggedValue v) : value_(v.GetInt()) {}
    explicit PropertyAttributes(const PropertyDescriptor &desc);

    static constexpr uint32_t DICTIONARY_ORDER_NUM = 20;
    static constexpr uint32_t OFFSET_BITFIELD_NUM = 10;
    static constexpr uint32_t REPRESENTATION_NUM = 2;
    static constexpr uint32_t TRACK_TYPE_NUM = 3;
    static constexpr uint32_t MAX_CAPACITY_OF_PROPERTIES = (1U << OFFSET_BITFIELD_NUM) - 1;
    static constexpr unsigned BITS_PER_BYTE = 8;

    using PropertyMetaDataField = BitField<int, 0, 4>;  // 4: property metaData field occupies 4 bits
    using AttributesField = BitField<int, 0, 4>;        // 4: attributes field occupies 4 bits
    using DefaultAttributesField = BitField<int, 0, 3>; // 3: default attributes field occupies 3 bits
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
    using OffsetField = FastModeStartField::NextField<uint32_t, OFFSET_BITFIELD_NUM>; // 17
    using TrackTypeField = OffsetField::NextField<TrackType, TRACK_TYPE_NUM>;     // 20: 3 bits

    static constexpr uint32_t NORMAL_ATTR_BITS = 20;
    using NormalAttrField = BitField<int, 0, NORMAL_ATTR_BITS>;
    using SortedIndexField = TrackTypeField::NextField<uint32_t, OFFSET_BITFIELD_NUM>; // 30
    using IsConstPropsField = SortedIndexField::NextFlag;                              // 31
    using IsNotHoleField = IsConstPropsField::NextFlag;                                // 32
    using FastModeLastField = IsNotHoleField;
    static_assert(
        FastModeLastField::START_BIT + FastModeLastField::SIZE <= sizeof(uint32_t) * BITS_PER_BYTE, "Invalid");

    // ---------------------------------------------------------------------------------------------
    // only for dictionary mode, include global
    using DictModeStartField = CommonLastBitField;
    static_assert(DictModeStartField::START_BIT == CommonLastBitField::START_BIT);
    static_assert(DictModeStartField::SIZE == CommonLastBitField::SIZE);
    using PropertyBoxTypeField = DictModeStartField::NextField<PropertyBoxType, 2>;               // 2: 2 bits, 8-9
    using DictionaryOrderField = PropertyBoxTypeField::NextField<uint32_t, DICTIONARY_ORDER_NUM>; // 29
    using DictModeLastField = DictionaryOrderField;
    static_assert(
        DictModeLastField::START_BIT + DictModeLastField::SIZE <= sizeof(uint32_t) * BITS_PER_BYTE, "Invalid");

    static constexpr uint32_t BIT_SIZE = 28;
    static constexpr int INITIAL_PROPERTY_INDEX = 0;

    inline int GetPropertyMetaData() const
    {
        return PropertyMetaDataField::Get(value_);
    }

    static PropertyAttributes Default()
    {
        return PropertyAttributes(GetDefaultAttributes());
    }

    static PropertyAttributes Default(bool w, bool e, bool c, bool isAccessor = false)
    {
        uint32_t value = WritableField::Encode(w) | EnumerableField::Encode(e) | ConfigurableField::Encode(c) |
                         IsAccessorField::Encode(isAccessor);
        return PropertyAttributes(value);
    }

    static PropertyAttributes DefaultAccessor(bool w, bool e, bool c)
    {
        uint32_t value = WritableField::Encode(w) | EnumerableField::Encode(e) | ConfigurableField::Encode(c) |
                         IsAccessorField::Encode(true);
        return PropertyAttributes(value);
    }

    inline void SetDefaultAttributes()
    {
        AttributesField::Set<uint32_t>(DefaultAttributesField::Mask(), &value_);
    }

    static inline int GetDefaultAttributes()
    {
        return DefaultAttributesField::Mask();
    }

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
        return AttributesField::Get(value_) == static_cast<int>(DefaultAttributesField::Mask());
    }

    inline void SetNoneAttributes()
    {
        AttributesField::Set<uint32_t>(0U, &value_);
    }

    inline bool IsNoneAttributes() const
    {
        return AttributesField::Get(value_) == 0;
    }

    inline void SetWritable(bool flag)
    {
        WritableField::Set<uint32_t>(flag, &value_);
    }
    inline bool IsWritable() const
    {
        return WritableField::Get(value_);
    }
    inline void SetEnumerable(bool flag)
    {
        EnumerableField::Set<uint32_t>(flag, &value_);
    }
    inline bool IsEnumerable() const
    {
        return EnumerableField::Get(value_);
    }
    inline void SetConfigurable(bool flag)
    {
        ConfigurableField::Set<uint32_t>(flag, &value_);
    }
    inline bool IsConfigurable() const
    {
        return ConfigurableField::Get(value_);
    }

    inline void SetIsAccessor(bool flag)
    {
        IsAccessorField::Set<uint32_t>(flag, &value_);
    }

    inline bool IsAccessor() const
    {
        return IsAccessorField::Get(value_);
    }

    inline void SetIsInlinedProps(bool flag)
    {
        IsInlinedPropsField::Set<uint32_t>(flag, &value_);
    }

    inline bool IsInlinedProps() const
    {
        return IsInlinedPropsField::Get(value_);
    }

    inline void SetIsConstProps(bool flag)
    {
        IsConstPropsField::Set<uint32_t>(flag, &value_);
    }

    inline bool IsConstProps() const
    {
        return IsConstPropsField::Get(value_);
    }

    inline void SetIsNotHole(bool flag)
    {
        IsNotHoleField::Set<uint32_t>(flag, &value_);
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
        RepresentationField::Set<uint32_t>(representation, &value_);
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
        TrackTypeField::Set(type, &value_);
    }

    inline void SetDictionaryOrder(uint32_t order)
    {
        DictionaryOrderField::Set<uint32_t>(order, &value_);
    }
    inline uint32_t GetDictionaryOrder() const
    {
        return DictionaryOrderField::Get(value_);
    }

    inline void SetOffset(uint32_t offset)
    {
        OffsetField::Set<uint32_t>(offset, &value_);
    }
    inline uint32_t GetOffset() const
    {
        return OffsetField::Get(value_);
    }

    inline void SetSortedIndex(uint32_t sortedIndex)
    {
        SortedIndexField::Set<uint32_t>(sortedIndex, &value_);
    }
    inline uint32_t GetSortedIndex() const
    {
        return SortedIndexField::Get(value_);
    }

    inline void SetNormalAttr(uint32_t normalAttr)
    {
        NormalAttrField::Set<uint32_t>(normalAttr, &value_);
    }

    inline uint32_t GetNormalAttr() const
    {
        return NormalAttrField::Get(value_);
    }

    inline JSTaggedValue GetNormalTagged() const
    {
        return JSTaggedValue(static_cast<int>(GetNormalAttr()));
    }

    inline uint32_t GetValue() const
    {
        return value_;
    }

    inline void SetBoxType(PropertyBoxType cellType)
    {
        PropertyBoxTypeField::Set<uint32_t>(cellType, &value_);
    }

    inline PropertyBoxType GetBoxType() const
    {
        return PropertyBoxTypeField::Get(value_);
    }

    inline static bool IsValidIndex(int index)
    {
        return DictionaryOrderField::IsValid(index);
    }

    inline JSTaggedValue GetTaggedValue() const
    {
        return JSTaggedValue(static_cast<int>(value_));
    }

private:
    uint32_t value_{0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PROPERTY_ATTRIBUTES_H
