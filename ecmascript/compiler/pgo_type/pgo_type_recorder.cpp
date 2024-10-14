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

#include "ecmascript/compiler/pgo_type/pgo_type_recorder.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript::kungfu {
using PGOType = pgo::PGOType;
using PGOObjectInfo = pgo::PGOObjectInfo;

PGOTypeRecorder::PGOTypeRecorder(const PGOProfilerDecoder &decoder) : decoder_(decoder) {}

PGOTypeRecorder::PGOTypeRecorder(
    const PGOProfilerDecoder &decoder, const JSPandaFile *jsPandaFile, uint32_t methodOffset)
    : decoder_(decoder)
{
    auto callback = [this] (uint32_t offset, const PGOType *type) {
        if (type->IsScalarOpType()) {
            bcOffsetPGOOpTypeMap_.emplace(offset, reinterpret_cast<const PGOSampleType *>(type));
        } else if (type->IsRwOpType()) {
            bcOffsetPGORwTypeMap_[offset] = reinterpret_cast<const PGORWOpType *>(type);
        } else if (type->IsDefineOpType()) {
            bcOffsetPGODefOpTypeMap_.emplace(offset, reinterpret_cast<const PGODefineOpType *>(type));
        } else {
            UNREACHABLE();
        }
    };
    const CString recordName = MethodLiteral::GetRecordName(jsPandaFile, EntityId(methodOffset));
    auto methodLiteral = jsPandaFile->FindMethodLiteral(methodOffset);
    decoder.GetTypeInfo(jsPandaFile, recordName, methodLiteral, callback);
}

std::vector<ElementsKind> PGOTypeRecorder::GetElementsKindsForUser(int32_t offset) const
{
    std::vector<ElementsKind> elementsKinds;
    if (bcOffsetPGORwTypeMap_.find(offset) == bcOffsetPGORwTypeMap_.end()) {
        elementsKinds.emplace_back(ElementsKind::GENERIC);
        return elementsKinds;
    }

    PGORWOpType rwType = *(bcOffsetPGORwTypeMap_.at(offset));
    if (rwType.GetCount() == 0) {
        elementsKinds.emplace_back(ElementsKind::GENERIC);
        return elementsKinds;
    }
    for (uint32_t i = 0; i < rwType.GetCount(); i++) {
        PGOObjectInfo info = rwType.GetObjectInfo(i);
        auto profileType = info.GetProfileType();
        if (profileType.IsBuiltinsArray()) {
            elementsKinds.emplace_back(profileType.GetElementsKindBeforeTransition());
            continue;
        }
    }

    return elementsKinds;
}

std::vector<ElementsKind> PGOTypeRecorder::GetTransitionElementsKindsForUser(int32_t offset) const
{
    std::vector<ElementsKind> elementsKinds;
    if (bcOffsetPGORwTypeMap_.find(offset) == bcOffsetPGORwTypeMap_.end()) {
        elementsKinds.emplace_back(ElementsKind::GENERIC);
        return elementsKinds;
    }

    PGORWOpType rwType = *(bcOffsetPGORwTypeMap_.at(offset));
    if (rwType.GetCount() == 0) {
        elementsKinds.emplace_back(ElementsKind::GENERIC);
        return elementsKinds;
    }
    for (uint32_t i = 0; i < rwType.GetCount(); i++) {
        PGOObjectInfo info = rwType.GetObjectInfo(i);
        auto profileType = info.GetProfileType();
        if (profileType.IsBuiltinsArray()) {
            elementsKinds.emplace_back(profileType.GetElementsKindAfterTransition());
            continue;
        }
    }

    return elementsKinds;
}

ElementsKind PGOTypeRecorder::GetElementsKindForCreater(int32_t offset) const
{
    if (bcOffsetPGODefOpTypeMap_.find(offset) != bcOffsetPGODefOpTypeMap_.end()) {
        const auto iter = bcOffsetPGODefOpTypeMap_.at(offset);
        return iter->GetElementsKind();
    }
    // When ElementsKind switch turned on, it should be GENERIC
    return ElementsKind::NONE;
}

uint32_t PGOTypeRecorder::GetElementsLength(int32_t offset) const
{
    if (bcOffsetPGODefOpTypeMap_.find(offset) != bcOffsetPGODefOpTypeMap_.end()) {
        const auto iter = bcOffsetPGODefOpTypeMap_.at(offset);
        return iter->GetElementsLength();
    }
    return 0;
}

PGOTypeRef PGOTypeRecorder::GetPGOType(int32_t offset) const
{
    if (bcOffsetPGOOpTypeMap_.find(offset) != bcOffsetPGOOpTypeMap_.end()) {
        return PGOTypeRef(bcOffsetPGOOpTypeMap_.at(offset));
    } else if (bcOffsetPGORwTypeMap_.find(offset) != bcOffsetPGORwTypeMap_.end()) {
        return PGOTypeRef(bcOffsetPGORwTypeMap_.at(offset));
    } else if (bcOffsetPGODefOpTypeMap_.find(offset) != bcOffsetPGODefOpTypeMap_.end()) {
        return PGOTypeRef(bcOffsetPGODefOpTypeMap_.at(offset));
    }
    return PGOTypeRef::NoneType();
}

bool PGOTypeRecorder::IsValidPt(ProfileType type) const
{
    auto abcId = type.GetAbcId();
    CString abcName;
    if (decoder_.GetAbcNameById(abcId, abcName)) {
        CString normalizedFileName = JSPandaFile::GetNormalizedFileDesc(abcName);
        if (JSPandaFileManager::GetInstance()->FindJSPandaFileByNormalizedName(normalizedFileName) == nullptr) {
            LOG_ECMA(DEBUG) << "ProfileType is invalid:" << type.GetTypeString() << ", abcName:" << abcName
                            << ", normalize name:" << normalizedFileName;
            return false;
        }
        return true;
    }
    LOG_ECMA(DEBUG) << "ProfileType is invalid:" << type.GetTypeString();
    return false;
}
}  // namespace panda::ecmascript
