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

#ifndef ECMASCRIPT_BUILTINS_GLOBAL_URI_H
#define ECMASCRIPT_BUILTINS_GLOBAL_URI_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
static constexpr uint8_t SIX_BIT_MASK = 0x3F;
static constexpr uint16_t BITMAP_SIZE = 0x3F;
static constexpr uint64_t RESERVED_URI_MASK = 0xAC00985000000000; // $&+,/:;=?
static constexpr uint64_t MARK_URI_MASK = 0x678200000000; // !'()*-.
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_GLOBAL_URI_H
