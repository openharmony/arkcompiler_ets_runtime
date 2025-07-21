/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_NAPI_INCLUDE_JSNAPI_EXPO_H
#define ECMASCRIPT_NAPI_INCLUDE_JSNAPI_EXPO_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <map>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/base/config.h"
#include "ecmascript/common.h"
#include "ecmascript/module/js_module_execute_type.h"
#include "ecmascript/napi/include/jsnapi_internals.h"

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
class SymbolRef;
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
class MapIteratorRef;
class SendableMapIteratorRef;
class BooleanRef;
class NativePointerRef;
class TypedArrayRef;
class JsiRuntimeCallInfo;
class RuntimeOption;
namespace test {
class JSNApiTests;
}  // namespace test
class BufferRef;
namespace ecmascript {
class EcmaVM;
class JSTaggedValue;
class JSRuntimeOptions;
class EcmaContext;
class JSThread;
struct EcmaRuntimeCallInfo;
namespace base {
template<size_t ElementAlign, typename... Ts>
struct AlignedStruct;
struct AlignedPointer;
}
namespace job {
enum class QueueType : uint8_t {
    QUEUE_PROMISE,
    QUEUE_SCRIPT,
};
}
}  // namespace ecmascript

enum class ForHybridApp {
    Normal,
    Hybrid
};

struct HmsMap {
    std::string originalPath;
    std::string targetPath;
    uint32_t sinceVersion;
};

struct StackInfo {
    size_t stackStart;
    size_t stackSize;
};

using WeakRefClearCallBack = void (*)(void *);
using WeakFinalizeTaskCallback = std::function<void()>;
using NativePointerCallbackFinishNotify = std::function<void(size_t totalBindSize_)>;
using NativePointerCallbackData = std::pair<NativePointerCallback, std::tuple<void*, void*, void*>>;
using TriggerGCData = std::pair<void*, uint8_t>;
using TriggerGCTaskCallback = std::function<void(TriggerGCData& data)>;
using StartIdleMonitorCallback = std::function<void()>;
using EcmaVM = ecmascript::EcmaVM;
using JSThread = ecmascript::JSThread;
using JSTaggedType = uint64_t;
using ConcurrentCallback = void (*)(Local<JSValueRef> result, bool success, void *taskInfo, void *data);
using SourceMapCallback = std::function<std::string(const std::string& rawStack)>;
using TimerCallbackFunc = void (*)(void *data);
using TimerTaskCallback = void* (*)(EcmaVM *vm, void *data, TimerCallbackFunc func, uint64_t timeout, bool repeat);
using CancelTimerCallback = void (*)(void *timerCallbackInfo);
using ReleaseSecureMemCallback = std::function<void(void* fileMapper)>;
using SourceMapTranslateCallback = std::function<bool(std::string& url, int& line, int& column,
    std::string& packageName)>;
using DeviceDisconnectCallback = std::function<bool()>;
using QueueType = ecmascript::job::QueueType;
using OnErrorCallback = std::function<void(Local<ObjectRef> value, void *data)>;
using StopPreLoadSoCallback = std::function<void()>;

#define ECMA_DISALLOW_COPY(className)      \
    className(const className &) = delete; \
    className &operator=(const className &) = delete

#define ECMA_DISALLOW_MOVE(className) \
    className(className &&) = delete; \
    className &operator=(className &&) = delete

