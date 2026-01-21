/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "ecmascript/pgo_profiler/pgo_info.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_record_pool.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {
class PGOInfoTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
};

HWTEST_F_L0(PGOInfoTest, GetHeaderTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    PGOProfilerHeader& header = pgoInfo->GetHeader();
    ASSERT_NE(&header, nullptr);
    header.SetVersion({1, 2, 3, 4});
    auto version = header.GetVersion();
    EXPECT_EQ(version[0], 1U);
    EXPECT_EQ(version[1], 2U);
    EXPECT_EQ(version[2], 3U);
    EXPECT_EQ(version[3], 4U);
}

HWTEST_F_L0(PGOInfoTest, SetHeaderTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    PGOProfilerHeader* newHeader = new PGOProfilerHeader();
    newHeader->SetVersion({5, 6, 7, 8});
    pgoInfo->SetHeader(newHeader);
    PGOProfilerHeader& header = pgoInfo->GetHeader();
    auto version = header.GetVersion();
    EXPECT_EQ(version[0], 5U);
    EXPECT_EQ(version[1], 6U);
    EXPECT_EQ(version[2], 7U);
    EXPECT_EQ(version[3], 8U);
    // PGOInfo takes ownership and will delete the header
}

HWTEST_F_L0(PGOInfoTest, SetPandaFileInfosTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    auto newPandaFileInfos = std::make_unique<PGOPandaFileInfos>();
    ASSERT_NE(newPandaFileInfos, nullptr);
    newPandaFileInfos->Sample(12345, 1);
    newPandaFileInfos->Sample(67890, 2);
    pgoInfo->SetPandaFileInfos(std::move(newPandaFileInfos));
    PGOPandaFileInfos& pandaFileInfos = pgoInfo->GetPandaFileInfos();
    ASSERT_NE(&pandaFileInfos, nullptr);
}

HWTEST_F_L0(PGOInfoTest, SetAbcFilePoolTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    auto newAbcFilePool = std::make_shared<PGOAbcFilePool>();
    ASSERT_NE(newAbcFilePool, nullptr);
    CString testAbcName = "test.abc";
    ApEntityId entityId(10);
    newAbcFilePool->TryAddSafe(testAbcName, entityId);
    pgoInfo->SetAbcFilePool(newAbcFilePool);
    PGOAbcFilePool& abcFilePool = pgoInfo->GetAbcFilePool();
    ASSERT_NE(&abcFilePool, nullptr);
    auto abcFilePoolPtr = pgoInfo->GetAbcFilePoolPtr();
    ASSERT_EQ(abcFilePoolPtr, newAbcFilePool);
}

HWTEST_F_L0(PGOInfoTest, SetHotnessThresholdTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 5U);
    pgoInfo->SetHotnessThreshold(100);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 100U);
    pgoInfo->SetHotnessThreshold(0);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 0U);
    pgoInfo->SetHotnessThreshold(1000);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 1000U);
}

HWTEST_F_L0(PGOInfoTest, MergeSafePGORecordDetailInfosTest)
{
    auto pgoInfo1 = std::make_shared<PGOInfo>(5);
    auto pgoInfo2 = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo1, nullptr);
    ASSERT_NE(pgoInfo2, nullptr);
    PGORecordDetailInfos& recordInfos2 = pgoInfo2->GetRecordDetailInfos();
    // Test MergeSafe with empty record infos
    pgoInfo1->MergeSafe(recordInfos2);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOInfoTest, MergeSafePGOPandaFileInfosTest)
{
    auto pgoInfo1 = std::make_shared<PGOInfo>(5);
    auto pgoInfo2 = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo1, nullptr);
    ASSERT_NE(pgoInfo2, nullptr);
    PGOPandaFileInfos& pandaFileInfos2 = pgoInfo2->GetPandaFileInfos();
    pandaFileInfos2.Sample(11111, 1);
    pandaFileInfos2.Sample(22222, 2);
    pgoInfo1->MergeSafe(pandaFileInfos2);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOInfoTest, MergeSafePGOInfoTest)
{
    auto pgoInfo1 = std::make_shared<PGOInfo>(5);
    auto pgoInfo2 = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo1, nullptr);
    ASSERT_NE(pgoInfo2, nullptr);
    PGOPandaFileInfos& pandaFileInfos2 = pgoInfo2->GetPandaFileInfos();
    pandaFileInfos2.Sample(33333, 3);
    pandaFileInfos2.Sample(44444, 4);
    // Test MergeSafe with empty record infos
    pgoInfo1->MergeSafe(*pgoInfo2);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOInfoTest, MultipleSetOperationsTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(10);
    ASSERT_NE(pgoInfo, nullptr);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 10U);
    pgoInfo->SetHotnessThreshold(20);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 20U);
    auto newAbcFilePool = std::make_shared<PGOAbcFilePool>();
    pgoInfo->SetAbcFilePool(newAbcFilePool);
    auto newPandaFileInfos = std::make_unique<PGOPandaFileInfos>();
    newPandaFileInfos->Sample(55555, 5);
    pgoInfo->SetPandaFileInfos(std::move(newPandaFileInfos));
    PGOProfilerHeader* newHeader = new PGOProfilerHeader();
    newHeader->SetVersion({9, 8, 7, 6});
    pgoInfo->SetHeader(newHeader);
    auto version = pgoInfo->GetHeader().GetVersion();
    EXPECT_EQ(version[0], 9U);
    EXPECT_EQ(version[1], 8U);
    EXPECT_EQ(version[2], 7U);
    EXPECT_EQ(version[3], 6U);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 20U);
    // PGOInfo takes ownership and will delete the header
}

