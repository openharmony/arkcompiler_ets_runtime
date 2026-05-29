/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_PGO_CONTEXT_H
#define ECMASCRIPT_ARKSTEED_PGO_CONTEXT_H

#include "ecmascript/arksteed/arksteed_access_info_factory.h"

namespace panda::ecmascript::arksteed {

class ArkSteedPGOContext {
public:
    ArkSteedPGOContext(JSThread *compilerThread, JitCompilationEnv *env)
        : compilerThread_(compilerThread), env_(env), broker_(compilerThread, env)
    {}

    ArkSteedAccessInfoFactory CreateAccessInfoFactory(
        const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo) const
    {
        return ArkSteedAccessInfoFactory(compilerThread_, bytecodeInfo, env_,
                                         const_cast<ArkSteedHeapBroker *>(&broker_));
    }

    ArkSteedHeapBroker::SerializingScope CreateSerializingScope(CString message) const
    {
        return ArkSteedHeapBroker::SerializingScope(&broker_, message);
    }

    ArkSteedHeapBroker *GetBroker()
    {
        return &broker_;
    }

    const ArkSteedHeapBroker *GetBroker() const
    {
        return &broker_;
    }

private:
    JSThread *compilerThread_ {nullptr};
    JitCompilationEnv *env_ {nullptr};
    mutable ArkSteedHeapBroker broker_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PGO_CONTEXT_H