#ifndef NDEBUG
#define ECMA_ASSERT(cond) \
    if (!(cond)) { \
        panda::debug::AssertionFail(#cond, __FILE__, __LINE__, __FUNCTION__); \
    }
#else
#define ECMA_ASSERT(cond) static_cast<void>(0)
#endif

#define INIT_CRASH_HOLDER(holder, tag) \
    ArkCrashHolder holder(tag, __FUNCTION__)

class PUBLIC_API ArkCrashHolder {
public:
    ArkCrashHolder(const std::string& tag, const std::string& info)
    {
        SetCrashObj(tag.c_str(), info.c_str());
    }

    ArkCrashHolder(const char* tag, const char* info)
    {
        SetCrashObj(tag, info);
    }

    ~ArkCrashHolder();

void UpdateCallbackPtr(uintptr_t addr);

private:
    void SetCrashObj(const char* tag, const char* info);

    uintptr_t handle_ {0};

    char* data_ {nullptr};
    size_t size_ {0};
};

class PUBLIC_API AsyncNativeCallbacksPack {
public:
    AsyncNativeCallbacksPack() = default;
    ~AsyncNativeCallbacksPack() = default;
    AsyncNativeCallbacksPack(AsyncNativeCallbacksPack&&) = default;
    AsyncNativeCallbacksPack& operator=(AsyncNativeCallbacksPack&&) = default;
    AsyncNativeCallbacksPack(const AsyncNativeCallbacksPack &) = default;
    AsyncNativeCallbacksPack &operator=(const AsyncNativeCallbacksPack &) = default;

    void Clear()
    {
        callBacks_.clear();
        totalBindingSize_ = 0;
        notify_ = nullptr;
    }

    bool TotallyEmpty() const
    {
        return callBacks_.empty() && totalBindingSize_ == 0 && notify_ == nullptr;
    }

    bool Empty() const
    {
        return callBacks_.empty();
    }

    void RegisterFinishNotify(NativePointerCallbackFinishNotify notify)
    {
        notify_ = notify;
    }

    size_t GetNumCallBacks() const
    {
        return callBacks_.size();
    }

    void ProcessAll(const char* tag)
    {
        INIT_CRASH_HOLDER(holder, tag);
        for (auto &iter : callBacks_) {
            NativePointerCallback callback = iter.first;
            std::tuple<void*, void*, void*> &param = iter.second;
            if (callback != nullptr) {
                holder.UpdateCallbackPtr(reinterpret_cast<uintptr_t>(callback));
                callback(std::get<0>(param), std::get<1>(param), std::get<2>(param)); // 2 is the param.
            }
        }
        NotifyFinish();
    }

    size_t GetTotalBindingSize() const
    {
        return totalBindingSize_;
    }

    void AddCallback(NativePointerCallbackData callback, size_t bindingSize)
    {
        callBacks_.emplace_back(callback);
        totalBindingSize_ += bindingSize;
    }
private:
    void NotifyFinish() const
    {
        if (notify_ != nullptr) {
            notify_(totalBindingSize_);
        }
    }

    std::vector<NativePointerCallbackData> callBacks_ {};
    size_t totalBindingSize_ {0};
    NativePointerCallbackFinishNotify notify_ {nullptr};
};
using NativePointerTaskCallback = std::function<void(AsyncNativeCallbacksPack *callbacksPack)>;

template<typename T>
class PUBLIC_API Local {  // NOLINT(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
public:
    inline Local() = default;

    template<typename S>
    inline Local(const Local<S> &current) : address_(reinterpret_cast<uintptr_t>(*current))
    {
        // Check
    }

    Local(const EcmaVM *vm, const Global<T> &current);

    Local(const EcmaVM *vm, const CopyableGlobal<T> &current);

    ~Local() = default;

    inline T *operator*() const
    {
        return GetAddress();
    }

    inline T *operator->() const
    {
        return GetAddress();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == nullptr;
    }

    inline void Empty()
    {
        address_ = 0;
    }

    inline bool IsNull() const
    {
        return IsEmpty() || GetAddress()->IsHole();
    }

    explicit inline Local(uintptr_t addr) : address_(addr) {}

    inline bool operator==(const Local<T> &other) const
    {
        return *reinterpret_cast<JSTaggedType *>(GetAddress()) ==
            *reinterpret_cast<JSTaggedType *>(other.GetAddress());
    }

private:
    inline T *GetAddress() const
    {
        return reinterpret_cast<T *>(address_);
    };
    uintptr_t address_ = 0U;
    friend JSNApiHelper;
    friend EscapeLocalScope;
    friend JsiRuntimeCallInfo;
};

/**
 * A Copyable global handle, keeps a separate global handle for each CopyableGlobal.
 *
 * Support Copy Constructor and Assign, Move Constructor And Assign.
 *
 * If destructed, the global handle held will be automatically released.
 *
 * Usage: It Can be used as heap object assign to another variable, a value passing parameter, or
 *        a value passing return value and so on.
 */
template<typename T>
class PUBLIC_API CopyableGlobal {
public:
    inline CopyableGlobal() = default;
    ~CopyableGlobal()
    {
        Free();
    }

    inline CopyableGlobal(const CopyableGlobal &that)
    {
        Copy(that);
    }

    inline CopyableGlobal &operator=(const CopyableGlobal &that)
    {
        Copy(that);
        return *this;
    }

    inline CopyableGlobal(CopyableGlobal &&that)
    {
        Move(that);
    }

    inline CopyableGlobal &operator=(CopyableGlobal &&that)
    {
        Move(that);
        return *this;
    }

    template<typename S>
    CopyableGlobal(const EcmaVM *vm, const Local<S> &current);

    CopyableGlobal(const EcmaVM *vm, const Local<T> &current);

    template<typename S>
    CopyableGlobal(const CopyableGlobal<S> &that)
    {
        Copy(that);
    }

    void Reset()
    {
        Free();
    }

    Local<T> ToLocal() const
    {
        if (IsEmpty()) {
            return Local<T>();
        }
        return Local<T>(vm_, *this);
    }

    void Empty()
    {
        address_ = 0;
    }

    inline T *operator*() const
    {
        return GetAddress();
    }

    inline T *operator->() const
    {
        return GetAddress();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == nullptr;
    }

    void SetWeakCallback(void *ref, WeakRefClearCallBack freeGlobalCallBack,
                         WeakRefClearCallBack nativeFinalizeCallback);
    void SetWeak();

    void ClearWeak();

    bool IsWeak() const;

    const EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

private:
    inline T *GetAddress() const
    {
        return reinterpret_cast<T *>(address_);
    };
    inline void Copy(const CopyableGlobal &that);
    template<typename S>
    inline void Copy(const CopyableGlobal<S> &that);
    inline void Move(CopyableGlobal &that);
    inline void Free();
    uintptr_t address_ = 0U;
    const EcmaVM *vm_ {nullptr};
};

template<typename T>
class PUBLIC_API Global {  // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions
public:
    inline Global() = default;

    inline Global(const Global &that)
    {
        Update(that);
    }

    inline Global &operator=(const Global &that)
    {
        Update(that);
        return *this;
    }

    inline Global(Global &&that)
    {
        Update(that);
    }

    inline Global &operator=(Global &&that)
    {
        Update(that);
        return *this;
    }

    template<typename S>
    Global(const EcmaVM *vm, const Local<S> &current);
    template<typename S>
    Global(const EcmaVM *vm, const Global<S> &current);

    ~Global() = default;

    Local<T> ToLocal() const
    {
        if (IsEmpty()) {
            return Local<T>();
        }
        return Local<T>(vm_, *this);
    }

    Local<T> ToLocal(const EcmaVM *vm) const
    {
        return Local<T>(vm, *this);
    }

    void Empty()
    {
        address_ = 0;
    }

    template<typename S>
    void CreateXRefGloablReference(const EcmaVM *vm, const Local<S> &current);

    // This method must be called before Global is released.
    void FreeGlobalHandleAddr();
    void FreeXRefGlobalHandleAddr();
    void MarkFromObject();
    bool IsObjectAlive() const;
    bool IsValidHeapObject() const;

    inline T *operator*() const
    {
        return GetAddress();
    }

    inline T *operator->() const
    {
        return GetAddress();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == nullptr;
    }

    void SetWeak();

    void SetWeakCallback(void *ref, WeakRefClearCallBack freeGlobalCallBack,
                         WeakRefClearCallBack nativeFinalizeCallback);

    void ClearWeak();

    bool IsWeak() const;

    const EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

private:
    inline T *GetAddress() const
    {
        return reinterpret_cast<T *>(address_);
    };
    inline void Update(const Global &that);
    uintptr_t address_ = 0U;
    const EcmaVM *vm_ {nullptr};
};

class PUBLIC_API JSValueRef {
public:
    static Local<PrimitiveRef> Undefined(const EcmaVM *vm);
    static Local<PrimitiveRef> Null(const EcmaVM *vm);
    static Local<PrimitiveRef> Hole(const EcmaVM *vm);
    static Local<PrimitiveRef> True(const EcmaVM *vm);
    static Local<PrimitiveRef> False(const EcmaVM *vm);

    bool BooleaValue(const EcmaVM *vm);
    int64_t IntegerValue(const EcmaVM *vm);
    uint32_t Uint32Value(const EcmaVM *vm);
    int32_t Int32Value(const EcmaVM *vm);
    double GetValueDouble(bool &isNumber);
    int32_t GetValueInt32(bool &isNumber);
    uint32_t GetValueUint32(bool &isNumber);
    int64_t GetValueInt64(bool &isNumber);
    bool GetValueBool(bool &isBool);

    Local<NumberRef> ToNumber(const EcmaVM *vm);
    Local<BooleanRef> ToBoolean(const EcmaVM *vm);
    Local<BigIntRef> ToBigInt(const EcmaVM *vm);
    Local<StringRef> ToString(const EcmaVM *vm);
    Local<ObjectRef> ToObject(const EcmaVM *vm);
    Local<ObjectRef> ToEcmaObject(const EcmaVM *vm);
    Local<ObjectRef> ToEcmaObjectWithoutSwitchState(const EcmaVM *vm);
    Local<NativePointerRef> ToNativePointer(const EcmaVM *vm);

    bool IsUndefined();
    bool IsNull();
    bool IsHole();
    bool IsTrue();
    bool IsFalse();
    bool IsNumber();
    bool IsBigInt(const EcmaVM *vm);
    bool IsInt();
    bool WithinInt32();
    bool IsBoolean();
    bool IsString(const EcmaVM *vm);
    bool IsStringWithoutSwitchState(const EcmaVM *vm);
    bool IsSymbol(const EcmaVM *vm);
    bool IsObject(const EcmaVM *vm);
    bool IsObjectWithoutSwitchState(const EcmaVM *vm);
    bool IsNativeBindingObject(const EcmaVM *vm);
    bool IsArray(const EcmaVM *vm);
    bool IsJSArray(const EcmaVM *vm);
    bool IsConstructor(const EcmaVM *vm);
    bool IsFunction(const EcmaVM *vm);

    bool IsJSFunction(const EcmaVM *vm);
    bool IsProxy(const EcmaVM *vm);
    bool IsPromise(const EcmaVM *vm);
    bool IsDataView(const EcmaVM *vm);
    bool IsTypedArray(const EcmaVM *vm);
    bool IsNativePointer(const EcmaVM *vm);
    bool IsDate(const EcmaVM *vm);
    bool IsError(const EcmaVM *vm);
    bool IsMap(const EcmaVM *vm);
    bool IsSet(const EcmaVM *vm);
    bool IsWeakRef(const EcmaVM *vm);
    bool IsWeakMap(const EcmaVM *vm);
    bool IsWeakSet(const EcmaVM *vm);
    bool IsRegExp(const EcmaVM *vm);
    bool IsArrayIterator(const EcmaVM *vm);
    bool IsStringIterator(const EcmaVM *vm);
    bool IsSetIterator(const EcmaVM *vm);
    bool IsMapIterator(const EcmaVM *vm);
    bool IsArrayBuffer(const EcmaVM *vm);
    bool IsBuffer(const EcmaVM *vm);
    bool IsUint8Array(const EcmaVM *vm);
    bool IsInt8Array(const EcmaVM *vm);
    bool IsUint8ClampedArray(const EcmaVM *vm);
    bool IsInt16Array(const EcmaVM *vm);
    bool IsUint16Array(const EcmaVM *vm);
    bool IsInt32Array(const EcmaVM *vm);
    bool IsUint32Array(const EcmaVM *vm);
    bool IsFloat32Array(const EcmaVM *vm);
    bool IsFloat64Array(const EcmaVM *vm);
    bool IsBigInt64Array(const EcmaVM *vm);
    bool IsBigUint64Array(const EcmaVM *vm);
    bool IsJSPrimitiveRef(const EcmaVM *vm);
    bool IsJSPrimitiveNumber(const EcmaVM *vm);
    bool IsJSPrimitiveInt(const EcmaVM *vm);
    bool IsJSPrimitiveBoolean(const EcmaVM *vm);
    bool IsJSPrimitiveString(const EcmaVM *vm);

    bool IsJSSharedInt8Array(const EcmaVM *vm);
    bool IsJSSharedUint8Array(const EcmaVM *vm);
    bool IsJSSharedUint8ClampedArray(const EcmaVM *vm);
    bool IsJSSharedInt16Array(const EcmaVM *vm);
    bool IsJSSharedUint16Array(const EcmaVM *vm);
    bool IsJSSharedInt32Array(const EcmaVM *vm);
    bool IsJSSharedUint32Array(const EcmaVM *vm);
    bool IsJSSharedFloat32Array(const EcmaVM *vm);
    bool IsJSSharedFloat64Array(const EcmaVM *vm);
    bool IsJSSharedBigInt64Array(const EcmaVM *vm);
    bool IsJSSharedBigUint64Array(const EcmaVM *vm);

    bool IsGeneratorObject(const EcmaVM *vm);
    bool IsJSPrimitiveSymbol(const EcmaVM *vm);

    bool IsArgumentsObject(const EcmaVM *vm);
    bool IsGeneratorFunction(const EcmaVM *vm);
    bool IsAsyncFunction(const EcmaVM *vm);
    bool IsConcurrentFunction(const EcmaVM *vm);
    bool IsJSLocale(const EcmaVM *vm);
    bool IsJSDateTimeFormat(const EcmaVM *vm);
    bool IsJSRelativeTimeFormat(const EcmaVM *vm);
    bool IsJSIntl(const EcmaVM *vm);
    bool IsJSNumberFormat(const EcmaVM *vm);
    bool IsJSCollator(const EcmaVM *vm);
    bool IsJSPluralRules(const EcmaVM *vm);
    bool IsJSListFormat(const EcmaVM *vm);
    bool IsAsyncGeneratorFunction(const EcmaVM *vm);
    bool IsAsyncGeneratorObject(const EcmaVM *vm);

    bool IsModuleNamespaceObject(const EcmaVM *vm);
    bool IsNativeModuleFailureInfoObject(const EcmaVM *vm);
    bool IsSharedArrayBuffer(const EcmaVM *vm);
    bool IsSendableArrayBuffer(const EcmaVM *vm);

    bool IsStrictEquals(const EcmaVM *vm, Local<JSValueRef> value);
    Local<StringRef> Typeof(const EcmaVM *vm);
    bool InstanceOf(const EcmaVM *vm, Local<JSValueRef> value);

    bool IsArrayList(const EcmaVM *vm);
    bool IsDeque(const EcmaVM *vm);
    bool IsHashMap(const EcmaVM *vm);
    bool IsHashSet(const EcmaVM *vm);
    bool IsLightWeightMap(const EcmaVM *vm);
    bool IsLightWeightSet(const EcmaVM *vm);
    bool IsLinkedList(const EcmaVM *vm);
    bool IsLinkedListIterator(const EcmaVM *vm);
    bool IsList(const EcmaVM *vm);
    bool IsPlainArray(const EcmaVM *vm);
    bool IsQueue(const EcmaVM *vm);
    bool IsStack(const EcmaVM *vm);
    bool IsTreeMap(const EcmaVM *vm);
    bool IsTreeSet(const EcmaVM *vm);
    bool IsVector(const EcmaVM *vm);
    bool IsBitVector(const EcmaVM *vm);
    bool IsSendableObject(const EcmaVM *vm);
    bool IsJSShared(const EcmaVM *vm);
    bool IsSharedArray(const EcmaVM *vm);
    bool IsSharedTypedArray(const EcmaVM *vm);
    bool IsSharedSet(const EcmaVM *vm);
    bool IsSharedMap(const EcmaVM *vm);
    bool IsSharedMapIterator(const EcmaVM *vm);
    bool IsHeapObject();
    void *GetNativePointerValue(const EcmaVM *vm, bool &isNativePointer);
    bool IsDetachedArraybuffer(const EcmaVM *vm, bool &isArrayBuffer);
    void DetachedArraybuffer(const EcmaVM *vm, bool &isArrayBuffer);
    void GetDataViewInfo(const EcmaVM *vm,
                         bool &isDataView,
                         size_t *byteLength,
                         void **data,
                         JSValueRef **arrayBuffer,
                         size_t *byteOffset);
    void TryGetArrayLength(const EcmaVM *vm, bool *isPendingException,
        bool *isArrayOrSharedArray, uint32_t *arrayLength);
    bool IsJsGlobalEnv(const EcmaVM *vm);

private:
    JSTaggedType value_;
    friend JSNApi;
    template<typename T>
    friend class Global;
    template<typename T>
    friend class Local;
    void *GetNativePointerValueImpl(const EcmaVM *vm, bool &isNativePointer);
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class PUBLIC_API PropertyAttribute {
public:
    static PropertyAttribute Default()
    {
        return PropertyAttribute();
    }
    PropertyAttribute() = default;
    PropertyAttribute(Local<JSValueRef> value, bool w, bool e, bool c)
        : value_(value),
          writable_(w),
          enumerable_(e),
          configurable_(c),
          hasWritable_(true),
          hasEnumerable_(true),
          hasConfigurable_(true)
    {}
    ~PropertyAttribute() = default;

    bool IsWritable() const
    {
        return writable_;
    }
    void SetWritable(bool flag)
    {
        writable_ = flag;
        hasWritable_ = true;
    }
    bool IsEnumerable() const
    {
        return enumerable_;
    }
    void SetEnumerable(bool flag)
    {
        enumerable_ = flag;
        hasEnumerable_ = true;
    }
    bool IsConfigurable() const
    {
        return configurable_;
    }
    void SetConfigurable(bool flag)
    {
        configurable_ = flag;
        hasConfigurable_ = true;
    }
    bool HasWritable() const
    {
        return hasWritable_;
    }
    bool HasConfigurable() const
    {
        return hasConfigurable_;
    }
    bool HasEnumerable() const
    {
        return hasEnumerable_;
    }
    Local<JSValueRef> GetValue(const EcmaVM *vm) const
    {
        if (value_.IsEmpty()) {
            return JSValueRef::Undefined(vm);
        }
        return value_;
    }
    void SetValue(Local<JSValueRef> value)
    {
        value_ = value;
    }
    inline bool HasValue() const
    {
        return !value_.IsEmpty();
    }
    Local<JSValueRef> GetGetter(const EcmaVM *vm) const
    {
        if (getter_.IsEmpty()) {
            return JSValueRef::Undefined(vm);
        }
        return getter_;
    }
    void SetGetter(Local<JSValueRef> value)
    {
        getter_ = value;
    }
    bool HasGetter() const
    {
        return !getter_.IsEmpty();
    }
    Local<JSValueRef> GetSetter(const EcmaVM *vm) const
    {
        if (setter_.IsEmpty()) {
            return JSValueRef::Undefined(vm);
        }
        return setter_;
    }
    void SetSetter(Local<JSValueRef> value)
    {
        setter_ = value;
    }
    bool HasSetter() const
    {
        return !setter_.IsEmpty();
    }

private:
    Local<JSValueRef> value_;
    Local<JSValueRef> getter_;
    Local<JSValueRef> setter_;
    bool writable_ = false;
    bool enumerable_ = false;
    bool configurable_ = false;
    bool hasWritable_ = false;
    bool hasEnumerable_ = false;
    bool hasConfigurable_ = false;
};

class PUBLIC_API NativePointerRef : public JSValueRef {
public:
    static Local<NativePointerRef> New(const EcmaVM *vm, void *nativePointer, size_t nativeBindingsize = 0);
    static Local<NativePointerRef> New(const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack,
                                       void *data, size_t nativeBindingsize = 0);
    static Local<NativePointerRef> NewConcurrent(const EcmaVM *vm, void *nativePointer,
                                                 NativePointerCallback callBack,
                                                 void *data, size_t nativeBindingsize = 0);
    static Local<NativePointerRef> NewSendable(const EcmaVM *vm,
                                               void *nativePointer,
                                               NativePointerCallback callBack = nullptr,
                                               void *data = nullptr,
                                               size_t nativeBindingsize = 0);
    void *Value();
};

class PUBLIC_API ObjectRef : public JSValueRef {
public:
    enum class SendableType {
        NONE,
        OBJECT,
        GENERIC,
    };
    struct SendablePropertiesInfo {
        std::vector<Local<JSValueRef>> keys;
        std::vector<SendableType> types;
        std::vector<PropertyAttribute> attributes;
    };
    static constexpr int MAX_PROPERTIES_ON_STACK = 32;
    static inline ObjectRef *Cast(JSValueRef *value)
    {
        return static_cast<ObjectRef *>(value);
    }
    static Local<ObjectRef> New(const EcmaVM *vm);
    static uintptr_t NewObject(const EcmaVM *vm);
    static Local<ObjectRef> NewJSXRefObject(const EcmaVM *vm);
    static Local<ObjectRef> NewS(const EcmaVM *vm);
    static Local<ObjectRef> NewWithProperties(const EcmaVM *vm, size_t propertyCount, const Local<JSValueRef> *keys,
                                              const PropertyAttribute *attributes);
    static Local<ObjectRef> NewSWithProperties(const EcmaVM *vm, SendablePropertiesInfo &info);
    static Local<ObjectRef> NewWithNamedProperties(const EcmaVM *vm, size_t propertyCount, const char **keys,
                                                   const Local<JSValueRef> *values);
    static Local<ObjectRef> CreateNativeModuleFailureInfo(const EcmaVM *vm, const std::string &failureInfo);
    static Local<ObjectRef> CreateAccessorData(const EcmaVM *vm, Local<FunctionRef> getter, Local<FunctionRef> setter);
    static Local<ObjectRef> CreateSendableAccessorData(const EcmaVM *vm,
                                                       Local<FunctionRef> getter,
                                                       Local<FunctionRef> setter);
    bool ConvertToNativeBindingObject(const EcmaVM *vm, Local<NativePointerRef> value);
    Local<NativePointerRef> GetNativeBindingPointer(const EcmaVM *vm);
    bool Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value);
    bool Set(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value);
    bool Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value);
    bool SetWithoutSwitchState(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value);
    bool SetAccessorProperty(const EcmaVM *vm, Local<JSValueRef> key, Local<FunctionRef> getter,
                             Local<FunctionRef> setter, PropertyAttribute attribute = PropertyAttribute::Default());
    Local<JSValueRef> Get(const EcmaVM *vm, Local<JSValueRef> key);
    Local<JSValueRef> Get(const EcmaVM *vm, const char *utf8);
    Local<JSValueRef> Get(const EcmaVM *vm, int32_t key);

    bool GetOwnProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute &property);
    Local<ArrayRef> GetOwnPropertyNames(const EcmaVM *vm);
    Local<ArrayRef> GetAllPropertyNames(const EcmaVM *vm, uint32_t filter);
    Local<ArrayRef> GetOwnEnumerablePropertyNames(const EcmaVM *vm);
    Local<JSValueRef> GetPrototype(const EcmaVM *vm);
    bool SetPrototype(const EcmaVM *vm, Local<ObjectRef> prototype);

    bool DefineProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute attribute);

    bool Has(const EcmaVM *vm, Local<JSValueRef> key);
    bool Has(const EcmaVM *vm, uint32_t key);

    bool HasOwnProperty(const EcmaVM *vm, Local<JSValueRef> key);

    bool Delete(const EcmaVM *vm, Local<JSValueRef> key);
    bool Delete(const EcmaVM *vm, uint32_t key);

    Local<JSValueRef> Freeze(const EcmaVM *vm);
    Local<JSValueRef> Seal(const EcmaVM *vm);
    void SetNativePointerFieldCount(const EcmaVM *vm, int32_t count);
    int32_t GetNativePointerFieldCount(const EcmaVM *vm);
    void *GetNativePointerField(const EcmaVM *vm, int32_t index);
    void SetNativePointerField(const EcmaVM *vm,
                               int32_t index,
                               void *nativePointer = nullptr,
                               NativePointerCallback callBack = nullptr,
                               void *data = nullptr, size_t nativeBindingsize = 0);
    void SetConcurrentNativePointerField(const EcmaVM *vm,
                                         int32_t index,
                                         void *nativePointer = nullptr,
                                         NativePointerCallback callBack = nullptr,
                                         void *data = nullptr, size_t nativeBindingsize = 0);
};

