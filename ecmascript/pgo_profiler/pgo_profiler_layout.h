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

#include <cstdint>
#include <string>

#include "ecmascript/elements.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript::pgo {
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

struct ElementsTrackInfo {
    std::string ToString() const
    {
        std::stringstream stream;
            stream << "(size: " << arrayLength_ << ", " << ToSpaceTypeName(spaceFlag_) << ")";
        return stream.str();
    }

    uint32_t arrayLength_ { 0 };
    RegionSpaceFlag spaceFlag_ { RegionSpaceFlag::UNINITIALIZED };
};

class PGOHClassLayoutDesc {
public:
    PGOHClassLayoutDesc() {};
    explicit PGOHClassLayoutDesc(ProfileType type) : type_(type) {}

    void SetSuperProfileType(ProfileType superType)
    {
        superType_ = superType;
    }

    ProfileType GetSuperProfileType() const
    {
        return superType_;
    }

    ProfileType GetProfileType() const
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

    ElementsTrackInfo GetElementsTrackInfo() const
    {
        return elementTrackInfo_;
    }

    void SetElementsKind(ElementsKind kind)
    {
        kind_ = kind;
    }

    uint32_t GetArrayLength() const
    {
        return elementTrackInfo_.arrayLength_;
    }

    void UpdateArrayLength(uint32_t size)
    {
        elementTrackInfo_.arrayLength_ = size;
    }

    RegionSpaceFlag GetSpaceFlag() const
    {
        return elementTrackInfo_.spaceFlag_;
    }

    void UpdateSpaceFlag(RegionSpaceFlag spaceFlag)
    {
        elementTrackInfo_.spaceFlag_ = spaceFlag;
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

    ProfileType type_;
    ProfileType superType_;
    ElementsTrackInfo elementTrackInfo_;
    ElementsKind kind_;
    LayoutDesc layoutDesc_;
    LayoutDesc ptLayoutDesc_;
    LayoutDesc ctorLayoutDesc_;
};

template <typename SampleType>
class PGOHClassLayoutTemplate {
public:
    PGOHClassLayoutTemplate(size_t size, SampleType type, SampleType superType, ElementsKind kind,
        ElementsTrackInfo &trackInfo)
        : size_(size), type_(type), superType_(superType)
    {
        SetElementsKind(kind);
        SetElementsTrackInfo(trackInfo);
    }

    static size_t CaculateSize(const PGOHClassLayoutDesc &desc)
    {
        if (desc.GetLayoutDesc().empty() && desc.GetPtLayoutDesc().empty() && desc.GetCtorLayoutDesc().empty()) {
            return sizeof(PGOHClassLayoutTemplate<SampleType>);
        }
        size_t size = sizeof(PGOHClassLayoutTemplate<SampleType>) - sizeof(PGOLayoutDescInfo);
        for (const auto &iter : desc.GetLayoutDesc()) {
            auto key = iter.first;
            if (key.size() > 0) {
                size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
            }
        }
        for (const auto &iter : desc.GetPtLayoutDesc()) {
            auto key = iter.first;
            if (key.size() > 0) {
                size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
            }
        }
        for (const auto &iter : desc.GetCtorLayoutDesc()) {
            auto key = iter.first;
            if (key.size() > 0) {
                size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
            }
        }
        size += sizeof(ElementsTrackInfo);
        size += sizeof(ElementsKind);
        return size;
    }
    static std::string GetTypeString(const PGOHClassLayoutDesc &desc)
    {
        std::string text;
        text += desc.GetProfileType().GetTypeString();
        if (!desc.GetSuperProfileType().IsNone()) {
            text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
            text += desc.GetSuperProfileType().GetTypeString();
        }
        if (desc.GetArrayLength() > 0 && desc.GetSpaceFlag() != RegionSpaceFlag::UNINITIALIZED) {
            text += desc.GetElementsTrackInfo().ToString();
        }
        if (!Elements::IsNone(desc.GetElementsKind())) {
            text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
            text += Elements::GetString(desc.GetElementsKind());
        }
        text += DumpUtils::BLOCK_AND_ARRAY_START;
        bool isLayoutFirst = true;
        for (const auto &layoutDesc : desc.GetLayoutDesc()) {
            if (!isLayoutFirst) {
                text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
            } else {
                text += DumpUtils::ARRAY_START;
            }
            isLayoutFirst = false;
            text += layoutDesc.first;
            text += DumpUtils::BLOCK_START;
            text += std::to_string(layoutDesc.second.GetValue());
        }
        if (!isLayoutFirst) {
            text += DumpUtils::ARRAY_END;
        }
        bool isPtLayoutFirst = true;
        for (const auto &layoutDesc : desc.GetPtLayoutDesc()) {
            if (!isPtLayoutFirst) {
                text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
            } else {
                if (!isLayoutFirst) {
                    text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
                }
                text += DumpUtils::ARRAY_START;
            }
            isPtLayoutFirst = false;
            text += layoutDesc.first;
            text += DumpUtils::BLOCK_START;
            text += std::to_string(layoutDesc.second.GetValue());
        }
        if (!isPtLayoutFirst) {
            text += DumpUtils::ARRAY_END;
        }
        bool isCtorLayoutFirst = true;
        for (const auto &layoutDesc : desc.GetCtorLayoutDesc()) {
            if (!isCtorLayoutFirst) {
                text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
            } else {
                if (!isLayoutFirst || !isPtLayoutFirst) {
                    text += DumpUtils::TYPE_SEPARATOR + DumpUtils::SPACE;
                }
                text += DumpUtils::ARRAY_START;
            }
            isCtorLayoutFirst = false;
            text += layoutDesc.first;
            text += DumpUtils::BLOCK_START;
            text += std::to_string(layoutDesc.second.GetValue());
        }
        if (!isCtorLayoutFirst) {
            text += DumpUtils::ARRAY_END;
        }
        text += (DumpUtils::SPACE + DumpUtils::ARRAY_END);
        return text;
    }

