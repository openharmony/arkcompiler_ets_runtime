/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/ts_types/ts_type_parser.h"

#include "ecmascript/subtyping_operator.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
// For each property of one class, object or interface, it's name and typeIndex are recorded in order,
// and if it is a field, then accessFlag and readonly of this property will be additionally recorded.
static constexpr uint32_t FIELD_LENGTH = 4;
static constexpr uint32_t METHOD_LENGTH = 2;
static constexpr uint32_t INDEX_OCCUPIED_OFFSET = 1;

TSTypeParser::TSTypeParser(TSManager *tsManager)
    : tsManager_(tsManager), vm_(tsManager->GetEcmaVM()),
      thread_(vm_->GetJSThread()), factory_(vm_->GetFactory()),
      tableGenerator_(tsManager_)
{
    auto bcInfoCollector = tsManager_->GetBytecodeInfoCollector();
    if (bcInfoCollector != nullptr) {
        bcInfo_ = bcInfoCollector->GetBytecodeInfoPtr();
    }
}

GlobalTSTypeRef TSTypeParser::CreateGT(const JSPandaFile *jsPandaFile, const CString &recordName, uint32_t typeId)
{
    if (typeId <= BUILDIN_TYPE_OFFSET) {
        return GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE), typeId);
    }

    if (!IsUserDefinedType(typeId)) {
        return ParseBuiltinObjType(typeId);
    }

    // Negative numbers are used to represent type parameters in generics types.
    if (static_cast<int32_t>(typeId) < 0) {
        return EncodeParaType(typeId);
    }

    GlobalTypeID gId(jsPandaFile, typeId);
    if (tsManager_->HasCreatedGT(gId)) {
        return tsManager_->GetGTByGlobalTypeID(gId);
    }
    return ParseType(jsPandaFile, recordName, typeId);
}

GlobalTSTypeRef TSTypeParser::ParseBuiltinObjType(uint32_t typeId)
{
    if (!tsManager_->IsBuiltinsDTSEnabled()) {
        return GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::BUILTIN), typeId);
    }
    const JSPandaFile *builtinjsPandaFile = tsManager_->GetBuiltinPandaFile();
    if (builtinjsPandaFile == nullptr) {
        LOG_COMPILER(FATAL) << "load lib_ark_builtins.d.ts failed";
    }
    uint32_t offset = tsManager_->GetBuiltinOffset(typeId);
    return CreateGT(builtinjsPandaFile, tsManager_->GetBuiltinRecordName(), offset);
}

