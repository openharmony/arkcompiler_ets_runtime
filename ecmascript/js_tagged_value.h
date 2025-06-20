/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_TAGGED_VALUE_H
#define ECMASCRIPT_JS_TAGGED_VALUE_H

#include "ecmascript/base/bit_helper.h"
#include "ecmascript/base/config.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/js_tagged_value_internals.h"

namespace panda::ecmascript {
class JSArray;
class JSObject;
class JSTaggedNumber;
template<typename T>
class JSHandle;
class TaggedArray;
class LinkedHashMap;
class LinkedHashSet;
class PropertyDescriptor;
class OperationResult;
class EcmaString;
class JSThread;
struct Reference;

namespace JSShared {
// check mode for js shared
enum SCheckMode: uint8_t {
    SKIP = 0,
    CHECK
};
} // namespace JSShared

using SCheckMode = JSShared::SCheckMode;

static constexpr double SAFE_NUMBER = 9007199254740991LL;

// Don't switch the order!
enum PreferredPrimitiveType : uint8_t { PREFER_NUMBER = 0, PREFER_STRING, NO_PREFERENCE };

// Result of an abstract relational comparison of x and y, implemented according
// to ES6 section 7.2.11 Abstract Relational Comparison.
enum class ComparisonResult {
    LESS,      // x < y
    EQUAL,     // x = y
    GREAT,     // x > y
    UNDEFINED  // at least one of x or y was undefined or NaN
};

enum class ClassKind : uint8_t { SENDABLE = 0, NON_SENDABLE };
class JSTaggedValue : public JSTaggedValueInternals {
public:
    static JSTaggedValue Cast(TaggedObject *object)
    {
        return JSTaggedValue(object);
    }

    static const JSTaggedType NULL_POINTER = VALUE_HOLE;
    static const JSTaggedType INVALID_VALUE_LIMIT = 0x40000ULL;

    static inline JSTaggedType CastDoubleToTagged(double value)
    {
        return base::bit_cast<JSTaggedType>(value);
    }

    static inline double CastTaggedToDouble(JSTaggedType value)
    {
        return base::bit_cast<double>(value);
    }

    static inline constexpr size_t TaggedTypeSize()
    {
        return sizeof(JSTaggedType);
    }

    static JSHandle<JSTaggedValue> PublishSharedValue(JSThread *thread, JSHandle<JSTaggedValue> value);

    static JSHandle<JSTaggedValue> PublishSharedValueSlow(JSThread *thread, JSHandle<JSTaggedValue> value);

    static constexpr size_t SizeArch32 = sizeof(JSTaggedType);

    static constexpr size_t SizeArch64 = sizeof(JSTaggedType);

    explicit JSTaggedValue(void *) = delete;

    ARK_INLINE constexpr JSTaggedValue() : value_(JSTaggedValue::NULL_POINTER) {}

    ARK_INLINE constexpr explicit JSTaggedValue(JSTaggedType v) : value_(v) {}

    ARK_INLINE constexpr explicit JSTaggedValue(int v) : value_(static_cast<JSTaggedType>(v) | TAG_INT) {}

    ARK_INLINE explicit JSTaggedValue(unsigned int v)
    {
        if (static_cast<int32_t>(v) < 0) {
            value_ = JSTaggedValue(static_cast<double>(v)).GetRawData();
            return;
        }
        value_ = JSTaggedValue(static_cast<int32_t>(v)).GetRawData();
    }

    ARK_INLINE constexpr explicit JSTaggedValue(bool v)
        : value_(static_cast<JSTaggedType>(v) | TAG_BOOLEAN_MASK) {}

    ARK_INLINE explicit JSTaggedValue(double v)
    {
        ASSERT_PRINT(!IsImpureNaN(v), "pureNaN will break the encoding of tagged double: "
                                          << std::hex << CastDoubleToTagged(v));
        value_ = CastDoubleToTagged(v) + DOUBLE_ENCODE_OFFSET;
    }

    ARK_INLINE explicit JSTaggedValue(const TaggedObject *v) : value_(static_cast<JSTaggedType>(ToUintPtr(v))) {}

    ARK_INLINE explicit JSTaggedValue(int64_t v)
    {
        if (UNLIKELY(static_cast<int32_t>(v) != v)) {
            value_ = JSTaggedValue(static_cast<double>(v)).GetRawData();
            return;
        }
        value_ = JSTaggedValue(static_cast<int32_t>(v)).GetRawData();
    }

