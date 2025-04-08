/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <optional>
#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/base/config.h"
#include "ecmascript/common.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/frames.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/module/js_module_execute_type.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "libpandafile/file.h"

namespace panda {
class JSNApi;
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class GlobalEnv;
class ObjectFactory;
class EcmaRuntimeStat;
class JSPandaFileManager;
class JSPandaFile;
class ConstantPool;
class RegExpExecResultCache;
class SustainingJSHandleList;
class SustainingJSHandle;
enum class PromiseRejectionEvent : uint8_t;

template<typename T>
class JSHandle;
class JSThread;
class JSFunction;
class JSTaggedValue;
class EcmaVM;
class QuickFixManager;
class AbcBufferCache;
struct CJSInfo;

namespace tooling {
class JsDebuggerManager;
} // namespace tooling

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
using JsAotReaderCallback = std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)>;
#endif
class EcmaContext {
public:
    static EcmaContext *CreateAndInitialize(JSThread *thread);
    static void CheckAndDestroy(JSThread *thread, EcmaContext *context);

    static EcmaContext *Create(JSThread *thread);
    static bool Destroy(EcmaContext *context);

    EcmaContext(JSThread *thread);
    ~EcmaContext();

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    bool Initialize();

    static EcmaContext *ConstCast(const EcmaContext *context)
    {
        return const_cast<EcmaContext *>(context);
    }

    bool IsInitialized() const
    {
        return initialized_;
    }

    ARK_INLINE JSThread *GetJSThread() const
    {
        return thread_;
    }

    JSHandle<GlobalEnv> GetGlobalEnv() const;
    bool GlobalEnvIsHole()
    {
        return globalEnv_.IsHole();
    }

    void Iterate(RootVisitor &v);
    static void MountContext(JSThread *thread);
    static void UnmountContext(JSThread *thread);
    void SetGlobalEnv(GlobalEnv* global);

    const GlobalEnvConstants *GlobalConstants() const
    {
        return &globalConst_;
    }

    void ClearKeptObjects();
    void AddToKeptObjects(JSHandle<JSTaggedValue> value);

private:
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    bool LoadAOTFiles(const std::string &aotFileName,
                      std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb);
#endif

    NO_MOVE_SEMANTIC(EcmaContext);
    NO_COPY_SEMANTIC(EcmaContext);
    JSThread *thread_{nullptr};
    EcmaVM *vm_{nullptr};

    bool initialized_ {false};
    ObjectFactory *factory_ {nullptr};

    // VM execution states.
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};

    GlobalEnvConstants globalConst_;

    friend class JSPandaFileExecutor;
    friend class ObjectFactory;
    friend class panda::JSNApi;
    friend class AOTFileManager;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_ECMA_CONTEXT_H
