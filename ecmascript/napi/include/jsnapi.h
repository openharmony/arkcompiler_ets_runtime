/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_NAPI_INCLUDE_JSNAPI_H
#define ECMASCRIPT_NAPI_INCLUDE_JSNAPI_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
#include <map>
#include <sys/time.h>

#include "common_components/log/log_base.h"
#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/base/config.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#ifndef NDEBUG
#include "libpandabase/utils/debug.h"
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace panda {
class JSNApiHelper;
class EscapeLocalScope;
class PromiseRejectInfo;
template<typename T>
class CopyableGlobal;
template<typename T>
class Global;
class JSNApi;
template<typename T>
class Local;
class JSValueRef;
class PrimitiveRef;
class ArrayRef;
class BigIntRef;
class StringRef;
class ObjectRef;
class FunctionRef;
class NumberRef;
class BooleanRef;
class NativePointerRef;
class JsiRuntimeCallInfo;
namespace test {
class JSNApiTests;
}  // namespace test
class BufferRef;
namespace ecmascript {
class EcmaVM;
class JSTaggedValue;
class JSRuntimeOptions;
class JSThread;
struct EcmaRuntimeCallInfo;
static constexpr uint32_t DEFAULT_GC_POOL_SIZE = 256 * 1024 * 1024;
namespace base {
template<size_t ElementAlign, typename... Ts>
struct AlignedStruct;
struct AlignedPointer;
}
}  // namespace ecmascript

using WeakRefClearCallBack = void (*)(void *);
using EcmaVM = ecmascript::EcmaVM;
using JSThread = ecmascript::JSThread;
using JSTaggedType = uint64_t;
using ConcurrentCallback = void (*)(Local<JSValueRef> result, bool success, void *taskInfo, void *data);
using SourceMapTranslateCallback = std::function<bool(std::string& url, int& line, int& column,
    std::string &packageName)>;
using DeviceDisconnectCallback = std::function<bool()>;

static constexpr size_t DEFAULT_GC_THREAD_NUM = 7;
static constexpr size_t DEFAULT_LONG_PAUSE_TIME = 40;

class PUBLIC_API RegExpRef : public ObjectRef {
public:
    Local<StringRef> GetOriginalSource(const EcmaVM *vm);
    std::string GetOriginalFlags(const EcmaVM *vm);
    Local<JSValueRef> IsGlobal(const EcmaVM *vm);
    Local<JSValueRef> IsIgnoreCase(const EcmaVM *vm);
    Local<JSValueRef> IsMultiline(const EcmaVM *vm);
    Local<JSValueRef> IsDotAll(const EcmaVM *vm);
    Local<JSValueRef> IsUtf16(const EcmaVM *vm);
    Local<JSValueRef> IsStick(const EcmaVM *vm);
};

class PUBLIC_API GeneratorFunctionRef : public ObjectRef {
public:
    bool IsGenerator(const EcmaVM *vm);
};

class PUBLIC_API GeneratorObjectRef : public ObjectRef {
public:
    Local<JSValueRef> GetGeneratorState(const EcmaVM *vm);
    Local<JSValueRef> GetGeneratorFunction(const EcmaVM *vm);
    Local<JSValueRef> GetGeneratorReceiver(const EcmaVM *vm);
};

class PUBLIC_API CollatorRef : public ObjectRef {
public:
    Local<JSValueRef> GetCompareFunction(const EcmaVM *vm);
};

class PUBLIC_API DataTimeFormatRef : public ObjectRef {
public:
    Local<JSValueRef> GetFormatFunction(const EcmaVM *vm);
};

class PUBLIC_API NumberFormatRef : public ObjectRef {
public:
    Local<JSValueRef> GetFormatFunction(const EcmaVM *vm);
};

class PUBLIC_API JSON {
public:
    static Local<JSValueRef> Parse(const EcmaVM *vm, Local<StringRef> string);
    static Local<JSValueRef> Stringify(const EcmaVM *vm, Local<JSValueRef> json);
};

using LOG_PRINT = int (*)(int id, int level, const char *tag, const char *fmt, const char *message);

class PUBLIC_API RuntimeOption {
public:
    enum class PUBLIC_API GC_TYPE : uint8_t { EPSILON, GEN_GC, STW };
    using LOG_LEVEL = common::LOG_LEVEL;

    // This enum should follow the same value as defined in the BMS subsystem.
    // Refer to the specification in aot-guide_zh.md.
    enum class AOTCompileStatus {
        NOT_COMPILED = 0,
        COMPILE_SUCCESS = 1,
        COMPILE_FAILED = 2,
        COMPILE_CRASH = 3,
        COMPILE_CANCELLED = 4,
    };

    void SetGcType(GC_TYPE type)
    {
        gcType_ = type;
    }

    void SetGcPoolSize(uint32_t size)
    {
        gcPoolSize_ = size;
    }

    void SetLogLevel(LOG_LEVEL logLevel)
    {
        logLevel_ = logLevel;
    }

    void SetLogBufPrint(LOG_PRINT out)
    {
        logBufPrint_ = out;
    }

    void SetDebuggerLibraryPath(const std::string &path)
    {
        debuggerLibraryPath_ = path;
    }

    void SetEnableArkTools(bool value)
    {
        enableArkTools_ = value;
    }

    void SetEnableCpuprofiler(bool value)
    {
        enableCpuprofiler_ = value;
    }

    void SetArkProperties(int prop)
    {
        arkProperties_ = prop;
    }

    void SetArkBundleName(const std::string &bundleName)
    {
        arkBundleName_ = bundleName;
    }

    void SetMemConfigProperty(std::string configProperty)
    {
        memConfigProperty_ = configProperty;
    }

    void SetGcThreadNum(size_t num)
    {
        gcThreadNum_ = num;
    }

    void SetLongPauseTime(size_t time)
    {
        longPauseTime_ = time;
    }

    void SetEnableAsmInterpreter(bool value)
    {
        enableAsmInterpreter_ = value;
    }

    void SetEnableBuiltinsLazy(bool value)
    {
        enableBuiltinsLazy_ = value;
    }

    void SetAsmOpcodeDisableRange(const std::string &value)
    {
        asmOpcodeDisableRange_ = value;
    }

    void SetIsWorker()
    {
        isWorker_ = true;
    }

    bool GetIsWorker() const
    {
        return isWorker_;
    }

    inline void SetIsRestrictedWorker(bool isRestrictedWorker)
    {
        isRestrictedWorker_ = isRestrictedWorker;
    }

    bool GetIsRestrictedWorker() const
    {
        return isRestrictedWorker_;
    }

    void SetBundleName(const std::string &value)
    {
        bundleName_ = value;
    }

    void SetEnableAOT(bool value)
    {
        enableAOT_ = value;
    }

    void SetAnDir(const std::string &value)
    {
        anDir_ = value;
    }

    void SetEnableProfile(bool value)
    {
        enableProfile_ = value;
    }

    // Valid only when SetEnableProfile(true)
    void SetProfileDir(const std::string &value)
    {
        profileDir_ = value;
    }

    void SetEnableJIT(bool value)
    {
        enableFastJIT_ = value;
    }

    void SetEnableBaselineJIT(bool value)
    {
        enableBaselineJIT_ = value;
    }

    void SetLargeHeap(bool largeHeap)
    {
        enableLargeHeap_ = largeHeap;
    }

    bool GetLargeHeap() const
    {
        return enableLargeHeap_;
    }

    void SetAOTCompileStatusMap(const std::map<std::string, int32_t> &value)
    {
        aotCompileStatusMap_ = value;
    }

    const std::map<std::string, int32_t> &GetAOTCompileStatusMap() const
    {
        return aotCompileStatusMap_;
    }

private:
    std::string GetGcType() const
    {
        std::string gcType;
        switch (gcType_) {
            case GC_TYPE::GEN_GC:
                gcType = "gen-gc";
                break;
            case GC_TYPE::STW:
                gcType = "stw";
                break;
            case GC_TYPE::EPSILON:
                gcType = "epsilon";
                break;
            default:
                break;
        }
        return gcType;
    }

    LOG_LEVEL GetLogLevel() const
    {
        return logLevel_;
    }

    uint32_t GetGcPoolSize() const
    {
        return gcPoolSize_;
    }

    LOG_PRINT GetLogBufPrint() const
    {
        return logBufPrint_;
    }

    std::string GetDebuggerLibraryPath() const
    {
        return debuggerLibraryPath_;
    }

    bool GetEnableArkTools() const
    {
        return enableArkTools_;
    }

    bool GetEnableCpuprofiler() const
    {
        return enableCpuprofiler_;
    }

    int GetArkProperties() const
    {
        return arkProperties_;
    }

    std::string GetArkBundleName() const
    {
        return arkBundleName_;
    }

    std::string GetMemConfigProperty() const
    {
        return memConfigProperty_;
    }

    size_t GetGcThreadNum() const
    {
        return gcThreadNum_;
    }

    size_t GetLongPauseTime() const
    {
        return longPauseTime_;
    }

    bool GetEnableAsmInterpreter() const
    {
        return enableAsmInterpreter_;
    }

    bool GetEnableBuiltinsLazy() const
    {
        return enableBuiltinsLazy_;
    }

    std::string GetAsmOpcodeDisableRange() const
    {
        return asmOpcodeDisableRange_;
    }

    std::string GetBundleName() const
    {
        return bundleName_;
    }

    bool GetEnableAOT() const
    {
        return enableAOT_;
    }

    std::string GetAnDir() const
    {
        return anDir_;
    }

    bool GetEnableProfile() const
    {
        return enableProfile_;
    }

    std::string GetProfileDir() const
    {
        return profileDir_;
    }

    bool GetEnableJIT() const
    {
        return enableFastJIT_;
    }

    bool GetEnableDFXHiSysEvent() const
    {
        return enableDFXHiSysEvent_;
    }

    bool GetEnableBaselineJIT() const
    {
        return enableBaselineJIT_;
    }

    GC_TYPE gcType_ = GC_TYPE::EPSILON;
    LOG_LEVEL logLevel_ = LOG_LEVEL::DEBUG;
    uint32_t gcPoolSize_ = ecmascript::DEFAULT_GC_POOL_SIZE;
    LOG_PRINT logBufPrint_ {nullptr};
    std::string debuggerLibraryPath_ {};
    bool enableArkTools_ {false};
    bool enableCpuprofiler_ {false};
    int arkProperties_ {-1};
    std::string arkBundleName_ = {""};
    std::string memConfigProperty_ = {""};
    size_t gcThreadNum_ {DEFAULT_GC_THREAD_NUM};
    size_t longPauseTime_ {DEFAULT_LONG_PAUSE_TIME};
    bool enableAsmInterpreter_ {true};
    bool enableBuiltinsLazy_ {true};
    bool isWorker_ {false};
    bool isRestrictedWorker_ {false};
    std::string asmOpcodeDisableRange_ {""};
    std::string bundleName_ {};
    bool enableAOT_ {false};
    std::string anDir_ {};
    bool enableProfile_ {false};
    std::string profileDir_ {};
    bool enableFastJIT_ {false};
    bool enableDFXHiSysEvent_ {true};
    bool enableBaselineJIT_ {false};
    bool enableLargeHeap_ {false};
    std::map<std::string, int32_t> aotCompileStatusMap_;
    friend JSNApi;
};

template<typename T>
template<typename S>
void Global<T>::CreateXRefGloablReference(const EcmaVM *vm, const Local<S> &current)
{
    vm_ = vm;
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetXRefGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
void Global<T>::FreeXRefGlobalHandleAddr()
{
    if (address_ == 0) {
        return;
    }
    JSNApi::DisposeXRefGlobalHandleAddr(vm_, address_);
    address_ = 0;
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
    template<typename T>
    void Global<T>::MarkFromObject()
    {
        if (address_ == 0) {
            return;
        }
        JSNApi::MarkFromObject(vm_, address_);
    }

    template<typename T>
    bool Global<T>::IsObjectAlive() const
    {
        if (address_ == 0) {
            return false ;
        }
        return JSNApi::IsObjectAlive(vm_, address_);
    }

    template<typename T>
    bool Global<T>::IsValidHeapObject() const
    {
        if (address_ == 0) {
            return false;
        }
        return JSNApi::IsValidHeapObject(vm_, address_);
    }
#endif // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda

#endif  // ECMASCRIPT_NAPI_INCLUDE_JSNAPI_H
