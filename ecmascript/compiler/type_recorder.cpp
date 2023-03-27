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

#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/ts_types/ts_type_parser.h"

#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript::kungfu {
TypeRecorder::TypeRecorder(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral,
                           TSManager *tsManager, const CString &recordName)
    : argTypes_(methodLiteral->GetNumArgsWithCallField() + static_cast<size_t>(TypedArgIdx::NUM_OF_TYPED_ARGS),
                GateType::AnyType())
{
    if (!jsPandaFile->HasTSTypes(recordName)) {
        return;
    }
    LoadTypes(jsPandaFile, methodLiteral, tsManager, recordName);
    tsManager->GenerateTSHClasses();
}

void TypeRecorder::LoadTypes(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral,
                             TSManager *tsManager, const CString &recordName)
{
    JSThread *thread = tsManager->GetThread();
    TSTypeParser typeParser(tsManager);
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId fieldId = methodLiteral->GetMethodId();
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
            if (::strcmp("_TypeOfInstruction", elemName) != 0) {
                continue;
            }

            panda_file::ScalarValue sv = adae.GetScalarValue();
            panda_file::File::EntityId literalOffset(sv.GetValue());
            JSHandle<TaggedArray> typeOfInstruction =
                LiteralDataExtractor::GetTypeLiteral(thread, jsPandaFile, literalOffset);

            GlobalTSTypeRef thisGT = GlobalTSTypeRef::Default();
            GlobalTSTypeRef funcGT = GlobalTSTypeRef::Default();
            for (uint32_t j = 0; j < typeOfInstruction->GetLength(); j = j + 2) {  // + 2 means bcOffset and typeId
                int32_t bcOffset = typeOfInstruction->Get(j).GetInt();
                uint32_t typeId =  static_cast<uint32_t>(typeOfInstruction->Get(j + 1).GetInt());
                GlobalTSTypeRef gt = typeParser.CreateGT(jsPandaFile, recordName, typeId);
                if (gt.IsDefault()) {
                    continue;
                }

                // The type of a function is recorded as (-1, funcTypeId). If the function is a member of a class,
                // the type of the class or its instance is is recorded as (-2, classTypeId). If it is a static
                // member, the type id refers to the type of the class; otherwise, it links to the type of the
                // instances of the class.
                if (bcOffset == METHOD_ANNOTATION_THIS_TYPE_OFFSET) {
                    thisGT = gt;
                    continue;
                }
                if (bcOffset == METHOD_ANNOTATION_FUNCTION_TYPE_OFFSET) {
                    tsManager->SetFuncMethodOffset(gt, methodLiteral->GetMethodId().GetOffset());
                    funcGT = gt;
                    continue;
                }
                auto type = GateType(gt);
                bcOffsetGtMap_.emplace(bcOffset, type);
            }
            LoadArgTypes(tsManager, funcGT, thisGT);
        }
    });
}

void TypeRecorder::LoadArgTypes(const TSManager *tsManager, GlobalTSTypeRef funcGT, GlobalTSTypeRef thisGT)
{
    argTypes_[static_cast<size_t>(TypedArgIdx::FUNC)] = TryGetFuncType(funcGT);
    argTypes_[static_cast<size_t>(TypedArgIdx::NEW_TARGET)] = TryGetNewTargetType(tsManager, thisGT);
    argTypes_[static_cast<size_t>(TypedArgIdx::THIS_OBJECT)] = TryGetThisType(tsManager, funcGT, thisGT);

    if (funcGT.IsDefault()) {
        return;
    }
    size_t extraParasNum = static_cast<size_t>(TypedArgIdx::NUM_OF_TYPED_ARGS);
    uint32_t numExplicitArgs = tsManager->GetFunctionTypeLength(funcGT);
    for (uint32_t explicitArgId = 0; explicitArgId < numExplicitArgs; explicitArgId++) {
        argTypes_[extraParasNum++] = GateType(tsManager->GetFuncParameterTypeGT(funcGT, explicitArgId));
    }
}

GateType TypeRecorder::TryGetThisType(const TSManager *tsManager, GlobalTSTypeRef funcGT, GlobalTSTypeRef thisGT) const
{
    // The parameter 'this' may be declared explicitly, e.g. foo(this: Person, num: number). In this case, the type
    // of 'this' is recorded in the type of the function. And this type is preferred over the type derived from
    // 'thisGT' if both are given.
    if (!funcGT.IsDefault()) {
        auto gt = tsManager->GetFuncThisGT(funcGT);
        if (!gt.IsDefault()) {
            return GateType(gt);
        }
    }
    return GateType(thisGT);
}

GateType TypeRecorder::TryGetNewTargetType(const TSManager *tsManager, GlobalTSTypeRef thisGT) const
{
    if (thisGT.IsDefault()) {
        return GateType::AnyType();
    }

    GateType thisType(thisGT);
    if (tsManager->IsClassInstanceTypeKind(thisType)) {
        return GateType(tsManager->GetClassType(thisGT));
    } else {
        return thisType;
    }
}

GateType TypeRecorder::TryGetFuncType(GlobalTSTypeRef funcGT) const
{
    if (funcGT.IsDefault()) {
        return GateType::AnyType();
    }
    return GateType(funcGT);
}

GateType TypeRecorder::GetType(const int32_t offset) const
{
    if (bcOffsetGtMap_.find(offset) != bcOffsetGtMap_.end()) {
        return bcOffsetGtMap_.at(offset);
    }
    return GateType::AnyType();
}

GateType TypeRecorder::GetArgType(const uint32_t argIndex) const
{
    ASSERT(argIndex < argTypes_.size());
    return argTypes_[argIndex];
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
