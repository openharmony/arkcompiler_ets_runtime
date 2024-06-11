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

#include "verify_annotation.h"
#include "global_tables.h"
#include "reflection_analysis.h"

namespace maple {
namespace verifyanno {
constexpr char kDeferredThrowVerifyError[] = "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoThrowVerifyError_3B";
constexpr char kDeferredExtendFinalCheck[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredExtendFinalCheck_3B";
constexpr char kDeferredOverrideFinalCheck[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredOverrideFinalCheck_3B";
constexpr char kDeferredAssignableCheck[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredAssignableCheck_3B";
constexpr char kAssignableChecksContainer[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredAssignableChecks_3B";
}  // namespace verifyanno

inline std::string SimplifyClassName(const std::string &name)
{
    const std::string postFix = "_3B";
    if (name.rfind(postFix) == (name.size() - postFix.size())) {
        std::string shortName = name.substr(0, name.size() - postFix.size());
        return shortName.append(";");
    }
    return name;
}

inline MIRStructType *GetOrCreateStructType(const std::string &name, MIRModule &md)
{
    return static_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetOrCreateClassType(name, md));
}

MIRPragmaElement *NewAnnotationElement(MIRModule &md, const GStrIdx &nameIdx, PragmaValueType type)
{
    auto *elem = md.GetMemPool()->New<MIRPragmaElement>(md.GetMPAllocator());
    elem->SetType(type);
    elem->SetNameStrIdx(nameIdx);
    return elem;
}

MIRPragmaElement *NewAnnotationElement(MIRModule &md, const std::string &name, PragmaValueType type)
{
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    return NewAnnotationElement(md, nameIdx, type);
}

MIRPragmaElement *NewAnnotationStringElement(MIRModule &md, const std::string &name, const std::string &value)
{
    MIRPragmaElement *elem = NewAnnotationElement(md, name, kValueString);
    if (!value.empty()) {
        elem->SetU64Val(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(value).GetIdx());
    }
    return elem;
}

MIRPragma *NewPragmaRTAnnotation(MIRModule &md, const MIRStructType &clsType, const MIRStructType &aType,
                                 PragmaKind kind)
{
    auto *pragma = md.GetMemPool()->New<MIRPragma>(md, md.GetMPAllocator());
    pragma->SetStrIdx(clsType.GetNameStrIdx());
    pragma->SetTyIdx(aType.GetTypeIndex());
    pragma->SetKind(kind);
    pragma->SetVisibility(kVisRuntime);
    return pragma;
}

MIRPragma *NewPragmaRTClassAnnotation(MIRModule &md, const MIRStructType &clsType, const std::string &aTypeName)
{
    MIRStructType *aType = GetOrCreateStructType(aTypeName, md);
    return NewPragmaRTAnnotation(md, clsType, *aType, kPragmaClass);
}

// creates pragma not inserted into clsType pragmaVec
MIRPragma *NewAssignableCheckAnno(MIRModule &md, const MIRStructType &clsType, const AssignableCheckPragma &info)
{
    MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredAssignableCheck);
    pr->PushElementVector(NewAnnotationStringElement(md, "dst", SimplifyClassName(info.GetToType())));
    pr->PushElementVector(NewAnnotationStringElement(md, "src", SimplifyClassName(info.GetFromType())));
    return pr;
}

void AddVerfAnnoThrowVerifyError(MIRModule &md, const ThrowVerifyErrorPragma &info, MIRStructType &clsType)
{
    const auto &pragmaVec = clsType.GetPragmaVec();
    for (auto pragmaPtr : pragmaVec) {
        if (pragmaPtr == nullptr) {
            continue;
        }
        MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pragmaPtr->GetTyIdx());
        if (mirType != nullptr && mirType->GetName().compare(verifyanno::kDeferredThrowVerifyError) == 0) {
            return;
        }
    }
    MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredThrowVerifyError);
    pr->PushElementVector(NewAnnotationStringElement(md, "msg", info.GetMessage()));
    clsType.PushbackPragma(pr);
}

void AddVerfAnnoAssignableCheck(MIRModule &md, std::vector<const AssignableCheckPragma *> &info, MIRStructType &clsType)
{
    if (info.empty()) {
        return;
    }

    if (info.size() == 1) {
        MIRPragma *pr = NewAssignableCheckAnno(md, clsType, *info.front());
        clsType.PushbackPragma(pr);
        return;
    }

    // container pragma
    MIRPragma *prContainer = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kAssignableChecksContainer);
    MIRPragmaElement *elemContainer = NewAnnotationElement(md, "value", kValueArray);

    GStrIdx singleAnnoTypeName =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(verifyanno::kDeferredAssignableCheck);
    for (auto *singleInfo : info) {
        MIRPragmaElement *elem = NewAnnotationElement(md, singleAnnoTypeName, kValueAnnotation);
        elem->SetTypeStrIdx(singleAnnoTypeName);
        elem->SubElemVecPushBack(NewAnnotationStringElement(md, "dst", SimplifyClassName(singleInfo->GetToType())));
        elem->SubElemVecPushBack(NewAnnotationStringElement(md, "src", SimplifyClassName(singleInfo->GetFromType())));
        elemContainer->SubElemVecPushBack(elem);
    }

    prContainer->PushElementVector(elemContainer);
    clsType.PushbackPragma(prContainer);
}

void AddVerfAnnoExtendFinalCheck(MIRModule &md, MIRStructType &clsType)
{
    MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredExtendFinalCheck);
    clsType.PushbackPragma(pr);
}

void AddVerfAnnoOverrideFinalCheck(MIRModule &md, MIRStructType &clsType)
{
    MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredOverrideFinalCheck);
    clsType.PushbackPragma(pr);
}
}  // namespace maple