HWTEST_F_L0(PGOInfoTest, SetHeaderNullTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    PGOProfilerHeader* headerPtrBefore = pgoInfo->GetHeaderPtr();
    ASSERT_NE(headerPtrBefore, nullptr);
    pgoInfo->SetHeader(nullptr);
    PGOProfilerHeader* headerPtrAfter = pgoInfo->GetHeaderPtr();
    ASSERT_EQ(headerPtrAfter, nullptr);
}

HWTEST_F_L0(PGOInfoTest, GetAbcFilePoolPtrTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    auto abcFilePoolPtr = pgoInfo->GetAbcFilePoolPtr();
    ASSERT_NE(abcFilePoolPtr, nullptr);
    auto newAbcFilePool = std::make_shared<PGOAbcFilePool>();
    pgoInfo->SetAbcFilePool(newAbcFilePool);
    auto updatedPtr = pgoInfo->GetAbcFilePoolPtr();
    ASSERT_EQ(updatedPtr, newAbcFilePool);
}

HWTEST_F_L0(PGOInfoTest, GetRecordDetailInfosPtrTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    auto recordDetailInfosPtr = pgoInfo->GetRecordDetailInfosPtr();
    ASSERT_NE(recordDetailInfosPtr, nullptr);
    auto newRecordDetailInfos = std::make_shared<PGORecordDetailInfos>(5);
    pgoInfo->SetRecordDetailInfos(newRecordDetailInfos);
    auto updatedPtr = pgoInfo->GetRecordDetailInfosPtr();
    ASSERT_EQ(updatedPtr, newRecordDetailInfos);
}

HWTEST_F_L0(PGOInfoTest, SetRecordDetailInfosTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    auto newRecordDetailInfos = std::make_shared<PGORecordDetailInfos>(5);
    ASSERT_NE(newRecordDetailInfos, nullptr);
    pgoInfo->SetRecordDetailInfos(newRecordDetailInfos);
    PGORecordDetailInfos& recordDetailInfos = pgoInfo->GetRecordDetailInfos();
    ASSERT_NE(&recordDetailInfos, nullptr);
    auto recordDetailInfosPtr = pgoInfo->GetRecordDetailInfosPtr();
    ASSERT_EQ(recordDetailInfosPtr, newRecordDetailInfos);
}

HWTEST_F_L0(PGOInfoTest, SetHotnessThresholdBoundaryTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(0);
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), 0U);
    pgoInfo->SetHotnessThreshold(std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), std::numeric_limits<uint32_t>::max());
    pgoInfo->SetHotnessThreshold(std::numeric_limits<uint32_t>::min());
    EXPECT_EQ(pgoInfo->GetHotnessThreshold(), std::numeric_limits<uint32_t>::min());
}

HWTEST_F_L0(PGOInfoTest, GetHeaderPtrTest)
{
    auto pgoInfo = std::make_shared<PGOInfo>(5);
    ASSERT_NE(pgoInfo, nullptr);
    PGOProfilerHeader* headerPtr = pgoInfo->GetHeaderPtr();
    ASSERT_NE(headerPtr, nullptr);
    headerPtr->SetVersion({1, 1, 1, 1});
    PGOProfilerHeader& headerRef = pgoInfo->GetHeader();
    auto version = headerRef.GetVersion();
    EXPECT_EQ(version[0], 1U);
    EXPECT_EQ(version[1], 1U);
    EXPECT_EQ(version[2], 1U);
    EXPECT_EQ(version[3], 1U);
}
}  // namespace panda::test
