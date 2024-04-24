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

#include "ecmascript/byte_array.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/js_dataview.h"


using namespace panda::ecmascript;

namespace panda::test {
class ByteArrayTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: ComputeSize / GetData
 * @tc.desc: Compute the bytesize of bytearray in memory and get the pointer of Onheap data.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ByteArrayTest, ComputeSizeAndGetData)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ByteArray> byteArray = factory->NewByteArray(5, 8);
    EXPECT_EQ(ByteArray::ComputeSize(8, 0), 16U);
    EXPECT_EQ(ByteArray::ComputeSize(16, 4), 80U);
    uintptr_t dataPointer = reinterpret_cast<uintptr_t>(byteArray.GetTaggedValue().GetTaggedObject()) + 32;
    EXPECT_EQ(byteArray->GetData(2), reinterpret_cast<void *>(dataPointer));
}

/**
 * @tc.name: Set / Get
 * @tc.desc: Set data to bytearray and get data from bytearray.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ByteArrayTest, SetAndGet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t value = 4294967295;
    JSTaggedType val = JSTaggedValue(value).GetRawData();
    JSHandle<ByteArray> byteArray = factory->NewByteArray(3, sizeof(value));
    byteArray->Set(thread, 1, DataViewType::UINT32, val, 2);
    EXPECT_EQ(byteArray->Get(thread, 1, DataViewType::UINT32, 2), JSTaggedValue(value));
}
}  // namespace panda::ecmascript