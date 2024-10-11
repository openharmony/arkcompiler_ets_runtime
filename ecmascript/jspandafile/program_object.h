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
#include "ecmascript/ecma_context.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/tagged_array-inl.h"

#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "libpandafile/index_accessor.h"

namespace panda {
namespace ecmascript {
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
 *      |               ...              |       ^           ^          ^   index 0
 *      |  Method / AOTLiteralInfo / Int |       |           |          |
 *      |              String            |       |           |          |
 *      |           Array Literal        | ConstpoolLength   |          |
 *      |           Class Literal        |       |           |          |
 *      |           Object Literal       |       |           |          |
 *      |               ...              |       v           |          |
 *      +--------------------------------+---------------    |          |
 *      |          ProtoTransTableInfo   |PointerToIndexDic  |          |
 *      +--------------------------------+---------------    |          |
 *      |          AOTSymbolInfo         |TaggedArray        |          |
 *      +--------------------------------+---------------    |          |
 *      |      unshared constpool index  |int32_t        CacheLength    |
 *      +--------------------------------+---------------    |        Length
 *      |      shared constpool id       |int32_t            |          |
 *      +--------------------------------+---------------    |          |
 *      |          AOTHClassInfo         |TaggedArray        |          |
 *      +--------------------------------+---------------    |          |
 *      |          AOTArrayInfo          |TaggedArray        |          |
 *      +--------------------------------+---------------    |          |
 *      |         constIndexInfo         |TaggedArray        v          |
 *      +--------------------------------+--------------------------    |
 *      |           IndexHeader          |                              |
 *      +--------------------------------+                              |
 *      |           JSPandaFile          |                              v    index: Length-1
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
    static constexpr int32_t CONSTPOOL_INVALID_ID = 0;

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

        JSHandle<ConstantPool> constpool(vm->GetJSThread(), JSTaggedValue::Hole());
        if (constpool.GetTaggedValue().IsHole()) {
            ObjectFactory *factory = vm->GetFactory();
            constpool = factory->NewSConstantPool(constpoolSize);
        }

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
        JSHandle<ConstantPool> sconstpool(vm->GetJSThread(), JSTaggedValue::Hole());
        uint32_t capacity = constpool->GetConstpoolLength();
        if (sconstpool.GetTaggedValue().IsHole()) {
            ObjectFactory *factory = vm->GetFactory();
            sconstpool = factory->NewSConstantPool(capacity);
        }
        JSThread *thread = vm->GetJSThread();
        for (uint32_t i = 0; i < capacity; i++) {
            JSTaggedValue val = constpool->GetObjectFromCache(i);
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
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<AOTLiteralInfo> dst = factory->NewSAOTLiteralInfo(1);
        for (uint32_t i = 0; i < src->GetCacheLength(); i++) {
            JSTaggedValue val = src->GetObjectFromCache(i);
            ASSERT(!val.IsHeapObject() || val.IsJSShared());
            dst->SetObjectToCache(vm->GetJSThread(), i, val);
        }
        dst->SetLiteralType(JSTaggedValue(src->GetLiteralType()));
        return dst;
    }

    static bool CheckUnsharedConstpool(JSTaggedValue constpool)
    {
        int32_t index = ConstantPool::Cast(constpool.GetTaggedObject())->GetUnsharedConstpoolIndex();
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
        return Barriers::GetValue<JSTaggedValue>(GetData(), GetUnsharedConstpoolIndexOffset()).GetInt();
    }

    inline void SetSharedConstpoolId(const JSTaggedValue index)
    {
        Barriers::SetPrimitive(GetData(), GetSharedConstpoolIdOffset(), index);
    }

    inline JSTaggedValue GetSharedConstpoolId() const
    {
        return Barriers::GetValue<JSTaggedValue>(GetData(), GetSharedConstpoolIdOffset());
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
        return Barriers::GetValue<panda_file::File::IndexHeader *>(GetData(), GetIndexHeaderOffset());
    }