    void Merge(const PGOHClassLayoutDesc &desc)
    {
        auto current = const_cast<PGOLayoutDescInfo *>(GetFirst());
        for (const auto &iter : desc.GetLayoutDesc()) {
            auto key = iter.first;
            auto type = iter.second;
            if (key.size() > 0) {
                new (current) PGOLayoutDescInfo(key, type);
                current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
                count_++;
            }
        }
        for (const auto &iter : desc.GetPtLayoutDesc()) {
            auto key = iter.first;
            auto type = iter.second;
            if (key.size() > 0) {
                new (current) PGOLayoutDescInfo(key, type);
                current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
                ptCount_++;
            }
        }
        for (const auto &iter : desc.GetCtorLayoutDesc()) {
            auto key = iter.first;
            auto type = iter.second;
            if (key.size() > 0) {
                new (current) PGOLayoutDescInfo(key, type);
                current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
                ctorCount_++;
            }
        }
    }

    int32_t Size() const
    {
        return size_;
    }

    SampleType GetType() const
    {
        return type_;
    }

    SampleType GetSuperType() const
    {
        return superType_;
    }

    PGOHClassLayoutDesc Convert(PGOContext& context)
    {
        PGOHClassLayoutDesc desc(ProfileType(context, GetType().GetProfileType()));
        desc.SetSuperProfileType(ProfileType(context, superType_.GetProfileType()));
        auto descInfo = GetFirst();
        for (int32_t i = 0; i < count_; i++) {
            desc.AddKeyAndDesc(descInfo->GetKey(), descInfo->GetHandler());
            descInfo = GetNext(descInfo);
        }
        for (int32_t i = 0; i < ptCount_; i++) {
            desc.AddPtKeyAndDesc(descInfo->GetKey(), descInfo->GetHandler());
            descInfo = GetNext(descInfo);
        }
        for (int32_t i = 0; i < ctorCount_; i++) {
            desc.AddCtorKeyAndDesc(descInfo->GetKey(), descInfo->GetHandler());
            descInfo = GetNext(descInfo);
        }
        if (context.SupportElementsKind()) {
            desc.SetElementsKind(GetElementsKind());
        }
        if (context.SupportElementsTrackInfo()) {
            auto trackInfo = GetElementsTrackInfo();
            desc.UpdateArrayLength(trackInfo->arrayLength_);
            desc.UpdateSpaceFlag(trackInfo->spaceFlag_);
        }
        return desc;
    }

    class PGOLayoutDescInfo {
    public:
        PGOLayoutDescInfo() = default;
        PGOLayoutDescInfo(const CString &key, PGOHandler handler) : handler_(handler)
        {
            size_t len = key.size();
            size_ = Size(len);
            if (len > 0 && memcpy_s(&key_, len, key.c_str(), len) != EOK) {
                LOG_ECMA(ERROR) << "SetMethodName memcpy_s failed" << key << ", len = " << len;
                UNREACHABLE();
            }
            *(&key_ + len) = '\0';
        }

        static int32_t Size(size_t len)
        {
            return sizeof(PGOLayoutDescInfo) + AlignUp(len, GetAlignmentInBytes(ALIGN_SIZE));
        }

        int32_t Size() const
        {
            return size_;
        }

        const char *GetKey() const
        {
            return &key_;
        }

        PGOHandler GetHandler() const
        {
            return handler_;
        }

    private:
        int32_t size_ {0};
        PGOHandler handler_;
        char key_ {'\0'};
    };

private:
    const PGOLayoutDescInfo *GetFirst() const
    {
        return &descInfos_;
    }

    const PGOLayoutDescInfo *GetNext(const PGOLayoutDescInfo *current) const
    {
        return reinterpret_cast<PGOLayoutDescInfo *>(reinterpret_cast<uintptr_t>(current) + current->Size());
    }

    void SetElementsTrackInfo(ElementsTrackInfo trackInfo)
    {
        auto trackInfoOffset = GetEnd() - sizeof(ElementsTrackInfo) - sizeof(ElementsKind);
        *reinterpret_cast<ElementsTrackInfo *>(trackInfoOffset) = trackInfo;
    }

    ElementsTrackInfo* GetElementsTrackInfo()
    {
        auto trackInfoOffset = GetEnd() - sizeof(ElementsTrackInfo) - sizeof(ElementsKind);
        return reinterpret_cast<ElementsTrackInfo *>(trackInfoOffset);
    }

    void SetElementsKind(ElementsKind kind)
    {
        *reinterpret_cast<ElementsKind *>(GetEnd() - sizeof(ElementsKind)) = kind;
    }

    ElementsKind GetElementsKind() const
    {
        return *reinterpret_cast<const ElementsKind *>(GetEnd() - sizeof(ElementsKind));
    }

    uintptr_t GetEnd() const
    {
        return reinterpret_cast<uintptr_t>(this) + Size();
    }

    int32_t size_;
    SampleType type_;
    SampleType superType_;
    int32_t count_ {0};
    int32_t ptCount_ {0};
    int32_t ctorCount_ {0};
    PGOLayoutDescInfo descInfos_;
};

using PGOHClassLayoutDescInner = PGOHClassLayoutTemplate<PGOSampleType>;
using PGOHClassLayoutDescInnerRef = PGOHClassLayoutTemplate<PGOSampleTypeRef>;
} // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_PROFILER_LAYOUT_H
