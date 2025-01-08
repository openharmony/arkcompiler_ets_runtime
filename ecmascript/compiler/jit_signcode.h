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

#ifndef CODE_SIGN_JIT_SIGN_CODE_H
#define CODE_SIGN_JIT_SIGN_CODE_H

#include "jit_code_signer.h"

namespace panda::ecmascript::kungfu {
using namespace OHOS::Security::CodeSign;


class JitSignCode {
public:
    ~JitSignCode() = default;

    static JitSignCode *GetInstance()
    {
        static JitSignCode instance;
        return &instance;
    }

    void SetCodeSigner(JitCodeSigner *p);
    JitCodeSigner *GetCodeSigner();
    void SetKind(int kind);
    int GetKind();
    void Reset();
    int signTableSize_ = -1;
private:
    JitSignCode();
    int kind_ = -1;
    JitCodeSigner *codeSigner_ {nullptr};
};

} // namespace panda::ecmascript::kungfu
#endif