using FunctionCallback = Local<JSValueRef>(*)(JsiRuntimeCallInfo*);
using InternalFunctionCallback = JSValueRef(*)(JsiRuntimeCallInfo*);
class PUBLIC_API FunctionRef : public ObjectRef {
public:
    struct SendablePropertiesInfos {
        SendablePropertiesInfo instancePropertiesInfo;
        SendablePropertiesInfo staticPropertiesInfo;
        SendablePropertiesInfo nonStaticPropertiesInfo;
    };
    static Local<FunctionRef> New(EcmaVM *vm, FunctionCallback nativeFunc, NativePointerCallback deleter = nullptr,
        void *data = nullptr, bool callNapi = false, size_t nativeBindingsize = 0);
    static Local<FunctionRef> New(EcmaVM *vm, const Local<JSValueRef> &context, FunctionCallback nativeFunc,
        NativePointerCallback deleter = nullptr, void *data = nullptr, bool callNapi = false,
        size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrent(EcmaVM *vm,
                                            FunctionCallback nativeFunc,
                                            NativePointerCallback deleter = nullptr,
                                            void *data = nullptr,
                                            bool callNapi = false,
                                            size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrent(EcmaVM *vm,
                                            const Local<JSValueRef> &context,
                                            FunctionCallback nativeFunc,
                                            NativePointerCallback deleter = nullptr,
                                            void *data = nullptr,
                                            bool callNapi = false,
                                            size_t nativeBindingsize = 0);
    static Local<FunctionRef> New(EcmaVM *vm, InternalFunctionCallback nativeFunc, NativePointerCallback deleter,
        void *data = nullptr, bool callNapi = false, size_t nativeBindingsize = 0);
    static Local<FunctionRef> New(EcmaVM *vm, const Local<JSValueRef> &context, InternalFunctionCallback nativeFunc,
        NativePointerCallback deleter, void *data = nullptr, bool callNapi = false, size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrent(EcmaVM *vm,
                                            const Local<JSValueRef> &context,
                                            InternalFunctionCallback nativeFunc,
                                            NativePointerCallback deleter,
                                            void *data = nullptr,
                                            bool callNapi = false,
                                            size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrent(EcmaVM *vm,
                                            InternalFunctionCallback nativeFunc,
                                            NativePointerCallback deleter,
                                            void *data = nullptr,
                                            bool callNapi = false,
                                            size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrentWithName(EcmaVM *vm, const Local<JSValueRef> &context,
                                                    InternalFunctionCallback nativeFunc, NativePointerCallback deleter,
                                                    const char *name, void *data = nullptr, bool callNapi = false,
                                                    size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewSendable(EcmaVM *vm,
                                          InternalFunctionCallback nativeFunc,
                                          NativePointerCallback deleter,
                                          void *data = nullptr,
                                          bool callNapi = false,
                                          size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm, FunctionCallback nativeFunc, NativePointerCallback deleter,
        void *data, bool callNapi = false, size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm, const Local<JSValueRef> &context,
        FunctionCallback nativeFunc, NativePointerCallback deleter, void *data, bool callNapi = false,
        size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrentClassFunction(EcmaVM *vm,
                                                         InternalFunctionCallback nativeFunc,
                                                         NativePointerCallback deleter,
                                                         void *data,
                                                         bool callNapi = false,
                                                         size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrentClassFunction(EcmaVM *vm,
                                                         const Local<JSValueRef> &context,
                                                         InternalFunctionCallback nativeFunc,
                                                         NativePointerCallback deleter,
                                                         void *data,
                                                         bool callNapi = false,
                                                         size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewConcurrentClassFunctionWithName(const EcmaVM *vm, const Local<JSValueRef> &context,
                                                                 InternalFunctionCallback nativeFunc,
                                                                 NativePointerCallback deleter, const char *name,
                                                                 void *data, bool callNapi = false,
                                                                 size_t propertyCount = 0,
                                                                 size_t staticPropCount = 0,
                                                                 Local<panda::JSValueRef> *keys = nullptr,
                                                                 PropertyAttribute *attrs = nullptr,
                                                                 size_t nativeBindingsize = 0);
                                                                 
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm,
                                               InternalFunctionCallback nativeFunc,
                                               NativePointerCallback deleter,
                                               void *data,
                                               bool callNapi = false,
                                               size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm,
                                               const Local<JSValueRef> &context,
                                               InternalFunctionCallback nativeFunc,
                                               NativePointerCallback deleter,
                                               void *data,
                                               bool callNapi = false,
                                               size_t nativeBindingsize = 0);
    static Local<FunctionRef> NewSendableClassFunction(const EcmaVM *vm,
                                                       InternalFunctionCallback nativeFunc,
                                                       NativePointerCallback deleter,
                                                       void *data,
                                                       Local<StringRef> name,
                                                       SendablePropertiesInfos &infos,
                                                       Local<FunctionRef> parent,
                                                       bool callNapi = false,
                                                       size_t nativeBindingsize = 0);
    JSValueRef* CallForNapi(const EcmaVM *vm, JSValueRef *thisObj, JSValueRef *const argv[],
        int32_t length);
    Local<JSValueRef> Call(const EcmaVM *vm, Local<JSValueRef> thisObj, const Local<JSValueRef> argv[],
        int32_t length);
    Local<JSValueRef> Constructor(const EcmaVM *vm, const Local<JSValueRef> argv[], int32_t length);
    JSValueRef* ConstructorOptimize(const EcmaVM *vm, JSValueRef* argv[], int32_t length);

    Local<JSValueRef> GetFunctionPrototype(const EcmaVM *vm);
    bool Inherit(const EcmaVM *vm, Local<FunctionRef> parent);
    void SetName(const EcmaVM *vm, Local<StringRef> name);
    Local<StringRef> GetName(const EcmaVM *vm);
    Local<StringRef> GetSourceCode(const EcmaVM *vm, int lineNumber);
    bool IsNative(const EcmaVM *vm);
    void SetData(const EcmaVM *vm, void *data, NativePointerCallback deleter = nullptr, bool callNapi = false);
    void* GetData(const EcmaVM *vm);
};

class PUBLIC_API PrimitiveRef : public JSValueRef {
public:
    Local<JSValueRef> GetValue(const EcmaVM *vm);
};

class PUBLIC_API SymbolRef : public PrimitiveRef {
public:
    static Local<SymbolRef> New(const EcmaVM *vm, Local<StringRef> description = Local<StringRef>());
    Local<StringRef> GetDescription(const EcmaVM *vm);
};

class PUBLIC_API BooleanRef : public PrimitiveRef {
public:
    static Local<BooleanRef> New(const EcmaVM *vm, bool input);
    bool Value();
};

class PUBLIC_API StringRef : public PrimitiveRef {
public:
    static inline StringRef *Cast(JSValueRef *value)
    {
        // check
        return static_cast<StringRef *>(value);
    }
    static Local<StringRef> NewFromUtf8WithoutStringTable(const EcmaVM *vm, const char *utf8, int length = -1);
    static Local<StringRef> NewFromUtf8(const EcmaVM *vm, const char *utf8, int length = -1);
    static Local<StringRef> NewFromUtf16WithoutStringTable(const EcmaVM *vm, const char16_t *utf16, int length = -1);
    static Local<StringRef> NewFromUtf16(const EcmaVM *vm, const char16_t *utf16, int length = -1);
    std::string ToString(const EcmaVM *vm);
    std::string DebuggerToString(const EcmaVM *vm);
    uint32_t Length(const EcmaVM *vm);
    bool IsCompressed(const EcmaVM *vm);
    size_t Utf8Length(const EcmaVM *vm, bool isGetBufferSize = false);
    uint32_t WriteUtf8(const EcmaVM *vm, char *buffer, uint32_t length, bool isWriteBuffer = false);
    uint32_t WriteUtf16(const EcmaVM *vm, char16_t *buffer, uint32_t length);
    uint32_t WriteLatin1(const EcmaVM *vm, char *buffer, uint32_t length);
    uint32_t WriteLatin1WithoutSwitchState(const EcmaVM *vm, char *buffer, uint32_t length);
    static Local<StringRef> GetNapiWrapperString(const EcmaVM *vm);
    static Local<StringRef> GetProxyNapiWrapperString(const EcmaVM *vm);
    Local<TypedArrayRef> EncodeIntoUint8Array(const EcmaVM *vm);
};

class PUBLIC_API PromiseRejectInfo {
public:
    enum class PUBLIC_API PROMISE_REJECTION_EVENT : uint32_t { REJECT = 0, HANDLE };
    PromiseRejectInfo(Local<JSValueRef> promise, Local<JSValueRef> reason,
                      PromiseRejectInfo::PROMISE_REJECTION_EVENT operation, void* data);
    ~PromiseRejectInfo() {}
    Local<JSValueRef> GetPromise() const;
    Local<JSValueRef> GetReason() const;
    PromiseRejectInfo::PROMISE_REJECTION_EVENT GetOperation() const;
    void* GetData() const;

private:
    Local<JSValueRef> promise_ {};
    Local<JSValueRef> reason_ {};
    PROMISE_REJECTION_EVENT operation_ = PROMISE_REJECTION_EVENT::REJECT;
    void* data_ {nullptr};
};

/**
 * An external exception handler.
 */
class PUBLIC_API TryCatch {
public:
    explicit TryCatch(const EcmaVM *ecmaVm) : ecmaVm_(ecmaVm) {};

    /**
     * Consumes the exception by default if not rethrow explicitly.
     */
    ~TryCatch();

    bool HasCaught() const;
    void Rethrow();
    Local<ObjectRef> GetAndClearException();
    Local<ObjectRef> GetException();
    void ClearException();

    ECMA_DISALLOW_COPY(TryCatch);
    ECMA_DISALLOW_MOVE(TryCatch);

    bool getrethrow_()
    {
        return rethrow_;
    }

private:
    // Disable dynamic allocation
    void* operator new(size_t size) = delete;
    void operator delete(void*, size_t) = delete;
    void* operator new[](size_t size) = delete;
    void operator delete[](void*, size_t) = delete;

    const EcmaVM *ecmaVm_ {nullptr};
    bool rethrow_ {false};
};

class PUBLIC_API BigIntRef : public PrimitiveRef {
public:
    static Local<BigIntRef> New(const EcmaVM *vm, uint64_t input);
    static Local<BigIntRef> New(const EcmaVM *vm, int64_t input);
    static Local<JSValueRef> CreateBigWords(const EcmaVM *vm, bool sign, uint32_t size, const uint64_t* words);
    void BigIntToInt64(const EcmaVM *vm, int64_t *value, bool *lossless);
    void BigIntToUint64(const EcmaVM *vm, uint64_t *value, bool *lossless);
    void GetWordsArray(const EcmaVM *vm, bool* signBit, size_t wordCount, uint64_t* words);
    uint32_t GetWordsArraySize(const EcmaVM *vm);
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class PUBLIC_API LocalScope {
public:
    explicit LocalScope(const EcmaVM *vm);
    virtual ~LocalScope();

protected:
    inline LocalScope(const EcmaVM *vm, JSTaggedType value);

private:
    void *prevNext_ = nullptr;
    void *prevEnd_ = nullptr;
    int32_t prevHandleStorageIndex_ {-1};
    int32_t prevPrimitiveStorageIndex_ {-1};
    void *prevPrimitiveNext_ = nullptr;
    void *prevPrimitiveEnd_ = nullptr;
    void *thread_ = nullptr;
};

class PUBLIC_API EscapeLocalScope final : public LocalScope {
public:
    explicit EscapeLocalScope(const EcmaVM *vm);
    ~EscapeLocalScope() override = default;

    ECMA_DISALLOW_COPY(EscapeLocalScope);
    ECMA_DISALLOW_MOVE(EscapeLocalScope);

    template<typename T>
    inline Local<T> Escape(Local<T> current)
    {
        ECMA_ASSERT(!alreadyEscape_);
        alreadyEscape_ = true;
        *(reinterpret_cast<T *>(escapeHandle_)) = **current;
        return Local<T>(escapeHandle_);
    }

private:
    bool alreadyEscape_ = false;
    uintptr_t escapeHandle_ = 0U;
};

class PUBLIC_API IntegerRef : public PrimitiveRef {
public:
    static Local<IntegerRef> New(const EcmaVM *vm, int input);
    static Local<IntegerRef> NewFromUnsigned(const EcmaVM *vm, unsigned int input);
    int Value();
};

class PUBLIC_API ArrayBufferRef : public ObjectRef {
public:
    static Local<ArrayBufferRef> New(const EcmaVM *vm, int32_t length);
    static Local<ArrayBufferRef> New(const EcmaVM *vm, void *buffer, int32_t length,
                                     const NativePointerCallback &deleter, void *data);

    int32_t ByteLength(const EcmaVM *vm);
    void *GetBuffer(const EcmaVM *vm);
    void *GetBufferAndLength(const EcmaVM *vm, int32_t *length);
    void Detach(const EcmaVM *vm);
    bool IsDetach(const EcmaVM *vm);
};

class PUBLIC_API SendableArrayBufferRef : public ObjectRef {
public:
    static Local<SendableArrayBufferRef> New(const EcmaVM *vm, int32_t length);
    static Local<SendableArrayBufferRef> New(const EcmaVM *vm, void *buffer, int32_t length,
                                             const NativePointerCallback &deleter, void *data);

    int32_t ByteLength(const EcmaVM *vm);
    void *GetBuffer(const EcmaVM *vm);

    void Detach(const EcmaVM *vm);
    bool IsDetach(const EcmaVM *vm);
};

class PUBLIC_API DateRef : public ObjectRef {
public:
    static Local<DateRef> New(const EcmaVM *vm, double time);
    Local<StringRef> ToString(const EcmaVM *vm);
    double GetTime(const EcmaVM *vm);
};

class PUBLIC_API TypedArrayRef : public ObjectRef {
public:
    uint32_t ByteLength(const EcmaVM *vm);
    uint32_t ByteOffset(const EcmaVM *vm);
    uint32_t ArrayLength(const EcmaVM *vm);
    Local<ArrayBufferRef> GetArrayBuffer(const EcmaVM *vm);
};

class PUBLIC_API SendableTypedArrayRef : public ObjectRef {
public:
    uint32_t ByteLength(const EcmaVM *vm);
    uint32_t ByteOffset(const EcmaVM *vm);
    uint32_t ArrayLength(const EcmaVM *vm);
    Local<SendableArrayBufferRef> GetArrayBuffer(const EcmaVM *vm);
};

class PUBLIC_API ArrayRef : public ObjectRef {
public:
    static Local<ArrayRef> New(const EcmaVM *vm, uint32_t length = 0);
    uint32_t Length(const EcmaVM *vm);
    static bool SetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value);
    static Local<JSValueRef> GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index);
};

class PUBLIC_API SendableArrayRef : public ObjectRef {
public:
    static Local<SendableArrayRef> New(const EcmaVM *vm, uint32_t length = 0);
    uint32_t Length(const EcmaVM *vm);
    static Local<JSValueRef> GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index);
    static bool SetProperty(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value);
};

class PUBLIC_API Int8ArrayRef : public TypedArrayRef {
public:
    static Local<Int8ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API SharedInt8ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedInt8ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                         int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint8ArrayRef : public TypedArrayRef {
public:
    static Local<Uint8ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API SharedUint8ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedUint8ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                          int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint8ClampedArrayRef : public TypedArrayRef {
public:
    static Local<Uint8ClampedArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                           int32_t length);
};

class PUBLIC_API Int16ArrayRef : public TypedArrayRef {
public:
    static Local<Int16ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API SharedInt16ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedInt16ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                          int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint16ArrayRef : public TypedArrayRef {
public:
    static Local<Uint16ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                     int32_t length);
};

class PUBLIC_API SharedUint16ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedUint16ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                           int32_t byteOffset, int32_t length);
};

class PUBLIC_API Int32ArrayRef : public TypedArrayRef {
public:
    static Local<Int32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API SharedInt32ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedInt32ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                          int32_t byteOffset, int32_t length);
};

class PUBLIC_API SharedFloat32ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedFloat32ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                            int32_t byteOffset, int32_t length);
};

class PUBLIC_API SharedUint8ClampedArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedUint8ClampedArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                                 int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint32ArrayRef : public TypedArrayRef {
public:
    static Local<Uint32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                     int32_t length);
};

class PUBLIC_API SharedUint32ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedUint32ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                           int32_t byteOffset, int32_t length);
};

