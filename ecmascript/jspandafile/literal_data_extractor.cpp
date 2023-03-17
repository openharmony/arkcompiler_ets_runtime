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

#include "ecmascript/jspandafile/literal_data_extractor.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
using LiteralTag = panda_file::LiteralTag;
using StringData = panda_file::StringData;
using LiteralValue = panda_file::LiteralDataAccessor::LiteralValue;

void LiteralDataExtractor::ExtractObjectDatas(JSThread *thread, const JSPandaFile *jsPandaFile, size_t index,
                                              JSMutableHandle<TaggedArray> elements,
                                              JSMutableHandle<TaggedArray> properties,
                                              JSHandle<ConstantPool> constpool,
                                              const CString &entryPoint)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    LOG_ECMA(VERBOSE) << "Panda File" << jsPandaFile->GetJSPandaFileDesc();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    elements.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    properties.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    uint32_t epos = 0;
    uint32_t ppos = 0;
    const uint8_t pairSize = 2;
    uint32_t methodId;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        index, [elements, properties, &epos, &ppos, vm, factory, thread, jsPandaFile, pf,
        &methodId, &kind, &constpool, &entryPoint]
        (const LiteralValue &value, const LiteralTag &tag) {
        JSTaggedValue jt = JSTaggedValue::Null();
        bool flag = false;
        switch (tag) {
            case LiteralTag::INTEGER: {
                jt = JSTaggedValue(std::get<uint32_t>(value));
                break;
            }
            case LiteralTag::DOUBLE: {
                jt = JSTaggedValue(std::get<double>(value));
                break;
            }
            case LiteralTag::BOOL: {
                jt = JSTaggedValue(std::get<bool>(value));
                break;
            }
            case LiteralTag::STRING: {
                StringData sd = pf->GetStringData(panda_file::File::EntityId(std::get<uint32_t>(value)));
                EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii,
                                                                       MemSpaceType::OLD_SPACE);
                jt = JSTaggedValue(str);
                uint32_t index = 0;
                if (JSTaggedValue::ToElementIndex(jt, &index) && ppos % pairSize == 0) {
                    flag = true;
                }
                break;
            }
            case LiteralTag::METHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::NORMAL_FUNCTION;
                break;
            }
            case LiteralTag::GENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GENERATOR_FUNCTION;
                break;
            }
            case LiteralTag::METHODAFFILIATE: {
                uint16_t length = std::get<uint16_t>(value);
                auto methodLiteral = jsPandaFile->FindMethodLiteral(methodId);
                ASSERT(methodLiteral != nullptr);
                methodLiteral->SetFunctionKind(kind);

                JSHandle<Method> method = factory->NewMethod(methodLiteral);
                if (jsPandaFile->IsNewVersion()) {
                    JSHandle<ConstantPool> newConstpool =
                        vm->FindOrCreateConstPool(jsPandaFile, panda_file::File::EntityId(methodId));
                    method->SetConstantPool(thread, newConstpool);
                } else {
                    method->SetConstantPool(thread, constpool.GetTaggedValue());
                }
                JSHandle<JSFunction> jsFunc =
                    DefineMethodInLiteral(thread, jsPandaFile, method, kind, length, entryPoint);
                jt = jsFunc.GetTaggedValue();
                break;
            }
            case LiteralTag::ACCESSOR: {
                JSHandle<AccessorData> accessor = factory->NewAccessorData();
                jt = JSTaggedValue(accessor.GetTaggedValue());
                break;
            }
            case LiteralTag::NULLVALUE: {
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
        if (tag != LiteralTag::METHOD && tag != LiteralTag::GENERATORMETHOD) {
            if (epos % pairSize == 0 && !flag) {
                properties->Set(thread, ppos++, jt);
            } else {
                elements->Set(thread, epos++, jt);
            }
        }
    });
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreTypeForClass(JSThread *thread,
    const JSPandaFile *jsPandaFile, size_t index, JSHandle<ConstantPool> constpool, const CString &entryPoint)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);
    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    // The num is 1, indicating that the current class has no member variable.
    if (num == 1) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->EmptyArray();
    }
    return EnumerateLiteralVals(thread, lda, jsPandaFile, index, constpool, entryPoint);
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreType(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                               size_t index, JSHandle<ConstantPool> constpool,
                                                               const CString &entryPoint)
{
    LOG_ECMA(VERBOSE) << "Panda File" << jsPandaFile->GetJSPandaFileDesc();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);
    return EnumerateLiteralVals(thread, lda, jsPandaFile, index, constpool, entryPoint);
}

