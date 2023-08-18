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
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/tagged_array-inl.h"

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
                EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::OLD_SPACE);
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
            case LiteralTag::GENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GENERATOR_FUNCTION;
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
                    EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::OLD_SPACE);
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
                                                                 uint32_t offset, JSHandle<ConstantPool> constpool,
                                                                 FunctionKind kind, uint16_t length,
                                                                 const CString &entryPoint,
                                                                 bool isLoadedAOT, uint32_t entryIndex)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    auto methodLiteral = jsPandaFile->FindMethodLiteral(offset);
    ASSERT(methodLiteral != nullptr);
    methodLiteral->SetFunctionKind(kind);
    bool canFastCall = false;
    JSHandle<Method> method = factory->NewMethod(
        jsPandaFile, methodLiteral, constpool, entryIndex, isLoadedAOT, &canFastCall);
    JSHandle<JSFunction> jsFunc = factory->NewJSFunction(method, kind, isLoadedAOT, canFastCall);
    jsFunc->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));

    CString moduleName = jsPandaFile->GetJSPandaFileDesc();
    CString entry = JSPandaFile::ENTRY_FUNCTION_NAME;
    if (!entryPoint.empty()) {
        moduleName = entryPoint;
        entry = entryPoint;
    }
    JSRecordInfo recordInfo;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(entry, recordInfo);
    if (!hasRecord) {
        LOG_ECMA(FATAL) << "cannot find record '" + entry + "', please check the request path.";
    }
    if (jsPandaFile->IsModule(recordInfo)) {
        JSHandle<SourceTextModule> module = thread->GetCurrentEcmaContext()->GetModuleManager()->HostGetImportedModule(
            moduleName);
        jsFunc->SetModule(thread, module.GetTaggedValue());
    }
    return jsFunc;
}

void LiteralDataExtractor::GetMethodOffsets(const JSPandaFile *jsPandaFile, size_t index,
                                            std::vector<uint32_t> &methodOffsets)
{
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    lda.EnumerateLiteralVals(index, [&methodOffsets](const LiteralValue &value, const LiteralTag &tag) {
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

void LiteralDataExtractor::GetMethodOffsets(const JSPandaFile *jsPandaFile, EntityId id,
                                            std::vector<uint32_t> &methodOffsets)
{
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    lda.EnumerateLiteralVals(id, [&methodOffsets](const LiteralValue &value, const LiteralTag &tag) {
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
                EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::OLD_SPACE);
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
            case LiteralTag::GENERATORMETHOD: {
                methodId = std::get<uint32_t>(value);
                kind = FunctionKind::GENERATOR_FUNCTION;
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
        if (tag != LiteralTag::METHOD && tag != LiteralTag::GENERATORMETHOD) {
            if ((epos % pairSize == 0) && !flag) {
                properties->Set(thread, ppos++, jt);
            } else {
                elements->Set(thread, epos++, jt);
            }
        }
    });
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreType(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                               EntityId id, JSHandle<ConstantPool> constpool,
                                                               const CString &entryPoint,
                                                               bool isLoadedAOT, JSHandle<AOTLiteralInfo> entryIndexes)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
    uint32_t num = lda.GetLiteralValsNum(id) / 2;  // 2: half
    JSHandle<TaggedArray> literals = JSHandle<TaggedArray>(factory->NewCOWTaggedArray(num));
    uint32_t pos = 0;
    uint32_t methodId = 0;
    FunctionKind kind;
    int index = 0;
    lda.EnumerateLiteralVals(
        id, [literals, &pos, factory, thread, jsPandaFile,
             &methodId, &kind, &constpool, &entryPoint, &entryIndexes, &index, isLoadedAOT]
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
                    EcmaString *str = factory->GetRawStringFromStringTable(sd, MemSpaceType::OLD_SPACE);
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
                    int entryIndex = 0;
                    bool needSetAotFlag = isLoadedAOT;
                    if (isLoadedAOT) {
                        entryIndex = entryIndexes->GetObjectFromCache(index++).GetInt();
                        if (entryIndex == -1) {
                            needSetAotFlag = false;
                        }
                    }
                    JSHandle<JSFunction> jsFunc =
                        DefineMethodInLiteral(thread, jsPandaFile, methodId, constpool,
                            kind, length, entryPoint, needSetAotFlag, entryIndex);
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
            if (tag != LiteralTag::METHOD && tag != LiteralTag::GENERATORMETHOD) {
                literals->Set(thread, pos++, jt);
            } else {
                uint32_t oldLength = literals->GetLength();
                literals->Trim(thread, oldLength - 1);
            }
        });
    return literals;
}
}  // namespace panda::ecmascript
