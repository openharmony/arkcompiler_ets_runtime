/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/js_intl.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> JSIntl::LegacyUnwrapReceiver(JSThread *thread, const JSHandle<JSTaggedValue> &receiver,
    const JSHandle<JSTaggedValue> &constructor, bool hasInitializedSlot)
{
    bool isInstanceOf = JSFunction::OrdinaryHasInstance(thread, constructor, receiver);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    if (!hasInitializedSlot && isInstanceOf) {
        JSHandle<GlobalEnv> env = thread->GetGlobalEnv();
        JSHandle<JSTaggedValue> key(thread, JSHandle<JSIntl>::Cast(env->GetIntlFunction())->GetFallbackSymbol(thread));
        OperationResult operationResult = JSTaggedValue::GetProperty(thread, receiver, key);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
        return operationResult.GetValue();
    }
    return receiver;
}
}  // namespace panda::ecmascript
