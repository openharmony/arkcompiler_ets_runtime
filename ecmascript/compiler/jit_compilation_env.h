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

#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/compilation_env.h"
#include "ecmascript/dependent_infos.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/jit/jit_thread.h"

namespace panda::ecmascript {
class JitCompilationEnv final : public CompilationEnv {
public:
    static constexpr uint32_t INVALID_HEAP_CONSTANT_INDEX = std::numeric_limits<uint32_t>::max();
    enum ConstantType : uint8_t {
        IN_SHARED_CONSTANTPOOL,
        IN_UNSHARED_CONSTANTPOOL
    };

    struct ConstantPoolHeapConstant {
        uint32_t constpoolId;
        uint32_t index;
        ConstantType constType;

        bool operator < (const ConstantPoolHeapConstant &heapConstant) const
        {
            if (index != heapConstant.index) {
                return index < heapConstant.index;
            }
            if (constType != heapConstant.constType) {
                return constType < heapConstant.constType;
            }
            return constpoolId < heapConstant.constpoolId;
        }
    };

    JitCompilationEnv(EcmaVM *vm, EcmaVM *hVm,
        JSHandle<JSFunction> &jsFunction, kungfu::LazyDeoptAllDependencies *dependencies);
    ~JitCompilationEnv() = default;
    bool IsJitCompiler() const override
    {
        return true;
    }
    bool SupportHeapConstant() const override
    {
#ifdef USE_CMC_GC
        // Disable heapConstant until we enable barrier for heap constant table
        return false;
#endif
        return hostThread_->GetEcmaVM()->GetJSOptions().IsCompilerEnableLiteCG();
    }

    kungfu::LazyDeoptAllDependencies *GetDependencies() const override
    {
        return dependencies_;
    }

    JSRuntimeOptions &GetJSOptions() const override;
    // thread
    GlobalEnvField GetArrayHClassIndex(ElementsKind kind, bool isProtoType) const override;
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

