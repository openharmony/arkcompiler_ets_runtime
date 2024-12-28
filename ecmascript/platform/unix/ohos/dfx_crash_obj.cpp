/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/dfx_crash_obj.h"

/* dfx interface, type 0 for string */
extern "C" uintptr_t DFX_SetCrashObj(uint8_t type, uintptr_t addr);
extern "C" uintptr_t DFX_ResetCrashObj(uintptr_t addr);

namespace panda::ecmascript {
uintptr_t SetCrashObject(DFXObjectType type, uintptr_t addr)
{
    return DFX_SetCrashObj(static_cast<uint8_t>(type), addr);
}

uintptr_t ResetCrashObject(uintptr_t addr)
{
    return DFX_ResetCrashObj(addr);
}
}  // namespace panda::ecmascript