GlobalTSTypeRef TSTypeParser::ParseType(const JSPandaFile *jsPandaFile, const CString &recordName, uint32_t typeId)
{
    TypeLiteralExtractor typeLiteralExtractor(jsPandaFile, typeId);
    if (!typeLiteralExtractor.IsVaildTypeLiteral()) {  // typeLiteral maybe hole in d.abc
        return GlobalTSTypeRef::Default();
    }

    if (TypeNeedResolve(&typeLiteralExtractor)) {
        return ResolveType(jsPandaFile, recordName, &typeLiteralExtractor);
    }

    uint32_t moduleId = tableGenerator_.TryGetModuleId(recordName);
    if (UNLIKELY(!GlobalTSTypeRef::IsValidModuleId(moduleId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypeTables is reached. All TSTypes in the record "
                            << recordName << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    JSHandle<TSTypeTable> table = tableGenerator_.GetOrGenerateTSTypeTable(jsPandaFile, recordName, moduleId);
    uint32_t localId = tableGenerator_.TryGetLocalId(table);
    if (UNLIKELY(!GlobalTSTypeRef::IsValidLocalId(localId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypes in TSTypeTable " << moduleId << " is reached. "
                            << "The TSType with typeId " << typeId << " in the record " << recordName
                            << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    table->SetNumberOfTypes(thread_, localId);
    GlobalTSTypeRef gt = GetAndStoreGT(jsPandaFile, typeId, recordName, moduleId, localId);
    JSHandle<JSTaggedValue> type = ParseNonImportType(jsPandaFile, recordName, &typeLiteralExtractor);
    if (UNLIKELY(type->IsUndefined())) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    SetTSType(table, type, gt);
    if (typeLiteralExtractor.GetTypeKind() != TSTypeKind::GENERIC_INSTANCE) {
        tsManager_->CollectGT(gt);  // collect types that need to generate hclasses
    }
    return gt;
}

GlobalTSTypeRef TSTypeParser::ResolveType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                          TypeLiteralExtractor *typeLiteralExtractor)
{
    if (typeLiteralExtractor->IsGenerics()) {
        return ParseGenericsType(jsPandaFile, recordName, typeLiteralExtractor);
    }

    auto kind = typeLiteralExtractor->GetTypeKind();
    switch (kind) {
        case TSTypeKind::IMPORT: {
            return ResolveImportType(jsPandaFile, recordName, typeLiteralExtractor);
        }
        case TSTypeKind::INDEXSIG: {
            return ParseIndexSigType(jsPandaFile, recordName, typeLiteralExtractor);
        }
        default: {
            return GlobalTSTypeRef::Default();
        }
    }
}

GlobalTSTypeRef TSTypeParser::ResolveImportType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                TypeLiteralExtractor *typeLiteralExtractor)
{
    uint32_t typeId = typeLiteralExtractor->GetTypeOffset();
    JSHandle<EcmaString> importVarNamePath =
        factory_->NewFromUtf8(typeLiteralExtractor->GetStringValue(DEFAULT_INDEX));
    JSHandle<EcmaString> relativePath = GenerateImportRelativePath(importVarNamePath);
    CString cstringRelativePath = ConvertToString(*relativePath);
    // skip @ohos:|@app:|@native: prefixed imports
    if (ModulePathHelper::IsNativeModuleRequest(cstringRelativePath)) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    CString baseFileName = jsPandaFile->GetJSPandaFileDesc();
    CString entryPoint =
        ModulePathHelper::ConcatFileNameWithMerge(thread_, jsPandaFile, baseFileName, recordName, cstringRelativePath);
    if (entryPoint.empty()) {
        LOG_COMPILER(DEBUG) << "EntryPoint is empty. Please check whether concating file name is correct or "
                               "whether the module request recorded in the import-type literal is correct.";
    }
    // skip files without type information
    if (UNLIKELY(!jsPandaFile->HasTypeSummaryOffset(entryPoint))) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    uint32_t moduleId = tableGenerator_.TryGetModuleId(entryPoint);
    if (UNLIKELY(!GlobalTSTypeRef::IsValidModuleId(moduleId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypeTables is reached. All TSTypes in the record "
                            << entryPoint << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    JSHandle<TSTypeTable> table = tableGenerator_.GetOrGenerateTSTypeTable(jsPandaFile, entryPoint, moduleId);
    JSHandle<JSTaggedValue> exportTable = GenerateExportTableFromRecord(jsPandaFile, entryPoint, table);
    JSHandle<TaggedArray> arrayWithGT(exportTable);
    JSHandle<EcmaString> targetVarName = GenerateImportVar(importVarNamePath);
    std::unordered_set<CString> markSet;
    GlobalTSTypeRef importedGT = GetExportGTByName(targetVarName, arrayWithGT, jsPandaFile, entryPoint, markSet);
    return GetAndStoreImportGT(jsPandaFile, typeId, recordName, importedGT);
}

GlobalTSTypeRef TSTypeParser::ParseIndexSigType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::INDEXSIG);
    uint32_t length = typeLiteralExtractor->GetIntValue(NUM_INDEX_SIG_INDEX);
    JSHandle<TSObjLayoutInfo> indexSignInfo = factory_->CreateTSObjLayoutInfo(length);
    typeLiteralExtractor->EnumerateTypesWithIntKey(NUM_INDEX_SIG_INDEX,
        [this, &jsPandaFile, &recordName, &indexSignInfo](const uint32_t literalTag, const uint32_t literalValue) {
            auto keyGT = CreateGT(jsPandaFile, recordName, literalTag);
            auto valueGT = CreateGT(jsPandaFile, recordName, literalValue);
            indexSignInfo->AddProperty(thread_, JSTaggedValue(keyGT.GetType()), JSTaggedValue(valueGT.GetType()));
        });

    uint32_t oriTypeId = typeLiteralExtractor->GetIntValue(DEFAULT_INDEX);
    auto oriGT = CreateGT(jsPandaFile, recordName, oriTypeId);
    JSHandle<JSTaggedValue> type = tsManager_->GetTSType(oriGT);
    if (type->IsTSClassType()) {
        JSHandle<TSClassType> classType(type);
        classType->SetIndexSigns(thread_, indexSignInfo);
    } else if (type->IsTSObjectType()) {
        JSHandle<TSObjectType> objectType(type);
        objectType->SetIndexSigns(thread_, indexSignInfo);
    } else if (type->IsTSInterfaceType()) {
        JSHandle<TSInterfaceType> interfaceType(type);
        interfaceType->SetIndexSigns(thread_, indexSignInfo);
    } else {
        LOG_COMPILER(DEBUG) << "Unsupport TSType with index signature: "
                            << static_cast<uint32_t>(typeLiteralExtractor->GetTypeKind());
    }
    uint32_t typeId = typeLiteralExtractor->GetTypeOffset();
    return GetAndStoreGT(jsPandaFile, typeId, recordName, oriGT.GetModuleId(), oriGT.GetLocalId());
}

JSHandle<JSTaggedValue> TSTypeParser::ParseNonImportType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                         TypeLiteralExtractor *typeLiteralExtractor)
{
    auto kind = typeLiteralExtractor->GetTypeKind();
    switch (kind) {
        case TSTypeKind::CLASS: {
            JSHandle<TSClassType> classType = ParseClassType(jsPandaFile, recordName, typeLiteralExtractor);
            return JSHandle<JSTaggedValue>(classType);
        }
        case TSTypeKind::CLASS_INSTANCE: {
            return ParseClassInstanceType(jsPandaFile, recordName, typeLiteralExtractor);
        }
        case TSTypeKind::INTERFACE: {
            JSHandle<TSInterfaceType> interfaceType = ParseInterfaceType(jsPandaFile, recordName, typeLiteralExtractor);
            return JSHandle<JSTaggedValue>(interfaceType);
        }
        case TSTypeKind::UNION: {
            JSHandle<TSUnionType> unionType = ParseUnionType(jsPandaFile, recordName, typeLiteralExtractor);
            return JSHandle<JSTaggedValue>(unionType);
        }
        case TSTypeKind::FUNCTION: {
            JSHandle<TSFunctionType> functionType =
                ParseFunctionType(jsPandaFile, recordName, typeLiteralExtractor);
            return JSHandle<JSTaggedValue>(functionType);
        }
        case TSTypeKind::ARRAY: {
            JSHandle<TSArrayType> arrayType = ParseArrayType(jsPandaFile, recordName, typeLiteralExtractor);
            return JSHandle<JSTaggedValue>(arrayType);
        }
        case TSTypeKind::OBJECT: {
            JSHandle<TSObjectType> objectType = ParseObjectType(jsPandaFile, recordName, typeLiteralExtractor);
            return JSHandle<JSTaggedValue>(objectType);
        }
        case TSTypeKind::BUILTIN_INSTANCE:
        case TSTypeKind::GENERIC_INSTANCE: {
            return ParseGenericsInstanceType(jsPandaFile, recordName, typeLiteralExtractor);
        }
        default: {
            LOG_COMPILER(DEBUG) << "Do not support parse types with kind " << static_cast<uint32_t>(kind) << ". "
                                << "Please check whether the type literal " << typeLiteralExtractor->GetTypeOffset()
                                << " recorded in the record " << recordName << " is correct.";
            return thread_->GlobalConstants()->GetHandledUndefined();
        }
    }
}

JSHandle<TSClassType> TSTypeParser::ParseClassType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                   TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::CLASS);
    ClassLiteralInfo classLitInfo(typeLiteralExtractor);
    uint32_t numNonStaticFieldsIndex = classLitInfo.numNonStaticFieldsIndex;
    uint32_t numNonStaticMethodsIndex = classLitInfo.numNonStaticMethodsIndex;
    uint32_t numStaticFieldsIndex = classLitInfo.numStaticFieldsIndex;
    uint32_t numStaticMethodsIndex = classLitInfo.numStaticMethodsIndex;

    JSHandle<TSClassType> classType = factory_->NewTSClassType();
    SetClassName(classType, jsPandaFile, typeLiteralExtractor);
    SetSuperClassType(classType, jsPandaFile, recordName, typeLiteralExtractor);

    // resolve instance type
    uint32_t numFields = typeLiteralExtractor->GetIntValue(numNonStaticFieldsIndex);
    JSHandle<TSObjectType> instanceType = factory_->NewTSObjectType(numFields);
    FillPropTypes(jsPandaFile, recordName, instanceType, typeLiteralExtractor, numNonStaticFieldsIndex, FIELD_LENGTH);
    classType->SetInstanceType(thread_, instanceType);

    // resolve prototype type
    uint32_t numNonStatic = typeLiteralExtractor->GetIntValue(numNonStaticMethodsIndex);
    JSHandle<TSObjectType> prototypeType = factory_->NewTSObjectType(numNonStatic);
    FillPropTypes(jsPandaFile, recordName, prototypeType, typeLiteralExtractor,
                  numNonStaticMethodsIndex, METHOD_LENGTH);
    classType->SetPrototypeType(thread_, prototypeType);

    // resolve constructor type
    uint32_t numStaticFields = typeLiteralExtractor->GetIntValue(numStaticFieldsIndex);
    uint32_t numStaticMethods = typeLiteralExtractor->GetIntValue(numStaticMethodsIndex);
    uint32_t numStatic = numStaticFields + numStaticMethods;
    JSHandle<TSObjectType> constructorType = factory_->NewTSObjectType(numStatic);
    FillPropTypes(jsPandaFile, recordName, constructorType, typeLiteralExtractor, numStaticFieldsIndex, FIELD_LENGTH);
    FillPropTypes(jsPandaFile, recordName, constructorType, typeLiteralExtractor,
                  numStaticMethodsIndex, METHOD_LENGTH);
    classType->SetConstructorType(thread_, constructorType);
    return classType;
}

JSHandle<JSTaggedValue> TSTypeParser::ParseClassInstanceType(const JSPandaFile *jsPandaFile,
                                                             const CString &recordName,
                                                             TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::CLASS_INSTANCE);
    // classTypeId is stored in the first position
    uint32_t classTypeId = typeLiteralExtractor->GetIntValue(DEFAULT_INDEX);
    if (classTypeId == static_cast<uint32_t>(BuiltinTypeId::ARRAY)) {
        return JSHandle<JSTaggedValue>(factory_->NewTSArrayType());
    }
    auto classGT = CreateGT(jsPandaFile, recordName, classTypeId);
    if (tsManager_->IsArrayTypeKind(classGT)) {
        return tsManager_->GetTSType(classGT);
    }
    JSHandle<TSClassInstanceType> classInstanceType = factory_->NewTSClassInstanceType();
    classInstanceType->SetClassGT(classGT);
    return JSHandle<JSTaggedValue>(classInstanceType);
}

JSHandle<TSInterfaceType> TSTypeParser::ParseInterfaceType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                           TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::INTERFACE);
    JSHandle<TSInterfaceType> interfaceType = factory_->NewTSInterfaceType();
    InterfaceLiteralInfo interfaceLitInfo(typeLiteralExtractor);
    uint32_t numFieldsIndex = interfaceLitInfo.numFieldsIndex;
    uint32_t numMethodsIndex = interfaceLitInfo.numMethodsIndex;

    // resolve extends of interface
    uint32_t numExtends = typeLiteralExtractor->GetIntValue(InterfaceLiteralInfo::NUM_EXTENDS_INDEX);
    JSHandle<TaggedArray> extendsId = factory_->NewTaggedArray(numExtends);
    uint32_t extendsIndex = 0;
    typeLiteralExtractor->EnumerateElements(InterfaceLiteralInfo::NUM_EXTENDS_INDEX,
        [this, &jsPandaFile, &recordName, &extendsIndex, &extendsId](const uint32_t literalValue) {
            auto extendGT = CreateGT(jsPandaFile, recordName, literalValue);
            extendsId->Set(thread_, extendsIndex++, JSTaggedValue(extendGT.GetType()));
        });
    interfaceType->SetExtends(thread_, extendsId);

    uint32_t numFields = typeLiteralExtractor->GetIntValue(numFieldsIndex);
    uint32_t numMethods = typeLiteralExtractor->GetIntValue(numMethodsIndex);
    uint32_t totalFields = numFields + numMethods;
    JSHandle<TSObjectType> fieldsType = factory_->NewTSObjectType(totalFields);
    FillPropTypes(jsPandaFile, recordName, fieldsType, typeLiteralExtractor, numFieldsIndex, FIELD_LENGTH);
    FillInterfaceMethodTypes(jsPandaFile, recordName, fieldsType, typeLiteralExtractor, numMethodsIndex);
    interfaceType->SetFields(thread_, fieldsType);
    return interfaceType;
}

