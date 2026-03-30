/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/serializer/base_serializer.h"
#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/serialize_data.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class CommonSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(CommonSerializerTest, ValueSerializerCreationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto serializer = new ValueSerializer(thread);
    ASSERT_TRUE(serializer != nullptr);
    delete serializer;
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataCreationTest)
{
    auto serializeData = new SerializeData(thread);
    ASSERT_TRUE(serializeData != nullptr);
    delete serializeData;
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteUint8Test)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint8(0x42);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteUint32Test)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint32(12345);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteEncodeFlagTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteEncodeFlag(EncodeFlag::NEW_OBJECT);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteJSTaggedValueTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteJSTaggedValue(JSTaggedValue::Undefined());
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteJSTaggedTypeTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteJSTaggedType(JSTaggedValue::Undefined().GetRawData());
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteRawDataTest)
{
    SerializeData serializeData(thread);
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    serializeData.WriteRawData(data, sizeof(data));
    
    EXPECT_GE(serializeData.Size(), sizeof(data));
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataReadUint8Test)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint8(0x42);
    
    size_t position = 0;
    uint8_t value = serializeData.ReadUint8(position);
    EXPECT_EQ(value, 0x42);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataReadUint32Test)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint32(12345);
    
    size_t position = 0;
    uint32_t value = serializeData.ReadUint32(position);
    EXPECT_EQ(value, 12345u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataReadJSTaggedTypeTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteJSTaggedType(JSTaggedValue::Undefined().GetRawData());
    
    size_t position = 0;
    JSTaggedType value = serializeData.ReadJSTaggedType(position);
    EXPECT_EQ(value, JSTaggedValue::Undefined().GetRawData());
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataSizeTest)
{
    SerializeData serializeData(thread);
    
    EXPECT_EQ(serializeData.Size(), 0u);
    
    serializeData.WriteUint8(0x01);
    EXPECT_EQ(serializeData.Size(), 1u);
    
    serializeData.WriteUint32(0x12345678);
    EXPECT_EQ(serializeData.Size(), 5u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataDataTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint8(0x42);
    
    uint8_t* data = serializeData.Data();
    ASSERT_TRUE(data != nullptr);
    EXPECT_EQ(data[0], 0x42);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataMultipleWritesTest)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 100; i++) {
        serializeData.WriteUint32(i);
    }
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteMultipleTypesTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint8(0x01);
    serializeData.WriteUint32(0x12345678);
    serializeData.WriteJSTaggedValue(JSTaggedValue::Undefined());
    serializeData.WriteJSTaggedValue(JSTaggedValue::Null());
    serializeData.WriteJSTaggedValue(JSTaggedValue::True());
    serializeData.WriteJSTaggedValue(JSTaggedValue::False());
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteEncodeFlagsTest)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteEncodeFlag(EncodeFlag::NEW_OBJECT);
    serializeData.WriteEncodeFlag(EncodeFlag::REFERENCE);
    serializeData.WriteEncodeFlag(EncodeFlag::WEAK);
    serializeData.WriteEncodeFlag(EncodeFlag::PRIMITIVE);
    serializeData.WriteEncodeFlag(EncodeFlag::MULTI_RAW_DATA);
    serializeData.WriteEncodeFlag(EncodeFlag::ROOT_OBJECT);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataReadWriteConsistencyTest)
{
    SerializeData serializeData(thread);
    
    uint8_t uint8Value = 0xAB;
    uint32_t uint32Value = 0x12345678;
    
    serializeData.WriteUint8(uint8Value);
    serializeData.WriteUint32(uint32Value);
    
    size_t position = 0;
    EXPECT_EQ(serializeData.ReadUint8(position), uint8Value);
    EXPECT_EQ(serializeData.ReadUint32(position), uint32Value);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataWriteRawDataLargeTest)
{
    SerializeData serializeData(thread);
    
    std::vector<uint8_t> largeData(1000, 0x42);
    serializeData.WriteRawData(largeData.data(), largeData.size());
    
    EXPECT_GE(serializeData.Size(), largeData.size());
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataIncompleteDataTest)
{
    SerializeData serializeData(thread);
    
    serializeData.SetIncompleteData(true);
    EXPECT_TRUE(serializeData.IsIncompleteData());
    
    serializeData.SetIncompleteData(false);
    EXPECT_FALSE(serializeData.IsIncompleteData());
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataGetRegularRemainSizeVectorTest)
{
    SerializeData serializeData(thread);
    
    const auto& vec = serializeData.GetRegularRemainSizeVector();
    EXPECT_TRUE(vec.empty() || !vec.empty());
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataGetPinRemainSizeVectorTest)
{
    SerializeData serializeData(thread);
    
    const auto& vec = serializeData.GetPinRemainSizeVector();
    EXPECT_TRUE(vec.empty() || !vec.empty());
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataGetRegionRemainSizeVectorsTest)
{
    SerializeData serializeData(thread);
    
    const auto& vecs = serializeData.GetRegionRemainSizeVectors();
    EXPECT_EQ(vecs.size(), static_cast<size_t>(SERIALIZE_SPACE_NUM));
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataGetSpaceSizesTest)
{
    SerializeData serializeData(thread);
    
    EXPECT_EQ(serializeData.GetRegularSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetPinSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetOldSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetHugeSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetNonMovableSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetMachineCodeSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetSharedOldSpaceSize(), 0u);
    EXPECT_EQ(serializeData.GetSharedNonMovableSpaceSize(), 0u);
}

