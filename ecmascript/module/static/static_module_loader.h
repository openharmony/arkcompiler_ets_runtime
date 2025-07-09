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
#ifndef ECMASCRIPT_SATTIC_MODULE_LOADER_H
#define ECMASCRIPT_SATTIC_MODULE_LOADER_H
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
class StaticModuleLoader {
public:
    static bool CanTryLoadStaticModulePath(const CString &requestPath);

    static Local<JSValueRef> GetStaticModuleLoadFunc(EcmaVM *vm);

    static JSHandle<JSTaggedValue> LoadStaticModule(JSThread *thread,
        Local<FunctionRef> getEsModuleFunc, const CString &key);
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_SATTIC_MODULE_NAMESPACE_H