class PUBLIC_API Float32ArrayRef : public TypedArrayRef {
public:
    static Local<Float32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API Float64ArrayRef : public TypedArrayRef {
public:
    static Local<Float64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API SharedFloat64ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedFloat64ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                            int32_t byteOffset, int32_t length);
};

class PUBLIC_API BigInt64ArrayRef : public TypedArrayRef {
public:
    static Local<BigInt64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API SharedBigInt64ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedBigInt64ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                            int32_t byteOffset, int32_t length);
};

class PUBLIC_API BigUint64ArrayRef : public TypedArrayRef {
public:
    static Local<BigUint64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API SharedBigUint64ArrayRef : public SendableTypedArrayRef {
public:
    static Local<SharedBigUint64ArrayRef> New(const EcmaVM *vm, Local<SendableArrayBufferRef> buffer,
                                            int32_t byteOffset, int32_t length);
};

class PUBLIC_API Exception {
public:
    static Local<JSValueRef> Error(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> RangeError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> ReferenceError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> SyntaxError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> TypeError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> AggregateError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> EvalError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> OOMError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> TerminationError(const EcmaVM *vm, Local<StringRef> message);
};

class PUBLIC_API FunctionCallScope {
public:
    FunctionCallScope(EcmaVM *vm);
    ~FunctionCallScope();

private:
    EcmaVM *vm_;
};

class PUBLIC_API JSExecutionScope {
public:
    explicit JSExecutionScope(const EcmaVM *vm);
    ~JSExecutionScope();
    ECMA_DISALLOW_COPY(JSExecutionScope);
    ECMA_DISALLOW_MOVE(JSExecutionScope);

private:
    void *lastCurrentThread_ = nullptr;
    bool isRevert_ = false;
};

class PUBLIC_API JsiNativeScope {
public:
    explicit JsiNativeScope(const EcmaVM *vm);
    ~JsiNativeScope();
    ECMA_DISALLOW_COPY(JsiNativeScope);
    ECMA_DISALLOW_MOVE(JsiNativeScope);

private:
    JSThread *thread_;
    uint16_t oldThreadState_ {0};
    uint32_t isEnableCMCGC_ {0};
    uint32_t hasSwitchState_ {0};
};

class PUBLIC_API JsiFastNativeScope {
public:
    explicit JsiFastNativeScope(const EcmaVM *vm);
    ~JsiFastNativeScope();
    ECMA_DISALLOW_COPY(JsiFastNativeScope);
    ECMA_DISALLOW_MOVE(JsiFastNativeScope);

private:
    JSThread *thread_ {nullptr};
    uint16_t oldThreadState_ {0};
    uint32_t isEnableCMCGC_ {0};
    uint32_t hasSwitchState_ {0};
};

/**
 * JsiRuntimeCallInfo is used for ace_engine and napi, is same to ark EcamRuntimeCallInfo except data.
 */
class PUBLIC_API JsiRuntimeCallInfo
    : public ecmascript::base::AlignedStruct<ecmascript::base::AlignedPointer::Size(),
                                             ecmascript::base::AlignedPointer,
                                             ecmascript::base::AlignedPointer,
                                             ecmascript::base::AlignedPointer> {
    enum class Index : size_t {
        ThreadIndex = 0,
        NumArgsIndex,
        StackArgsIndex,
        NumOfMembers
    };
public:
    JsiRuntimeCallInfo() = default;
    ~JsiRuntimeCallInfo() = default;

    inline JSThread *GetThread() const
    {
        return thread_;
    }

    EcmaVM *GetVM() const;

    inline uint32_t GetArgsNumber() const
    {
        ECMA_ASSERT(numArgs_ >= FIRST_ARGS_INDEX);
        return numArgs_ - FIRST_ARGS_INDEX;
    }

    void* GetData();

    inline Local<JSValueRef> GetFunctionRef() const
    {
        return GetArgRef(FUNC_INDEX);
    }

    inline Local<JSValueRef> GetNewTargetRef() const
    {
        return GetArgRef(NEW_TARGET_INDEX);
    }

    inline Local<JSValueRef> GetThisRef() const
    {
        return GetArgRef(THIS_INDEX);
    }

    inline Local<JSValueRef> GetCallArgRef(uint32_t idx) const
    {
        return GetArgRef(FIRST_ARGS_INDEX + idx);
    }

private:
    enum ArgsIndex : uint8_t { FUNC_INDEX = 0, NEW_TARGET_INDEX, THIS_INDEX, FIRST_ARGS_INDEX };

    Local<JSValueRef> GetArgRef(uint32_t idx) const
    {
        return Local<JSValueRef>(GetArgAddress(idx));
    }

    uintptr_t GetArgAddress(uint32_t idx) const
    {
        if (idx < GetArgsNumber() + FIRST_ARGS_INDEX) {
            return reinterpret_cast<uintptr_t>(&stackArgs_[idx]);
        }
        return 0U;
    }

private:
    alignas(sizeof(JSTaggedType)) JSThread *thread_ {nullptr};
    alignas(sizeof(JSTaggedType))  uint32_t numArgs_ = 0;
    __extension__ alignas(sizeof(JSTaggedType)) JSTaggedType stackArgs_[0];
    friend class FunctionRef;
};

class PUBLIC_API MapRef : public ObjectRef {
public:
    int32_t GetSize(const EcmaVM *vm);
    int32_t GetTotalElements(const EcmaVM *vm);
    Local<JSValueRef> Get(const EcmaVM *vm, Local<JSValueRef> key);
    Local<JSValueRef> Get(const EcmaVM *vm, const char *utf8);
    Local<JSValueRef> GetKey(const EcmaVM *vm, int entry);
    Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);
    static Local<MapRef> New(const EcmaVM *vm);
    void Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value);
    void Set(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value);
    bool Has(const EcmaVM *vm, Local<JSValueRef> key);
    bool Has(const EcmaVM *vm, const char *utf8);
    void Delete(const EcmaVM *vm, Local<JSValueRef> key);
    void Clear(const EcmaVM *vm);
    Local<MapIteratorRef> GetEntries(const EcmaVM *vm);
    Local<MapIteratorRef> GetKeys(const EcmaVM *vm);
    Local<MapIteratorRef> GetValues(const EcmaVM *vm);
};

