/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_MESSAGE_HELPER_H
#define ECMASCRIPT_MODULE_MESSAGE_HELPER_H

#include "ecmascript/base/string_helper.h"

namespace panda::ecmascript {

class ModuleMessageHelper {
public:
    static inline CString VmModuleInfoMessage(JSThread *thread)
    {
        EcmaVM *vm = thread->GetEcmaVM();
        return "bundleName:" + vm->GetBundleName() +
               ", moduleName:" + vm->GetModuleName() +
               ", assetPath:" + vm->GetAssetPath();
    }

    static void PrintAndThrowError(JSThread *thread, JSHandle<SourceTextModule> module)
    {
        // root error need throw error msg, for interface like pushurl(etc.) doesn't actually need ldmodulevar.
        LOG_FULL(ERROR) << "Error found in module:" << module->GetEcmaModuleRecordNameString();
        PrintErrorMessage(thread, module);
        auto &options = const_cast<EcmaVM *>(thread->GetEcmaVM())->GetJSOptions();
        if (options.EnableModuleException()) {
            THROW_NEW_ERROR_AND_RETURN(thread, module->GetException());
        }
    }

    static void PrintErrorMessage(JSThread *thread, JSHandle<SourceTextModule> module)
    {
        JSHandle<JSTaggedValue> exceptionInfo(thread, module->GetException());
        if (exceptionInfo->IsJSError()) {
            base::ErrorHelper::PrintJSErrorInfo(thread, exceptionInfo);
            return;
        }
        JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, exceptionInfo);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        CString message = ConvertToString(*str);
        LOG_ECMA(ERROR) << "Error occurs:" << message;
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_SHARED_MODULE_H
