/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"

namespace panda::ecmascript {
enum class TSObjectTypeKind: uint8_t {
    INSTANCE = 0,
    PROTOTYPE,
};

#define ACCESSORS_ATTACHED_TYPEREF(name, offset, endOffset)               \
    ACCESSORS_PRIMITIVE_FIELD(name##RawData, uint32_t, offset, endOffset) \
    inline void Set##name(GlobalTSTypeRef type)                           \
    {                                                                     \
        Set##name##RawData(type.GetType());                               \
    }                                                                     \
    inline GlobalTSTypeRef Get##name() const                              \
    {                                                                     \
        return GlobalTSTypeRef(Get##name##RawData());                     \
    }

class TSType : public TaggedObject {
public:
    static constexpr size_t GT_OFFSET = TaggedObjectSize();

    inline static TSType *Cast(const TaggedObject *object)
    {
        return static_cast<TSType *>(const_cast<TaggedObject *>(object));
    }

    ACCESSORS_ATTACHED_TYPEREF(GT, GT_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
};

class TSObjectType : public TSType {
public:
    CAST_CHECK(TSObjectType, IsTSObjectType);

    static constexpr size_t PROPERTIES_OFFSET = TSType::SIZE;

    static JSHClass *GetOrCreateHClass(JSThread *thread, JSHandle<TSObjectType> objectType, TSObjectTypeKind kind);

    static GlobalTSTypeRef GetPropTypeGT(JSHandle<TSObjectType> objectType, JSHandle<EcmaString> propName);

    ACCESSORS(ObjLayoutInfo, PROPERTIES_OFFSET, HCLASS_OFFSET);
    ACCESSORS(HClass, HCLASS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(PROPERTIES_OFFSET, SIZE)
    DECL_DUMP()

private:
    JSHClass *CreateHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const;

    JSHClass *CreatePrototypeHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const;
};

class TSClassType : public TSType {
public:
    CAST_CHECK(TSClassType, IsTSClassType);

    static constexpr size_t FIELD_LENGTH = 4;  // every field record name, typeIndex, accessFlag, readonly
    static constexpr size_t INSTANCE_TYPE_OFFSET = TSType::SIZE;

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                         JSHandle<EcmaString> propName);

    static GlobalTSTypeRef GetSuperPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                              JSHandle<EcmaString> propName, PropertyType propType);

    static GlobalTSTypeRef GetNonStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                                  JSHandle<EcmaString> propName);

    ACCESSORS(InstanceType, INSTANCE_TYPE_OFFSET, CONSTRUCTOR_TYPE_OFFSET);
    ACCESSORS(ConstructorType, CONSTRUCTOR_TYPE_OFFSET, PROTOTYPE_TYPE_OFFSET);
    ACCESSORS(PrototypeType, PROTOTYPE_TYPE_OFFSET, NAME_OFFSET);
    ACCESSORS(Name, NAME_OFFSET, EXTENSION_GT_OFFSET);
    ACCESSORS_ATTACHED_TYPEREF(ExtensionGT, EXTENSION_GT_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t HAS_LINKED_BITS = 1;
    FIRST_BIT_FIELD(BitField, HasLinked, bool, HAS_LINKED_BITS);

    DECL_VISIT_OBJECT(INSTANCE_TYPE_OFFSET, EXTENSION_GT_OFFSET)
    DECL_DUMP()

    // Judgment base classType by extends typeId, ts2abc write 0 in base class type extends domain
    inline static bool IsBaseClassType(int extendsTypeId)
    {
        const int baseClassTypeExtendsTypeId = 0;
        return extendsTypeId == baseClassTypeExtendsTypeId;
    }
};

class TSClassInstanceType : public TSType {
public:
    CAST_CHECK(TSClassInstanceType, IsTSClassInstanceType);

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSClassInstanceType> classInstanceType,
                                         JSHandle<EcmaString> propName);

    static constexpr size_t CLASS_GT_OFFSET = TSType::SIZE;
    static constexpr size_t CREATE_CLASS_OFFSET = 1;
    ACCESSORS_ATTACHED_TYPEREF(ClassGT, CLASS_GT_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_DUMP()
};

class TSUnionType : public TSType {
public:
    CAST_CHECK(TSUnionType, IsTSUnionType);

    bool IsEqual(JSHandle<TSUnionType> unionB);

    static constexpr size_t COMPONENTS_OFFSET = TSType::SIZE;
    ACCESSORS(Components, COMPONENTS_OFFSET, SIZE);  // the gt collection of union type components

    DECL_VISIT_OBJECT(COMPONENTS_OFFSET, SIZE)
    DECL_DUMP()
};

class TSInterfaceType : public TSType {
public:
    CAST_CHECK(TSInterfaceType, IsTSInterfaceType);

    // every field record name, typeIndex, accessFlag, readonly
    static constexpr size_t FIELD_LENGTH = 4;

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSInterfaceType> classInstanceType,
                                         JSHandle<EcmaString> propName);

    static constexpr size_t EXTENDS_TYPE_ID_OFFSET = TSType::SIZE;
    ACCESSORS(Extends, EXTENDS_TYPE_ID_OFFSET, KEYS_OFFSET);
    ACCESSORS(Fields, KEYS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(EXTENDS_TYPE_ID_OFFSET, SIZE)
    DECL_DUMP()
};

class TSFunctionType : public TSType {
public:
    CAST_CHECK(TSFunctionType, IsTSFunctionType);

    uint32_t GetLength() const
    {
        TaggedArray* parameterTypes = TaggedArray::Cast(GetParameterTypes().GetTaggedObject());
        return parameterTypes->GetLength();
    }

    GlobalTSTypeRef GetParameterTypeGT(int index) const;

    static constexpr size_t NAME_OFFSET = TSType::SIZE;
    ACCESSORS(Name, NAME_OFFSET, PARAMETER_TYPES_OFFSET);
    ACCESSORS(ParameterTypes, PARAMETER_TYPES_OFFSET, RETURN_GT_OFFSET);
    ACCESSORS_ATTACHED_TYPEREF(ReturnGT, RETURN_GT_OFFSET, THIS_GT_OFFSET);
    ACCESSORS_ATTACHED_TYPEREF(ThisGT, THIS_GT_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, METHOD_OFFSET_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(MethodOffset, uint32_t, METHOD_OFFSET_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    enum class Visibility : uint8_t { PUBLIC = 0, PRIVATE, PROTECTED };

    // define BitField
    static constexpr size_t ONE_BIT = 1;
    static constexpr size_t TWO_BITS = 2;
    FIRST_BIT_FIELD(BitField, Visibility, Visibility, TWO_BITS);
    NEXT_BIT_FIELD(BitField, Static, bool, ONE_BIT, Visibility);
    NEXT_BIT_FIELD(BitField, Async, bool, ONE_BIT, Static);
    NEXT_BIT_FIELD(BitField, Generator, bool, ONE_BIT, Async);
    NEXT_BIT_FIELD(BitField, IsGetterSetter, bool, ONE_BIT, Generator);
    NEXT_BIT_FIELD(BitField, IsAbstract, bool, ONE_BIT, IsGetterSetter);
    NEXT_BIT_FIELD(BitField, IsSignature, bool, ONE_BIT, IsAbstract);

    DECL_VISIT_OBJECT(NAME_OFFSET, RETURN_GT_OFFSET)
    DECL_DUMP()
};

class TSArrayType : public TSType {
public:
    CAST_CHECK(TSArrayType, IsTSArrayType);
    static constexpr size_t ELEMENT_GT_OFFSET = TSType::SIZE;

    ACCESSORS_ATTACHED_TYPEREF(ElementGT, ELEMENT_GT_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_DUMP()
};

class TSIteratorInstanceType : public TSType {
public:
    CAST_CHECK(TSIteratorInstanceType, IsTSIteratorInstanceType);
    static constexpr size_t KIND_GT_OFFSET = TSType::SIZE;

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSIteratorInstanceType> instanceType,
                                         JSHandle<EcmaString> propName);

    ACCESSORS_ATTACHED_TYPEREF(KindGT, KIND_GT_OFFSET, ELEMENT_GT_OFFSET);
    ACCESSORS_ATTACHED_TYPEREF(ElementGT, ELEMENT_GT_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_H
