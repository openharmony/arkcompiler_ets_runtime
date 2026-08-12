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

#ifndef ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
#define ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H

#include <atomic>
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value_wrapper-inl.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/mem/hole_memory.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/tagged_array-inl.h"

#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "class_data_accessor-inl.h"
#include "index_accessor.h"

namespace panda {
namespace ecmascript {
class BaseSerializer;
class JSThread;

class Program : public ECMAObject {
public:
    DECL_CAST(Program)

    static constexpr size_t MAIN_FUNCTION_OFFSET = ECMAObject::SIZE;
    ACCESSORS(MainFunction, MAIN_FUNCTION_OFFSET, SIZE)

    DECL_VISIT_OBJECT(MAIN_FUNCTION_OFFSET, SIZE)
    DECL_DUMP()
};

/*                         ConstantPool(TaggedArray)
 *      +--------------------------------+----------------------------------
 *      |               ...              |       ^           ^            ^   index 0
 *      |  Method / AOTLiteralInfo / Int |       |           |            |
 *      |              String            |    CacheLength    |            |
 *      |           Array Literal        | (with special     |            |
 *      |           Class Literal        |compressed pointer)|            |
 *      |           Object Literal       |       |           |            |
 *      |               ...              |       v           |            |
 *      +--------------------------------+---------------    |            |
 *      |          ProtoTransTableInfo   |PointerToIndexDic  |            |
 *      +--------------------------------+---------------    |            |
 *      |          AOTSymbolInfo         |TaggedArray        |            |
 *      +--------------------------------+---------------    |            |
 *      |      shared constpool id       |Tagged int32_t ConstpoolLength  |
 *      +--------------------------------+---------------    |          Length
 *      |      unshared constpool id     |Tagged int32_t     |            |
 *      +--------------------------------+---------------    |            |
 *      |          AOTHClassInfo         |TaggedArray        |            |
 *      +--------------------------------+---------------    |            |
 *      |          AOTArrayInfo          |TaggedArray        |            |
 *      +--------------------------------+---------------    |            |
 *      |         constIndexInfo         |TaggedArray        v            |
 *      +--------------------------------+--------------------------      |
 *      |           IndexHeader          |                                |
 *      +--------------------------------+                                |
 *      |           JSPandaFile          |                                v    index: Length-1
 *      +--------------------------------+-----------------------------------
 */
class ConstantPool : public TaggedArray {
public:
    static constexpr size_t JS_PANDA_FILE_INDEX = 1; // not need gc
    static constexpr size_t INDEX_HEADER_INDEX = 2; // not need gc
    static constexpr size_t CONSTANT_INDEX_INFO_INDEX = 3;
    static constexpr size_t AOT_ARRAY_INFO_INDEX = 4;
    static constexpr size_t AOT_HCLASS_INFO_INDEX = 5;
    static constexpr size_t UNSHARED_CONSTPOOL_INDEX = 6;
    static constexpr size_t SHARED_CONSTPOOL_ID = 7;
    static constexpr size_t AOT_SYMBOL_INFO_INDEX = 8;
    static constexpr size_t PROTO_TRANS_TABLE_INFO_INDEX = 9;
    static constexpr size_t RESERVED_POOL_LENGTH = INDEX_HEADER_INDEX; // divide the gc area

    // AOTHClassInfo, AOTArrayInfo, ConstIndexInfo, unsharedConstpoolIndex, constpoolId, AOTSymbolInfo,
    // ProtoTransTableInfo
    static constexpr size_t EXTEND_DATA_NUM = 7;

    static constexpr int32_t CONSTPOOL_TYPE_FLAG = INT32_MAX; // INT32_MAX : unshared constpool.

    static ConstantPool *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsConstantPool());
        return static_cast<ConstantPool *>(object);
    }

