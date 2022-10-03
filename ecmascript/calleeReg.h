/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_CALLEE_REG_H
#define ECMASCRIPT_COMPILER_CALLEE_REG_H
#include <map>
#include "ecmascript/common.h"
#include "ecmascript/llvm_stackmap_type.h"
namespace panda::ecmascript::kungfu {
#if defined(PANDA_TARGET_AMD64)
static const int MAX_CALLEE_SAVE_REIGISTER_NUM = 32;
enum class DwarfReg: DwarfRegType
{
    RBX = 3,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
};
#else
// todo for arm64
static const int MAX_CALLEE_SAVE_REIGISTER_NUM = 32;
enum class DwarfReg: DwarfRegType
{
};

#endif
class CalleeReg
{
public:
    PUBLIC_API CalleeReg();
    virtual PUBLIC_API ~CalleeReg() = default;
    int PUBLIC_API FindCallRegOrder(const DwarfRegType reg) const;
    int PUBLIC_API FindCallRegOrder(const DwarfReg reg) const;
    int PUBLIC_API GetCallRegNum() const;
private:
    std::map<DwarfReg, int> reg2Location_;

};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_CALLEE_REG_H
