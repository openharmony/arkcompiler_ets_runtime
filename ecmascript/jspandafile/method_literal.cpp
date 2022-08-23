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

#include "ecmascript/jspandafile/method_literal.h"

#include "ecmascript/jspandafile/js_pandafile.h"

#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
MethodLiteral::MethodLiteral(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile != nullptr) {
        panda_file::MethodDataAccessor mda(*(jsPandaFile->GetPandaFile()), methodId);
        auto codeId = mda.GetCodeId().value();
        if (!codeId.IsValid()) {
            nativePointerOrBytecodeArray_ = nullptr;
        }
        panda_file::CodeDataAccessor cda(*(jsPandaFile->GetPandaFile()), codeId);
        nativePointerOrBytecodeArray_ = cda.GetInstructions();
    }
    SetHotnessCounter(static_cast<int16_t>(0));
    SetMethodId(methodId);
    UpdateSlotSize(static_cast<uint8_t>(0));
}

void MethodLiteral::InitializeCallField(const JSPandaFile *jsPandaFile, uint32_t numVregs, uint32_t numArgs)
{
    uint32_t callType = UINT32_MAX;  // UINT32_MAX means not found
    const panda_file::File *pandaFile = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pandaFile, GetMethodId());
    mda.EnumerateAnnotations([&](EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(*pandaFile, annotation_id);
        auto *annotation_name = reinterpret_cast<const char *>(pandaFile->GetStringData(ada.GetClassId()).data);
        if (::strcmp("L_ESCallTypeAnnotation;", annotation_name) == 0) {
            uint32_t elem_count = ada.GetCount();
            for (uint32_t i = 0; i < elem_count; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elem_name = reinterpret_cast<const char *>(pandaFile->GetStringData(adae.GetNameId()).data);
                if (::strcmp("callType", elem_name) == 0) {
                    callType = adae.GetScalarValue().GetValue();
                }
            }
        }
    });
    // Needed info for call can be got by loading callField only once.
    // Native bit will be set in NewMethodForNativeFunction();
    callField_ = (callType & CALL_TYPE_MASK) |
                 NumVregsBits::Encode(numVregs) |
                 NumArgsBits::Encode(numArgs - HaveFuncBit::Decode(callType)  // exclude func
                                             - HaveNewTargetBit::Decode(callType)  // exclude new target
                                             - HaveThisBit::Decode(callType));  // exclude this
}

// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
std::string MethodLiteral::ParseFunctionName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return std::string();
    }
    std::string methodName(utf::Mutf8AsCString(GetName(jsPandaFile, methodId).data));
    if (LIKELY(methodName[0] != '#')) {
        return methodName;
    }
    size_t index = methodName.find_last_of('#');
    return methodName.substr(index + 1);
}

const char *MethodLiteral::GetMethodName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return "";
    }
    return utf::Mutf8AsCString(GetName(jsPandaFile, methodId).data);
}

panda_file::File::StringData MethodLiteral::GetName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    panda_file::MethodDataAccessor mda(*(jsPandaFile->GetPandaFile()), methodId);
    return jsPandaFile->GetPandaFile()->GetStringData(mda.GetNameId());
}

uint32_t MethodLiteral::GetNumVregs(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral)
{
    if (jsPandaFile == nullptr) {
        return 0;
    }
    panda_file::MethodDataAccessor mda(*(jsPandaFile->GetPandaFile()), methodLiteral->GetMethodId());
    auto codeId = mda.GetCodeId().value();
    if (!codeId.IsValid()) {
        return 0;
    }
    panda_file::CodeDataAccessor cda(*(jsPandaFile->GetPandaFile()), codeId);
    return cda.GetNumVregs();
}

uint32_t MethodLiteral::GetCodeSize(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return 0;
    }
    const panda_file::File *pandaFile = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pandaFile, methodId);
    auto codeId = mda.GetCodeId().value();
    if (!codeId.IsValid()) {
        return 0;
    }
    panda_file::CodeDataAccessor cda(*pandaFile, codeId);
    return cda.GetCodeSize();
}
} // namespace panda::ecmascript
