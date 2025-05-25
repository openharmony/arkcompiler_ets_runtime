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

#include "ecmascript/js_api/js_api_buffer.h"
#include <cstring>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/byte_array.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_factory.h"
#include "jsnapi_expo.h"
#include "macros.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using string = std::string;
using string_view = std::string_view;
using ErrorFlag = containers::ErrorFlag;
using ElementSize = base::ElementSize;
using EncodingType = JSAPIFastBuffer::EncodingType;
using TypedArrayHelper = base::TypedArrayHelper;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;
using u16string = std::u16string;

namespace {
const std::map<string, EncodingType> ENCODING_MAP = {
    {"hex", EncodingType::HEX},         {"base64url", EncodingType::BASE64URL}, {"base64", EncodingType::BASE64},
    {"ascii", EncodingType::ASCII},     {"latin1", EncodingType::LATIN1},       {"binary", EncodingType::BINARY},
    {"ucs2", EncodingType::UTF16LE},    {"ucs-2", EncodingType::UTF16LE},       {"utf-16le", EncodingType::UTF16LE},
    {"utf16le", EncodingType::UTF16LE}, {"utf-8", EncodingType::UTF8},          {"utf8", EncodingType::UTF8},
};
}  // namespace

const uint8_t BASE64_TABLE[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, -1, -1, -1, -1, 63, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

inline bool RangeChecker(uint32_t rangeLeft, uint32_t rangeRight, uint32_t l, uint32_t r)
{
    return l <= r && l <= rangeRight && r >= rangeLeft;
}

bool IsBase64Char(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/') || (c == '-') || (c == '_'));
}

EncodingType JSAPIFastBuffer::GetEncodingType(string encode)
{
    if (ENCODING_MAP.find(encode) != ENCODING_MAP.end()) {
        return ENCODING_MAP.at(encode);
    }
    return EncodingType::INVALID;
}

EncodingType JSAPIFastBuffer::GetEncodingType(const JSHandle<JSTaggedValue> &encode)
{
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(encode));
    auto str = strAccessor.ToStdString();
    return GetEncodingType(str);
}

JSTypedArray *GetUInt8ArrayFromBufferObject(JSTaggedValue buffer)
{
    if (buffer.IsJSUint8Array()) {
        return JSTypedArray::Cast(buffer.GetTaggedObject());
    }
    ASSERT(buffer.IsJSAPIBuffer());
    return JSTypedArray::Cast(JSAPIFastBuffer::Cast(buffer.GetTaggedObject())->GetFastBufferData().GetTaggedObject());
}

JSTypedArray *GetUInt8ArrayFromBufferObject(JSHandle<JSTaggedValue> buffer)
{
    return GetUInt8ArrayFromBufferObject(buffer.GetTaggedValue());
}

uint8_t *GetBufferData(JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset)
{
    if (!RangeChecker(0, buffer->GetLength(), offset, offset)) {
        return nullptr;
    }
    JSTypedArray *array = GetUInt8ArrayFromBufferObject(buffer.GetTaggedValue());
    JSTaggedValue arrayBuffer = array->GetViewedArrayBufferOrByteArray();
    // If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(arrayBuffer)) {
        return nullptr;
    }
    auto res = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(arrayBuffer, buffer->GetOffset() + offset));
    return res;
}

int32_t GetValueInt32(JSHandle<JSTaggedValue> valueHandle)
{
    JSTaggedValue value = valueHandle.GetTaggedValue();
    if (UNLIKELY(value.IsDouble())) {
        return static_cast<int32_t>(
            ecmascript::base::NumberHelper::DoubleToInt(value.GetDouble(), ecmascript::base::INT32_BITS));
    }
    return value.GetInt();
}

uint32_t GetValueUInt32(JSHandle<JSTaggedValue> valueHandle)
{
    return static_cast<uint32_t>(GetValueInt32(valueHandle));
}

string FromStringUtf8(const JSHandle<JSTaggedValue> &str)
{
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    auto res = strAccessor.ToStdString();
    return res;
}

string_view FromStringUtf16(const JSHandle<JSTaggedValue> &str, string &stringDecoded)
{
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    auto u16string = StringConverter::Utf8ToUtf16BE(strAccessor.ToStdString());
    stringDecoded = StringConverter::Utf16StrToStr(u16string);
    return string_view(stringDecoded);
}

string_view FromStringASCII(const JSHandle<JSTaggedValue> &str, string &stringDecoded)
{
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    uint32_t length = strAccessor.GetLength();
    stringDecoded.reserve(length);
    stringDecoded.resize(length);
    strAccessor.WriteToOneByte(reinterpret_cast<uint8_t *>(stringDecoded.data()), length);
    return string_view(stringDecoded);
}

string_view FromStringBase64(const JSHandle<JSTaggedValue> &str, string &stringDecoded)
{
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = strAccessor.ToUtf8Span(buf);
    StringConverter::Base64Decode(string_view(reinterpret_cast<const char *>(sp.data()), sp.size()), stringDecoded);
    return std::string_view(stringDecoded);
}

std::string ConvertEcmaStringToStdString(const JSHandle<JSTaggedValue> &str)
{
    ASSERT(str->IsString());
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    return strAccessor.ToStdString();
}

JSHandle<JSTypedArray> JSAPIFastBuffer::NewUint8Array(JSThread *thread, uint32_t length, uint32_t byteOffset)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleTagValFunc = env->GetUint8ArrayFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(handleTagValFunc), handleTagValFunc);
    DataViewType arrayType = DataViewType::UINT8;
    JSTypedArray::Cast(*obj)->SetTypedArrayName(thread, thread->GlobalConstants()->GetUint8ArrayString());
    TypedArrayHelper::AllocateTypedArrayBuffer(thread, obj, length, arrayType);
    auto res = JSHandle<JSTypedArray>(obj);
    if (byteOffset < length) {
        res->SetByteOffset(byteOffset);
    }
    return res;
}

JSTaggedValue JSAPIFastBuffer::Copy(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                    const JSHandle<JSTaggedValue> &src, uint32_t tStart, uint32_t sStart, uint32_t sEnd)
{
    ASSERT(tStart >= 0 && sStart >= 0 && sStart <= sEnd);
    uint32_t copyLength = sEnd - sStart;
    JSHandle<JSTypedArray> srcArray = JSHandle<JSTypedArray>(thread, GetUInt8ArrayFromBufferObject(src));
    JSHandle<JSTypedArray> dstArray = JSHandle<JSTypedArray>(thread, GetUInt8ArrayFromBufferObject(dst));
    uint32_t dstLength = dstArray->GetByteLength();
    if (dstLength <= tStart) {
        return JSTaggedValue(0);
    }
    copyLength = std::min(copyLength, dstLength - tStart);
    if (src->IsJSAPIBuffer()) {
        sStart += JSHandle<JSAPIFastBuffer>(src)->GetOffset();
    }
    if (dst->IsJSAPIBuffer()) {
        tStart += JSHandle<JSAPIFastBuffer>(dst)->GetOffset();
    }
    if (BuiltinsArrayBuffer::IsDetachedBuffer(srcArray->GetViewedArrayBufferOrByteArray())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "srcBuffer is Detached Buffer", JSTaggedValue::Exception());
    }
    if (BuiltinsArrayBuffer::IsDetachedBuffer(dstArray->GetViewedArrayBufferOrByteArray())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "dstBuffer is Detached Buffer", JSTaggedValue::Exception());
    }
    WriteBytes(thread, JSHandle<JSTaggedValue>(thread, srcArray->GetViewedArrayBufferOrByteArray()),
               JSHandle<JSTaggedValue>(thread, dstArray->GetViewedArrayBufferOrByteArray()), sStart, tStart,
               copyLength);
    return JSTaggedValue(copyLength);
}

