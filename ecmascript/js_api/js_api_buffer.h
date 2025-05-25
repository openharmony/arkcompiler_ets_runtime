/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_API_JS_API_BUFFER_H
#define ECMASCRIPT_JS_API_JS_API_BUFFER_H

#include "ecmascript/base/atomic_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include <iomanip>
#include <ios>
#include <string_view>

namespace panda::ecmascript {
/**
 * Provide the object of non ECMA standard jsapi container.
 * JSAPIFastBuffer provides dynamically modified array.
 * */
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
using ElementSize = base::ElementSize;
using TypedArrayHelper = base::TypedArrayHelper;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;

#define JSAPI_BUFFER_ACCESSORS(name, type)                                                                            \
    static JSTaggedValue Write##name(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,                       \
                                     const JSHandle<JSTaggedValue> &value, uint32_t offset, bool littleEndian = true) \
    {                                                                                                                 \
        JSType type = JSType::JS_##type##_ARRAY;                                                                      \
        auto byteSize = base::TypedArrayHelper::GetElementSize(type);                                                 \
        JSHandle<JSTypedArray> typedArray =                                                                           \
            JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));        \
        double valueNum = value->GetNumber();                                                                         \
        double left = (type##_MIN);                                                                                   \
        double right = (type##_MAX);                                                                                  \
        if (valueNum > right || valueNum < left) {                                                                    \
            std::ostringstream oss;                                                                                   \
            oss << "The value of \"value\" is out of range. It must be >= " << left << " and <= " << right            \
                << ". Received value is: ";                                                                           \
            if (value->IsInt()) {                                                                                     \
                oss << std::fixed << std::setprecision(0) << valueNum;                                                \
            } else {                                                                                                  \
                oss << valueNum;                                                                                      \
            }                                                                                                         \
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());   \
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                              \
        }                                                                                                             \
        if (offset + NumberSize::type > buffer->GetLength()) {                                                        \
            std::ostringstream oss;                                                                                   \
            oss << "The value of \"offset\" is out of range. It must be >= " << 0                                     \
                << " and <= " << buffer->GetLength() - NumberSize::type << ". Received value is: " << offset;         \
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());   \
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                              \
        }                                                                                                             \
        JSAPIFastBuffer::SetValueByIndex(thread, typedArray.GetTaggedValue(), buffer->GetOffset() + offset,           \
                                         value.GetTaggedValue(), type, littleEndian);                                 \
        return JSTaggedValue(offset + byteSize);                                                                      \
    }                                                                                                                 \
    static JSTaggedValue Read##name(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,       \
                                    bool littleEndian = true)                                                         \
    {                                                                                                                 \
        JSType type = JSType::JS_##type##_ARRAY;                                                                      \
        JSHandle<JSTypedArray> typedArray =                                                                           \
            JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));        \
        if (offset + NumberSize::type > buffer->GetLength()) {                                                        \
            std::ostringstream oss;                                                                                   \
            oss << "The value of \"offset\" is out of range. It must be >= " << 0                                     \
                << " and <= " << buffer->GetLength() - NumberSize::type << ". Received value is: " << offset;         \
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());   \
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                              \
        }                                                                                                             \
        return JSAPIFastBuffer::GetValueByIndex(thread, typedArray.GetTaggedValue(), buffer->GetOffset() + offset,    \
                                                type, littleEndian);                                                  \
    }

