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
#ifndef ECMASCRIPT_CROSS_VM_JS_NAPI_HYBRID_H
#define ECMASCRIPT_CROSS_VM_JS_NAPI_HYBRID_H

#include <cstdint>
#include "ecmascript/base/aligned_struct.h"

#define GLOBAL_PUBLIC_DEF_HYBRID_EXTENSION()                                                        \
    template<typename T>                                                                            \
    template<typename S>                                                                            \
    void Global<T>::CreateXRefGloablReference(const EcmaVM *vm, const Local<S> &current)            \
    {                                                                                               \
        vm_ = vm;                                                                                   \
        if (!current.IsEmpty()) {                                                                   \
            address_ = JSNApi::GetXRefGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current)); \
        }                                                                                           \
    }                                                                                               \
    template<typename T>                                                                            \
    void Global<T>::FreeXRefGlobalHandleAddr()                                                      \
    {                                                                                               \
        if (address_ == 0) {                                                                        \
            return;                                                                                 \
        }                                                                                           \
        JSNApi::DisposeXRefGlobalHandleAddr(vm_, address_);                                         \
        address_ = 0;                                                                               \
    }

#define GLOBAL_PUBLIC_DEF_HYBRID_MODE_EXTENSION()                                              \
    template<typename T>                                                                       \
    void Global<T>::MarkFromObject(std::function<void(uintptr_t)> &visitor)                    \
    {                                                                                          \
        if (address_ == 0) {                                                                   \
            return;                                                                            \
        }                                                                                      \
        JSNApi::MarkFromObject(vm_, address_, visitor);                                        \
    }                                                                                          \
    template<typename T>                                                                       \
    void Global<T>::MarkFromObject()                                                           \
    {                                                                                          \
        if (address_ == 0) {                                                                   \
            return;                                                                            \
        }                                                                                      \
        JSNApi::MarkFromObject(vm_, address_);                                                 \
    }                                                                                          \
    template<typename T>                                                                       \
    bool Global<T>::IsObjectAlive() const                                                      \
    {                                                                                          \
        if (address_ == 0) {                                                                   \
            return false ;                                                                     \
        }                                                                                      \
        return JSNApi::IsObjectAlive(vm_, address_);                                           \
    }                                                                                          \
    template<typename T>                                                                       \
    bool Global<T>::IsValidHeapObject() const                                                  \
    {                                                                                          \
        if (address_ == 0) {                                                                   \
            return false;                                                                      \
        }                                                                                      \
        return JSNApi::IsValidHeapObject(vm_, address_);                                       \
    }

namespace panda {
namespace ecmascript {
    class EcmaVM;
}
using EcmaVM = ecmascript::EcmaVM;

#ifdef PANDA_JS_ETS_HYBRID_MODE
class PUBLIC_API HandshakeHelper final {
    public:
        static void DoHandshake(EcmaVM *vm, void *stsiface, void **ecmaiface);
};
#endif  // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda
#endif  // ECMASCRIPT_CROSS_VM_JS_NAPI_HYBRID_H
