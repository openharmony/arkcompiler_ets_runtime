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

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/global_env.h"
#include "ecmascript/module/js_shared_module_manager.h"

namespace panda::ecmascript {
using LiteralTag = panda_file::LiteralTag;
using StringData = panda_file::StringData;
using LiteralDataAccessor = panda_file::LiteralDataAccessor;
using LiteralValue = panda_file::LiteralDataAccessor::LiteralValue;

void LiteralDataExtractor::ExtractObjectDatas(JSThread *thread, const JSPandaFile *jsPandaFile, size_t index,
                                              JSMutableHandle<TaggedArray> elements,
                                              JSMutableHandle<TaggedArray> properties,
                                              JSHandle<ConstantPool> constpool,
                                              const CString &entryPoint)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    elements.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    properties.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    uint32_t epos = 0;
    uint32_t ppos = 0;
    const uint8_t pairSize = 2;
    uint32_t methodId = 0;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        index, [elements, properties, &epos, &ppos, factory, thread, jsPandaFile,
                &methodId, &kind, &constpool, &entryPoint](const LiteralValue &value, const LiteralTag &tag) {
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
                StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::SHARED_OLD_SPACE);
                jt = JSTaggedValue(str);
                uint32_t elementIndex = 0;
                if (JSTaggedValue::ToElementIndex(jt, &elementIndex) && ppos % pairSize == 0) {
                    flag = true;
                }
                break;
            }
            case LiteralTag::METHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::NORMAL_FUNCTION;
                break;
            }
            case LiteralTag::GETTER: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GETTER_FUNCTION;
                break;
            }
            case LiteralTag::SETTER: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::SETTER_FUNCTION;
                break;
            }
            case LiteralTag::GENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GENERATOR_FUNCTION;
                break;
            }
            case LiteralTag::ASYNCGENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
                break;
            }
            case LiteralTag::METHODAFFILIATE: {
                uint16_t length = std::get<uint16_t>(value);
                JSHandle<JSFunction> jsFunc =
                    DefineMethodInLiteral(thread, jsPandaFile, methodId, constpool, kind, length, entryPoint);
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
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                break;
            }
        }
        if (tag != LiteralTag::METHOD && tag != LiteralTag::GETTER && tag != LiteralTag::SETTER &&
            tag != LiteralTag::GENERATORMETHOD && tag != LiteralTag::ASYNCGENERATORMETHOD) {
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
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
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
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    return EnumerateLiteralVals(thread, lda, jsPandaFile, index, constpool, entryPoint);
}

JSHandle<TaggedArray> LiteralDataExtractor::EnumerateLiteralVals(JSThread *thread, LiteralDataAccessor &lda,
    const JSPandaFile *jsPandaFile, size_t index, JSHandle<ConstantPool> constpool, const CString &entryPoint)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    JSHandle<TaggedArray> literals = factory->NewOldSpaceTaggedArray(num);
    uint32_t pos = 0;
    uint32_t methodId = 0;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        index, [literals, &pos, factory, thread, jsPandaFile, &methodId, &kind, &constpool, &entryPoint]
        (const LiteralValue &value, const LiteralTag &tag) {
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
                    StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                    EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::SHARED_OLD_SPACE);
                    jt = JSTaggedValue(str);
                    break;
                }
                case LiteralTag::METHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::NORMAL_FUNCTION;
                    break;
                }
                case LiteralTag::GETTER: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::GETTER_FUNCTION;
                    break;
                }
                case LiteralTag::SETTER: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::SETTER_FUNCTION;
                    break;
                }
                case LiteralTag::GENERATORMETHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::GENERATOR_FUNCTION;
                    break;
                }
                case LiteralTag::ASYNCGENERATORMETHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
                    break;
                }
                case LiteralTag::METHODAFFILIATE: {
                    uint16_t length = std::get<uint16_t>(value);
                    JSHandle<FunctionTemplate> jsFunc =
                        DefineFunctionTemplate(thread, jsPandaFile, methodId, constpool, kind, length, entryPoint);
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
                    LOG_ECMA(FATAL) << "this branch is unreachable";
                    UNREACHABLE();
                    break;
                }
            }
            if (tag != LiteralTag::METHOD && tag != LiteralTag::GETTER && tag != LiteralTag::SETTER &&
                tag != LiteralTag::GENERATORMETHOD && tag != LiteralTag::ASYNCGENERATORMETHOD) {
                literals->Set(thread, pos++, jt);
            } else {
                uint32_t oldLength = literals->GetLength();
                ASSERT(oldLength > 0);
                literals->Trim(thread, oldLength - 1);
            }
        });
    return literals;
}