JSHandle<TaggedArray> LiteralDataExtractor::EnumerateLiteralVals(JSThread *thread, panda_file::LiteralDataAccessor &lda,
    const JSPandaFile *jsPandaFile, size_t index, JSHandle<ConstantPool> constpool, const CString &entryPoint)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    JSHandle<TaggedArray> literals = factory->NewOldSpaceTaggedArray(num);
    uint32_t pos = 0;
    uint32_t methodId;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        index, [literals, &pos, vm, factory, thread, jsPandaFile, &methodId, &kind, &constpool, &entryPoint]
        (const panda_file::LiteralDataAccessor::LiteralValue &value, const LiteralTag &tag) {
            JSTaggedValue jt = JSTaggedValue::Null();
            switch (tag) {
                case LiteralTag::INTEGER: {
                    jt = JSTaggedValue(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::DOUBLE: {
                    jt = JSTaggedValue(std::get<double>(value));
                    break;
                }
                case LiteralTag::BOOL: {
                    jt = JSTaggedValue(std::get<bool>(value));
                    break;
                }
                case LiteralTag::STRING: {
                    const panda_file::File *pf = jsPandaFile->GetPandaFile();
                    StringData sd = pf->GetStringData(panda_file::File::EntityId(std::get<uint32_t>(value)));
                    EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii,
                                                                           MemSpaceType::OLD_SPACE);
                    jt = JSTaggedValue(str);
                    break;
                }
                case LiteralTag::METHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::NORMAL_FUNCTION;
                    break;
                }
                case LiteralTag::GENERATORMETHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::GENERATOR_FUNCTION;
                    break;
                }
                case LiteralTag::METHODAFFILIATE: {
                    uint16_t length = std::get<uint16_t>(value);
                    auto methodLiteral = jsPandaFile->FindMethodLiteral(methodId);
                    ASSERT(methodLiteral != nullptr);

                    JSHandle<Method> method = factory->NewMethod(methodLiteral);
                    if (jsPandaFile->IsNewVersion()) {
                        JSHandle<ConstantPool> newConstpool =
                            vm->FindOrCreateConstPool(jsPandaFile, panda_file::File::EntityId(methodId));
                        method->SetConstantPool(thread, newConstpool);
                    } else {
                        method->SetConstantPool(thread, constpool);
                    }
                    JSHandle<JSFunction> jsFunc =
                        DefineMethodInLiteral(thread, jsPandaFile, method, kind, length, entryPoint);
                    jt = jsFunc.GetTaggedValue();
                    break;
                }
                case LiteralTag::ACCESSOR: {
                    JSHandle<AccessorData> accessor = factory->NewAccessorData();
                    jt = accessor.GetTaggedValue();
                    break;
                }
                case LiteralTag::NULLVALUE: {
                    break;
                }
                default: {
                    UNREACHABLE();
                    break;
                }
            }
            if (tag != LiteralTag::METHOD && tag != LiteralTag::GENERATORMETHOD) {
                literals->Set(thread, pos++, jt);
            } else {
                uint32_t oldLength = literals->GetLength();
                literals->Trim(thread, oldLength - 1);
            }
        });
    return literals;
}

JSHandle<JSFunction> LiteralDataExtractor::DefineMethodInLiteral(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                                 JSHandle<Method> method,
                                                                 FunctionKind kind, uint16_t length,
                                                                 const CString &entryPoint)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSHClass> functionClass;
    if (kind == FunctionKind::NORMAL_FUNCTION) {
        functionClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    } else {
        functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    }
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByHClass(method, functionClass, MemSpaceType::OLD_SPACE);

    jsFunc->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    CString moduleName = jsPandaFile->GetJSPandaFileDesc();
    CString entry = JSPandaFile::ENTRY_FUNCTION_NAME;
    if (!entryPoint.empty()) {
        moduleName = entryPoint;
        entry = entryPoint;
    }
    if (jsPandaFile->IsModule(thread, entry)) {
        EcmaVM *vm = thread->GetEcmaVM();
        JSHandle<SourceTextModule> module = vm->GetModuleManager()->HostGetImportedModule(moduleName);
        jsFunc->SetModule(thread, module.GetTaggedValue());
    }
    return jsFunc;
}

void LiteralDataExtractor::GetMethodOffsets(const JSPandaFile *jsPandaFile, size_t index,
                                            std::vector<uint32_t> &methodOffsets)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    lda.EnumerateLiteralVals(
        index, [&methodOffsets]
        (const panda_file::LiteralDataAccessor::LiteralValue &value, const LiteralTag &tag) {
            switch (tag) {
                case LiteralTag::METHOD:
                case LiteralTag::GENERATORMETHOD: {
                    methodOffsets.emplace_back(std::get<uint32_t>(value));
                    break;
                }
                default: {
                    break;
                }
            }
        });
}

