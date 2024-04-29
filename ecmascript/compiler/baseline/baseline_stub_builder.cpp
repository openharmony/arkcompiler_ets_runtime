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

#include "ecmascript/compiler/baseline/baseline_stub_builder.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

GateRef BaselineStubBuilder::GetHomeObjectFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::HOME_OBJECT_OFFSET));
}

GateRef BaselineStubBuilder::GetResumeModeFromGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
}

GateRef BaselineStubBuilder::GetResumeModeFromAsyncGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSAsyncGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSAsyncGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSAsyncGeneratorObject::ResumeModeBits::SIZE) - 1));
}

GateRef BaselineStubBuilder::GetLastLeaveFrame(GateRef glue)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(isArch32));
    return Load(VariableType::NATIVE_POINTER(), glue, spOffset);
}

GateRef BaselineStubBuilder::GetProfileTypeInfoFromFunction(GateRef function)
{
    GateRef raw = Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    return Load(VariableType::JS_POINTER(), raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
}

GateRef BaselineStubBuilder::GetModuleFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::ECMA_MODULE_OFFSET));
}

}  // namespace panda::ecmascript::kungfu