JSHandle<FunctionTemplate> LiteralDataExtractor::DefineFunctionTemplate(JSThread *thread,
                                                                        const JSPandaFile *jsPandaFile,
                                                                        uint32_t offset,
                                                                        JSHandle<ConstantPool> constpool,
                                                                        FunctionKind kind, uint16_t length,
                                                                        const CString &entryPoint,
                                                                        bool isLoadedAOT, uint32_t entryIndex,
                                                                        JSHandle<JSTaggedValue> sendableEnv,
                                                                        ClassKind classKind)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    // New Method
    auto methodLiteral = jsPandaFile->FindMethodLiteral(offset);
    CHECK_INPUT_NULLPTR(methodLiteral,
                        "DefineFunctionTemplate:methodLiteral is nullptr, offset: " + std::to_string(offset));
    FunctionKind literalKind = methodLiteral->GetFunctionKind();
    if (literalKind == FunctionKind::NONE_FUNCTION || classKind == ClassKind::SENDABLE) {
        methodLiteral->SetFunctionKind(kind);
    } else {
        kind = literalKind;
    }
    JSHandle<Method> method = factory->NewSMethod(jsPandaFile, methodLiteral, constpool, entryIndex, isLoadedAOT);
    if (classKind == ClassKind::SENDABLE) {
        method->SetIsSendable(true);
    }

    // Generate Module
    JSHandle<JSTaggedValue> module = SharedModuleManager::GetInstance()->GenerateFuncModule(thread, jsPandaFile,
                                                                                            entryPoint, classKind);

    // for classLiteral. The SENDABLE kind will generate new module.
    if (module->IsSourceTextModule() && classKind == ClassKind::SENDABLE) {
        SourceTextModule::Cast(module->GetTaggedObject())->SetSendableEnv(thread, sendableEnv);
    }

    // New FunctionTemplate
    JSHandle<FunctionTemplate> funcTemp;
    if (classKind == ClassKind::SENDABLE) {
        funcTemp = factory->NewSFunctionTemplate(method, module, length);
    } else {
        funcTemp = factory->NewFunctionTemplate(method, module, length);
    }
    return funcTemp;
}

JSHandle<JSFunction> LiteralDataExtractor::CreateJSFunctionInLiteral(EcmaVM *vm,
                                                                     const JSHandle<Method> &method,
                                                                     FunctionKind kind,
                                                                     ClassKind classKind)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> jsFunc;
    JSHandle<JSHClass> functionClass;
    if (classKind == ClassKind::SENDABLE) {
        if (kind == FunctionKind::NORMAL_FUNCTION ||
            kind == FunctionKind::GETTER_FUNCTION ||
            kind == FunctionKind::SETTER_FUNCTION) {
            functionClass = JSHandle<JSHClass>::Cast(env->GetSFunctionClassWithoutProto());
        } else if (kind == FunctionKind::ASYNC_FUNCTION) {
            functionClass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
        } else {
            functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
        }
        jsFunc = factory->NewSFunctionByHClass(method, functionClass);
    } else {
        if (kind == FunctionKind::NORMAL_FUNCTION ||
            kind == FunctionKind::GETTER_FUNCTION ||
            kind == FunctionKind::SETTER_FUNCTION) {
            functionClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
        } else if (kind == FunctionKind::ASYNC_FUNCTION) {
            functionClass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
        } else {
            functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
        }
        jsFunc = factory->NewJSFunctionByHClass(method, functionClass, MemSpaceType::OLD_SPACE);
    }
    return jsFunc;
}

JSHandle<JSFunction> LiteralDataExtractor::DefineMethodInLiteral(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                                 uint32_t offset, JSHandle<ConstantPool> constpool,
                                                                 FunctionKind kind, uint16_t length,
                                                                 const CString &entryPoint,
                                                                 bool isLoadedAOT, uint32_t entryIndex,
                                                                 JSHandle<JSTaggedValue> sendableEnv,
                                                                 ClassKind classKind)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    auto methodLiteral = jsPandaFile->FindMethodLiteral(offset);
    CHECK_INPUT_NULLPTR(methodLiteral,
                        "DefineMethodInLiteral:methodLiteral is nullptr, offset: " + std::to_string(offset));
    FunctionKind literalKind = methodLiteral->GetFunctionKind();
    if (literalKind == FunctionKind::NONE_FUNCTION || classKind == ClassKind::SENDABLE) {
        methodLiteral->SetFunctionKind(kind);
    } else {
        kind = literalKind;
    }
    bool canFastCall = false;
    JSHandle<Method> method =
        factory->NewSMethod(jsPandaFile, methodLiteral, constpool, entryIndex, isLoadedAOT, &canFastCall);
    if (classKind == ClassKind::SENDABLE) {
        method->SetIsSendable(true);
    }
    JSHandle<JSHClass> functionClass;
    JSHandle<JSFunction> jsFunc = CreateJSFunctionInLiteral(vm, method, kind, classKind);
    JSHandle<JSTaggedValue> module = SharedModuleManager::GetInstance()->GenerateFuncModule(thread, jsPandaFile,
                                                                                            entryPoint, classKind);

    // for objectLiteral and arrayLiteral. The NON_SENDABLE kind does not need to reset sendableEnv.
    if (module->IsSourceTextModule() && classKind == ClassKind::SENDABLE) {
        SourceTextModule::Cast(module->GetTaggedObject())->SetSendableEnv(thread, sendableEnv);
    }

    jsFunc->SetModule(thread, module.GetTaggedValue());
    jsFunc->SetLength(length);
    return jsFunc;
}

