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


#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_string.h"
#include "common_components/platform/string_hash.h"

using namespace panda::ecmascript;
using namespace common;

namespace panda::test {
class EcmaStringHashTest : public BaseTestWithScope<false> {
public:
    template <typename T>
    static uint32_t ComputeHashRaw(const T *data, size_t size, uint32_t hashSeed)
    {
        uint32_t hash = hashSeed;
        for (size_t i = 0; i < size; i++) {
            hash = hash * StringHash::HASH_MULTIPLY + data[i];
        }
        return hash;
    }

    template <typename T>
    static uint32_t ComputeHashOpt(const T *data, size_t size, uint32_t hashSeed)
    {
        return BaseString::ComputeHashForData(data, size, hashSeed);
    }

    static uint32_t ComputeConcatHash(
        const JSHandle<EcmaString> &firstString,
        const JSHandle<EcmaString> &secondString)
    {
        return EcmaString::CalculateConcatHashCode(firstString, secondString);
    }
};
#if ENABLE_NEXT_OPTIMIZATION
/*
* @tc.name: ComputeHashForData
* @tc.desc: For short data,
* test new hashcode compute function result equals original algorithm: hash = hash*31 + data[i]
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaStringHashTest, ComputeHashForData_001)
{
    std::string data = "Hello World! OpenHarmony";
    uint32_t rawHash8 = ComputeHashRaw(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
    uint32_t optHash8 = ComputeHashOpt(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
    EXPECT_EQ(rawHash8, optHash8);

    std::u16string data16 = u"Hello World! 鸿蒙 OpenHarmony";
    uint32_t rawHash16 = ComputeHashRaw(reinterpret_cast<uint16_t*>(data16.data()), data16.size(), 0);
    uint32_t optHash16 = ComputeHashOpt(reinterpret_cast<uint16_t*>(data16.data()), data16.size(), 0);
    EXPECT_EQ(rawHash16, optHash16);
}

/*
* @tc.name: ComputeHashForData
* @tc.desc: For medium data,
* test new hashcode compute function result equals original algorithm: hash = hash*31 + data[i]
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaStringHashTest, ComputeHashForData_002)
{
    std::string data = "Hello World! abcdefghijklmnopqrstuvwxyz0123456789 !#$%&'()*+,-./:;=?@[]^_`{|}~";
    uint32_t rawHash8 = ComputeHashRaw(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
    uint32_t optHash8 = ComputeHashOpt(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
    EXPECT_EQ(rawHash8, optHash8);

    std::u16string data16 = u"Hello World! abcdefghijklmnopqrstuvwxyz0123456789 !#$%&'()*+,-./:;=?@[]^_`{|}~ 鸿蒙";
    uint32_t rawHash16 = ComputeHashRaw(reinterpret_cast<uint16_t*>(data16.data()), data16.size(), 0);
    uint32_t optHash16 = ComputeHashOpt(reinterpret_cast<uint16_t*>(data16.data()), data16.size(), 0);
    EXPECT_EQ(rawHash16, optHash16);
}

/*
* @tc.name: ComputeHashForData
* @tc.desc: For long data,
* test new hashcode compute function result equals original algorithm: hash = hash*31 + data[i]
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaStringHashTest, ComputeHashForData_003)
{
    std::string data = "Hello World! abcdefghijklmnopqrstuvwxyz0123456789 !#$%&'()*+,-./:;=?@[]^_`{|}~";
    std::u16string data16 = u"Hello World! abcdefghijklmnopqrstuvwxyz0123456789 !#$%&'()*+,-./:;=?@[]^_`{|}~ 鸿蒙";
    for (int i = 0; i < 100; i++) {
        data += std::to_string(i);
        std::wstring wstring = std::to_wstring(i);
        data16 += std::u16string(wstring.begin(), wstring.end());
    }
    uint32_t rawHash8 = ComputeHashRaw(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
    uint32_t optHash8 = ComputeHashOpt(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
    EXPECT_EQ(rawHash8, optHash8);

    uint32_t rawHash16 = ComputeHashRaw(reinterpret_cast<uint16_t*>(data16.data()), data16.size(), 0);
    uint32_t optHash16 = ComputeHashOpt(reinterpret_cast<uint16_t*>(data16.data()), data16.size(), 0);
    EXPECT_EQ(rawHash16, optHash16);
}

HWTEST_F_L0(EcmaStringHashTest, CalculateConcatHashCode)
{
    std::string firstData = "Hello World! OpenHarmony";
    std::string secondData = "Hello World! OpenHarmony";
    JSHandle<EcmaString> firstString(thread, EcmaStringAccessor::CreateFromUtf8(
        instance, reinterpret_cast<uint8_t*>(firstData.data()), firstData.size(), true));
    JSHandle<EcmaString> secondString(thread, EcmaStringAccessor::CreateFromUtf8(
        instance, reinterpret_cast<uint8_t*>(secondData.data()), secondData.size(), true));

    std::string totalString = firstData + secondData;
    uint32_t totalHash = ComputeHashOpt(reinterpret_cast<uint8_t*>(totalString.data()), totalString.size(), 0);
    uint32_t concatHash = ComputeConcatHash(firstString, secondString);
    EXPECT_EQ(totalHash, concatHash);
}
#endif
}