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

#ifndef ECMASCRIPT_COMPILER_PROFILER_OPERATION_H
#define ECMASCRIPT_COMPILER_PROFILER_OPERATION_H

#include <functional>

#include "ecmascript/compiler/gate_meta_data.h"

namespace panda::ecmascript::kungfu {
enum class OperationType : uint8_t {
    CALL,
    OPERATION_TYPE,
    DEFINE_CLASS,
    STORE_LAYOUT,
    LOAD_LAYOUT,
};

using Callback = std::function<void(GateRef, OperationType)>;
class ProfileOperation {
public:
    ProfileOperation() : callback_(nullptr) {}
    explicit ProfileOperation(Callback callback) : callback_(callback) {}

    inline bool IsEmpty() const
    {
        return callback_ == nullptr;
    }

    inline void ProfileCall(GateRef func) const
    {
        if (callback_) {
            callback_(func, OperationType::CALL);
        }
    }

    inline void ProfileOpType(GateRef type) const
    {
        if (callback_) {
            callback_(type, OperationType::OPERATION_TYPE);
        }
    }

    inline void ProfileDefineClass(GateRef constructor) const
    {
        if (callback_) {
            callback_(constructor, OperationType::DEFINE_CLASS);
        }
    }

    inline void ProfileObjLayoutByStore(GateRef object) const
    {
        if (callback_) {
            callback_(object, OperationType::STORE_LAYOUT);
        }
    }

    inline void ProfileObjLayoutByLoad(GateRef object) const
    {
        if (callback_) {
            callback_(object, OperationType::LOAD_LAYOUT);
        }
    }

private:
    Callback callback_;
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PROFILER_OPERATION_H
