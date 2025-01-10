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

#include "jit_signcode.h"

namespace panda::ecmascript::kungfu {

JitSignCode::JitSignCode() {}

void JitSignCode::SetCodeSigner(JitCodeSigner *p)
{
    codeSigner_ = p;
}

JitCodeSigner *JitSignCode::GetCodeSigner()
{
    return codeSigner_;
}

void JitSignCode::SetKind(int kind)
{
    kind_ = kind;
}

int JitSignCode::GetKind()
{
    return kind_;
}

void JitSignCode::Reset()
{
    codeSigner_ = nullptr;
    kind_ = 0;
    signTableSize_ = 0;
}

} // namespace panda::ecmascript::kungfu
