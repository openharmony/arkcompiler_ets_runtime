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
#include "ecmascript/jspandafile/js_patch_manager.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
bool JSPatchManager::LoadPatch(JSThread *thread, const std::string &patchFileName, const std::string &baseFileName)
{
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    EcmaVM *vm = thread->GetEcmaVM();

    // Get base constpool.
    baseFile_ = pfManager->FindJSPandaFile(ConvertToString(baseFileName));
    if (baseFile_ == nullptr) {
        return false;
    }
    // TODO: use "for"
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_, 0);
    if (baseConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> baseConstpool(thread, baseConstpoolValue);

    // Resolve patch.abc and get patch constpool.
    patchFile_ = pfManager->LoadJSPandaFile(thread, ConvertToString(patchFileName), JSPandaFile::PATCH_ENTRY_FUNCTION);
    if (patchFile_ == nullptr) {
        return false;
    }
    JSHandle<Program> patchProgram =
        PandaFileTranslator::GenerateProgram(vm, patchFile_, JSPandaFile::ENTRY_FUNCTION_NAME);
    JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile_, 0);
    if (patchConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> patchConstpool(thread, patchConstpoolValue);

    return ReplaceMethod(thread, baseConstpool, patchConstpool, patchProgram);
}

bool JSPatchManager::LoadPatch(JSThread *thread, const std::string &patchFileName, const void *patchBuffer,
                               size_t patchSize, const std::string &baseFileName)
{
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    EcmaVM *vm = thread->GetEcmaVM();

    // Get base constpool.
    baseFile_ = pfManager->FindJSPandaFile(ConvertToString(baseFileName));
    if (baseFile_ == nullptr) {
        return false;
    }
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_, 0);
    if (baseConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> baseConstpool(thread, baseConstpoolValue);

    // Resolve patch.abc and get patch constpool.
    patchFile_ = pfManager->LoadJSPandaFile(
        thread, ConvertToString(patchFileName), JSPandaFile::PATCH_ENTRY_FUNCTION, patchBuffer, patchSize);
    if (patchFile_ == nullptr) {
        return false;
    }

    JSHandle<Program> patchProgram =
        PandaFileTranslator::GenerateProgram(vm, patchFile_, JSPandaFile::ENTRY_FUNCTION_NAME);
    JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile_, 0);
    if (patchConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> patchConstpool(thread, patchConstpoolValue);

    return ReplaceMethod(thread, baseConstpool, patchConstpool, patchProgram);
}

bool JSPatchManager::ReplaceMethod(JSThread *thread,
                                   const JSHandle<ConstantPool> &baseConstpool,
                                   const JSHandle<ConstantPool> &patchConstpool,
                                   const JSHandle<Program> &patchProgram)
{
    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile_->GetMethodLiteralMap();
    auto baseConstpoolSize = baseConstpool->GetLength();

    // Find same method both in base and patch.
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile_, methodId);

        for (uint32_t index = 1; index < baseConstpoolSize; index++) { // 1: first value is JSPandaFile.
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(index);
            if (!constpoolValue.IsJSFunctionBase()) {
                continue;
            }

            JSFunctionBase *func = JSFunctionBase::Cast(constpoolValue.GetTaggedObject());
            JSHandle<Method> baseMethod(thread, func->GetMethod());
            if (std::strcmp(patchMethodName, baseMethod->GetMethodName()) != 0) {
                continue;
            }

            // Save base MethodLiteral and constpool index.
            MethodLiteral *base = baseFile_->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
            ASSERT(base != nullptr);
            reservedBaseInfo_.emplace(index, base);

            // Replace base method.
            baseMethod->SetCallField(patch->GetCallField());
            baseMethod->SetLiteralInfo(patch->GetLiteralInfo());
            baseMethod->SetNativePointerOrBytecodeArray(const_cast<void *>(patch->GetNativePointer()));
            baseMethod->SetConstantPool(thread, patchConstpool.GetTaggedValue());
        }
    }

    if (reservedBaseInfo_.empty()) {
        return false;
    }

    // Call patch_main_0 for newly function.
    if (!patchProgram->GetMainFunction().IsUndefined()) {
        JSHandle<JSTaggedValue> global = thread->GetEcmaVM()->GetGlobalEnv()->GetJSGlobalObject();
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> func(thread, patchProgram->GetMainFunction());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, func, global, undefined, 0);
        EcmaInterpreter::Execute(info);
    }
    return true;
}

bool JSPatchManager::UnLoadPatch(JSThread *thread, const std::string &patchFileName)
{
    if ((ConvertToString(patchFileName) != patchFile_->GetJSPandaFileDesc()) ||
        (reservedBaseInfo_.empty())) {
        return false;
    }

    EcmaVM *vm = thread->GetEcmaVM();
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_, 0);
    if (baseConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> baseConstpool(thread, baseConstpoolValue);

    for (const auto& item : reservedBaseInfo_) {
        uint32_t constpoolIndex = item.first;
        MethodLiteral *base = item.second;

        JSTaggedValue value = baseConstpool->Get(thread, constpoolIndex);
        ASSERT(value.IsJSFunctionBase());
        JSHandle<JSFunction> func(thread, value);
        JSHandle<Method> method(thread, func->GetMethod());

        method->SetCallField(base->GetCallField());
        method->SetLiteralInfo(base->GetLiteralInfo());
        method->SetNativePointerOrBytecodeArray(const_cast<void *>(base->GetNativePointer()));
        method->SetConstantPool(thread, baseConstpool.GetTaggedValue());
    }
    return true;
}
}  // namespace panda::ecmascript