/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_errors.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainerError::BusinessError(JSThread *thread, int32_t errorCode, const char *msg)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::ERROR, msg, StackCheck::NO);
    JSHandle<JSTaggedValue> code(thread, JSTaggedValue(errorCode));
    JSHandle<EcmaString> key = factory->NewFromUtf8("code");
    JSHandle<EcmaString> name = factory->NewFromUtf8("name");
    JSHandle<EcmaString> value = factory->NewFromUtf8("BusinessError");
    JSObject::CreateDataPropertyOrThrow(thread, error, JSHandle<JSTaggedValue>::Cast(key), code);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSObject::CreateDataPropertyOrThrow(thread, error, JSHandle<JSTaggedValue>::Cast(name),
                                        JSHandle<JSTaggedValue>::Cast(value));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return error.GetTaggedValue();
}

JSTaggedValue ContainerError::BindError(JSThread *thread, const char *msg)
{
    return ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR, msg);
}

JSTaggedValue ContainerError::ParamError(JSThread *thread, const char *msg)
{
    return ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, msg);
}

JSTaggedValue ContainerError::ReferenceError(JSThread *thread, const char *msg)
{
    return ContainerError::BusinessError(thread, ErrorFlag::REFERENCE_ERROR, msg);
}
}  // namespace panda::ecmascript::containers