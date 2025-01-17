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

#include "phase_impl.h"

namespace maple {
// ========== FuncOptimizeImpl ==========
FuncOptimizeImpl::FuncOptimizeImpl(MIRModule &mod, bool currTrace)
    : trace(currTrace), module(&mod)
{
    builder = new (std::nothrow) MIRBuilderExt(module);
    CHECK_FATAL(builder != nullptr, "New fail in FuncOptimizeImpl ctor!");
}

FuncOptimizeImpl::~FuncOptimizeImpl()
{
    if (builder != nullptr) {
        delete builder;
        builder = nullptr;
    }
    currFunc = nullptr;
    module = nullptr;
}
}  // namespace maple
