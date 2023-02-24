/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_ASYNC_GENERATOR_OBJECT_H
#define ECMASCRIPT_JS_ASYNC_GENERATOR_OBJECT_H

#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/record.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"

namespace panda {
namespace ecmascript {
enum class JSAsyncGeneratorState : uint8_t {
    UNDEFINED = 0,
    SUSPENDED_START,
    SUSPENDED_YIELD,
    EXECUTING,
    COMPLETED,
    AWAITING_RETURN,
};

enum class AsyncGeneratorResumeMode : uint8_t {
    RETURN = 0,
    THROW,
    NEXT,
    UNDEFINED
};

class AsyncGeneratorRequest final : public Record {
public:
    CAST_CHECK(AsyncGeneratorRequest, IsAsyncGeneratorRequest);

    static constexpr size_t COMPLETION_OFFSET = Record::SIZE;
    ACCESSORS(Completion, COMPLETION_OFFSET, CAPABILITY_OFFSET);
    ACCESSORS(Capability, CAPABILITY_OFFSET, SIZE);

    DECL_DUMP()

    DECL_VISIT_OBJECT(COMPLETION_OFFSET, SIZE)
};

class JSAsyncGeneratorObject : public JSObject {
public:
    CAST_CHECK(JSAsyncGeneratorObject, IsAsyncGeneratorObject);

    static constexpr size_t GENERATOR_CONTEXT_OFFSET = JSObject::SIZE;
    ACCESSORS(GeneratorContext, GENERATOR_CONTEXT_OFFSET, ASYNC_GENERATOR_QUEUE_OFFSET)
    ACCESSORS(AsyncGeneratorQueue, ASYNC_GENERATOR_QUEUE_OFFSET, GENERATOR_OFFSET)
    ACCESSORS(GeneratorBrand, GENERATOR_OFFSET, GENERATOR_RESUME_RESULT_OFFSET)
    ACCESSORS(ResumeResult, GENERATOR_RESUME_RESULT_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define Bitfield
    static constexpr size_t ASYNC_GENERATOE_STATE_BITS = 4;
    static constexpr size_t RESUME_MODE_BITS = 3;
    FIRST_BIT_FIELD(BitField, AsyncGeneratorState, JSAsyncGeneratorState, ASYNC_GENERATOE_STATE_BITS)
    NEXT_BIT_FIELD(BitField, ResumeMode, AsyncGeneratorResumeMode, RESUME_MODE_BITS, AsyncGeneratorState)
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, GENERATOR_CONTEXT_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
	
    // AsyncGeneratorValidate ( generator, generatorBrand )
    static void AsyncGeneratorValidate(JSThread *thread, const JSHandle<JSTaggedValue> &gen, const JSTaggedValue &val);

    // AsyncGeneratorResolve ( generator, value, done )
    static JSTaggedValue AsyncGeneratorResolve(JSThread *thread, const JSHandle<JSAsyncGeneratorObject> &generator,
                                               const JSHandle<JSTaggedValue> value, bool done);

    static JSTaggedValue AsyncGeneratorReject(JSThread *thread, const JSHandle<JSAsyncGeneratorObject> &generator,
                                              const JSHandle<JSTaggedValue> value);

    // AsyncGeneratorResumeNext ( generator )
    static JSTaggedValue AsyncGeneratorResumeNext(JSThread *thread, const JSHandle<JSAsyncGeneratorObject> &generator);

    // 27.6.3.7 AsyncGeneratorEnqueue ( generator, completion, generatorBrand )
    static JSTaggedValue AsyncGeneratorEnqueue(JSThread *thread, const JSHandle<JSTaggedValue> &generator,
                                               const JSHandle<CompletionRecord> completionRecord);

    static JSTaggedValue PromiseResolve(JSThread *thread, const JSHandle<JSTaggedValue> promise,
                                        const JSHandle<JSTaggedValue> value);

    static JSTaggedValue ProcessorFulfilledFunc(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ProcessorRejectedFunc(EcmaRuntimeCallInfo *argv);

    inline bool IsSuspendYield() const
    {
        return GetAsyncGeneratorState() == JSAsyncGeneratorState::SUSPENDED_YIELD;
    }

    inline bool IsExecuting() const
    {
        return GetAsyncGeneratorState() == JSAsyncGeneratorState::EXECUTING;
    }
};

class JSAsyncGeneratorResNextRetProRstFtn : public JSFunction {
public:
    CAST_CHECK(JSAsyncGeneratorResNextRetProRstFtn, IsJSAsyncGeneratorResNextRetProRstFtn);
    static constexpr size_t ASYNC_GENERATOR_OBJECT_OFFSET = JSFunction::SIZE;
    ACCESSORS(AsyncGeneratorObject, ASYNC_GENERATOR_OBJECT_OFFSET, SIZE);
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSFunction, ASYNC_GENERATOR_OBJECT_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_JS_ASYNC_GENERATOR_OBJECT_H
