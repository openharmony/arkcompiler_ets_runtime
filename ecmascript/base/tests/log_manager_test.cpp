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

#include "ecmascript/base/log_manager.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript::base;

namespace panda::test {

class LogManagerTest : public BaseTestWithScope<false> {
protected:
    void SetUp() override
    {
        logManager_.ClearRateLimitCache();
        logManager_.SetRateLimitParams(DEFAULT_MAX_ENTRIES, DEFAULT_PRINT_INTERVAL);
    }

    void TearDown() override
    {
        logManager_.ClearRateLimitCache();
    }

    static constexpr size_t DEFAULT_MAX_ENTRIES = 30;
    static constexpr uint64_t DEFAULT_PRINT_INTERVAL = 1000;
    LogManager logManager_;
};

/**
 * @tc.name: ComputeHashKey_001
 * @tc.desc: Test ComputeHashKey returns consistent hash for the same input
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ComputeHashKey_001)
{
    std::string key1 = "TypeError: Cannot read property";
    std::string key2 = "TypeError: Cannot read property";
    uint64_t hash1 = LogManager::ComputeHashKey(key1);
    uint64_t hash2 = LogManager::ComputeHashKey(key2);
    EXPECT_EQ(hash1, hash2);
}

/**
 * @tc.name: ComputeHashKey_002
 * @tc.desc: Test ComputeHashKey returns different hashes for different inputs
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ComputeHashKey_002)
{
    std::string key1 = "TypeError: Cannot read property";
    std::string key2 = "RangeError: Invalid array length";
    uint64_t hash1 = LogManager::ComputeHashKey(key1);
    uint64_t hash2 = LogManager::ComputeHashKey(key2);
    EXPECT_NE(hash1, hash2);
}

/**
 * @tc.name: ComputeHashKey_003
 * @tc.desc: Test ComputeHashKey with empty string
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ComputeHashKey_003)
{
    uint64_t hash = LogManager::ComputeHashKey("");
    // Should return a valid hash value (no crash, consistent result)
    uint64_t hash2 = LogManager::ComputeHashKey("");
    EXPECT_EQ(hash, hash2);
}

/**
 * @tc.name: ShouldPrintByRateLimit_001
 * @tc.desc: First occurrence of a key should always be printed
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_001)
{
    uint64_t key = LogManager::ComputeHashKey("ErrorKey1");
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key));
}

/**
 * @tc.name: ShouldPrintByRateLimit_002
 * @tc.desc: Second occurrence of a key should NOT be printed (default interval=1000)
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_002)
{
    uint64_t key = LogManager::ComputeHashKey("ErrorKey2");
    logManager_.ShouldPrintByRateLimit(key); // first: true
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key)); // second: false (count=2, 2%1000!=0)
}

/**
 * @tc.name: ShouldPrintByRateLimit_003
 * @tc.desc: Print should occur when count reaches printInterval
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_003)
{
    logManager_.SetRateLimitParams(DEFAULT_MAX_ENTRIES, 5); // interval=5
    uint64_t key = LogManager::ComputeHashKey("ErrorKey3");
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key));   // count=1, print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=2
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=3
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=4
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key));   // count=5, print (5%5==0)
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=6
}

/**
 * @tc.name: ShouldPrintByRateLimit_004
 * @tc.desc: Print at multiples of printInterval
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_004)
{
    logManager_.SetRateLimitParams(DEFAULT_MAX_ENTRIES, 3); // interval=3
    uint64_t key = LogManager::ComputeHashKey("ErrorKey4");
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key));   // count=1, print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=2
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key));   // count=3, print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=4
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key));  // count=5
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key));   // count=6, print
}

/**
 * @tc.name: ShouldPrintByRateLimit_005
 * @tc.desc: Different keys are tracked independently
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_005)
{
    uint64_t keyA = LogManager::ComputeHashKey("ErrorA");
    uint64_t keyB = LogManager::ComputeHashKey("ErrorB");
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(keyA));  // A first: print
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(keyB));  // B first: print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(keyA)); // A second: no print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(keyB)); // B second: no print
}

/**
 * @tc.name: ShouldPrintByRateLimit_006
 * @tc.desc: LRU eviction: when maxEntries exceeded, oldest key is evicted
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_006)
{
    logManager_.SetRateLimitParams(3, 1000); // maxEntries=3, interval=1000
    uint64_t key1 = LogManager::ComputeHashKey("Key1");
    uint64_t key2 = LogManager::ComputeHashKey("Key2");
    uint64_t key3 = LogManager::ComputeHashKey("Key3");
    uint64_t key4 = LogManager::ComputeHashKey("Key4");

    logManager_.ShouldPrintByRateLimit(key1); // insert key1
    logManager_.ShouldPrintByRateLimit(key2); // insert key2
    logManager_.ShouldPrintByRateLimit(key3); // insert key3, LRU is full

    // key4 evicts key1 (LRU), so key1 should be treated as new key again
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key4)); // key4 first: print
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key1)); // key1 re-inserted (evicted): print
}

/**
 * @tc.name: ShouldPrintByRateLimit_007
 * @tc.desc: LRU access promotes key to front, preventing eviction
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ShouldPrintByRateLimit_007)
{
    logManager_.SetRateLimitParams(3, 1000); // maxEntries=3
    uint64_t key1 = LogManager::ComputeHashKey("Key1");
    uint64_t key2 = LogManager::ComputeHashKey("Key2");
    uint64_t key3 = LogManager::ComputeHashKey("Key3");
    uint64_t key4 = LogManager::ComputeHashKey("Key4");

    logManager_.ShouldPrintByRateLimit(key1); // LRU: [key1]
    logManager_.ShouldPrintByRateLimit(key2); // LRU: [key2, key1]
    logManager_.ShouldPrintByRateLimit(key3); // LRU: [key3, key2, key1]

    // Access key1 to promote it to front: LRU becomes [key1, key3, key2]
    logManager_.ShouldPrintByRateLimit(key1);

    // key4 evicts key2 (now LRU), key1 should remain
    logManager_.ShouldPrintByRateLimit(key4);

    // key2 was evicted, should be treated as new entry (print=true)
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key2));
    // key1 still exists in cache (was promoted), should not print on 3rd access
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key1)); // count=3, 3%1000!=0
}

/**
 * @tc.name: ClearRateLimitCache_001
 * @tc.desc: After clearing cache, all keys are treated as new
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ClearRateLimitCache_001)
{
    uint64_t key = LogManager::ComputeHashKey("ClearTest");
    logManager_.ShouldPrintByRateLimit(key); // first: print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(key)); // second: no print

    logManager_.ClearRateLimitCache();
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(key)); // treated as new: print
}

/**
 * @tc.name: SetRateLimitParams_001
 * @tc.desc: Changing maxEntries and printInterval takes effect
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, SetRateLimitParams_001)
{
    logManager_.SetRateLimitParams(2, 2); // maxEntries=2, interval=2
    uint64_t keyA = LogManager::ComputeHashKey("ParamA");
    uint64_t keyB = LogManager::ComputeHashKey("ParamB");
    uint64_t keyC = LogManager::ComputeHashKey("ParamC");

    logManager_.ShouldPrintByRateLimit(keyA);
    logManager_.ShouldPrintByRateLimit(keyB);

    // LRU is full (maxEntries=2), keyC evicts keyA
    logManager_.ShouldPrintByRateLimit(keyC);

    // keyA was evicted, re-inserting should print (new entry)
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(keyA));

    // Test interval: keyA count=2 after re-insert, 2%2==0 should print
    EXPECT_TRUE(logManager_.ShouldPrintByRateLimit(keyA)); // count=2, 2%2==0: print
    EXPECT_FALSE(logManager_.ShouldPrintByRateLimit(keyA)); // count=3, 3%2!=0: no print
}

/**
 * @tc.name: ComputeHashKey_LongString
 * @tc.desc: ComputeHashKey handles long strings without overflow
 * @tc.type: FUNC
 */
HWTEST_F_L0(LogManagerTest, ComputeHashKey_LongString)
{
    std::string longKey(10000, 'a');
    uint64_t hash = LogManager::ComputeHashKey(longKey);
    // Should not crash and return consistent result
    EXPECT_EQ(hash, LogManager::ComputeHashKey(longKey));
}

}  // namespace panda::test
