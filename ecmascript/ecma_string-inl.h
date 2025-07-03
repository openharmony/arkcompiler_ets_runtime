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
#include "common_interfaces/objects/string/base_string-inl2.h"

namespace panda::ecmascript {
/* static */
inline EcmaString *EcmaString::CreateEmptyString(const EcmaVM *vm)
{
    auto string = vm->GetFactory()->AllocNonMovableLineStringObject(BaseString::SIZE);
    string->InitLengthAndFlags(0, true);
    string->SetRawHashcode(0);
    return string;
}

#if ENABLE_NEXT_OPTIMIZATION && defined(USE_CMC_GC)
/* static */
inline EcmaString *EcmaString::CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                              bool canBeCompress, MemSpaceType type)
{
    if (utf8Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    auto allocator = [vm, type](size_t size, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::LINE_STRING && "Can only allocate line string");
        return EcmaString::AllocLineString(vm, size, type)->ToBaseString();
    };
    BaseString *str = BaseString::CreateFromUtf8(std::move(allocator), utf8Data, utf8Len, canBeCompress);
    return EcmaString::FromBaseString(str);
}
#else
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
        auto utf16Len = common::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        string = CreateLineStringWithSpaceType(vm, utf16Len, false, type);
        ASSERT(string != nullptr);

        [[maybe_unused]] auto len =
            common::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, string->GetDataUtf16Writable(), utf8Len, utf16Len);
        ASSERT(len == utf16Len);
    }

    ASSERT_PRINT(canBeCompress == CanBeCompressed(string), "Bad input canBeCompress!");
    return string;
}
#endif

/* static */
inline EcmaString *EcmaString::CreateFromUtf8CompressedSubString(const EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                                 uint32_t offset, uint32_t utf8Len, MemSpaceType type)
{
    if (UNLIKELY(utf8Len == 0)) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }

    auto allocator = [vm, type](size_t size, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::LINE_STRING && "Can only allocate line string");
        return EcmaString::AllocLineString(vm, size, type)->ToBaseString();
    };
    BaseString *str = BaseString::CreateFromUtf8CompressedSubString(std::move(allocator), string, offset, utf8Len);
    return EcmaString::FromBaseString(str);
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
        string->TrimLineString(len);
    }
    ASSERT_PRINT(false == CanBeCompressed(string), "Bad input canBeCompress!");
    return string;
}

inline void EcmaString::TrimLineString(uint32_t newLength)
{
    BaseString* baseThis = reinterpret_cast<BaseString*>(this);
    reinterpret_cast<LineString *>(baseThis)->LineString::Trim(newLength);
}

inline EcmaString *EcmaString::CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                               bool canBeCompress, MemSpaceType type)
{
    if (utf16Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }

    auto allocator = [vm, type](size_t size, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::LINE_STRING && "Can only allocate line string");
        return EcmaString::AllocLineString(vm, size, type)->ToBaseString();
    };
    BaseString *str = BaseString::CreateFromUtf16(std::move(allocator), utf16Data, utf16Len, canBeCompress);
    return EcmaString::FromBaseString(str);
}

/* static */
inline EcmaString *EcmaString::CreateLineString(const EcmaVM *vm, size_t length, bool compressed)
{
    auto allocator = [vm](size_t size, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::LINE_STRING && "Can only allocate line string");
        EcmaString* string = vm->GetFactory()->AllocLineStringObject(size);
        return string;
    };
    BaseString *str = BaseString::CreateLineString(std::move(allocator), length, compressed);
    return EcmaString::FromBaseString(str);
}

/* static */
inline EcmaString *EcmaString::CreateLineStringNoGC(const EcmaVM *vm, size_t length, bool compressed)
{
    auto allocator = [vm](size_t size, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::LINE_STRING && "Can only allocate line string");
        size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        EcmaString* string = vm->GetFactory()->AllocLineStringObjectNoGC(size);
        return string;
    };
    BaseString *str = BaseString::CreateLineString(std::move(allocator), length, compressed);
    return EcmaString::FromBaseString(str);
}

inline EcmaString* EcmaString::AllocLineString(const EcmaVM* vm, size_t size, MemSpaceType type)
{
    EcmaString* string = nullptr;
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
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateLineStringWithSpaceType(const EcmaVM *vm, size_t length, bool compressed,
                                                             MemSpaceType type)
{
    auto allocator = [vm, type](size_t size, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::LINE_STRING && "Can only allocate line string");
        ASSERT(IsSMemSpace(type));
        return AllocLineString(vm, size, type);
    };
    BaseString *str = BaseString::CreateLineString(std::move(allocator), length, compressed);
    return EcmaString::FromBaseString(str);
}

