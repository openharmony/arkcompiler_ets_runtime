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

#include "ecmascript/compiler/builtins/builtins_number_stub_builder.h"

#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript::kungfu {
void BuiltinsNumberStubBuilder::ParseFloat(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label definedMsg(env);
    Label undefinedMsg(env);
    GateRef msg = GetCallArg0(numArgs_);
    BRANCH(TaggedIsUndefined(msg), &undefinedMsg, &definedMsg);
    Bind(&undefinedMsg);
    {
        *result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
        Jump(exit);
    }
    Bind(&definedMsg);
    {
        Label heapObj(env);
        Label stringObj(env);
        BRANCH(TaggedIsHeapObject(msg), &heapObj, slowPath);
        Bind(&heapObj);
        BRANCH(IsString(msg), &stringObj, slowPath);
        Bind(&stringObj);
        {
            *result = CallNGCRuntime(glue_, RTSTUB_ID(NumberHelperStringToDouble), { msg });
            Jump(exit);
        }
    }
}
}  // namespace panda::ecmascript::kungfu