    static JSHandle<ConstantPool> CreateUnSharedConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                          panda_file::File::EntityId id)
    {
        const panda_file::File::IndexHeader *mainIndex = jsPandaFile->GetPandaFile()->GetIndexHeader(id);
        LOG_ECMA_IF(mainIndex == nullptr, FATAL) << "Unknown methodId: " << id.GetOffset();
        auto constpoolSize = mainIndex->method_idx_size;

        JSHandle<JSTaggedValue> constpool(vm->GetJSThread(), JSTaggedValue::Hole());
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        if (isLoadedAOT) {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
            int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
            constpool = GetDeserializedConstantPool(vm, jsPandaFile, index);
#else
            LOG_FULL(FATAL) << "Aot don't support Windows and MacOS platform";
            UNREACHABLE();
#endif
        }
        JSHandle<ConstantPool> constpoolObj;
        if (constpool.GetTaggedValue().IsHole()) {
            ObjectFactory *factory = vm->GetFactory();
            constpoolObj = factory->NewConstantPool(constpoolSize);
        } else {
            constpoolObj = JSHandle<ConstantPool>(constpool);
        }

        constpoolObj->SetJSPandaFile(jsPandaFile);
        constpoolObj->SetIndexHeader(mainIndex);

        return constpoolObj;
    }

    static JSHandle<ConstantPool> CreateUnSharedConstPoolBySharedConstpool(
        EcmaVM *vm, const JSPandaFile *jsPandaFile, ConstantPool *shareCp)
    {
        const panda_file::File::IndexHeader *mainIndex = shareCp->GetIndexHeader();
        auto constpoolSize = mainIndex->method_idx_size;

        JSHandle<JSTaggedValue> constpool(vm->GetJSThread(), JSTaggedValue::Hole());
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        if (isLoadedAOT) {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            int32_t cpId = shareCp->GetSharedConstpoolId().GetInt();
            constpool = GetDeserializedConstantPool(vm, jsPandaFile, cpId);
#else
            LOG_FULL(FATAL) << "Aot don't support Windows and MacOS platform";
            UNREACHABLE();
#endif
        }
        JSHandle<ConstantPool> constpoolObj;
        if (constpool.GetTaggedValue().IsHole()) {
            ObjectFactory *factory = vm->GetFactory();
            constpoolObj = factory->NewConstantPool(constpoolSize);
        } else {
            constpoolObj = JSHandle<ConstantPool>(constpool);
        }

        constpoolObj->SetJSPandaFile(jsPandaFile);
        constpoolObj->SetIndexHeader(mainIndex);

        return constpoolObj;
    }

    static JSHandle<ConstantPool> CreateSharedConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                       panda_file::File::EntityId id,
                                                       int32_t cpId = 0)
    {
        const panda_file::File::IndexHeader *mainIndex = jsPandaFile->GetPandaFile()->GetIndexHeader(id);
        LOG_ECMA_IF(mainIndex == nullptr, FATAL) << "Unknown methodId: " << id.GetOffset();
        auto constpoolSize = mainIndex->method_idx_size;

        JSHandle<ConstantPool> constpool(vm->GetFactory()->NewSConstantPool(constpoolSize));

        constpool->SetJSPandaFile(jsPandaFile);
        constpool->SetIndexHeader(mainIndex);
        constpool->SetUnsharedConstpoolIndex(JSTaggedValue(0));
        constpool->SetSharedConstpoolId(JSTaggedValue(cpId));

        return constpool;
    }

    static bool IsAotSymbolInfoExist(JSHandle<TaggedArray> symbolInfo, JSTaggedValue symbol)
    {
        return symbolInfo->GetLength() > 0 && !symbol.IsHole();
    }

    static JSHandle<ConstantPool> CreateSharedConstPoolForAOT(
        EcmaVM *vm, JSHandle<ConstantPool> constpool, int32_t cpId = 0)
    {
        uint32_t numOfCache = constpool->GetNumOfCacheElement();
        JSHandle<ConstantPool> sconstpool(vm->GetFactory()->NewSConstantPool(numOfCache));
        JSThread *thread = vm->GetJSThread();
        for (uint32_t i = 0; i < numOfCache; i++) {
            JSTaggedValue val = constpool->GetObjectFromCache(thread, i);
            if (val.IsString()) {
                sconstpool->SetObjectToCache(thread, i, val);
            } else if (IsAotMethodLiteralInfo(val)) {
                JSHandle<AOTLiteralInfo> valHandle(thread, val);
                JSHandle<AOTLiteralInfo> methodLiteral = CopySharedMethodAOTLiteralInfo(vm, valHandle);
                sconstpool->SetObjectToCache(thread, i, methodLiteral.GetTaggedValue());
            } else if (val.IsInt()) {
                // Here is to copy methodCodeEntry which does not have ihc infos from aot.
                sconstpool->SetObjectToCache(thread, i, val);
            }
        }

        JSHandle<TaggedArray> array(thread->GlobalConstants()->GetHandledEmptyArray());
        sconstpool->SetAotSymbolInfo(thread, array.GetTaggedValue());
        sconstpool->SetProtoTransTableInfo(thread, JSTaggedValue::Undefined());
        sconstpool->SetAotHClassInfo(thread, array.GetTaggedValue());
        sconstpool->SetAotArrayInfo(thread, array.GetTaggedValue());
        sconstpool->SetConstantIndexInfo(thread, array.GetTaggedValue());
        sconstpool->SetJSPandaFile(constpool->GetJSPandaFile());
        sconstpool->SetIndexHeader(constpool->GetIndexHeader());
        sconstpool->SetUnsharedConstpoolIndex(JSTaggedValue(0));
        sconstpool->SetSharedConstpoolId(JSTaggedValue(cpId));
        return sconstpool;
    }

    static JSHandle<AOTLiteralInfo> CopySharedMethodAOTLiteralInfo(EcmaVM *vm,
                                                                   JSHandle<AOTLiteralInfo> src)
    {
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<AOTLiteralInfo> dst = factory->NewSAOTLiteralInfo(1);
        for (uint32_t i = 0; i < src->GetCacheLength(); i++) {
            JSTaggedValue val = src->GetObjectFromCache(thread, i);
            ASSERT(!val.IsHeapObject() || val.IsJSShared());
            dst->SetObjectToCache(thread, i, val);
        }
        dst->SetLiteralType(JSTaggedValue(src->GetLiteralType()));
        return dst;
    }

    static bool CheckUnsharedConstpool(JSTaggedValue constpool)
    {
        int32_t index = static_cast<int32_t>(
           ConstantPool::Cast(constpool.GetTaggedObject())->GetSharedConstpoolId().GetInt());
        if (index == CONSTPOOL_TYPE_FLAG) {
            return true;
        }
        return false;
    }

    inline void SetUnsharedConstpoolIndex(const JSTaggedValue index)
    {
        Barriers::SetPrimitive(GetData(), GetUnsharedConstpoolIndexOffset(), index);
    }

    inline int32_t GetUnsharedConstpoolIndex() const
    {
        return Barriers::GetPrimitive<JSTaggedValue>(GetData(), GetUnsharedConstpoolIndexOffset()).GetInt();
    }

    inline void SetSharedConstpoolId(const JSTaggedValue index)
    {
        Barriers::SetPrimitive(GetData(), GetSharedConstpoolIdOffset(), index);
    }

    inline JSTaggedValue GetSharedConstpoolId() const
    {
        return JSTaggedValue(Barriers::GetPrimitive<JSTaggedValue>(GetData(), GetSharedConstpoolIdOffset()));
    }

    panda_file::File::EntityId GetEntityId(uint32_t index) const
    {
        JSPandaFile *jsPandaFile = GetJSPandaFile();
        panda_file::File::IndexHeader *indexHeader = GetIndexHeader();
        Span<const panda_file::File::EntityId> indexs = jsPandaFile->GetMethodIndex(indexHeader);
        return indexs[index];
    }

    int GetMethodIndexByEntityId(panda_file::File::EntityId entityId) const
    {
        JSPandaFile *jsPandaFile = GetJSPandaFile();
        panda_file::File::IndexHeader *indexHeader = GetIndexHeader();
        Span<const panda_file::File::EntityId> indexs = jsPandaFile->GetMethodIndex(indexHeader);
        int size = static_cast<int>(indexs.size());
        for (int i = 0; i < size; i++) {
            if (indexs[i] == entityId) {
                return i;
            }
        }
        return -1;
    }

    inline void SetIndexHeader(const panda_file::File::IndexHeader *indexHeader)
    {
        Barriers::SetPrimitive(GetData(), GetIndexHeaderOffset(), indexHeader);
    }

    inline panda_file::File::IndexHeader *GetIndexHeader() const
    {
        return Barriers::GetPrimitive<panda_file::File::IndexHeader *>(GetData(), GetIndexHeaderOffset());
    }

    static constexpr size_t CACHE_NUM_TO_SIZE_FACTOR = CompressedJSTaggedValue::CompressFactorToJSTaggedValue();

    static size_t AlignUpNumOfCacheForCompressedPointer(uint32_t numOfCache)
    {
        return AlignUp(numOfCache, CACHE_NUM_TO_SIZE_FACTOR);
    }

    static size_t ComputeSize(uint32_t numOfCache)
    {
        ASSERT(IsAligned(numOfCache, CACHE_NUM_TO_SIZE_FACTOR));
        size_t cacheLength = numOfCache / CACHE_NUM_TO_SIZE_FACTOR;
        return TaggedArray::ComputeSize(
            JSTaggedValue::TaggedTypeSize(), cacheLength + EXTEND_DATA_NUM + RESERVED_POOL_LENGTH);
    }

    void InitializeWithSpecialValue(JSThread *thread, CompressedJSTaggedValue initValue,
        uint32_t numOfCache, uint32_t extraLength = 0)
    {
        ASSERT(initValue.GetCompressedRawData() == CompressedJSTaggedValue::Hole().GetCompressedRawData());
        ASSERT(IsAligned(numOfCache, CACHE_NUM_TO_SIZE_FACTOR));
        size_t cacheLength = numOfCache / CACHE_NUM_TO_SIZE_FACTOR;
        SetLength(cacheLength + EXTEND_DATA_NUM + RESERVED_POOL_LENGTH);
        SetExtraLength(extraLength);
        // A large pool is allocated onto the shared hole template, so most of it
        // already reads as Hole. Writing those elements would copy-on-write every
        // page; only the partial pages at either end still need to be written.
        // Cache slots are compressed: CompressedTaggedTypeSize() wide, written
        // through SetPrimitive<CompressedJSTaggedType>. Using JSTaggedType here
        // would stride and store 8 bytes over 4-byte slots.
        constexpr size_t ELEM = CompressedJSTaggedValue::CompressedTaggedTypeSize();
        uintptr_t data = reinterpret_cast<uintptr_t>(GetData());
        uint32_t skipFrom = numOfCache;
        uint32_t skipTo = numOfCache;
        uintptr_t skipBegin = 0;
        uintptr_t skipEnd = 0;
        if (HoleMemory::SkipRange(data, static_cast<size_t>(numOfCache) * ELEM, ELEM, skipBegin,
                                  skipEnd)) {
            skipFrom = static_cast<uint32_t>((skipBegin - data) / ELEM);
            skipTo = static_cast<uint32_t>((skipEnd - data) / ELEM);
        }
        for (uint32_t i = 0; i < skipFrom; i++) {
            Barriers::SetPrimitive<CompressedJSTaggedType>(GetData(), ELEM * i,
                                                           initValue.GetCompressedRawData());
        }
        for (uint32_t i = skipTo; i < numOfCache; i++) {
            Barriers::SetPrimitive<CompressedJSTaggedType>(GetData(), ELEM * i,
                                                           initValue.GetCompressedRawData());
        }
        InitializeWithSpecialValue(thread);
    }

    inline void InitializeWithSpecialValue(JSThread *thread)
    {
        JSHandle<TaggedArray> array(thread->GlobalConstants()->GetHandledEmptyArray());
        SetAotSymbolInfo(thread, array.GetTaggedValue());
        SetProtoTransTableInfo(thread, JSTaggedValue::Undefined());
        SetAotHClassInfo(thread, array.GetTaggedValue());
        SetAotArrayInfo(thread, array.GetTaggedValue());
        SetConstantIndexInfo(thread, array.GetTaggedValue());
        SetJSPandaFile(nullptr);
        SetIndexHeader(nullptr);
        SetUnsharedConstpoolIndex(JSTaggedValue(CONSTPOOL_TYPE_FLAG));
        SetSharedConstpoolId(JSTaggedValue(CONSTPOOL_TYPE_FLAG));
    }

    inline uint32_t GetNumOfCacheElement() const
    {
        return GetLengthOfCacheElement() * CACHE_NUM_TO_SIZE_FACTOR;
    }

    inline constexpr uint32_t GetNumOfExtendElement() const
    {
        return EXTEND_DATA_NUM;
    }

    inline uint32_t GetLengthOfConstPoolElement() const
    {
        return GetLength() - RESERVED_POOL_LENGTH;
    }

    inline void SetJSPandaFile(const void *jsPandaFile)
    {
        Barriers::SetPrimitive(GetData(), GetJSPandaFileOffset(), jsPandaFile);
    }

    inline JSPandaFile *GetJSPandaFile() const
    {
        return Barriers::GetPrimitive<JSPandaFile *>(GetData(), GetJSPandaFileOffset());
    }

    inline void InitConstantPoolTail(const JSThread *thread, JSHandle<ConstantPool> constPool)
    {
        SetAotArrayInfo(thread, constPool->GetAotArrayInfo(thread));
        SetAotHClassInfo(thread, constPool->GetAotHClassInfo(thread));
        SetConstantIndexInfo(thread, constPool->GetConstantIndexInfo(thread));
        SetAotSymbolInfo(thread, constPool->GetAotSymbolInfo(thread));
        SetProtoTransTableInfo(thread, constPool->GetProtoTransTableInfo(thread));
    }

    inline void SetConstantIndexInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - CONSTANT_INDEX_INFO_INDEX), info);
    }

    inline JSTaggedValue GetConstantIndexInfo(const JSThread *thread) const
    {
        return JSTaggedValue(Barriers::GetTaggedValue(thread, this, TaggedArray::DATA_OFFSET +
                                                                    GetConstantIndexInfoOffset()));
    }

    inline void SetAotArrayInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - AOT_ARRAY_INFO_INDEX), info);
    }

    inline JSTaggedValue GetAotArrayInfo(const JSThread *thread) const
    {
        return JSTaggedValue(Barriers::GetTaggedValue(thread, this, TaggedArray::DATA_OFFSET +
                                                                    GetAotArrayInfoOffset()));
    }

    inline JSTaggedValue GetAotSymbolInfo(const JSThread *thread) const
    {
        return JSTaggedValue(Barriers::GetTaggedValue(thread, this, TaggedArray::DATA_OFFSET +
                                                                    GetAotSymbolInfoOffset()));
    }

    inline JSTaggedValue GetProtoTransTableInfo(const JSThread *thread) const
    {
        return JSTaggedValue(Barriers::GetTaggedValue(thread, this, TaggedArray::DATA_OFFSET +
                                                                    GetProtoTransTableInfoOffset()));
    }

    static JSTaggedValue GetSymbolFromSymbolInfo(const JSThread *thread, JSHandle<TaggedArray> symbolInfoHandler,
                                                 uint64_t id)
    {
        uint32_t len = symbolInfoHandler->GetLength();
        for (uint32_t j = 0; j < len; j += 2) { // 2: symbolId, symbol
            ASSERT(j + 1 < len);
            uint64_t symbolId = symbolInfoHandler->Get(thread, j).GetRawData();
            if (symbolId == id) {
                return symbolInfoHandler->Get(thread, j + 1);
            }
        }
        return JSTaggedValue::Hole();
    }

    static JSTaggedValue GetSymbolFromSymbolInfo(const JSThread *thread, JSTaggedValue symbolInfo, uint64_t id)
    {
        TaggedArray* info = TaggedArray::Cast(symbolInfo.GetTaggedObject());
        uint32_t len = info->GetLength();
        for (uint32_t j = 0; j < len; j += 2) { // 2: symbolId, symbol
            ASSERT(j + 1 < len);
            uint64_t symbolId = info->Get(thread, j).GetRawData();
            if (symbolId == id) {
                return info->Get(thread, j + 1);
            }
        }
        return JSTaggedValue::Hole();
    }

    inline void SetAotHClassInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - AOT_HCLASS_INFO_INDEX), info);
    }

    inline JSTaggedValue GetAotHClassInfo(const JSThread *thread) const
    {
        return JSTaggedValue(Barriers::GetTaggedValue(thread, this, TaggedArray::DATA_OFFSET +
                                                                    GetAotHClassInfoOffset()));
    }

    inline void SetAotSymbolInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - AOT_SYMBOL_INFO_INDEX), info);
    }

    inline void SetProtoTransTableInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - PROTO_TRANS_TABLE_INFO_INDEX), info);
    }

    inline void SetObjectToCache(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        ASSERT(index < GetNumOfCacheElement());
        SetCompressed(thread, index, value);
    }

    static void CASSetObjectToCache(
        JSThread *thread, const JSTaggedValue constpool, uint32_t index, JSTaggedValue value)
    {
        ASSERT(index < ConstantPool::Cast(constpool.GetTaggedObject())->GetNumOfCacheElement());
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        JSHandle<ConstantPool> constpoolHandle(thread, constpool);
        size_t offset = index * CompressedJSTaggedValue::CompressedTaggedTypeSize();
        std::atomic<CompressedJSTaggedType> *atomicVal = reinterpret_cast<std::atomic<CompressedJSTaggedType> *>(
            ToUintPtr(taggedPool) + DATA_OFFSET + offset);
        TemporaryJSTaggedValue tempVal = taggedPool->GetObjectFromCacheUnsafe(thread, index);
        CompressedJSTaggedType expected = IsLoadingAOTMethodInfo(taggedPool->GetJSPandaFile(), tempVal)
            ? CompressedJSTaggedValue::Compress(tempVal).GetCompressedRawData()
            : CompressedJSTaggedValue::Hole().GetCompressedRawData();
        CompressedJSTaggedType desired = CompressedJSTaggedValue::Compress(value).GetCompressedRawData();
        if (std::atomic_compare_exchange_strong_explicit(atomicVal, &expected, desired,
            std::memory_order_release, std::memory_order_relaxed)) {
            // set val by Barrier.
            constpoolHandle->SetObjectToCache(thread, index, value);
        }
    }

    inline JSTaggedValue GetFromExtend(const JSThread *thread, uint32_t index) const
    {
        ASSERT(JSTaggedValue(this).IsConstantPool());
        ASSERT(index < GetNumOfExtendElement());
        uint32_t idx = index + GetLengthOfCacheElement();
        ASSERT(idx < GetLength());
        // Note: Here we can't statically decide the element type is a primitive or heap object, especially for
        //       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.
        size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
        // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
        return JSTaggedValue(Barriers::GetTaggedValue(thread, reinterpret_cast<JSTaggedType *>(ToUintPtr(this)),
                                                      DATA_OFFSET + offset));
    }

    static JSTaggedValue GetMethodFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        TemporaryJSTaggedValue val = taggedPool->GetObjectFromCacheUnsafe(thread, index);
        ASSERT(val.IsHeapObject() || val.IsHole() || val.IsInt());
        JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

        // For AOT
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        bool hasEntryIndex = false;
        uint32_t entryIndex = 0;
        if (IsLoadingAOTMethodInfo(jsPandaFile, val)) {
            int entryIndexVal = 0; // 0: only one method
            if (val.IsInt()) {
                // For MethodInfo which does not have ihc infos, we store codeEntry directly.
                entryIndexVal = val.GetInt();
            } else {
                JSHandle<AOTLiteralInfo> entryIndexes(thread, val.ConvertHeapObjectToJSTaggedValue());
                entryIndexVal = entryIndexes->GetObjectFromCache(thread, 0).GetInt(); // 0: only one method
            }
            if (entryIndexVal != static_cast<int>(AOTLiteralInfo::NO_FUNC_ENTRY_VALUE)) {
                hasEntryIndex = true;
                entryIndex = static_cast<uint32_t>(entryIndexVal);
            }
            val = TemporaryJSTaggedValue::Hole();
        }

        ASSERT(val.IsHeapObject() || val.IsHole());
        if (!val.IsHole()) {
            return val.ConvertHeapObjectToJSTaggedValue();
        }

        if (!taggedPool->GetJSPandaFile()->IsNewVersion()) {
            JSTaggedValue unsharedCp = thread->GetEcmaVM()->FindOrCreateUnsharedConstpool(constpool);
            taggedPool = ConstantPool::Cast(unsharedCp.GetTaggedObject());
            return taggedPool->GetObjectFromCache(thread, index);
        }

        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        ASSERT(jsPandaFile->IsNewVersion());
        JSHandle<ConstantPool> constpoolHandle(thread, constpool);
        EcmaVM *vm = thread->GetEcmaVM();

        EntityId id = constpoolHandle->GetEntityId(index);
        MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(id.GetOffset());
        CHECK_INPUT_NULLPTR(methodLiteral,
                            "GetMethodFromCache:methodLiteral is nullptr, offset: " + std::to_string(id.GetOffset()));
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<Method> method = factory->NewSMethod(
            jsPandaFile, methodLiteral, constpoolHandle, entryIndex, isLoadedAOT && hasEntryIndex);

        CASSetObjectToCache(thread, constpoolHandle.GetTaggedValue(), index, method.GetTaggedValue());
        return method.GetTaggedValue();
    }

    static JSTaggedValue PUBLIC_API GetMethodFromCache(JSTaggedValue constpool, uint32_t index, JSThread *thread);

    static void PUBLIC_API UpdateConstpoolWhenDeserialAI(EcmaVM *vm, JSHandle<ConstantPool> aiCP,
        JSHandle<ConstantPool> sharedCP, JSHandle<ConstantPool> unsharedCP);

    static bool PUBLIC_API IsAotMethodLiteralInfo(JSTaggedValue literalInfo);
    static JSTaggedValue PUBLIC_API GetIhcFromAOTLiteralInfo(JSThread *thread, JSTaggedValue constpool, uint32_t index);

    static JSTaggedValue GetClassLiteralFromCache(JSThread *thread, JSHandle<ConstantPool> constpool,
        uint32_t literal, CString entry, JSHandle<JSTaggedValue> sendableEnv = JSHandle<JSTaggedValue>(),
        ClassKind kind = ClassKind::NON_SENDABLE);

    static JSHandle<TaggedArray> GetFieldLiteral(JSThread *thread, JSHandle<ConstantPool> constpool,
                                                 uint32_t literal, CString entry);

    template <ConstPoolType type>
    static JSTaggedValue GetLiteralFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index, CString entry)
    {
        static_assert(type == ConstPoolType::OBJECT_LITERAL || type == ConstPoolType::ARRAY_LITERAL);
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        TemporaryJSTaggedValue val = taggedPool->GetObjectFromCacheUnsafe(thread, index);
        ASSERT(val.IsHeapObject() || val.IsHole());
        JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

        // For AOT
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        JSHandle<AOTLiteralInfo> entryIndexes(thread, JSTaggedValue::Undefined());
        if (isLoadedAOT && val.IsAOTLiteralInfo()) {
            entryIndexes = JSHandle<AOTLiteralInfo>(thread, val.ConvertHeapObjectToJSTaggedValue());
            val = TemporaryJSTaggedValue::Hole();
        }

        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);

            ASSERT(jsPandaFile->IsNewVersion());
            panda_file::File::EntityId id = taggedPool->GetEntityId(index);
            bool needSetAotFlag = isLoadedAOT && !entryIndexes.GetTaggedValue().IsUndefined();
            // New inst
            switch (type) {
                case ConstPoolType::OBJECT_LITERAL: {
                    JSMutableHandle<TaggedArray> elements(thread, JSTaggedValue::Undefined());
                    JSMutableHandle<TaggedArray> properties(thread, JSTaggedValue::Undefined());
                    LiteralDataExtractor::ExtractObjectDatas(thread, jsPandaFile, id, elements,
                        properties, constpoolHandle, entry, needSetAotFlag, entryIndexes);
                    JSTaggedValue ihcVal = JSTaggedValue::Undefined();
                    if (needSetAotFlag) {
                        ihcVal = entryIndexes->GetIhc(thread);
                        if (!ihcVal.IsUndefined()) {
                            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
                            JSHandle<JSTaggedValue> ihcHandle(thread, ihcVal);
                            JSHClass::SetPrototype(thread, JSHandle<JSHClass>(ihcHandle),
                                                   env->GetObjectFunctionPrototype());
                            ihcVal = ihcHandle.GetTaggedValue();
                            JSHClass::EnableProtoChangeMarker(thread, JSHandle<JSHClass>(thread, ihcVal));
                        }
                    }
                    JSHandle<JSObject> obj = JSObject::CreateObjectFromProperties(thread, properties, ihcVal);
                    auto profiler = thread->GetEcmaVM()->GetPGOProfiler();
                    profiler->RecordProfileType(obj->GetClass(), jsPandaFile, id.GetOffset());
                    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
                    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
                    size_t elementsLen = elements->GetLength();
                    for (size_t i = 0; i < elementsLen; i += 2) {  // 2: Each literal buffer has a pair of key-value.
                        key.Update(elements->Get(thread, i));
                        if (key->IsHole()) {
                            break;
                        }
                        valueHandle.Update(elements->Get(thread, i + 1));
                        JSObject::DefinePropertyByLiteral(thread, obj, key, valueHandle);
                    }
                    JSTaggedValue value = obj.GetTaggedValue();
                    constpoolHandle->SetObjectToCache(thread, index, value);
                    return value;
                }
                case ConstPoolType::ARRAY_LITERAL: {
                    // literal fetching from AOT ArrayInfos
                    JSMutableHandle<TaggedArray> literal(thread, JSTaggedValue::Undefined());
                    #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
                    ElementsKind dataKind = ElementsKind::GENERIC;
                    #else
                    ElementsKind dataKind = ElementsKind::NONE;
                    #endif
                    literal.Update(LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile, id,
                                                                            constpoolHandle, entry,
                                                                            needSetAotFlag, entryIndexes,
                                                                            &dataKind));
                    uint32_t length = literal->GetLength();
                    JSHandle<JSArray> arr(JSArray::ArrayCreate(thread, JSTaggedNumber(length), ArrayMode::LITERAL));
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    arr->SetElements(thread, literal);
                    if (thread->GetEcmaVM()->IsEnablePGOProfiler() || thread->GetEcmaVM()->IsEnableElementsKind() ||
                        thread->GetEcmaVM()->GetAOTFileManager()->IsEnableAOT()) {
                        // for all JSArray, the initial ElementsKind should be NONE
                        // Because AOT Stable Array Deopt check, we have support arrayLiteral elementskind
                        // If array is loaded from AOT, no need to do migration.
                        JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
                        auto classIndex = static_cast<size_t>(GlobalEnvField::ELEMENT_NONE_HCLASS_INDEX);
                        auto hclassVal = globalEnv->GetGlobalEnvObjectByIndex(classIndex).GetTaggedValue();
                        arr->SynchronizedTransitionClass(thread, JSHClass::Cast(hclassVal.GetTaggedObject()));
                        ElementsKind oldKind = arr->GetClass()->GetElementsKind();
                        JSHClass::TransitToElementsKind(thread, arr, dataKind);
                        ElementsKind newKind = arr->GetClass()->GetElementsKind();
                        JSHandle<JSObject> receiver(arr);
                        Elements::MigrateArrayWithKind(thread, receiver, oldKind, newKind);
                    }
                    JSTaggedValue value = arr.GetTaggedValue();
                    constpoolHandle->SetObjectToCache(thread, index, value);
                    return value;
                }
                default:
                    LOG_FULL(FATAL) << "Unknown type: " << static_cast<uint8_t>(type);
                    UNREACHABLE();
            }
        }

        return val.ConvertHeapObjectToJSTaggedValue();
    }

    template <ConstPoolType type>
    static JSTaggedValue GetLiteralFromCacheNoScope(JSThread *thread, JSTaggedValue constpool,
                                                    uint32_t index, [[maybe_unused]] CString entry)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        TemporaryJSTaggedValue val = taggedPool->GetObjectFromCacheUnsafe(thread, index);
        ASSERT(val.IsHeapObject() || val.IsHole());
        JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        if (isLoadedAOT && val.IsAOTLiteralInfo()) {
            return JSTaggedValue::Undefined();
        }
        return val.IsHole() ? JSTaggedValue::Undefined() : val.ConvertHeapObjectToJSTaggedValue();
    }

    static panda_file::File::EntityId GetIdFromCache(JSTaggedValue constpool, uint32_t index)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        panda_file::File::EntityId id = taggedPool->GetEntityId(index);
        return id;
    }

    template <ConstPoolType type>
    static JSTaggedValue GetLiteralFromCache(JSThread *thread, JSTaggedValue constpool,
                                             uint32_t index, JSTaggedValue module)
    {
        CString entry = ModuleManager::GetRecordName(thread, module);
        return GetLiteralFromCache<type>(thread, constpool, index, entry);
    }

    static JSTaggedValue PUBLIC_API GetStringFromCacheForJit(
        JSThread *thread, JSTaggedValue constpool, uint32_t index, bool allowAlloc = true);

    static JSTaggedValue PUBLIC_API GetStringFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index);

    // may return a fake JSTaggedValue, which consists of the high bit of heap address and the
    // low bit of `JSTaggedValue::Hole`.
    inline TemporaryJSTaggedValue GetObjectFromCacheUnsafe(const JSThread *thread, uint32_t index) const
    {
        ASSERT(index < GetNumOfCacheElement());
        return GetFromCompressed(thread, index);
    }

    inline JSTaggedValue GetObjectFromCache(const JSThread *thread, uint32_t index) const
    {
        TemporaryJSTaggedValue res = GetObjectFromCacheUnsafe(thread, index);
        return res.ConvertToJSTaggedValue();
    }

    DECL_VISIT_CONST_POOL(DATA_OFFSET, GetLengthOfCacheElement(), GetLengthOfConstPoolElement(), GetLength());

    DECL_DUMP()