inline SlicedEcmaString* EcmaString::CreateSlicedString(const EcmaVM* vm, JSHandle<EcmaString> parent,
                                                        MemSpaceType type)
{
    auto allocator = [vm, type](size_t, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::SLICED_STRING && "Can only allocate sliced string");
        EcmaString* string = vm->GetFactory()->AllocSlicedStringObject(type);
        return string;
    };
    auto writeBarrier = [vm](void* obj, size_t offset, BaseObject* str) {
        Barriers::SetObject<true>(vm->GetJSThread(), obj, offset, reinterpret_cast<JSTaggedType>(str));
    };
    SlicedString* slicedString = BaseString::CreateSlicedString(std::move(allocator), std::move(writeBarrier), parent);
    return SlicedEcmaString::FromBaseString(slicedString);
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

    auto allocator = [vm](size_t, common::CommonType stringType) -> BaseObject* {
        ASSERT(stringType == common::CommonType::TREE_STRING && "Can only allocate tree string");
        EcmaString* string = vm->GetFactory()->AllocTreeStringObject();
        return string;
    };
    auto writeBarrier = [thread](void* obj, size_t offset, BaseObject* str) {
        Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
    };
    TreeString* treeString = BaseString::CreateTreeString(std::move(allocator), std::move(writeBarrier), left, right,
                                                          length, compressed);
    return TreeEcmaString::FromBaseString(treeString);
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
        BaseString::CopyChars(string->GetDataUtf8Writable(), srcFlat.GetDataUtf16() + start, length);
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

inline const uint8_t* EcmaString::GetDataUtf8() const
{
    return ToBaseString()->GetDataUtf8();
}

inline const uint16_t* EcmaString::GetDataUtf16() const
{
    return ToBaseString()->GetDataUtf16();
}

// require is LineString
inline uint8_t* EcmaString::GetDataUtf8Writable()
{
    return ToBaseString()->GetDataUtf8Writable();
}

inline uint16_t* EcmaString::GetDataUtf16Writable()
{
    return ToBaseString()->GetDataUtf16Writable();
}

inline size_t EcmaString::GetUtf8Length(const JSThread *thread, bool modify, bool isGetBufferSize) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->GetUtf8Length(std::move(readBarrier), modify, isGetBufferSize);
}

template <bool verify>
uint16_t EcmaString::At(const JSThread *thread, int32_t index) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->At<verify>(std::move(readBarrier), index);
}

inline void EcmaString::WriteData(uint32_t index, uint16_t src)
{
    return ToBaseString()->WriteData(index, src);
}

inline Span<const uint8_t> EcmaString::FastToUtf8Span() const
{
    uint32_t len = GetLength();
    ASSERT(IsUtf8());
    const uint8_t *data = GetDataUtf8();
    return Span<const uint8_t>(data, len);
}

inline bool EcmaString::IsFlat(const JSThread *thread) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->IsFlat(std::move(readBarrier));
}

template <typename Char>
void EcmaString::WriteToFlat(const JSThread *thread, EcmaString *src, Char *buf, uint32_t maxLength)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::WriteToFlat(std::move(readBarrier), src->ToBaseString(), buf, maxLength);
}

template <typename Char>
void EcmaString::WriteToFlatWithPos(const JSThread *thread, EcmaString *src, Char *buf, uint32_t length, uint32_t pos)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::WriteToFlatWithPos(std::move(readBarrier), src->ToBaseString(), buf, length, pos);
}

// It allows user to copy into buffer even if maxLength < length
inline size_t EcmaString::WriteUtf8(const JSThread *thread, uint8_t *buf, size_t maxLength, bool isWriteBuffer) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->WriteUtf8(std::move(readBarrier), buf, maxLength, isWriteBuffer);
}

// It allows user to copy into buffer even if maxLength < length
inline size_t EcmaString::WriteUtf16(const JSThread *thread, uint16_t *buf, uint32_t targetLength,
                                     uint32_t bufLength) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->WriteUtf16(std::move(readBarrier), buf, targetLength, bufLength);
}

inline size_t EcmaString::WriteOneByte(const JSThread *thread, uint8_t *buf, size_t maxLength) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->WriteOneByte(std::move(readBarrier), buf, maxLength);
}

inline uint32_t EcmaString::CopyDataUtf16(const JSThread *thread, uint16_t *buf, uint32_t maxLength) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->CopyDataUtf16(std::move(readBarrier), buf, maxLength);
}

inline Span<const uint8_t> EcmaString::ToUtf8Span(const JSThread *thread, CVector<uint8_t> &buf, bool modify,
                                                  bool cesu8)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->ToUtf8Span(std::move(readBarrier), buf, modify, cesu8);
}

inline Span<const uint8_t> EcmaString::DebuggerToUtf8Span(const JSThread *thread, CVector<uint8_t> &buf, bool modify)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->DebuggerToUtf8Span(std::move(readBarrier), buf, modify);
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

inline size_t EcmaStringAccessor::GetUtf8Length(const JSThread *thread, bool isGetBufferSize) const
{
    return string_->GetUtf8Length(thread, true, isGetBufferSize);
}

template <RBMode mode>
inline void EcmaStringAccessor::ReadData(const JSThread *thread, EcmaString *dst, EcmaString *src, uint32_t start,
                                         uint32_t destSize, uint32_t length)
{
    dst->WriteData<mode>(thread, src, start, destSize, length);
}

inline Span<const uint8_t> EcmaStringAccessor::FastToUtf8Span() const
{
    return string_->FastToUtf8Span();
}
}  // namespace panda::ecmascript
#endif
