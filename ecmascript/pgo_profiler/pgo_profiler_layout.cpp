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

#include "ecmascript/pgo_profiler/pgo_profiler_layout.h"

namespace panda::ecmascript {
void PGOHClassLayoutDesc::UpdateKeyAndDesc(const CString &key, const PGOHandler &handler, PGOObjKind kind)
{
    switch (kind) {
        case PGOObjKind::LOCAL:
            UpdateKeyAndDesc(key, handler, layoutDesc_);
            break;
        case PGOObjKind::PROTOTYPE:
            UpdateKeyAndDesc(key, handler, ptLayoutDesc_);
            break;
        case PGOObjKind::CONSTRUCTOR:
            UpdateKeyAndDesc(key, handler, ctorLayoutDesc_);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

bool PGOHClassLayoutDesc::FindDescWithKey(const CString &key, PGOHandler &handler) const
{
    PropertyDesc desc;
    if (!FindProperty(key, desc)) {
        return false;
    }
    handler = desc.second;
    return true;
}

void PGOHClassLayoutDesc::Merge(const PGOHClassLayoutDesc &from)
{
    for (const auto &iter : from.layoutDesc_) {
        UpdateKeyAndDesc(iter.first, iter.second, PGOObjKind::LOCAL);
    }
    for (const auto &iter : from.ptLayoutDesc_) {
        UpdateKeyAndDesc(iter.first, iter.second, PGOObjKind::PROTOTYPE);
    }
    for (const auto &iter : from.ctorLayoutDesc_) {
        UpdateKeyAndDesc(iter.first, iter.second, PGOObjKind::CONSTRUCTOR);
    }
}

void PGOHClassLayoutDesc::UpdateKeyAndDesc(const CString &key, const PGOHandler &handler, LayoutDesc &layoutDesc)
{
    for (auto &iter : layoutDesc) {
        if (iter.first == key) {
            PGOHandler oldType = iter.second;
            if (oldType == handler) {
                return;
            }
            if (oldType.GetTrackType() == TrackType::NONE) {
                iter.second = handler;
            } else {
                if (oldType.GetTrackType() != handler.GetTrackType()) {
                    iter.second = PGOHandler(TrackType::TAGGED, handler.IsAccessor());
                } else {
                    iter.second.SetIsAccessor(handler.IsAccessor());
                }
            }
            return;
        }
    }
    layoutDesc.emplace_back(key, handler);
}
} // namespace panda::ecmascript
