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

#include "common_components/heap/collector/gc_request.h"
#include "common_components/heap/collector/gc_stats.h"
#include "common_components/base/time_utils.h"

#include "common_components/tests/test_helper.h"

using namespace common;

uint64_t fakeCurrentTime = 0;

namespace common {
uint64_t TimeUtil::NanoSeconds()
{
    return fakeCurrentTime;
}
} // namespace panda

class GCRequestTest : public common::test::BaseTestWithScope {
protected:
    void SetUp() override
    {
        fakeCurrentTime = 0;
    }

    void SetPrevRequestTime(GCRequest& req, uint64_t time)
    {
        req.SetPrevRequestTime(time);
    }

    void SetMinInterval(GCRequest& req, uint64_t intervalNs)
    {
        req.SetMinInterval(intervalNs);
    }
};

void SetLastGCFinishTime(uint64_t time)
{
    GCStats::SetPrevGCFinishTime(time);
}


bool ShouldBeIgnoredWithReason(GCReason reason, uint64_t minIntervalNs, uint64_t prevReqTime, uint64_t now,
                               uint64_t lastGCFinishTime = 0)
{
    fakeCurrentTime = now;
    SetLastGCFinishTime(lastGCFinishTime);

    GCRequest req = {
        reason,
        "", // name
        false, // isSync
        false, // isConcurrent
        minIntervalNs,
        prevReqTime
    };

    return req.ShouldBeIgnored();
}


HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_Heu_ReturnsTrue_IfFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_HEU,
        1000,          // minIntervalNs
        fakeCurrentTime - 500, // prevReqTime < now - minInterval
        fakeCurrentTime
    );
    EXPECT_TRUE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_Native_ReturnsTrue_IfFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_NATIVE,
        1000,
        fakeCurrentTime - 500,
        fakeCurrentTime,
        fakeCurrentTime - 500 // lastGCFinishTime
    );
    EXPECT_TRUE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_Native_ReturnsFalse_IfNotFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_NATIVE,
        1000,
        fakeCurrentTime - 1500,
        fakeCurrentTime,
        fakeCurrentTime - 1500
    );
    EXPECT_FALSE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_OOM_ReturnsTrue_IfFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_OOM,
        1000,
        fakeCurrentTime - 500,
        fakeCurrentTime
    );
    EXPECT_TRUE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_OOM_ReturnsFalse_IfNotFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_OOM,
        1000,
        fakeCurrentTime - 1500,
        fakeCurrentTime
    );
    EXPECT_FALSE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_Force_ReturnsTrue_IfFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_FORCE,
        1000,
        fakeCurrentTime - 500,
        fakeCurrentTime
    );
    EXPECT_TRUE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_Force_ReturnsFalse_IfNotFrequent) {
    bool result = ShouldBeIgnoredWithReason(
        GC_REASON_FORCE,
        1000,
        fakeCurrentTime - 1500,
        fakeCurrentTime
    );
    EXPECT_FALSE(result);
}

HWTEST_F_L0(GCRequestTest, ShouldBeIgnored_User_ReturnsFalse) {
    fakeCurrentTime = 1000;

    GCRequest req = {
        GC_REASON_USER,
        "",
        false,
        false,
        0,
        0
    };

    EXPECT_FALSE(req.ShouldBeIgnored());
}