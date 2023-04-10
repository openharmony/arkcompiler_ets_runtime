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

#include "ecmascript/base/path_helper.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/module/js_module_manager.h"

#include "libpandafile/annotation_data_accessor.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
GlobalTSTypeRef TSTypeParser::CreateGT(const JSPandaFile *jsPandaFile, const CString &recordName, uint32_t typeId)
{
    if (typeId <= BUILDIN_TYPE_OFFSET) {
        return GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, typeId);
    }

    if (typeId <= USER_DEFINED_TYPE_OFFSET) {
        return ParseBuiltinObjType(typeId);
    }

    if (tsManager_->HasCreatedGT(jsPandaFile, typeId)) {
        return tsManager_->GetGTFromOffset(jsPandaFile, typeId);
    }
    return ParseType(jsPandaFile, recordName, typeId);
}

GlobalTSTypeRef TSTypeParser::ParseBuiltinObjType(uint32_t typeId)
{
    if (!tsManager_->IsBuiltinsDTSEnabled()) {
        return GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, typeId);
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
    panda_file::File::EntityId offset(typeId);
    JSHandle<TaggedArray> literal = LiteralDataExtractor::GetTypeLiteral(thread_, jsPandaFile, offset);
    if (literal->GetLength() == 0) {  // typeLiteral maybe hole in d.abc
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }
    TSTypeKind kind = static_cast<TSTypeKind>(literal->Get(TYPE_KIND_INDEX_IN_LITERAL).GetInt());
    if (kind == TSTypeKind::IMPORT) {
        return ResolveImportType(jsPandaFile, recordName, literal, typeId);
    }

    uint32_t moduleId = tableGenerator_.TryGetModuleId(recordName);
    if (UNLIKELY(!GlobalTSTypeRef::IsVaildModuleId(moduleId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypeTables is reached. All TSTypes in the record "
                            << recordName << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    JSHandle<TSTypeTable> table = tableGenerator_.GetOrGenerateTSTypeTable(jsPandaFile, recordName, moduleId);
    uint32_t localId = tableGenerator_.TryGetLocalId(table);
    if (UNLIKELY(!GlobalTSTypeRef::IsVaildLocalId(localId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypes in TSTypeTable " << moduleId << " is reached. "
                            << "The TSType with typeId " << typeId << " in the record " << recordName
                            << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    table->SetNumberOfTypes(thread_, localId);
    GlobalTSTypeRef gt = GetAndStoreGT(jsPandaFile, typeId, recordName, moduleId, localId);
    JSHandle<JSTaggedValue> type = ParseNonImportType(jsPandaFile, recordName, literal, kind, typeId);
    if (UNLIKELY(type->IsUndefined())) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    SetTSType(table, type, gt);
    tsManager_->CollectTypeOffsets(gt);  // collect types that need to generate hclasses
    return gt;
}

GlobalTSTypeRef TSTypeParser::ResolveImportType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                JSHandle<TaggedArray> literal, uint32_t typeId)
{
    JSHandle<EcmaString> importVarNamePath(thread_, literal->Get(IMPORT_PATH_OFFSET_IN_LITERAL)); // #A#./A
    JSHandle<EcmaString> relativePath = GenerateImportRelativePath(importVarNamePath);
    CString cstringRelativePath = ConvertToString(*relativePath);
    // skip @ohos:|@app:|@native: prefixed imports
    auto [isNative, _] = ModuleManager::CheckNativeModule(cstringRelativePath);
    if (isNative) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    CString baseFileName = jsPandaFile->GetJSPandaFileDesc();
    CString entryPoint =
        base::PathHelper::ConcatFileNameWithMerge(thread_, jsPandaFile, baseFileName, recordName, cstringRelativePath);
    ASSERT_PRINT(!entryPoint.empty(),
        "EntryPoint is empty. Please check whether concating file name is correct or "
        "whether the module request recorded in the import-type literal is correct.");
    // skip files without type information
    if (UNLIKELY(!jsPandaFile->HasTypeSummaryOffset(entryPoint))) {
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    uint32_t moduleId = tableGenerator_.TryGetModuleId(entryPoint);
    if (UNLIKELY(!GlobalTSTypeRef::IsVaildModuleId(moduleId))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypeTables is reached. All TSTypes in the recored "
                            << entryPoint << " will not be parsed and will be treated as any.";
        return GetAndStoreGT(jsPandaFile, typeId, recordName);
    }

    JSHandle<TSTypeTable> table = tableGenerator_.GetOrGenerateTSTypeTable(jsPandaFile, entryPoint, moduleId);
    JSHandle<JSTaggedValue> exportTable = GenerateExportTableFromRecord(jsPandaFile, entryPoint, table);
    JSHandle<TaggedArray> arrayWithGT(exportTable);
    JSHandle<EcmaString> targetVarName = GenerateImportVar(importVarNamePath);
    GlobalTSTypeRef importedGT = GetExportGTByName(targetVarName, arrayWithGT);
    return GetAndStoreImportGT(jsPandaFile, typeId, recordName, importedGT);
}

JSHandle<JSTaggedValue> TSTypeParser::ParseNonImportType(const JSPandaFile *jsPandaFile, const CString &recordName,
    JSHandle<TaggedArray> literal, TSTypeKind kind, uint32_t typeId)
{
    switch (kind) {
        case TSTypeKind::CLASS: {
            JSHandle<TSClassType> classType = ParseClassType(jsPandaFile, recordName, literal, typeId);
            return JSHandle<JSTaggedValue>(classType);
        }
        case TSTypeKind::CLASS_INSTANCE: {
            JSHandle<TSClassInstanceType> classInstanceType = ParseClassInstanceType(jsPandaFile, recordName, literal);
            return JSHandle<JSTaggedValue>(classInstanceType);
        }
        case TSTypeKind::INTERFACE_KIND: {
            JSHandle<TSInterfaceType> interfaceType = ParseInterfaceType(jsPandaFile, recordName, literal);
            return JSHandle<JSTaggedValue>(interfaceType);
        }
        case TSTypeKind::UNION: {
            JSHandle<TSUnionType> unionType = ParseUnionType(jsPandaFile, recordName, literal);
            return JSHandle<JSTaggedValue>(unionType);
        }
        case TSTypeKind::FUNCTION: {
            JSHandle<TSFunctionType> functionType = ParseFunctionType(jsPandaFile, recordName, literal);
            return JSHandle<JSTaggedValue>(functionType);
        }
        case TSTypeKind::ARRAY: {
            JSHandle<TSArrayType> arrayType = ParseArrayType(jsPandaFile, recordName, literal);
            return JSHandle<JSTaggedValue>(arrayType);
        }
        case TSTypeKind::OBJECT: {
            JSHandle<TSObjectType> objectType = ParseObjectType(jsPandaFile, recordName, literal);
            return JSHandle<JSTaggedValue>(objectType);
        }
        default: {
            LOG_COMPILER(DEBUG) << "Do not support parse types with kind " << static_cast<uint32_t>(kind) << ". "
                                << "Please check whether the type literal " << typeId
                                << " recorded in the record " << recordName << " is correct.";
            return thread_->GlobalConstants()->GetHandledUndefined();
        }
    }
}

JSHandle<TSClassType> TSTypeParser::ParseClassType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                   const JSHandle<TaggedArray> &literal, uint32_t typeId)
{
    JSHandle<TSClassType> classType = factory_->NewTSClassType();

    std::string className = tsManager_->GetClassNameByOffset(jsPandaFile, typeId);
    JSHandle<EcmaString> classEcmaString = factory_->NewFromStdString(className);
    classType->SetName(thread_, classEcmaString.GetTaggedValue());

    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::CLASS);

    const uint32_t ignoreLength = 2;  // 2: ignore accessFlag and readonly
    index += ignoreLength;
    int extendsTypeId = literal->Get(index++).GetInt();
    if (TSClassType::IsBaseClassType(extendsTypeId)) {
        classType->SetHasLinked(true);
    } else {
        auto extensionGT = CreateGT(jsPandaFile, recordName, extendsTypeId);
        classType->SetExtensionGT(extensionGT);
        if (extensionGT == GlobalTSTypeRef::Default()) {
            classType->SetHasLinked(true);
        }
    }

    // ignore implement
    uint32_t numImplement = literal->Get(index++).GetInt();
    index += numImplement;

    // resolve instance type
    uint32_t numFields = static_cast<uint32_t>(literal->Get(index++).GetInt());

    JSHandle<TSObjectType> instanceType = factory_->NewTSObjectType(numFields);
    JSHandle<TSObjLayoutInfo> instanceTypeInfo(thread_, instanceType->GetObjLayoutInfo());
    ASSERT(instanceTypeInfo->GetPropertiesCapacity() == numFields);
    FillPropertyTypes(jsPandaFile, recordName, instanceTypeInfo, literal, 0, numFields, index, true);
    classType->SetInstanceType(thread_, instanceType);

    // resolve prototype type
    uint32_t numNonStatic = literal->Get(index++).GetInt();
    JSHandle<TSObjectType> prototypeType = factory_->NewTSObjectType(numNonStatic);

    JSHandle<TSObjLayoutInfo> nonStaticTypeInfo(thread_, prototypeType->GetObjLayoutInfo());
    ASSERT(nonStaticTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numNonStatic));
    FillPropertyTypes(jsPandaFile, recordName, nonStaticTypeInfo, literal, 0, numNonStatic, index, false);
    classType->SetPrototypeType(thread_, prototypeType);

    // resolve constructor type
    // static include fields and methods, which the former takes up 4 spaces and the latter takes up 2 spaces.
    uint32_t numStaticFields = literal->Get(index++).GetInt();
    uint32_t numStaticMethods = literal->Get(index + numStaticFields * TSClassType::FIELD_LENGTH).GetInt();
    uint32_t numStatic = numStaticFields + numStaticMethods;
    // new function type when support it
    JSHandle<TSObjectType> constructorType = factory_->NewTSObjectType(numStatic);

    JSHandle<TSObjLayoutInfo> staticTypeInfo(thread_, constructorType->GetObjLayoutInfo());
    ASSERT(staticTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numStatic));
    FillPropertyTypes(jsPandaFile, recordName, staticTypeInfo, literal, 0, numStaticFields, index, true);
    index++;  // jmp over numStaticMethods
    // static methods
    FillPropertyTypes(jsPandaFile, recordName, staticTypeInfo, literal, numStaticFields, numStatic, index, false);
    classType->SetConstructorType(thread_, constructorType);
    return classType;
}

JSHandle<TSClassInstanceType> TSTypeParser::ParseClassInstanceType(const JSPandaFile *jsPandaFile,
                                                                   const CString &recordName,
                                                                   const JSHandle<TaggedArray> &literal)
{
    ASSERT(static_cast<TSTypeKind>(literal->Get(TYPE_KIND_INDEX_IN_LITERAL).GetInt()) ==
                                   TSTypeKind::CLASS_INSTANCE);
    JSHandle<TSClassInstanceType> classInstanceType = factory_->NewTSClassInstanceType();
    uint32_t classTypeId = static_cast<uint32_t>(literal->Get(TSClassInstanceType::CREATE_CLASS_OFFSET).GetInt());
    auto classGT = CreateGT(jsPandaFile, recordName, classTypeId);
    classInstanceType->SetClassGT(classGT);
    return classInstanceType;
}

JSHandle<TSInterfaceType> TSTypeParser::ParseInterfaceType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                           const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    JSHandle<TSInterfaceType> interfaceType = factory_->NewTSInterfaceType();
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::INTERFACE_KIND);

    index++;
    // resolve extends of interface
    uint32_t numExtends = literal->Get(index++).GetInt();
    JSHandle<TaggedArray> extendsId = factory_->NewTaggedArray(numExtends);
    JSMutableHandle<JSTaggedValue> extendsType(thread_, JSTaggedValue::Undefined());
    for (uint32_t extendsIndex = 0; extendsIndex < numExtends; extendsIndex++) {
        auto typeId = literal->Get(index++).GetInt();
        auto extendGT = CreateGT(jsPandaFile, recordName, typeId);
        extendsType.Update(JSTaggedValue(extendGT.GetType()));
        extendsId->Set(thread_, extendsIndex, extendsType);
    }
    interfaceType->SetExtends(thread_, extendsId);

    // resolve fields and methods of interface
    uint32_t numFields = literal->Get(index++).GetInt();
    // field takes up 4 spaces and method takes up 2 spaces.
    uint32_t numMethods =
        static_cast<uint32_t>(literal->Get(index + numFields * TSInterfaceType::FIELD_LENGTH).GetInt());
    uint32_t totalFields = numFields + numMethods;

    JSHandle<TSObjectType> fieldsType = factory_->NewTSObjectType(totalFields);
    JSHandle<TSObjLayoutInfo> fieldsTypeInfo(thread_, fieldsType->GetObjLayoutInfo());
    ASSERT(fieldsTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(totalFields));
    FillPropertyTypes(jsPandaFile, recordName, fieldsTypeInfo, literal, 0, numFields, index, true);
    index++;  // jmp over numMethod
    FillInterfaceMethodTypes(jsPandaFile, recordName, fieldsTypeInfo, literal, numFields, totalFields, index);
    interfaceType->SetFields(thread_, fieldsType);
    return interfaceType;
}

