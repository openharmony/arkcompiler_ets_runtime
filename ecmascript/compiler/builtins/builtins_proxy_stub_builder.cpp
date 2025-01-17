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

#include "ecmascript/compiler/builtins/builtins_proxy_stub_builder.h"

#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {
void BuiltinsProxyStubBuilder::GenProxyConstructor(GateRef nativeCode, GateRef func, GateRef newTarget)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label thisCollectionObj(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    Label handleIsEcma(env);
    Label targetIsEcma(env);
    Label newTargetNotUndefined(env);
    BRANCH(TaggedIsUndefined(newTarget), &slowPath, &newTargetNotUndefined);
    Bind(&newTargetNotUndefined);

    GateRef target = GetArgFromArgv(IntPtr(0), numArgs_, true);
    GateRef handler = GetArgFromArgv(IntPtr(1), numArgs_, true);

    BRANCH(IsEcmaObject(target), &targetIsEcma, &slowPath);
    Bind(&targetIsEcma);
    BRANCH(IsEcmaObject(handler), &handleIsEcma, &slowPath);
    Bind(&handleIsEcma);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue_, IntPtr(0));
        res = newBuilder.NewJSProxy(glue_, target, handler);
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue_, { glue_, nativeCode, func, thisValue_, numArgs_, argv }, true);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}
}  // namespace panda::ecmascript::kungfu