class PUBLIC_API SendableMapRef : public ObjectRef {
public:
    static Local<SendableMapRef> New(const EcmaVM *vm);
    uint32_t GetSize(const EcmaVM *vm);
    uint32_t GetTotalElements(const EcmaVM *vm);
    Local<JSValueRef> Get(const EcmaVM *vm, Local<JSValueRef> key);
    Local<JSValueRef> Get(const EcmaVM *vm, const char *utf8);
    Local<JSValueRef> GetKey(const EcmaVM *vm, int entry);
    Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);
    void Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value);
    void Set(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value);
    bool Has(const EcmaVM *vm, Local<JSValueRef> key);
    bool Has(const EcmaVM *vm, const char *utf8);
    void Delete(const EcmaVM *vm, Local<JSValueRef> key);
    void Clear(const EcmaVM *vm);
    Local<SendableMapIteratorRef> GetEntries(const EcmaVM *vm);
    Local<SendableMapIteratorRef> GetKeys(const EcmaVM *vm);
    Local<SendableMapIteratorRef> GetValues(const EcmaVM *vm);
};

class PUBLIC_API SendableSetRef : public ObjectRef {
public:
    static Local<SendableSetRef> New(const EcmaVM *vm);
    uint32_t GetSize(const EcmaVM *vm);
    uint32_t GetTotalElements(const EcmaVM *vm);
    Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);
    void Add(const EcmaVM *vm, Local<JSValueRef> value);
};