private:
    inline uint32_t GetLengthOfCacheElement() const
    {
        return GetLength() - RESERVED_POOL_LENGTH - EXTEND_DATA_NUM;
    }

    template <RBMode mode = RBMode::DEFAULT_RB>
    TemporaryJSTaggedValue GetFromCompressed(const JSThread *thread, uint32_t idx) const
    {
        ASSERT(JSTaggedValue(this).IsConstantPool());
        // Note: Here we can't statically decide the element type is a primitive or heap object, especially for
        //       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.
        size_t offset = CompressedJSTaggedValue::CompressedTaggedTypeSize() * idx;
        ASSERT(offset < GetLength() * JSTaggedValue::TaggedTypeSize());
        // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
        return TemporaryJSTaggedValue(Barriers::GetFromCompressedTaggedValue<mode>(
            thread, JSTaggedValue(this), DATA_OFFSET + offset));
    }

    template<bool needBarrier = true, typename T = JSTaggedValue>
    void Set(const JSThread *thread, uint32_t idx, const T &value)
    {
        ASSERT(JSTaggedValue(this).IsConstantPool());
        ASSERT(idx < GetLength());
        size_t offset = JSTaggedValue::TaggedTypeSize() * idx;

        if constexpr (std::is_same_v<T, JSTaggedValue>) {
            if (needBarrier && value.IsHeapObject()) {
                Barriers::SetObject<true>(thread, reinterpret_cast<void*>(this), offset + DATA_OFFSET,
                                          value.GetRawData());
            } else {
                Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, value.GetRawData());
            }
        } else if constexpr (IsJSHandle<T>::value) {
            auto taggedValue = value.GetTaggedValue();
            if (taggedValue.IsHeapObject()) {
                Barriers::SetObject<true>(thread, reinterpret_cast<void*>(this),
                                        offset + DATA_OFFSET, taggedValue.GetRawData());
            } else {
                Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, taggedValue.GetRawData());
            }
        } else {
            static_assert(!std::is_same_v<T, T>, "T must be either JSTaggedValue or JSHandle<>");
        }
    }

    template<bool needBarrier = true>
    void SetCompressed(const JSThread *thread, uint32_t idx, JSTaggedValue value)
    {
        ASSERT(JSTaggedValue(this).IsConstantPool());
        size_t offset = CompressedJSTaggedValue::CompressedTaggedTypeSize() * idx;
        ASSERT(offset < GetLength() * JSTaggedValue::TaggedTypeSize());

        if (needBarrier && value.IsHeapObject()) {
            Barriers::SetCompressedObject<true>(
                thread, reinterpret_cast<void*>(this), offset + DATA_OFFSET, value.GetRawData());
        } else {
            Barriers::SetPrimitive<CompressedJSTaggedType>(
                GetData(), offset, CompressedJSTaggedValue::Compress(value).GetCompressedRawData());
        }
    }

    inline size_t GetJSPandaFileOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - JS_PANDA_FILE_INDEX);
    }

    inline size_t GetIndexHeaderOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - INDEX_HEADER_INDEX);
    }

    inline size_t GetConstantIndexInfoOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - CONSTANT_INDEX_INFO_INDEX);
    }

    inline size_t GetAotArrayInfoOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_ARRAY_INFO_INDEX);
    }

    inline size_t GetAotHClassInfoOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_HCLASS_INFO_INDEX);
    }

    inline size_t GetUnsharedConstpoolIndexOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - UNSHARED_CONSTPOOL_INDEX);
    }

    inline size_t GetSharedConstpoolIdOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - SHARED_CONSTPOOL_ID);
    }

    inline size_t GetAotSymbolInfoOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_SYMBOL_INFO_INDEX);
    }

    inline size_t GetProtoTransTableInfoOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - PROTO_TRANS_TABLE_INFO_INDEX);
    }

    inline size_t GetLastOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * GetLength() + DATA_OFFSET;
    }

    static bool IsLoadingAOTMethodInfo(const JSPandaFile *pf, TemporaryJSTaggedValue cachedVal)
    {
        // Two types of AOT method infos are stored in the constpool:
        // 1. AOTLiteralInfo which includes function entry index and ihc/chc
        // 2. An int value(function entry index) if ihc/chc is not needed
        return pf->IsLoadedAOT() && (cachedVal.IsAOTLiteralInfo() || cachedVal.IsInt());
    };

    static JSHandle<JSTaggedValue> GetDeserializedConstantPool(
        EcmaVM *vm, const JSPandaFile *jsPandaFile, int32_t cpID);
    static void MergeObjectLiteralHClassCache(EcmaVM *vm, const JSHandle<JSTaggedValue> &pool);

    friend BaseSerializer;
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