JSTaggedValue JSAPIFastBuffer::CreateBufferFromArrayLike(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                                         const JSHandle<JSTaggedValue> &obj)
{
    if (!obj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "CreateBufferFromArrayLike must accept object", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, obj, lengthKeyHandle).GetValue();
    JSTaggedNumber number = JSTaggedValue::ToLength(thread, value);
    if (number.GetNumber() > JSObject::MAX_ELEMENT_INDEX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "len is bigger than 2^32 - 1", JSTaggedValue::Exception());
    }
    uint32_t len = number.ToUint32();
    auto fastBufferData = NewUint8Array(thread, len);
    buffer->SetFastBufferData(thread, fastBufferData);
    buffer->SetLength(len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue next = JSTaggedValue::GetProperty(thread, obj, i).GetValue().GetTaggedValue();
        if (!next.IsInt()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "value in arraylike must be a integer", JSTaggedValue::Exception());
        }
        buffer->SetValueByIndex(thread, i, JSTaggedValue(next.GetInt() & StringConverter::LOWER_EIGHT_BITS_MASK),
                                JSType::JS_UINT8_ARRAY);
    }
    return buffer.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::FromArrayBuffer(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                               const JSHandle<JSTaggedValue> &src, uint32_t byteOffset, uint32_t length)
{
    if (BuiltinsArrayBuffer::IsDetachedBuffer(src.GetTaggedValue())) {
        THROW_REFERENCE_ERROR_AND_RETURN(thread, "The srcData is detached buffer.", JSTaggedValue::Exception());
    }
    auto srcBuffer = JSHandle<JSArrayBuffer>(src);
    uint32_t srcLength = srcBuffer->GetArrayBufferByteLength();
    if (srcLength <= byteOffset) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "byteOffset must less than length", JSTaggedValue::Exception());
    }
    auto len = std::min(length, srcLength);
    auto array = NewUint8Array(thread, len);
    array->SetViewedArrayBufferOrByteArray(thread, srcBuffer);
    array->SetByteLength(len);
    array->SetArrayLength(len);
    buffer->SetFastBufferData(thread, array);
    buffer->SetLength(len);
    buffer->SetOffset(byteOffset);
    return buffer.GetTaggedValue();
}

string_view FromStringHex(const JSHandle<JSTaggedValue> &str, string &stringDecoded)
{
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    auto hexStr = strAccessor.ToStdString();
    StringConverter::HexDecode(hexStr, stringDecoded);
    return std::string_view(stringDecoded);
}

bool IsOneByte(uint8_t u8Char)
{
    return (u8Char & 0x80) == 0;
}

void StringConverter::Utf8ToUtf16BEToData(const unsigned char *data, u16string &u16Str, string::size_type &index,
                                          uint8_t &c1)
{
    uint8_t c2 = data[++index];  // The second byte
    uint8_t c3 = data[++index];  // The third byte
    uint8_t c4 = data[++index];  // The forth byte
    // Calculate the UNICODE code point value (3 bits lower for the first byte, 6
    // bits for the other) 3 : shift left 3 times of UTF8_VALID_BITS
    uint32_t codePoint = ((c1 & LOWER_3_BITS_MASK) << (3 * UTF8_VALID_BITS)) |
                         // 2 : shift left 2 times of UTF8_VALID_BITS
                         ((c2 & LOWER_6_BITS_MASK) << (2 * UTF8_VALID_BITS)) |
                         ((c3 & LOWER_6_BITS_MASK) << UTF8_VALID_BITS) | (c4 & LOWER_6_BITS_MASK);
    // In UTF-16, U+10000 to U+10FFFF represent surrogate pairs with two 16-bit
    // units
    if (codePoint >= UTF16_SPECIAL_VALUE) {
        codePoint -= UTF16_SPECIAL_VALUE;
        // 10 : a half of 20 , shift right 10 bits
        u16Str.push_back(static_cast<char16_t>((codePoint >> 10) | HIGH_AGENT_MASK));
        u16Str.push_back(static_cast<char16_t>((codePoint & LOWER_10_BITS_MASK) | LOW_AGENT_MASK));
    } else {  // In UTF-16, U+0000 to U+D7FF and U+E000 to U+FFFF are Unicode code
        // point values
        // assume it does not exist (if any, not encoded)
        u16Str.push_back(static_cast<char16_t>(codePoint));
    }
}

u16string StringConverter::Utf8ToUtf16BE(const string &u8Str, bool *ok)
{
    u16string u16Str = u"";
    u16Str.reserve(u8Str.size());
    string::size_type len = u8Str.length();
    const unsigned char *data = reinterpret_cast<const unsigned char *>(u8Str.data());
    bool isOk = true;
    for (string::size_type i = 0; i < len; ++i) {
        uint8_t c1 = data[i];  // The first byte
        if (IsOneByte(c1)) {   // only 1 byte represents the UNICODE code point
            u16Str.push_back(static_cast<char16_t>(c1));
            continue;
        }
        switch (c1 & HIGER_4_BITS_MASK) {
            case FOUR_BYTES_STYLE: {  // 4 byte characters, from 0x10000 to 0x10FFFF
                Utf8ToUtf16BEToData(data, u16Str, i, c1);
                break;
            }
            case THREE_BYTES_STYLE: {    // 3 byte characters, from 0x800 to 0xFFFF
                uint8_t c2 = data[++i];  // The second byte
                uint8_t c3 = data[++i];  // The third byte
                // Calculates the UNICODE code point value
                // (4 bits lower for the first byte, 6 bits lower for the other)
                // 2 : shift left 2 times of UTF8_VALID_BITS
                uint32_t codePoint = ((c1 & LOWER_4_BITS_MASK) << (2 * UTF8_VALID_BITS)) |
                                     ((c2 & LOWER_6_BITS_MASK) << UTF8_VALID_BITS) | (c3 & LOWER_6_BITS_MASK);
                u16Str.push_back(static_cast<char16_t>(codePoint));
                break;
            }
            case TWO_BYTES_STYLE1:  // 2 byte characters, from 0x80 to 0x7FF
            case TWO_BYTES_STYLE2: {
                uint8_t c2 = data[++i];  // The second byte
                // Calculates the UNICODE code point value
                // (5 bits lower for the first byte, 6 bits lower for the other)
                uint32_t codePoint = ((c1 & LOWER_5_BITS_MASK) << UTF8_VALID_BITS) | (c2 & LOWER_6_BITS_MASK);
                u16Str.push_back(static_cast<char16_t>(codePoint));
                break;
            }
            default: {
                isOk = false;
                break;
            }
        }
    }
    if (ok != nullptr) {
        *ok = isOk;
    }
    return u16Str;
}

