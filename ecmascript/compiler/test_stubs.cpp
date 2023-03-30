/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/test_stubs.h"

#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/message_string.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#ifndef NDEBUG
void FooAOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef pcOffset = Int32(1);
    (void)calltarget;
    GateRef barIndex = IntToTaggedInt(Int32(CommonStubCSigns::BarAOT));
    GateRef numArgs = IntToTaggedInt(Int32(2));
    GateRef barfunc = CallRuntime(glue, RTSTUB_ID(DefineAotFunc), {barIndex, numArgs});
    GateRef result =
        CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, argc, barfunc, newtarget, thisObj, a, b, pcOffset});
    Return(result);
}

void BarAOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    [[maybe_unused]] GateRef argc = Int32Argument(1);
    [[maybe_unused]] GateRef calltarget = TaggedArgument(2);
    [[maybe_unused]] GateRef newtarget = TaggedArgument(3);
    [[maybe_unused]] GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef result = CallRuntime(glue, RTSTUB_ID(Add2), {a, b});
    Return(result);
}

void Foo1AOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef pcOffset = Int32(1);
    (void)calltarget;
    GateRef barIndex = IntToTaggedInt(Int32(CommonStubCSigns::Bar1AOT));
    GateRef numArgs = IntToTaggedInt(Int32(3));
    GateRef barfunc = CallRuntime(glue, RTSTUB_ID(DefineAotFunc), {barIndex, numArgs});
    GateRef result =
        CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, argc, barfunc, newtarget, thisObj, a, b, pcOffset});
    Return(result);
}

void Bar1AOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    (void)argc;
    (void)calltarget;
    (void)newtarget;
    (void)thisObj;
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef c = TaggedArgument(7);
    GateRef result = CallRuntime(glue, RTSTUB_ID(Add2), {a, b});
    GateRef result2 = CallRuntime(glue, RTSTUB_ID(Add2), {result, c});
    Return(result2);
}

void Foo2AOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef pcOffset = Int32(1);
    (void)calltarget;
    GateRef actualArgC = Int64Add(argc, Int64(1));
    GateRef barIndex = IntToTaggedInt(Int32(CommonStubCSigns::BarAOT));
    GateRef numArgs = IntToTaggedInt(Int32(2));
    GateRef barfunc = CallRuntime(glue, RTSTUB_ID(DefineAotFunc), {barIndex, numArgs});
    GateRef result = CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, actualArgC, barfunc, newtarget, thisObj,
                                    a, b, Undefined(), pcOffset});
    Return(result);
}

void FooNativeAOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef pcOffset = Int32(1);
    (void)calltarget;
    GateRef actualArgC = Int64Add(argc, Int64(1));
    GateRef printfunc = CallRuntime(glue, RTSTUB_ID(GetPrintFunc), {});
    GateRef result = CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, actualArgC, printfunc, newtarget, thisObj,
                                    a, b, Undefined(), pcOffset});
    Return(result);
}

void FooBoundAOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef bindArguments = IntToTaggedInt(Int32(37));
    GateRef pcOffset = Int32(1);
    (void)calltarget;
    GateRef numArgs = IntToTaggedInt(Int32(2));
    GateRef barIndex = IntToTaggedInt(Int32(CommonStubCSigns::BarAOT));
    GateRef barfunc = CallRuntime(glue, RTSTUB_ID(DefineAotFunc), {barIndex, numArgs});
    GateRef bindfunc = CallRuntime(glue, RTSTUB_ID(GetBindFunc), {barfunc});
    GateRef newjsfunc = CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, Int64(5), bindfunc, newtarget, barfunc,
                                    Int64(0x02), bindArguments, pcOffset});
    GateRef result = CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, argc, newjsfunc, newtarget, thisObj,
                                a, b, pcOffset});
    Return(result);
}

void FooProxyAOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    [[maybe_unused]] GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef pcOffset = Int32(1);

    GateRef barIndex = IntToTaggedInt(Int32(CommonStubCSigns::BarAOT));
    GateRef numArgs = IntToTaggedInt(Int32(2));
    GateRef barfunc = CallRuntime(glue, RTSTUB_ID(DefineAotFunc), {barIndex, numArgs});

    GateRef proxyfunc = CallRuntime(glue, RTSTUB_ID(DefineProxyFunc), {barfunc});
    GateRef result =
        CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, argc, proxyfunc, newtarget, thisObj, a, b, pcOffset});
    Return(result);
}

void FooProxy2AOTStubBuilder::GenerateCircuit()
{
    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    [[maybe_unused]] GateRef calltarget = TaggedArgument(2);
    GateRef newtarget = TaggedArgument(3);
    GateRef thisObj = TaggedArgument(4);
    GateRef a = TaggedArgument(5);
    GateRef b = TaggedArgument(6);
    GateRef pcOffset = Int32(1);

    GateRef barIndex = IntToTaggedInt(Int32(CommonStubCSigns::Bar2AOT));
    GateRef numArgs = IntToTaggedInt(Int32(2));
    GateRef barfunc = CallRuntime(glue, RTSTUB_ID(DefineAotFunc), {barIndex, numArgs});
    GateRef proxyHandler = CallRuntime(glue, RTSTUB_ID(DefineProxyHandler), {barfunc});

    GateRef proxyfunc = CallRuntime(glue, RTSTUB_ID(DefineProxyFunc2), {barfunc, proxyHandler});
    GateRef result =
        CallNGCRuntime(glue, RTSTUB_ID(JSCall), {glue, argc, proxyfunc, newtarget, thisObj, a, b, pcOffset});
    Return(result);
}

void Bar2AOTStubBuilder::GenerateCircuit()
{
    [[maybe_unused]] GateRef glue = PtrArgument(0);
    [[maybe_unused]] GateRef argc = Int32Argument(1);
    [[maybe_unused]] GateRef calltarget = TaggedArgument(2);
    [[maybe_unused]] GateRef newtarget = TaggedArgument(3);
    [[maybe_unused]] GateRef thisObj = TaggedArgument(4);
    CallRuntime(glue, RTSTUB_ID(DumpTaggedType), {thisObj});
    Return(thisObj);
}

void TestAbsoluteAddressRelocationStubBuilder::GenerateCircuit()
{
    auto env = GetEnvironment();
    GateRef a = Int64Argument(0);
    Label start(env);
    Jump(&start);
    Bind(&start);
    GateRef globalValueC = RelocatableData(0xabc);
    GateRef dummyValueC = Load(VariableType::INT64(), globalValueC);
    GateRef result = ZExtInt1ToInt64(Int64Equal(a, dummyValueC));
    Return(result);
}
#endif
}   // namespace panda::ecmascript::kungfu