JSHandle<TSUnionType> TSTypeParser::ParseUnionType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                   TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::UNION);
    // the number of union types is stored in the first position
    uint32_t numOfUnionMembers = typeLiteralExtractor->GetIntValue(DEFAULT_INDEX);
    JSHandle<TSUnionType> unionType = factory_->NewTSUnionType(numOfUnionMembers);

    JSHandle<TaggedArray> components(thread_, unionType->GetComponents());
    uint32_t index = 0;
    typeLiteralExtractor->EnumerateElements(DEFAULT_INDEX,
        [this, &jsPandaFile, &recordName, &index, &components](const uint32_t literalValue) {
            auto componentGT = CreateGT(jsPandaFile, recordName, literalValue);
            components->Set(thread_, index++, JSTaggedValue(componentGT.GetType()));
        });
    unionType->SetComponents(thread_, components);
    return unionType;
}

JSHandle<TSFunctionType> TSTypeParser::ParseFunctionType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                         TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::FUNCTION);
    FunctionLiteralInfo functionLitInfo(typeLiteralExtractor);
    uint32_t numParasIndex = functionLitInfo.numParasIndex;
    uint32_t returnTypeIndex = functionLitInfo.returnTypeIndex;

    uint32_t length = typeLiteralExtractor->GetIntValue(numParasIndex);
    JSHandle<TSFunctionType> functionType = factory_->NewTSFunctionType(length);

    SetFunctionThisType(functionType, jsPandaFile, recordName, typeLiteralExtractor);
    JSHandle<TaggedArray> parameterTypes(thread_, functionType->GetParameterTypes());
    uint32_t index = 0;
    typeLiteralExtractor->EnumerateElements(numParasIndex,
        [this, &jsPandaFile, &recordName, &index, &parameterTypes](const uint32_t literalValue) {
            auto parameterGT = CreateGT(jsPandaFile, recordName, literalValue);
            if (tsManager_->IsClassTypeKind(parameterGT)) {
                parameterGT = tsManager_->CreateClassInstanceType(parameterGT);
            }
            parameterTypes->Set(thread_, index++, JSTaggedValue(parameterGT.GetType()));
        });
    functionType->SetParameterTypes(thread_, parameterTypes);

    JSHandle<JSTaggedValue> functionName(
        factory_->NewFromUtf8(typeLiteralExtractor->GetStringValue(FunctionLiteralInfo::NAME_INDEX)));
    functionType->SetName(thread_, functionName);

    uint32_t returntypeId = typeLiteralExtractor->GetIntValue(returnTypeIndex);
    auto returnGT = CreateGT(jsPandaFile, recordName, returntypeId);
    functionType->SetReturnGT(returnGT);

    uint32_t bitField = typeLiteralExtractor->GetIntValue(FunctionLiteralInfo::BITFIELD_INDEX);
    functionType->SetBitField(bitField);

    StoreMethodOffset(functionType, typeLiteralExtractor);
    return functionType;
}

