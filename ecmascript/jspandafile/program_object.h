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

#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "libpandafile/class_data_accessor-inl.h"
#ifdef NEW_INSTRUCTION_DEFINE
#include "libpandafile/index_accessor.h"
#endif


namespace panda {
namespace ecmascript {
class JSThread;
class NativeAreaAllocator;

class Program : public ECMAObject {
public:
    DECL_CAST(Program)

    static constexpr size_t MAIN_FUNCTION_OFFSET = ECMAObject::SIZE;
    ACCESSORS(MainFunction, MAIN_FUNCTION_OFFSET, SIZE)

    DECL_VISIT_OBJECT(MAIN_FUNCTION_OFFSET, SIZE)
    DECL_DUMP()
};

class ConstantPool : public TaggedArray {
public:
    static ConstantPool *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<ConstantPool *>(object);
    }

    JSPandaFile *GetJSPandaFile() const
    {
        auto size = GetLength();
        JSTaggedValue fileValue = GetObjectFromCache(size - 1);
        if (!fileValue.IsJSNativePointer()) {
            return nullptr;
        }

        void *nativePointer = JSNativePointer::Cast(fileValue.GetTaggedObject())->GetExternalPointer();
        return static_cast<JSPandaFile *>(nativePointer);
    }

#ifdef NEW_INSTRUCTION_DEFINE
    static JSHandle<ConstantPool> CreateConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile, uint32_t methodId)
    {
        const panda_file::File::IndexHeader *mainIndex =
            jsPandaFile->GetPandaFile()->GetIndexHeader(panda_file::File::EntityId(methodId));
        LOG_ECMA_IF(mainIndex == nullptr, FATAL) << "Unknown methodId: " << methodId;

        ObjectFactory *factory = vm->GetFactory();
        JSHandle<ConstantPool> constpool = factory->NewConstantPool(mainIndex->method_idx_size + 2);

        // Put JSPandaFile at the last index of constpool.
        JSHandle<JSNativePointer> jsPandaFilePointer = factory->NewJSNativePointer(
            const_cast<JSPandaFile *>(jsPandaFile), JSPandaFileManager::RemoveJSPandaFile,
            JSPandaFileManager::GetInstance(), true);
        constpool->Set(vm->GetJSThread(), mainIndex->method_idx_size + 1, jsPandaFilePointer.GetTaggedValue());

        // Put IndexHeader before JSPandaFile.
        JSHandle<JSNativePointer> indexHeaderPointer =
            factory->NewJSNativePointer(const_cast<panda_file::File::IndexHeader *>(mainIndex));
        constpool->Set(vm->GetJSThread(), mainIndex->method_idx_size, indexHeaderPointer.GetTaggedValue());

        return constpool;
    }

    panda_file::File::IndexHeader *GetIndexHeader() const
    {
        auto size = GetLength();
        JSTaggedValue fileValue = GetObjectFromCache(size - 2);
        if (!fileValue.IsJSNativePointer()) {
            return nullptr;
        }

        void *nativePointer = JSNativePointer::Cast(fileValue.GetTaggedObject())->GetExternalPointer();
        return static_cast<panda_file::File::IndexHeader *>(nativePointer);
    }
#endif

    inline JSTaggedValue GetObjectFromCache(uint32_t index) const
    {
        return Get(index);
    }

    static JSTaggedValue GetMethodFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        auto val = taggedPool->Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();

            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = taggedPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];

            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(id.GetOffset());
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            panda_file::IndexAccessor newIndexAccessor(*pf, id);
            auto constpoolIndex = newIndexAccessor.GetHeaderIndex();
            JSTaggedValue newConstpool = vm->FindConstpool(jsPandaFile, constpoolIndex);
            if (newConstpool.IsHole()) {
                newConstpool = ConstantPool::CreateConstPool(vm, jsPandaFile, id.GetOffset()).GetTaggedValue();
                vm->AddConstpool(jsPandaFile, newConstpool, constpoolIndex);
            }
            method->SetConstantPool(thread, newConstpool);

            val = method.GetTaggedValue();
            constpoolHandle->Set(thread, index, val);
