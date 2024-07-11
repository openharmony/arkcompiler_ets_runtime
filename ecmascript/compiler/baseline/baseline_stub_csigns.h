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

#ifndef ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUB_CSIGNS_H
#define ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUB_CSIGNS_H

#include "ecmascript/compiler/baseline/baseline_compiler_builtins.h"
#include "ecmascript/compiler/call_signature.h"

namespace panda::ecmascript::kungfu {

#define BASELINE_STUB_ID_LIST(V)      \
    BASELINE_COMPILER_BUILTIN_LIST(V)

class BaselineStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name) name,
        BASELINE_STUB_ID_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        NUM_OF_STUBS
    };

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& callSigns);

    static const CallSignature *Get(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        return &callSigns_[index];
    }

    static const std::string &GetName(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        return callSigns_[index].GetName();
    }

private:
    static CallSignature callSigns_[NUM_OF_STUBS];
};

} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUB_CSIGNS_H
