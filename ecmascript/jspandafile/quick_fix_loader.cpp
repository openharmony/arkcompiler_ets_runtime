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
#include "ecmascript/jspandafile/quick_fix_loader.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {
QuickFixLoader::~QuickFixLoader()
{
    ClearReservedInfo();
}

bool QuickFixLoader::LoadPatch(JSThread *thread, const std::string &patchFileName, const std::string &baseFileName)
{
    if (hasLoadedPatch_) {
        LOG_ECMA(ERROR) << "Cannot repeat load patch";
        return false;
    }
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    EcmaVM *vm = thread->GetEcmaVM();

    // Get base constpool.
    baseFile_ = pfManager->FindJSPandaFile(ConvertToString(baseFileName));
    if (baseFile_ == nullptr) {
        return false;
    }
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_);
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
    JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile_);
    if (patchConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> patchConstpool(thread, patchConstpoolValue);

    return ReplaceMethod(thread, baseConstpool, patchConstpool, patchProgram);
}

bool QuickFixLoader::LoadPatch(JSThread *thread, const std::string &patchFileName, const void *patchBuffer,
                               size_t patchSize, const std::string &baseFileName)
{
    if (hasLoadedPatch_) {
        LOG_ECMA(ERROR) << "Cannot repeat load patch";
        return false;
    }
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    EcmaVM *vm = thread->GetEcmaVM();

    // Get base constpool.
    baseFile_ = pfManager->FindJSPandaFile(ConvertToString(baseFileName));
    if (baseFile_ == nullptr) {
        return false;
    }
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_);
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
    JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile_);
    if (patchConstpoolValue.IsHole()) {
        return false;
    }
    JSHandle<ConstantPool> patchConstpool(thread, patchConstpoolValue);

    return ReplaceMethod(thread, baseConstpool, patchConstpool, patchProgram);
}

bool QuickFixLoader::ReplaceMethod(JSThread *thread,
                                   const JSHandle<ConstantPool> &baseConstpool,
                                   const JSHandle<ConstantPool> &patchConstpool,
                                   const JSHandle<Program> &patchProgram)
{
    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile_->GetMethodLiteralMap();
    auto baseConstpoolSize = baseConstpool->GetCacheLength();

    // Find same method both in base and patch.
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile_, methodId);

        for (uint32_t index = 0; index < baseConstpoolSize; index++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(index);
            // For class inner function modified.
            if (constpoolValue.IsMethod()) {
                JSHandle<TaggedArray> classLiteral(thread, baseConstpool->GetObjectFromCache(index + 1));
                CUnorderedMap<uint32_t, MethodLiteral *> classLiteralInfo;
                for (uint32_t i = 0; i < classLiteral->GetLength(); i++) {
                    JSTaggedValue literalItem = classLiteral->Get(thread, i);
                    if (!literalItem.IsJSFunctionBase()) {
                        continue;
                    }
                    JSFunctionBase *func = JSFunctionBase::Cast(literalItem.GetTaggedObject());
                    Method *baseMethod = Method::Cast(func->GetMethod().GetTaggedObject());
                    if (std::strcmp(patchMethodName, baseMethod->GetMethodName()) != 0) {
                        continue;
                    }

                    // Save base MethodLiteral and constpool index.
                    MethodLiteral *base = baseFile_->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
                    ASSERT(base != nullptr);
                    classLiteralInfo.emplace(i, base);
                    reservedBaseClassInfo_.emplace(index, classLiteralInfo);

                    ReplaceMethodInner(thread, baseMethod, patch, patchConstpool.GetTaggedValue());
                }
            }
            // For function modified.
            if (constpoolValue.IsJSFunctionBase()) {
                JSFunctionBase *func = JSFunctionBase::Cast(constpoolValue.GetTaggedObject());
                Method *baseMethod = Method::Cast(func->GetMethod().GetTaggedObject());
                if (std::strcmp(patchMethodName, baseMethod->GetMethodName()) != 0) {
                    continue;
                }

                // Save base MethodLiteral and constpool index.
                MethodLiteral *base = baseFile_->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
                ASSERT(base != nullptr);
                reservedBaseMethodInfo_.emplace(index, base);

                ReplaceMethodInner(thread, baseMethod, patch, patchConstpool.GetTaggedValue());
            }
        }
    }

    if (reservedBaseMethodInfo_.empty() && reservedBaseClassInfo_.empty()) {
        return false;
    }

    // For add a new function, Call patch_main_0.
    if (!patchProgram->GetMainFunction().IsUndefined()) {
        JSHandle<JSTaggedValue> global = thread->GetEcmaVM()->GetGlobalEnv()->GetJSGlobalObject();
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> func(thread, patchProgram->GetMainFunction());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, func, global, undefined, 0);
        EcmaInterpreter::Execute(info);
    }
    hasLoadedPatch_ = true;
    return true;
}

