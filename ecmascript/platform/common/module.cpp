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
    // inputFileName: moduleName/ets/xxx/xxx.abc
    return vm->GetBundleName() + PathHelper::SLASH_TAG + inputFileName;
}

CString GetBaseFileName(EcmaVM *vm, [[maybe_unused]] CString &moduleRequestName, CString &baseFileName,
    const CString &bundleName, const CString &moduleName, [[maybe_unused]] CString &recordName)
{
    if (bundleName != vm->GetBundleName()) {
        baseFileName = ModulePathHelper::BUNDLE_INSTALL_PATH + bundleName + PathHelper::SLASH_TAG + moduleName +
                        PathHelper::SLASH_TAG + moduleName + ModulePathHelper::MERGE_ABC_ETS_MODULES;
    } else if (moduleName != vm->GetModuleName()) {
        baseFileName = ModulePathHelper::BUNDLE_INSTALL_PATH + moduleName + ModulePathHelper::MERGE_ABC_ETS_MODULES;
    } else {
        // Support multi-module card service
        baseFileName = vm->GetAssetPath();
    }
    return baseFileName;
}

JSHandle<JSTaggedValue> CheckEntryPointPreview(JSThread *thread, [[maybe_unused]] const CString &entryPoint)
{
    return thread->GlobalConstants()->GetHandledNull();
}
} // namespace panda::ecmascript