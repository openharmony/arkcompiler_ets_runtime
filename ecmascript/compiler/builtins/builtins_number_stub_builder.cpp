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

#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_primitive_ref.h"
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

void BuiltinsNumberStubBuilder::GenNumberConstructor(GateRef nativeCode, GateRef func, GateRef newTarget)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(numberValue, VariableType::JS_ANY(), IntToTaggedPtr(IntPtr(0)));
    Label thisCollectionObj(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    Label hasArg(env);
    Label numberCreate(env);
    Label newTargetIsHeapObject(env);
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(Int64GreaterThan(numArgs_, IntPtr(0)), &hasArg, &numberCreate);
    Bind(&hasArg);
    {
        GateRef value = GetArgFromArgv(Int32(0));
        Label number(env);
        BRANCH(TaggedIsNumber(value), &number, &slowPath);
        Bind(&number);
        {
            numberValue = value;
            res = value;
            Jump(&numberCreate);
        }
    }

    Bind(&numberCreate);
    Label newObj(env);
    Label newTargetIsJSFunction(env);
    BRANCH(TaggedIsUndefined(newTarget), &exit, &newObj);
    Bind(&newObj);
    {
        BRANCH(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
        Bind(&newTargetIsJSFunction);
        {
            Label intialHClassIsHClass(env);
            GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget,
                IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            BRANCH(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath1);
            Bind(&intialHClassIsHClass);
            {
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue_, 0);
                Label afterNew(env);
                newBuilder.NewJSObject(&res, &afterNew, intialHClass);
                Bind(&afterNew);
                {
                    GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
                    Store(VariableType::INT64(), glue_, *res, valueOffset, *numberValue);
                    Jump(&exit);
                }
            }
            Bind(&slowPath1);
            {
                GateRef argv = GetArgv();
                res = CallBuiltinRuntimeWithNewTarget(glue_,
                    { glue_, nativeCode, func, thisValue_, numArgs_, argv, newTarget });
                Jump(&exit);
            }
        }
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
