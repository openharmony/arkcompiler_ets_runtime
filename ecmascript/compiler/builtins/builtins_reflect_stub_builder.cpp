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

#include "ecmascript/compiler/builtins/builtins_reflect_stub_builder.h"

#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/compiler/profiler_operation.h"

namespace panda::ecmascript::kungfu {
void BuiltinsReflectStubBuilder::Get(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs_);
    Label isEcmaObject(env);
    BRANCH(IsEcmaObject(obj), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    {
        GateRef key = GetCallArg1(numArgs_);
        Label twoArg(env);
        BRANCH(Int64Equal(numArgs_, IntPtr(2)), &twoArg, slowPath); // 2 : two args
        Bind(&twoArg);
        {
            *result = GetPropertyByValue(glue_, obj, key, ProfileOperation());
            BRANCH(TaggedIsHole(result->ReadVariable()), slowPath, exit);
        }
    }
}

}  // namespace panda::ecmascript::kungfu
