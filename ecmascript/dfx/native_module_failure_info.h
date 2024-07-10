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

#ifndef ECMASCRIPT_DFX_NATIVE_MODULE_FAILURE_INFO_H
#define ECMASCRIPT_DFX_NATIVE_MODULE_FAILURE_INFO_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory-inl.h"
namespace panda::ecmascript {
class NativeModuleFailureInfo final : public JSObject {
public:
    CAST_CHECK(NativeModuleFailureInfo, IsNativeModuleFailureInfo);

    static JSHandle<NativeModuleFailureInfo> CreateNativeModuleFailureInfo(const EcmaVM *vm,
        const std::string &failureInfo);

    static constexpr size_t ARK_NATIVE_MODULE_FAILURE_INFO_OFFSET = JSObject::SIZE;
    ACCESSORS(ArkNativeModuleFailureInfo, ARK_NATIVE_MODULE_FAILURE_INFO_OFFSET, SIZE)

    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ARK_NATIVE_MODULE_FAILURE_INFO_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_NATIVE_MODULE_FAILURE_INFO_H
