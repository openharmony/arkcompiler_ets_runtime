/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef CRT_MOCK_H
#define CRT_MOCK_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <atomic>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
    class TaggedObject;
    class ObjectSlot;
    class EcmaVM;
}

namespace panda {
    using CommonRootVisitor = void (*)(void *root);

    using panda::ecmascript::TaggedObject;
    using WorkStackAddHookType = void (*)(void *workStack, TaggedObject *obj);
    using AllocBufferAddHookType = void (*)(uint64_t *obj);
    using EnumThreadStackRootHookType = void (*)(void *vm, AllocBufferAddHookType hook);
    using EnumStaticRootsHookType = void (*)(void *workStack, WorkStackAddHookType hook);
    using FillFreeObjectHookType = void (*)(void *object, size_t size);
    using JSGCCallbackHookType = void (*)(void *ecmaVM);
    using ForwardUpdateRawRefHookType = void (*)(void *collector, TaggedObject *obj);
    using ForwardUpdateWeakRawRefHookType = bool (*)(void *collector, TaggedObject *obj);
    using PreforwardStaticRootsHookType =
        void (*)(void *collector, ForwardUpdateRawRefHookType hook, ForwardUpdateWeakRawRefHookType weakHook);
    using SetBaseAddrHookType = void (*)(uintptr_t base);

    extern "C" void ArkCommonRuntimeInit();
    extern "C" void ArkCommonRuntimeFini();
    // enum thread stack root
    extern "C" void ArkRegisterEnumThreadStackRootHook(EnumThreadStackRootHookType);
    // enum static root
    extern "C" void ArkRegisterEnumStaticRootsHook(EnumStaticRootsHookType);
    extern "C" void ArkRegisterFillFreeObjectHook(FillFreeObjectHookType);
    extern "C" void ArkRegisterJSGCCallbackHook(JSGCCallbackHookType);
    extern "C" void ArkRegisterSetBaseAddrHook(SetBaseAddrHookType);
    // forward static root
    extern "C" void RegisterPreforwardStaticRootsHook(PreforwardStaticRootsHookType);

    extern "C" void VisitJSThread(void *jsThread, CommonRootVisitor visitor);
    extern "C" void VisitCoroutine(void *coroutine, CommonRootVisitor visitor);

    class CommonRuntime {
    public:
        CommonRuntime() = default;
        ~CommonRuntime() = default;

        static CommonRuntime *GetInstance() { return instance_; }

        static void Create();

        static void Destroy();

        void Init();
        void Fini();

    private:
        static CommonRuntime *instance_;
    };

} // namespace panda

#endif // CRT_MOCK_H
