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

#ifndef ECMASCRIPT_COMPILER_JIT_COMPILATION_ENV_H
#define ECMASCRIPT_COMPILER_JIT_COMPILATION_ENV_H

#include "ecmascript/compiler/compilation_env.h"
#include "ecmascript/ic/profile_type_info.h"

namespace panda::ecmascript {
class JitCompilationEnv final : public CompilationEnv {
public:
    JitCompilationEnv(EcmaVM *vm, EcmaVM *hVm, JSHandle<JSFunction> &jsFunction);
    ~JitCompilationEnv() = default;
    bool IsJitCompiler() const override
    {
        return true;
    }
    JSRuntimeOptions &GetJSOptions() override;
    // thread
    ConstantIndex GetArrayHClassIndex(ElementsKind kind, bool isProtoType) const override;
    const BuiltinHClassEntries &GetBuiltinHClassEntries() const override;
    JSHClass *GetBuiltinPrototypeHClass(BuiltinTypeId type) const override;
    void SetTsManagerCompilationEnv();

    std::shared_ptr<pgo::PGOProfiler> GetPGOProfiler() const override;

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

    JSThread *GetHostThread() const override
    {
        return hostThread_;
    }

    JSPandaFile *GetJSPandaFile() const override
    {
        return jsPandaFile_;
    }

    MethodLiteral *GetMethodLiteral() const override
    {
        return methodLiteral_;
    }
    
    const uint8_t *GetMethodPcStart() const override
    {
        return pcStart_;
    }

    pgo::ApEntityId GetMethodAbcId() const override
    {
        return abcId_;
    }

    void SetProfileTypeInfo(const JSHandle<ProfileTypeInfo> &info)
    {
        profileTypeInfo_ = info;
    }

    void UpdateFuncSlotIdMap(uint32_t calleeOffset, uint32_t callerOffset, uint32_t slotId)
    {
        if (functionSlotIdMap_.find(calleeOffset) != functionSlotIdMap_.end()) {
            return;
        }
        if (callee2CallerMap_.find(calleeOffset) != callee2CallerMap_.end()) {
            return;
        }
        functionSlotIdMap_[calleeOffset] = slotId;
        callee2CallerMap_[calleeOffset] = callerOffset;
    }

    JSFunction *GetJsFunctionByMethodOffset(uint32_t methodOffset) const;
private:
    JSThread *hostThread_ {nullptr};
    JSHandle<JSFunction> jsFunction_;
    JSPandaFile *jsPandaFile_ {nullptr};
    MethodLiteral *methodLiteral_ {nullptr};
    const uint8_t* pcStart_ {nullptr};
    pgo::ApEntityId abcId_ {0};
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    std::map<uint32_t, uint32_t> functionSlotIdMap_;
    std::map<uint32_t, uint32_t> callee2CallerMap_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_JIT_COMPILATION_ENV_H
