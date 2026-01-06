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

#ifndef ECMASCRIPT_BASE_JSON_STRINGIFY_INL_H
#define ECMASCRIPT_BASE_JSON_STRINGIFY_INL_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/base/json_helper.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/global_env.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript::base {

// All cached keys are guaranteed to be valid for fast path (already checked by IsFastValueToQuotedString),
// meaning they are raw UTF8 strings without escape characters and can be processed directly without re-validation.
// A new cache instance is created for each JsonStringifier and cleaned up by GC.
class JsonStringifierKeyCache {
public:
    static constexpr size_t KEY_CACHE_SIZE = 64;
    static constexpr size_t KEY_CACHE_MASK = KEY_CACHE_SIZE - 1;
    static constexpr uintptr_t KEY_CACHE_PTR_SHIFT = 4;

    JsonStringifierKeyCache()
    {
        Clear();
    }

    void Clear()
    {
        for (size_t i = 0; i < KEY_CACHE_SIZE; ++i) {
            keyCache_[i] = JSTaggedValue::Undefined();
        }
    }

    inline size_t GetKeyCacheIndex(const JSTaggedValue &key) const
    {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(key.GetTaggedObject());
        return (ptr >> KEY_CACHE_PTR_SHIFT) & KEY_CACHE_MASK;
    }

    inline bool TryGetCachedKey(const JSTaggedValue &key) const
    {
        ASSERT(key != JSTaggedValue::Undefined());
        size_t index = GetKeyCacheIndex(key);
        JSTaggedValue cached = keyCache_[index];
        return cached == key;
    }

    inline void CacheKey(const JSTaggedValue &key)
    {
        size_t index = GetKeyCacheIndex(key);
        keyCache_[index] = key;
    }

private:
    JSTaggedValue keyCache_[KEY_CACHE_SIZE];
};

#if ENABLE_LATEST_OPTIMIZATION
class JsonStringifier {
    using TransformType = base::JsonHelper::TransformType;
public:
    explicit JsonStringifier(JSThread *thread, TransformType transformType = TransformType::NORMAL)
        : encoding_(Encoding::ONE_BYTE_ENCODING), oneByteResult_(), twoBytesResult_(),
        gap_(), indent_(0), thread_(thread), transformType_(transformType)
    {
        if (auto existingKeyCache = thread_->GetJsonStringifierKeyCache().lock()) {
            keyCache_ = existingKeyCache;
        } else {
            keyCache_ = std::make_shared<JsonStringifierKeyCache>();
            thread_->SetJsonStringifierKeyCache(keyCache_);
        }
    }

    ~JsonStringifier()
    {
    }

    NO_COPY_SEMANTIC(JsonStringifier);
    NO_MOVE_SEMANTIC(JsonStringifier);

    static constexpr size_t INT_MAX_LENGTH = 11;       // Max int length: -2147483648 (11 chars)
    static constexpr size_t DOUBLE_MAX_LENGTH = 32;    // Max double length with exponent

    JSHandle<JSTaggedValue> Stringify(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer,
                                      const JSHandle<JSTaggedValue> &gap);

    template <bool ReplacerAndGapUndefined>
    JSHandle<JSTaggedValue> StringifyInternal(const JSHandle<JSTaggedValue> &value,
                                              const JSHandle<JSTaggedValue> &gap);

    // For all member methods using oneByteResult_ or twoBytesResult_,
    // guarantee length_ + length <= capacity_ in advance.
    template <typename DestChar>
    class FastStringBuilder {
    public:
        using value_type = DestChar;

        FastStringBuilder()
            : buffer_(stackBuffer_), length_(0), capacity_(STACK_BUFFER_SIZE), useHeap_(false) {}

        ~FastStringBuilder()
        {
            if (useHeap_) {
                delete[] buffer_;
                buffer_ = nullptr;
            }
        }

        NO_COPY_SEMANTIC(FastStringBuilder);
        NO_MOVE_SEMANTIC(FastStringBuilder);

        inline void Append(DestChar c)
        {
            buffer_[length_++] = c;
        }

        inline void AppendCString(const char* s)
        {
            while (*s != '\0') {
                Append(static_cast<DestChar>(*(s++)));
            }
        }

        template <typename SrcChar>
        inline void AppendString(const SrcChar* s, size_t len)
        {
            switch (len) {
#define CASE(N)                               \
    case N:                                   \
        std::copy_n(s, N, buffer_ + length_); \
        break;
                CASE(1) CASE(2) CASE(3) CASE(4)
                CASE(5) CASE(6) CASE(7) CASE(8)
                CASE(9) CASE(10) CASE(11) CASE(12)
                CASE(13) CASE(14) CASE(15) CASE(16)
#undef CASE
                default:
                    std::copy_n(s, len, buffer_ + length_);
                    break;
            }
            length_ += len;
        }

