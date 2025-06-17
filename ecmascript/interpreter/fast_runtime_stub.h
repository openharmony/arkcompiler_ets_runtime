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

#ifndef ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_H
#define ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_H

#include <memory>

#include "ecmascript/frames.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_fast_operator.h"

namespace panda::ecmascript {
class GlobalEnv;
class PropertyAttributes;

class FastRuntimeStub {
public:
    static inline JSTaggedValue FastMul(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastDiv(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastMod(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastEqual(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastTypeOf(JSThread *thread, JSTaggedValue obj);
    static inline JSTaggedValue FastStrictEqual(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue NewLexicalEnv(JSThread *thread, ObjectFactory *factory, uint16_t numVars);
    static inline JSTaggedValue GetGlobalOwnProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<ObjectFastOperator::Status status = ObjectFastOperator::Status::None>
    static inline JSTaggedValue GetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<ObjectFastOperator::Status status = ObjectFastOperator::Status::None>
    static inline JSTaggedValue GetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<ObjectFastOperator::Status status = ObjectFastOperator::Status::None>
    static inline JSTaggedValue GetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index);
    template<ObjectFastOperator::Status status = ObjectFastOperator::Status::None>
    static inline JSTaggedValue SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue value);
    template<ObjectFastOperator::Status status = ObjectFastOperator::Status::None>
    static inline JSTaggedValue SetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                   JSTaggedValue value);
    template<ObjectFastOperator::Status status = ObjectFastOperator::Status::None>
    static inline JSTaggedValue SetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                   JSTaggedValue value);

    static inline JSTaggedValue NewThisObject(JSThread *thread, JSTaggedValue ctor, JSTaggedValue newTarget,
                                              InterpretedFrame* state);
    static inline JSTaggedValue CallGetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                           JSTaggedValue value);

private:
    friend class ICRuntimeStub;

    static inline JSTaggedValue CallSetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue value,
                                           JSTaggedValue accessorValue);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INTERPRETER_OBJECT_OPERATOR_INL_H
