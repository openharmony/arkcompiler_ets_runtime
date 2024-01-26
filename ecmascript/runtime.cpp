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

#include "ecmascript/runtime.h"

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
static Runtime *runtime = nullptr;

Runtime *Runtime::GetInstance()
{
    return runtime;
}

void Runtime::Initialize()
{

}

void Runtime::Destroy()
{
    if (runtime != nullptr) {
        delete runtime;
        runtime = nullptr;
    }
}

// Created once, and should called first.
void Runtime::Create()
{
    runtime = new (std::nothrow) Runtime();
    if (UNLIKELY(runtime == nullptr)) {
        LOG_ECMA(ERROR) << "Failed to create Runtime";
        return;
    }
    runtime->Initialize();
}
}  // namespace panda::ecmascript
