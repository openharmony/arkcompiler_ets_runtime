/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/interpreter/interpreter.h"
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

void MethodLiteral::Initialize(const JSPandaFile *jsPandaFile, const JSThread *thread, const uint32_t offset)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    EntityId methodId = GetMethodId();
    if (UNLIKELY(offset != 0 && methodId.GetOffset() != offset)) {
        LOG_ECMA(FATAL) << "Invalid methodId, expected methodId: " << offset << ", actual methodId: " << methodId;
    }
    panda_file::MethodDataAccessor mda(*pf, methodId);
    auto codeId = mda.GetCodeId().value();
    ASSERT(codeId.IsValid());

    panda_file::CodeDataAccessor cda(*pf, codeId);
    nativePointerOrBytecodeArray_ = cda.GetInstructions();
    uint32_t codeSize = cda.GetCodeSize();
    // When triggering jit compile through the execution count of the js function, set the hotness counter value to 0
    // to ensure that the profile type info object can be created on the first execution of the js function.
    bool cancelThreshold = (thread != nullptr && thread->GetEcmaVM()->GetJSOptions().GetJitCallThreshold() != 0);
    SetHotnessCounter(EcmaInterpreter::GetHotnessCounter(codeSize, cancelThreshold));

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
    ASSERT((numArgs - HaveFuncBit::Decode(callType) -
        HaveNewTargetBit::Decode(callType) - HaveThisBit::Decode(callType)) >= 0);
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
std::string MethodLiteral::ParseFunctionName(const JSPandaFile* jsPandaFile, EntityId methodId)
{
    std::string_view methodName = ParseFunctionNameView(jsPandaFile, methodId).first;
    return std::string(methodName);
}

// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
std::pair<std::string_view, bool> MethodLiteral::ParseFunctionNameView(
    const JSPandaFile* jsPandaFile, EntityId methodId)
{
    if (UNLIKELY(jsPandaFile == nullptr)) {
        return {"", true};
    }

    auto [methodName, isASCII] = GetMethodNameView(jsPandaFile, methodId);
    if (LIKELY(methodName[0] != '#')) {
        return {methodName, isASCII};
    }

    size_t index = methodName.find_last_of('#');
    methodName = methodName.substr(index + 1); // #...#functionName
    if (index = methodName.find_last_of('^'); index != std::string::npos) {
        methodName = methodName.substr(0, index); // #...#functionName^1
    }
    return {methodName, isASCII};
}

// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
CString MethodLiteral::ParseFunctionNameToCString(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return "";
    }

    CString methodName(GetMethodName(jsPandaFile, methodId));
    if (LIKELY(methodName[0] != '#')) {
        return methodName;
    }

    size_t index = methodName.find_last_of('#');
    methodName = methodName.substr(index + 1);  // #...#functionName
    if (methodName.find('^') != std::string::npos) {
        index = methodName.find_last_of('^');
        methodName = methodName.substr(0, index);  // #...#functionName^1
    }
    return methodName;
}

const char* MethodLiteral::GetMethodName(const JSPandaFile* jsPandaFile, EntityId methodId, bool cpuProfiler)
{
    if (jsPandaFile == nullptr) {
        return "";
    }
    return GetMethodNameView(jsPandaFile, methodId, cpuProfiler).first.data();
}

std::pair<std::string_view, bool> MethodLiteral::GetMethodNameView(
    const JSPandaFile* jsPandaFile, EntityId methodId, bool cpuProfiler)
{
    ASSERT(jsPandaFile != nullptr && "jsPandaFile is null");
    if (cpuProfiler) {
        return jsPandaFile->GetCpuProfilerMethodName(methodId);
    }
    return const_cast<JSPandaFile*>(jsPandaFile)->GetMethodName(methodId);
}

CString MethodLiteral::GetRecordName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    if (jsPandaFile == nullptr) {
        return "";
    }

    return const_cast<JSPandaFile *>(jsPandaFile)->GetRecordName(methodId);
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

std::optional<std::set<uint32_t>> MethodLiteral::GetConcurrentRequestedModules(const JSPandaFile *jsPandaFile) const
{
    ASSERT(jsPandaFile != nullptr);
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    EntityId methodId = GetMethodId();
    panda_file::MethodDataAccessor mda(*pf, methodId);
    std::set<uint32_t> requestedModules;
    bool hasRequestedModules = false;
    mda.EnumerateAnnotations([&](EntityId annotationId) {
        panda_file::AnnotationDataAccessor ada(*pf, annotationId);
        auto *annotationName = reinterpret_cast<const char *>(pf->GetStringData(ada.GetClassId()).data);
        if (::strcmp("L_ESConcurrentModuleRequestsAnnotation;", annotationName) == 0) {
            hasRequestedModules = true;
            uint32_t elemCount = ada.GetCount();
            for (uint32_t i = 0; i < elemCount; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elemName = reinterpret_cast<const char *>(pf->GetStringData(adae.GetNameId()).data);
                if (::strcmp("ConcurrentModuleRequest", elemName) == 0) {
                    uint32_t index = adae.GetScalarValue().GetValue();
                    requestedModules.insert(index);
                }
            }
        }
    });
    if (!hasRequestedModules) {
        return std::nullopt;
    }
    return requestedModules;
}
} // namespace panda::ecmascript