class JSAPIFastBuffer : public JSObject {
public:
    enum EncodingType { INVALID = 0, ASCII, UTF8, UTF16LE, BASE64, BASE64URL, LATIN1, BINARY, HEX };
    enum NumberSize {
        UINT8 = 1,
        INT8 = 1,
        UINT16 = 2,
        INT16 = 2,
        UINT32 = 4,
        INT32 = 4,
        FLOAT32 = 4,
        FLOAT64 = 8,
        BIGINT64 = 8,
        BIGUINT64 = 8
    };
    enum ByteLength {
        OneByte = 1,
        TwoBytes,
        ThreeBytes,
        FourBytes,
        FiveBytes,
        SixBytes,
        SevenBytes,
        EightBytes,
        OneByteBits = 8,
    };
    static constexpr uint32_t ONE_BYTE_BIT_LENGTH = 8;
    static constexpr uint32_t DEFAULT_CAPACITY_LENGTH = 4;
    static constexpr uint32_t UINT8_MIN = 0;
    static constexpr uint32_t UINT16_MIN = 0;
    static constexpr uint32_t UINT32_MIN = 0;
    static constexpr float FLOAT32_MIN = std::numeric_limits<float>::min();
    static constexpr float FLOAT32_MAX = std::numeric_limits<float>::max();
    static constexpr double FLOAT64_MIN = std::numeric_limits<double>::min();
    static constexpr double FLOAT64_MAX = std::numeric_limits<double>::max();
    static JSAPIFastBuffer *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIBuffer());
        return static_cast<JSAPIFastBuffer *>(object);
    }

    // these operation will modify buffer
    static JSTaggedValue WriteString(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                     JSHandle<JSTaggedValue> &value, uint32_t offset, uint32_t maxLen,
                                     EncodingType encoding);
    static JSTaggedValue WriteBytesValue(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                         JSHandle<JSTaggedValue> &value, uint32_t offset, ByteLength byteLength,
                                         bool littleEndian = false);
    static JSTaggedValue ReadBytes(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                   ByteLength byteLength, bool littleEndian = false);
    static JSTaggedValue ReadInt(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                 ByteLength byteLength, bool littleEndian = false);

    static JSTaggedValue WriteBigUInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                        const JSHandle<JSTaggedValue> &value, uint32_t offset,
                                        bool littleEndian = true);
    static JSTaggedValue ReadBigUInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                       bool littleEndian = true);
    static JSTaggedValue WriteBigInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                       const JSHandle<JSTaggedValue> &value, uint32_t offset, bool littleEndian = true);
    static JSTaggedValue ReadBigInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                      bool littleEndian = true);
    static JSTaggedValue Fill(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, JSHandle<JSTaggedValue> &valueHandle,
                              EncodingType encoding, uint32_t start, uint32_t end);
    static JSTaggedValue FillString(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                    JSHandle<JSTaggedValue> &valueHandle, EncodingType encoding, uint32_t start,
                                    uint32_t end);

    // query or create buffer
    static void ExtendBufferCapacity(JSThread *thread, JSHandle<JSAPIFastBuffer> &dst, uint32_t capacity);
    static JSTaggedValue WriteStringLoop(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, std::string_view data,
                                         uint32_t start, uint32_t end);
    static JSTaggedValue WriteBufferObjectLoop(JSHandle<JSAPIFastBuffer> &buffer, JSHandle<JSTaggedValue> &srcObj,
                                               uint32_t start, uint32_t end);
    static JSTaggedValue Copy(JSThread *thread, const JSHandle<JSTaggedValue> &dst, const JSHandle<JSTaggedValue> &src,
                              uint32_t tStart, uint32_t sStart, uint32_t sEnd);
    static JSTaggedValue FromArrayBuffer(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                         const JSHandle<JSTaggedValue> &src, uint32_t byteOffset, uint32_t length);
    static JSTaggedValue FromString(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                    const JSHandle<JSTaggedValue> &str, const JSHandle<JSTaggedValue> &encoding);
    static JSTaggedValue FromString(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                    const JSHandle<JSTaggedValue> &str, EncodingType encoding = UTF8);
    static JSTaggedValue ToString(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, EncodingType encodingType,
                                  uint32_t start, uint32_t end);
    static std::string GetString(const JSHandle<JSTaggedValue> &str, JSHandle<JSTaggedValue> encoding);
    static std::string GetString(const JSHandle<JSTaggedValue> &str, EncodingType encodingType = UTF8);
    static std::string_view FastGetString(const JSHandle<JSTaggedValue> &str, EncodingType encodingType,
                                          std::string &strDecoded);
    static JSTaggedValue AllocateFromBufferObject(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                                  const JSHandle<JSTaggedValue> &src, uint32_t byteLength = 0,
                                                  uint32_t byteOffset = 0);
    static JSHandle<JSTypedArray> NewUint8Array(JSThread *thread, uint32_t length, uint32_t byteOffset = 0);
    static JSTaggedValue AllocateFastBuffer(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                            uint32_t byteLength, uint32_t byteOffset = 0);
    static JSTaggedValue FromBufferToArray(JSThread *thread, JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Entries(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer);
    static JSTaggedValue Keys(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer);
    static JSTaggedValue Values(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer);
    static JSTaggedValue Includes(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                  JSHandle<JSTaggedValue> valueHandle, uint32_t start = 0,
                                  EncodingType encoding = UTF8);
    static JSTaggedValue IndexOf(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                 JSHandle<JSTaggedValue> valueHandle, uint32_t start = 0, EncodingType encoding = UTF8,
                                 bool isReverse = false);
    static JSTaggedValue Compare(JSThread *thread, JSHandle<JSAPIFastBuffer> &a, JSHandle<JSTaggedValue> &b,
                                 uint32_t sStart, uint32_t sEnd, uint32_t tStart, uint32_t tEnd);
    static JSTaggedValue GetArrayBuffer(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer);
    static JSTaggedValue CreateBufferFromArrayLike(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                                   const JSHandle<JSTaggedValue> &obj);
    static EncodingType GetEncodingType(std::string encode);
    static EncodingType GetEncodingType(const JSHandle<JSTaggedValue> &encode);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSAPIFastBuffer> &obj,
                                       const JSHandle<JSTaggedValue> &key);
    static bool SetProperty(JSThread *thread, const JSHandle<JSAPIFastBuffer> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value);

    static constexpr size_t TYPEDARRAY_OFFSET = JSObject::SIZE;
    ACCESSORS(FastBufferData, TYPEDARRAY_OFFSET, BUFFER_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, BUFFER_LENGTH_OFFSET, OFFSET_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Offset, uint32_t, OFFSET_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static const uint32_t MAX_BUFFER_INDEX = MAX_ELEMENT_INDEX;
    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, TYPEDARRAY_OFFSET, BUFFER_LENGTH_OFFSET)

    inline uint32_t GetSize() const
    {
        return GetLength();
    }

    inline uint32_t End() const
    {
        return GetOffset() + GetLength();
    }

    inline uint32_t Begin(uint32_t offset = 0) const
    {
        ASSERT(GetOffset() + offset < End());
        return GetOffset() + offset;
    }

    JSAPI_BUFFER_ACCESSORS(UInt8, UINT8);
    JSAPI_BUFFER_ACCESSORS(UInt16, UINT16);
    JSAPI_BUFFER_ACCESSORS(UInt32, UINT32);
    JSAPI_BUFFER_ACCESSORS(Int8, INT8);
    JSAPI_BUFFER_ACCESSORS(Int16, INT16);
    JSAPI_BUFFER_ACCESSORS(Int32, INT32);
    JSAPI_BUFFER_ACCESSORS(Float32, FLOAT32);
    JSAPI_BUFFER_ACCESSORS(Float64, FLOAT64);

