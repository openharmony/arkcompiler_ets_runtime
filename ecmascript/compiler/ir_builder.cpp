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

#include "ecmascript/compiler/ir_builder.h"

namespace panda::ecmascript::kungfu {
bool IsAddIntergerType(MachineType machineType)
{
    switch (machineType) {
        case MachineType::I8:
        case MachineType::I16:
        case MachineType::I32:
        case MachineType::I64:
        case MachineType::ARCH:
            return true;
        default:
            return false;
    }
}

bool IsMulIntergerType(MachineType machineType)
{
    switch (machineType) {
        case MachineType::I32:
        case MachineType::I64:
        case MachineType::ARCH:
            return true;
        default:
            return false;
    }
}
}  // namespace panda::ecmascript::kungfu