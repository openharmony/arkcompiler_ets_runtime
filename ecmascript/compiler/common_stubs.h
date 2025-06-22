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

#ifndef ECMASCRIPT_COMPILER_COMMON_STUBS_H
#define ECMASCRIPT_COMPILER_COMMON_STUBS_H

#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {

#define DECLARE_STUB_CLASS(name)                                                   \
    class name##StubBuilder : public StubBuilder {                                 \
    public:                                                                        \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env) \
            : StubBuilder(callSignature, env) {}                                   \
        ~name##StubBuilder() = default;                                            \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                       \
        NO_COPY_SEMANTIC(name##StubBuilder);                                       \
        void GenerateCircuit() override;                                           \
    };
    COMMON_STUB_LIST(DECLARE_STUB_CLASS)
#undef DECLARE_STUB_CLASS

#define DECLARE_STW_COPY_STUB_CLASS(name, base)                                    \
    class name##StubBuilder : public base##StubBuilder {                           \
    public:                                                                        \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env) \
            : base##StubBuilder(callSignature, env) {}                             \
        ~name##StubBuilder() = default;                                            \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                       \
        NO_COPY_SEMANTIC(name##StubBuilder);                                       \
    };

#define DECLARE_STW_COPY_STUB_CLASS_SECOND(base)                                   \
    DECLARE_STW_COPY_STUB_CLASS(base##StwCopy, base)

    COMMON_STW_COPY_STUB_LIST(DECLARE_STW_COPY_STUB_CLASS_SECOND)
#undef DECLARE_STW_COPY_STUB_CLASS_SECOND
#undef DECLARE_STW_COPY_STUB_CLASS

}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMMON_STUBS_H