bool QuickFixLoader::UnLoadPatch(JSThread *thread, const std::string &patchFileName)
{
    if (ConvertToString(patchFileName) != patchFile_->GetJSPandaFileDesc()) {
        return false;
    }

    if (reservedBaseMethodInfo_.empty() && reservedBaseClassInfo_.empty()) {
        return false;
    }

    EcmaVM *vm = thread->GetEcmaVM();
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_);
    if (baseConstpoolValue.IsHole()) {
        return false;
    }

    ConstantPool *baseConstpool = ConstantPool::Cast(baseConstpoolValue.GetTaggedObject());
    for (const auto& item : reservedBaseMethodInfo_) {
        uint32_t constpoolIndex = item.first;
        MethodLiteral *base = item.second;

        JSTaggedValue value = baseConstpool->GetObjectFromCache(constpoolIndex);
        ASSERT(value.IsJSFunctionBase());
        JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
        Method *method = Method::Cast(func->GetMethod().GetTaggedObject());

        ReplaceMethodInner(thread, method, base, baseConstpoolValue);
    }

    for (const auto& item : reservedBaseClassInfo_) {
        uint32_t constpoolIndex = item.first;
        CUnorderedMap<uint32_t, MethodLiteral *> classLiteralInfo = item.second;
        JSHandle<TaggedArray> classLiteral(thread, baseConstpool->GetObjectFromCache(constpoolIndex + 1));

        for (const auto& classItem : classLiteralInfo) {
            MethodLiteral *base = classItem.second;

            JSTaggedValue value = classLiteral->Get(thread, classItem.first);
            ASSERT(value.IsJSFunctionBase());
            JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
            Method *method = Method::Cast(func->GetMethod().GetTaggedObject());

            ReplaceMethodInner(thread, method, base, baseConstpoolValue);
        }
    }

    ClearReservedInfo();
    hasLoadedPatch_ = false;
    return true;
}

void QuickFixLoader::ReplaceMethodInner(JSThread *thread,
                                        Method* destMethod,
                                        MethodLiteral *srcMethodLiteral,
                                        JSTaggedValue srcConstpool)
{
    destMethod->SetCallField(srcMethodLiteral->GetCallField());
    destMethod->SetLiteralInfo(srcMethodLiteral->GetLiteralInfo());
    destMethod->SetNativePointerOrBytecodeArray(const_cast<void *>(srcMethodLiteral->GetNativePointer()));
    destMethod->SetConstantPool(thread, srcConstpool);
}

bool QuickFixLoader::IsQuickFixCausedException(JSThread *thread,
                                               const JSHandle<JSTaggedValue> &exceptionInfo,
                                               const std::string &patchFileName)
{
    if (patchFile_ == nullptr || ConvertToString(patchFileName) != patchFile_->GetJSPandaFileDesc()) {
        return false;
    }

    // get and parse stackinfo.
    JSHandle<JSTaggedValue> stackKey = thread->GlobalConstants()->GetHandledStackString();
    JSHandle<EcmaString> stack(JSObject::GetProperty(thread, exceptionInfo, stackKey).GetValue());
    CString stackInfo = ConvertToString(*stack);
    CUnorderedSet<CString> methodNames = ParseStackInfo(stackInfo);

    // check whether the methodNames contains a patch method name.
    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile_->GetMethodLiteralMap();
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile_, methodId);
        if (std::strcmp(patchMethodName, JSPandaFile::ENTRY_FUNCTION_NAME) != 0 &&
            methodNames.find(CString(patchMethodName)) != methodNames.end()) {
            return true;
        }
    }
    return false;
}

CUnorderedSet<CString> QuickFixLoader::ParseStackInfo(const CString &stackInfo)
{
    const uint32_t methodNameOffsetToFirstIndex = 5; // offset of the starting position of methodname to firstIndex.
    uint32_t lineIndex = 0; // index of "\n".
    uint32_t firstIndex = 0; // index of "at".
    uint32_t nextIndex = 0; // index of "(".

    CUnorderedSet<CString> methodNames {}; // method names are from exception stack information.
    while (lineIndex != stackInfo.length() - 1) {
        firstIndex = stackInfo.find("  at ", lineIndex + 1);
        nextIndex = stackInfo.find("(", lineIndex + 1);
        CString methodName = stackInfo.substr(firstIndex + methodNameOffsetToFirstIndex,
            nextIndex - firstIndex - methodNameOffsetToFirstIndex - 1);
        methodNames.emplace(methodName);
        lineIndex = stackInfo.find("\n", lineIndex + 1);
    }
    return methodNames;
}
}  // namespace panda::ecmascript
