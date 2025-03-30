/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/module.h"

#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript {
CString GetOutEntryPoint(EcmaVM *vm, const CString &inputFileName)
{
    CString outEntryPoint;
    // if the inputFileName starts with '.test', the preview test page is started.
    // in this case, the path ets does not need to be combined.
    // inputFileName: .test/xxx/xxx.abc
    if (StringHelper::StringStartWith(inputFileName, ModulePathHelper::PREVIER_TEST_DIR)) {
        outEntryPoint = base::ConcatToCString(vm->GetBundleName(), PathHelper::SLASH_TAG, vm->GetModuleName(),
            PathHelper::SLASH_TAG, inputFileName);
    } else {
        // inputFileName: xxx/xxx.abc
        outEntryPoint = base::ConcatToCString(vm->GetBundleName(), PathHelper::SLASH_TAG, vm->GetModuleName(),
            ModulePathHelper::MODULE_DEFAULE_ETS, inputFileName);
    }
    return outEntryPoint;
}

CString GetBaseFileName(EcmaVM *vm, [[maybe_unused]] CString &moduleRequestName, CString &baseFileName,
    const CString &bundleName, const CString &moduleName, [[maybe_unused]] CString &recordName)
{
    CVector<CString> currentVec;
    StringHelper::SplitString(moduleRequestName, currentVec, 0, ModulePathHelper::SEGMENTS_LIMIT_TWO);
    if (currentVec.size() < ModulePathHelper::SEGMENTS_LIMIT_TWO) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << " Exceptional module path : " << moduleRequestName << ", abc path: " <<
            baseFileName << ", current file name: " << recordName;
    }
    CString currentModuleName = currentVec[1];
    PathHelper::DeleteNamespace(currentModuleName);
    if (bundleName != vm->GetBundleName()) {
        baseFileName = base::ConcatToCString(ModulePathHelper::BUNDLE_INSTALL_PATH, bundleName, PathHelper::SLASH_TAG,
            moduleName, PathHelper::SLASH_TAG, moduleName, ModulePathHelper::MERGE_ABC_ETS_MODULES);
    } else if (currentModuleName != vm->GetModuleName()) {
        baseFileName = base::ConcatToCString(ModulePathHelper::BUNDLE_INSTALL_PATH, moduleName,
            ModulePathHelper::MERGE_ABC_ETS_MODULES);
    }
    return baseFileName;
}

JSHandle<JSTaggedValue> CheckEntryPointPreview(JSThread *thread, [[maybe_unused]] const CString &entryPoint)
{
    if (entryPoint == ModulePathHelper::PREVIEW_OF_ACROSS_HAP_FLAG) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread, "", thread->GlobalConstants()->GetHandledUndefined());
    }
    return thread->GlobalConstants()->GetHandledNull();
}
} // namespace panda::ecmascript