        inline void PopBack()
        {
            if (length_ > 0) {
                --length_;
            }
        }

        inline size_t GetLength() const
        {
            return length_;
        }

        inline DestChar* GetBuffer()
        {
            return buffer_;
        }

        inline size_t GetCapacity() const
        {
            return capacity_;
        }

        inline void SetLength(size_t newLength)
        {
            length_ = newLength;
        }

        inline void Reverse(size_t start, size_t end)
        {
            std::reverse(&buffer_[start], &buffer_[end]);
        }

        ARK_NOINLINE void GrowCapacity(size_t len)
        {
            size_t newCapacity = capacity_ * GROWTH_FACTOR;
            while (newCapacity < length_ + len) {
                newCapacity *= GROWTH_FACTOR;
            }
            DestChar* newBuffer = new DestChar[newCapacity];
            std::copy_n(buffer_, length_, newBuffer);
            if (useHeap_) {
                delete[] buffer_;
                buffer_ = nullptr;
            }
            buffer_ = newBuffer;
            capacity_ = newCapacity;
            useHeap_ = true;
        }

        inline void EnsureCapacity(size_t len)
        {
            if (LIKELY(length_ + len <= capacity_)) {
                return;
            }

            GrowCapacity(len);
        }

    private:
        static constexpr size_t STACK_BUFFER_SIZE = 2048;
        static constexpr size_t GROWTH_FACTOR = 2;

        DestChar* buffer_;
        size_t length_;
        size_t capacity_;
        bool useHeap_;
        DestChar stackBuffer_[STACK_BUFFER_SIZE];
    };
    // Static helper methods for FastStringBuilder - public for testing
    template <typename DestChar>
    static void AppendIntToFastStringBuilder(FastStringBuilder<DestChar> &str, int number);

    template <typename DestChar>
    static void AppendDoubleToFastStringBuilder(FastStringBuilder<DestChar> &str, double d);

    template <typename DestChar>
    static bool AppendSpecialDoubleToFastStringBuilder(FastStringBuilder<DestChar> &str, double d);

    template <typename DestChar>
    static bool AppendQuotedStringToFastStringBuilder(const JSThread *thread,
        FastStringBuilder<DestChar> &output, const EcmaString *str);
    
    template <typename DestType>
    static void AppendBigIntToString(DestType &str, BigInt *bigint);

    // Public methods for testing
    inline void AppendNumberToResult(const JSHandle<JSTaggedValue> &value);

    inline FastStringBuilder<uint8_t> &GetOneByteResult()
    {
        return oneByteResult_;
    }

    inline const FastStringBuilder<uint8_t> &GetOneByteResult() const
    {
        return oneByteResult_;
    }
private:
    void AddDeduplicateProp(const JSHandle<JSTaggedValue> &property);

