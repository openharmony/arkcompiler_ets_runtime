/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/pgo_profiler/pgo_profiler_layout.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::pgo {

bool PGOHandler::SetAttribute(const JSThread *thread, PropertyAttributes &attr) const
{
    bool ret = false;
    if (thread->GetEcmaVM()->GetJSOptions().IsEnableOptTrackField()) {
        TrackType trackType = GetTrackType();
        attr.SetTrackType(trackType);
        switch (trackType) {
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
            default:
                attr.SetRepresentation(Representation::TAGGED);
                break;
        }
    } else {
        attr.SetRepresentation(Representation::TAGGED);
    }
    attr.SetWritable(IsWritable());
    attr.SetEnumerable(IsEnumerable());
    attr.SetConfigurable(IsConfigurable());
    attr.SetIsAccessor(IsAccessor());
    return ret;
}

void PGOHClassTreeDesc::Clear()
{
    IterateAll([] (HClassLayoutDesc *desc) {
        delete desc;
    });
    transitionLayout_.clear();
}

void PGOHClassTreeDesc::Merge(const PGOHClassTreeDesc &from)
{
    ASSERT(from.GetProfileType() == GetProfileType());
    from.IterateAll([this] (HClassLayoutDesc *fromDesc) {
        auto curLayoutDesc = GetHClassLayoutDesc(fromDesc->GetProfileType());
        if (curLayoutDesc == nullptr) {
            if (fromDesc->GetProfileType().IsRootType()) {
                RootHClassLayoutDesc *rootFromTreeDesc = reinterpret_cast<RootHClassLayoutDesc *>(fromDesc);
                curLayoutDesc = new RootHClassLayoutDesc(*rootFromTreeDesc);
            } else {
                ChildHClassLayoutDesc *childFromTreeDesc = reinterpret_cast<ChildHClassLayoutDesc *>(fromDesc);
                curLayoutDesc = new ChildHClassLayoutDesc(*childFromTreeDesc);
            }
            transitionLayout_.emplace(fromDesc->GetProfileType(), curLayoutDesc);
        } else {
            curLayoutDesc->Merge(fromDesc);
        }
    });
}

HClassLayoutDesc *PGOHClassTreeDesc::GetHClassLayoutDesc(ProfileType type) const
{
    auto iter = transitionLayout_.find(type);
    if (iter != transitionLayout_.end()) {
        return iter->second;
    }
    return nullptr;
}

HClassLayoutDesc *PGOHClassTreeDesc::GetOrInsertHClassLayoutDesc(ProfileType type, bool root)
{
    auto iter = transitionLayout_.find(type);
    if (iter != transitionLayout_.end()) {
        return iter->second;
    } else {
        HClassLayoutDesc *layout;
        if (root) {
            layout = new RootHClassLayoutDesc(type);
        } else {
            layout = new ChildHClassLayoutDesc(type);
        }
        transitionLayout_.emplace(type, layout);
        return layout;
    }
}

bool PGOHClassTreeDesc::DumpForRoot(JSTaggedType root, ProfileType rootType)
{
    ASSERT(rootType.IsRootType());
    HClassLayoutDesc *rootLayout;
    auto iter = transitionLayout_.find(rootType);
    auto rootHClass = JSHClass::Cast(JSTaggedValue(root).GetTaggedObject());
    if (iter != transitionLayout_.end()) {
        rootLayout = iter->second;
        return JSHClass::UpdateRootLayoutDescByPGO(rootHClass, rootLayout);
    } else {
        rootLayout = new RootHClassLayoutDesc(rootType, rootHClass->GetObjectType(),
                                              rootHClass->GetObjectSizeExcludeInlinedProps());
        transitionLayout_.emplace(rootType, rootLayout);
    }

    return JSHClass::DumpRootHClassByPGO(rootHClass, rootLayout);
}

bool PGOHClassTreeDesc::DumpForChild(JSTaggedType child, ProfileType childType)
{
    ASSERT(!childType.IsRootType());
    auto childHClass = JSHClass::Cast(JSTaggedValue(child).GetTaggedObject());

    HClassLayoutDesc *childLayout;
    auto iter = transitionLayout_.find(childType);
    if (iter != transitionLayout_.end()) {
        childLayout = iter->second;
        return JSHClass::UpdateChildLayoutDescByPGO(childHClass, childLayout);
    } else {
        childLayout = new ChildHClassLayoutDesc(childType);
        transitionLayout_.emplace(childType, childLayout);
        return JSHClass::DumpChildHClassByPGO(childHClass, childLayout);
    }
}

bool PGOHClassTreeDesc::UpdateLayout(JSTaggedType curHClass, ProfileType curType)
{
    if (curType.IsRootType()) {
        return DumpForRoot(curHClass, curType);
    } else {
        return DumpForChild(curHClass, curType);
    }
}

bool PGOHClassTreeDesc::IsDumped(ProfileType curType) const
{
    return transitionLayout_.find(curType) != transitionLayout_.end();
}

bool PGOHClassTreeDesc::UpdateForTransition(
    JSTaggedType parent, ProfileType parentType, JSTaggedType child, ProfileType childType)
{
    if (parentType.IsRootType()) {
        if (!DumpForRoot(parent, parentType)) {
            return false;
        }
    }
    if (transitionLayout_.find(parentType) == transitionLayout_.end()) {
        return false;
    }
    
    bool ret = DumpForChild(child, childType);
    auto parentLayoutDesc = transitionLayout_.find(parentType)->second;
    auto childLayoutDesc = transitionLayout_.find(childType)->second;
    parentLayoutDesc->AddChildHClassLayoutDesc(childLayoutDesc->GetProfileType());
    return ret;
}

void HClassLayoutDesc::Merge(const HClassLayoutDesc *from)
{
    from->IterateChilds([this] (const ProfileType &type) -> bool {
        AddChildHClassLayoutDesc(type);
        return true;
    });
}

void HClassLayoutDesc::InsertKeyAndDesc(const PGOHandler &handler, PropertyDesc &desc)
{
    PGOHandler oldHandler = desc.second;
    if (oldHandler == handler) {
        return;
    }
    auto oldTrackType = oldHandler.GetTrackType();
    auto newTrackType = handler.GetTrackType();
    if (oldTrackType == newTrackType) {
        desc.second.SetPropertyMeta(handler.GetPropertyMeta());
        return;
    }

    switch (oldTrackType) {
        case TrackType::TAGGED:
            desc.second.SetPropertyMeta(handler.GetPropertyMeta());
            break;
        case TrackType::NONE:
        case TrackType::INT:
        case TrackType::DOUBLE:
            if (newTrackType != TrackType::TAGGED) {
                newTrackType = static_cast<TrackType>(static_cast<uint8_t>(newTrackType) |
                    static_cast<uint8_t>(oldTrackType));
            }
            desc.second = PGOHandler(newTrackType, handler.GetPropertyMeta());
            break;
        default:
            break;
    }
}

void RootHClassLayoutDesc::Merge(const HClassLayoutDesc *from)
{
    ASSERT(from->GetProfileType() == GetProfileType());
    ASSERT(from->GetProfileType().IsRootType());
    auto fromDesc = reinterpret_cast<const RootHClassLayoutDesc *>(from);
    fromDesc->IterateProps([this] (const PropertyDesc &desc) {
        InsertKeyAndDesc(desc.first, desc.second);
    });
    HClassLayoutDesc::Merge(from);
}

void RootHClassLayoutDesc::InsertKeyAndDesc(const CString &key, const PGOHandler &handler)
{
    if (!UpdateKeyAndDesc(key, handler)) {
        layoutDesc_.emplace_back(key, handler);
    }
}

bool RootHClassLayoutDesc::UpdateKeyAndDesc(const CString &key, const PGOHandler &handler)
{
    for (auto &iter : layoutDesc_) {
        if (iter.first == key) {
            HClassLayoutDesc::InsertKeyAndDesc(handler, iter);
            return true;
        }
    }
    return false;
}

void ChildHClassLayoutDesc::Merge(const HClassLayoutDesc *from)
{
    ASSERT(from->GetProfileType() == GetProfileType());
    ASSERT(!from->GetProfileType().IsRootType());
    auto fromDesc = reinterpret_cast<const ChildHClassLayoutDesc *>(from);
    auto fromPropDesc = fromDesc->GetPropertyDesc();
    InsertKeyAndDesc(fromPropDesc.first, fromPropDesc.second);
    HClassLayoutDesc::Merge(from);
}

void ChildHClassLayoutDesc::InsertKeyAndDesc(const CString &key, const PGOHandler &handler)
{
    if (!UpdateKeyAndDesc(key, handler)) {
        propertyDesc_ = PropertyDesc(key, handler);
    }
}

bool ChildHClassLayoutDesc::UpdateKeyAndDesc(const CString &key, const PGOHandler &handler)
{
    if (propertyDesc_.first == key) {
        HClassLayoutDesc::InsertKeyAndDesc(handler, propertyDesc_);
        return true;
    }
    return false;
}
} // namespace panda::ecmascript::pgo