u16string StringConverter::Utf16BEToLE(const u16string &wstr)
{
    u16string str16 = u"";
    const char16_t *data = wstr.data();
    for (unsigned int i = 0; i < wstr.length(); i++) {
        char16_t wc = data[i];
        char16_t high = (wc >> 8) & 0x00FF;
        char16_t low = wc & 0x00FF;
        char16_t c16 = (low << 8) | high;
        str16.push_back(c16);
    }
    return str16;
}

string StringConverter::Utf16BEToANSI(const u16string &wstr)
{
    string ret = "";
    for (u16string::const_iterator it = wstr.begin(); it != wstr.end(); ++it) {
        char16_t wc = (*it);
        // get the lower bit from the UNICODE code point
        char c = static_cast<char>(wc & LOWER_8_BITS_MASK);
        ret.push_back(c);
    }
    return ret;
}

string StringConverter::Utf8ToUtf16BEToANSI(const string &str)
{
    u16string u16Str = Utf8ToUtf16BE(str);
    string ret = Utf16BEToANSI(u16Str);
    return ret;
}

string JSAPIFastBuffer::GetString(const JSHandle<JSTaggedValue> &str, EncodingType encodingType)
{
    string_view data;
    string strDecoded;
    switch (encodingType) {
        case UTF8:
            strDecoded = FromStringUtf8(str);
            data = strDecoded;
            break;
        case ASCII:
        case LATIN1:
        case BINARY:
            data = FromStringASCII(str, strDecoded);
            break;
        case UTF16LE:
            data = FromStringUtf16(str, strDecoded);
            break;
        case BASE64:
        case BASE64URL:
            data = FromStringBase64(str, strDecoded);
            break;
        case HEX:
            data = FromStringHex(str, strDecoded);
            break;
        default:
            return "";
    }
    return strDecoded;
}

string JSAPIFastBuffer::GetString(const JSHandle<JSTaggedValue> &str, JSHandle<JSTaggedValue> encoding)
{
    EncodingType encodingType = UTF8;
    encodingType = GetEncodingType(ConvertEcmaStringToStdString(encoding));
    return GetString(str, encodingType);
}

JSTaggedValue JSAPIFastBuffer::FromString(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                          const JSHandle<JSTaggedValue> &str, const JSHandle<JSTaggedValue> &encoding)
{
    if (encoding->IsUndefined() || encoding->IsNull()) {
        return FromString(thread, buffer, str);
    }
    EncodingType encodingType = GetEncodingType(encoding);
    return FromString(thread, buffer, str, encodingType);
}

