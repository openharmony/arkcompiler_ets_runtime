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
    static JSTaggedValue CreateConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile, uint32_t methodId)
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

        return constpool.GetTaggedValue();
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

    inline JSTaggedValue GetObjectFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index) const
    {
        auto val = Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            // New inst of Function
            EcmaVM *vm = thread->GetEcmaVM();
            JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
            ObjectFactory *factory = vm->GetFactory();

            JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
            JSHandle<JSHClass> normalClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
            JSHandle<JSHClass> asyncClass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
            JSHandle<JSHClass> generatorClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
            JSHandle<JSHClass> asyncGeneratorClass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClass());

            const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = taggedPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];

            MethodLiteral *methodLitera = jsPandaFile->FindMethods(id.GetOffset());
            LOG_ECMA_IF(methodLitera == nullptr, FATAL) << "Unknown method: " << id.GetOffset();
            JSHandle<Method> method = factory->NewJSMethod(methodLitera);

            panda_file::IndexAccessor newIndexAccessor(*pf, id);
            auto constpoolIndex = newIndexAccessor.GetHeaderIndex();
            JSTaggedValue newConstpool = vm->FindConstpool(jsPandaFile, constpoolIndex);
            if (newConstpool.IsHole()) {
                newConstpool = ConstantPool::CreateConstPool(vm, jsPandaFile, id.GetOffset());
                vm->AddConstpool(jsPandaFile, newConstpool, constpoolIndex);
            }
            method->SetConstantPool(thread, newConstpool);

            JSHandle<JSFunction> jsFunc;
            FunctionKind kind = methodLitera->GetFunctionKind();
            switch (kind)
            {
                case FunctionKind::NORMAL_FUNCTION:
                    jsFunc = factory->NewJSFunctionByHClass(method, hclass, kind, MemSpaceType::OLD_SPACE);
                    break;
                case FunctionKind::ARROW_FUNCTION:
                    jsFunc = factory->NewJSFunctionByHClass(method, normalClass, kind, MemSpaceType::OLD_SPACE);
                    break;
                case FunctionKind::GENERATOR_FUNCTION:
                    jsFunc = factory->NewJSFunctionByHClass(method, generatorClass, kind, MemSpaceType::OLD_SPACE);
                    break;
                case FunctionKind::ASYNC_FUNCTION:
                    jsFunc = factory->NewJSFunctionByHClass(method, asyncClass, kind, MemSpaceType::OLD_SPACE);
                    break;
                case FunctionKind::ASYNC_GENERATOR_FUNCTION:
                    jsFunc = factory->NewJSFunctionByHClass(method, asyncGeneratorClass,
                                                            kind, MemSpaceType::OLD_SPACE);
                    break;
                case FunctionKind::ASYNC_ARROW_FUNCTION:
                    // Add hclass for async arrow function
                    jsFunc = factory->NewJSFunctionByHClass(method, asyncClass,
                                                            kind, MemSpaceType::OLD_SPACE);
                    break;
                default:
                    UNREACHABLE();
            }
            val = jsFunc.GetTaggedValue();
            constpoolHandle->Set(thread, index, val);
#endif
        }

        return val;
    }

    inline JSTaggedValue GetClassFromCache(JSThread *thread, JSTaggedValue constpool,
                                           uint32_t index, uint32_t literal) const
    {
        auto val = Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();

            const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = taggedPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];
            panda_file::File::EntityId literalId = indexs[literal];

            panda_file::IndexAccessor newIndexAccessor(*pf, id);
            auto constpoolIndex = newIndexAccessor.GetHeaderIndex();
            JSTaggedValue newConstpool = vm->FindConstpool(jsPandaFile, constpoolIndex);
            if (newConstpool.IsHole()) {
                newConstpool = ConstantPool::CreateConstPool(vm, jsPandaFile, id.GetOffset());
                vm->AddConstpool(jsPandaFile, newConstpool, constpoolIndex);
            }
            JSHandle<JSTaggedValue> newConstpoolHandle(thread, newConstpool);

            JSHandle<TaggedArray> literalArray = LiteralDataExtractor::GetDatasIgnoreType(
                thread, jsPandaFile, literalId, newConstpoolHandle);

            MethodLiteral *methodLitera = jsPandaFile->FindMethods(id.GetOffset());
            ASSERT(methodLitera != nullptr);
            JSHandle<ClassInfoExtractor> classInfoExtractor = factory->NewClassInfoExtractor(methodLitera);
            ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(thread, classInfoExtractor,
                                                                   literalArray, jsPandaFile);
            JSHandle<JSFunction> cls = ClassHelper::DefineClassTemplate(thread, classInfoExtractor,
                                                                        JSHandle<ConstantPool>(newConstpoolHandle));
            val = cls.GetTaggedValue();
            constpoolHandle->Set(thread, index, val);
#else
            (void) literal;
#endif
        }

        return val;
    }

    template <ConstPoolType type>
    inline JSTaggedValue GetLiteralFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index) const
    {
        static_assert(type == ConstPoolType::OBJECT_LITERAL || type == ConstPoolType::ARRAY_LITERAL);

        auto val = Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE

            const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
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

    inline JSTaggedValue GetMethodFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index) const
    {
        auto val = Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();
            JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
            ObjectFactory *factory = vm->GetFactory();

            JSHandle<JSHClass> normalClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());

            const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
            JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();
            panda_file::File::IndexHeader *indexHeader = taggedPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId id = indexs[index];

            MethodLiteral *methodLitera = jsPandaFile->FindMethods(id.GetOffset());
            ASSERT(methodLitera != nullptr);

            JSHandle<Method> method = factory->NewJSMethod(methodLitera);
            panda_file::IndexAccessor newIndexAccessor(*pf, id);
            auto constpoolIndex = newIndexAccessor.GetHeaderIndex();
            JSTaggedValue newConstpool = vm->FindConstpool(jsPandaFile, constpoolIndex);
            if (newConstpool.IsHole()) {
                newConstpool = ConstantPool::CreateConstPool(vm, jsPandaFile, id.GetOffset());
                vm->AddConstpool(jsPandaFile, newConstpool, constpoolIndex);
            }
            method->SetConstantPool(thread, newConstpool);

            JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByHClass(
                method, normalClass, FunctionKind::NORMAL_FUNCTION, MemSpaceType::OLD_SPACE);
            val = jsFunc.GetTaggedValue();

            constpoolHandle->Set(thread, index, val);
#endif
        }

        return val;
    }

    inline JSTaggedValue GetStringFromCache(JSThread *thread, JSTaggedValue constpool, uint32_t index) const
    {
        auto val = Get(index);
        if (val.IsHole()) {
            JSHandle<ConstantPool> constpoolHandle(thread, constpool);
#ifdef NEW_INSTRUCTION_DEFINE
            EcmaVM *vm = thread->GetEcmaVM();
            ObjectFactory *factory = vm->GetFactory();

            const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
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