void LiteralDataExtractor::GetMethodOffsets(const JSPandaFile *jsPandaFile, panda_file::File::EntityId index,
                                            std::vector<uint32_t> &methodOffsets)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    lda.EnumerateLiteralVals(
        index, [&methodOffsets]
        (const panda_file::LiteralDataAccessor::LiteralValue &value, const LiteralTag &tag) {
            switch (tag) {
                case LiteralTag::METHOD:
                case LiteralTag::GENERATORMETHOD: {
                    methodOffsets.emplace_back(std::get<uint32_t>(value));
                    break;
                }
                default: {
                    break;
                }
            }
        });
}

void LiteralDataExtractor::ExtractObjectDatas(JSThread *thread, const JSPandaFile *jsPandaFile,
                                              panda_file::File::EntityId index,
                                              JSMutableHandle<TaggedArray> elements,
                                              JSMutableHandle<TaggedArray> properties,
                                              JSHandle<ConstantPool> constpool, const CString &entry)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    LOG_ECMA(VERBOSE) << "Panda File" << jsPandaFile->GetJSPandaFileDesc();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    elements.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    properties.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    uint32_t epos = 0;
    uint32_t ppos = 0;
    const uint8_t pairSize = 2;
    uint32_t methodId;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        index, [elements, properties, &epos, &ppos, vm, factory, thread, jsPandaFile,
                pf, &methodId, &kind, &constpool, &entry]
        (const LiteralValue &value, const LiteralTag &tag) {
        JSTaggedValue jt = JSTaggedValue::Null();
        bool flag = false;
        switch (tag) {
            case LiteralTag::INTEGER: {
                jt = JSTaggedValue(std::get<uint32_t>(value));
                break;
            }
            case LiteralTag::DOUBLE: {
                jt = JSTaggedValue(std::get<double>(value));
                break;
            }
            case LiteralTag::BOOL: {
                jt = JSTaggedValue(std::get<bool>(value));
                break;
            }
            case LiteralTag::STRING: {
                StringData sd = pf->GetStringData(panda_file::File::EntityId(std::get<uint32_t>(value)));
                EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii,
                                                                       MemSpaceType::OLD_SPACE);
                jt = JSTaggedValue(str);
                uint32_t index = 0;
                if (JSTaggedValue::ToElementIndex(jt, &index) && ppos % pairSize == 0) {
                    flag = true;
                }
                break;
            }
            case LiteralTag::METHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::NORMAL_FUNCTION;
                break;
            }
            case LiteralTag::GENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GENERATOR_FUNCTION;
                break;
            }
            case LiteralTag::METHODAFFILIATE: {
                uint16_t length = std::get<uint16_t>(value);
                auto methodLiteral = jsPandaFile->FindMethodLiteral(methodId);
                ASSERT(methodLiteral != nullptr);
                // Should replace with ASSERT(kind == methodLiteral->GetFunctionKind())
                methodLiteral->SetFunctionKind(kind);

                JSHandle<Method> method = factory->NewMethod(methodLiteral);
                if (jsPandaFile->IsNewVersion()) {
                    JSHandle<ConstantPool> newConstpool =
                        vm->FindOrCreateConstPool(jsPandaFile, panda_file::File::EntityId(methodId));
                    method->SetConstantPool(thread, newConstpool);
                } else {
                    method->SetConstantPool(thread, constpool.GetTaggedValue());
                }
                JSHandle<JSFunction> jsFunc = DefineMethodInLiteral(thread, jsPandaFile, method, kind, length, entry);
                jt = jsFunc.GetTaggedValue();
                break;
            }
            case LiteralTag::ACCESSOR: {
                JSHandle<AccessorData> accessor = factory->NewAccessorData();
                jt = JSTaggedValue(accessor.GetTaggedValue());
                break;
            }
            case LiteralTag::NULLVALUE: {
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
        if (tag != LiteralTag::METHOD && tag != LiteralTag::GENERATORMETHOD) {
            if (epos % pairSize == 0 && !flag) {
                properties->Set(thread, ppos++, jt);
            } else {
                elements->Set(thread, epos++, jt);
            }
        }
    });
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreType(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                               panda_file::File::EntityId index,
                                                               JSHandle<ConstantPool> constpool,
                                                               const CString &entryPoint)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    LOG_ECMA(VERBOSE) << "Panda File" << jsPandaFile->GetJSPandaFileDesc();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    JSHandle<TaggedArray> literals = JSHandle<TaggedArray>(factory->NewCOWTaggedArray(num));
    uint32_t pos = 0;
    uint32_t methodId;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        index, [literals, &pos, vm, factory, thread, jsPandaFile, &methodId, &kind, &constpool, &entryPoint]
        (const panda_file::LiteralDataAccessor::LiteralValue &value, const LiteralTag &tag) {
            JSTaggedValue jt = JSTaggedValue::Null();
            switch (tag) {
                case LiteralTag::INTEGER: {
                    jt = JSTaggedValue(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::DOUBLE: {
                    jt = JSTaggedValue(std::get<double>(value));
                    break;
                }
                case LiteralTag::BOOL: {
                    jt = JSTaggedValue(std::get<bool>(value));
                    break;
                }
                case LiteralTag::STRING: {
                    const panda_file::File *pf = jsPandaFile->GetPandaFile();
                    StringData sd = pf->GetStringData(panda_file::File::EntityId(std::get<uint32_t>(value)));
                    EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii,
                                                                           MemSpaceType::OLD_SPACE);
                    jt = JSTaggedValue(str);
                    break;
                }
                case LiteralTag::METHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::NORMAL_FUNCTION;
                    break;
                }
                case LiteralTag::GENERATORMETHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::GENERATOR_FUNCTION;
                    break;
                }
                case LiteralTag::METHODAFFILIATE: {
                    uint16_t length = std::get<uint16_t>(value);
                    auto methodLiteral = jsPandaFile->FindMethodLiteral(methodId);
                    ASSERT(methodLiteral != nullptr);

                    JSHandle<Method> method = factory->NewMethod(methodLiteral);
                    if (jsPandaFile->IsNewVersion()) {
                        JSHandle<ConstantPool> newConstpool =
                            vm->FindOrCreateConstPool(jsPandaFile, panda_file::File::EntityId(methodId));
                        method->SetConstantPool(thread, newConstpool);
                    } else {
                        method->SetConstantPool(thread, constpool.GetTaggedValue());
                    }
                    JSHandle<JSFunction> jsFunc =
                        DefineMethodInLiteral(thread, jsPandaFile, method, kind, length, entryPoint);
                    jt = jsFunc.GetTaggedValue();
                    break;
                }
                case LiteralTag::ACCESSOR: {
                    JSHandle<AccessorData> accessor = factory->NewAccessorData();
                    jt = accessor.GetTaggedValue();
                    break;
                }
                case LiteralTag::NULLVALUE: {
                    break;
                }
                default: {
                    UNREACHABLE();
                    break;
                }
            }
            if (tag != LiteralTag::METHOD && tag != LiteralTag::GENERATORMETHOD) {
                literals->Set(thread, pos++, jt);
            } else {
                uint32_t oldLength = literals->GetLength();
                literals->Trim(thread, oldLength - 1);
            }
        });
    return literals;
}

