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
#include "ecmascript/patch/quick_fix_manager.h"

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
QuickFixManager::~QuickFixManager()
{
    methodInfos_.clear();
}

void QuickFixManager::RegisterQuickFixQueryFunc(const QuickFixQueryCallBack callBack)
{
    callBack_ = callBack;
}

void QuickFixManager::LoadPatchIfNeeded(JSThread *thread, const std::string &baseFileName)
{
    // callback and load patch.
    std::string patchFileName;
    void *patchBuffer = nullptr;
    size_t patchSize = 0;
    if (HasQueryQuickFixInfoFunc()) {
        bool needLoadPatch = callBack_(baseFileName, patchFileName, &patchBuffer, patchSize);
        if (needLoadPatch && !HasLoadedPatch(patchFileName, baseFileName)) {
            LoadPatch(thread, patchFileName, patchBuffer, patchSize, baseFileName);
        }
    }
}

PatchErrorCode QuickFixManager::LoadPatch(JSThread *thread, const std::string &patchFileName,
                                          const std::string &baseFileName)
{
    LOG_ECMA(INFO) << "Load patch, patch: " << patchFileName << ", base:" << baseFileName;
    if (methodInfos_.find(baseFileName.c_str()) != methodInfos_.end()) {
        LOG_ECMA(ERROR) << "Cannot repeat load patch!";
        return PatchErrorCode::PATCH_HAS_LOADED;
    }

    const JSPandaFile *baseFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFileName.c_str(), "");
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return PatchErrorCode::FILE_NOT_EXECUTED;
    }

    // The entry point is not work for merge abc.
    const JSPandaFile *patchFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, patchFileName.c_str(), "");
    if (patchFile == nullptr) {
        LOG_ECMA(ERROR) << "load patch jsPandafile failed";
        return PatchErrorCode::FILE_NOT_FOUND;
    }

    PatchInfo patchInfo;
    auto ret = PatchLoader::LoadPatchInternal(thread, baseFile, patchFile, patchInfo);
    if (ret != PatchErrorCode::SUCCESS) {
        LOG_ECMA(ERROR) << "Load patch fail!";
        return ret;
    }

    methodInfos_.emplace(baseFileName.c_str(), patchInfo);
    LOG_ECMA(INFO) << "Load patch success!";
    return PatchErrorCode::SUCCESS;
}

PatchErrorCode QuickFixManager::LoadPatch(JSThread *thread, const std::string &patchFileName,
                                          const void *patchBuffer, size_t patchSize, const std::string &baseFileName)
{
    LOG_ECMA(INFO) << "Load patch, patch: " << patchFileName << ", base:" << baseFileName;
    if (HasLoadedPatch(patchFileName, baseFileName)) {
        LOG_ECMA(ERROR) << "Cannot repeat load patch!";
        return PatchErrorCode::PATCH_HAS_LOADED;
    }

    const JSPandaFile *baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName.c_str());
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return PatchErrorCode::FILE_NOT_EXECUTED;
    }

    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(
        thread, patchFileName.c_str(), "", patchBuffer, patchSize);
    if (patchFile == nullptr) {
        LOG_ECMA(ERROR) << "load patch jsPandafile failed";
        return PatchErrorCode::FILE_NOT_FOUND;
    }

    PatchInfo patchInfo;
    auto ret = PatchLoader::LoadPatchInternal(thread, baseFile, patchFile, patchInfo);
    if (ret != PatchErrorCode::SUCCESS) {
        LOG_ECMA(ERROR) << "Load patch fail!";
        return ret;
    }

    methodInfos_.emplace(baseFileName.c_str(), patchInfo);
    LOG_ECMA(INFO) << "Load patch success!";
    return PatchErrorCode::SUCCESS;
}

PatchErrorCode QuickFixManager::UnloadPatch(JSThread *thread, const std::string &patchFileName)
{
    LOG_ECMA(INFO) << "Unload patch, patch: " << patchFileName;
    CString baseFileName;
    for (const auto &item : methodInfos_) {
        if (item.second.patchFileName == patchFileName.c_str()) {
            baseFileName = item.first;
        }
    }
    if (baseFileName.empty()) {
        LOG_ECMA(ERROR) << "patch has not been loaded!";
        return PatchErrorCode::PATCH_NOT_LOADED;
    }

    PatchInfo &patchInfo = methodInfos_.find(baseFileName)->second;
    auto ret = PatchLoader::UnloadPatchInternal(thread, patchFileName.c_str(), baseFileName.c_str(), patchInfo);
    if (ret != PatchErrorCode::SUCCESS) {
        LOG_ECMA(ERROR) << "Unload patch fail!";
        return ret;
    }

    methodInfos_.erase(baseFileName.c_str());
    LOG_ECMA(INFO) << "Unload patch success!";
    return PatchErrorCode::SUCCESS;
}

bool QuickFixManager::HasLoadedPatch(const std::string &patchFileName, const std::string &baseFileName) const
{
    auto iter = methodInfos_.find(baseFileName.c_str());
    if (iter == methodInfos_.end()) {
        return false;
    }

    if (iter->second.patchFileName != patchFileName.c_str()) {
        return false;
    }
    return true;
}

JSTaggedValue QuickFixManager::CheckAndGetPatch(JSThread *thread, const JSPandaFile *baseFile, EntityId baseMethodId)
{
    if (methodInfos_.empty()) {
        return JSTaggedValue::Hole();
    }

    auto iter = methodInfos_.find(baseFile->GetJSPandaFileDesc());
    if (iter == methodInfos_.end()) {
        return JSTaggedValue::Hole();
    }

    PatchInfo patchInfo = iter->second;
    MethodLiteral *patchMethodLiteral = PatchLoader::FindSameMethod(patchInfo, baseFile, baseMethodId);
    if (patchMethodLiteral == nullptr) {
        return JSTaggedValue::Hole();
    }

    // Generate patch constpool.
    CString patchFileName = patchInfo.patchFileName;
    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName);
    ASSERT(patchFile != nullptr);

    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<Method> method = vm->GetFactory()->NewMethod(patchMethodLiteral);
    JSHandle<ConstantPool> newConstpool = vm->FindOrCreateConstPool(patchFile, patchMethodLiteral->GetMethodId());
    method->SetConstantPool(thread, newConstpool);
    return method.GetTaggedValue();
}

bool QuickFixManager::IsQuickFixCausedException(JSThread *thread,
                                                const JSHandle<JSTaggedValue> &exceptionInfo,
                                                const std::string &patchFileName)
{
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const JSPandaFile* patchFile = pfManager->FindJSPandaFile(ConvertToString(patchFileName));
    if (patchFile == nullptr || ConvertToString(patchFileName) != patchFile->GetJSPandaFileDesc()) {
        return false;
    }

    // get and parse stackinfo.
    JSHandle<JSTaggedValue> stackKey = thread->GlobalConstants()->GetHandledStackString();
    JSHandle<EcmaString> stack(JSObject::GetProperty(thread, exceptionInfo, stackKey).GetValue());
    CString stackInfo = ConvertToString(*stack);
    CUnorderedSet<CString> methodNames = ParseStackInfo(stackInfo);

    // check whether the methodNames contains a patch method name.
    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile->GetMethodLiteralMap();
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile, methodId);
        if (std::strcmp(patchMethodName, JSPandaFile::ENTRY_FUNCTION_NAME) != 0 &&
            methodNames.find(CString(patchMethodName)) != methodNames.end()) {
            return true;
        }
    }
    return false;
}

CUnorderedSet<CString> QuickFixManager::ParseStackInfo(const CString &stackInfo)
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