JSHandle<TSArrayType> TSTypeParser::ParseArrayType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                   TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::ARRAY);
    JSHandle<TSArrayType> arrayType = factory_->NewTSArrayType();
    // the type of elements of array is stored in the first position
    auto elemetnGT = CreateGT(jsPandaFile, recordName, typeLiteralExtractor->GetIntValue(DEFAULT_INDEX));
    if (tsManager_->IsClassTypeKind(elemetnGT)) {
        elemetnGT = tsManager_->CreateClassInstanceType(elemetnGT);
    }
    arrayType->SetElementGT(elemetnGT);
    return arrayType;
}

JSHandle<TSObjectType> TSTypeParser::ParseObjectType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                     TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::OBJECT);
    // the number of properties of object is stored in the first position
    uint32_t length = typeLiteralExtractor->GetIntValue(DEFAULT_INDEX);
    JSHandle<TSObjectType> objectType = factory_->NewTSObjectType(length);
    FillPropTypes(jsPandaFile, recordName, objectType, typeLiteralExtractor, DEFAULT_INDEX, METHOD_LENGTH);
    return objectType;
}

void TSTypeParser::FillPropTypes(const JSPandaFile *jsPandaFile,
                                 const CString &recordName,
                                 const JSHandle<TSObjectType> &objectType,
                                 TypeLiteralExtractor *typeLiteralExtractor,
                                 const uint32_t numOfFieldIndex,
                                 const uint32_t gap)
{
    JSHandle<TSObjLayoutInfo> layout(thread_, objectType->GetObjLayoutInfo());
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    typeLiteralExtractor->EnumerateProperties(numOfFieldIndex, gap,
        [this, &key, &jsPandaFile, &recordName, &layout](const CString &literalKey, const uint32_t literalValue) {
            JSHandle<JSTaggedValue> propName(factory_->NewFromUtf8(literalKey));
            key.Update(propName.GetTaggedValue());
            ASSERT(key->IsString());
            auto gt = CreateGT(jsPandaFile, recordName, literalValue);
            if (tsManager_->IsClassTypeKind(gt)) {
                gt = tsManager_->CreateClassInstanceType(gt);
            }
            layout->AddProperty(thread_, key.GetTaggedValue(), JSTaggedValue(gt.GetType()));
        });
}

void TSTypeParser::FillInterfaceMethodTypes(const JSPandaFile *jsPandaFile,
                                            const CString &recordName,
                                            const JSHandle<TSObjectType> &objectType,
                                            TypeLiteralExtractor *typeLiteralExtractor,
                                            const uint32_t numExtends)
{
    JSHandle<TSObjLayoutInfo> layout(thread_, objectType->GetObjLayoutInfo());
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    typeLiteralExtractor->EnumerateElements(numExtends,
        [this, &jsPandaFile, &recordName, &layout, &key](const uint32_t literalValue) {
            auto gt = CreateGT(jsPandaFile, recordName, literalValue);
            if (tsManager_->IsFunctionTypeKind(gt)) {
                JSHandle<JSTaggedValue> tsType = tsManager_->GetTSType(gt);
                ASSERT(tsType->IsTSFunctionType());
                JSHandle<TSFunctionType> functionType(tsType);
                key.Update(functionType->GetName());
            };
            layout->AddProperty(thread_, key.GetTaggedValue(), JSTaggedValue(gt.GetType()));
        });
}

void TSTypeParser::SetClassName(const JSHandle<TSClassType> &classType,
                                const JSPandaFile *jsPandaFile,
                                TypeLiteralExtractor *typeLiteralExtractor)
{
    std::string className = tsManager_->GetClassNameByOffset(jsPandaFile, typeLiteralExtractor->GetTypeOffset());
    JSHandle<EcmaString> classEcmaString = factory_->NewFromStdString(className);
    classType->SetName(thread_, classEcmaString.GetTaggedValue());
}

