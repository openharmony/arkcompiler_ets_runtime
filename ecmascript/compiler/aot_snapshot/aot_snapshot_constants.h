/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_CONSTANTS_H
#define ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_CONSTANTS_H

#include <cstdint>

#include "ecmascript/compiler/aot_snapshot/snapshot_global_data.h"

namespace panda::ecmascript {
using CP_TOP_ITEM = kungfu::SnapshotGlobalData::CP_TOP_ITEM;

class AOTSnapshotConstants final {
public:
    static constexpr uint8_t SNAPSHOT_DATA_ITEM_SIZE = static_cast<uint8_t>(CP_TOP_ITEM::COUNT);
    static constexpr uint8_t SNAPSHOT_CP_ARRAY_ITEM_SIZE = 2;

private:
    AOTSnapshotConstants() = default;
    ~AOTSnapshotConstants() = default;
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_AOT_SNAPSHOT_AOT_SNAPSHOT_CONSTANTS_H
