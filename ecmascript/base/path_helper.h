/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_BASE_PATH_HELPER_H
#define ECMASCRIPT_BASE_PATH_HELPER_H

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::base {
class PathHelper {
public:
    static void ResolveCurrentPath(JSThread *thread,
                                   JSMutableHandle<JSTaggedValue> &dirPath,
                                   JSMutableHandle<JSTaggedValue> &fileName,
                                   const JSPandaFile *jsPandaFile)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        CString fullName = jsPandaFile->GetJSPandaFileDesc();
        // find last '/'
        int foundPos = static_cast<int>(fullName.find_last_of("/\\"));
        if (foundPos == -1) {
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        CString dirPathStr = fullName.substr(0, foundPos + 1);
        JSHandle<EcmaString> dirPathName = factory->NewFromUtf8(dirPathStr);
        dirPath.Update(dirPathName.GetTaggedValue());

        // Get filename from JSPandaFile
        JSHandle<EcmaString> cbFileName = factory->NewFromUtf8(fullName);
        fileName.Update(cbFileName.GetTaggedValue());
    }

    static JSHandle<EcmaString> ResolveDirPath(JSThread *thread,
                                               JSHandle<JSTaggedValue> fileName)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        CString fullName = ConvertToString(fileName.GetTaggedValue());
        // find last '/'
        int foundPos = static_cast<int>(fullName.find_last_of("/\\"));
        if (foundPos == -1) {
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
        }
        CString dirPathStr = fullName.substr(0, foundPos + 1);
        return factory->NewFromUtf8(dirPathStr);
    }
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_PATH_HELPER_H