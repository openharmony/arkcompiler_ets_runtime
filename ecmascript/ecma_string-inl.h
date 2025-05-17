/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_STRING_INL_H
#define ECMASCRIPT_STRING_INL_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/debugger/js_debugger_manager.h"

namespace panda::ecmascript {
/* static */
inline EcmaString *EcmaString::CreateEmptyString(const EcmaVM *vm)
{
    auto string = vm->GetFactory()->AllocNonMovableLineStringObject(EcmaString::SIZE);
    string->InitLengthAndFlags(0, true);
    string->SetRawHashcode(0);
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                              bool canBeCompress, MemSpaceType type)
{
    if (utf8Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    EcmaString *string = nullptr;
    if (canBeCompress) {
        string = CreateLineStringWithSpaceType(vm, utf8Len, true, type);
        ASSERT(string != nullptr);
        std::copy(utf8Data, utf8Data + utf8Len, string->GetDataUtf8Writable());
    } else {
        auto utf16Len = base::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        string = CreateLineStringWithSpaceType(vm, utf16Len, false, type);
        ASSERT(string != nullptr);

        [[maybe_unused]] auto len =
            base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, string->GetDataUtf16Writable(), utf8Len, utf16Len);
        ASSERT(len == utf16Len);
    }

    ASSERT_PRINT(canBeCompress == CanBeCompressed(string), "Bad input canBeCompress!");
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateFromUtf8CompressedSubString(const EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                                 uint32_t offset, uint32_t utf8Len, MemSpaceType type)
{
    if (UNLIKELY(utf8Len == 0)) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    EcmaString *subString = CreateLineStringWithSpaceType(vm, utf8Len, true, type);
    ASSERT(subString != nullptr);

    auto *utf8Data = string->GetDataUtf8() + offset;
    std::copy(utf8Data, utf8Data + utf8Len, subString->GetDataUtf8Writable());
    ASSERT_PRINT(CanBeCompressed(subString), "String cannot be compressed!");
    return subString;
}

inline EcmaString *EcmaString::CreateUtf16StringFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
    MemSpaceType type)
{
    if (utf16Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    auto string = CreateLineStringWithSpaceType(vm, utf16Len, false, type);
    ASSERT(string != nullptr);
    auto len = utf::ConvertRegionMUtf8ToUtf16(
        utf8Data, string->GetDataUtf16Writable(), utf::Mutf8Size(utf8Data), utf16Len, 0);
    if (len < utf16Len) {
        string->TrimLineString(vm->GetJSThread(), len);
    }
    ASSERT_PRINT(false == CanBeCompressed(string), "Bad input canBeCompress!");
    return string;
}

inline void EcmaString::TrimLineString(const JSThread *thread, uint32_t newLength)
{
    ASSERT(IsLineString());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = GetLength();
    ASSERT(oldLength > newLength);
    size_t trimBytes = (oldLength - newLength) * (IsUtf8() ? sizeof(uint8_t) : sizeof(uint16_t));
    size_t size = IsUtf8() ? LineEcmaString::ComputeSizeUtf8(newLength) : LineEcmaString::ComputeSizeUtf16(newLength);
    factory->FillFreeObject(ToUintPtr(this) + size, trimBytes, RemoveSlots::YES, ToUintPtr(this));
    InitLengthAndFlags(newLength, CanBeCompressed(this));
}

inline EcmaString *EcmaString::CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                               bool canBeCompress, MemSpaceType type)
{
    if (utf16Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    auto string = CreateLineStringWithSpaceType(vm, utf16Len, canBeCompress, type);
    ASSERT(string != nullptr);

    if (canBeCompress) {
        CopyChars(string->GetDataUtf8Writable(), utf16Data, utf16Len);
    } else {
        uint32_t len = utf16Len * (sizeof(uint16_t) / sizeof(uint8_t));
        if (memcpy_s(string->GetDataUtf16Writable(), len, utf16Data, len) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }

    ASSERT_PRINT(canBeCompress == CanBeCompressed(string), "Bad input canBeCompress!");
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateLineString(const EcmaVM *vm, size_t length, bool compressed)
{
    size_t size = compressed ? LineEcmaString::ComputeSizeUtf8(length) : LineEcmaString::ComputeSizeUtf16(length);
    auto string = vm->GetFactory()->AllocLineStringObject(size);
    string->InitLengthAndFlags(length, compressed);
    string->SetRawHashcode(0);
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateLineStringNoGC(const EcmaVM *vm, size_t length, bool compressed)
{
    size_t size = compressed ? LineEcmaString::ComputeSizeUtf8(length) : LineEcmaString::ComputeSizeUtf16(length);
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    auto string = vm->GetFactory()->AllocLineStringObjectNoGC(size);
    string->InitLengthAndFlags(length, compressed);
    string->SetRawHashcode(0);
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateLineStringWithSpaceType(const EcmaVM *vm, size_t length, bool compressed,
                                                             MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    size_t size = compressed ? LineEcmaString::ComputeSizeUtf8(length) : LineEcmaString::ComputeSizeUtf16(length);
    EcmaString *string = nullptr;
    switch (type) {
        case MemSpaceType::SHARED_OLD_SPACE:
            string = vm->GetFactory()->AllocOldSpaceLineStringObject(size);
            break;
        case MemSpaceType::SHARED_NON_MOVABLE:
            string = vm->GetFactory()->AllocNonMovableLineStringObject(size);
            break;
        case MemSpaceType::SHARED_READ_ONLY_SPACE:
            string = vm->GetFactory()->AllocReadOnlyLineStringObject(size);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    string->InitLengthAndFlags(length, compressed);
    string->SetRawHashcode(0);
    return string;
}

inline SlicedString *EcmaString::CreateSlicedString(const EcmaVM *vm, MemSpaceType type)
{
    auto slicedString = SlicedString::Cast(vm->GetFactory()->AllocSlicedStringObject(type));
    slicedString->SetRawHashcode(0);
    slicedString->SetParent(vm->GetJSThread(), JSTaggedValue::Undefined(), BarrierMode::SKIP_BARRIER);
    return slicedString;
}

/*
 * In the multi-thread optimization scenario, start the application.
 * 1.The thread executes until CheckThread () acquires the lock.
 * 2.At this time, the thread receives the SIGPROF signal, interrupts the current program execution,
 * and enters the signal processing function.
 * 3.When CreateTreeString()->GetJSThread()->CheckThread() is executed, the lock cannot be obtained
 * and the system has to wait, causing a deadlock.
 * Therefore, if the function is executed during signal processing, the thread ID is directly obtained and
 * the thread detection is not performed, thereby avoiding deadlock.
 */

inline void GetDebuggerThread(const EcmaVM *vm, JSThread **thread)
{
    if (vm->GetJsDebuggerManager()->GetSignalState()) {
        *thread = vm->GetJSThreadNoCheck();
    } else {
        *thread = vm->GetJSThread();
    }
}

inline EcmaString *EcmaString::CreateTreeString(const EcmaVM *vm,
    const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right, uint32_t length, bool compressed)
{
    ECMA_STRING_CHECK_LENGTH_AND_TRHOW(vm, length);
    JSThread *thread = nullptr;
    GetDebuggerThread(vm, &thread);
    auto string = TreeEcmaString::Cast(vm->GetFactory()->AllocTreeStringObject());
    string->InitLengthAndFlags(length, compressed);
    string->SetRawHashcode(0);
    string->SetFirst(thread, left.GetTaggedValue());
    string->SetSecond(thread, right.GetTaggedValue());
    return string;
}

/* static */
EcmaString *EcmaString::FastSubUtf8String(const EcmaVM *vm, const JSHandle<EcmaString> &src, uint32_t start,
                                          uint32_t length)
{
    JSHandle<EcmaString> string(vm->GetJSThread(), CreateLineString(vm, length, true));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    Span<uint8_t> dst(string->GetDataUtf8Writable(), length);
    Span<const uint8_t> source(srcFlat.GetDataUtf8() + start, length);
    EcmaString::MemCopyChars(dst, length, source, length);

    ASSERT_PRINT(CanBeCompressed(*string), "canBeCompresse does not match the real value!");
    return *string;
}

/* static */
EcmaString *EcmaString::FastSubUtf16String(const EcmaVM *vm, const JSHandle<EcmaString> &src, uint32_t start,
                                           uint32_t length)
{
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    bool canBeCompressed = CanBeCompressed(srcFlat.GetDataUtf16() + start, length);
    JSHandle<EcmaString> string(vm->GetJSThread(), CreateLineString(vm, length, canBeCompressed));
    // maybe happen GC,so get srcFlat again
    srcFlat = FlattenAllString(vm, src);
    if (canBeCompressed) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CopyChars(string->GetDataUtf8Writable(), srcFlat.GetDataUtf16() + start, length);
    } else {
        uint32_t len = length * (sizeof(uint16_t) / sizeof(uint8_t));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        Span<uint16_t> dst(string->GetDataUtf16Writable(), length);
        Span<const uint16_t> source(srcFlat.GetDataUtf16() + start, length);
        EcmaString::MemCopyChars(dst, len, source, len);
    }
    ASSERT_PRINT(canBeCompressed == CanBeCompressed(*string), "canBeCompresse does not match the real value!");
    return *string;
}

inline uint16_t *EcmaString::GetData() const
{
    ASSERT_PRINT(IsLineString(), "EcmaString: Read data from not LineString");
    return LineEcmaString::Cast(this)->GetData();
}

inline const uint8_t *EcmaString::GetDataUtf8() const
{
    ASSERT_PRINT(IsUtf8(), "EcmaString: Read data as utf8 for utf16 string");
    return reinterpret_cast<uint8_t *>(GetData());
}

inline const uint16_t *EcmaString::GetDataUtf16() const
{
    LOG_ECMA_IF(!IsUtf16(), FATAL) << "EcmaString: Read data as utf16 for utf8 string";
    return GetData();
}

inline uint8_t *EcmaString::GetDataUtf8Writable()
{
    ASSERT_PRINT(IsUtf8(), "EcmaString: Read data as utf8 for utf16 string");
    return reinterpret_cast<uint8_t *>(GetData());
}

inline uint16_t *EcmaString::GetDataUtf16Writable()
{
    LOG_ECMA_IF(!IsUtf16(), FATAL) << "EcmaString: Read data as utf16 for utf8 string";
    return GetData();
}

inline size_t EcmaString::GetUtf8Length(bool modify, bool isGetBufferSize) const
{
    if (!IsUtf16()) {
        return GetLength() + 1;  // add place for zero in the end
    }
    CVector<uint16_t> tmpBuf;
    const uint16_t *data = GetUtf16DataFlat(this, tmpBuf);
    return base::utf_helper::Utf16ToUtf8Size(data, GetLength(), modify, isGetBufferSize);
}

template<bool verify>
inline uint16_t EcmaString::At(int32_t index) const
{
    int32_t length = static_cast<int32_t>(GetLength());
    if constexpr (verify) {
        if ((index < 0) || (index >= length)) {
            return 0;
        }
    }
    switch (GetStringType()) {
        case JSType::LINE_STRING:
            return LineEcmaString::Cast(this)->Get<verify>(index);
        case JSType::SLICED_STRING:
            return SlicedString::Cast(this)->Get<verify>(index);
        case JSType::TREE_STRING:
            return TreeEcmaString::Cast(this)->Get<verify>(index);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

inline Span<const uint8_t> EcmaString::FastToUtf8Span() const
{
    uint32_t strLen = GetLength();
    ASSERT(IsUtf8());
    const uint8_t *data = GetDataUtf8();
    return Span<const uint8_t>(data, strLen);
}

inline void EcmaString::WriteData(uint32_t index, uint16_t src)
{
    ASSERT(index < GetLength());
    ASSERT(IsLineString());
    LineEcmaString::Cast(this)->Set(index, src);
}

inline bool EcmaString::IsFlat() const
{
    if (!JSTaggedValue(this).IsTreeString()) {
        return true;
    }
    return TreeEcmaString::Cast(this)->IsFlat();
}

template <typename Char>
void EcmaString::WriteToFlat(EcmaString *src, Char *buf, uint32_t maxLength)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t length = src->GetLength();
    if (length == 0) {
        return;
    }
    while (true) {
        ASSERT(length <= maxLength && length > 0);
        ASSERT(length <= src->GetLength());
        switch (src->GetStringType()) {
            case JSType::LINE_STRING: {
                if (src->IsUtf8()) {
                    CopyChars(buf, src->GetDataUtf8(), length);
                } else {
                    CopyChars(buf, src->GetDataUtf16(), length);
                }
                return;
            }
            case JSType::TREE_STRING: {
                TreeEcmaString *treeSrc = TreeEcmaString::Cast(src);
                EcmaString *first = EcmaString::Cast(treeSrc->GetFirst());
                EcmaString *second = EcmaString::Cast(treeSrc->GetSecond());
                uint32_t firstLength = first->GetLength();
                uint32_t secondLength = second->GetLength();
                if (secondLength >= firstLength) {
                    // second string is longer. So recurse over first.
                    WriteToFlat(first, buf, maxLength);
                    if (first == second) {
                        CopyChars(buf + firstLength, buf, firstLength);
                        return;
                    }
                    buf += firstLength;
                    maxLength -= firstLength;
                    src = second;
                    length -= firstLength;
                } else {
                    // first string is longer.  So recurse over second.
                    // if src{first:A,second:B} is half flat to {first:A+B,second:empty} by another thread
                    // but the other thread didn't end, and this thread get  {first:A+B,second:B}
                    // it may cause write buffer overflower in line 424, buf + firstLength is overflower.
                    // so use 'length > firstLength' instead of 'secondLength > 0'
                    if (length > firstLength) {
                        if (secondLength == 1) {
                            buf[firstLength] = static_cast<Char>(second->At<false>(0));
                        } else if ((second->IsLineString()) && second->IsUtf8()) {
                            CopyChars(buf + firstLength, second->GetDataUtf8(), secondLength);
                        } else {
                            WriteToFlat(second, buf + firstLength, maxLength - firstLength);
                        }
                        length -= secondLength;
                    }
                    maxLength = firstLength;
                    src = first;
                }
                continue;
            }
            case JSType::SLICED_STRING: {
                EcmaString *parent = EcmaString::Cast(SlicedString::Cast(src)->GetParent());
                if (src->IsUtf8()) {
                    CopyChars(buf, parent->GetDataUtf8() + SlicedString::Cast(src)->GetStartIndex(), length);
                } else {
                    CopyChars(buf, parent->GetDataUtf16() + SlicedString::Cast(src)->GetStartIndex(), length);
                }
                return;
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }
}

template <typename Char>
void EcmaString::WriteToFlatWithPos(EcmaString *src, Char *buf, uint32_t length, uint32_t pos)
{
    DISALLOW_GARBAGE_COLLECTION;
    [[ maybe_unused ]] uint32_t maxLength = src->GetLength();
    if (length == 0) {
        return;
    }
    while (true) {
        ASSERT(length + pos <= maxLength && length > 0);
        ASSERT(length <= src->GetLength());
        ASSERT(pos >= 0);
        switch (src->GetStringType()) {
            case JSType::LINE_STRING: {
                if (src->IsUtf8()) {
                    CopyChars(buf, src->GetDataUtf8() + pos, length);
                } else {
                    CopyChars(buf, src->GetDataUtf16() + pos, length);
                }
                return;
            }
            case JSType::TREE_STRING: {
                TreeEcmaString *treeSrc = TreeEcmaString::Cast(src);
                EcmaString *first = EcmaString::Cast(treeSrc->GetFirst());
                ASSERT(first->IsLineString());
                src = first;
                continue;
            }
            case JSType::SLICED_STRING: {
                EcmaString *parent = EcmaString::Cast(SlicedString::Cast(src)->GetParent());
                if (src->IsUtf8()) {
                    CopyChars(buf, parent->GetDataUtf8() + SlicedString::Cast(src)->GetStartIndex() + pos, length);
                } else {
                    CopyChars(buf, parent->GetDataUtf16() + SlicedString::Cast(src)->GetStartIndex() + pos, length);
                }
                return;
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }
}

inline const uint8_t *FlatStringInfo::GetDataUtf8() const
{
    return string_->GetDataUtf8() + startIndex_;
}

inline const uint16_t *FlatStringInfo::GetDataUtf16() const
{
    return string_->GetDataUtf16() + startIndex_;
}

inline uint8_t *FlatStringInfo::GetDataUtf8Writable() const
{
    return string_->GetDataUtf8Writable() + startIndex_;
}

inline uint16_t *FlatStringInfo::GetDataUtf16Writable() const
{
    return string_->GetDataUtf16Writable() + startIndex_;
}

inline const uint8_t *EcmaStringAccessor::GetDataUtf8()
{
    return string_->GetDataUtf8();
}

inline const uint16_t *EcmaStringAccessor::GetDataUtf16()
{
    return string_->GetDataUtf16();
}

inline size_t EcmaStringAccessor::GetUtf8Length(bool isGetBufferSize) const
{
    return string_->GetUtf8Length(true, isGetBufferSize);
}

inline void EcmaStringAccessor::ReadData(EcmaString *dst, EcmaString *src,
    uint32_t start, uint32_t destSize, uint32_t length)
{
    dst->WriteData(src, start, destSize, length);
}

inline Span<const uint8_t> EcmaStringAccessor::FastToUtf8Span()
{
    return string_->FastToUtf8Span();
}
}  // namespace panda::ecmascript
#endif