JSTaggedValue JSAPIFastBuffer::FromString(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                          const JSHandle<JSTaggedValue> &str, EncodingType encoding)
{
    string_view data;
    string strDecoded;
    switch (encoding) {
        case UTF8:
            strDecoded = FromStringUtf8(str);
            data = strDecoded;
            break;
        case ASCII:
        case LATIN1:
        case BINARY:
            data = FromStringASCII(str, strDecoded);
            break;
        case UTF16LE:
            data = FromStringUtf16(str, strDecoded);
            break;
        case BASE64:
        case BASE64URL:
            data = FromStringBase64(str, strDecoded);
            break;
        case HEX:
            data = FromStringHex(str, strDecoded);
            break;
        default:
            THROW_RANGE_ERROR_AND_RETURN(thread, "Not supported encodingType", JSTaggedValue::Exception());
    }
    // using data to new buffer
    uint32_t length = data.length();
    JSHandle<JSTypedArray> array = NewUint8Array(thread, length);
    buffer->SetFastBufferData(thread, array);
    buffer->SetLength(length);
    auto *dst = builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(array->GetViewedArrayBufferOrByteArray(),
                                                                      buffer->GetOffset());
    WriteBytes(thread, reinterpret_cast<const uint8_t *>(data.data()), length, reinterpret_cast<uint8_t *>(dst));
    return buffer.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::ToString(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, EncodingType encodingType,
                                        uint32_t start, uint32_t end)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!RangeChecker(0, buffer->GetLength(), start, end)) {
        std::ostringstream oss;
        oss << "Buffer ToString parameter are out of range";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    auto arrayBuffer = GetArrayBuffer(thread, buffer);
    if (BuiltinsArrayBuffer::IsDetachedBuffer(arrayBuffer)) {
        std::ostringstream oss;
        oss << "The arraybuffer is detached";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::REFERENCE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    uint8_t *data = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(arrayBuffer, buffer->GetOffset() + start));
    uint32_t len = end - start;
    string strDecoded;
    switch (encodingType) {
        case UTF8:
            return factory->NewFromUtf8WithoutStringTable(string_view(reinterpret_cast<const char *>(data), len))
                .GetTaggedValue();
        case ASCII:
            return factory->NewFromASCII(string_view(reinterpret_cast<const char *>(data), len)).GetTaggedValue();
        case UTF16LE:
            // 2 : uint16_t is double of uint8_t
            return factory->NewFromUtf16(reinterpret_cast<uint16_t *>(data), len / 2).GetTaggedValue();
        case LATIN1:
        case BINARY:
            StringConverter::Latin1Encode(reinterpret_cast<unsigned char *>(data), len, strDecoded);
            break;
        case BASE64:
        case BASE64URL:
            StringConverter::Base64Encode(reinterpret_cast<unsigned char *>(data), len, strDecoded,
                                          encodingType == BASE64URL);
            break;
        case HEX:
            StringConverter::HexEncode(reinterpret_cast<unsigned char *>(data), len, strDecoded);
            break;
        default:
            THROW_RANGE_ERROR_AND_RETURN(thread, "Not supported encodingType", JSTaggedValue::Exception());
    }
    return factory->NewFromUtf8(strDecoded).GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::WriteString(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                           JSHandle<JSTaggedValue> &value, uint32_t offset, uint32_t maxLen,
                                           EncodingType encoding)
{
    ASSERT(value->IsString());
    uint32_t bufLength = buffer->GetLength();
    if (offset > bufLength) {
        return JSTaggedValue(0);
    }
    maxLen = std::min(maxLen, bufLength - offset);
    string_view data;
    string strDecoded;  // data = string_view(strDecoded)
    switch (encoding) {
        case UTF8:
            strDecoded = FromStringUtf8(value);
            data = strDecoded;
            break;
        case ASCII:
        case LATIN1:
        case BINARY:
            data = FromStringASCII(value, strDecoded);
            break;
        case UTF16LE:
            strDecoded = FromStringUtf16(value, strDecoded);
            data = strDecoded;
            break;
        case BASE64:
        case BASE64URL:
            data = FromStringBase64(value, strDecoded);
            break;
        case HEX:
            data = FromStringHex(value, strDecoded);
            break;
        default:
            THROW_RANGE_ERROR_AND_RETURN(thread, "Not supported encodingType", JSTaggedValue::Exception());
    }
    uint32_t strLength = data.length();
    maxLen = std::min(maxLen, strLength);
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>(thread, GetUInt8ArrayFromBufferObject(buffer.GetTaggedValue()));
    auto *dst = static_cast<uint8_t *>(builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(
        typedArray->GetViewedArrayBufferOrByteArray(), buffer->GetOffset() + offset));
    WriteBytes(thread, reinterpret_cast<const uint8_t *>(data.data()), maxLen, reinterpret_cast<uint8_t *>(dst));
    return JSTaggedValue(maxLen);
}

JSTaggedValue JSAPIFastBuffer::AllocateFastBuffer(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                                  uint32_t byteLength, uint32_t byteOffset)
{
    JSHandle<JSTypedArray> handleUint8Array = JSAPIFastBuffer::NewUint8Array(thread, byteLength, byteOffset);
    buffer->SetFastBufferData(thread, handleUint8Array);
    buffer->SetLength(byteLength);
    return buffer.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::AllocateFromBufferObject(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                                        const JSHandle<JSTaggedValue> &src, uint32_t byteLength,
                                                        uint32_t byteOffset)
{
    auto handleUint8Array = JSTaggedValue(GetUInt8ArrayFromBufferObject(src));
    buffer->SetFastBufferData(thread, handleUint8Array);
    buffer->SetLength(byteLength);
    buffer->SetOffset(byteOffset);
    return buffer.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::GetArrayBuffer(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer)
{
    // if buffer size smaller than Buffer.poolSize, it may return the pool arraybuffer.
    auto array = JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    return JSTypedArray::GetOffHeapBuffer(thread, array);
}

// return a JSArray copied from buffer.Uint8Array
JSTaggedValue JSAPIFastBuffer::FromBufferToArray(JSThread *thread, JSHandle<JSTaggedValue> &value)
{
    auto buffer = JSHandle<JSAPIFastBuffer>(value);
    int32_t length = static_cast<int32_t>(buffer->GetLength());
    JSHandle<JSTaggedValue> array = JSArray::ArrayCreate(thread, JSTaggedNumber(length));
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    for (int i = 0; i < length; i++) {
        JSTaggedValue bufferValue = GetValueByIndex(thread, typedArray.GetTaggedValue(), i, JSType::JS_UINT8_ARRAY);
        JSArray::FastSetPropertyByValue(thread, array, i, JSHandle<JSTaggedValue>(thread, bufferValue));
    }
    return array.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::Entries(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer)
{
    auto array = JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayIterator> iter(
        factory->NewJSArrayIterator(JSHandle<JSObject>(thread, array.GetTaggedValue()), IterationKind::KEY_AND_VALUE));
    return iter.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::Keys(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer)
{
    auto array = JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayIterator> iter(
        factory->NewJSArrayIterator(JSHandle<JSObject>(thread, array.GetTaggedValue()), IterationKind::KEY));
    return iter.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::Values(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer)
{
    auto array = JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayIterator> iter(
        factory->NewJSArrayIterator(JSHandle<JSObject>(thread, array.GetTaggedValue()), IterationKind::VALUE));
    return iter.GetTaggedValue();
}

std::string StringConverter::Utf16StrToStr(std::u16string &value)
{
    string str = "";
    const char16_t *data = reinterpret_cast<const char16_t *>(value.data());
    for (unsigned int i = 0; i < value.length(); i++) {
        char16_t c = data[i];
        char high = static_cast<char>((c >> 8) & 0x00FF);
        char low = static_cast<char>(c & 0x00FF);
        str.push_back(low);
        str.push_back(high);
    }
    return str;
}

JSTaggedValue JSAPIFastBuffer::FillString(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                          JSHandle<JSTaggedValue> &valueHandle, EncodingType encoding, uint32_t start,
                                          uint32_t end)
{
    string str = GetString(valueHandle, encoding);
    if (str.length() == 0) {
        str.push_back(0);
    }
    return WriteStringLoop(thread, buffer, str, start, end);
}

JSTaggedValue JSAPIFastBuffer::Fill(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                    JSHandle<JSTaggedValue> &valueHandle, EncodingType encoding, uint32_t start,
                                    uint32_t end)
{
    if (valueHandle->IsNumber()) {
        if (!valueHandle->IsInt()) {
            std::ostringstream oss;
            oss << "Buffer fill number shoule be a integer";
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, oss.str().c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Undefined());
        }
        // only use low 8 bits of value
        uint8_t value = static_cast<uint8_t>(valueHandle->GetInt() & 255);
        uint32_t length = end - start;
        JSHandle<JSTypedArray> typedArray =
            JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
        uint8_t *data = GetBufferData(buffer, start);
        if (memset_s(data, length, value, length) != EOK) {
            std::ostringstream oss;
            oss << "Buffer fill number memset failed";
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::REFERENCE_ERROR, oss.str().c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Undefined());
        }
    } else if (valueHandle->IsString()) {
        FillString(thread, buffer, valueHandle, encoding, start, end);
    } else if (valueHandle->IsJSAPIBuffer() || valueHandle->IsTypedArray()) {
        WriteBufferObjectLoop(buffer, valueHandle, start, end);
    }
    return buffer.GetTaggedValue();
}

JSTaggedValue JSAPIFastBuffer::Compare(JSThread *thread, JSHandle<JSAPIFastBuffer> &a, JSHandle<JSTaggedValue> &bObj,
                                       uint32_t sStart, uint32_t sEnd, uint32_t tStart, uint32_t tEnd)
{
    JSHandle<JSTypedArray> typedArrayA =
        JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(a->GetFastBufferData().GetTaggedObject()));
    JSHandle<JSTypedArray> typedArrayB = JSHandle<JSTypedArray>(thread, GetUInt8ArrayFromBufferObject(bObj));
    uint32_t aCmpLength = sEnd - sStart;
    uint32_t bCmpLength = tEnd - tStart;
    if (typedArrayA->GetByteLength() < aCmpLength || typedArrayB->GetByteLength() < bCmpLength) {
        std::ostringstream oss;
        oss << "Buffer Compare parameters are out of range.";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> bufferA =
        JSHandle<JSTaggedValue>(thread, JSTypedArray::GetOffHeapBuffer(thread, typedArrayA));
    JSHandle<JSTaggedValue> bufferB =
        JSHandle<JSTaggedValue>(thread, JSTypedArray::GetOffHeapBuffer(thread, typedArrayB));
    // If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(bufferA.GetTaggedValue()) ||
        BuiltinsArrayBuffer::IsDetachedBuffer(bufferA.GetTaggedValue())) {
        THROW_REFERENCE_ERROR_AND_RETURN(thread, "buffer is Detached Buffer", JSTaggedValue::Exception());
    }
    auto aPointer = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(bufferA.GetTaggedValue(), sStart));
    auto bPointer = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(bufferB.GetTaggedValue(), tStart));
    if (aPointer == nullptr || bPointer == nullptr) {
        THROW_REFERENCE_ERROR_AND_RETURN(thread, "Buffer Compare memory ptr is nullptr.", JSTaggedValue::Exception());
    }
    uint32_t length = std::min(aCmpLength, bCmpLength);
    int32_t ret = memcmp(aPointer, bPointer, length);
    if (ret == 0) {
        if (aCmpLength != bCmpLength) {
            return aCmpLength < bCmpLength ? JSTaggedValue(-1) : JSTaggedValue(1);
        }
        return JSTaggedValue(0);
    }
    return ret < 0 ? JSTaggedValue(-1) : JSTaggedValue(1);
}

JSTaggedValue JSAPIFastBuffer::Includes(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                        JSHandle<JSTaggedValue> valueHandle, uint32_t start, EncodingType encoding)
{
    auto res = IndexOf(thread, buffer, valueHandle, start, encoding, false);
    auto resIndex = res.GetInt();
    return resIndex == -1 ? JSTaggedValue::False() : JSTaggedValue::True();
}

int32_t JSAPIFastBuffer::StringMatch(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, const uint8_t *str,
                                     uint32_t start, uint32_t strLength, bool isReverse)
{
    if (buffer->GetLength() < strLength) {
        return -1;
    }
    auto array = JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    JSHandle<JSTaggedValue> srcBuffer = JSHandle<JSTaggedValue>(thread, array->GetViewedArrayBufferOrByteArray());
    // If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
        THROW_REFERENCE_ERROR_AND_RETURN(thread, "buffer is Detached Buffer", false);
    }
    int32_t offset = static_cast<int32_t>(buffer->GetOffset());
    auto src = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(srcBuffer.GetTaggedValue(), offset));
    uint32_t end = buffer->GetLength() - strLength;
    // reverse is mean enumeration order
    if (isReverse) {
        for (int32_t i = std::min(end, start); i >= 0; --i) {
            if (memcmp(str, src + i, strLength) == 0) {
                return i;
            }
        }
    } else {
        for (uint32_t i = start; i <= end; ++i) {
            if (memcmp(str, src + i, strLength) == 0) {
                return i;
            }
        }
    }
    return -1;
}