JSHandle<TSUnionType> TSTypeParser::ParseUnionType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                   const JSHandle<TaggedArray> &literal)
{
    uint32_t literalIndex = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(literalIndex).GetInt()) == TSTypeKind::UNION);
    literalIndex++;
    uint32_t numOfUnionMembers = literal->Get(literalIndex++).GetInt();

    JSHandle<TSUnionType> unionType = factory_->NewTSUnionType(numOfUnionMembers);
    JSHandle<TaggedArray> components(thread_, unionType->GetComponents());
    for (uint32_t index = 0; index < numOfUnionMembers; ++index) {
        uint32_t componentTypeId = literal->Get(literalIndex++).GetInt();
        auto componentGT = CreateGT(jsPandaFile, recordName, componentTypeId);
        components->Set(thread_, index, JSTaggedValue(componentGT.GetType()));
    }
    unionType->SetComponents(thread_, components);
    return unionType;
}

JSHandle<TSFunctionType> TSTypeParser::ParseFunctionType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                         const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::FUNCTION);
    index++;

    int32_t bitField = literal->Get(index++).GetInt();

    JSHandle<JSTaggedValue> functionName(thread_, literal->Get(index++));
    bool hasThisType = static_cast<bool>(literal->Get(index++).GetInt());
    int32_t thisTypeId = 0;
    if (hasThisType) {
        thisTypeId = literal->Get(index++).GetInt();
    }

    int32_t length = literal->Get(index++).GetInt();
    JSHandle<TSFunctionType> functionType = factory_->NewTSFunctionType(length);
    JSHandle<TaggedArray> parameterTypes(thread_, functionType->GetParameterTypes());
    JSMutableHandle<JSTaggedValue> parameterTypeRef(thread_, JSTaggedValue::Undefined());
    for (int32_t i = 0; i < length; ++i) {
        auto typeId = literal->Get(index++).GetInt();
        auto parameterGT = CreateGT(jsPandaFile, recordName, typeId);
        if (tsManager_->IsClassTypeKind(parameterGT)) {
            parameterGT = tsManager_->CreateClassInstanceType(parameterGT);
        }
        parameterTypeRef.Update(JSTaggedValue(parameterGT.GetType()));
        parameterTypes->Set(thread_, i, parameterTypeRef);
    }
    int32_t returntypeId = literal->Get(index++).GetInt();

    functionType->SetName(thread_, functionName);
    if (hasThisType) {
        auto thisGT = CreateGT(jsPandaFile, recordName, thisTypeId);
        functionType->SetThisGT(thisGT);
    }

    functionType->SetParameterTypes(thread_, parameterTypes);
    auto returnGT = CreateGT(jsPandaFile, recordName, returntypeId);
    functionType->SetReturnGT(returnGT);
    functionType->SetBitField(bitField);

    return functionType;
}