private:
    JSTaggedValue SetValueByIndex(JSThread *thread, uint32_t index, JSTaggedValue value, JSType jsType,
                                  bool littleEndian = true)
    {
        auto typeArray = this->GetFastBufferData();
        return SetValueByIndex(thread, typeArray, index, value, jsType, littleEndian);
    }
    static JSTaggedValue SetValueByIndex(JSThread *thread, const JSTaggedValue typedarray, uint32_t index,
                                         JSTaggedValue value, JSType jsType, bool littleEndian = true);
    static JSTaggedValue GetValueByIndex(JSThread *thread, const JSTaggedValue typedarray, uint32_t index,
                                         JSType jsType, bool littleEndian = true);
    static bool WriteBytes(JSThread *thread, const uint8_t *src, unsigned int size, uint8_t *dest);
    static bool WriteDataLoop(const uint8_t *src, uint8_t *dest, uint32_t length, uint32_t start, uint32_t end);
    static bool WriteBytes(JSThread *thread, JSHandle<JSTaggedValue> srcData, JSHandle<JSTaggedValue> dstData,
                           uint32_t sStart, uint32_t tStart, uint32_t size);
    static int32_t StringMatch(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, const uint8_t *str, uint32_t start,
                               uint32_t strLength, bool isReverse = false);
};

