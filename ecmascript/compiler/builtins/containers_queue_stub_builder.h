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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_QUEUE_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_QUEUE_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_api/js_api_queue.h"

namespace panda::ecmascript::kungfu {
class ContainersQueueStubBuilder : public BuiltinsStubBuilder {
public:
    explicit ContainersQueueStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~ContainersQueueStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersQueueStubBuilder);
    NO_COPY_SEMANTIC(ContainersQueueStubBuilder);
    void GenerateCircuit() override {}

#define DECLARE_CONTAINERS_QUEUE_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_CONTAINERS_QUEUE_STUB_BUILDER(DECLARE_CONTAINERS_QUEUE_STUB_BUILDER)
#undef DECLARE_CONTAINERS_QUEUE_STUB_BUILDER

    GateRef GetArrayLength(GateRef glue, GateRef obj)
    {
        auto env = GetEnvironment();
        Label entry(env);
        env->SubCfgEntry(&entry);
        Label exit(env);
        Label endGreatBeging(env);
        Label endNotGreatBeging(env);
        DEFVARIABLE(length, VariableType::INT32(), Int32(0));
        GateRef begin = LoadPrimitive(VariableType::INT32(), obj, IntPtr(JSAPIQueue::FRONT_OFFSET));
        GateRef end = LoadPrimitive(VariableType::INT32(), obj, IntPtr(JSAPIQueue::TAIL_OFFSET));
        BRANCH(Int32GreaterThanOrEqual(end, begin), &endGreatBeging, &endNotGreatBeging);
        Bind(&endGreatBeging);
        {
            length = Int32Sub(end, begin);
            Jump(&exit);
        }
        Bind(&endNotGreatBeging);
        {
            GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
            GateRef elements = Load(VariableType::JS_POINTER(), glue, obj, elementsOffset);
            GateRef elementsSize = LoadPrimitive(VariableType::INT32(), elements, IntPtr(TaggedArray::LENGTH_OFFSET));
            length = Int32Add(Int32Sub(end, begin), elementsSize);
            Jump(&exit);
        }
        Bind(&exit);
        auto ret = *length;
        env->SubCfgExit();
        return ret;
    }

    GateRef Get(GateRef glue, GateRef obj, GateRef index)
    {
        GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
        GateRef elements = Load(VariableType::JS_POINTER(), glue, obj, elementsOffset);
        GateRef capacity = LoadPrimitive(VariableType::INT32(), elements, IntPtr(TaggedArray::LENGTH_OFFSET));
        GateRef front = LoadPrimitive(VariableType::INT32(), obj, IntPtr(JSAPIQueue::FRONT_OFFSET));
        GateRef curIndex = Int32Mod(Int32Add(front, index), capacity);
        return GetValueFromTaggedArray(glue, elements, curIndex);
    }

    GateRef GetNextPosition(GateRef glue, GateRef obj, GateRef index)
    {
        GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
        GateRef elements = Load(VariableType::JS_POINTER(), glue, obj, elementsOffset);
        GateRef elementsSize = LoadPrimitive(VariableType::INT32(), elements, IntPtr(TaggedArray::LENGTH_OFFSET));
        return Int32Mod(Int32Add(index, Int32(1)), elementsSize);
    }

    GateRef GetCurrentFront(GateRef obj)
    {
        return LoadPrimitive(VariableType::INT32(), obj, IntPtr(JSAPIQueue::FRONT_OFFSET));
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_QUEUE_STUB_BUILDER_H
