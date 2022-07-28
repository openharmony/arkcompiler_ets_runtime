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

namespace panda::ecmascript {
JSHandle<JSTaggedValue> TSTypeParser::ParseType(JSHandle<TaggedArray> &literal)
{
    TSTypeKind kind = static_cast<TSTypeKind>(literal->Get(TYPE_KIND_INDEX_IN_LITERAL).GetInt());
    switch (kind) {
        case TSTypeKind::CLASS: {
            JSHandle<TSClassType> classType = ParseClassType(literal);
            return JSHandle<JSTaggedValue>(classType);
        }
        case TSTypeKind::CLASS_INSTANCE: {
            JSHandle<TSClassInstanceType> classInstanceType = ParseClassInstanceType(literal);
            return JSHandle<JSTaggedValue>(classInstanceType);
        }
        case TSTypeKind::INTERFACE_KIND: {
            JSHandle<TSInterfaceType> interfaceType = ParseInterfaceType(literal);
            return JSHandle<JSTaggedValue>(interfaceType);
        }
        case TSTypeKind::IMPORT: {
            JSHandle<TSImportType> importType = ParseImportType(literal);
            return JSHandle<JSTaggedValue>(importType);
        }
        case TSTypeKind::UNION: {
            JSHandle<TSUnionType> unionType = ParseUnionType(literal);
            return JSHandle<JSTaggedValue>(unionType);
        }
        case TSTypeKind::FUNCTION: {
            JSHandle<TSFunctionType> functionType = ParseFunctionType(literal);
            return JSHandle<JSTaggedValue>(functionType);
        }
        case TSTypeKind::ARRAY: {
            JSHandle<TSArrayType> arrayType = ParseArrayType(literal);
            return JSHandle<JSTaggedValue>(arrayType);
        }
        case TSTypeKind::OBJECT: {
            JSHandle<TSObjectType> objectType = ParseObjectType(literal);
            return JSHandle<JSTaggedValue>(objectType);
        }
        default:
            UNREACHABLE();
    }
    // not support type yet
    return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Null());
}

void TSTypeParser::SetTypeRef(JSHandle<JSTaggedValue> type, uint32_t loaclId)
{
    if (!type->IsNull()) {
        JSHandle<TSType>(type)->SetGT(CreateGT(moduleId_, loaclId));
    }
}

JSHandle<TSClassType> TSTypeParser::ParseClassType(const JSHandle<TaggedArray> &literal)
{
    JSHandle<TSClassType> classType = factory_->NewTSClassType();
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::CLASS);

    const uint32_t ignoreLength = 2;  // 2: ignore accessFlag and readonly
    index += ignoreLength;
    int extendsTypeId = literal->Get(index++).GetInt();
    if (TSClassType::IsBaseClassType(extendsTypeId)) {
        classType->SetHasLinked(true);
    } else {
        classType->SetExtensionGT(CreateGT(moduleId_, extendsTypeId));
    }

    // ignore implement
    uint32_t numImplement = literal->Get(index++).GetInt();
    index += numImplement;

    // resolve instance type
    uint32_t numFields = static_cast<uint32_t>(literal->Get(index++).GetInt());

    JSHandle<TSObjectType> instanceType = factory_->NewTSObjectType(numFields);
    JSHandle<TSObjLayoutInfo> instanceTypeInfo(thread_, instanceType->GetObjLayoutInfo());
    ASSERT(instanceTypeInfo->GetPropertiesCapacity() == numFields);
    FillPropertyTypes(instanceTypeInfo, literal, 0, numFields, index, true);
    classType->SetInstanceType(thread_, instanceType);

    // resolve prototype type
    uint32_t numNonStatic = literal->Get(index++).GetInt();
    JSHandle<TSObjectType> prototypeType = factory_->NewTSObjectType(numNonStatic);

    JSHandle<TSObjLayoutInfo> nonStaticTypeInfo(thread_, prototypeType->GetObjLayoutInfo());
    ASSERT(nonStaticTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numNonStatic));
    FillPropertyTypes(nonStaticTypeInfo, literal, 0, numNonStatic, index, false);
    classType->SetPrototypeType(thread_, prototypeType);

    // resolve constructor type
    // stitic include fields and methods, which the former takes up 4 spaces and the latter takes up 2 spaces.
    uint32_t numStaticFields = literal->Get(index++).GetInt();
    uint32_t numStaticMethods = literal->Get(index + numStaticFields * TSClassType::FIELD_LENGTH).GetInt();
    uint32_t numStatic = numStaticFields + numStaticMethods;
    // new function type when support it
    JSHandle<TSObjectType> constructorType = factory_->NewTSObjectType(numStatic);

    JSHandle<TSObjLayoutInfo> staticTypeInfo(thread_, constructorType->GetObjLayoutInfo());
    ASSERT(staticTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numStatic));
    FillPropertyTypes(staticTypeInfo, literal, 0, numStaticFields, index, true);
    index++;  // jmp over numStaticMethods
    // static methods
    FillPropertyTypes(staticTypeInfo, literal, numStaticFields, numStatic, index, false);
    classType->SetConstructorType(thread_, constructorType);
    return classType;
}

