/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_PROMISE_JOB_H
#define ECMASCRIPT_JS_PROMISE_JOB_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsPromiseJob : base::BuiltinsBase {
public:
    static JSTaggedValue PromiseReactionJob(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue PromiseResolveThenableJob(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue DynamicImportJob(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue CatchException(JSThread *thread, JSHandle<JSPromiseReactionsFunction> reject);

    static JSTaggedValue HandleModuleException(JSThread *thread, JSHandle<JSPromiseReactionsFunction> resolve,
                                               JSHandle<JSPromiseReactionsFunction> reject,
                                               JSHandle<EcmaString> specifierString);
    static JSTaggedValue HandleModuleException(JSThread *thread, JSHandle<JSPromiseReactionsFunction> resolve,
                                               JSHandle<JSPromiseReactionsFunction> reject,
                                               const CString &requestPath);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_JS_PROMISE_JOB_H