    ~JSTaggedValue() = default;
    DEFAULT_COPY_SEMANTIC(JSTaggedValue);
    DEFAULT_MOVE_SEMANTIC(JSTaggedValue);

    inline void CreateWeakRef()
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == 0U),
                     "The least significant two bits of JSTaggedValue are not zero.");
        value_ = value_ | TAG_WEAK;
    }

    inline void RemoveWeakTag()
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == TAG_WEAK), "The tagged value is not a weak ref.");
        value_ = value_ & (~TAG_WEAK);
    }

    inline JSTaggedValue CreateAndGetWeakRef() const
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == 0U),
                     "The least significant two bits of JSTaggedValue are not zero.");
        return JSTaggedValue(value_ | TAG_WEAK);
    }

    inline JSTaggedValue GetWeakRawValue() const
    {
        if (IsHeapObject()) {
            return JSTaggedValue(value_ & (~TAG_WEAK));
        }
        return JSTaggedValue(value_);
    }

    ARK_INLINE bool IsWeak() const
    {
        return ((value_ & TAG_WEAK_MASK) == TAG_WEAK);
    }

    ARK_INLINE bool IsDouble() const
    {
        return !IsInt() && !IsObject();
    }

    ARK_INLINE bool IsInt() const
    {
        return (value_ & TAG_MARK) == TAG_INT;
    }

    ARK_INLINE bool IsSpecial() const
    {
        return ((value_ & TAG_SPECIAL_MASK) == TAG_SPECIAL) || IsHole();
    }

    ARK_INLINE bool IsObject() const
    {
        return ((value_ & TAG_MARK) == TAG_OBJECT);
    }

    ARK_INLINE TaggedObject *GetWeakReferentUnChecked() const
    {
        return reinterpret_cast<TaggedObject *>(value_ & (~TAG_WEAK));
    }

    ARK_INLINE bool IsHeapObject() const
    {
        return ((value_ & TAG_HEAPOBJECT_MASK) == 0U);
    }

    ARK_INLINE bool IsInvalidValue() const
    {
        return value_ <= INVALID_VALUE_LIMIT;
    }

    ARK_INLINE bool HasReadBarrierDFXTag() const
    {
#if defined(ENABLE_CMC_RB_DFX)
        if (g_isEnableCMCGC) {
            return (value_ & TAG_READ_BARRIER_DFX_MASK) == TAG_READ_BARRIER_DFX;
        } else {
            return false;
        }
#else
        return false;
#endif
    }

    inline void AddReadBarrierDFXTag()
    {
#if defined(ENABLE_CMC_RB_DFX)
        if (g_isEnableCMCGC) {
            if (IsHeapObject()) {
                value_ |= TAG_READ_BARRIER_DFX;
            }
        }
#endif
    }

    inline void RemoveReadBarrierDFXTag()
    {
#if defined(ENABLE_CMC_RB_DFX)
        if (g_isEnableCMCGC) {
            value_ &= ~TAG_READ_BARRIER_DFX;
        }
#endif
    }

    ARK_INLINE double GetDouble() const
    {
        ASSERT_PRINT(IsDouble(), "can not convert JSTaggedValue to Double : " << std::hex << value_);
        return CastTaggedToDouble(value_ - DOUBLE_ENCODE_OFFSET);
    }

    ARK_INLINE int GetInt() const
    {
        ASSERT_PRINT(IsInt(), "can not convert JSTaggedValue to Int :" << std::hex << value_);
        return static_cast<int>(value_ & (~TAG_MARK));
    }

    ARK_INLINE uint64_t GetLargeUInt() const
    {
        ASSERT_PRINT(IsInt(), "can not convert JSTaggedValue to Int :" << std::hex << value_);
        return (value_ & (~TAG_MARK));
    }

    ARK_INLINE JSTaggedType GetRawData() const
    {
        return value_;
    }

    //  This function returns the heap object pointer which may have the weak tag.
    ARK_INLINE TaggedObject *GetRawHeapObject() const
    {
        ASSERT_PRINT(IsHeapObject(), "can not convert JSTaggedValue to HeapObject :" << std::hex << value_);
        return reinterpret_cast<TaggedObject *>(value_);
    }

    ARK_INLINE TaggedObject *GetWeakReferent() const
    {
        ASSERT_PRINT(IsWeak(), "can not convert JSTaggedValue to WeakRef HeapObject :" << std::hex << value_);
        return reinterpret_cast<TaggedObject *>(value_ & (~TAG_WEAK));
    }

    static ARK_INLINE JSTaggedType Cast(void *ptr)
    {
        ASSERT_PRINT(sizeof(void *) == TaggedTypeSize(), "32bit platform is not support yet");
        return static_cast<JSTaggedType>(ToUintPtr(ptr));
    }

    ARK_INLINE bool IsFalse() const
    {
        return value_ == VALUE_FALSE;
    }

    ARK_INLINE bool IsTrue() const
    {
        return value_ == VALUE_TRUE;
    }

    ARK_INLINE bool IsUndefined() const
    {
        return value_ == VALUE_UNDEFINED;
    }

    ARK_INLINE bool IsNull() const
    {
        return value_ == VALUE_NULL;
    }

    ARK_INLINE bool IsUndefinedOrNull() const
    {
        return ((value_ & TAG_HEAPOBJECT_MASK) == TAG_SPECIAL);
    }

    ARK_INLINE bool IsHole() const
    {
        return value_ == VALUE_HOLE || value_ == 0U;
    }

    ARK_INLINE bool IsException() const
    {
        return value_ == VALUE_EXCEPTION;
    }

    ARK_INLINE bool IsNaN() const
    {
        if (!IsDouble()) {
            return false;
        }
        double untagged = GetDouble();
        return std::isnan(untagged);
    }

    static ARK_INLINE bool IsImpureNaN(double value)
    {
        // Tests if the double value would break tagged double encoding.
        return base::bit_cast<JSTaggedType>(value) >= (TAG_INT - DOUBLE_ENCODE_OFFSET);
    }

    ARK_INLINE bool operator==(const JSTaggedValue &other) const
    {
        return value_ == other.value_;
    }

    ARK_INLINE bool operator!=(const JSTaggedValue &other) const
    {
        return value_ != other.value_;
    }

    ARK_INLINE bool IsWeakForHeapObject() const
    {
        return (value_ & TAG_WEAK) == 1U;
    }

    static ARK_INLINE constexpr JSTaggedValue False()
    {
        return JSTaggedValue(VALUE_FALSE);
    }

    static ARK_INLINE constexpr JSTaggedValue True()
    {
        return JSTaggedValue(VALUE_TRUE);
    }

    static ARK_INLINE constexpr JSTaggedValue Undefined()
    {
        return JSTaggedValue(VALUE_UNDEFINED);
    }

    static ARK_INLINE constexpr JSTaggedValue Null()
    {
        return JSTaggedValue(VALUE_NULL);
    }

    static ARK_INLINE constexpr JSTaggedValue Hole()
    {
        return JSTaggedValue(VALUE_HOLE);
    }

    static ARK_INLINE constexpr JSTaggedValue Exception()
    {
        return JSTaggedValue(VALUE_EXCEPTION);
    }

    ARK_INLINE double GetNumber() const
    {
        return IsInt() ? GetInt() : GetDouble();
    }

    ARK_INLINE TaggedObject *GetTaggedObject() const
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == 0U),
                     "can not convert JSTaggedValue to HeapObject :" << std::hex << value_);
        return reinterpret_cast<TaggedObject *>(value_);
    }

    ARK_INLINE TaggedObject *GetHeapObject() const
    {
        if (IsWeakForHeapObject()) {
            return GetTaggedWeakRef();
        }
        return GetTaggedObject();
    }

    ARK_INLINE TaggedObject *GetRawTaggedObject() const
    {
        return reinterpret_cast<TaggedObject *>(GetRawHeapObject());
    }

    ARK_INLINE TaggedObject *GetTaggedWeakRef() const
    {
        return reinterpret_cast<TaggedObject *>(GetWeakReferent());
    }

    static JSTaggedValue OrdinaryToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                             PreferredPrimitiveType type = PREFER_NUMBER);

    // ecma6 7.1 Type Conversion
    static JSTaggedValue ToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                     PreferredPrimitiveType type = NO_PREFERENCE);
    bool PUBLIC_API ToBoolean() const;
    static JSTaggedNumber ToNumber(JSThread *thread, JSTaggedValue tagged);
    static JSTaggedNumber ToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue ToBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue ToBigInt64(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue ToBigUint64(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber ToInteger(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<JSTaggedValue> ToNumeric(JSThread *thread, JSHandle<JSTaggedValue> tagged);
    static int32_t ToInt32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint32_t ToUint32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static int16_t ToInt16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint16_t ToUint16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static int8_t ToInt8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint8_t ToUint8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint8_t ToUint8Clamp(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<EcmaString> PUBLIC_API ToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static std::string ExceptionToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<EcmaString> ToString(JSThread *thread, JSTaggedValue val);
    static JSHandle<JSObject> ToObject(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<JSTaggedValue> ToPropertyKey(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber ToLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue CanonicalNumericIndexString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber ToIndex(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber StringToDouble(JSTaggedValue tagged);
    static JSTaggedNumber StringToNumber(JSTaggedValue tagged);

    static bool ToArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged, uint32_t *output);
    static bool ToElementIndex(JSTaggedValue key, uint32_t *output);
    static bool StringToElementIndex(JSTaggedValue key, uint32_t *output);
    static bool IsPureString(JSTaggedValue key);
    uint32_t GetArrayLength() const;

    // ecma6 7.2 Testing and Comparison Operations
    bool IsCallable() const;
    bool IsConstructor() const;
    bool IsExtensible(JSThread *thread) const;
    bool IsInteger() const;
    bool WithinInt32(bool acceptsNegativeZero = false) const;
    bool IsZero() const;
    bool IsExactlyZero() const;
    static bool IsPropertyKey(const JSHandle<JSTaggedValue> &key);
    static JSHandle<JSTaggedValue> RequireObjectCoercible(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                                          const char *message = "RequireObjectCoercible throw Error");
    static bool SameValue(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool SameValue(const JSHandle<JSTaggedValue> &xHandle, const JSHandle<JSTaggedValue> &yHandle);
    static bool SameValueString(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool SameValueString(const JSHandle<JSTaggedValue> &xHandle, const JSHandle<JSTaggedValue> &yHandle);
    static bool SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool Less(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y);
    static bool Equal(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y);
    static bool StrictEqual(const JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y);
    static bool StrictEqual(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool SameValueNumberic(const JSTaggedValue &x, const JSTaggedValue &y);

    // ES6 7.4 Operations on Iterator Objects
    static JSObject *CreateIterResultObject(JSThread *thread, const JSHandle<JSTaggedValue> &value, bool done);

    // ECMAScript 2023 allow the use of most Symbols as keys in weak collections
    static bool CanBeHeldWeakly(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);

    // ecma6 7.3
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode = SCheckMode::CHECK);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);
    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key,
                            const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static bool PUBLIC_API SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                       bool mayThrow = false, SCheckMode checkMode = SCheckMode::CHECK);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver,
                            bool mayThrow = false);
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode = SCheckMode::CHECK);
    static bool DeletePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key);
    static bool DefinePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                  const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc,
                                  SCheckMode sCheckMode = SCheckMode::CHECK);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);
    static bool SetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                             const JSHandle<JSTaggedValue> &proto, bool isChangeProto = false);
    static JSTaggedValue GetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static bool PreventExtensions(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static JSHandle<TaggedArray> GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static JSHandle<TaggedArray> GetAllPropertyKeys(JSThread *thread,
        const JSHandle<JSTaggedValue> &obj, uint32_t filter);
    static JSHandle<TaggedArray> GetOwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static bool HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key);
    static bool HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key);
    static bool HasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &key);
    static bool GlobalHasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key);

    // Type
    bool IsJSMap() const;
    bool IsJSSharedMap() const;
    bool IsJSSet() const;
    bool IsJSSharedSet() const;
    bool IsJSWeakMap() const;
    bool IsJSWeakSet() const;
    bool IsJSWeakRef() const;
    bool IsJSFinalizationRegistry() const;
    bool IsCellRecord() const;
    bool IsJSRegExp() const;
    bool IsPrimitive(uint8_t primitiveType) const;
    bool IsNumber() const;
    bool PUBLIC_API IsBigInt() const;
    bool IsString() const;
    bool IsLineString() const;
    bool IsTreeString() const;
    bool IsSlicedString() const;
    bool IsStringOrSymbol() const;
    bool IsLexicalEnv() const;
    bool IsSFunctionEnv() const;
    bool PUBLIC_API IsTaggedArray() const;
    bool IsDictionary() const;
    bool IsByteArray() const;
    bool IsConstantPool() const;
    bool IsAOTLiteralInfo() const;
    bool IsExtraProfileTypeInfo() const;
    bool IsProfileTypeInfoCell() const;
    bool IsProfileTypeInfoCell0() const;
    bool IsFunctionTemplate() const;
    bool IsVTable() const;
    bool IsLinkedNode() const;
    bool IsRBTreeNode() const;
    bool IsNativePointer() const;
    bool IsJSNativePointer() const;
    bool CheckIsJSNativePointer() const;
    bool PUBLIC_API IsBoolean() const;
    bool IsSymbol() const;
    bool IsJSObject() const;
    bool IsOnlyJSObject() const;
    bool IsJSGlobalObject() const;
    bool IsJSError() const;
    bool IsArray(JSThread *thread) const;
    bool IsSArray(JSThread *thread) const;
    bool IsCOWArray() const;
    bool IsMutantTaggedArray() const;
    bool IsJSArray() const;
    bool IsJSSharedArray() const;
    bool PUBLIC_API IsJSCOWArray() const;
    bool IsStableJSArray(JSThread *thread) const;
    bool IsStableJSArguments(JSThread *thread) const;
    bool IsTypedArray() const;
    bool IsJSTypedArray() const;
    bool IsJSInt8Array() const;
    bool IsJSUint8Array() const;
    bool IsJSUint8ClampedArray() const;
    bool IsJSInt16Array() const;
    bool IsJSUint16Array() const;
    bool IsJSInt32Array() const;
    bool IsJSUint32Array() const;
    bool IsJSFloat32Array() const;
    bool IsJSFloat64Array() const;
    bool IsJSBigInt64Array() const;
    bool IsJSBigUint64Array() const;
    bool IsSharedTypedArray() const;
    bool IsJSSharedTypedArray() const;
    bool IsJSSharedInt8Array() const;
    bool IsJSSharedUint8Array() const;
    bool IsJSSharedUint8ClampedArray() const;
    bool IsJSSharedInt16Array() const;
    bool IsJSSharedUint16Array() const;
    bool IsJSSharedInt32Array() const;
    bool IsJSSharedUint32Array() const;
    bool IsJSSharedFloat32Array() const;
    bool IsJSSharedFloat64Array() const;
    bool IsJSSharedBigInt64Array() const;
    bool IsJSSharedBigUint64Array() const;
    bool IsArguments() const;
    bool IsDate() const;
    bool IsBoundFunction() const;
    bool IsJSIntlBoundFunction() const;
    bool IsProxyRevocFunction() const;
    bool IsJSAsyncFunction() const;
    bool IsJSSharedAsyncFunction() const;
    bool IsJSAsyncAwaitStatusFunction() const;
    bool IsClassConstructor() const;
    bool IsClassPrototype() const;
    bool IsJSFunction() const;
    bool IsJSFunctionBase() const;
    bool CheckIsJSFunctionBase() const;
    bool IsECMAObject() const;
    bool IsJSPrimitiveRef() const;
    bool IsJSPrimitive() const;
    bool IsAccessorData() const;
    bool IsInternalAccessor() const;
    bool IsAccessor() const;
    bool IsJSGlobalEnv() const;
    bool PUBLIC_API IsJSProxy() const;
    bool CheckIsJSProxy() const;
    bool IsJSHClass() const;
    bool IsForinIterator() const;
    bool IsStringIterator() const;
    bool IsArrayBuffer() const;
    bool IsSharedArrayBuffer() const;
    bool IsSendableArrayBuffer() const;

    bool IsJSSetIterator() const;
    bool IsJSSharedSetIterator() const;
    bool IsJSRegExpIterator() const;
    bool IsJSMapIterator() const;
    bool IsJSSharedMapIterator() const;
    bool IsJSArrayIterator() const;
    bool IsJSSharedArrayIterator() const;
    bool IsIterator() const;
    bool IsAsyncIterator() const;
    bool IsGeneratorFunction() const;
    bool IsAsyncGeneratorFunction() const;
    bool IsGeneratorObject() const;
    bool IsGeneratorContext() const;
    bool IsAsyncGeneratorRequest() const;
    bool IsAsyncIteratorRecord() const;
    bool IsAsyncFromSyncIterator() const;
    bool IsAsyncGeneratorObject() const;
    bool IsAsyncFuncObject() const;
    bool IsJSPromise() const;
    bool IsJSXRefObject() const;
    bool IsRecord() const;
    bool IsPromiseReaction() const;
    bool IsProgram() const;
    bool IsJSPromiseReactionFunction() const;
    bool IsJSPromiseExecutorFunction() const;
    bool IsJSAsyncModuleFulfilledFunction() const;
    bool IsJSAsyncModuleRejectedFunction() const;
    bool IsJSAsyncFromSyncIterUnwarpFunction() const;
    bool IsJSPromiseAllResolveElementFunction() const;
    bool IsJSAsyncGeneratorResNextRetProRstFtn() const;
    bool IsPromiseCapability() const;
    bool IsPromiseIteratorRecord() const;
    bool IsPromiseRecord() const;
    bool IsJSPromiseAnyRejectElementFunction() const;
    bool IsJSPromiseAllSettledElementFunction() const;
    bool IsJSPromiseFinallyFunction() const;
    bool IsJSPromiseValueThunkOrThrowerFunction() const;
    bool IsResolvingFunctionsRecord() const;
    bool IsCompletionRecord() const;
    bool IsDataView() const;
    bool IsTemplateMap() const;
    bool IsMicroJobQueue() const;
    bool IsPendingJob() const;
    bool IsJSLocale() const;
    bool IsJSDateTimeFormat() const;
    bool IsJSRelativeTimeFormat() const;
    bool IsJSIntl() const;
    bool IsJSNumberFormat() const;
    bool IsJSCollator() const;
    bool IsJSPluralRules() const;
    bool IsJSDisplayNames() const;
    bool IsJSSegmenter() const;
    bool IsJSSegments() const;
    bool IsJSSegmentIterator() const;
    bool IsJSListFormat() const;
    bool IsMethod() const;
    bool IsClassLiteral() const;

    // non ECMA standard jsapis
    bool IsJSAPIArrayList() const;
    bool IsJSAPIArrayListIterator() const;
    bool IsJSAPIHashMap() const;
    bool IsJSAPIHashMapIterator() const;
    bool IsJSAPIHashSet() const;
    bool IsJSAPIHashSetIterator() const;
    bool IsJSAPILightWeightMap() const;
    bool IsJSAPILightWeightMapIterator() const;
    bool IsJSAPILightWeightSet() const;
    bool IsJSAPILightWeightSetIterator() const;
    bool IsJSAPITreeMap() const;
    bool IsJSAPITreeSet() const;
    bool IsJSAPITreeMapIterator() const;
    bool IsJSAPITreeSetIterator() const;
    bool IsJSAPIVector() const;
    bool IsJSAPIVectorIterator() const;
    bool IsJSAPIBitVector() const;
    bool IsJSAPIBitVectorIterator() const;
    bool IsJSAPIBuffer() const;
    bool IsJSAPIQueue() const;
    bool IsJSAPIQueueIterator() const;
    bool IsJSAPIPlainArray() const;
    bool IsJSAPIPlainArrayIterator() const;
    bool IsJSAPIDeque() const;
    bool IsJSAPIDequeIterator() const;
    bool IsJSAPIStack() const;
    bool IsJSAPIStackIterator() const;
    bool IsJSAPIList() const;
    bool IsJSAPILinkedList() const;
    bool IsJSAPIListIterator() const;
    bool IsJSAPILinkedListIterator() const;
    bool IsSpecialContainer() const;
    bool HasOrdinaryGet() const;
    bool IsPrototypeHandler() const;
    bool IsTransitionHandler() const;
    bool IsTransWithProtoHandler() const;
    bool IsStoreAOTHandler() const;
    bool IsPropertyBox() const;
    bool IsEnumCache() const;
    bool IsProtoChangeMarker() const;
    bool IsProtoChangeDetails() const;
    bool IsMarkerCell() const;
    bool IsTrackInfoObject() const;
    bool IsSpecialKeysObject() const;
    bool IsSlowKeysObject() const;
    bool IsRegularObject() const;
    bool IsMachineCodeObject() const;
    bool IsClassInfoExtractor() const;

    bool IsCjsExports() const;
    bool IsCjsModule() const;
    bool IsCjsRequire() const;
    bool IsModuleRecord() const;
    bool IsSourceTextModule() const;
    bool IsImportEntry() const;
    bool IsLocalExportEntry() const;
    bool IsIndirectExportEntry() const;
    bool IsStarExportEntry() const;
    bool IsModuleBinding() const;
    bool IsResolvedBinding() const;
    bool IsResolvedIndexBinding() const;
    bool IsResolvedRecordIndexBinding() const;
    bool IsResolvedRecordBinding() const;
    bool IsModuleNamespace() const;
    bool IsNativeModuleFailureInfo() const;
    bool IsJSSharedObject() const;
    bool IsJSSharedFunction() const;
    bool IsJSShared() const;
    bool IsSharedType() const;

    bool PUBLIC_API IsInSharedHeap() const;
    bool IsInSharedSweepableSpace() const;
    bool IsEnumCacheAllValid() const;
    bool IsEnumCacheOwnValid() const;
    bool IsEnumCacheProtoInfoUndefined() const;
    static bool IsSameTypeOrHClass(JSTaggedValue x, JSTaggedValue y);

    static ComparisonResult Compare(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                    const JSHandle<JSTaggedValue> &y);
    static int IntLexicographicCompare(JSTaggedValue x, JSTaggedValue y);
    static int DoubleLexicographicCompare(JSTaggedValue x, JSTaggedValue y);
    static ComparisonResult StrictNumberCompare(double x, double y);
    static bool StrictNumberEquals(double x, double y);
    static bool StrictIntEquals(int x, int y);
    static bool StringCompare(EcmaString *xStr, EcmaString *yStr);

    static JSHandle<JSTaggedValue> ToPrototypeOrObj(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    inline uint32_t GetKeyHashCode() const;
    uint32_t GetStringKeyHashCode() const;
    static JSTaggedValue GetSuperBase(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static JSTaggedValue TryCastDoubleToInt32(double d);

    void DumpTaggedValue(std::ostream &os) const DUMP_API_ATTR;
    void DumpTaggedValueType(std::ostream &os) const DUMP_API_ATTR;
    void Dump(std::ostream &os, bool isPrivacy = false) const DUMP_API_ATTR;
    void DumpHeapObjAddress(std::ostream &os) const DUMP_API_ATTR;
    void D() const DUMP_API_ATTR;
    void DumpForSnapshot(std::vector<Reference> &vec, bool isVmMode = true) const;
    static void DesensitizedDump(const JSHandle<JSTaggedValue> &obj);
    static void DV(JSTaggedType val) DUMP_API_ATTR;
    friend std::ostream& operator<<(std::ostream& os, const JSTaggedValue& value)
    {
        value.Dump(os);
        return os;
    }

private:
    JSTaggedType value_;

    inline double ExtractNumber() const;

    void DumpSpecialValue(std::ostream &os) const;
    void DumpHeapObjectType(std::ostream &os) const;

    // non ECMA standard jsapis
    static bool HasContainerProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key);
    static JSHandle<TaggedArray> GetOwnContainerPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static JSHandle<TaggedArray> GetOwnContainerEnumPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static bool GetContainerProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);
    static OperationResult GetJSAPIProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                            const JSHandle<JSTaggedValue> &key);
    static bool SetJSAPIProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                 const JSHandle<JSTaggedValue> &key,
                                 const JSHandle<JSTaggedValue> &value);
    static JSHandle<EcmaString> NativePointerToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static bool EqualNumber(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                            const JSHandle<JSTaggedValue> &y);
    static bool EqualString(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                            const JSHandle<JSTaggedValue> &y);
    static bool EqualSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                            const JSHandle<JSTaggedValue> &y);
    static bool EqualBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                            const JSHandle<JSTaggedValue> &y);
    static bool EqualHeapObject(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                const JSHandle<JSTaggedValue> &y);
    static bool EqualNullOrUndefined(const JSHandle<JSTaggedValue> &x,
                                     const JSHandle<JSTaggedValue> &y);
    static ARK_INLINE JSTaggedValue WrapUint64(uint64_t v)
    {
        return JSTaggedValue(static_cast<JSTaggedType>(v) | TAG_INT);
    }
    static ARK_INLINE uint64_t UnwrapToUint64(JSTaggedValue v)
    {
        ASSERT_PRINT(v.IsInt(), "can not convert JSTaggedValue to Int :" << std::hex << v.GetRawData());
        return static_cast<uint64_t>(v.GetRawData() & (~TAG_INT));
    }
    static void DumpExceptionObject(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    friend class PropertyAttributes;
    friend class ICRuntimeStub;
    friend class LoadHandler;
    friend class StoreHandler;
};
STATIC_ASSERT_EQ_ARCH(sizeof(JSTaggedValue), JSTaggedValue::SizeArch32, JSTaggedValue::SizeArch64);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_TAGGED_VALUE_H
