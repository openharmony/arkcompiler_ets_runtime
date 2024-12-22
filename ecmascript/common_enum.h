/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMMON_ENUM_H
#define ECMASCRIPT_COMMON_ENUM_H

namespace panda {
namespace ecmascript {
enum BarrierMode { SKIP_BARRIER, WRITE_BARRIER, READ_BARRIER };

enum CheckIdleGCType {
    VSYNC,
    LOOPER
};

enum class MarkType : uint8_t {
    MARK_EDEN,
    MARK_YOUNG,
    MARK_FULL
};

/*
 * TriggerGCType is categorized according to the scope the GC expects to cover.
 * Different GC algorithms may be applied to different GC types.
 * For example, SemiSpace GC for young generation GC, Mark-Sweep-Compact for full GC, etc.
 */
enum TriggerGCType {
    EDEN_GC,
    // GC is expected to cover young space only;
    YOUNG_GC,
    // GC is expected to cover young space and necessary old spaces;
    OLD_GC,
    // GC is expected to cover all valid heap spaces;
    FULL_GC,
    // GC is expected to compress objects into appspawn space;
    APPSPAWN_FULL_GC,
    SHARED_GC,
    SHARED_FULL_GC,
    APPSPAWN_SHARED_FULL_GC,
    GC_TYPE_LAST
};

enum class GCReason : uint8_t {
    ALLOCATION_LIMIT,
    ALLOCATION_FAILED,
    IDLE,
    SWITCH_BACKGROUND,
    EXTERNAL_TRIGGER,
    WORKER_DESTRUCTION,
    TRIGGER_BY_JS,
    TRIGGER_BY_ARKUI,
    TRIGGER_BY_ABILITY,
    TRIGGER_BY_MEM_TOOLS,
    TRIGGER_BY_TASKPOOL,
    HINT_GC,
    IDLE_NATIVE,
    OTHER
};

enum class RequestAotMode : uint8_t {
    RE_COMPILE_ON_IDLE = 0
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_COMMON_ENUM_H
