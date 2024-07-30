/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
void JSNativePointer::ResetExternalPointer(JSThread *thread, void *externalPointer)
{
    DeleteExternalPointer(thread);
    SetExternalPointer(externalPointer);
}

void JSNativePointer::Destroy(JSThread *thread)
{
    DeleteExternalPointer(thread);
    SetExternalPointer(nullptr);
    SetDeleter(nullptr);
    SetData(nullptr);
    SetNativeFlag(NativeFlag::NO_DIV);
}

void JSNativePointer::Detach()
{
    // Keep other fields accessible after detached
    SetDeleter(nullptr);
}

void JSNativePointer::DeleteExternalPointer(JSThread *thread)
{
    void *externalPointer = GetExternalPointer();
    NativePointerCallback deleter = GetDeleter();
    auto env = thread->GetEnv();
    if (deleter != nullptr) {
        deleter(env, externalPointer, GetData());
    }
}
}  // namespace panda::ecmascript
