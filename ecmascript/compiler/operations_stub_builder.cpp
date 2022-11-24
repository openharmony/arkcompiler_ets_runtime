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

#include "ecmascript/compiler/operations_stub_builder.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
GateRef OperationsStubBuilder::Equal(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHole(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(left, right);
    Branch(TaggedIsHole(*result), &isHole, &exit);
    Bind(&isHole);
    {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Eq), { left, right });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef OperationsStubBuilder::NotEqual(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(left, right);
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(NotEq), { left, right });
        Jump(&exit);
    }
    Bind(&notHole);
    {
        Label resultIsTrue(env);
        Label resultNotTrue(env);
        Branch(TaggedIsTrue(*result), &resultIsTrue, &resultNotTrue);
        Bind(&resultIsTrue);
        {
            result = TaggedFalse();
            Jump(&exit);
        }
        Bind(&resultNotTrue);
        {
            result = TaggedTrue();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