JSHandle<TSArrayType> TSTypeParser::ParseArrayType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                   const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::ARRAY);
    index++;
    JSHandle<JSTaggedValue> elementTypeId(thread_, literal->Get(index++));
    ASSERT(elementTypeId->IsInt());
    JSHandle<TSArrayType> arrayType = factory_->NewTSArrayType();
    auto elemetnGT = CreateGT(jsPandaFile, recordName, elementTypeId->GetInt());
    if (tsManager_->IsClassTypeKind(elemetnGT)) {
        elemetnGT = tsManager_->CreateClassInstanceType(elemetnGT);
    }
    arrayType->SetElementGT(elemetnGT);
    return arrayType;
}

JSHandle<TSObjectType> TSTypeParser::ParseObjectType(const JSPandaFile *jsPandaFile, const CString &recordName,
                                                     const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::OBJECT);
    index++;
    uint32_t length = literal->Get(index++).GetInt();
    JSHandle<TSObjectType> objectType = factory_->NewTSObjectType(length);
    JSHandle<TSObjLayoutInfo> propertyTypeInfo(thread_, objectType->GetObjLayoutInfo());
    ASSERT(propertyTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(length));
    FillPropertyTypes(jsPandaFile, recordName, propertyTypeInfo, literal, 0, length, index, false);
    objectType->SetObjLayoutInfo(thread_, propertyTypeInfo);
    return objectType;
}