    static size_t ComputeSize(uint32_t cacheSize)
    {
        return TaggedArray::ComputeSize(
            JSTaggedValue::TaggedTypeSize(), cacheSize + EXTEND_DATA_NUM + RESERVED_POOL_LENGTH);
    }

    void InitializeWithSpecialValue(JSThread *thread, JSTaggedValue initValue,
        uint32_t capacity, uint32_t extraLength = 0)
    {
        ASSERT(initValue.IsSpecial());
        SetLength(capacity + EXTEND_DATA_NUM + RESERVED_POOL_LENGTH);
        SetExtraLength(extraLength);
        for (uint32_t i = 0; i < capacity; i++) {
            size_t offset = JSTaggedValue::TaggedTypeSize() * i;
            Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, initValue.GetRawData());
        }
        JSHandle<TaggedArray> array(thread->GlobalConstants()->GetHandledEmptyArray());
        SetAotSymbolInfo(thread, array.GetTaggedValue());
        SetProtoTransTableInfo(thread, JSTaggedValue::Undefined());
        SetAotHClassInfo(thread, array.GetTaggedValue());
        SetAotArrayInfo(thread, array.GetTaggedValue());
        SetConstantIndexInfo(thread, array.GetTaggedValue());
        SetJSPandaFile(nullptr);
        SetIndexHeader(nullptr);
        SetUnsharedConstpoolIndex(JSTaggedValue(CONSTPOOL_TYPE_FLAG));
        SetSharedConstpoolId(JSTaggedValue(CONSTPOOL_INVALID_ID));
    }

    inline uint32_t GetCacheLength() const
    {
        return GetLength() - RESERVED_POOL_LENGTH;
    }

    inline uint32_t GetConstpoolLength() const
    {
        return GetLength() - RESERVED_POOL_LENGTH - EXTEND_DATA_NUM;
    }

    inline void SetJSPandaFile(const void *jsPandaFile)
    {
        Barriers::SetPrimitive(GetData(), GetJSPandaFileOffset(), jsPandaFile);
    }

    inline JSPandaFile *GetJSPandaFile() const
    {
        return Barriers::GetValue<JSPandaFile *>(GetData(), GetJSPandaFileOffset());
    }

    inline void InitConstantPoolTail(const JSThread *thread, JSHandle<ConstantPool> constPool)
    {
        SetAotArrayInfo(thread, constPool->GetAotArrayInfo());
        SetAotHClassInfo(thread, constPool->GetAotHClassInfo());
        SetConstantIndexInfo(thread, constPool->GetConstantIndexInfo());
        SetAotSymbolInfo(thread, constPool->GetAotSymbolInfo());
        SetProtoTransTableInfo(thread, constPool->GetProtoTransTableInfo());
    }