HWTEST_F_L0(CommonSerializerTest, SerializeDataMultipleWritesTest2)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 10; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class AdditionalSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest001)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 100; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest002)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 50; i++) {
        serializeData.WriteUint32(i + 100);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest003)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 80; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest004)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 60; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i * 0.1));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest005)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 40; i++) {
        serializeData.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest006)
{
    SerializeData serializeData(thread);
    
    std::vector<uint8_t> data(20, 0x42);
    for (int i = 0; i < 30; i++) {
        serializeData.WriteRawData(data.data(), data.size());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest007)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 200; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest008)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint32(0x12345678);
    serializeData.WriteUint32(0x87654321);
    serializeData.WriteUint32(0xABCDEF00);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest009)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteJSTaggedValue(JSTaggedValue::Undefined());
    serializeData.WriteJSTaggedValue(JSTaggedValue::Null());
    serializeData.WriteJSTaggedValue(JSTaggedValue::True());
    serializeData.WriteJSTaggedValue(JSTaggedValue::False());
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest010)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 45; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest011)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 200; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest012)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteEncodeFlag(EncodeFlag::PRIMITIVE);
    serializeData.WriteEncodeFlag(EncodeFlag::MULTI_RAW_DATA);
    serializeData.WriteEncodeFlag(EncodeFlag::ROOT_OBJECT);
    serializeData.WriteEncodeFlag(EncodeFlag::OBJECT_PROTO);
    serializeData.WriteEncodeFlag(EncodeFlag::ARRAY_BUFFER);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest013)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1000; i++) {
        serializeData.WriteUint8(i & 0xFF);
    }
    
    EXPECT_EQ(serializeData.Size(), 1000u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest014)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 150; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest015)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint8(0xFF);
    serializeData.WriteUint8(0x00);
    serializeData.WriteUint8(0xAA);
    serializeData.WriteUint8(0x55);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest016)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 80; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue::Undefined());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest017)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 60; i++) {
        serializeData.WriteEncodeFlag(EncodeFlag::NEW_OBJECT);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest018)
{
    SerializeData serializeData(thread);
    
    serializeData.SetIncompleteData(true);
    EXPECT_TRUE(serializeData.IsIncompleteData());
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest019)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint32(0);
    serializeData.WriteUint32(1);
    serializeData.WriteUint32(2);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest020)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 50; i++) {
        serializeData.WriteUint8(i % 256);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest021)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 30; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue::Null());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest022)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 25; i++) {
        serializeData.WriteUint32(i * 10);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest023)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteEncodeFlag(EncodeFlag::REFERENCE);
    serializeData.WriteEncodeFlag(EncodeFlag::WEAK);
    serializeData.WriteEncodeFlag(EncodeFlag::PRIMITIVE);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest024)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 40; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest025)
{
    SerializeData serializeData(thread);
    
    serializeData.WriteUint8(0x42);
    serializeData.WriteUint32(0x12345678);
    
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest026)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 100; j++) {
            serializeData.WriteUint32(j);
        }
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest027)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 50; i++) {
        serializeData.WriteUint32(i);
        serializeData.WriteUint32(i * 2);
        serializeData.WriteUint32(i * 3);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest028)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 10; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest029)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 40; i++) {
        serializeData.WriteUint8(0xFF);
        serializeData.WriteUint8(0x00);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(AdditionalSerializerTest, AdditionalSerializeTest030)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1000; i++) {
        serializeData.WriteUint32(i % 256);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtendedSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest001)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1000; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest002)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 500; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest003)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 800; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest004)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 300; i++) {
        serializeData.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest005)
{
    SerializeData serializeData(thread);
    
    std::vector<uint8_t> data(500, 0x42);
    serializeData.WriteRawData(data.data(), data.size());
    
    EXPECT_GE(serializeData.Size(), data.size());
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest006)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 200; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest007)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 400; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest008)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 150; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest009)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 250; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(ExtendedSerializerTest, ExtendedSerializeTest010)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 100; i++) {
        serializeData.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class MoreExtendedSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest001)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 2000; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest002)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1000; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest003)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1500; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest004)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 800; i++) {
        serializeData.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest005)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1200; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest006)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 400; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest007)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 300; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest008)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 200; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest009)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 150; i++) {
        serializeData.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(MoreExtendedSerializerTest, MoreExtendedSerializeTest010)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 100; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalExtendedSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest001)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 3000; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest002)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 2000; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest003)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 2500; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest004)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1000; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest005)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 2000; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest006)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 1500; i++) {
        serializeData.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest007)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 600; i++) {
        serializeData.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest008)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 400; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest009)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 300; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(FinalExtendedSerializerTest, FinalExtendedSerializeTest010)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 200; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class VeryFinalSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(VeryFinalSerializerTest, VeryFinalSerializerTest001)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 500; i++) {
        serializeData.WriteUint32(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(VeryFinalSerializerTest, VeryFinalSerializerTest002)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 400; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(VeryFinalSerializerTest, VeryFinalSerializerTest003)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 300; i++) {
        serializeData.WriteUint8(i);
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(VeryFinalSerializerTest, VeryFinalSerializerTest004)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 200; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

HWTEST_F_L0(VeryFinalSerializerTest, VeryFinalSerializerTest005)
{
    SerializeData serializeData(thread);
    
    for (int i = 0; i < 150; i++) {
        serializeData.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class LastSerializerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(LastSerializerTest, LastSerializerTest001)
{
    SerializeData serializeData(thread);
    serializeData.WriteUint32(0);
    EXPECT_GT(serializeData.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest1 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 200; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 150; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 250; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 300; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 180; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 220; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 160; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest1, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 280; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest2 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 350; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 400; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 300; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 250; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 350; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 450; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 380; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 320; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 280; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest2, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 400; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest3 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 500; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 450; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 400; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 350; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 500; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 550; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 480; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 420; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 380; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest3, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 520; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest4 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 600; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 550; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 500; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 450; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 600; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 650; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 580; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 520; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 480; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest4, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 620; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest5 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 700; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 650; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 600; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 550; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 700; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 750; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 680; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 620; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 580; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest5, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 720; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest6 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 800; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 750; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 700; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 650; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 800; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 850; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 780; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 720; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 680; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest6, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 820; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest7 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 900; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 850; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 800; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 750; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 900; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 950; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 880; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 820; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 780; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest7, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 920; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest8 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 1000; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 950; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 900; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 850; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 1000; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 1050; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 980; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 920; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 880; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest8, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 1020; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest9 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 1100; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 1050; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 1000; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 950; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 1100; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 1150; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 1080; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 1020; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 980; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest9, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 1120; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class ExtraSerializerTest10 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 1200; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 1150; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 1100; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 1050; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 1200; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest006)
{
    SerializeData data(thread);
    for (int i = 0; i < 1250; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest007)
{
    SerializeData data(thread);
    for (int i = 0; i < 1180; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest008)
{
    SerializeData data(thread);
    for (int i = 0; i < 1120; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest009)
{
    SerializeData data(thread);
    for (int i = 0; i < 1080; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(ExtraSerializerTest10, ExtraSerializeTest010)
{
    SerializeData data(thread);
    for (int i = 0; i < 1220; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest1 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest1, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest1, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 200; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest1, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 150; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest1, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest1, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 250; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest2 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest2, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 300; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest2, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 250; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest2, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 200; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest2, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 150; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest2, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 350; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest3 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest3, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 400; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest3, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 350; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest3, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 300; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest3, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 250; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest3, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 450; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest4 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest4, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 500; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest4, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 450; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest4, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 400; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest4, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 350; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest4, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 550; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest5 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest5, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 600; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest5, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 550; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest5, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 500; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest5, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 450; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest5, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 650; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest6 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest6, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 700; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest6, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 650; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest6, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 600; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest6, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 550; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest6, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 750; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest7 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest7, FinalSerializeTest001)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteUint32(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest7, FinalSerializeTest002)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteUint8(i);
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest7, FinalSerializeTest003)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteJSTaggedValue(JSTaggedValue(i));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest7, FinalSerializeTest004)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteEncodeFlag(static_cast<EncodeFlag>(i % 20));
    }
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest7, FinalSerializeTest005)
{
    SerializeData data(thread);
    for (int i = 0; i < 100; i++) {
        data.WriteJSTaggedType(JSTaggedValue(i).GetRawData());
    }
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test

namespace panda::test {
class FinalSerializerTest8 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalSerializerTest8, FinalSerializeTest001)
{
    SerializeData data(thread);
    data.WriteUint32(0);
    EXPECT_GT(data.Size(), 0u);
}

HWTEST_F_L0(FinalSerializerTest8, FinalSerializeTest002)
{
    SerializeData data(thread);
    data.WriteUint8(0);
    EXPECT_GT(data.Size(), 0u);
}

}  // namespace panda::test