void TSTypeParser::FillPropertyTypes(const JSPandaFile *jsPandaFile, const CString &recordName,
                                     JSHandle<TSObjLayoutInfo> &layout, const JSHandle<TaggedArray> &literal,
                                     uint32_t startIndex, uint32_t lastIndex, uint32_t &index, bool isField)
{
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread_, JSTaggedValue::Undefined());
    for (uint32_t fieldIndex = startIndex; fieldIndex < lastIndex; ++fieldIndex) {
        key.Update(literal->Get(index++));
        ASSERT(key->IsString());
        auto gt = CreateGT(jsPandaFile, recordName, literal->Get(index++).GetInt());
        if (tsManager_->IsClassTypeKind(gt)) {
            gt = tsManager_->CreateClassInstanceType(gt);
        }
        value.Update(JSTaggedValue(gt.GetType()));
        layout->AddKeyAndType(thread_, key.GetTaggedValue(), value.GetTaggedValue());
        if (isField) {
            index += 2;  // 2: ignore accessFlag and readonly
        }
    }
}

void TSTypeParser::FillInterfaceMethodTypes(const JSPandaFile *jsPandaFile, const CString &recordName,
                                            JSHandle<TSObjLayoutInfo> &layout, const JSHandle<TaggedArray> &literal,
                                            uint32_t startIndex, uint32_t lastIndex, uint32_t &index)
{
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread_, JSTaggedValue::Undefined());
    for (uint32_t methodIndex = startIndex; methodIndex < lastIndex; ++methodIndex) {
        auto gt = CreateGT(jsPandaFile, recordName, literal->Get(index++).GetInt());
        value.Update(JSTaggedValue(gt.GetType()));

        if (tsManager_->IsFunctionTypeKind(gt)) {
            JSHandle<JSTaggedValue> tsType = tsManager_->GetTSType(gt);
            ASSERT(tsType->IsTSFunctionType());

            JSHandle<TSFunctionType> functionType(tsType);
            key.Update(functionType->GetName());
        };

        layout->AddKeyAndType(thread_, key.GetTaggedValue(), value.GetTaggedValue());
    }
}

