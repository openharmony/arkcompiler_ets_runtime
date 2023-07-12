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

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"

#include "libpandafile/class_data_accessor.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
MethodLiteral::MethodLiteral(EntityId methodId)
{
    ASSERT(methodId.IsValid());
    SetMethodId(methodId);
}

void MethodLiteral::Initialize(const JSPandaFile *jsPandaFile)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    EntityId methodId = GetMethodId();
    panda_file::MethodDataAccessor mda(*pf, methodId);
    auto codeId = mda.GetCodeId().value();
    ASSERT(codeId.IsValid());

    panda_file::CodeDataAccessor cda(*pf, codeId);
    nativePointerOrBytecodeArray_ = cda.GetInstructions();
    uint32_t codeSize = cda.GetCodeSize();
    SetHotnessCounter(EcmaInterpreter::GetHotnessCounter(codeSize));

    uint32_t callType = UINT32_MAX;  // UINT32_MAX means not found
    uint32_t slotSize = 0;
    mda.EnumerateAnnotations([&](EntityId annotationId) {
        panda_file::AnnotationDataAccessor ada(*pf, annotationId);
        auto *annotationName = reinterpret_cast<const char *>(pf->GetStringData(ada.GetClassId()).data);
        if (::strcmp("L_ESCallTypeAnnotation;", annotationName) == 0) {
            uint32_t elemCount = ada.GetCount();
            for (uint32_t i = 0; i < elemCount; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elemName = reinterpret_cast<const char *>(pf->GetStringData(adae.GetNameId()).data);
                if (::strcmp("callType", elemName) == 0) {
                    callType = adae.GetScalarValue().GetValue();
                }
            }
        } else if (::strcmp("L_ESSlotNumberAnnotation;", annotationName) == 0) {
            uint32_t elemCount = ada.GetCount();
            for (uint32_t i = 0; i < elemCount; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elemName = reinterpret_cast<const char *>(pf->GetStringData(adae.GetNameId()).data);
                if (::strcmp("SlotNumber", elemName) == 0) {
                    slotSize = adae.GetScalarValue().GetValue();
                }
            }
        }
    });

    uint32_t numVregs = cda.GetNumVregs();
    uint32_t numArgs = cda.GetNumArgs();
    // Needed info for call can be got by loading callField only once.
    // Native bit will be set in NewMethodForNativeFunction();
    callField_ = (callType & CALL_TYPE_MASK) |
                 NumVregsBits::Encode(numVregs) |
                 NumArgsBits::Encode(numArgs - HaveFuncBit::Decode(callType)  // exclude func
                                             - HaveNewTargetBit::Decode(callType)  // exclude new target
                                             - HaveThisBit::Decode(callType));  // exclude this
    SetSlotSize(slotSize);
}

// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
std::string MethodLiteral::ParseFunctionName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return std::string();
    }

    std::string methodName(GetMethodName(jsPandaFile, methodId));
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

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, methodId);
    auto sd = jsPandaFile->GetStringData(mda.GetNameId());
    return utf::Mutf8AsCString(sd.data);
}

CString MethodLiteral::GetRecordName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return "";
    }

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, methodId);
    panda_file::ClassDataAccessor cda(*pf, mda.GetClassId());
    CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
    return JSPandaFile::ParseEntryPoint(desc);
}

const char *MethodLiteral::GetRecordNameWithSymbol(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return "";
    }

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, methodId);
    panda_file::ClassDataAccessor cda(*pf, mda.GetClassId());
    return utf::Mutf8AsCString(cda.GetDescriptor());
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