    void ProcessMethod(MethodLiteral *method, const JSPandaFile *jsPandaFile) const override
    {
        kungfu::BytecodeInfoCollector::ProcessMethodForJIT(method, jsPandaFile);
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

    const std::vector<JSHandle<JSTaggedValue>> &GetHeapConstantTable() const
    {
        return heapConstantInfo_.heapConstantTable;
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
    JSHandle<JSFunction> GetJsFunction() const
    {
        return jsFunction_;
    }

    JSFunction *GetJsFunctionByMethodOffset(uint32_t methodOffset) const;

    uint32_t RecordHeapConstant(const JSHandle<JSTaggedValue> &heapObj)
    {
        heapConstantInfo_.heapConstantTable.push_back(heapObj);
        ASSERT(heapConstantInfo_.heapConstantTable.size() < INVALID_HEAP_CONSTANT_INDEX);
        size_t index = heapConstantInfo_.heapConstantTable.size() - 1;
        return static_cast<uint32_t>(index);
    }

    uint32_t RecordHeapConstant(ConstantPoolHeapConstant heapConstant, const JSHandle<JSTaggedValue> &heapObj);

    JSHandle<JSTaggedValue> NewJSHandle(JSTaggedValue value) const override
    {
        ASSERT(thread_->IsJitThread());
        auto jitThread = static_cast<JitThread*>(thread_);
        return jitThread->NewHandle(value);
    }

    void SetLdExtModuleVarResolved(uint32_t methodOffset, uint32_t pcOffset)
    {
        ldExtModuleVarResolved_[methodOffset][pcOffset] = true;
    }

    bool IsLdExtModuleVarResolved(uint32_t methodOffset, uint32_t bcOffset) const
    {
        auto itMethodOffset = ldExtModuleVarResolved_.find(methodOffset);
        if (itMethodOffset != ldExtModuleVarResolved_.end()) {
            auto &bcOffsetMap = itMethodOffset->second;
            auto itBcOffset = bcOffsetMap.find(bcOffset);
            if (itBcOffset != bcOffsetMap.end()) {
                return itBcOffset->second;
            }
        }
        return false;
    }

    JSHandle<JSTaggedValue> GetHeapConstantHandle(uint32_t heapConstantIndex) const;

    void RecordGate2HeapConstantIndex(uint32_t gate, uint32_t heapConstantIndex)
    {
        heapConstantInfo_.gate2HeapConstantIndex[gate] = heapConstantIndex;
    }

    const std::map<int32_t, uint32_t> &GetGate2HeapConstantIndex() const
    {
        return heapConstantInfo_.gate2HeapConstantIndex;
    }

    void RecordCallMethodId2HeapConstantIndex(uint32_t callMethodId, uint32_t heapConstantIndex)
    {
        heapConstantInfo_.callMethodId2HeapConstantIndex[callMethodId] = heapConstantIndex;
    }

    const std::unordered_map<uint32_t, uint32_t> &GetCallMethodId2HeapConstantIndex() const
    {
        return heapConstantInfo_.callMethodId2HeapConstantIndex;
    }

    void RecordCtorMethodId2HeapConstantIndex(uint32_t callMethodId, uint32_t heapConstantIndex)
    {
        heapConstantInfo_.ctorMethodId2HeapConstantIndex[callMethodId] = heapConstantIndex;
    }

    const std::unordered_map<uint32_t, uint32_t> &GetCtorMethodId2HeapConstantIndex() const
    {
        return heapConstantInfo_.ctorMethodId2HeapConstantIndex;
    }

    void RecordOnlyInlineMethodId2HeapConstantIndex(uint32_t callMethodId, uint32_t heapConstantIndex)
    {
        heapConstantInfo_.onlyInlineMethodId2HeapConstantIndex[callMethodId] = heapConstantIndex;
    }

    const std::unordered_map<uint32_t, uint32_t> &GetOnlyInlineMethodId2HeapConstantIndex() const
    {
        return heapConstantInfo_.onlyInlineMethodId2HeapConstantIndex;
    }

    void RecordLdGlobalByNameBcOffset2HeapConstantIndex(uint32_t methodOffset,
        uint32_t bcOffset, uint32_t heapConstantIndex)
    {
        heapConstantInfo_.ldGlobalByNameBcOffset2HeapConstantIndex[methodOffset][bcOffset] = heapConstantIndex;
    }

    uint32_t GetLdGlobalByNameBcOffset2HeapConstantIndex(uint32_t methodOffset, uint32_t bcOffset) const
    {
        auto itMethodOffset = heapConstantInfo_.ldGlobalByNameBcOffset2HeapConstantIndex.find(methodOffset);
        if (itMethodOffset != heapConstantInfo_.ldGlobalByNameBcOffset2HeapConstantIndex.end()) {
            auto &bcOffsetMap = itMethodOffset->second;
            auto itBcOffset = bcOffsetMap.find(bcOffset);
            if (itBcOffset != bcOffsetMap.end()) {
                return itBcOffset->second;
            }
        }
        return INVALID_HEAP_CONSTANT_INDEX;
    }

    void RecordHolderHClassIndex2HeapConstantIndex(int32_t holderHClassIndex, uint32_t heapConstantIndex)
    {
        heapConstantInfo_.holderHClassIndex2HeapConstantIndex[holderHClassIndex] = heapConstantIndex;
    }

    const std::unordered_map<int32_t, uint32_t> &GetHolderHClassIndex2HeapConstantIndex() const
    {
        return heapConstantInfo_.holderHClassIndex2HeapConstantIndex;
    }
private:
    JSThread *hostThread_ {nullptr};
    JSHandle<JSFunction> jsFunction_;
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};
    JSPandaFile *jsPandaFile_ {nullptr};
    MethodLiteral *methodLiteral_ {nullptr};
    const uint8_t* pcStart_ {nullptr};
    pgo::ApEntityId abcId_ {0};
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, bool>> ldExtModuleVarResolved_;
    std::map<uint32_t, uint32_t> functionSlotIdMap_;
    std::map<uint32_t, uint32_t> callee2CallerMap_;
    struct HeapConstantInfo {
        std::vector<JSHandle<JSTaggedValue>> heapConstantTable;
        std::map<ConstantPoolHeapConstant, uint32_t> constPoolHeapConstant2Index;
        std::map<int32_t, uint32_t> gate2HeapConstantIndex;
        std::unordered_map<uint32_t, uint32_t> callMethodId2HeapConstantIndex;
        std::unordered_map<uint32_t, uint32_t> ctorMethodId2HeapConstantIndex;
        std::unordered_map<uint32_t, uint32_t> onlyInlineMethodId2HeapConstantIndex;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> ldGlobalByNameBcOffset2HeapConstantIndex;
        std::unordered_map<int32_t, uint32_t> holderHClassIndex2HeapConstantIndex;
    } heapConstantInfo_;
    kungfu::LazyDeoptAllDependencies *dependencies_ {nullptr};
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_JIT_COMPILATION_ENV_H