JSTaggedValue JSAPIFastBuffer::IndexOf(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                       JSHandle<JSTaggedValue> valueHandle, uint32_t start, EncodingType encoding,
                                       bool isReverse)
{
    if (!valueHandle->IsNumber()) {
        uint32_t len;
        const uint8_t *data;
        if (valueHandle->IsString()) {
            auto str = GetString(valueHandle, encoding);
            data = reinterpret_cast<const uint8_t *>(str.data());
            len = str.length();
        } else if (valueHandle->IsJSAPIBuffer() || valueHandle->IsJSUint8Array()) {
            auto srcData = JSHandle<JSTaggedValue>(
                thread, GetUInt8ArrayFromBufferObject(valueHandle)->GetViewedArrayBufferOrByteArray());
            data = reinterpret_cast<uint8_t *>(
                builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(srcData.GetTaggedValue()));
            len = GetUInt8ArrayFromBufferObject(valueHandle)->GetByteLength();
        } else {
            THROW_TYPE_ERROR_AND_RETURN(thread, "IndexOf value invalid.", JSTaggedValue::Exception());
        }
        auto res = StringMatch(thread, buffer, data, start, len, isReverse);
        return JSTaggedValue(res);
    }
    if (!valueHandle->IsInt()) {
        std::ostringstream oss;
        oss << "Buffer includes value shoule be integer when value is number";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    uint8_t value = static_cast<uint8_t>(valueHandle->GetInt() & 255);
    uint32_t length = buffer->GetLength();
    if (isReverse) {
        for (int32_t i = std::min(length - 1, start); i >= 0; --i) {
            if (value == ReadUInt8(thread, buffer, i, true).GetInt()) {
                return JSTaggedValue(i);
            }
        }
    } else {
        for (uint32_t i = start; i < length; ++i) {
            if (value == ReadUInt8(thread, buffer, i, true).GetInt()) {
                return JSTaggedValue(i);
            }
        }
    }
    return JSTaggedValue(-1);  // value not found
}

bool JSAPIFastBuffer::WriteBytes(JSThread *thread, const uint8_t *src, unsigned int size, uint8_t *dest)
{
    if (src == nullptr || dest == nullptr) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    if (memmove_s(dest, size, src, size) != EOK) {
        std::ostringstream oss;
        oss << "Buffer WriteBytes memmove_s failed";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }
    return true;
}

bool JSAPIFastBuffer::WriteDataLoop(const uint8_t *src, uint8_t *dest, uint32_t length, uint32_t start, uint32_t end)
{
    if (end - start <= 0 || length == 0) {
        return false;
    }
    dest += start;
    end -= start;
    uint32_t pos = 0;
    uint32_t copySize = std::min(length, end);
    if (memmove_s(dest, copySize, src, copySize) != EOK) {
        return false;
    }
    pos += copySize;
    while (pos < end) {
        copySize = std::min(copySize, end - pos);
        if (memmove_s(dest + pos, copySize, dest, copySize) != EOK) {
            return false;
        }
        pos += copySize;
        copySize <<= 1;
    }
    return true;
}

JSTaggedValue JSAPIFastBuffer::WriteStringLoop(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, string_view data,
                                               uint32_t start, uint32_t end)
{
    uint32_t length = data.length();
    auto dstData = JSHandle<JSTaggedValue>(
        thread, GetUInt8ArrayFromBufferObject(buffer.GetTaggedValue())->GetViewedArrayBufferOrByteArray());
    uint8_t *dst = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(dstData.GetTaggedValue(), buffer->GetOffset()));
    uint8_t *str = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(data.data()));
    if (!WriteDataLoop(str, dst, length, start, end)) {
        std::ostringstream oss;
        oss << "Buffer WriteStringLoop failed.";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::REFERENCE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSTaggedValue(length);
}

JSTaggedValue JSAPIFastBuffer::WriteBufferObjectLoop(JSHandle<JSAPIFastBuffer> &buffer, JSHandle<JSTaggedValue> &srcObj,
                                                     uint32_t start, uint32_t end)
{
    ASSERT(end - start <= buffer->GetLength());
    auto srcArray = GetUInt8ArrayFromBufferObject(srcObj);
    uint32_t length = srcArray->GetByteLength();
    auto srcData = srcArray->GetViewedArrayBufferOrByteArray();
    uint8_t *src = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(srcData, buffer->GetOffset()));
    uint8_t *dst = GetBufferData(buffer, 0);
    WriteDataLoop(src, dst, length, start, end);
    return JSTaggedValue(length);
}

bool JSAPIFastBuffer::WriteBytes(JSThread *thread, JSHandle<JSTaggedValue> srcData, JSHandle<JSTaggedValue> dstData,
                                 uint32_t sStart, uint32_t tStart, uint32_t size)
{
    ASSERT(srcData->IsByteArray() || srcData->IsArrayBuffer());
    ASSERT(dstData->IsByteArray() || dstData->IsArrayBuffer());
    auto src = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(srcData.GetTaggedValue(), sStart));
    auto dst = reinterpret_cast<uint8_t *>(
        builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(dstData.GetTaggedValue(), tStart));
    return WriteBytes(thread, src, size, dst);
}

JSTaggedValue JSAPIFastBuffer::SetValueByIndex(JSThread *thread, const JSTaggedValue typedarray, uint32_t index,
                                               JSTaggedValue value, JSType valueType, bool littleEndian)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(typedarray.IsJSUint8Array());
    JSTypedArray *typedarrayObj = JSTypedArray::Cast(typedarray.GetTaggedObject());
    if (UNLIKELY(value.IsECMAObject())) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue buffer = typedarrayObj->GetViewedArrayBufferOrByteArray();
    DataViewType elementType = TypedArrayHelper::GetType(valueType);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    return BuiltinsArrayBuffer::SetValueInBuffer(thread, buffer, index, elementType, valueHandle, littleEndian);
}