void TSTypeParser::SetSuperClassType(const JSHandle<TSClassType> &classType,
                                     const JSPandaFile *jsPandaFile,
                                     const CString &recordName,
                                     TypeLiteralExtractor *typeLiteralExtractor)
{
    uint32_t extendsTypeId = typeLiteralExtractor->GetIntValue(ClassLiteralInfo::SUPER_CLASS_INDEX);
    if (TSClassType::IsBaseClassType(extendsTypeId)) {
        classType->SetHasLinked(true);
    } else {
        auto extensionGT = CreateGT(jsPandaFile, recordName, extendsTypeId);
        classType->SetExtensionGT(extensionGT);
        if (extensionGT.IsDefault()) {
            classType->SetHasLinked(true);
        }
    }
}

void TSTypeParser::SetFunctionThisType(const JSHandle<TSFunctionType> &functionType,
                                       const JSPandaFile *jsPandaFile,
                                       const CString &recordName,
                                       TypeLiteralExtractor *typeLiteralExtractor)
{
    bool hasThisType = static_cast<bool>(typeLiteralExtractor->GetIntValue(FunctionLiteralInfo::HAS_THIS_TYPE_INDEX));
    if (hasThisType) {
        // if hasThisType is true, then the next position will store typeId of this
        uint32_t thisTypeId = typeLiteralExtractor->GetIntValue(FunctionLiteralInfo::HAS_THIS_TYPE_INDEX + 1);
        auto thisGT = CreateGT(jsPandaFile, recordName, thisTypeId);
        functionType->SetThisGT(thisGT);
    }
}

void TSTypeParser::StoreMethodOffset(const JSHandle<TSFunctionType> &functionType,
                                     TypeLiteralExtractor *typeLiteralExtractor)
{
    if (bcInfo_ != nullptr) {
        uint32_t typeOffset = typeLiteralExtractor->GetTypeOffset();
        uint32_t methodOffset = bcInfo_->IterateFunctionTypeIDAndMethodOffset(typeOffset);
        if (methodOffset != 0) {
            functionType->SetMethodOffset(methodOffset);
            functionType->SetIsMethodOffsetVaild(true);
            bool isVaild;
            kungfu::FastCallInfo info = bcInfo_->IterateMethodOffsetToFastCallInfo(methodOffset, &isVaild);
            functionType->SetIsFastCallVaild(isVaild);
            functionType->SetIsFastCall(info.canFastCall_);
            functionType->SetIsNoGC(info.isNoGC_);
        } else {
            functionType->SetIsMethodOffsetVaild(false);
            functionType->SetIsFastCallVaild(false);
        }
    }
}

JSHandle<JSTaggedValue> TSTypeParser::GenerateExportTableFromRecord(const JSPandaFile *jsPandaFile,
                                                                    const CString &recordName,
                                                                    const JSHandle<TSTypeTable> &table)
{
    JSHandle<JSTaggedValue> exportValeTable = TSTypeTable::GetExportValueTable(thread_, table);
    if (exportValeTable->IsUndefined()) {
        // Read export-data from annotation of the .abc File
        JSHandle<TaggedArray> literalTable = tsManager_->GetExportTableFromLiteral(jsPandaFile, recordName);
        ObjectFactory *factory = vm_->GetFactory();
        uint32_t length = literalTable->GetLength();
        JSHandle<TaggedArray> exportTable = factory->NewAndCopyTaggedArray(literalTable, length, length);
        for (uint32_t i = 1; i < length; i += 2) {  // 2: skip a pair of key and value
            JSTaggedValue target = exportTable->Get(i);
            // Create GT based on typeId, and wrapped it into a JSTaggedValue
            uint32_t typeId = static_cast<uint32_t>(target.GetInt());
            GlobalTSTypeRef gt = CreateGT(jsPandaFile, recordName, typeId);
            // Set the wrapped GT to exportTable
            exportTable->Set(thread_, i, JSTaggedValue(gt.GetType()));
        }
        TSTypeTable::SetExportValueTable(thread_, table, exportTable);
        return JSHandle<JSTaggedValue>(exportTable);
    }
    ASSERT(exportValeTable->IsTaggedArray());
    return exportValeTable;
}

JSHandle<EcmaString> TSTypeParser::GenerateImportRelativePath(JSHandle<EcmaString> importRel) const
{
    // importNamePath #A#./A
    CString importNamePath = ConvertToString(importRel.GetTaggedValue());
    auto lastPos = importNamePath.find_last_of('#');
    CString path = importNamePath.substr(lastPos + 1, importNamePath.size() - lastPos - 1);
    return factory_->NewFromUtf8(path); // #A#./A -> ./A
}

JSHandle<EcmaString> TSTypeParser::GenerateImportVar(JSHandle<EcmaString> import) const
{
    // importNamePath #A#./A
    CString importVarNamePath = ConvertToString(import.GetTaggedValue());
    auto firstPos = importVarNamePath.find_first_of('#');
    auto lastPos = importVarNamePath.find_last_of('#');
    CString target = importVarNamePath.substr(firstPos + 1, lastPos - firstPos - 1);
    return factory_->NewFromUtf8(target); // #A#./A -> A
}

GlobalTSTypeRef TSTypeParser::GetExportGTByName(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable,
                                                const JSPandaFile *jsPandaFile, const CString &recordName,
                                                std::unordered_set<CString> &markSet)
{
    uint32_t length = exportTable->GetLength();
    // the exportTable is arranged as follows ["A", "101", "B", "102"]
    // get GT of a export type specified by its descriptor/name
    for (uint32_t i = 0; i < length; i = i + 2) {  // 2: symbol and symbolType
        EcmaString *valueString = EcmaString::Cast(exportTable->Get(i).GetTaggedObject());
        if (EcmaStringAccessor::StringsAreEqual(*target, valueString)) {
            // Transform raw data of JSTaggedValue to GT
            return GlobalTSTypeRef(exportTable->Get(i + 1).GetInt());
        }
    }
    // if we can't find the exportName in exportTable of this record, we will try to search in its starExportRecords.
    return IterateStarExport(target, jsPandaFile, recordName, markSet);
}

