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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tests/test_helper.h"


using namespace panda::ecmascript;

namespace panda::test {
class JsSendableArrayBufferTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: CopyDataBlockBytes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, CopyDataBlockBytesTest001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    uint8_t value = 100;
    size_t length = 5;
    void *toBuffer = vm->GetNativeAreaAllocator()->AllocateBuffer(length);
    JSHandle<JSNativePointer> toNativePointer = factory->NewJSNativePointer(toBuffer, nullptr, nullptr);
    uint8_t *data = static_cast<uint8_t *>(vm->GetNativeAreaAllocator()->AllocateBuffer(length));
    if (memset_s(data, length, value, length) != EOK) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    void *formBuffer = vm->GetNativeAreaAllocator()->AllocateBuffer(length);
    JSHandle<JSNativePointer> fromNativePointer =
        factory->NewJSNativePointer(formBuffer, nullptr, reinterpret_cast<void *>(data));
    int32_t fromIndex = 0;
    int32_t count = 5;
    JSSendableArrayBuffer::CopyDataBlockBytes(JSHandle<JSTaggedValue>::Cast(toNativePointer).GetTaggedValue(),
        JSHandle<JSTaggedValue>::Cast(fromNativePointer).GetTaggedValue(), fromIndex, count);
    auto toData = reinterpret_cast<uint8_t *>(toNativePointer->GetExternalPointer());
    auto fromData = reinterpret_cast<uint8_t *>(fromNativePointer->GetExternalPointer());
    for (uint32_t i = 0; i < length; i++) {
        EXPECT_EQ(*(toData + i), *(fromData + i));
    }
    vm->GetNativeAreaAllocator()->FreeBuffer(toBuffer);
    vm->GetNativeAreaAllocator()->FreeBuffer(data);
    vm->GetNativeAreaAllocator()->FreeBuffer(formBuffer);
}

/**
 * @tc.name: CopyDataPointBytes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, CopyDataPointBytesTest001)
{
    auto vm = thread->GetEcmaVM();

    size_t length = 5;
    void *toBuffer = vm->GetNativeAreaAllocator()->AllocateBuffer(length);
    uint8_t *fromData = static_cast<uint8_t *>(vm->GetNativeAreaAllocator()->AllocateBuffer(length));
    uint8_t *toData = static_cast<uint8_t *>(vm->GetNativeAreaAllocator()->AllocateBuffer(length));
    *(fromData + 1) = 1;

    int32_t fromIndex = 0;
    int32_t count = 5;
    JSSendableArrayBuffer::CopyDataPointBytes(fromData, toData, fromIndex, count);
    for (uint32_t i = 0; i < length; i++) {
        EXPECT_EQ(*(toData + i), *(fromData + i));
    }
    vm->GetNativeAreaAllocator()->FreeBuffer(toBuffer);
    vm->GetNativeAreaAllocator()->FreeBuffer(fromData);
    vm->GetNativeAreaAllocator()->FreeBuffer(toData);
}

/**
 * @tc.name: Attach
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, AttachTest001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    size_t length = 5;
    const JSHandle<JSSendableArrayBuffer> arrBuf = factory->NewJSSendableArrayBuffer(5);
    factory->NewJSSendableArrayBufferData(arrBuf, 5);
    JSTaggedValue taggedValue = arrBuf->GetArrayBufferData();
    arrBuf->Attach(thread, length + 1, taggedValue);
    ASSERT_EQ(arrBuf->GetArrayBufferByteLength(), 6U);
    ASSERT_EQ(arrBuf->GetArrayBufferData().GetRawData(), taggedValue.GetRawData());
}

/**
 * @tc.name: Attach
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, AttachTest002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    size_t length = 5;
    const JSHandle<JSSendableArrayBuffer> arrBuf = factory->NewJSSendableArrayBuffer(5);
    factory->NewJSSendableArrayBufferData(arrBuf, 5);
    JSTaggedValue taggedValue = arrBuf->GetArrayBufferData();
    arrBuf->Attach(thread, length + 1, taggedValue, true);
    ASSERT_EQ(arrBuf->GetArrayBufferByteLength(), 6U);
    ASSERT_EQ(arrBuf->GetArrayBufferData().GetRawData(), taggedValue.GetRawData());
}

/**
 * @tc.name: Detach
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, DetachTest001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    size_t length = 5;
    const JSHandle<JSSendableArrayBuffer> arrBuf = factory->NewJSSendableArrayBuffer(5);
    factory->NewJSSendableArrayBufferData(arrBuf, 5);
    JSTaggedValue taggedValue = arrBuf->GetArrayBufferData();
    arrBuf->Attach(thread, length + 1, taggedValue);
    ASSERT_EQ(arrBuf->GetArrayBufferByteLength(), 6U);
    ASSERT_EQ(arrBuf->GetArrayBufferData().GetRawData(), taggedValue.GetRawData());

    arrBuf->Detach(thread);
    EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 0U);
    EXPECT_EQ(arrBuf->GetArrayBufferData().GetRawData(), JSTaggedValue::Null().GetRawData());
    EXPECT_TRUE(arrBuf->IsDetach());
}

/**
 * @tc.name: Detach
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, DetachTest002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    size_t length = 5;
    const JSHandle<JSSendableArrayBuffer> arrBuf = factory->NewJSSendableArrayBuffer(5);
    factory->NewJSSendableArrayBufferData(arrBuf, 5);
    JSTaggedValue taggedValue = arrBuf->GetArrayBufferData();
    arrBuf->Attach(thread, length + 1, taggedValue, true);
    ASSERT_EQ(arrBuf->GetArrayBufferByteLength(), 6U);
    ASSERT_EQ(arrBuf->GetArrayBufferData().GetRawData(), taggedValue.GetRawData());

    arrBuf->Detach(thread, true);
    EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 0U);
    EXPECT_EQ(arrBuf->GetArrayBufferData().GetRawData(), JSTaggedValue::Null().GetRawData());
    EXPECT_TRUE(arrBuf->IsDetach());
}

/**
 * @tc.name: Detach
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsSendableArrayBufferTest, DetachTest003)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    size_t length = 5;
    const JSHandle<JSSendableArrayBuffer> arrBuf = factory->NewJSSendableArrayBuffer(5);
    factory->NewJSSendableArrayBufferData(arrBuf, 5);
    JSTaggedValue taggedValue = arrBuf->GetArrayBufferData();
    arrBuf->Attach(thread, length + 1, taggedValue);

    arrBuf->Detach(thread);
    EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 0U);

    arrBuf->Detach(thread);
}

} // namespace panda::test