class StringConverter {
public:
    static constexpr uint32_t LOWER_EIGHT_BITS_MASK = 0x00FF;
    static constexpr uint8_t HIGER_4_BITS_MASK = 0xF0;
    static constexpr uint8_t FOUR_BYTES_STYLE = 0xF0;
    static constexpr uint8_t THREE_BYTES_STYLE = 0xE0;
    static constexpr uint8_t TWO_BYTES_STYLE1 = 0xD0;
    static constexpr uint8_t TWO_BYTES_STYLE2 = 0xC0;
    static constexpr uint32_t LOWER_10_BITS_MASK = 0x03FFU;
    static constexpr uint32_t LOWER_8_BITS_MASK = 0x00FFU;
    static constexpr uint8_t LOWER_6_BITS_MASK = 0x3FU;
    static constexpr uint8_t LOWER_5_BITS_MASK = 0x1FU;
    static constexpr uint8_t LOWER_4_BITS_MASK = 0x0FU;
    static constexpr uint8_t LOWER_3_BITS_MASK = 0x07U;
    static constexpr uint8_t LOWER_2_BITS_MASK = 0x03U;
    static constexpr uint8_t MIDDLE_4_BITS_MASK = 0x3CU;
    static constexpr uint32_t HIGH_AGENT_MASK = 0xD800U;
    static constexpr uint32_t LOW_AGENT_MASK = 0xDC00U;
    static constexpr uint32_t UTF8_VALID_BITS = 6;
    static constexpr uint32_t UTF8_ONE_BYTE_MAX = 0x007F;
    static constexpr uint32_t UTF8_ONE_BYTE_SCALE = UTF8_ONE_BYTE_MAX + 1;
    static constexpr uint32_t UTF8_TWO_BYTES_MAX = 0x07FF;
    static constexpr uint32_t HIGH_AGENT_RANGE_FROM = 0xD800;
    static constexpr uint32_t HIGH_AGENT_RANGE_TO = 0xDBFF;
    static constexpr uint32_t LOW_AGENT_RANGE_FROM = 0xDC00;
    static constexpr uint8_t UTF8_TWO_BYTES_HEAD_BYTE_MASK = 0xC0;
    static constexpr uint8_t UTF8_TAIL_BYTE_MASK = 0x80;
    static constexpr uint8_t UTF8_THREE_BYTES_HEAD_BYTE_MASK = 0xE0;
    static constexpr uint8_t UTF8_FOUR_BYTES_HEAD_BYTE_MASK = 0xF0;
    static constexpr uint32_t UTF16_SPECIAL_VALUE = 0x10000;
    static void Utf8ToUtf16BEToData(const unsigned char *data, std::u16string &u16Str, std::string::size_type &index,
                                    uint8_t &c1);
    static std::u16string Utf8ToUtf16BE(const std::string &u8Str, bool *ok = nullptr);
    static std::string Utf16BEToANSI(const std::u16string &wstr);
    static std::u16string Utf16BEToLE(const std::u16string &wstr);
    static std::string Utf8ToUtf16BEToANSI(const std::string &str);
    static std::string Utf16StrToStr(std::u16string &value);
    static void Latin1Encode(const unsigned char *data, uint32_t len, std::string &ret);
    static void Base64Encode(const unsigned char *encodedStr, uint32_t len, std::string &ret, bool isUrl = false);
    static void Base64Decode(std::string_view encodedStr, std::string &ret);
    static void HexEncode(const unsigned char *hexStr, uint32_t len, std::string &ret);
    static void HexDecode(std::string_view hexStr, std::string &ret);
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_JS_API_BUFFER_H