    inline void SetConstantIndexInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - CONSTANT_INDEX_INFO_INDEX), info);
    }

    inline JSTaggedValue GetConstantIndexInfo() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetConstantIndexInfoOffset()));
    }

    inline void SetAotArrayInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - AOT_ARRAY_INFO_INDEX), info);
    }

    inline JSTaggedValue GetAotArrayInfo() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetAotArrayInfoOffset()));
    }

    inline JSTaggedValue GetAotSymbolInfo() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetAotSymbolInfoOffset()));
    }

    inline JSTaggedValue GetProtoTransTableInfo() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetProtoTransTableInfoOffset()));
    }

    static JSTaggedValue GetSymbolFromSymbolInfo(JSHandle<TaggedArray> symbolInfoHandler, uint64_t id)
    {
        uint32_t len = symbolInfoHandler->GetLength();
        for (uint32_t j = 0; j < len; j += 2) { // 2: symbolId, symbol
            ASSERT(j + 1 < len);
            uint64_t symbolId = symbolInfoHandler->Get(j).GetRawData();
            if (symbolId == id) {
                return symbolInfoHandler->Get(j + 1);
            }
        }
        return JSTaggedValue::Hole();
    }

    static JSTaggedValue GetSymbolFromSymbolInfo(JSTaggedValue symbolInfo, uint64_t id)
    {
        TaggedArray* info = TaggedArray::Cast(symbolInfo.GetTaggedObject());
        uint32_t len = info->GetLength();
        for (uint32_t j = 0; j < len; j += 2) { // 2: symbolId, symbol
            ASSERT(j + 1 < len);
            uint64_t symbolId = info->Get(j).GetRawData();
            if (symbolId == id) {
                return info->Get(j + 1);
            }
        }
        return JSTaggedValue::Hole();
    }

    inline void SetAotHClassInfo(const JSThread *thread, JSTaggedValue info)
    {
        Set(thread, (GetLength() - AOT_HCLASS_INFO_INDEX), info);
    }

    inline JSTaggedValue GetAotHClassInfo() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetAotHClassInfoOffset()));
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
        Set(thread, index, value);
    }

    static void CASSetObjectToCache(
        JSThread *thread, const JSTaggedValue constpool, uint32_t index, JSTaggedValue value)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        JSHandle<ConstantPool> constpoolHandle(thread, constpool);
        std::atomic<JSTaggedValue> *atomicVal = reinterpret_cast<std::atomic<JSTaggedValue> *>(
            reinterpret_cast<uintptr_t>(taggedPool) + DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        JSTaggedValue tempVal = taggedPool->GetObjectFromCache(index);
        JSTaggedValue expected = IsLoadedMethodInfoFromAOT(taggedPool->GetJSPandaFile(), tempVal) ? tempVal :
            JSTaggedValue::Hole();
        JSTaggedValue desired = value;
        if (std::atomic_compare_exchange_strong_explicit(atomicVal, &expected, desired,
            std::memory_order_release, std::memory_order_relaxed)) {
            // set val by Barrier.
            constpoolHandle->SetObjectToCache(thread, index, value);
        }
    }

    inline JSTaggedValue GetObjectFromCache(uint32_t index) const
    {
        return Get(index);
    }

    static JSTaggedValue GetMethodFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        auto val = taggedPool->GetObjectFromCache(index);
        JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

        // For AOT
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        bool hasEntryIndex = false;
        uint32_t entryIndex = 0;
        if (IsLoadedMethodInfoFromAOT(jsPandaFile, val)) {
            int entryIndexVal = 0; // 0: only one method
            if (val.IsInt()) {
                // For MethodInfo which does not have ihc infos, we store codeEntry directly.
                entryIndexVal = val.GetInt();
            } else {
                JSHandle<AOTLiteralInfo> entryIndexes(thread, val);
                entryIndexVal = entryIndexes->GetObjectFromCache(0).GetInt(); // 0: only one method
            }
            if (entryIndexVal != static_cast<int>(AOTLiteralInfo::NO_FUNC_ENTRY_VALUE)) {
                hasEntryIndex = true;
                entryIndex = static_cast<uint32_t>(entryIndexVal);
            }
            val = JSTaggedValue::Hole();
        }

        if (!val.IsHole()) {
            return val;
        }

        if (!taggedPool->GetJSPandaFile()->IsNewVersion()) {
            JSTaggedValue unsharedCp = thread->GetCurrentEcmaContext()->FindOrCreateUnsharedConstpool(constpool);
            taggedPool = ConstantPool::Cast(unsharedCp.GetTaggedObject());
            return taggedPool->Get(index);
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

    static JSTaggedValue PUBLIC_API GetMethodFromCache(JSTaggedValue constpool, uint32_t index);

    static void PUBLIC_API UpdateConstpoolWhenDeserialAI(EcmaVM *vm, JSHandle<ConstantPool> aiCP,
        JSHandle<ConstantPool> sharedCP, JSHandle<ConstantPool> unsharedCP);

    static bool PUBLIC_API IsAotMethodLiteralInfo(JSTaggedValue literalInfo);

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
        auto val = taggedPool->GetObjectFromCache(index);
        JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

        // For AOT
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        JSHandle<AOTLiteralInfo> entryIndexes(thread, JSTaggedValue::Undefined());
        if (isLoadedAOT && val.IsAOTLiteralInfo()) {
            entryIndexes = JSHandle<AOTLiteralInfo>(thread, val);
            val = JSTaggedValue::Hole();
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
                        ihcVal = entryIndexes->GetIhc();
                        if (!ihcVal.IsUndefined()) {
                            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
                            JSHClass::Cast(ihcVal.GetTaggedObject())->SetPrototype(thread,
                                                                                   env->GetObjectFunctionPrototype());
                        }
                    }
                    JSHandle<JSObject> obj = JSObject::CreateObjectFromProperties(thread, properties, ihcVal);
                    auto profiler = thread->GetEcmaVM()->GetPGOProfiler();
                    profiler->RecordProfileType(obj->GetClass(), jsPandaFile, id.GetOffset());
                    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
                    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
                    size_t elementsLen = elements->GetLength();
                    for (size_t i = 0; i < elementsLen; i += 2) {  // 2: Each literal buffer has a pair of key-value.
                        key.Update(elements->Get(i));
                        if (key->IsHole()) {
                            break;
                        }
                        valueHandle.Update(elements->Get(i + 1));
                        JSObject::DefinePropertyByLiteral(thread, obj, key, valueHandle);
                    }
                    val = obj.GetTaggedValue();
                    break;
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
                    if (thread->GetEcmaVM()->IsEnablePGOProfiler() || thread->GetEcmaVM()->IsEnableElementsKind()) {
                        // for all JSArray, the initial ElementsKind should be NONE
                        // Because AOT Stable Array Deopt check, we have support arrayLiteral elementskind
                        // If array is loaded from AOT, no need to do migration.
                        auto globalConstant = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
                        auto classIndex = static_cast<size_t>(ConstantIndex::ELEMENT_NONE_HCLASS_INDEX);
                        auto hclassVal = globalConstant->GetGlobalConstantObject(classIndex);
                        arr->SynchronizedSetClass(thread, JSHClass::Cast(hclassVal.GetTaggedObject()));
                        ElementsKind oldKind = arr->GetClass()->GetElementsKind();
                        JSHClass::TransitToElementsKind(thread, arr, dataKind);
                        ElementsKind newKind = arr->GetClass()->GetElementsKind();
                        JSHandle<JSObject> receiver(arr);
                        Elements::MigrateArrayWithKind(thread, receiver, oldKind, newKind);
                    }
                    val = arr.GetTaggedValue();
                    break;
                }
                default:
                    LOG_FULL(FATAL) << "Unknown type: " << static_cast<uint8_t>(type);
                    UNREACHABLE();
            }
            constpoolHandle->SetObjectToCache(thread, index, val);
        }

        return val;
    }

    template <ConstPoolType type>
    static JSTaggedValue GetLiteralFromCache(JSTaggedValue constpool, uint32_t index, [[maybe_unused]] CString entry)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        auto val = taggedPool->GetObjectFromCache(index);
        JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        if (isLoadedAOT && val.IsAOTLiteralInfo()) {
            val = JSTaggedValue::Hole();
        }
        return val.IsHole() ? JSTaggedValue::Undefined() : val;
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
        CString entry = ModuleManager::GetRecordName(module);
        return GetLiteralFromCache<type>(thread, constpool, index, entry);
    }

    static JSTaggedValue PUBLIC_API GetStringFromCacheForJit(JSThread *thread, JSTaggedValue constpool, uint32_t index,
        bool allowAlloc = true);

    static JSTaggedValue PUBLIC_API GetStringFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index);

    DECL_VISIT_ARRAY(DATA_OFFSET, GetCacheLength(), GetLength());

    DECL_DUMP()

private:
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

    static bool IsLoadedMethodInfoFromAOT(const JSPandaFile *pf, JSTaggedValue cachedVal)
    {
        return pf->IsLoadedAOT() && (cachedVal.IsAOTLiteralInfo() || cachedVal.IsInt());
    };

    static JSHandle<JSTaggedValue> GetDeserializedConstantPool(
        EcmaVM *vm, const JSPandaFile *jsPandaFile, int32_t cpID);
    static void MergeObjectLiteralHClassCache(EcmaVM *vm, const JSHandle<JSTaggedValue> &pool);
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
