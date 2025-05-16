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

namespace panda::ecmascript::kungfu {
void BuiltinsReflectStubBuilder::Get(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef target = GetCallArg0(numArgs_);
    Label isEcmaObject(env);
    BRANCH(IsEcmaObject(glue_, target), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    {
        GateRef propKey = GetCallArg1(numArgs_);
        Label twoArgs(env);
        Label threeArgs(env);
#if ENABLE_NEXT_OPTIMIZATION
        BRANCH(Int64Equal(numArgs_, IntPtr(2)), &twoArgs, &threeArgs); // 2 : two args
        Bind(&twoArgs);
        {
            *result = GetPropertyByValue(glue_, target, propKey);
            BRANCH(TaggedIsHole(result->ReadVariable()), slowPath, exit);
        }
        Bind(&threeArgs);
        {
            GateRef receiver = GetCallArg2(numArgs_);
            *result = GetPropertyByValue(glue_, target, propKey, receiver);
            BRANCH(TaggedIsHole(result->ReadVariable()), slowPath, exit);
        }
#else
        BRANCH(Int64Equal(numArgs_, IntPtr(2)), &twoArgs, slowPath); // 2 : two args
        Bind(&twoArgs);
        {
            *result = GetPropertyByValue(glue_, target, propKey);
            BRANCH(TaggedIsHole(result->ReadVariable()), slowPath, exit);
        }
#endif
    }
}

#if ENABLE_NEXT_OPTIMIZATION
void BuiltinsReflectStubBuilder::Has(Variable* result, Label* exit, Label* slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs_);
    GateRef key = GetCallArg1(numArgs_);
    Label isEcmaObject(env);
    Label isPropertyKey(env);
    BRANCH(IsEcmaObject(glue_, obj), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    {
        GateRef propertyKey = ToPropertyKey(glue_, key);
        BRANCH_UNLIKELY(HasPendingException(glue_), slowPath, &isPropertyKey);
        Bind(&isPropertyKey);
        {
            *result = HasProperty(glue_, obj, propertyKey);
            Jump(exit);
        }
    }
}

void BuiltinsReflectStubBuilder::Set(Variable* result, Label* exit, Label* slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs_);
    Label isEcmaObject(env);
    Label isPropertyKey(env);
    Label trySuccess(env);
    BRANCH(IsEcmaObject(glue_, obj), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    {
        GateRef key = GetCallArg1(numArgs_);
        GateRef value = GetCallArg2(numArgs_);
        Label threeArgs(env);
        Label checkType(env);
        Label returnTrue(env);
        Label returnValue(env);
        BRANCH(Int64Equal(numArgs_, IntPtr(3)), &threeArgs, slowPath); // 3 : three args
        Bind(&threeArgs);
        {
            BRANCH(IsModuleNamespace(glue_, obj), slowPath, &checkType);
            Bind(&checkType);
            GateRef setResult = SetPropertyByValue(glue_, obj, key, value, false, ProfileOperation(), false, false);
            BRANCH(TaggedIsHole(setResult), slowPath, &trySuccess);
            Bind(&trySuccess);
            {
                BRANCH(TaggedIsUndefined(setResult), &returnTrue, &returnValue);
                Bind(&returnTrue);
                {
                    *result = TaggedTrue();
                    Jump(exit);
                }
                Bind(&returnValue);
                *result = setResult;
                Jump(exit);
            }
        }
    }
}
#endif

}  // namespace panda::ecmascript::kungfu