GlobalTSTypeRef TSTypeParser::IterateStarExport(JSHandle<EcmaString> target, const JSPandaFile *jsPandaFile,
                                                const CString &recordName, std::unordered_set<CString> &markSet)
{
    if (bcInfo_->HasStarExportToRecord(recordName)) {
        const auto &starRecords = bcInfo_->GetstarExportToRecord(recordName);
        markSet.insert(recordName);
        JSMutableHandle<TaggedArray> starTable(thread_, JSTaggedValue::Undefined());
        for (const auto &star : starRecords) {
            // use markSet to avoid circular import
            if (markSet.find(star) != markSet.end()) {
                continue;
            }
            uint32_t starModuleId = tableGenerator_.TryGetModuleId(star);
            if (UNLIKELY(!GlobalTSTypeRef::IsValidModuleId(starModuleId))) {
                continue;
            }
            JSHandle<TSTypeTable> table = tableGenerator_.GetOrGenerateTSTypeTable(jsPandaFile, star, starModuleId);
            starTable.Update(GenerateExportTableFromRecord(jsPandaFile, star, table));
            // the target name will be the same under retransmission.
            auto gt = GetExportGTByName(target, starTable, jsPandaFile, star, markSet);
            if (!gt.IsDefault()) {
                return gt;
            }
        }
    }
    return GlobalTSTypeRef::Default();
}

GlobalTSTypeRef TSTypeParser::ParseGenericsType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                TypeLiteralExtractor *typeLiteralExtractor)
{
    uint32_t typeId = typeLiteralExtractor->GetTypeOffset();
    JSHandle<JSTaggedValue> type = ParseNonImportType(jsPandaFile, recordName, typeLiteralExtractor);
    if (UNLIKELY(type->IsUndefined())) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }
    auto gt = tsManager_->AddTSTypeToGenericsTable(JSHandle<TSType>(type));
    return GetAndStoreGT(jsPandaFile, typeId, recordName, gt.GetModuleId(), gt.GetLocalId());
}

JSHandle<JSTaggedValue> TSTypeParser::ParseGenericsInstanceType(const JSPandaFile *jsPandaFile,
                                                                const CString &recordName,
                                                                TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::BUILTIN_INSTANCE ||
           typeLiteralExtractor->GetTypeKind() == TSTypeKind::GENERIC_INSTANCE);
    GlobalTSTypeRef genericsGT;
    if (IsGenericsArrayType(typeLiteralExtractor)) {
        genericsGT = tsManager_->CreateArrayType();
    } else {
        genericsGT = CreateGT(jsPandaFile, recordName, typeLiteralExtractor->GetIntValue(DEFAULT_INDEX));
    }
    JSHandle<JSTaggedValue> genericsType = tsManager_->GetTSType(genericsGT);
    std::vector<GlobalTSTypeRef> paras {};
    typeLiteralExtractor->EnumerateElements(NUM_GENERICS_PARA_INDEX,
        [this, &jsPandaFile, &recordName, &paras](const uint32_t literalValue) {
            auto gt = CreateGT(jsPandaFile, recordName, literalValue);
            paras.emplace_back(gt);
        });
    return InstantiateGenericsType(genericsType, paras);
}

JSHandle<JSTaggedValue> TSTypeParser::InstantiateGenericsType(const JSHandle<JSTaggedValue> &genericsType,
                                                              const std::vector<GlobalTSTypeRef> &paras)
{
    if (genericsType->IsTSClassType()) {
        JSHandle<TSClassType> classType = InstantiateClassGenericsType(JSHandle<TSClassType>(genericsType), paras);
        return JSHandle<JSTaggedValue>(classType);
    } else if (genericsType->IsTSFunctionType()) {
        JSHandle<TSFunctionType> funcType = InstantiateFuncGenericsType(JSHandle<TSFunctionType>(genericsType), paras);
        return JSHandle<JSTaggedValue>(funcType);
    } else if (genericsType->IsTSInterfaceType()) {
        JSHandle<TSInterfaceType> interfaceType =
            InstantiateInterfaceGenericsType(JSHandle<TSInterfaceType>(genericsType), paras);
        return JSHandle<JSTaggedValue>(interfaceType);
    } else if (genericsType->IsTSObjectType()) {
        JSHandle<TSObjectType> objectType = InstantiateObjGenericsType(JSHandle<TSObjectType>(genericsType), paras);
        return JSHandle<JSTaggedValue>(objectType);
    } else if (genericsType->IsTSArrayType()) {
        ASSERT(paras.size() == 1);
        JSHandle<TSArrayType> arrayType(genericsType);
        arrayType->SetElementGT(paras[0]);
        return JSHandle<JSTaggedValue>(arrayType);
    }
    LOG_COMPILER(DEBUG) << "Unsupport GenericsType Instantiate: "
                        << static_cast<uint32_t>(genericsType->GetTaggedObject()->GetClass()->GetObjectType());
    return genericsType;
}

JSHandle<TSFunctionType> TSTypeParser::InstantiateFuncGenericsType(const JSHandle<TSFunctionType> &genericsType,
                                                                   const std::vector<GlobalTSTypeRef> &paras)
{
    uint32_t length = genericsType->GetLength();
    JSHandle<TSFunctionType> functionType = factory_->NewTSFunctionType(length);
    functionType->SetBitField(genericsType->GetBitField());
    functionType->SetName(thread_, genericsType->GetName());
    GlobalTSTypeRef thisGT = genericsType->GetThisGT();
    functionType->SetThisGT(TryReplaceTypePara(thisGT, paras));
    GlobalTSTypeRef returnGT = genericsType->GetReturnGT();
    functionType->SetReturnGT(TryReplaceTypePara(returnGT, paras));

    JSHandle<TaggedArray> argumentTypes(thread_, functionType->GetParameterTypes());
    JSHandle<TaggedArray> parameterTypes(thread_, genericsType->GetParameterTypes());
    for (uint32_t i = 0; i < length; ++i) {
        GlobalTSTypeRef parameterGT(parameterTypes->Get(thread_, i).GetInt());
        GlobalTSTypeRef argumentGT = TryReplaceTypePara(parameterGT, paras);
        JSTaggedValue parameter(argumentGT.GetType());
        argumentTypes->Set(thread_, i, parameter);
    }
    functionType->SetParameterTypes(thread_, argumentTypes);
    return functionType;
}