void LiteralDataExtractor::GetMethodOffsets(const JSPandaFile *jsPandaFile, size_t index,
                                            std::vector<uint32_t> &methodOffsets)
{
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    lda.EnumerateLiteralVals(index, [&methodOffsets](const LiteralValue &value, const LiteralTag &tag) {
        switch (tag) {
            case LiteralTag::METHOD:
            case LiteralTag::GETTER:
            case LiteralTag::SETTER:
            case LiteralTag::ASYNCGENERATORMETHOD:
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

void LiteralDataExtractor::GetMethodOffsets(const JSPandaFile *jsPandaFile, EntityId id,
                                            std::vector<uint32_t> &methodOffsets)
{
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    lda.EnumerateLiteralVals(id, [&methodOffsets](const LiteralValue &value, const LiteralTag &tag) {
        switch (tag) {
            case LiteralTag::METHOD:
            case LiteralTag::GETTER:
            case LiteralTag::SETTER:
            case LiteralTag::ASYNCGENERATORMETHOD:
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

void LiteralDataExtractor::ExtractObjectDatas(JSThread *thread, const JSPandaFile *jsPandaFile, EntityId id,
                                              JSMutableHandle<TaggedArray> elements,
                                              JSMutableHandle<TaggedArray> properties,
                                              JSHandle<ConstantPool> constpool, const CString &entry,
                                              bool isLoadedAOT, JSHandle<AOTLiteralInfo> entryIndexes)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    uint32_t num = lda.GetLiteralValsNum(id) / 2;  // 2: half
    elements.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    properties.Update(factory->NewOldSpaceTaggedArray(num).GetTaggedValue());
    uint32_t epos = 0;
    uint32_t ppos = 0;
    const uint8_t pairSize = 2;
    uint32_t methodId = 0;
    int pos = 0;
    FunctionKind kind;
    lda.EnumerateLiteralVals(
        id, [elements, properties, &entryIndexes, &pos, &epos, &ppos, factory, thread, jsPandaFile,
                &methodId, &kind, &constpool, &entry, &isLoadedAOT](const LiteralValue &value, const LiteralTag &tag) {
        JSTaggedValue jt = JSTaggedValue::Null();
        bool flag = false;
        switch (tag) {
            case LiteralTag::INTEGER: {
                jt = JSTaggedValue(static_cast<int32_t>(std::get<uint32_t>(value)));
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
                StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::SHARED_OLD_SPACE);
                jt = JSTaggedValue(str);
                uint32_t elementIndex = 0;
                if (JSTaggedValue::ToElementIndex(jt, &elementIndex) && ppos % pairSize == 0) {
                    flag = true;
                }
                break;
            }
            case LiteralTag::METHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::NORMAL_FUNCTION;
                break;
            }
            case LiteralTag::GETTER: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GETTER_FUNCTION;
                break;
            }
            case LiteralTag::SETTER: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::SETTER_FUNCTION;
                break;
            }
            case LiteralTag::GENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GENERATOR_FUNCTION;
                break;
            }

            case LiteralTag::ASYNCGENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
                break;
            }

            case LiteralTag::METHODAFFILIATE: {
                uint16_t length = std::get<uint16_t>(value);
                int entryIndex = 0;
                bool needSetAotFlag = (isLoadedAOT && (epos % pairSize == 0) && !flag);
                if (needSetAotFlag) {
                    entryIndex = entryIndexes->GetObjectFromCache(pos++).GetInt();
                    // -1 : this jsfunction is a large function
                    if (entryIndex == -1) {
                        needSetAotFlag = false;
                    }
                }
                JSHandle<JSFunction> jsFunc =
                    DefineMethodInLiteral(thread, jsPandaFile, methodId, constpool, kind,
                                          length, entry, needSetAotFlag, entryIndex);
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
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                break;
            }
        }
        if (tag != LiteralTag::METHOD && tag != LiteralTag::GETTER && tag != LiteralTag::SETTER &&
            tag != LiteralTag::GENERATORMETHOD && tag != LiteralTag::ASYNCGENERATORMETHOD) {
            if ((epos % pairSize == 0) && !flag) {
                properties->Set(thread, ppos++, jt);
            } else {
                elements->Set(thread, epos++, jt);
            }
        }
    });
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreTypeForClass(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                                       EntityId id, JSHandle<ConstantPool> constpool,
                                                                       const CString &entryPoint, bool isLoadedAOT,
                                                                       JSHandle<AOTLiteralInfo> entryIndexes,
                                                                       ElementsKind *newKind,
                                                                       JSHandle<JSTaggedValue> sendableEnv,
                                                                       ClassKind classKind)
{
    ASSERT(jsPandaFile != nullptr);
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    uint32_t num = lda.GetLiteralValsNum(id) / 2;  // 2: half
    // The num is 1, indicating that the current class has no member variable.
    if (num == 1) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->EmptyArray();
    }
    return GetDatasIgnoreType(
        thread, jsPandaFile, id, constpool, entryPoint, isLoadedAOT, entryIndexes, newKind, sendableEnv, classKind);
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreType(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                               EntityId id, JSHandle<ConstantPool> constpool,
                                                               const CString &entryPoint, bool isLoadedAOT,
                                                               JSHandle<AOTLiteralInfo> entryIndexes,
                                                               ElementsKind *newKind,
                                                               JSHandle<JSTaggedValue> sendableEnv,
                                                               ClassKind classKind)
{
    ASSERT(jsPandaFile != nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    uint32_t num = lda.GetLiteralValsNum(id) / 2;  // 2: half
    JSHandle<TaggedArray> literals;
    if (classKind == ClassKind::SENDABLE) {
        literals = JSHandle<TaggedArray>(factory->NewSCOWTaggedArray(num));
    } else {
        literals = JSHandle<TaggedArray>(factory->NewCOWTaggedArray(num));
    }
    uint32_t pos = 0;
    uint32_t methodId = 0;
    FunctionKind kind;
    int index = 0;
    lda.EnumerateLiteralVals(
        id, [literals, &pos, factory, thread, jsPandaFile, &methodId, &kind, &constpool,
            &entryPoint, &entryIndexes, &index, isLoadedAOT, newKind, classKind, &sendableEnv]
        (const LiteralValue &value, const LiteralTag &tag) {
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
                    StringData sd = jsPandaFile->GetStringData(EntityId(std::get<uint32_t>(value)));
                    EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::SHARED_OLD_SPACE);
                    jt = JSTaggedValue(str);
                    break;
                }
                case LiteralTag::METHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::NORMAL_FUNCTION;
                    break;
                }
                case LiteralTag::GETTER: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::GETTER_FUNCTION;
                    break;
                }
                case LiteralTag::SETTER: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::SETTER_FUNCTION;
                    break;
                }
                case LiteralTag::GENERATORMETHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::GENERATOR_FUNCTION;
                    break;
                }
                case LiteralTag::ASYNCGENERATORMETHOD: {
                    methodId = std::get<uint32_t>(value);
                    kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
                    break;
                }
                case LiteralTag::METHODAFFILIATE: {
                    uint16_t length = std::get<uint16_t>(value);
                    int entryIndex = 0;
                    bool needSetAotFlag = isLoadedAOT;
                    if (isLoadedAOT) {
                        entryIndex = entryIndexes->GetObjectFromCache(index++).GetInt();
                        if (entryIndex == -1) {
                            needSetAotFlag = false;
                        }
                    }
                    JSHandle<FunctionTemplate> funcTemp = DefineFunctionTemplate(thread, jsPandaFile, methodId,
                        constpool, kind, length, entryPoint, needSetAotFlag, entryIndex, sendableEnv, classKind);
                    jt = funcTemp.GetTaggedValue();
                    break;
                }
                case LiteralTag::ACCESSOR: {
                    JSHandle<AccessorData> accessor = factory->NewAccessorData();
                    jt = accessor.GetTaggedValue();
                    break;
                }
                case LiteralTag::LITERALARRAY: {
                    jt = JSTaggedValue(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::NULLVALUE: {
                    break;
                }
                default: {
                    LOG_ECMA(FATAL) << "this branch is unreachable";
                    UNREACHABLE();
                    break;
                }
            }
            if (tag != LiteralTag::METHOD && tag != LiteralTag::GETTER && tag != LiteralTag::SETTER &&
                tag != LiteralTag::GENERATORMETHOD && tag != LiteralTag::ASYNCGENERATORMETHOD) {
                if (newKind != nullptr) {
                    *newKind = Elements::ToElementsKind(jt, *newKind);
                }
                literals->Set(thread, pos++, jt);
            } else {
                uint32_t oldLength = literals->GetLength();
                ASSERT(oldLength > 0);
                literals->Trim(thread, oldLength - 1);
            }
        });
    return literals;
}
}  // namespace panda::ecmascript