    template <bool ReplacerAndGapUndefined>
    JSTaggedValue SerializeJSONProperty(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    JSTaggedValue GetSerializeValue(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSTaggedValue> &value);

    void SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONObject(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeObjectProperties(const JSHandle<JSObject> &obj, bool hasContent);

    template <bool ReplacerAndGapUndefined>
    bool SerializeObjectWithReplacerArray(const JSHandle<JSObject> &obj, bool hasContent);

    template <bool ReplacerAndGapUndefined>
    bool ProcessSerializedProperty(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value, bool hasContent);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONSharedMap(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONSharedSet(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONMap(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONSet(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONHashMap(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSONHashSet(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeLinkedHashMap(const JSHandle<LinkedHashMap> &hashMap);

    template <bool ReplacerAndGapUndefined>
    bool SerializeLinkedHashSet(const JSHandle<LinkedHashSet> &hashSet);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSArray(const JSHandle<JSTaggedValue> &value);

    template <bool ReplacerAndGapUndefined>
    bool SerializeJSProxy(const JSHandle<JSTaggedValue> &object);

    template <bool ReplacerAndGapUndefined>
    void SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef);

    bool PushValue(const JSHandle<JSTaggedValue> &value);

    void PopValue();

    bool CalculateNumberGap(JSTaggedValue gap);

    bool CalculateStringGap(const JSHandle<EcmaString> &primString);

    template <bool ReplacerAndGapUndefined>
    bool AppendJsonString(const JSHandle<JSObject> &obj, bool hasContent);

    template <bool ReplacerAndGapUndefined>
    bool SerializeElements(const JSHandle<JSObject> &obj, bool hasContent);

    template <bool ReplacerAndGapUndefined>
    bool SerializeKeys(const JSHandle<JSObject> &obj, bool hasContent);

    JSHandle<JSTaggedValue> SerializeHolder(const JSHandle<JSTaggedValue> &object,
                                            const JSHandle<JSTaggedValue> &value);

    bool CheckStackPushSameValue(JSHandle<JSTaggedValue> value);

    inline void Indent()
    {
        indent_++;
    }

    inline void Unindent()
    {
        indent_--;
    }

    inline void NewLine();

    void ChangeEncoding();

    template <size_t N>
    inline void AppendLiteral(const char(&src)[N]);

    inline void AppendChar(const char src);

    inline void AppendIntToResult(int32_t key);

    inline void EnsureCapacityFor(size_t size);

    inline bool AppendEcmaStringToResult(JSHandle<EcmaString> &string);

    inline void AppendBigIntToResult(JSHandle<JSTaggedValue> &valHandle);

    inline void PopBack();

    template <typename BuilderType>
    inline void AppendFastPathKeyString(const EcmaString* strPtr, BuilderType &output);

    inline JsonStringifierKeyCache* GetKeyCache() const
    {
        ASSERT(thread_->GetJsonStringifierKeyCache().lock() == keyCache_);
        ASSERT(keyCache_ != nullptr);
        return keyCache_.get();
    }

    template <typename SrcChar>
    inline void AppendStringInternal(const SrcChar* s, size_t len)
    {
        if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
            oneByteResult_.AppendString(s, len);
        } else {
            twoBytesResult_.AppendString(s, len);
        }
    }

    enum class Encoding {
        ONE_BYTE_ENCODING,
        TWO_BYTE_ENCODING
    };

    Encoding encoding_;
    FastStringBuilder<uint8_t> oneByteResult_;
    FastStringBuilder<uint16_t> twoBytesResult_;
    C16String gap_;
    int indent_;
    std::shared_ptr<JsonStringifierKeyCache> keyCache_;

    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    CVector<JSHandle<JSTaggedValue>> stack_;
    CVector<JSHandle<JSTaggedValue>> propList_;
    JSMutableHandle<JSTaggedValue> handleKey_ {};
    JSMutableHandle<JSTaggedValue> handleValue_ {};
    JSHandle<JSTaggedValue> replacer_ {};
    TransformType transformType_ {};
};
#else
class JsonStringifier {
    using TransformType = base::JsonHelper::TransformType;
public:
    JsonStringifier() = default;

    explicit JsonStringifier(JSThread *thread, TransformType transformType = TransformType::NORMAL)
        : thread_(thread), transformType_(transformType)
    {
    }

    ~JsonStringifier() = default;

    NO_COPY_SEMANTIC(JsonStringifier);
    NO_MOVE_SEMANTIC(JsonStringifier);

    JSHandle<JSTaggedValue> Stringify(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer,
                                      const JSHandle<JSTaggedValue> &gap);

private:
    void AddDeduplicateProp(const JSHandle<JSTaggedValue> &property);

    JSTaggedValue SerializeJSONProperty(const JSHandle<JSTaggedValue> &value,
                                        const JSHandle<JSTaggedValue> &replacer);

    JSTaggedValue GetSerializeValue(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    void SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent);

    bool SerializeJSONObject(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSONSharedMap(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSONSharedSet(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSONMap(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSONSet(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSONHashMap(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSONHashSet(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeLinkedHashMap(const JSHandle<LinkedHashMap> &hashMap, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeLinkedHashSet(const JSHandle<LinkedHashSet> &hashSet, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSArray(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSProxy(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &replacer);

    void SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef, const JSHandle<JSTaggedValue> &replacer);

    bool PushValue(const JSHandle<JSTaggedValue> &value);

    void PopValue();

    bool CalculateNumberGap(JSTaggedValue gap);

    bool CalculateStringGap(const JSHandle<EcmaString> &primString);
    bool AppendJsonString(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer, bool hasContent);
    bool SerializeElements(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer, bool hasContent);
    bool SerializeKeys(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer, bool hasContent);
    JSHandle<JSTaggedValue> SerializeHolder(const JSHandle<JSTaggedValue> &object,
                                            const JSHandle<JSTaggedValue> &value);
    bool CheckStackPushSameValue(JSHandle<JSTaggedValue> value);

    CString gap_;
    CString result_;
    CString indent_;

    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    CVector<JSHandle<JSTaggedValue>> stack_;
    CVector<JSHandle<JSTaggedValue>> propList_;
    JSMutableHandle<JSTaggedValue> handleKey_ {};
    JSMutableHandle<JSTaggedValue> handleValue_ {};
    TransformType transformType_ {};
};
#endif
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_JSON_STRINGIFY_INL_H