/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"

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

/*                  ConstantPool
 *      +--------------------------------+----
 *      |           ReviseData           |  ^
 *      |              ...               |  |
 *      |        string(EcmaString)      |  |
 *      |        method(Method)          |cacheLength
 *      |     array literal(JSArray)     |  |
 *      |    object literal(JSObject)    |  |
 *      |   class literal(ClassLiteral)  |  v
 *      +--------------------------------+----
 *      |          AOTHClassInfo         |TaggedArray
 *      +--------------------------------+----
 *      |          AOTArrayInfo          |TaggedArray
 *      +--------------------------------+----
 *      |         constIndexInfo         |TaggedArray
 *      +--------------------------------+----
 *      |           IndexHeader          |
 *      +--------------------------------+
 *      |           JSPandaFile          |
 *      +--------------------------------+
 */
class ConstantPool : public TaggedArray {
public:
    static constexpr size_t JS_PANDA_FILE_INDEX = 1; // not need gc
    static constexpr size_t INDEX_HEADER_INDEX = 2; // not need gc
    static constexpr size_t CONSTANT_INDEX_INFO_INDEX = 3;
    static constexpr size_t AOT_ARRAY_INFO_INDEX = 4;
    static constexpr size_t AOT_HCLASS_INFO_INDEX = 5;
    static constexpr size_t UNSHARED_CONSTPOOL_INDEX = 6;
    static constexpr size_t RESERVED_POOL_LENGTH = INDEX_HEADER_INDEX; // divide the gc area

    static constexpr size_t EXTEND_DATA_NUM = 4; // AOTHClassInfo, AOTArrayInfo, ConstIndexInfo. unsharedConstpoolIndex

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

        JSHandle<ConstantPool> constpool(vm->GetJSThread(), JSTaggedValue::Hole());
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        if (isLoadedAOT) {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
            int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
            // TODO(aot) aot need to adapt. deserialize share constpool.
            constpool = GetDeserializedConstantPool(vm, jsPandaFile, index);
#else
            LOG_FULL(FATAL) << "Aot don't support Windows and MacOS platform";
            UNREACHABLE();
#endif
        }
        if (constpool.GetTaggedValue().IsHole()) {
            ObjectFactory *factory = vm->GetFactory();
            constpool = factory->NewConstantPool(constpoolSize);
        }

        constpool->SetJSPandaFile(jsPandaFile);
        constpool->SetIndexHeader(mainIndex);

        return constpool;
    }

    static JSHandle<ConstantPool> CreateSharedConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                       panda_file::File::EntityId id,
                                                       int32_t unsharedConstpoolIndex = 0)
    {
        const panda_file::File::IndexHeader *mainIndex = jsPandaFile->GetPandaFile()->GetIndexHeader(id);
        LOG_ECMA_IF(mainIndex == nullptr, FATAL) << "Unknown methodId: " << id.GetOffset();
        auto constpoolSize = mainIndex->method_idx_size;

        JSHandle<ConstantPool> constpool(vm->GetJSThread(), JSTaggedValue::Hole());
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        if (isLoadedAOT) {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), id);
            int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
            // TODO aot need to adapt.
            constpool = GetDeserializedConstantPool(vm, jsPandaFile, index);
#else
            LOG_FULL(FATAL) << "Aot don't support Windows and MacOS platform";
            UNREACHABLE();
