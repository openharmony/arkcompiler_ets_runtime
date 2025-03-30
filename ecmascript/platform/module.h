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

#ifndef ECMASCRIPT_PLATFORM_MODULE_H
#define ECMASCRIPT_PLATFORM_MODULE_H

#include "ecmascript/base/path_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
    using PathHelper = base::PathHelper;

    CString GetOutEntryPoint(EcmaVM *vm, const CString &inputFileName);
    CString GetBaseFileName(EcmaVM *vm, [[maybe_unused]] CString &moduleRequestName, CString &baseFileName,
        const CString &bundleName, const CString &moduleName, [[maybe_unused]] CString &recordName);
    JSHandle<JSTaggedValue> CheckEntryPointPreview(JSThread *thread, [[maybe_unused]] const CString &entryPoint);
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_MODULE_H