class PUBLIC_API BufferRef : public ObjectRef {
public:
    static Local<BufferRef> New(const EcmaVM *vm, int32_t length);
    static Local<BufferRef> New(const EcmaVM *vm, const Local<JSValueRef> &context, int32_t length);
    static Local<BufferRef> New(const EcmaVM *vm, void *buffer, int32_t length, const NativePointerCallback &deleter,
                                void *data);
    static Local<BufferRef> New(const EcmaVM *vm, const Local<JSValueRef> &context, void *buffer, int32_t length,
                                const NativePointerCallback &deleter, void *data);

    int32_t ByteLength(const EcmaVM *vm);
    void *GetBuffer(const EcmaVM *vm);
    static ecmascript::JSTaggedValue BufferToStringCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo);
};

class PUBLIC_API PromiseRef : public ObjectRef {
public:
    Local<PromiseRef> Catch(const EcmaVM *vm, Local<FunctionRef> handler);
    Local<PromiseRef> Then(const EcmaVM *vm, Local<FunctionRef> handler);
    Local<PromiseRef> Finally(const EcmaVM *vm, Local<FunctionRef> handler);
    Local<PromiseRef> Then(const EcmaVM *vm, Local<FunctionRef> onFulfilled, Local<FunctionRef> onRejected);

    Local<JSValueRef> GetPromiseState(const EcmaVM *vm);
    Local<JSValueRef> GetPromiseResult(const EcmaVM *vm);
};

class PUBLIC_API PromiseCapabilityRef : public ObjectRef {
public:
    static Local<PromiseCapabilityRef> New(const EcmaVM *vm);
    bool Resolve(const EcmaVM *vm, uintptr_t value);
    bool Resolve(const EcmaVM *vm, Local<JSValueRef> value);
    bool Reject(const EcmaVM *vm, uintptr_t reason);
    bool Reject(const EcmaVM *vm, Local<JSValueRef> reason);
    Local<PromiseRef> GetPromise(const EcmaVM *vm);
};

class PUBLIC_API NumberRef : public PrimitiveRef {
public:
    static Local<NumberRef> New(const EcmaVM *vm, double input);
    static Local<NumberRef> New(const EcmaVM *vm, int32_t input);
    static Local<NumberRef> New(const EcmaVM *vm, uint32_t input);
    static Local<NumberRef> New(const EcmaVM *vm, int64_t input);

    double Value();
};

class PUBLIC_API DataViewRef : public ObjectRef {
public:
    static Local<DataViewRef> New(const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer, uint32_t byteOffset,
                                  uint32_t byteLength);
    static Local<DataViewRef> NewWithoutSwitchState(const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer,
                                                    uint32_t byteOffset, uint32_t byteLength);
    uint32_t ByteLength();
    uint32_t ByteOffset();
    Local<ArrayBufferRef> GetArrayBuffer(const EcmaVM *vm);
};

class PUBLIC_API MapIteratorRef : public ObjectRef {
public:
    int32_t GetIndex();
    Local<JSValueRef> GetKind(const EcmaVM *vm);
    static Local<MapIteratorRef> New(const EcmaVM *vm, Local<MapRef> map);
    ecmascript::EcmaRuntimeCallInfo* GetEcmaRuntimeCallInfo(const EcmaVM *vm);
    static Local<ArrayRef> Next(const EcmaVM *vm, ecmascript::EcmaRuntimeCallInfo* ecmaRuntimeCallInfo);
    Local<JSValueRef> Next(const EcmaVM *vm);
};

class PUBLIC_API SendableMapIteratorRef : public ObjectRef {
public:
    Local<JSValueRef> Next(const EcmaVM *vm);
};

class PUBLIC_API JSNApi {
public:
    struct DebugOption {
        const char *libraryPath;
        bool isDebugMode = false;
        int port = -1;
        bool isFaApp = false;
    };
    using DebuggerPostTask = std::function<void(std::function<void()>&&)>;
    using PatchErrorCode = panda::PatchErrorCode;
    using UncatchableErrorHandler = std::function<void(panda::TryCatch&)>;

    struct NativeBindingInfo {
        static NativeBindingInfo* CreateNewInstance() { return new(std::nothrow) NativeBindingInfo(); }
        void *env = nullptr;
        void *nativeValue = nullptr;
        void *attachFunc = nullptr;
        void *attachData = nullptr;
        void *detachFunc = nullptr;
        void *detachData = nullptr;
        void *hint = nullptr;
        void *detachedFinalizer = nullptr;
        void *detachedHint = nullptr;
    };

    struct XRefBindingInfo {
        static XRefBindingInfo* CreateNewInstance() { return new(std::nothrow) XRefBindingInfo(); }
        void *attachXRefFunc = nullptr;
        void *attachXRefData = nullptr;
    };

    // JSVM
    // fixme: Rename SEMI_GC to YOUNG_GC
    enum class PUBLIC_API TRIGGER_GC_TYPE : uint8_t {
        SEMI_GC,
        OLD_GC,
        FULL_GC,
        SHARED_GC,
        SHARED_FULL_GC
    };

    enum class PUBLIC_API TRIGGER_IDLE_GC_TYPE : uint8_t {
        LOCAL_CONCURRENT_YOUNG_MARK = 1,
        LOCAL_CONCURRENT_FULL_MARK = 1 << 1,
        LOCAL_REMARK = 1 << 2,
        FULL_GC = 1 << 3,
        SHARED_CONCURRENT_MARK = 1 << 4,
        SHARED_CONCURRENT_PARTIAL_MARK = 1 << 5,
        SHARED_FULL_GC = 1 << 6,
    };

    enum class PUBLIC_API MemoryReduceDegree : uint8_t {
        LOW = 0,
        MIDDLE,
        HIGH,
    };

    static EcmaVM *CreateJSVM(const RuntimeOption &option);
    static void DestroyJSVM(EcmaVM *ecmaVm);
    static void SetStackInfo(const EcmaVM *vm, const panda::StackInfo &info);
    static panda::StackInfo GetStackInfo(const EcmaVM *vm);
    static void RegisterUncatchableErrorHandler(EcmaVM *ecmaVm, const UncatchableErrorHandler &handler);

