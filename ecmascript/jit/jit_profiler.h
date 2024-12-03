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
#ifndef ECMASCRIPT_JIT_PROFILER_H
#define ECMASCRIPT_JIT_PROFILER_H
#include <chrono>
#include <memory>
#include "ecmascript/common.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/compilation_env.h"
#include "ecmascript/elements.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/patch/patch_loader.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/pgo_profiler/types/pgo_type_generator.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/taskpool/task.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
namespace panda::ecmascript {
using namespace pgo;
class ProfileTypeInfo;
class JSFunction;
class PGOProfilerManager;
class JITProfiler {
public:
    NO_COPY_SEMANTIC(JITProfiler);
    NO_MOVE_SEMANTIC(JITProfiler);

    JITProfiler(EcmaVM *vm);

    virtual ~JITProfiler();
    void PUBLIC_API ProfileBytecode(JSThread *thread, const JSHandle<ProfileTypeInfo> &profileTypeInfo,
                                    ProfileTypeInfo *rawProfileTypeInfo,
                                    EntityId methodId, ApEntityId abcId, const uint8_t *pcStart,
                                    uint32_t codeSize, const panda_file::File::Header *header,
                                    bool useRawProfileTypeInfo = false);

    std::unordered_map<int32_t, const PGOSampleType *> GetOpTypeMap()
    {
        return bcOffsetPGOOpTypeMap_;
    }
    std::unordered_map<int32_t, const PGORWOpType *> GetRwTypeMap()
    {
        return bcOffsetPGORwTypeMap_;
    }
    std::unordered_map<int32_t, const PGODefineOpType *> GetDefOpTypeMap()
    {
        return bcOffsetPGODefOpTypeMap_;
    }
    void InitJITProfiler()
    {
        ptManager_ = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
    }
    void SetCompilationEnv(CompilationEnv *env)
    {
        compilationEnv_ = env;
    }
    void InitChunk(Chunk* chunk)
    {
        chunk_ = chunk;
    }
private:
    enum class BCType : uint8_t {
        STORE,
        LOAD,
    };

    // SampleType
    void ConvertOpType(uint32_t slotId, long bcOffset);
    void ConvertCall(uint32_t slotId, long bcOffset);
    void ConvertNewObjRange(uint32_t slotId, long bcOffset);
    void ConvertGetIterator(uint32_t slotId, long bcOffset);

    // DefineType
    void ConvertCreateObject(uint32_t slotId, long bcOffset, int32_t traceId);

    // RwOpType
    void ConvertICByName(int32_t bcOffset, uint32_t slotId,  BCType type);
    void ConvertICByNameWithHandler(ApEntityId abcId, int32_t bcOffset, JSHClass *hclass,
		                    JSTaggedValue secondValue, BCType type, uint32_t slotId);
    void HandleLoadType(ApEntityId &abcId, int32_t &bcOffset,
                        JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId);
    void HandleLoadTypeInt(ApEntityId &abcId, int32_t &bcOffset,
                           JSHClass *hclass, JSTaggedValue &secondValue);
    void HandleLoadTypePrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                        JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId);
    void HandleOtherTypes(ApEntityId &abcId, int32_t &bcOffset,
                          JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId);
    void HandleTransitionHandler(ApEntityId &abcId, int32_t &bcOffset,
                                 JSHClass *hclass, JSTaggedValue &secondValue);
    void HandleTransWithProtoHandler(ApEntityId &abcId, int32_t &bcOffset,
                                     JSHClass *hclass, JSTaggedValue &secondValue);
    void HandleOtherTypesPrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                          JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId);
    void HandleStoreAOTHandler(ApEntityId &abcId, int32_t &bcOffset,
                              JSHClass *hclass, JSTaggedValue &secondValue);
    void ConvertICByNameWithPoly(ApEntityId abcId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type,
                                 uint32_t slotId);
    void ConvertICByValue(int32_t bcOffset, uint32_t slotId, BCType type);
    void ConvertICByValueWithHandler(ApEntityId abcId, int32_t bcOffset, JSHClass *hclass,
		                     JSTaggedValue secondValue, BCType type);
    void HandleStoreType(ApEntityId &abcId, int32_t &bcOffset,
                         JSHClass *hclass, JSTaggedValue &secondValue);
    void HandleTransition(ApEntityId &abcId, int32_t &bcOffset,
                          JSHClass *hclass, JSTaggedValue &secondValue);
    void HandleTransWithProto(ApEntityId &abcId, int32_t &bcOffset,
                              JSHClass *hclass, JSTaggedValue &secondValue);
    void HandlePrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                JSHClass *hclass, JSTaggedValue &secondValue);
    void ConvertICByValueWithPoly(ApEntityId abcId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type);
    void ConvertInstanceof(int32_t bcOffset, uint32_t slotId);

    // RwOpType related
    void AddObjectInfoWithMega(int32_t bcOffset);
    void AddObjectInfoImplement(int32_t bcOffset, const PGOObjectInfo &info);
    bool AddTranstionObjectInfo(int32_t bcOffset, JSHClass *receiver,
		                JSHClass *hold, JSHClass *holdTra, PGOSampleType accessorMethod);
    bool AddObjectInfo(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver,
		       JSHClass *hold, JSHClass *holdTra, uint32_t accessorMethodId = 0);
    void AddBuiltinsInfo(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver,
                         JSHClass *transitionHClass, OnHeapMode onHeap = OnHeapMode::NONE,
                         bool everOutOfBounds = false);
    void AddBuiltinsGlobalInfo(ApEntityId abcId, int32_t bcOffset, GlobalIndex globalsId);
    bool AddBuiltinsInfoByNameInInstance(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver);
    bool AddBuiltinsInfoByNameInProt(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver, JSHClass *hold);

    JSTaggedValue TryFindKeyInPrototypeChain(TaggedObject *currObj, JSHClass *currHC, JSTaggedValue key);
    bool IsJSHClassNotEqual(JSHClass *receiver, JSHClass *hold, JSHClass *exceptRecvHClass,
		            JSHClass *exceptRecvHClassOnHeap, JSHClass *exceptHoldHClass,
			    JSHClass *exceptPrototypeOfPrototypeHClass);
    // Other
    void UpdatePGOType(uint32_t offset, const pgo::PGOType *type)
    {
        if (type->IsScalarOpType()) {
            bcOffsetPGOOpTypeMap_[offset] = reinterpret_cast<const PGOSampleType *>(type);
        } else if (type->IsRwOpType()) {
            bcOffsetPGORwTypeMap_[offset] = reinterpret_cast<const PGORWOpType *>(type);
        } else if (type->IsDefineOpType()) {
            bcOffsetPGODefOpTypeMap_[offset] = reinterpret_cast<const PGODefineOpType *>(type);
        } else {
            UNREACHABLE();
        }
    }

    void Clear();

    EcmaVM *vm_ { nullptr };
    kungfu::PGOTypeManager *ptManager_ { nullptr };
    ProfileTypeInfo* profileTypeInfo_ { nullptr };
    ApEntityId abcId_ { 0 };
    EntityId methodId_ {};
    std::unordered_map<int32_t, const PGOSampleType*> bcOffsetPGOOpTypeMap_ {};
    std::unordered_map<int32_t, const PGORWOpType*> bcOffsetPGORwTypeMap_ {};
    std::unordered_map<int32_t, const PGODefineOpType*> bcOffsetPGODefOpTypeMap_{};
    RecursiveMutex mutex_;
    CompilationEnv *compilationEnv_ {nullptr};
    Chunk *chunk_ {nullptr};
};

}
#endif //ECMASCRIPT_JIT_PROFILER_H