JSHandle<TSClassType> TSTypeParser::InstantiateClassGenericsType(const JSHandle<TSClassType> &genericsType,
                                                                 const std::vector<GlobalTSTypeRef> &paras)
{
    JSHandle<TSClassType> classType = factory_->NewTSClassType();
    CopyClassName(genericsType, classType);
    classType->SetExtensionGT(genericsType->GetExtensionGT());
    classType->SetHasLinked(genericsType->GetHasLinked());

    JSHandle<TSObjectType> oldInstanceType(thread_, genericsType->GetInstanceType());
    JSHandle<TSObjectType> newInstanceType = InstantiateObjGenericsType(oldInstanceType, paras);
    classType->SetInstanceType(thread_, newInstanceType);

    JSHandle<TSObjectType> oldPrototypeType(thread_, genericsType->GetPrototypeType());
    JSHandle<TSObjectType> newPrototypeType = InstantiateObjGenericsType(oldPrototypeType, paras);
    classType->SetPrototypeType(thread_, newPrototypeType);

    JSHandle<TSObjectType> oldConstructorType(thread_, genericsType->GetConstructorType());
    JSHandle<TSObjectType> newConstructorType = InstantiateObjGenericsType(oldConstructorType, paras);
    classType->SetConstructorType(thread_, newConstructorType);
    return classType;
}

void TSTypeParser::CopyClassName(const JSHandle<TSClassType> &genericsType, const JSHandle<TSClassType> &classType)
{
    auto gt = genericsType->GetGT();
    if (gt.IsBuiltinModule()) {
        const std::string name = tsManager_->GetBuiltinsName(gt);
        JSHandle<EcmaString> ecmaStr = factory_->NewFromStdString(name);
        classType->SetName(thread_, ecmaStr);
    } else {
        classType->SetName(thread_, genericsType->GetName());
    }
}

JSHandle<TSInterfaceType> TSTypeParser::InstantiateInterfaceGenericsType(const JSHandle<TSInterfaceType> &genericsType,
                                                                         const std::vector<GlobalTSTypeRef> &paras)
{
    JSHandle<TSInterfaceType> interfaceType = factory_->NewTSInterfaceType();
    JSHandle<TaggedArray> oldExtends(thread_, genericsType->GetExtends());
    uint32_t numExtends = oldExtends->GetLength();
    JSHandle<TaggedArray> extends = factory_->NewTaggedArray(numExtends);
    for (uint32_t i = 0; i < numExtends; ++i) {
        GlobalTSTypeRef parameterGT(oldExtends->Get(thread_, i).GetInt());
        JSTaggedValue parameter(TryReplaceTypePara(parameterGT, paras).GetType());
        extends->Set(thread_, i, parameter);
    }

    JSHandle<TSObjectType> fields(thread_, genericsType->GetFields());
    JSHandle<TSObjectType> newFields = InstantiateObjGenericsType(fields, paras);
    interfaceType->SetFields(thread_, newFields);
    return interfaceType;
}

JSHandle<TSObjectType> TSTypeParser::InstantiateObjGenericsType(const JSHandle<TSObjectType> &oldObjType,
                                                                const std::vector<GlobalTSTypeRef> &paras)
{
    JSHandle<TSObjLayoutInfo> oldLayout(thread_, oldObjType->GetObjLayoutInfo());
    uint32_t numOfProps = static_cast<uint32_t>(oldLayout->GetNumOfProperties());

    JSHandle<TSObjectType> newObjType = factory_->NewTSObjectType(numOfProps);
    JSHandle<TSObjLayoutInfo> newLayout(thread_, newObjType->GetObjLayoutInfo());
    ASSERT(newLayout->GetPropertiesCapacity() == numOfProps);
    for (uint32_t i = 0; i < numOfProps; ++i) {
        GlobalTSTypeRef parameterGT(oldLayout->GetTypeId(i).GetInt());
        JSTaggedValue parameter(TryReplaceTypePara(parameterGT, paras).GetType());
        newLayout->AddProperty(thread_, oldLayout->GetKey(i), parameter);
    }
    return newObjType;
}

GlobalTSTypeRef TSTypeParser::TryReplaceTypePara(GlobalTSTypeRef gt, const std::vector<GlobalTSTypeRef> &paras)
{
    // replace fields with types of templated
    if (IsGenericsParaType(gt)) {
        uint32_t paraTypeIndex = DecodePrarIndex(gt);
        if (paraTypeIndex < paras.size()) {
            return paras[paraTypeIndex];
        }
        return GlobalTSTypeRef::Default();
    }

    // replace methods with signature that contains template type parameters
    if (tsManager_->IsFunctionTypeKind(gt)) {
        JSHandle<JSTaggedValue> tsType = tsManager_->GetTSType(gt);
        ASSERT(tsType->IsTSFunctionType());
        JSHandle<TSFunctionType> funcTSType(tsType);
        JSHandle<TSFunctionType> funcInst = InstantiateFuncGenericsType(funcTSType, paras);
        return tsManager_->AddTSTypeToInferredTable(JSHandle<TSType>(funcInst));
    }
    return gt;
}

GlobalTSTypeRef TSTypeParser::CreatePGOGT(PGOInfo info)
{
    GlobalTypeID gId(info.jsPandaFile, info.pgoType);
    if (tsManager_->HasCreatedGT(gId)) {
        return tsManager_->GetGTByGlobalTypeID(gId);
    }
    return ParsePGOType(info);
}