JSTaggedValue JSAPIFastBuffer::GetValueByIndex(JSThread *thread, const JSTaggedValue typedarray, uint32_t index,
                                               JSType jsType, bool littleEndian)
{
    ASSERT(typedarray.IsTypedArray() || typedarray.IsSharedTypedArray());
    // Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTypedArray *typedarrayObj = JSTypedArray::Cast(typedarray.GetTaggedObject());
    JSTaggedValue buffer = typedarrayObj->GetViewedArrayBufferOrByteArray();
    if (buffer.IsArrayBuffer() && BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue::Undefined();
    }
    DataViewType elementType = TypedArrayHelper::GetType(jsType);
    return BuiltinsArrayBuffer::GetValueFromBuffer(thread, buffer, index, elementType, littleEndian);
}

OperationResult JSAPIFastBuffer::GetProperty(JSThread *thread, const JSHandle<JSAPIFastBuffer> &obj,
                                             const JSHandle<JSTaggedValue> &key)
{
    int length = obj->GetLength();
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 and <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
                                         OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    return OperationResult(thread, ReadUInt8(thread, obj, index, true), PropertyMetaData(false));
}

bool JSAPIFastBuffer::SetProperty(JSThread *thread, const JSHandle<JSAPIFastBuffer> &obj,
                                  const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    int length = obj->GetLength();
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        return false;
    }
    WriteUInt8(thread, obj, value, index, true);
    return true;
}

void StringConverter::Base64Encode(const unsigned char *src, uint32_t len, string &outStr, bool isUrl)
{
    if (!len) {
        outStr = "";
        return;
    }
    char *out = nullptr;
    char *pos = nullptr;
    const unsigned char *pEnd = nullptr;
    const unsigned char *pStart = nullptr;
    size_t outLen = 4 * ((len + 2) / 3);  // 3-byte blocks to 4-byte

    outStr.resize(outLen);
    out = outStr.data();

    pEnd = src + len;
    pStart = src;
    pos = out;
    const char *table = isUrl ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
                              : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    // 3 : 3 bytes is just 24 bits which is 4 times of 6 bits
    while (pEnd - pStart >= 3) {
        // 2 : add two zeros in front of the first set of 6 bits to become a new 8 binary bits
        *pos = table[pStart[0] >> 2];
        // 4 : add two zeros in front of the following second set of 6 bits to become the new 8 binary bits
        *(pos + 1) = table[((pStart[0] & LOWER_2_BITS_MASK) << 4) | (pStart[1] >> 4)];  // 1 means the first encoded
        // 2 : 4 : 6 : add two zeros in front of the following third set of 6 bits to become the new 8 binary bits
        *(pos + 2) = table[((pStart[1] & LOWER_4_BITS_MASK) << 2) | (pStart[2] >> 6)];
        // 2 : 3 : add two zeros in front of the following forth set of 6 bits to become the new 8 binary bits
        *(pos + 3) = table[pStart[2] & LOWER_6_BITS_MASK];
        // 4 : the pointer of pos scrolls off 4 bytes to point the next 4 bytes of encoded chars
        pos += 4;
        // 3 : the pointer of pStart scrolls off 3 bytes to point the next 3 bytes of which will be encoded chars
        pStart += 3;
    }
    if (pEnd - pStart > 0) {
        // 2 : add two zeros in front of the first set of 6 bits to become a new 8 binary bits
        *pos = table[pStart[0] >> 2];
        if (pEnd - pStart == 1) {
            // 4 : paddle the last two bits of the last byte with two zeros in front of it and four zeros after it
            *(pos + 1) = table[(pStart[0] & LOWER_2_BITS_MASK) << 4];
            // 2 : fill in the missing bytes with '='
            *(pos + 2) = '=';
        } else {
            // 4 : add two zeros in front of the second set of 6 bits to become the new 8 binary bits
            *(pos + 1) = table[((pStart[0] & LOWER_2_BITS_MASK) << 4) | (pStart[1] >> 4)];
            // 2 : paddle the last four bits of the last byte with two zeros in front of it and two zeros after it
            *(pos + 2) = table[(pStart[1] & LOWER_4_BITS_MASK) << 2];
        }
        // 3 : fill in the missing bytes with '='
        *(pos + 3) = '=';
    }

    if (isUrl) {
        size_t poss = outStr.find_last_not_of('=');
        if (poss != std::string::npos) {
            outStr.erase(poss + 1);
        }
    }
}

void DecodeBase64CharSlow(const uint8_t *data, string &ret, size_t len, uint32_t &cursor, uint32_t &retIter)
{
    auto &table = BASE64_TABLE;
    size_t count = 0;
    // 4 : four char to decode each time
    CVector<unsigned char> temp(4, 0);
    // 4 : try to map 4 index from base64 table
    while (cursor < len && count < 4) {
        unsigned char ch = table[data[cursor++]];
        if (ch > StringConverter::LOWER_6_BITS_MASK) {
            continue;
        }
        temp[count++] = ch;
    }
    if (count > JSAPIFastBuffer::OneByte) {
        ret[retIter] =
            // 2 : 4 : two bits(except two higer bits) of the second byte, combine them to a new byte
            ((temp[0] & StringConverter::LOWER_6_BITS_MASK) << 2) | ((temp[JSAPIFastBuffer::OneByte] & 0x30) >> 4);
        ++retIter;
    }
    if (count > JSAPIFastBuffer::TwoBytes) {
        // 4 : 2 : four bits(except two higer bits) of the third byte, combine them to a new byte
        ret[retIter] = ((temp[JSAPIFastBuffer::OneByte] & StringConverter::LOWER_4_BITS_MASK) << 4) |
                       // 2 : (except two higer bits)
                       ((temp[JSAPIFastBuffer::TwoBytes] & StringConverter::MIDDLE_4_BITS_MASK) >> 2);
        ++retIter;
    }
    if (count > JSAPIFastBuffer::ThreeBytes) {
        // 2 : 3 : 6 : combine them to a new byte
        ret[retIter] = ((temp[JSAPIFastBuffer::TwoBytes] & StringConverter::LOWER_2_BITS_MASK) << 6) |
                       (temp[JSAPIFastBuffer::ThreeBytes] & StringConverter::LOWER_6_BITS_MASK);
        ++retIter;
    }
}

