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
#ifndef MAPLEBE_INCLUDE_TARGET_MACHINE_H
#define MAPLEBE_INCLUDE_TARGET_MACHINE_H
#include "types_def.h"

namespace maplebe {
using namespace maple;
enum TargetKind : uint8 {
    kAArch64,
    kX8664,
    kRiscV,
    kArm32,
    kArk
};

class TargetMachine {
public:
    TargetMachine(TargetKind kind) : kind(kind) {}
    bool isAArch64()
    {
        return kind == kAArch64;
    }

    bool isX8664()
    {
        return kind == kX8664;
    }

    bool isRiscV()
    {
        return kind == kRiscV;
    }

    bool isArm32()
    {
        return kind == kArm32;
    }

    bool isArk()
    {
        return kind == kArk;
    }

private:
    TargetKind kind;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_TARGET_MACHINE_H */
