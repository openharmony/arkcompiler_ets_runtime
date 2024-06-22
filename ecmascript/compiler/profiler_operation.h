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

#include "ecmascript/compiler/share_gate_meta_data.h"

namespace panda::ecmascript::kungfu {
enum class OperationType : uint8_t {
    CALL,
    NATIVE_CALL,
    GETTER_SETTER_CALL,
    OPERATION_TYPE,
    DEFINE_CLASS,
    CREATE_OBJECT,
    TRUE_BRANCH,
    FALSE_BRANCH,
    TRY_DUMP,
    TRY_PREDUMP,
    ITERATOR_FUNC_KIND,
    TRY_JIT,
};

using SlotIDFormat = BytecodeInstruction::Format;

#define COMBINE_TYPE_CALL_BACK(curType, type) \
    callback.ProfileCombineOpType(            \
        *(curType), type, [this](GateRef curType, GateRef type)->GateRef{ return Int32Or(curType, type); })

using Callback = std::function<void(const std::initializer_list<GateRef> &, OperationType)>;
class ProfileOperation {
public:
    ProfileOperation() : callback_(nullptr), jitCallback_(nullptr) {}
    explicit ProfileOperation(Callback callback, Callback jitCallback) : callback_(callback),
        jitCallback_(jitCallback) {}

    inline bool IsEmpty() const
    {
        return callback_ == nullptr;
    }

    inline bool IsJitEmpty() const
    {
        return jitCallback_ == nullptr;
    }

    inline void ProfileCall(GateRef func) const
    {
        if (callback_) {
            callback_({ func }, OperationType::CALL);
        }
    }

    inline void ProfileNativeCall(GateRef func) const
    {
        if (callback_) {
            callback_({ func }, OperationType::NATIVE_CALL);
        }
    }

    inline void ProfileGetterSetterCall(GateRef func) const
    {
        if (callback_) {
            callback_({ func }, OperationType::GETTER_SETTER_CALL);
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

    inline void TryDump() const
    {
        if (callback_) {
            callback_({ }, OperationType::TRY_DUMP);
        }
    }

    inline void TryJitCompile() const
    {
        if (callback_) {
            callback_({ }, OperationType::TRY_JIT);
        } else if (jitCallback_) {
            jitCallback_({ }, OperationType::TRY_JIT);
        }
    }

    inline void TryPreDump() const
    {
        if (callback_) {
            callback_({ }, OperationType::TRY_PREDUMP);
        }
    }

    inline void ProfileBranch(bool isTrue) const
    {
        if (callback_) {
            callback_({}, isTrue ? OperationType::TRUE_BRANCH : OperationType::FALSE_BRANCH);
        }
    }

    inline void ProfileGetIterator(GateRef iterator) const
    {
        if (callback_) {
            callback_({ iterator }, OperationType::ITERATOR_FUNC_KIND);
        }
    }

private:
    Callback callback_;
    Callback jitCallback_;
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PROFILER_OPERATION_H
