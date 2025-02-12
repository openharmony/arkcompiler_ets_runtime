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

#ifndef ECMASCRIPT_COMPILER_AOT_COMPILATION_ENV_H
#define ECMASCRIPT_COMPILER_AOT_COMPILATION_ENV_H

#include "ecmascript/compiler/compilation_env.h"

namespace panda::ecmascript {
class AOTCompilationEnv final : public CompilationEnv {
public:
    PUBLIC_API AOTCompilationEnv(EcmaVM *vm);
    ~AOTCompilationEnv() = default;
    bool IsAotCompiler() const override
    {
        return true;
    }

    JSRuntimeOptions &GetJSOptions() override;

    // thread
    const CMap<ElementsKind, std::pair<ConstantIndex, ConstantIndex>> &GetArrayHClassIndexMap() const override;
    const BuiltinHClassEntries &GetBuiltinHClassEntries() const override;
    JSHClass *GetBuiltinPrototypeHClass(BuiltinTypeId type) const override;

    // context
    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id) const override;
    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, int32_t index) const override;
    JSTaggedValue FindOrCreateUnsharedConstpool(const uint32_t methodOffset) const override;
    JSTaggedValue FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool) const override;
    JSHandle<ConstantPool> FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
        panda_file::File::EntityId id) override;
    JSTaggedValue GetConstantPoolByMethodOffset(const uint32_t methodOffset) const override;

    // ConstantPool
    JSTaggedValue GetArrayLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const override;
    JSTaggedValue GetObjectLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const override;
    JSTaggedValue GetMethodFromCache(JSTaggedValue constpool, uint32_t index) const override;
    panda_file::File::EntityId GetIdFromCache(JSTaggedValue constpool, uint32_t index) const override;

    // GlobalEnv
    JSHandle<GlobalEnv> GetGlobalEnv() const override;

    // GlobalConstants
    const GlobalEnvConstants *GlobalConstants() const override;

    JSTaggedValue GetStringFromConstantPool(const uint32_t methodOffset, const uint16_t cpIdx,
        bool allowAlloc = true) const override;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_COMPILATION_ENV_H
