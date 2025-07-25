/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_BASE_GC_HELPER_H
#define ECMASCRIPT_BASE_GC_HELPER_H

#include "ecmascript/mem/barriers.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::base {
class GCHelper {
public:
    static inline void CopyCallTarget(const JSThread *thread, void *callTargetToRef)
    {
        if (!JSTaggedValue(static_cast<JSTaggedType>(ToUintPtr(callTargetToRef))).IsHeapObject()) {
            return;
        }
        Barriers::UpdateSlot(thread, callTargetToRef, TaggedObject::HCLASS_OFFSET);
        if (JSTaggedValue(reinterpret_cast<TaggedObject*>(callTargetToRef)).IsJSProxy()) {
            // JSProxy
            TaggedObject *methodToRef =
                reinterpret_cast<TaggedObject *>(Barriers::UpdateSlot(thread, callTargetToRef, JSProxy::METHOD_OFFSET));
            Barriers::UpdateSlot(thread, methodToRef, Method::CONSTANT_POOL_OFFSET);
        } else if (JSTaggedValue(reinterpret_cast<TaggedObject*>(callTargetToRef)).IsJSFunctionBase()) {
            // JSFunctionBase
            TaggedObject *methodToRef = reinterpret_cast<TaggedObject *>(
                Barriers::UpdateSlot(thread, callTargetToRef, JSFunctionBase::METHOD_OFFSET));
            Barriers::UpdateSlot(thread, methodToRef, Method::CONSTANT_POOL_OFFSET);
            // JSFunction : FunctionBase
            if (JSTaggedValue(reinterpret_cast<TaggedObject*>(callTargetToRef)).IsJSFunction()) {
                TaggedObject *profileTypeInfoToRef = reinterpret_cast<TaggedObject *>(
                    Barriers::UpdateSlot(thread, callTargetToRef, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
                Barriers::UpdateSlot(thread, profileTypeInfoToRef, ProfileTypeInfoCell::VALUE_OFFSET);
                Barriers::UpdateSlot(thread, callTargetToRef, JSFunction::LEXICAL_ENV_OFFSET);
            }
            // BoundFunction : FunctionBase
            if (JSTaggedValue(reinterpret_cast<TaggedObject*>(callTargetToRef)).IsBoundFunction()) {
                auto toTaggedArray = reinterpret_cast<TaggedObject *>(
                    Barriers::UpdateSlot(thread, callTargetToRef, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
                auto boundTargetToRef = reinterpret_cast<TaggedObject *>(
                    Barriers::UpdateSlot(thread, callTargetToRef, JSBoundFunction::BOUND_TARGET_OFFSET));
                Barriers::UpdateSlot(thread, callTargetToRef, JSBoundFunction::BOUND_THIS_OFFSET);
                CopyCallTarget(thread, boundTargetToRef);

                uintptr_t argvAddr = ToUintPtr(toTaggedArray) + TaggedArray::DATA_OFFSET;
                void *argvToRef = reinterpret_cast<void *>(argvAddr);
                uint64_t argc =
                    static_cast<uint64_t>(Barriers::GetPrimitive<uint32_t>(toTaggedArray, TaggedArray::LENGTH_OFFSET));
                CopyArgvArray(thread, argvToRef, argc);
            }
        } else {
            ASSERT(!JSTaggedValue(reinterpret_cast<TaggedObject*>(callTargetToRef)).IsCallable());
        }
    }

    static inline void CopyArgvArray(const JSThread *thread, void *argvToRef, uint64_t argc)
    {
        for (uint64_t i = 0; i < argc; i++) {
            size_t offset = i * sizeof(JSTaggedType);
            Barriers::UpdateSlot(thread, argvToRef, offset);
        }
    }

    static inline void CopyGeneratorContext(const JSThread *thread, void *contextToRef)
    {
        Barriers::UpdateSlot(thread, contextToRef, GeneratorContext::GENERATOR_METHOD_OFFSET);
        Barriers::UpdateSlot(thread, contextToRef, GeneratorContext::GENERATOR_THIS_OFFSET);
        Barriers::UpdateSlot(thread, contextToRef, GeneratorContext::GENERATOR_LEXICALENV_OFFSET);
        Barriers::UpdateSlot(thread, contextToRef, GeneratorContext::GENERATOR_ACC_OFFSET);
        TaggedObject *toTaggedArray = reinterpret_cast<TaggedObject *>(
            Barriers::UpdateSlot(thread, contextToRef, GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET));
        uintptr_t argvAddr = ToUintPtr(toTaggedArray) + TaggedArray::DATA_OFFSET;
        void *argvToRef = reinterpret_cast<void *>(argvAddr);
        uint64_t argc = static_cast<uint64_t>(
            Barriers::GetPrimitive<uint32_t>(contextToRef, GeneratorContext::GENERATOR_NREGS_OFFSET));
        CopyArgvArray(thread, argvToRef, argc);
    }
};
}
#endif // ECMASCRIPT_BASE_GC_HELPER_H