JSHandle<TSClassInstanceType> TSTypeParser::ParseClassInstanceType(const JSHandle<TaggedArray> &literal)
{
    JSHandle<TSClassInstanceType> classInstanceType = factory_->NewTSClassInstanceType();
    uint32_t classIndex = literal->Get(TSClassInstanceType::CREATE_CLASS_OFFSET).GetInt();
    classInstanceType->SetClassGT(CreateGT(moduleId_, classIndex));
    return classInstanceType;
}

JSHandle<TSInterfaceType> TSTypeParser::ParseInterfaceType(const JSHandle<TaggedArray> &literal)
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
        extendsType.Update(literal->Get(index++));
        extendsId->Set(thread_, extendsIndex, extendsType);
    }
    interfaceType->SetExtends(thread_, extendsId);

    // resolve fields of interface
    uint32_t numFields = literal->Get(index++).GetInt();

    JSHandle<TSObjectType> fieldsType = factory_->NewTSObjectType(numFields);
    JSHandle<TSObjLayoutInfo> fieldsTypeInfo(thread_, fieldsType->GetObjLayoutInfo());
    ASSERT(fieldsTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numFields));
    FillPropertyTypes(fieldsTypeInfo, literal, 0, numFields, index, true);
    interfaceType->SetFields(thread_, fieldsType);
    return interfaceType;
}

JSHandle<TSImportType> TSTypeParser::ParseImportType(const JSHandle<TaggedArray> &literal)
{
    JSHandle<EcmaString> importVarNamePath(thread_,
                                           literal->Get(TSImportType::IMPORT_PATH_OFFSET_IN_LITERAL)); // #A#./A
    JSHandle<EcmaString> targetAndPathEcmaStr = TSTypeTable::GenerateVarNameAndPath(thread_, importVarNamePath,
                                                                                    fileName_, recordImportModules_);
    JSHandle<TSImportType> importType = factory_->NewTSImportType();
    importType->SetImportPath(thread_, targetAndPathEcmaStr);
    return importType;
}

JSHandle<TSUnionType> TSTypeParser::ParseUnionType(const JSHandle<TaggedArray> &literal)
{
    uint32_t literalIndex = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(literalIndex).GetInt()) == TSTypeKind::UNION);
    literalIndex++;
    uint32_t numOfUnionMembers = literal->Get(literalIndex++).GetInt();

    JSHandle<TSUnionType> unionType = factory_->NewTSUnionType(numOfUnionMembers);
    JSHandle<TaggedArray> components(thread_, unionType->GetComponents());
    for (uint32_t index = 0; index < numOfUnionMembers; ++index) {
        uint32_t componentTypeId = literal->Get(literalIndex++).GetInt();
        components->Set(thread_, index, JSTaggedValue(CreateGT(moduleId_, componentTypeId).GetType()));
    }
    unionType->SetComponents(thread_, components);
    return unionType;
}

