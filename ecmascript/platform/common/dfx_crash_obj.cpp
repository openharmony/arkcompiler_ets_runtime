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

#include "ecmascript/platform/dfx_crash_obj.h"

namespace panda::ecmascript {
uintptr_t SetCrashObject([[maybe_unused]] DFXObjectType type, [[maybe_unused]] uintptr_t addr)
{
    return 0;
}

uintptr_t ResetCrashObject([[maybe_unused]] uintptr_t addr)
{
    return 0;
}
}  // namespace panda::ecmascript