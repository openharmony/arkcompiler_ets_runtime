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

#include "llvm_interface.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "llvm/Transforms/Scalar.h"
#if defined(PANDA_TARGET_MACOS)
#include "llvm/CodeGen/BuiltinGCs.h"
#else
#include "llvm/IR/BuiltinGCs.h"
#endif

llvm::ModulePass *LLVMCreateRewriteStatepointsForGCLegacyPass()
{
    return llvm::createRewriteStatepointsForGCLegacyPass();
}

void LLVMLinkAllBuiltinGCs(void)
{
    llvm::linkAllBuiltinGCs();
}