// use for parsing specific literal which record TS type info
JSHandle<TaggedArray> LiteralDataExtractor::GetTypeLiteral(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                           panda_file::File::EntityId offset)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t num = lda.GetLiteralValsNum(offset) / 2;  // 2: half
    JSHandle<TaggedArray> literals = factory->NewOldSpaceTaggedArray(num);
    uint32_t pos = 0;
    lda.EnumerateLiteralVals(
        offset, [literals, &pos, factory, thread, pf]
        (const panda_file::LiteralDataAccessor::LiteralValue &value, const LiteralTag &tag) {
            JSTaggedValue jt = JSTaggedValue::Null();
            switch (tag) {
                case LiteralTag::INTEGER: {
                    jt = JSTaggedValue(bit_cast<int32_t>(std::get<uint32_t>(value)));
                    break;
                }
                case LiteralTag::LITERALARRAY: {
                    ASSERT(std::get<uint32_t>(value) > LITERALARRAY_VALUE_LOW_BOUNDARY);
                    jt = JSTaggedValue(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::BUILTINTYPEINDEX: {
                    jt = JSTaggedValue(std::get<uint8_t>(value));
                    break;
                }
                case LiteralTag::STRING: {
                    StringData sd = pf->GetStringData(panda_file::File::EntityId(std::get<uint32_t>(value)));
                    EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii,
                                                                           MemSpaceType::OLD_SPACE);
                    jt = JSTaggedValue(str);
                    break;
                }
                default: {
                    LOG_FULL(FATAL) << "type literal should not exist LiteralTag: " << static_cast<uint8_t>(tag);
                    break;
                }
            }
            literals->Set(thread, pos++, jt);
        });
    return literals;
}
}  // namespace panda::ecmascript