#endif
        }
        if (constpool.GetTaggedValue().IsHole()) {
            ObjectFactory *factory = vm->GetFactory();
            constpool = factory->NewSConstantPool(constpoolSize);
        }

        constpool->SetJSPandaFile(jsPandaFile);
        constpool->SetIndexHeader(mainIndex);
        constpool->SetUnsharedConstpoolIndex(JSTaggedValue(unsharedConstpoolIndex));

        return constpool;
    }

    static int32_t CheckUnsharedConstpool(JSTaggedValue constpool)
    {
        int32_t index = ConstantPool::Cast(constpool.GetTaggedObject())->GetUnsharedConstpoolIndex().GetInt();
        ASSERT(index == CONSTPOOL_TYPE_FLAG);
        return index;
    }

    inline void SetUnsharedConstpoolIndex(const JSTaggedValue index)
    {
        Barriers::SetPrimitive(GetData(), GetUnsharedConstpoolIndexOffset(), index);
    }

    inline JSTaggedValue GetUnsharedConstpoolIndex() const
    {
        return Barriers::GetValue<JSTaggedValue>(GetData(), GetUnsharedConstpoolIndexOffset());
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
        SetAotHClassInfo(array.GetTaggedValue());
        SetAotArrayInfo(array.GetTaggedValue());
        SetConstantIndexInfo(array.GetTaggedValue());
        SetJSPandaFile(nullptr);
        SetIndexHeader(nullptr);
        SetUnsharedConstpoolIndex(JSTaggedValue(CONSTPOOL_TYPE_FLAG));
    }

    inline uint32_t GetCacheLength() const
    {
        return GetLength() - RESERVED_POOL_LENGTH;
    }

    inline void SetJSPandaFile(const void *jsPandaFile)
    {
        Barriers::SetPrimitive(GetData(), GetJSPandaFileOffset(), jsPandaFile);
    }

    inline JSPandaFile *GetJSPandaFile() const
    {
        return Barriers::GetValue<JSPandaFile *>(GetData(), GetJSPandaFileOffset());
    }

    inline void SetConstantIndexInfo(JSTaggedValue info)
    {
        Barriers::SetPrimitive(GetData(), GetConstantIndexInfoOffset(), info.GetRawData());
    }

    inline void SetAotArrayInfo(JSTaggedValue info)
    {
        Barriers::SetPrimitive(GetData(), GetAotArrayInfoOffset(), info.GetRawData());
    }

    inline JSTaggedValue GetAotArrayInfo()
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetAotArrayInfoOffset()));
    }

    inline void SetAotHClassInfo(JSTaggedValue info)
    {
        Barriers::SetPrimitive(GetData(), GetAotHClassInfoOffset(), info.GetRawData());
    }

    inline void SetObjectToCache(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        Set(thread, index, value);
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
        if (isLoadedAOT && val.IsAOTLiteralInfo()) {
            JSHandle<AOTLiteralInfo> entryIndexes(thread, val);
            int entryIndexVal = entryIndexes->GetObjectFromCache(0).GetInt(); // 0: only one method
            if (entryIndexVal != static_cast<int>(AOTLiteralInfo::NO_FUNC_ENTRY_VALUE)) {
                hasEntryIndex = true;
                entryIndex = static_cast<uint32_t>(entryIndexVal);
            }
            val = JSTaggedValue::Hole();
        }

        if (!val.IsHole()) {
            return val;
        }

        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        ASSERT(jsPandaFile->IsNewVersion());
        JSHandle<ConstantPool> constpoolHandle(thread, constpool);
        EcmaVM *vm = thread->GetEcmaVM();

        EntityId id = constpoolHandle->GetEntityId(index);
        MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(id.GetOffset());
        ASSERT(methodLiteral != nullptr);
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<Method> method = factory->NewSMethod(
            jsPandaFile, methodLiteral, constpoolHandle, entryIndex, isLoadedAOT && hasEntryIndex);
        constpoolHandle->SetObjectToCache(thread, index, method.GetTaggedValue());
        return method.GetTaggedValue();
    }

    static JSTaggedValue GetClassLiteralFromCache(JSThread *thread, JSHandle<ConstantPool> constpool,
        uint32_t literal, CString entry, ClassKind kind = ClassKind::NON_SENDABLE)
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        // Do not use cache when sendable for get wrong obj from cache,
        // shall be fix or refactor during shared object implements
        JSTaggedValue val = constpool->GetObjectFromCache(literal);
        JSPandaFile *jsPandaFile = constpool->GetJSPandaFile();

        // For AOT
        bool isLoadedAOT = jsPandaFile->IsLoadedAOT();
        JSHandle<AOTLiteralInfo> entryIndexes(thread, JSTaggedValue::Undefined());
        if (isLoadedAOT && val.IsAOTLiteralInfo()) {
            entryIndexes = JSHandle<AOTLiteralInfo>(thread, val);
            val = JSTaggedValue::Hole();
        }

        if (val.IsHole()) {
            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();
            ASSERT(jsPandaFile->IsNewVersion());
            panda_file::File::EntityId literalId = constpool->GetEntityId(literal);
            bool needSetAotFlag = isLoadedAOT && !entryIndexes.GetTaggedValue().IsUndefined();
            JSHandle<TaggedArray> literalArray = LiteralDataExtractor::GetDatasIgnoreType(
                thread, jsPandaFile, literalId, constpool, entry, needSetAotFlag, entryIndexes, nullptr, kind);
            JSHandle<ClassLiteral> classLiteral;
            if (kind == ClassKind::SENDABLE) {
                classLiteral = factory->NewSClassLiteral();
            } else {
                classLiteral = factory->NewClassLiteral();
            }
            classLiteral->SetArray(thread, literalArray);
            val = classLiteral.GetTaggedValue();
            constpool->SetObjectToCache(thread, literal, val);
        }

        return val;
    }

    static JSHandle<TaggedArray> GetFieldLiteral(JSThread *thread, JSHandle<ConstantPool> constpool,
                                                 uint32_t literal, CString entry)
    {
        JSPandaFile *jsPandaFile = constpool->GetJSPandaFile();
        JSHandle<AOTLiteralInfo> entryIndexes(thread, JSTaggedValue::Undefined());
        ASSERT(jsPandaFile->IsNewVersion());
        panda_file::File::EntityId literalId(literal);
        JSHandle<TaggedArray> literalArray = LiteralDataExtractor::GetDatasIgnoreType(
            thread, jsPandaFile, literalId, constpool, entry, false, entryIndexes);
        return literalArray;
    }

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
                    if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
                        pgo::ApEntityId abcId(0);
                        pgo::PGOProfilerManager::GetInstance()->GetPandaFileId(jsPandaFile->GetJSPandaFileDesc(),
                                                                               abcId);
                        thread->GetEcmaVM()->GetPGOProfiler()->ProfileCreateObject(obj.GetTaggedType(), abcId,
                                                                                   id.GetOffset());
                    }
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
                    ElementsKind dataKind = ElementsKind::NONE;
                    if (!constpoolHandle->TryGetAOTArrayLiteral(thread, needSetAotFlag,
                                                                entryIndexes, literal, &dataKind)) {
                        literal.Update(LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile, id,
                                                                                constpoolHandle, entry,
                                                                                needSetAotFlag, entryIndexes,
                                                                                &dataKind));
                    }
                    uint32_t length = literal->GetLength();
                    JSHandle<JSArray> arr(JSArray::ArrayCreate(thread, JSTaggedNumber(length), ArrayMode::LITERAL));
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    arr->SetElements(thread, literal);
                    if (thread->GetEcmaVM()->IsEnablePGOProfiler() || thread->GetEcmaVM()->IsEnableElementsKind()) {
                        // for all JSArray, the initial ElementsKind should be NONE
                        // Because AOT Stable Array Deopt check, we have support arrayLiteral elementskind
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

    bool TryGetAOTArrayLiteral(JSThread *thread, bool loadAOT, JSHandle<AOTLiteralInfo> entryIndexes,
                               JSMutableHandle<TaggedArray> literal, ElementsKind *dataKind)
    {
        if (loadAOT) {
            int elementIndex = entryIndexes->GetElementIndex();
            if (elementIndex != kungfu::BaseSnapshotInfo::AOT_ELEMENT_INDEX_DEFAULT_VALUE) {
                JSTaggedValue arrayInfos = GetAotArrayInfo();
                JSHandle<TaggedArray> aotArrayInfos(thread, arrayInfos);
                literal.Update(aotArrayInfos->Get(elementIndex));
                *dataKind = ElementsKind::HOLE_TAGGED;
                return true;
            }
        }
        return false;
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

    static JSTaggedValue PUBLIC_API GetStringFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        auto val = taggedPool->Get(index);
        if (val.IsHole()) {
            [[maybe_unused]] EcmaHandleScope handleScope(thread);

            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::EntityId id = taggedPool->GetEntityId(index);
            auto foundStr = jsPandaFile->GetStringData(id);

            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
            auto string = factory->GetRawStringFromStringTable(foundStr, MemSpaceType::SHARED_OLD_SPACE,
                jsPandaFile->IsFirstMergedAbc(), id.GetOffset());

            val = JSTaggedValue(string);
            constpoolHandle->SetObjectToCache(thread, index, val);
        }

        return val;
    }

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

    inline size_t GetLastOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * GetLength() + DATA_OFFSET;
    }

    static JSHandle<ConstantPool> GetDeserializedConstantPool(EcmaVM *vm, const JSPandaFile *jsPandaFile, int32_t cpID);
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
