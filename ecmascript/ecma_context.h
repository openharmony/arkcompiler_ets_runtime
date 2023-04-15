/* * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ECMA_CONTEXT_H
#define ECMASCRIPT_ECMA_CONTEXT_H

#include "ecmascript/base/config.h"
#include "ecmascript/common.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

namespace panda {
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class GlobalEnv;
class ObjectFactory;
class JSPandaFile;
class EcmaStringTable;
class ConstantPool;
template<typename T>
class JSHandle;
class JSThread;
class EcmaVM;

namespace job {
class MicroJobQueue;
}  // namespace job
namespace tooling {
class JsDebuggerManager;
}  // namespace tooling

class EcmaContext {
public:
    static EcmaContext *Create(EcmaVM *vm);

    static bool Destroy(EcmaContext *context);

    EcmaContext(EcmaVM *vm);
    ~EcmaContext();

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    bool Initialize();

    bool ExecutePromisePendingJob();

    static EcmaContext *ConstCast(const EcmaContext *context)
    {
        return const_cast<EcmaContext *>(context);
    }

    EcmaStringTable *GetEcmaStringTable() const
    {
        ASSERT(stringTable_ != nullptr);
        return stringTable_;
    }

    ARK_INLINE JSThread *GetJSThread() const
    {
        return thread_;
    }

    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
    void EnableUserUncaughtErrorHandler();

    void AddConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool, int32_t index = 0);

    bool HasCachedConstpool(const JSPandaFile *jsPandaFile) const;

    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, int32_t index);
    // For new version instruction.
    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id);
    std::optional<std::reference_wrapper<CMap<int32_t, JSTaggedValue>>> FindConstpools(
        const JSPandaFile *jsPandaFile);

    JSHandle<ConstantPool> PUBLIC_API FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
                                                            panda_file::File::EntityId id);
    void CreateAllConstpool(const JSPandaFile *jsPandaFile);

    void HandleUncaughtException(JSTaggedValue exception);

    JSHandle<GlobalEnv> GetGlobalEnv() const;
    JSHandle<job::MicroJobQueue> GetMicroJobQueue() const;

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo);
    void Iterate(const RootVisitor &v, const RootRangeVisitor &rv);
    void MountContext();
    void UnmountContext();

private:
    void SetGlobalEnv(GlobalEnv *global);
    void SetMicroJobQueue(job::MicroJobQueue *queue);
    void ClearBufferData();
    Expected<JSTaggedValue, bool> InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                                       bool excuteFromJob = false);

    NO_MOVE_SEMANTIC(EcmaContext);
    NO_COPY_SEMANTIC(EcmaContext);

    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    JSThread *thread_ {nullptr};

    bool isUncaughtExceptionRegistered_ {false};
    bool isProcessingPendingJob_ {false};

    EcmaStringTable *stringTable_ {nullptr};
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};

    JSTaggedValue microJobQueue_ {JSTaggedValue::Hole()};
    CMap<const JSPandaFile *, CMap<int32_t, JSTaggedValue>> cachedConstpools_ {};
    CString assetPath_;

    friend class JSPandaFileExecutor;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_ECMA_CONTEXT_H
