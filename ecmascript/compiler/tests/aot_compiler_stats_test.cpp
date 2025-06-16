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

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "ecmascript/compiler/aot_compiler_stats.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class AotCompilerStatsTest : public testing::Test {
public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

HWTEST_F_L0(AotCompilerStatsTest, AotCompilerStatsTestt_001)
{
    std::string anFile = "/data/app/el1/public/aot_compiler/ark_cache/com.test.bundleName/arm64";
    AotCompilerStats aotCompilerStats;
    ASSERT_EQ(aotCompilerStats.SendDataPartitionSysEvent(anFile), 0);
}
} // namespace panda::test