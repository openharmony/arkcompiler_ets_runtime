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

#ifndef ECMASCRIPT_MODULE_NAPI_MODULE_LOADER_H
#define ECMASCRIPT_MODULE_NAPI_MODULE_LOADER_H

#include "ecmascript/js_tagged_value-inl.h"
namespace panda::ecmascript {
class NapiModuleLoader {
public:
    template<ForHybridApp isHybrid = ForHybridApp::Normal>
    static JSHandle<JSTaggedValue> LoadModuleNameSpace(EcmaVM *vm, CString requestPath, const CString &moduleName,
                                                       CString &abcFilePath);
    template<ForHybridApp isHybrid = ForHybridApp::Normal>
    static JSHandle<JSTaggedValue> LoadModuleNameSpace(EcmaVM *vm, CString requestPath,
                                                       CString modulePath);

    static JSHandle<JSTaggedValue> GetModuleNameSpace(JSThread *thread, const CString &entryPoint,
                                                      const CString &abcFilePath);

    template<ForHybridApp isHybrid = ForHybridApp::Normal>
    static JSHandle<JSTaggedValue> LoadModuleNameSpaceFromFile(JSThread *thread, const CString &entryPoint,
                                                               const CString &abcFilePath);
private:
    template<ForHybridApp isHybrid = ForHybridApp::Normal>
    static JSHandle<JSTaggedValue> LoadModuleNameSpaceWithModuleInfo(
        EcmaVM *vm, CString &requestPath, CString &modulePath,
        CString &abcFilePath);
    
    static JSHandle<JSTaggedValue> LoadModuleNameSpaceWithPath(
        JSThread *thread, CString &abcFilePath, const CString &requestPath,
        const CString &modulePath, const JSPandaFile *pandaFile);
    
    template<ForHybridApp isHybrid = ForHybridApp::Normal>
    static JSHandle<JSTaggedValue> LoadFilePathWithinModule(
        JSThread *thread, const CString &abcFilePath, const CString &srcPrefix,
        const CString &requestPath, const CString &modulePath);
};
}

#endif  // ECMASCRIPT_MODULE_NAPI_MODULE_LOADER_H