#endif
        }

        return val;
    }

    static JSTaggedValue GetClassMethodFromCache(JSThread *thread, JSHandle<ConstantPool> constpool,
                                                 uint32_t index)
    {
        auto val = constpool->Get(index);
        if (val.IsHole()) {
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();

            JSPandaFile *jsPandaFile = constpool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = constpool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];

            MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(id.GetOffset());
            ASSERT(methodLiteral != nullptr);
            JSHandle<Method> method = factory->NewMethod(methodLiteral);

            panda_file::IndexAccessor newIndexAccessor(*pf, id);
            auto constpoolIndex = newIndexAccessor.GetHeaderIndex();
            JSTaggedValue newConstpool = vm->FindConstpool(jsPandaFile, constpoolIndex);
            if (newConstpool.IsHole()) {
                newConstpool = ConstantPool::CreateConstPool(vm, jsPandaFile, id.GetOffset()).GetTaggedValue();
                vm->AddConstpool(jsPandaFile, newConstpool, constpoolIndex);
            }
            method->SetConstantPool(thread, newConstpool);
        
            val = method.GetTaggedValue();
            constpool->Set(thread, index, val);
            return val;
#else
        (void) thread;
        (void) constpool;
#endif
        }

        return val;
    }

    static JSTaggedValue GetClassLiteralFromCache(JSThread *thread, JSHandle<ConstantPool> constpool,
                                                  uint32_t literal)
    {
        auto val = constpool->Get(literal);
        if (val.IsHole()) {
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();

            JSPandaFile *jsPandaFile = constpool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = constpool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId literalId = indexs[literal];

            panda_file::IndexAccessor newIndexAccessor(*pf, literalId);
            auto constpoolIndex = newIndexAccessor.GetHeaderIndex();
            JSHandle<ConstantPool> newConstpoolHandle;
            JSTaggedValue newConstpool = vm->FindConstpool(jsPandaFile, constpoolIndex);
            if (newConstpool.IsHole()) {
                newConstpoolHandle = ConstantPool::CreateConstPool(vm, jsPandaFile, literalId.GetOffset());
                vm->AddConstpool(jsPandaFile, newConstpoolHandle.GetTaggedValue(), constpoolIndex);
            } else {
                newConstpoolHandle = JSHandle<ConstantPool>(thread, newConstpool);
            }
            JSHandle<TaggedArray> literalArray = LiteralDataExtractor::GetDatasIgnoreType(
                thread, jsPandaFile, literalId, JSHandle<JSTaggedValue>(newConstpoolHandle));

            val = literalArray.GetTaggedValue();
            constpool->Set(thread, literal, val);
            return val;
#else
        (void) thread;
        (void) constpool;
#endif
        }

        return val;
    }

    template <ConstPoolType type>
    static JSTaggedValue GetLiteralFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index)
    {
        static_assert(type == ConstPoolType::OBJECT_LITERAL || type == ConstPoolType::ARRAY_LITERAL);
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());

        auto val = taggedPool->Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = taggedPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];

            // New inst
            switch (type)
            {
                case ConstPoolType::OBJECT_LITERAL: {
                    JSMutableHandle<TaggedArray> elements(thread, JSTaggedValue::Undefined());
                    JSMutableHandle<TaggedArray> properties(thread, JSTaggedValue::Undefined());
                    LiteralDataExtractor::ExtractObjectDatas(
                        thread, jsPandaFile, id, elements, properties, JSHandle<JSTaggedValue>(constpoolHandle));
                    JSHandle<JSObject> obj = JSObject::CreateObjectFromProperties(thread, properties);
                    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
                    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
                    size_t elementsLen = elements->GetLength();
                    for (size_t i = 0; i < elementsLen; i += 2) {  // 2: Each literal buffer contains a pair of key-value.
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
                    JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(
                        thread, jsPandaFile, id, JSHandle<JSTaggedValue>(constpoolHandle));

                    uint32_t length = literal->GetLength();

                    JSHandle<JSArray> arr(JSArray::ArrayCreate(thread, JSTaggedNumber(length)));
                    arr->SetElements(thread, literal);
                    val = arr.GetTaggedValue();
                    break;
                }
                default:
                    LOG_FULL(FATAL) << "Unknown type: " << static_cast<uint8_t>(type);
                    UNREACHABLE();
            }
            constpoolHandle->Set(thread, index, val);
#endif
        }

        return val;
    }

    static JSTaggedValue GetStringFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index)
    {
        const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
        auto val = taggedPool->Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();

            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = taggedPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];

            auto foundStr = pf->GetStringData(id);

            auto string = factory->GetRawStringFromStringTable(
                foundStr.data, foundStr.utf16_length, foundStr.is_ascii, MemSpaceType::OLD_SPACE);
            val = JSTaggedValue(string);

            constpoolHandle->Set(thread, index, val);
#endif
        }

        return val;
    }

    std::string PUBLIC_API GetStdStringByIdx(size_t index) const;

    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