JSHandle<TSFunctionType> TSTypeParser::ParseFunctionType(const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::FUNCTION);
    index++;

    index += TSFunctionType::FIELD_LENGTH;
    JSHandle<JSTaggedValue> functionName(thread_, literal->Get(index++));

    uint32_t length = 0;
    length = static_cast<uint32_t>(literal->Get(index++).GetInt());
    JSHandle<TSFunctionType> functionType = factory_->NewTSFunctionType(length);
    JSHandle<TaggedArray> parameterTypes(thread_, functionType->GetParameterTypes());
    JSMutableHandle<JSTaggedValue> parameterTypeRef(thread_, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; ++i) {
        auto typeId = literal->Get(index++).GetInt();
        parameterTypeRef.Update(JSTaggedValue(CreateGT(moduleId_, typeId).GetType()));
        parameterTypes->Set(thread_, i + TSFunctionType::PARAMETER_START_ENTRY, parameterTypeRef);
    }

    auto returntypeId = literal->Get(index++);
    auto returngt = CreateGT(moduleId_, returntypeId.GetInt());
    JSHandle<JSTaggedValue> returnValueTypeRef = JSHandle<JSTaggedValue>(thread_, JSTaggedValue(returngt.GetType()));
    parameterTypes->Set(thread_, TSFunctionType::FUNCTION_NAME_OFFSET, functionName);
    parameterTypes->Set(thread_, TSFunctionType::RETURN_VALUE_TYPE_OFFSET, returnValueTypeRef);
    functionType->SetParameterTypes(thread_, parameterTypes);
    return functionType;
}

JSHandle<TSArrayType> TSTypeParser::ParseArrayType(const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::ARRAY);
    index++;
    JSHandle<JSTaggedValue> elementTypeId(thread_, literal->Get(index++));
    ASSERT(elementTypeId->IsInt());
    JSHandle<TSArrayType> arrayType = factory_->NewTSArrayType();
    arrayType->SetElementGT(CreateGT(moduleId_, elementTypeId->GetInt()));
    return arrayType;
}

JSHandle<TSObjectType> TSTypeParser::ParseObjectType(const JSHandle<TaggedArray> &literal)
{
    uint32_t index = 0;
    ASSERT(static_cast<TSTypeKind>(literal->Get(index).GetInt()) == TSTypeKind::OBJECT);
    index++;
    uint32_t length = literal->Get(index++).GetInt();
    JSHandle<TSObjectType> objectType = factory_->NewTSObjectType(length);
    JSHandle<TSObjLayoutInfo> propertyTypeInfo(thread_, objectType->GetObjLayoutInfo());
    ASSERT(propertyTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(length));
    FillPropertyTypes(propertyTypeInfo, literal, 0, length, index, false);
    objectType->SetObjLayoutInfo(thread_, propertyTypeInfo);
    return objectType;
}

void TSTypeParser::FillPropertyTypes(JSHandle<TSObjLayoutInfo> &layOut, const JSHandle<TaggedArray> &literal,
                                     uint32_t startIndex, uint32_t lastIndex, uint32_t &index, bool isField)
{
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread_, JSTaggedValue::Undefined());
    for (uint32_t fieldIndex = startIndex; fieldIndex < lastIndex; ++fieldIndex) {
        key.Update(literal->Get(index++));
        ASSERT(key->IsString());
        auto gt = CreateGT(moduleId_, literal->Get(index++).GetInt());
        value.Update(JSTaggedValue(gt.GetType()));
        layOut->SetKey(thread_, fieldIndex, key.GetTaggedValue(), value.GetTaggedValue());
        if (isField) {
        index += 2;  // 2: ignore accessFlag and readonly
        }
    }
}
}  // namespace panda::ecmascript
