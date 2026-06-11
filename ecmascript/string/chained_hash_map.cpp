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

#include "ecmascript/string/chained_hash_map.h"

#include <algorithm>

#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

namespace panda::ecmascript {
// Reads the configured bucket-count exponent (MAP_BIT) from the system
// parameter "const.ark.string_table_map_bit", falling back to DEFAULT_MAP_BIT
// on non-OHOS / standalone builds. The result is clamped to
// [MIN_MAP_BIT, MAX_MAP_BIT]. MIN_MAP_BIT == SWEEP_PARTITION_BIT lets sweeping
// split the map evenly into SWEEP_PARTITION_COUNT partitions.
uint32_t GetConfiguredMapBit()
{
    uint32_t mapBit = ChainedHashMapConfig::DEFAULT_MAP_BIT;
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
    mapBit = OHOS::system::GetUintParameter<uint32_t>("const.ark.string_table_map_bit",
                                                      ChainedHashMapConfig::DEFAULT_MAP_BIT);
    LOG_ECMA(INFO) << "GetConfiguredMapBit: " << mapBit;
#endif
    return std::clamp(mapBit, ChainedHashMapConfig::MIN_MAP_BIT, ChainedHashMapConfig::MAX_MAP_BIT);
}
} // namespace panda::ecmascript