    // aot load
    static void LoadAotFile(EcmaVM *vm, const std::string &moduleName);
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    static void LoadAotFile(EcmaVM *vm, [[maybe_unused]] const std::string &bundleName,
                            const std::string &moduleName,
                            std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb);
#endif
    // JS code
    static bool ExecuteForAbsolutePath(const EcmaVM *vm, const std::string &fileName,
                                       const std::string &entry, bool needUpdate = false,
                                       const ecmascript::ExecuteTypes &executeType = ecmascript::ExecuteTypes::STATIC);
    static bool Execute(const EcmaVM *vm, const std::string &fileName,
                        const std::string &entry, bool needUpdate = false,
                        const ecmascript::ExecuteTypes &executeType = ecmascript::ExecuteTypes::STATIC);
    static bool Execute(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &entry,
                        const std::string &filename = "", bool needUpdate = false,
                        [[maybe_unused]] void* fileMapper = nullptr);
    static int ExecuteWithSingletonPatternFlag(EcmaVM *vm, const std::string &bundleName,
        const std::string &moduleName, const std::string &ohmurl, bool isSingletonPattern);
    static bool IsExecuteModuleInAbcFile(EcmaVM *vm, const std::string &bundleName,
        const std::string &moduleName, const std::string &ohmurl);
    // merge abc, execute module buffer
    static bool ExecuteModuleBuffer(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &filename = "",
                                    bool needUpdate = false);
    static bool ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file);
    static Local<ObjectRef> GetExportObject(EcmaVM *vm, const std::string &file, const std::string &key);
    static Local<ObjectRef> GetExportObjectFromBuffer(EcmaVM *vm, const std::string &file, const std::string &key);
    static Local<ObjectRef> GetExportObjectFromOhmUrl(EcmaVM *vm, const std::string &ohmUrl, const std::string &key);
    static Local<ObjectRef> ExecuteNativeModule(EcmaVM *vm, const std::string &key);
    static Local<ObjectRef> GetModuleNameSpaceFromFile(EcmaVM *vm, const std::string &file);
    template<ForHybridApp isHybrid = ForHybridApp::Normal>
    static Local<ObjectRef> GetModuleNameSpaceWithModuleInfo(EcmaVM *vm, const std::string &file,
                                                             const std::string &module_path);
    static Local<ObjectRef> GetModuleNameSpaceWithModuleInfoForNormalApp(EcmaVM *vm, const std::string &file,
                                                             const std::string &module_path);
    static Local<ObjectRef> GetModuleNameSpaceWithModuleInfoForHybridApp(EcmaVM *vm, const std::string &file,
                                                             const std::string &module_path);
    static Local<ObjectRef> GetModuleNameSpaceWithPath(const EcmaVM *vm, const char *path);
    static std::pair<std::string, std::string> ResolveOhmUrl(std::string ohmUrl);

    /*
     * Execute panda file from secure mem. secure memory lifecycle managed externally.
     * The data parameter needs to be created externally by an external caller and managed externally
     * by the external caller. The size parameter is the size of the data memory. The entry parameter
     * is the name of the entry function. The filename parameter is used to uniquely identify this
     * memory internally.
     */
    static bool ExecuteSecure(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &entry,
                              const std::string &filename = "", bool needUpdate = false, void* fileMapper = nullptr);
    /*
     * Execute panda file(merge abc) from secure mem. secure memory lifecycle managed externally.
     * The data parameter needs to be created externally by an external caller and managed externally
     * by the external caller. The size parameter is the size of the data memory. The filename parameter
     * is used to uniquely identify this memory internally.
     */
    static bool ExecuteModuleBufferSecure(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &filename = "",
                                          bool needUpdate = false);

    static bool ExecuteSecureWithOhmUrl(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &srcFilename,
                                        const std::string &ohmUrl);

    // ObjectRef Operation
    static Local<ObjectRef> GetGlobalObject(const EcmaVM *vm);
    static Local<ObjectRef> GetGlobalObject(const EcmaVM *vm, const Local<JSValueRef> &context);
    static void ExecutePendingJob(const EcmaVM *vm);

    // Memory
    // fixme: Rename SEMI_GC to YOUNG_GC
    static void TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType = TRIGGER_GC_TYPE::SEMI_GC);
    static void TriggerGC(const EcmaVM *vm, ecmascript::GCReason reason,
        TRIGGER_GC_TYPE gcType = TRIGGER_GC_TYPE::SEMI_GC);
    static void HintGC(const EcmaVM *vm, MemoryReduceDegree degree, ecmascript::GCReason reason);
    static void TriggerIdleGC(const EcmaVM *vm, TRIGGER_IDLE_GC_TYPE gcType);
    static void SetStartIdleMonitorCallback(const StartIdleMonitorCallback& callback);
    static StartIdleMonitorCallback GetStartIdleMonitorCallback();
    // Exception
    static void ThrowException(const EcmaVM *vm, Local<JSValueRef> error);
    static void PrintExceptionInfo(const EcmaVM *vm);
    static void SetOnErrorCallback(EcmaVM *vm, OnErrorCallback cb, void* data);
    static Local<ObjectRef> GetAndClearUncaughtException(const EcmaVM *vm);
    static Local<ObjectRef> GetUncaughtException(const EcmaVM *vm);
    static bool IsExecutingPendingJob(const EcmaVM *vm);
    static bool HasPendingException(const EcmaVM *vm);
    static bool HasPendingJob(const EcmaVM *vm);
    static void EnableUserUncaughtErrorHandler(EcmaVM *vm);
    // prevewer debugger.
    static bool StartDebuggerCheckParameters(EcmaVM *vm, const DebugOption &option, int32_t instanceId,
                                             const DebuggerPostTask &debuggerPostTask);
    static bool StartDebugger(EcmaVM *vm, const DebugOption &option, int32_t instanceId = 0,
        const DebuggerPostTask &debuggerPostTask = {});
    // To be compatible with the old process.
    static bool StartDebuggerForOldProcess(EcmaVM *vm, const DebugOption &option, int32_t instanceId = 0,
        const DebuggerPostTask &debuggerPostTask = {});
    // socketpair process in ohos platform.
    static bool StartDebuggerForSocketPair(int tid, int socketfd = -1);
    static bool StopDebugger(int tid);
    static bool NotifyDebugMode(int tid, EcmaVM *vm, const DebugOption &option, int32_t instanceId = 0,
                                const DebuggerPostTask &debuggerPostTask = {}, bool debugApp = false);
    static bool StoreDebugInfo(
        int tid, EcmaVM *vm, const DebugOption &option, const DebuggerPostTask &debuggerPostTask, bool debugApp);
    static bool StopDebugger(EcmaVM *vm);
    static bool IsMixedDebugEnabled(const EcmaVM *vm);
    static bool IsDebugModeEnabled(const EcmaVM *vm);
    static void NotifyNativeCalling(const EcmaVM *vm, const void *nativeAddress);
    static void NotifyNativeReturn(const EcmaVM *vm, const void *nativeAddress);
    static void NotifyLoadModule(const EcmaVM *vm);
    static void NotifyUIIdle(const EcmaVM *vm, int idleTime);
    static bool NotifyLooperIdleStart(const EcmaVM *vm, int64_t timestamp, int idleTime);
    static void NotifyLooperIdleEnd(const EcmaVM *vm, int64_t timestamp);
    static bool IsJSMainThreadOfEcmaVM(const EcmaVM *vm);
    static void SetDeviceDisconnectCallback(EcmaVM *vm, DeviceDisconnectCallback cb);
    // Serialize & Deserialize.
    static void* SerializeValue(const EcmaVM *vm, Local<JSValueRef> data, Local<JSValueRef> transfer,
                                Local<JSValueRef> cloneList,
                                bool defaultTransfer = false,
                                bool defaultCloneShared = true);
    static void* SerializeValueWithError(const EcmaVM *vm, Local<JSValueRef> data, Local<JSValueRef> transfer,
                                         Local<JSValueRef> cloneList, std::string &error, bool defaultTransfer = false,
                                         bool defaultCloneShared = true);
    static Local<JSValueRef> DeserializeValue(const EcmaVM *vm, void *recoder, void *hint);
    // InterOp Serialize & Deserialize.
    static void* InterOpSerializeValue(const EcmaVM *vm, Local<JSValueRef> data, Local<JSValueRef> transfer,
        Local<JSValueRef> cloneList, bool defaultTransfer = false, bool defaultCloneShared = true);
    static Local<JSValueRef> InterOpDeserializeValue(const EcmaVM *vm, void *recoder, void *hint);
    static void DeleteSerializationData(void *data);
    static void SetHostPromiseRejectionTracker(EcmaVM *vm, void *cb, void* data);
    static void SetTimerTaskCallback(EcmaVM *vm, TimerTaskCallback callback);
    static void SetCancelTimerCallback(EcmaVM *vm, CancelTimerCallback callback);
    static void NotifyEnvInitialized(EcmaVM *vm);
    static void SetReleaseSecureMemCallback(ReleaseSecureMemCallback releaseSecureMemFunc);
    static void SetHostResolveBufferTracker(EcmaVM *vm, std::function<bool(std::string dirPath,
                                            uint8_t **buff, size_t *buffSize, std::string &errorMsg)> cb);
    static void PandaFileSerialize(const EcmaVM *vm);
    static void ModuleSerialize(const EcmaVM *vm);
    static void ModuleDeserialize(EcmaVM *vm, const uint32_t appVersion);
    static void SetHostResolveBufferTrackerForHybridApp(EcmaVM *vm, std::function<bool(std::string dirPath,
                                            uint8_t **buff, size_t *buffSize, std::string &errorMsg)> cb);
    static void SetUnloadNativeModuleCallback(EcmaVM *vm, const std::function<bool(const std::string &moduleKey)> &cb);
    static void SetNativePtrGetter(EcmaVM *vm, void* cb);
    static void SetSourceMapCallback(EcmaVM *vm, SourceMapCallback cb);
    static void SetSourceMapTranslateCallback(EcmaVM *vm, SourceMapTranslateCallback cb);
    static void SetHostEnqueueJob(const EcmaVM* vm, Local<JSValueRef> cb,
                                QueueType queueType = QueueType::QUEUE_PROMISE);
    static EcmaVM* CreateEcmaVM(const ecmascript::JSRuntimeOptions &options);
    static void PreFork(EcmaVM *vm);
    static void PostFork(EcmaVM *vm, const RuntimeOption &option);
    static void AddWorker(EcmaVM *hostVm, EcmaVM *workerVm);
    static bool DeleteWorker(EcmaVM *hostVm, EcmaVM *workerVm);
    static void GetStackBeforeCallNapiSuccess(EcmaVM *vm, bool &getStackBeforeCallNapiSuccess);
    static void GetStackAfterCallNapi(EcmaVM *vm);
    static PatchErrorCode LoadPatch(EcmaVM *vm, const std::string &patchFileName, const std::string &baseFileName);
    static PatchErrorCode LoadPatch(EcmaVM *vm,
                                    const std::string &patchFileName, uint8_t *patchBuffer, size_t patchSize,
                                    const std::string &baseFileName, uint8_t *baseBuffer, size_t baseSize);
    static PatchErrorCode UnloadPatch(EcmaVM *vm, const std::string &patchFileName);
    // check whether the exception is caused by quickfix methods.
    static bool IsQuickFixCausedException(EcmaVM *vm, Local<ObjectRef> exception, const std::string &patchFileName);
    // register quickfix query function.
    static void RegisterQuickFixQueryFunc(EcmaVM *vm, std::function<bool(std::string baseFileName,
                        std::string &patchFileName,
                        uint8_t **patchBuffer,
                        size_t &patchSize)> callBack);
    static bool IsBundle(EcmaVM *vm);
    static void SetBundle(EcmaVM *vm, bool value);
    static bool IsNormalizedOhmUrlPack(EcmaVM *vm);
    static bool IsOhmUrl(const std::string &srcName);
    static void SetAssetPath(EcmaVM *vm, const std::string &assetPath);
    static void SetMockModuleList(EcmaVM *vm, const std::map<std::string, std::string> &list);
    static void SetPkgNameList(EcmaVM *vm, const std::map<std::string, std::string> &list);
    static void UpdatePkgNameList(EcmaVM *vm, const std::map<std::string, std::string> &list);
    static std::string GetPkgName(EcmaVM *vm, const std::string &moduleName);
    static void SetPkgAliasList(EcmaVM *vm, const std::map<std::string, std::string> &list);
    static void UpdatePkgAliasList(EcmaVM *vm, const std::map<std::string, std::string> &list);
    static void SetHmsModuleList(EcmaVM *vm, const std::vector<panda::HmsMap> &list);
    static void SetModuleInfo(EcmaVM *vm, const std::string &assetPath, const std::string &entryPoint);
    static void SetpkgContextInfoList(EcmaVM *vm, const std::map<std::string,
        std::vector<std::vector<std::string>>> &list);
    static void UpdatePkgContextInfoList(EcmaVM *vm,
        const std::map<std::string, std::vector<std::vector<std::string>>> &list);
    static void SetExecuteBufferMode(const EcmaVM *vm);
    // Stop preloading so task callback.
    static void SetStopPreLoadSoCallback(EcmaVM *vm, const StopPreLoadSoCallback &callback);
    static void SetLoop(EcmaVM *vm, void *loop);
    static void SetWeakFinalizeTaskCallback(EcmaVM *vm, const WeakFinalizeTaskCallback &callback);
    static void SetAsyncCleanTaskCallback(EcmaVM *vm, const NativePointerTaskCallback &callback);
    static void SetTriggerGCTaskCallback(EcmaVM *vm, const TriggerGCTaskCallback& callback);
    static void SetStartIdleMonitorCallback(EcmaVM *vm, const StartIdleMonitorCallback& callback);
    static std::string GetAssetPath(EcmaVM *vm);
    static bool InitForConcurrentThread(EcmaVM *vm, ConcurrentCallback cb, void *data);
    static bool InitForConcurrentFunction(EcmaVM *vm, Local<JSValueRef> func, void *taskInfo);
    static void* GetCurrentTaskInfo(const EcmaVM *vm);
    static void ClearCurrentTaskInfo(const EcmaVM *vm);
    static void SetBundleName(EcmaVM *vm, const std::string &bundleName);
    static std::string GetBundleName(EcmaVM *vm);
    static void SetModuleName(EcmaVM *vm, const std::string &moduleName);
    static std::string GetModuleName(EcmaVM *vm);
    static void SetLargeHeap(bool isLargeHeap);
    static std::pair<std::string, std::string> GetCurrentModuleInfo(EcmaVM *vm, bool needRecordName = false);
    static std::string NormalizePath(const std::string &string);
    static void AllowCrossThreadExecution(EcmaVM *vm);
    static bool CheckAndSetAllowCrossThreadExecution(EcmaVM *vm);
    static void SynchronizVMInfo(EcmaVM *vm, const EcmaVM *hostVM);
    static bool IsProfiling(EcmaVM *vm);
    static void SetProfilerState(const EcmaVM *vm, bool value);
    static void SetRequestAotCallback(EcmaVM *vm, const std::function<int32_t(const std::string &bundleName,
                    const std::string &moduleName,
                    int32_t triggerMode)> &cb);
    static void SetSearchHapPathTracker(EcmaVM *vm, std::function<bool(const std::string moduleName,
                    std::string &hapPath)> cb);
    static void *GetEnv(EcmaVM *vm);
    static void SetEnv(EcmaVM *vm, void *env);
    static void SetMultiThreadCheck(bool multiThreadCheck = true);
    static void SetErrorInfoEnhance(bool errorInfoEnhance = true);

    // Napi Heavy Logics fast path
    static Local<JSValueRef> NapiHasProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key);
    static Local<JSValueRef> NapiHasOwnProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key);
    static Local<JSValueRef> NapiGetProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key);
    static Local<JSValueRef> NapiDeleteProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key);
    static Local<JSValueRef> NapiGetNamedProperty(const EcmaVM *vm, uintptr_t nativeObj, const char* utf8Key);
    static Local<JSValueRef> CreateLocal(const EcmaVM *vm, JSValueRef src);

    // Napi helper function
    static bool KeyIsNumber(const char* utf8);
    static int GetStartRealTime(const EcmaVM *vm);
    static void NotifyTaskBegin(const EcmaVM *vm);
    static void NotifyTaskFinished(const EcmaVM *vm);
    static bool IsMultiThreadCheckEnabled(const EcmaVM *vm);
    static uint32_t GetCurrentThreadId();
    static bool IsObjectAlive(const EcmaVM *vm, uintptr_t addr);
    static bool IsValidHeapObject(const EcmaVM *vm, uintptr_t addr);

    //set VM apiVersion
    static void SetVMAPIVersion(EcmaVM *vm, const int32_t apiVersion);

    // Napi Update SubStackInfo
    static void UpdateStackInfo(EcmaVM *vm, void *currentStackInfo, uint32_t opKind);

    static Local<JSValueRef> CreateContext(const EcmaVM *vm);

    static Local<JSValueRef> GetCurrentContext(const EcmaVM *vm);

    static void SwitchContext(const EcmaVM *vm, const Local<JSValueRef> &context);
    // 1.2runtime interface info
    static Local<JSValueRef> GetImplements(const EcmaVM *vm, Local<JSValueRef> instance);
    static void InitHybridVMEnv(const EcmaVM *vm);

