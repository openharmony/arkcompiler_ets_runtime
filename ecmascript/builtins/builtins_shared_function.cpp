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

#include "ecmascript/builtins/builtins_shared_function.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsSharedFunction::SharedFunctionConstructor(EcmaRuntimeCallInfo *argv)
{
    // not support
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Function, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "Not support eval. Forbidden using new SharedFuction()/SharedFuction().",
                                JSTaggedValue::Exception());
}
}  // namespace panda::ecmascript::builtins