JSHandle<TaggedArray> TSTypeParser::GetExportDataFromRecord(const JSPandaFile *jsPandaFile,
                                                            const CString &recordName)
{
    const panda_file::File &pf = *jsPandaFile->GetPandaFile();
    panda_file::File::EntityId methodId(jsPandaFile->GetMainMethodIndex(recordName));
    panda_file::MethodDataAccessor mda(pf, methodId);

    const char *symbolTypes;
    auto *fileName = pf.GetFilename().c_str();
    if (::strcmp(TSTypeTable::BUILTINS_TABLE_NAME, fileName) == 0) {
        symbolTypes = DECLARED_SYMBOL_TYPES;
    } else {
        symbolTypes = EXPORTED_SYMBOL_TYPES;
    }

    JSHandle<TaggedArray> typeOfExportedSymbols(thread_, thread_->GlobalConstants()->GetEmptyArray());
    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(pf, annotation_id);
        auto *annotationName = reinterpret_cast<const char *>(pf.GetStringData(ada.GetClassId()).data);
        ASSERT(annotationName != nullptr);
        if (::strcmp("L_ESTypeAnnotation;", annotationName) != 0) {
            return;
        }
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elemName = reinterpret_cast<const char *>(pf.GetStringData(adae.GetNameId()).data);
            ASSERT(elemName != nullptr);

            if (::strcmp(symbolTypes, elemName) != 0) {
                continue;
            }

            panda_file::ScalarValue sv = adae.GetScalarValue();
            panda_file::File::EntityId literalOffset(sv.GetValue());
            typeOfExportedSymbols = LiteralDataExtractor::GetTypeLiteral(thread_, jsPandaFile, literalOffset);
            // typeOfExprotedSymbols: "symbol0", "type0", "symbol1", "type1" ...
        }
    });

    return typeOfExportedSymbols;
}

JSHandle<JSTaggedValue> TSTypeParser::GenerateExportTableFromRecord(const JSPandaFile *jsPandaFile,
                                                                    const CString &recordName,
                                                                    const JSHandle<TSTypeTable> &table)
{
    JSHandle<JSTaggedValue> exportValeTable = TSTypeTable::GetExportValueTable(thread_, table);
    if (exportValeTable->IsUndefined()) {
        // Read export-data from annotation of the .abc File
        JSHandle<TaggedArray> exportTable = GetExportDataFromRecord(jsPandaFile, recordName);
        uint32_t length = exportTable->GetLength();

        // Replace typeIds with GT
        JSTaggedValue target;
        for (uint32_t i = 1; i < length; i += 2) {  // 2: skip a pair of key and value
            target = exportTable->Get(i);
            // Create GT based on typeId, and wrapped it into a JSTaggedValue
            uint32_t typeId = static_cast<uint32_t>(target.GetInt());
            GlobalTSTypeRef typeGT = CreateGT(jsPandaFile, recordName, typeId);
            // Set the wrapped GT to exportTable
            exportTable->Set(thread_, i, JSTaggedValue(typeGT.GetType()));
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

GlobalTSTypeRef TSTypeParser::GetExportGTByName(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const
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
    return GlobalTSTypeRef::Default();
}
}  // namespace panda::ecmascript
