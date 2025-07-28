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

#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
JSHandle<JSObject> ObjectFactory::NewJSXRefObject()
{
    JSHandle<JSHClass> jsXRefHClass = JSHandle<JSHClass>::Cast(thread_->GlobalConstants()->GetHandledXRefObjectClass());
    JSHandle<JSObject> object(NewJSObject(jsXRefHClass));
    return object;
}
}  // namespace panda::ecmascript