GlobalTSTypeRef TSTypeParser::ParsePGOType(PGOInfo &info)
{
    uint32_t moduleId = tableGenerator_.TryGetModuleId(info.recordName);
    if (UNLIKELY(!GlobalTSTypeRef::IsValidModuleId(moduleId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypeTables is reached. All TSTypes in the record "
                            << info.recordName << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(info.jsPandaFile, info.pgoType);
    }

    JSHandle<TSTypeTable> table = tableGenerator_.GetOrGenerateTSTypeTable(info.jsPandaFile, info.recordName,
                                                                           moduleId);
    uint32_t localId = tableGenerator_.TryGetLocalId(table);
    if (UNLIKELY(!GlobalTSTypeRef::IsValidLocalId(localId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypes in TSTypeTable " << moduleId << " is reached. "
                            << "The objLiteral with constantpool index " << info.cpIdx << " in the record "
                            << info.recordName << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(info.jsPandaFile, info.pgoType);
    }

    table->SetNumberOfTypes(thread_, localId);
    GlobalTSTypeRef gt = GetAndStoreGT(info.jsPandaFile, info.pgoType, moduleId, localId);
    JSHandle<JSTaggedValue> parseType = ParseNonImportPGOType(gt, info);
    if (UNLIKELY(parseType->IsUndefined())) {
        return GetAndStoreGT(info.jsPandaFile, info.pgoType);
    }
    SetTSType(table, parseType, gt);
    return gt;
}

JSHandle<JSTaggedValue> TSTypeParser::ParseNonImportPGOType(GlobalTSTypeRef gt, PGOInfo &info)
{
    switch (info.type) {
        case kungfu::PGOBCInfo::Type::OBJ_LITERAL: {
            return ParseObjectPGOType(gt, info);
        }
        default:
            LOG_COMPILER(DEBUG) << "Do not support parse extend types with kind " << static_cast<uint32_t>(info.type);
            return thread_->GlobalConstants()->GetHandledUndefined();
    }
}

JSHandle<JSTaggedValue> TSTypeParser::ParseObjectPGOType(GlobalTSTypeRef gt, PGOInfo &info)
{
    JSHandle<ConstantPool> constpoolHandle(tsManager_->GetConstantPool());
    JSTaggedValue obj = ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
        thread_, constpoolHandle.GetTaggedValue(), info.cpIdx, info.recordName);
    JSHandle<JSObject> objHandle(thread_, obj);

    if (info.enableOptTrackField) {
        ASSERT(info.pgoType.IsClassType());
        PGOHClassLayoutDesc *desc;
        if (info.decoder->GetHClassLayoutDesc(info.pgoType, &desc)) {
            if (!VerifyObjIhcPGOType(objHandle, *desc)) {
                LOG_COMPILER(DEBUG) << "Verify ihc type failed";
                return thread_->GlobalConstants()->GetHandledUndefined();
            }
        }
    }

    JSHandle<JSHClass> oldHClass(thread_, objHandle->GetClass());
    if (oldHClass->IsDictionaryMode()) {
        return thread_->GlobalConstants()->GetHandledUndefined();
    }
    JSHandle<JSHClass> hclass = JSHClass::Clone(thread_, oldHClass);
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<LayoutInfo> newLayout = factory->CopyLayoutInfo(JSHandle<LayoutInfo>(thread_, hclass->GetLayout()));
    hclass->SetLayout(thread_, newLayout);

    hclass->SetTS(true);
    JSHandle<TSObjectType> objectType = factory_->NewTSObjectType(0);
    tsManager_->AddInstanceTSHClass(gt, hclass);
    return JSHandle<JSTaggedValue>(objectType);
}

bool TSTypeParser::VerifyObjIhcPGOType(JSHandle<JSObject> obj, const PGOHClassLayoutDesc &desc)
{
    auto hclass = obj->GetClass();
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    int numOfProps = hclass->NumberOfProps();
    for (int i = 0; i < numOfProps; i++) {
        auto key = layoutInfo->GetKey(i);
        if (!key.IsString()) {
            continue;
        }

        auto attr = layoutInfo->GetAttr(i);
        if (!attr.IsInlinedProps()) {
            continue;
        }
        JSTaggedValue value = obj->GetPropertyInlinedProps(i);

        auto keyString = EcmaStringAccessor(key).ToCString();
        PGOHandler newHandler;
        if (!desc.FindDescWithKey(keyString, newHandler)) {
            continue;
        }
        PropertyAttributes newAttr;
        if (!newHandler.SetAttribute(newAttr)) {
            continue;
        }
        if (newAttr.IsDoubleRep()) {
            if (!value.IsNumber()) {
                return false;
            }
        } else if (newAttr.IsIntRep()) {
            if (!value.IsInt()) {
                return false;
            }
        }
    }
    return true;
}

static uint32_t CalculateNextNumIndex(const TypeLiteralExtractor *typeLiteralExtractor,
                                      uint32_t startIndex = 0,
                                      uint32_t gap = 1)
{
    return startIndex + INDEX_OCCUPIED_OFFSET + (typeLiteralExtractor->GetIntValue(startIndex) * gap);
}

ClassLiteralInfo::ClassLiteralInfo(const TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::CLASS);
    numNonStaticFieldsIndex = CalculateNextNumIndex(typeLiteralExtractor, NUM_IMPLEMENTS_INDEX);
    numNonStaticMethodsIndex = CalculateNextNumIndex(typeLiteralExtractor, numNonStaticFieldsIndex, FIELD_LENGTH);
    numStaticFieldsIndex = CalculateNextNumIndex(typeLiteralExtractor, numNonStaticMethodsIndex, METHOD_LENGTH);
    numStaticMethodsIndex = CalculateNextNumIndex(typeLiteralExtractor, numStaticFieldsIndex, FIELD_LENGTH);
}

InterfaceLiteralInfo::InterfaceLiteralInfo(const TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::INTERFACE);
    numFieldsIndex = CalculateNextNumIndex(typeLiteralExtractor, NUM_EXTENDS_INDEX);
    numMethodsIndex = CalculateNextNumIndex(typeLiteralExtractor, numFieldsIndex, FIELD_LENGTH);
}

FunctionLiteralInfo::FunctionLiteralInfo(const TypeLiteralExtractor *typeLiteralExtractor)
{
    ASSERT(typeLiteralExtractor->GetTypeKind() == TSTypeKind::FUNCTION);
    numParasIndex = HAS_THIS_TYPE_INDEX + INDEX_OCCUPIED_OFFSET +
                    typeLiteralExtractor->GetIntValue(HAS_THIS_TYPE_INDEX);
    returnTypeIndex = CalculateNextNumIndex(typeLiteralExtractor, numParasIndex);
}
}  // namespace panda::ecmascript
