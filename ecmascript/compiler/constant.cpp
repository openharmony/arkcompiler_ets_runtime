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

#include "ecmascript/compiler/constant.h"
#include "ecmascript/base/bit_helper.h"
#include "ecmascript/compiler/gate_accessor.h"
#include <cstdint>

namespace panda::ecmascript::kungfu {

Constant GetConstant(GateAccessor acc, GateRef gate)
{
    auto raw = acc.GetConstantValue(gate);
    auto gate_type = acc.GetGateType(gate);
    if (gate_type.IsNJSValueType()) {
        switch (acc.GetMachineType(gate)) {
            case MachineType::I1:
                return Constant(static_cast<bool>(raw));
            case MachineType::I32:
                return Constant(static_cast<int>(raw));
            case MachineType::I64:
                return Constant(static_cast<int64_t>(raw));
            case MachineType::F64:
                return Constant(base::bit_cast<double>(raw));
            default:
                return Constant();
        }
        return Constant();
    }
    auto tag = JSTaggedValue(raw);
    // skip special value: null undefined
    if (tag.IsSpecial()) {
        return Constant();
    }
    switch (acc.GetMachineType(gate)) {
        case MachineType::I1:
            return Constant((bool)tag.GetInt());
        case MachineType::I32:
            return Constant(tag.GetInt());
        case MachineType::I64:
            if (tag.IsInt()) {
                return Constant(tag.GetInt());
            } else if (tag.IsDouble()) {
                return Constant(tag.GetDouble());
            } else {
                return Constant();
            }
        case MachineType::F64:
            return Constant(tag.GetDouble());
        default:
            return Constant();
    }
}
} // namespace panda::ecmascript::kungfu