void StringConverter::Base64Decode(string_view encodedStr, string &ret)
{
    size_t len = encodedStr.length();
    unsigned int cursor = 0;
    unsigned char charArray4[4] = {0};
    // why upperLength: len = ceil(decodedString.length / 3) * 4
    // upperLength = len / 4 * 3 = ceil(decodedString.length / 3) * 3 >=
    // decodedString.length
    uint32_t upperLength = (len + 3) / 4 * 3;
    ret.reserve(upperLength);
    ret.resize(upperLength);
    uint32_t maxLen = len / 4 * 4;
    uint32_t retIter = 0;
    auto &table = BASE64_TABLE;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(encodedStr.data());
    while (cursor < maxLen) {
        // 0 : the first index map to base64 table
        charArray4[0] = static_cast<unsigned char>(table[data[cursor]]);
        // 1 : the second index map to base64 table
        charArray4[1] = static_cast<unsigned char>(table[data[cursor + 1]]);
        // 2 : the third index map to base64 table
        charArray4[2] = static_cast<unsigned char>(table[data[cursor + 2]]);
        // 3 : the fourth index map to base64 table
        charArray4[3] = static_cast<unsigned char>(table[data[cursor + 3]]);
        // 0 : 1 : 2 : 3 : check if invalid index exist
        if ((charArray4[0] | charArray4[1] | charArray4[2] | charArray4[3]) > LOWER_6_BITS_MASK) {
            DecodeBase64CharSlow(data, ret, len, cursor, retIter);
            continue;
        }
        // 1 : result first char
        // 2 : 4 : two bits(except two higer bits) of the second byte, combine them to a new byte
        ret[retIter] = ((charArray4[0] & LOWER_6_BITS_MASK) << 2) | ((charArray4[1] & 0x30) >> 4);
        // 1 : 2 : result second char
        // 4 : 2 : four bits(except two higer bits) of the third byte, combine them to a new byte
        ret[retIter + 1] = ((charArray4[1] & LOWER_4_BITS_MASK) << 4) | ((charArray4[2] & MIDDLE_4_BITS_MASK) >> 2);
        // 2 : 3 : result third char
        // 2 : 3 : 6 : combine them to a new byte
        ret[retIter + 2] = ((charArray4[2] & LOWER_2_BITS_MASK) << 6) | (charArray4[3] & LOWER_6_BITS_MASK);
        // 3 : every time 3 bytes
        retIter += 3;
        // 4 : every time 4 bytes
        cursor += 4;
    }
    if (cursor < len) {
        DecodeBase64CharSlow(data, ret, len, cursor, retIter);
    }
    ret.resize(retIter);
}

// encoding bytesflow(latin1) to utf8
void StringConverter::Latin1Encode(const unsigned char *data, uint32_t len, std::string &ret)
{
    // 2 : reserve enough space to encode
    ret.reserve(len * 2);
    for (uint32_t i = 0; i < len; i++) {
        unsigned char c = data[i];
        if (c < 0x80) {
            ret.push_back(c);
        } else {
            ret.push_back(0xc0 | (c >> UTF8_VALID_BITS));
            ret.push_back(0x80 | (c & 0x3f));
        }
    }
}

bool IsValidHex(const string &hex)
{
    if (!hex.length()) {
        return false;
    }
    for (unsigned int i = 0; i < hex.size(); i++) {
        char c = hex.at(i);
        // 0 ~ 9, A ~ F, a ~ f
        if (!(c <= '9' && c >= '0') && !(c <= 'F' && c >= 'A') && !(c <= 'f' && c >= 'a')) {
            return false;
        }
    }
    return true;
}

void StringConverter::HexEncode(const unsigned char *hexStr, uint32_t len, string &ret)
{
    // 2 : reserve enough length to encode
    ret.reserve(2 * len);
    for (uint32_t i = 0; i < len; i++) {
        uint8_t high = hexStr[i] >> 4;
        uint8_t low = hexStr[i] & 0x0f;
        // 9 : 10 : convert high to hex
        ret.push_back(high > 9 ? 'a' + (high % 10) : '0' + high);
        // 9 : 10 : convert low to hex
        ret.push_back(low > 9 ? 'a' + (low % 10) : '0' + low);
    }
}

void StringConverter::HexDecode(string_view hexStr, string &ret)
{
    unsigned int arrSize = hexStr.size();
    // 2 : 1 : means a half length of hex str's size
    ret.reserve(arrSize / 2);

    string hexStrTmp;
    int num = 0;
    // 2 : hex string to stoi
    hexStrTmp.resize(2);
    // 2 : means a half length of hex str's size
    for (unsigned int i = 0; i < arrSize / 2; i++) {
        // 2 : offset is i * 2
        hexStrTmp[0] = hexStr[i * 2];
        // 2 : offset is i * 2 + 1
        hexStrTmp[1] = hexStr[i * 2 + 1];
        if (!IsValidHex(hexStrTmp)) {
            break;
        }
        // 16 : the base is 16
        num = stoi(hexStrTmp, nullptr, 16);
        ret.push_back(static_cast<char>(num));
    }
}

std::pair<JSTaggedValue, JSTaggedValue> SplitUInt(JSHandle<JSTaggedValue> &value, uint64_t byteLength)
{
    ASSERT(value->IsNumber());
    uint64_t temp;
    if (value->IsDouble()) {
        temp = value->GetDouble();
        if (UNLIKELY(static_cast<uint64_t>(temp) != temp)) {
            return {JSTaggedValue::Undefined(), JSTaggedValue::Exception()};
        }
        temp = static_cast<uint64_t>(temp);
    } else {
        temp = value->GetInt();
    }
    byteLength *= JSAPIFastBuffer::ONE_BYTE_BIT_LENGTH;
    uint64_t bitMask = static_cast<uint64_t>(static_cast<uint64_t>(1) << byteLength) - 1;
    auto l = JSTaggedValue(static_cast<uint32_t>(temp & bitMask));
    auto r = JSTaggedValue(static_cast<uint32_t>(temp >> byteLength));
    return std::make_pair(l, r);
}

JSTaggedValue MergeUInt(JSTaggedValue low, JSTaggedValue high, uint64_t byteLengthLow)
{
    byteLengthLow *= JSAPIFastBuffer::ONE_BYTE_BIT_LENGTH;
    uint64_t lowValue = 0;
    uint64_t highValue = 0;
    lowValue |= static_cast<uint64_t>(low.GetNumber());
    highValue |= static_cast<uint64_t>(high.GetNumber());
    return JSTaggedValue(static_cast<double>((highValue << byteLengthLow) | lowValue));
}

JSTaggedValue JSAPIFastBuffer::WriteBytesValue(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer,
                                               JSHandle<JSTaggedValue> &value, uint32_t offset, ByteLength byteLength,
                                               bool littleEndian)
{
    if (UNLIKELY(byteLength > SixBytes)) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR,
                                                            "WriteInt or WriteUInt only support 1 byte to 6 bytes");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    switch (byteLength) {
        case OneByte:
            return WriteUInt8(thread, buffer, value, offset, littleEndian);
        case TwoBytes:
            return WriteUInt16(thread, buffer, value, offset, littleEndian);
        case FourBytes:
            return WriteUInt32(thread, buffer, value, offset, littleEndian);
        default:
            break;
    }
    if (byteLength > FourBytes) {
        std::pair<JSTaggedValue, JSTaggedValue> valuePair = SplitUInt(value, ThreeBytes);
        if (valuePair.first == JSTaggedValue::Undefined()) {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR,
                                                                "WriteInt or WriteUInt only support Integer Type");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        JSHandle<JSTaggedValue> lowValue = JSHandle<JSTaggedValue>(thread, valuePair.first);
        JSHandle<JSTaggedValue> highValue = JSHandle<JSTaggedValue>(thread, valuePair.second);
        ByteLength highBytes = static_cast<ByteLength>(byteLength - ThreeBytes);
        if (littleEndian) {
            auto nextIndex = WriteBytesValue(thread, buffer, lowValue, offset, ThreeBytes, littleEndian);
            return WriteBytesValue(thread, buffer, highValue, nextIndex.GetInt(), highBytes, littleEndian);
        }
        auto nextIndex = WriteBytesValue(thread, buffer, highValue, offset, highBytes, littleEndian);
        return WriteBytesValue(thread, buffer, lowValue, nextIndex.GetInt(), ThreeBytes, littleEndian);
    }
    std::pair<JSTaggedValue, JSTaggedValue> valuePair = SplitUInt(value, TwoBytes);
    if (valuePair.first == JSTaggedValue::Undefined()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR,
                                                            "WriteInt or WriteUInt only support Integer Type");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> lowValue = JSHandle<JSTaggedValue>(thread, valuePair.first);
    JSHandle<JSTaggedValue> highValue = JSHandle<JSTaggedValue>(thread, valuePair.second);
    ByteLength highBytes = static_cast<ByteLength>(byteLength - TwoBytes);
    if (littleEndian) {
        auto nextIndex = WriteBytesValue(thread, buffer, lowValue, offset, TwoBytes, littleEndian);
        return WriteBytesValue(thread, buffer, highValue, nextIndex.GetInt(), highBytes, littleEndian);
    }
    auto nextIndex = WriteBytesValue(thread, buffer, highValue, offset, highBytes, littleEndian);
    return WriteBytesValue(thread, buffer, lowValue, nextIndex.GetInt(), TwoBytes, littleEndian);
}

