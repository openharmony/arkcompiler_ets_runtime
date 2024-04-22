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
#include "ecmascript/compiler/compilation_env.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"

namespace panda::ecmascript {
CompilationEnv::CompilationEnv(EcmaVM *vm) : vm_(vm), thread_(vm_->GetJSThread()),
    ptManager_(nullptr) { }

NativeAreaAllocator *CompilationEnv::GetNativeAreaAllocator() const
{
    return vm_->GetNativeAreaAllocator();
}

std::shared_ptr<pgo::PGOProfiler> CompilationEnv::GetPGOProfiler() const
{
    return vm_->GetPGOProfiler();
}
} // namespace panda::ecmascript
