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

#include "ecmascript/compiler/builtins/builtins_object_stub_builder.h"

#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/typed_array_stub_builder.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/message_string.h"
namespace panda::ecmascript::kungfu {
GateRef BuiltinsObjectStubBuilder::CreateListFromArrayLike(GateRef glue, GateRef arrayObj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label exit(env);

    // 3. If Type(obj) is Object, throw a TypeError exception.
    Label targetIsHeapObject(env);
    Label targetIsEcmaObject(env);
    Label targetNotEcmaObject(env);
    Branch(TaggedIsHeapObject(arrayObj), &targetIsHeapObject, &targetNotEcmaObject);
    Bind(&targetIsHeapObject);
    Branch(TaggedObjectIsEcmaObject(arrayObj), &targetIsEcmaObject, &targetNotEcmaObject);
    Bind(&targetNotEcmaObject);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(TargetTypeNotObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        Jump(&exit);
    }
    Bind(&targetIsEcmaObject);
    {
        // 4. Let len be ToLength(Get(obj, "length")).
        GateRef lengthString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::LENGTH_STRING_INDEX);
        GateRef value = FastGetPropertyByName(glue, arrayObj, lengthString, ProfileOperation());
        GateRef number = ToLength(glue, value);
        // 5. ReturnIfAbrupt(len).
        Label isPendingException1(env);
        Label noPendingException1(env);
        Branch(HasPendingException(glue), &isPendingException1, &noPendingException1);
        Bind(&isPendingException1);
        {
            Jump(&exit);
        }
        Bind(&noPendingException1);
        {
            Label indexInRange(env);
            Label indexOutRange(env);

            GateRef doubleLen = GetDoubleOfTNumber(number);
            Branch(DoubleGreaterThan(doubleLen, Double(JSObject::MAX_ELEMENT_INDEX)), &indexOutRange, &indexInRange);
            Bind(&indexOutRange);
            {
                DebugPrint(glue, { Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable)) });
                GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(LenGreaterThanMax));
                CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                Jump(&exit);
            }
            Bind(&indexInRange);
            {
                GateRef int32Len = DoubleToInt(glue, doubleLen);
                // 6. Let list be an empty List.
                NewObjectStubBuilder newBuilder(this);
                GateRef array = newBuilder.NewTaggedArray(glue, int32Len);
                Label targetIsTypeArray(env);
                Label targetNotTypeArray(env);
                Branch(IsTypedArray(arrayObj), &targetIsTypeArray, &targetNotTypeArray);
                Bind(&targetIsTypeArray);
                {
                    TypedArrayStubBuilder arrayStubBuilder(this);
                    arrayStubBuilder.FastCopyElementToArray(glue, arrayObj, array);
                    // c. ReturnIfAbrupt(next).
                    Label isPendingException2(env);
                    Label noPendingException2(env);
                    Branch(HasPendingException(glue), &isPendingException2, &noPendingException2);
                    Bind(&isPendingException2);
                    {
                        Jump(&exit);
                    }
                    Bind(&noPendingException2);
                    {
                        res = array;
                        Jump(&exit);
                    }
                }
                Bind(&targetNotTypeArray);
                // 8. Repeat while index < len
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Label isPendingException3(env);
                Label noPendingException3(env);
                Label storeValue(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int32UnsignedLessThan(*index, int32Len), &storeValue, &afterLoop);
                    Bind(&storeValue);
                    {
                        GateRef next = FastGetPropertyByIndex(glue, arrayObj, *index, ProfileOperation());
                        // c. ReturnIfAbrupt(next).
                        Branch(HasPendingException(glue), &isPendingException3, &noPendingException3);
                        Bind(&isPendingException3);
                        {
                            Jump(&exit);
                        }
                        Bind(&noPendingException3);
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *index, next);
                        index = Int32Add(*index, Int32(1));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead);
                Bind(&afterLoop);
                {
                    res = array;
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    GateRef ret = *res;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
