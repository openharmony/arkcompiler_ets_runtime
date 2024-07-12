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
#include "ecmascript/dfx/native_module_failure_info.h"

namespace panda::ecmascript {
JSHandle<NativeModuleFailureInfo> NativeModuleFailureInfo::CreateNativeModuleFailureInfo(const EcmaVM *vm,
    const std::string &failureInfo)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<NativeModuleFailureInfo> nativeModuleFailureInfo = factory->NewNativeModuleFailureInfo();
    JSHandle<EcmaString> failureInfoStr = factory->NewFromUtf8(failureInfo);
    nativeModuleFailureInfo->SetArkNativeModuleFailureInfo(vm->GetJSThread(), JSHandle<JSTaggedValue>(failureInfoStr));
    return nativeModuleFailureInfo;
}
}