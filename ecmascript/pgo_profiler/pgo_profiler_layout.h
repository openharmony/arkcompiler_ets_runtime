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

#ifndef ECMASCRIPT_PGO_PROFILER_LAYOUT_H
#define ECMASCRIPT_PGO_PROFILER_LAYOUT_H

#include <stdint.h>
#include <string>

#include "ecmascript/elements.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/pgo_profiler/pgo_profiler_type.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
class PGOHandler {
public:
    using TrackTypeField = BitField<TrackType, 0, PropertyAttributes::TRACK_TYPE_NUM>; // 3 : three binary bits
    using IsAccessorField = TrackTypeField::NextFlag;

    PGOHandler()
    {
        SetTrackType(TrackType::NONE);
        SetIsAccessor(false);
    }

    PGOHandler(TrackType type, bool isAccessor)
    {
        SetTrackType(type);
        SetIsAccessor(isAccessor);
    }

    uint32_t GetValue() const
    {
        return value_;
    }

    bool SetAttribute(PropertyAttributes &attr) const
    {
        bool ret = false;
        switch (GetTrackType()) {
            case TrackType::DOUBLE:
            case TrackType::NUMBER:
                attr.SetRepresentation(Representation::DOUBLE);
                ret = true;
                break;
            case TrackType::INT:
                attr.SetRepresentation(Representation::INT);
                ret = true;
                break;
            case TrackType::TAGGED:
                attr.SetRepresentation(Representation::TAGGED);
                break;
            default:
                break;
        }
        return ret;
    }

    void SetTrackType(TrackType type)
    {
        TrackTypeField::Set(type, &value_);
    }

    TrackType GetTrackType() const
    {
        return TrackTypeField::Get(value_);
    }

    void SetIsAccessor(bool accessor)
    {
        IsAccessorField::Set(accessor, &value_);
    }

    bool IsAccessor() const
    {
        return IsAccessorField::Get(value_);
    }

    bool operator!=(const PGOHandler &right) const
    {
        return value_ != right.value_;
    }

    bool operator==(const PGOHandler &right) const
    {
        return value_ == right.value_;
    }

private:
    uint32_t value_ { 0 };
};

using PropertyDesc = std::pair<CString, PGOHandler>;
using LayoutDesc = CVector<PropertyDesc>;

class PGOHClassLayoutDesc {
public:
    PGOHClassLayoutDesc() {};
    explicit PGOHClassLayoutDesc(ClassType type) : type_(type) {}

    void SetSuperClassType(ClassType superType)
    {
        superType_ = superType;
    }

    ClassType GetSuperClassType() const
    {
        return superType_;
    }

    ClassType GetClassType() const
    {
        return type_;
    }

    LayoutDesc GetLayoutDesc() const
    {
        return layoutDesc_;
    }

    void SetLayoutDesc(LayoutDesc &layoutDesc)
    {
        layoutDesc_ = layoutDesc;
    }

    LayoutDesc GetPtLayoutDesc() const
    {
        return ptLayoutDesc_;
    }

    LayoutDesc GetCtorLayoutDesc() const
    {
        return ctorLayoutDesc_;
    }

    ElementsKind GetElementsKind() const
    {
        return kind_;
    }

    void SetElementsKind(ElementsKind kind)
    {
        kind_ = kind;
    }

    bool FindProperty(const CString &key, PropertyDesc &desc) const
    {
        for (const auto &iter : layoutDesc_) {
            if (iter.first == key) {
                desc = iter;
                return true;
            }
        }
        return false;
    }

    void AddKeyAndDesc(const CString &key, const PGOHandler &handler)
    {
        layoutDesc_.emplace_back(key, handler);
    }

    void AddPtKeyAndDesc(const CString &key, const PGOHandler &handler)
    {
        ptLayoutDesc_.emplace_back(key, handler);
    }

    void AddCtorKeyAndDesc(const CString &key, const PGOHandler &handler)
    {
        ctorLayoutDesc_.emplace_back(key, handler);
    }

    void UpdateElementKind(const ElementsKind kind);
    void UpdateKeyAndDesc(const CString &key, const PGOHandler &handler, PGOObjKind kind);

    bool FindDescWithKey(const CString &key, PGOHandler &handler) const;

    void Merge(const PGOHClassLayoutDesc &from);

    bool operator<(const PGOHClassLayoutDesc &right) const
    {
        return type_ < right.type_;
    }

private:
    void UpdateKeyAndDesc(const CString &key, const PGOHandler &handler, LayoutDesc &layoutDesc);

    ClassType type_;
    ClassType superType_;
    ElementsKind kind_;
    LayoutDesc layoutDesc_;
    LayoutDesc ptLayoutDesc_;
    LayoutDesc ctorLayoutDesc_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_LAYOUT_H
