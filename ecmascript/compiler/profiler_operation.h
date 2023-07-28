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
#include <initializer_list>

#include "ecmascript/compiler/gate_meta_data.h"

namespace panda::ecmascript::kungfu {
enum class OperationType : uint8_t {
    CALL,
    OPERATION_TYPE,
    DEFINE_CLASS,
    CREATE_OBJECT,
    STORE_LAYOUT,
    LOAD_LAYOUT,
    INDEX,
    TRUE_BRANCH,
    FALSE_BRANCH,
};

#define COMBINE_TYPE_CALL_BACK(curType, type) \
    callback.ProfileCombineOpType(            \
        *(curType), type, [this](GateRef curType, GateRef type)->GateRef{ return Int32Or(curType, type); })

using Callback = std::function<void(const std::initializer_list<GateRef> &, OperationType)>;
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
            callback_({ func }, OperationType::CALL);
        }
    }

    inline void ProfileOpType(GateRef type) const
    {
        if (callback_) {
            callback_({ type }, OperationType::OPERATION_TYPE);
        }
    }

    template <typename TypeCombine>
    inline void ProfileCombineOpType(GateRef curType, GateRef type, TypeCombine combine) const
    {
        if (callback_) {
            GateRef ret = combine(curType, type);
            callback_({ ret }, OperationType::OPERATION_TYPE);
        }
    }

    inline void ProfileDefineClass(GateRef constructor) const
    {
        if (callback_) {
            callback_({ constructor }, OperationType::DEFINE_CLASS);
        }
    }

    inline void ProfileCreateObject(GateRef newObj) const
    {
        if (callback_) {
            callback_({ newObj }, OperationType::CREATE_OBJECT);
        }
    }

    inline void ProfileObjLayoutByStore(GateRef object) const
    {
        if (callback_) {
            callback_({ object }, OperationType::STORE_LAYOUT);
        }
    }

    inline void ProfileObjLayoutByLoad(GateRef object) const
    {
        if (callback_) {
            callback_({ object }, OperationType::LOAD_LAYOUT);
        }
    }

    inline void ProfileObjIndex(GateRef object) const
    {
        if (callback_) {
            callback_({ object }, OperationType::INDEX);
        }
    }

    inline void ProfileBranch(bool isTrue) const
    {
        if (callback_) {
            callback_({}, isTrue ? OperationType::TRUE_BRANCH : OperationType::FALSE_BRANCH);
        }
    }

private:
    Callback callback_;
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PROFILER_OPERATION_H
