/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SUSTAINING_JS_HANDLE_H
#define ECMASCRIPT_SUSTAINING_JS_HANDLE_H

#include "ecmascript/common.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {

class SustainingJSHandle {
public:
    SustainingJSHandle(EcmaVM *vm);
    ~SustainingJSHandle();
    template <typename T>
    JSHandle<T> NewHandle(JSHandle<T> value)
    {
        return JSHandle<T>(GetJsHandleSlot(value.GetTaggedType()));
    }

    template <typename T>
    JSHandle<T> NewHandle(JSTaggedType value)
    {
        return JSHandle<T>(GetJsHandleSlot(value));
    }

    template <typename T>
    static JSHandle<T> NewHandle(const JitThread *thread, JSTaggedType value)
    {
        return JSHandle<T>(GetJsHandleSlot(thread, value));
    }

    void Iterate(RootVisitor &v);

private:
    uintptr_t GetJsHandleSlot(JSTaggedType value);
    static uintptr_t GetJsHandleSlot(const JitThread *thread, JSTaggedType value);
    uintptr_t Expand();

    EcmaVM *vm_ { nullptr };

    JSTaggedType *blockNext_ { nullptr };
    JSTaggedType *blockLimit_ { nullptr };
    SustainingJSHandle *pre_ { nullptr };
    SustainingJSHandle *next_ { nullptr };

    static constexpr uint32_t BLOCK_SIZE = 256L;
    std::vector<std::array<JSTaggedType, BLOCK_SIZE>*> handleBlocks_;
    friend class SustainingJSHandleList;
};

class SustainingJSHandleList {
public:
    void AddSustainingJSHandle(SustainingJSHandle *sustainingJSHandle);
    void RemoveSustainingJSHandle(SustainingJSHandle *sustainingJSHandle);
    void Iterate(RootVisitor &v);

private:
    SustainingJSHandle *listHead_ { nullptr };
    Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SUSTAINING_JS_HANDLE_H
