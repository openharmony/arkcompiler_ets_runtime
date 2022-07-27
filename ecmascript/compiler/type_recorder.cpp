/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/type_recorder.h"

namespace panda::ecmascript::kungfu {
TypeRecorder::TypeRecorder(const JSMethod *method, TSManager *tsManager)
{
    LoadTypes(method, tsManager);
}

void TypeRecorder::LoadTypes(const JSMethod *method, TSManager *tsManager)
{
    const panda_file::File *pf = method->GetJSPandaFile()->GetPandaFile();
    panda_file::File::EntityId fieldId = method->GetMethodId();
    panda_file::MethodDataAccessor mda(*pf, fieldId);
    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(*pf, annotation_id);
        auto *annotationName = reinterpret_cast<const char *>(pf->GetStringData(ada.GetClassId()).data);
        ASSERT(annotationName != nullptr);
        if (::strcmp("L_ESTypeAnnotation;", annotationName) != 0) {
            return;
        }
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elemName = reinterpret_cast<const char *>(pf->GetStringData(adae.GetNameId()).data);
            ASSERT(elemName != nullptr);
            uint32_t elemCount = adae.GetArrayValue().GetCount();
            if (::strcmp("_TypeOfInstruction", elemName) != 0) {
                continue;
            }
            for (uint32_t j = 0; j < elemCount; j = j + 2) { // + 2 means typeId index
                int32_t bcOffset = adae.GetArrayValue().Get<int32_t>(j);
                uint32_t typeId = adae.GetArrayValue().Get<uint32_t>(j + 1);
                auto type = GateType(tsManager->CreateGT(*pf, typeId));
                if (!type.IsAnyType()) {
                    bcOffsetGtMap_.emplace(bcOffset, GateType(type));
                }
            }
        }
    });
}

GateType TypeRecorder::GetType(const int32_t offset) const
{
    if (bcOffsetGtMap_.find(offset) != bcOffsetGtMap_.end()) {
        return bcOffsetGtMap_.at(offset);
    }
    return GateType::AnyType();
}

GateType TypeRecorder::GetArgType(const int32_t argIndex) const
{
    int32_t argOffset = GetArgOffset(argIndex);
    if (bcOffsetGtMap_.find(argOffset) != bcOffsetGtMap_.end()) {
        return bcOffsetGtMap_.at(argOffset);
    }
    return GateType::AnyType();
}

GateType TypeRecorder::UpdateType(const int32_t offset, const GateType &type) const
{
    auto tempType = GetType(offset);
    if (!tempType.IsAnyType()) {
        ASSERT(type.IsAnyType());
        return tempType;
    }
    return type;
}
}  // namespace panda::ecmascript
