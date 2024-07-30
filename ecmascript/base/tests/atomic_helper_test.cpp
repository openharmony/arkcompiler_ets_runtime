/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/atomic_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class AtomicHelperTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(AtomicHelperTest, ValidateIntegerTypedArray)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    bool waitable = false;
    uint32_t bufferSize = 10;
    JSHandle<JSTaggedValue> func = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(func), func));
    JSHandle<JSArrayBuffer> buffer = factory->NewJSArrayBuffer(bufferSize);
    JSHandle<JSTaggedValue> bufferVal = JSHandle<JSTaggedValue>::Cast(buffer);
    array->SetViewedArrayBufferOrByteArray(thread, bufferVal);
    JSHandle<JSTaggedValue> arrayVal = JSHandle<JSTaggedValue>::Cast(array);
    JSHandle<JSTaggedValue> resultBuffer(thread, AtomicHelper::ValidateIntegerTypedArray(thread, arrayVal, waitable));
    EXPECT_EQ(resultBuffer.GetTaggedValue(), buffer.GetTaggedValue());
}

HWTEST_F_L0(AtomicHelperTest, ValidateAtomicAccess)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t bufferSize = 256;
    uint32_t byteOffset = 7;
    uint32_t arrayLength = 3;
    JSHandle<JSTaggedValue> func = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(func), func));
    JSHandle<JSArrayBuffer> buffer = factory->NewJSArrayBuffer(bufferSize);
    JSHandle<JSTaggedValue> bufferVal = JSHandle<JSTaggedValue>::Cast(buffer);
    array->SetViewedArrayBufferOrByteArray(thread, bufferVal);
    array->SetTypedArrayName(thread, globalConst->GetHandledInt8ArrayString()); // test int8 array
    array->SetByteOffset(byteOffset);
    array->SetArrayLength(arrayLength);
    JSHandle<JSTaggedValue> arrayVal = JSHandle<JSTaggedValue>::Cast(array);
    uint32_t index0 =
        AtomicHelper::ValidateAtomicAccess(thread, arrayVal, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)));
    uint32_t index1 =
        AtomicHelper::ValidateAtomicAccess(thread, arrayVal, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    uint32_t index2 =
        AtomicHelper::ValidateAtomicAccess(thread, arrayVal, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));
    EXPECT_EQ(index0, 0 * sizeof(int8_t) + byteOffset);
    EXPECT_EQ(index1, 1 * sizeof(int8_t) + byteOffset);
    EXPECT_EQ(index2, 2 * sizeof(int8_t) + byteOffset);
}

HWTEST_F_L0(AtomicHelperTest, Atomic_Store_Load)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t bufferSize = 256;
    uint32_t byteOffset = 7;
    uint32_t arrayLength = 3;
    JSHandle<JSTaggedValue> func = env->GetUint32ArrayFunction();
    JSHandle<JSTypedArray> array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(func), func));
    JSHandle<JSArrayBuffer> buffer = factory->NewJSArrayBuffer(bufferSize);
    JSHandle<JSTaggedValue> bufferVal = JSHandle<JSTaggedValue>::Cast(buffer);
    array->SetViewedArrayBufferOrByteArray(thread, bufferVal);
    array->SetTypedArrayName(thread, globalConst->GetHandledUint32ArrayString()); // test uint32_t array
    array->SetByteOffset(byteOffset);
    array->SetArrayLength(arrayLength);
    JSHandle<JSTaggedValue> arrayVal = JSHandle<JSTaggedValue>::Cast(array);
    JSHandle<JSTaggedValue> index0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> index1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> index2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value0(thread, JSTaggedValue(-1)); // to uint32_t : 4294967295
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
#ifdef PANDA_TARGET_MACOS
    JSHandle<JSTaggedValue> value2(thread, static_cast<JSTaggedValue>(static_cast<uint32_t>(4294967295 + 1))); // to uint32_t : 0
#else
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4294967295 + 1));
#endif
    JSHandle<JSTaggedValue> bufferTag0(thread, AtomicHelper::AtomicStore(thread, arrayVal, index0, value0));
    JSHandle<JSTaggedValue> bufferTag1(thread, AtomicHelper::AtomicStore(thread, arrayVal, index1, value1));
    JSHandle<JSTaggedValue> bufferTag2(thread, AtomicHelper::AtomicStore(thread, arrayVal, index2, value2));
    EXPECT_EQ(value0.GetTaggedValue().GetNumber(), -1);
    EXPECT_EQ(value1.GetTaggedValue().GetNumber(), 1);
    EXPECT_EQ(value2.GetTaggedValue().GetNumber(), 4294967296);

    JSHandle<JSTaggedValue> result0(thread, AtomicHelper::AtomicLoad(thread, arrayVal, index0));
    JSHandle<JSTaggedValue> result1(thread, AtomicHelper::AtomicLoad(thread, arrayVal, index1));
    JSHandle<JSTaggedValue> result2(thread, AtomicHelper::AtomicLoad(thread, arrayVal, index2));
    EXPECT_EQ(result0.GetTaggedValue().GetNumber(), 4294967295); // 4294967295 : -1 to uint32_t
    EXPECT_EQ(result1.GetTaggedValue().GetNumber(), 1);
    EXPECT_EQ(result2.GetTaggedValue().GetNumber(), 0); // 0 : 4294967296 to uint32_t
}
}  // namespace panda::test
