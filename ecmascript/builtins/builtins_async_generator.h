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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ASYNC_GENERATOR_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ASYNC_GENERATOR_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsAsyncGenerator : public base::BuiltinsBase {
public:
    // ecma 27.6.1.1
    static JSTaggedValue AsyncGeneratorFunctionConstructor(EcmaRuntimeCallInfo *argv);

    // ecma 27.6.1.2 AsyncGenerator.prototype.next
    static JSTaggedValue AsyncGeneratorPrototypeNext(EcmaRuntimeCallInfo *argv);

    // ecma 27.6.1.3 AsyncGenerator.prototype.return
    static JSTaggedValue AsyncGeneratorPrototypeReturn(EcmaRuntimeCallInfo *argv);

    // ecma 27.6.1.4 AsyncGenerator.prototype.throw
    static JSTaggedValue AsyncGeneratorPrototypeThrow(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ASYNC_GENERATOR_H