JSTaggedValue JSAPIFastBuffer::ReadBytes(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                         ByteLength byteLength, bool littleEndian)
{
    if (UNLIKELY(byteLength > SixBytes)) {
        std::ostringstream oss;
        oss << "ReadInt or ReadUInt only support 1 byte to 6 bytes";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    switch (byteLength) {
        case OneByte:
            return ReadUInt8(thread, buffer, offset, littleEndian);
        case TwoBytes:
            return ReadUInt16(thread, buffer, offset, littleEndian);
        case FourBytes:
            return ReadUInt32(thread, buffer, offset, littleEndian);
        default:
            break;
    }
    if (byteLength > FourBytes) {
        auto valueLow = ReadBytes(thread, buffer, offset, ThreeBytes, littleEndian);
        ByteLength highBytes = static_cast<ByteLength>(byteLength - ThreeBytes);
        auto valueHigh = ReadBytes(thread, buffer, offset + ThreeBytes, highBytes, littleEndian);
        if (littleEndian) {
            return MergeUInt(valueLow, valueHigh, ThreeBytes);
        }
        return MergeUInt(valueHigh, valueLow, highBytes);
    }
    auto valueLow = ReadBytes(thread, buffer, offset, TwoBytes, littleEndian);
    ByteLength highBytes = static_cast<ByteLength>(byteLength - TwoBytes);
    auto valueHigh = ReadBytes(thread, buffer, offset + TwoBytes, highBytes, littleEndian);
    if (littleEndian) {
        return MergeUInt(valueLow, valueHigh, TwoBytes);
    }
    return MergeUInt(valueHigh, valueLow, highBytes);
}

JSTaggedValue JSAPIFastBuffer::ReadInt(JSThread *thread, JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                       ByteLength byteLength, bool littleEndian)
{
    auto ret = ReadBytes(thread, buffer, offset, byteLength, littleEndian);
    return JSTaggedValue(static_cast<int64_t>(ret.GetNumber()));
}

JSTaggedValue JSAPIFastBuffer::WriteBigUInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                              const JSHandle<JSTaggedValue> &value, uint32_t offset, bool littleEndian)
{
    JSType type = JSType::JS_BIGUINT64_ARRAY;
    auto byteSize = base::TypedArrayHelper::GetElementSize(type);
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>(thread, JSTypedArray::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    uint64_t valueNum;
    bool isLossLess;
    ASSERT(value->IsBigInt() || value->IsNumber() || value->IsBoolean());
    BigInt::BigIntToUint64(thread, value, &valueNum, &isLossLess);
    uint64_t left = 0;
    uint64_t right = UINT64_MAX;
    if (valueNum > right || valueNum < left) {
        std::ostringstream oss;
        oss << std::fixed << "The value of \"value\" is out of range. It must be >= " << left << " and <= " << right
            << ". Received value is: " << valueNum;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (offset < 0 || offset + NumberSize::BIGUINT64 > buffer->GetLength()) {
        std::ostringstream oss;
        oss << std::fixed << "The value of \"offset\" is out of range. It must be >= " << 0
            << " and <= " << buffer->GetLength() - NumberSize::BIGUINT64 << ". Received value is: " << offset;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIFastBuffer::SetValueByIndex(thread, typedArray.GetTaggedValue(), offset, value.GetTaggedValue(), type,
                                     littleEndian);
    return JSTaggedValue(offset + byteSize);
}

JSTaggedValue JSAPIFastBuffer::ReadBigUInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                             bool littleEndian)
{
    JSType type = JSType::JS_BIGUINT64_ARRAY;
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>(thread, JSTypedArray ::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    if (offset < 0 || offset + NumberSize::BIGUINT64 > buffer->GetLength()) {
        std::ostringstream oss;
        oss << std::fixed << "The value of \"offset\" is out of range. It must be >= " << 0
            << " and <= " << buffer->GetLength() - NumberSize::BIGUINT64 << ". Received value is: " << offset;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSAPIFastBuffer::GetValueByIndex(thread, typedArray.GetTaggedValue(), offset, type, littleEndian);
}

JSTaggedValue JSAPIFastBuffer::WriteBigInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer,
                                             const JSHandle<JSTaggedValue> &value, uint32_t offset, bool littleEndian)
{
    JSType type = JSType::JS_BIGINT64_ARRAY;
    auto byteSize = base::TypedArrayHelper::GetElementSize(type);
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>(thread, JSTypedArray ::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    int64_t valueNum;
    bool isLossLess;
    ASSERT(value->IsBigInt() || value->IsNumber() || value->IsBoolean());
    BigInt::BigIntToInt64(thread, value, &valueNum, &isLossLess);
    int64_t left = INT64_MIN;
    int64_t right = INT64_MAX;
    if (valueNum > right || valueNum < left) {
        std::ostringstream oss;
        oss << std::fixed << "The value of \"value\" is out of range. It must be >= " << left << " and <= " << right
            << ". Received value is: " << valueNum;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (offset < 0 || offset + NumberSize::BIGUINT64 > buffer->GetLength()) {
        std::ostringstream oss;
        oss << std::fixed << "The value of \"offset\" is out of range. It must be >= " << 0
            << " and <= " << buffer->GetLength() - NumberSize::BIGUINT64 << ". Received value is: " << offset;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIFastBuffer::SetValueByIndex(thread, typedArray.GetTaggedValue(), offset, value.GetTaggedValue(), type,
                                     littleEndian);
    return JSTaggedValue(offset + byteSize);
}

JSTaggedValue JSAPIFastBuffer::ReadBigInt64(JSThread *thread, const JSHandle<JSAPIFastBuffer> &buffer, uint32_t offset,
                                            bool littleEndian)
{
    JSType type = JSType::JS_BIGINT64_ARRAY;
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>(thread, JSTypedArray ::Cast(buffer->GetFastBufferData().GetTaggedObject()));
    if (offset < 0 || offset + NumberSize::BIGINT64 > buffer->GetLength()) {
        std::ostringstream oss;
        oss << std::fixed << "The value of \"offset\" is out of range. It must be >= " << 0
            << " and <= " << buffer->GetLength() - NumberSize::BIGINT64 << ". Received value is: " << offset;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSAPIFastBuffer ::GetValueByIndex(thread, typedArray.GetTaggedValue(), offset, type, littleEndian);
}
}  // namespace panda::ecmascript