private:
    static bool isForked_;
    static bool CreateRuntime(const RuntimeOption &option);
    static bool DestroyRuntime();
    static StartIdleMonitorCallback startIdleMonitorCallback_;

    static uintptr_t GetHandleAddr(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t GetGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t GetXRefGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t SetWeak(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t SetWeakCallback(const EcmaVM *vm, uintptr_t localAddress, void *ref,
                                     WeakRefClearCallBack freeGlobalCallBack,
                                     WeakRefClearCallBack nativeFinalizeCallback);
    static uintptr_t ClearWeak(const EcmaVM *vm, uintptr_t localAddress);
    static bool IsWeak(const EcmaVM *vm, uintptr_t localAddress);
    static void DisposeGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr);
    static void DisposeXRefGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr);
#ifdef PANDA_JS_ETS_HYBRID_MODE
    static void MarkFromObject(const EcmaVM *vm, uintptr_t addr);
#endif // PANDA_JS_ETS_HYBRID_MODE

    static bool IsSerializationTimeoutCheckEnabled(const EcmaVM *vm);
    static void GenerateTimeoutTraceIfNeeded(const EcmaVM *vm, std::chrono::system_clock::time_point &start,
                                     std::chrono::system_clock::time_point &end, bool isSerialization);
    static void UpdateAOTCompileStatus(ecmascript::JSRuntimeOptions &jsOption, const RuntimeOption &option);
    template<typename T>
    friend class Global;
    template<typename T>
    friend class CopyableGlobal;
    template<typename T>
    friend class Local;
    friend class test::JSNApiTests;
};

class PUBLIC_API ProxyRef : public ObjectRef {
public:
    Local<JSValueRef> GetHandler(const EcmaVM *vm);
    Local<JSValueRef> GetTarget(const EcmaVM *vm);
    bool IsRevoked();
};

class PUBLIC_API WeakMapRef : public ObjectRef {
public:
    int32_t GetSize(const EcmaVM *vm);
    int32_t GetTotalElements(const EcmaVM *vm);
    Local<JSValueRef> GetKey(const EcmaVM *vm, int entry);
    Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);
    static Local<WeakMapRef> New(const EcmaVM *vm);
    void Set(const EcmaVM *vm, const Local<JSValueRef> &key, const Local<JSValueRef> &value);
    bool Has(const EcmaVM *vm, Local<JSValueRef> key);
};

class PUBLIC_API SetRef : public ObjectRef {
public:
    int32_t GetSize(const EcmaVM *vm);
    int32_t GetTotalElements(const EcmaVM *vm);
    Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);
    static Local<SetRef> New(const EcmaVM *vm);
    void Add(const EcmaVM *vm, Local<JSValueRef> value);
};

class PUBLIC_API WeakSetRef : public ObjectRef {
public:
    int32_t GetSize(const EcmaVM *vm);
    int32_t GetTotalElements(const EcmaVM *vm);
    Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);
    static Local<WeakSetRef> New(const EcmaVM *vm);
    void Add(const EcmaVM *vm, Local<JSValueRef> value);
};

class PUBLIC_API SetIteratorRef : public ObjectRef {
public:
    int32_t GetIndex();
    Local<JSValueRef> GetKind(const EcmaVM *vm);
    static Local<SetIteratorRef> New(const EcmaVM *vm, Local<SetRef> set);
    ecmascript::EcmaRuntimeCallInfo *GetEcmaRuntimeCallInfo(const EcmaVM *vm);
    static Local<ArrayRef> Next(const EcmaVM *vm, ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo);
};

/* Attention pls, ExternalStringCache only can be utilized in main thread. Threads of Worker or Taskpool call
 * functions of this class will cause data race.
 */
class PUBLIC_API ExternalStringCache final {
public:
    static bool RegisterStringCacheTable(const EcmaVM *vm, uint32_t size);
    static bool SetCachedString(const EcmaVM *vm, const char *name, uint32_t propertyIndex);
    static bool HasCachedString(const EcmaVM *vm, uint32_t propertyIndex);
    static Local<StringRef> GetCachedString(const EcmaVM *vm, uint32_t propertyIndex);
};

template<typename T>
template<typename S>
Global<T>::Global(const EcmaVM *vm, const Local<S> &current) : vm_(vm)
{
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
template<typename S>
Global<T>::Global(const EcmaVM *vm, const Global<S> &current) : vm_(vm)
{
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
CopyableGlobal<T>::CopyableGlobal(const EcmaVM *vm, const Local<T> &current) : vm_(vm)
{
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
template<typename S>
CopyableGlobal<T>::CopyableGlobal(const EcmaVM *vm, const Local<S> &current) : vm_(vm)
{
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
void CopyableGlobal<T>::Copy(const CopyableGlobal &that)
{
    Free();
    vm_ = that.vm_;
    if (!that.IsEmpty()) {
        ECMA_ASSERT(vm_ != nullptr);
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*that));
    }
}

template<typename T>
template<typename S>
void CopyableGlobal<T>::Copy(const CopyableGlobal<S> &that)
{
    Free();
    vm_ = that.GetEcmaVM();
    if (!that.IsEmpty()) {
        ECMA_ASSERT(vm_ != nullptr);
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*that));
    }
}

template<typename T>
void CopyableGlobal<T>::Move(CopyableGlobal &that)
{
    Free();
    vm_ = that.vm_;
    address_ = that.address_;
    that.vm_ = nullptr;
    that.address_ = 0U;
}

template<typename T>
inline void CopyableGlobal<T>::Free()
{
    if (!IsEmpty()) {
        JSNApi::DisposeGlobalHandleAddr(vm_, address_);
        address_ = 0U;
    }
}

template <typename T>
void CopyableGlobal<T>::SetWeakCallback(void *ref, WeakRefClearCallBack freeGlobalCallBack,
                                        WeakRefClearCallBack nativeFinalizeCallback)
{
    address_ = JSNApi::SetWeakCallback(vm_, address_, ref, freeGlobalCallBack, nativeFinalizeCallback);
}

template<typename T>
void CopyableGlobal<T>::SetWeak()
{
    address_ = JSNApi::SetWeak(vm_, address_);
}

template<typename T>
void CopyableGlobal<T>::ClearWeak()
{
    address_ = JSNApi::ClearWeak(vm_, address_);
}

template<typename T>
bool CopyableGlobal<T>::IsWeak() const
{
    return JSNApi::IsWeak(vm_, address_);
}

template<typename T>
void Global<T>::Update(const Global &that)
{
    if (address_ != 0) {
        JSNApi::DisposeGlobalHandleAddr(vm_, address_);
    }
    address_ = that.address_;
    vm_ = that.vm_;
}

template<typename T>
void Global<T>::FreeGlobalHandleAddr()
{
    if (address_ == 0) {
        return;
    }
    JSNApi::DisposeGlobalHandleAddr(vm_, address_);
    address_ = 0;
}

template<typename T>
void Global<T>::SetWeak()
{
    address_ = JSNApi::SetWeak(vm_, address_);
}

template <typename T>
void Global<T>::SetWeakCallback(void *ref, WeakRefClearCallBack freeGlobalCallBack,
                                WeakRefClearCallBack nativeFinalizeCallback)
{
    address_ = JSNApi::SetWeakCallback(vm_, address_, ref, freeGlobalCallBack, nativeFinalizeCallback);
}

template<typename T>
void Global<T>::ClearWeak()
{
    address_ = JSNApi::ClearWeak(vm_, address_);
}

template<typename T>
bool Global<T>::IsWeak() const
{
    return JSNApi::IsWeak(vm_, address_);
}

// ---------------------------------- Local --------------------------------------------
template<typename T>
Local<T>::Local(const EcmaVM *vm, const CopyableGlobal<T> &current)
{
    address_ = JSNApi::GetHandleAddr(vm, reinterpret_cast<uintptr_t>(*current));
}

template<typename T>
Local<T>::Local(const EcmaVM *vm, const Global<T> &current)
{
    address_ = JSNApi::GetHandleAddr(vm, reinterpret_cast<uintptr_t>(*current));
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
class PUBLIC_API HandshakeHelper final {
   public:
    static void DoHandshake(EcmaVM *vm, void *stsiface, void **ecmaiface);
};
#endif  // PANDA_JS_ETS_HYBRID_MODE

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
#endif
