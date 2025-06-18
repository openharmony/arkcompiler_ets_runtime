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

#ifndef ECMASCRIPT_COMPILER_COMPILATION_ENV_H
#define ECMASCRIPT_COMPILER_COMPILATION_ENV_H

#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"

namespace panda::ecmascript {
namespace kungfu {
    class PGOTypeManager;
    class LazyDeoptAllDependencies;
};
class ConstantPool;
namespace pgo {
class PGOProfiler;
};
class JSThread;

class CompilationEnv {
public:
    CompilationEnv(EcmaVM *vm);
    virtual ~CompilationEnv() = default;
    virtual bool IsJitCompiler() const
    {
        return false;
    }

    virtual bool IsAotCompiler() const
    {
        return false;
    }

    virtual kungfu::LazyDeoptAllDependencies *GetDependencies() const = 0;

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    JSThread *GetJSThread() const
    {
        return thread_;
    }

    kungfu::PGOTypeManager *GetPTManager() const
    {
        return ptManager_;
    }

    NativeAreaAllocator *GetNativeAreaAllocator() const;
    virtual JSRuntimeOptions &GetJSOptions() const = 0;
    virtual std::shared_ptr<pgo::PGOProfiler> GetPGOProfiler() const;

    // thread
    virtual GlobalEnvField GetArrayHClassIndex(ElementsKind kind, bool isProtoType) const = 0;
    virtual const BuiltinHClassEntries &GetBuiltinHClassEntries() const = 0;
    virtual JSHClass *GetBuiltinPrototypeHClass(BuiltinTypeId type) const = 0;

    // context
    virtual JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id) const = 0;
    virtual JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, int32_t index) const = 0;
    virtual JSTaggedValue FindOrCreateUnsharedConstpool(const uint32_t methodOffset) const = 0;
    virtual JSTaggedValue FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool) const = 0;
    virtual JSHandle<ConstantPool> FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
        panda_file::File::EntityId id) = 0;
    virtual JSTaggedValue GetConstantPoolByMethodOffset(const uint32_t methodOffset) const = 0;

    // ConstantPool
    virtual JSTaggedValue GetArrayLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const = 0;
    virtual JSTaggedValue GetObjectLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const = 0;
    virtual JSTaggedValue GetMethodFromCache(JSTaggedValue constpool, uint32_t index) const = 0;
    virtual panda_file::File::EntityId GetIdFromCache(JSTaggedValue constpool, uint32_t index) const = 0;
    virtual JSTaggedValue GetStringFromConstantPool(const uint32_t methodOffset, const uint16_t cpIdx,
        bool allowAlloc = true) const = 0;

    // GlobalEnv
    virtual JSHandle<GlobalEnv> GetGlobalEnv() const = 0;

    // GlobalConstants
    virtual const GlobalEnvConstants *GlobalConstants() const = 0;

    virtual JSHandle<JSTaggedValue> NewJSHandle(JSTaggedValue value) const = 0;

    virtual bool SupportHeapConstant() const = 0;

    virtual bool SupportIntrinsic() const = 0;

    virtual bool SupportNonExistIC() const = 0;

    virtual JSThread *GetHostThread() const
    {
        ASSERT(0);
        return nullptr;
    }

    virtual JSPandaFile *GetJSPandaFile() const
    {
        ASSERT(0);
        return nullptr;
    }

    virtual MethodLiteral *GetMethodLiteral() const
    {
        ASSERT(0);
        return nullptr;
    }

    virtual void ProcessMethod(MethodLiteral *method, const JSPandaFile *jsPandaFile) const
    {
        ASSERT(0);
    }

    virtual const uint8_t *GetMethodPcStart() const
    {
        ASSERT(0);
        return nullptr;
    }

    virtual pgo::ApEntityId GetMethodAbcId() const
    {
        ASSERT(0);
        return 0;
    }

protected:
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    kungfu::PGOTypeManager *ptManager_ {nullptr};
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_COMPILATION_ENV_H
