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
    static constexpr size_t ONE_BIT = 1;
    static constexpr size_t TWO_BITS = 2;

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

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSObjectType> objectType,
                                         JSHandle<JSTaggedValue> propName);

    static bool UpdatePropTypeGT(JSThread *thread, JSHandle<TSObjectType> objectType,
                                 JSHandle<JSTaggedValue> propName, GlobalTSTypeRef newGT);

    static GlobalTSTypeRef GetIndexSignType(JSThread *thread, const JSHandle<TSObjectType> &objectType,
                                            const uint32_t typeId);

    ACCESSORS(ObjLayoutInfo, PROPERTIES_OFFSET, INDEX_SIGNS_OFFSET);
    ACCESSORS(IndexSigns, INDEX_SIGNS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(PROPERTIES_OFFSET, SIZE)
    DECL_DUMP()
};

class TSClassType : public TSType {
public:
    CAST_CHECK(TSClassType, IsTSClassType);

    static constexpr size_t INSTANCE_TYPE_OFFSET = TSType::SIZE;

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                         JSHandle<JSTaggedValue> propName);

    static GlobalTSTypeRef GetSuperPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                              JSHandle<JSTaggedValue> propName, PropertyType propType);

    static GlobalTSTypeRef GetNonStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                                  JSHandle<JSTaggedValue> propName);

    static void UpdateNonStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                          JSHandle<JSTaggedValue> propName, GlobalTSTypeRef newGT);

    static void UpdateStaticPropTypeGT(JSThread *thread, JSHandle<TSClassType> classType,
                                       JSHandle<JSTaggedValue> propName, GlobalTSTypeRef newGT);

    ACCESSORS(InstanceType, INSTANCE_TYPE_OFFSET, CONSTRUCTOR_TYPE_OFFSET);
    ACCESSORS(ConstructorType, CONSTRUCTOR_TYPE_OFFSET, PROTOTYPE_TYPE_OFFSET);
    ACCESSORS(PrototypeType, PROTOTYPE_TYPE_OFFSET, NAME_OFFSET);
    ACCESSORS(Name, NAME_OFFSET, INDEX_SIGNS_OFFSET);
    ACCESSORS(IndexSigns, INDEX_SIGNS_OFFSET, EXTENSION_GT_OFFSET);
    ACCESSORS_ATTACHED_TYPEREF(ExtensionGT, EXTENSION_GT_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    FIRST_BIT_FIELD(BitField, HasLinked, bool, ONE_BIT);
    NEXT_BIT_FIELD(BitField, HasPassedSubtypingCheck, bool, ONE_BIT, HasLinked);
    NEXT_BIT_FIELD(BitField, HasAnalysedInitialization, bool, ONE_BIT, HasPassedSubtypingCheck);
    NEXT_BIT_FIELD(BitField, HasEscapedThisInConstructor, bool, ONE_BIT, HasAnalysedInitialization);
    NEXT_BIT_FIELD(BitField, HasSuperCallInConstructor, bool, ONE_BIT, HasEscapedThisInConstructor);

    DECL_VISIT_OBJECT(INSTANCE_TYPE_OFFSET, EXTENSION_GT_OFFSET)
    DECL_DUMP()

    // Judgment base classType by extends typeId, ts2abc write 0 in base class type extends domain
    inline static bool IsBaseClassType(int extendsTypeId)
    {
        const int baseClassTypeExtendsTypeId = 0;
        return extendsTypeId == baseClassTypeExtendsTypeId;
    }

    inline bool IsBaseClassType() const
    {
        return GetExtensionGT().IsDefault();
    }
};

class TSClassInstanceType : public TSType {
public:
    CAST_CHECK(TSClassInstanceType, IsTSClassInstanceType);

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSClassInstanceType> classInstanceType,
                                         JSHandle<JSTaggedValue> propName);

    static GlobalTSTypeRef GetIndexSignType(JSThread *thread, const JSHandle<TSClassInstanceType> &classInstanceType,
                                            const uint32_t typeId);

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

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, JSHandle<TSInterfaceType> classInstanceType,
                                         JSHandle<JSTaggedValue> propName);

    static GlobalTSTypeRef GetIndexSignType(JSThread *thread, const JSHandle<TSInterfaceType> &interfaceType,
                                            const uint32_t typeId);

    static constexpr size_t EXTENDS_TYPE_ID_OFFSET = TSType::SIZE;
    ACCESSORS(Extends, EXTENDS_TYPE_ID_OFFSET, KEYS_OFFSET);
    ACCESSORS(Fields, KEYS_OFFSET, INDEX_SIGNS_OFFSET);
    ACCESSORS(IndexSigns, INDEX_SIGNS_OFFSET, SIZE);

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
    FIRST_BIT_FIELD(BitField, Visibility, Visibility, TWO_BITS);
    NEXT_BIT_FIELD(BitField, Static, bool, ONE_BIT, Visibility);
    NEXT_BIT_FIELD(BitField, Async, bool, ONE_BIT, Static);
    NEXT_BIT_FIELD(BitField, Generator, bool, ONE_BIT, Async);
    NEXT_BIT_FIELD(BitField, IsGetterSetter, bool, ONE_BIT, Generator);
    NEXT_BIT_FIELD(BitField, IsAbstract, bool, ONE_BIT, IsGetterSetter);
    NEXT_BIT_FIELD(BitField, IsSignature, bool, ONE_BIT, IsAbstract);
    NEXT_BIT_FIELD(BitField, IsFastCall, bool, ONE_BIT, IsSignature);
    NEXT_BIT_FIELD(BitField, IsFastCallVaild, bool, ONE_BIT, IsFastCall);
    NEXT_BIT_FIELD(BitField, IsMethodOffsetVaild, bool, ONE_BIT, IsFastCallVaild);
    NEXT_BIT_FIELD(BitField, IsNoGC, bool, ONE_BIT, IsMethodOffsetVaild);

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
                                         JSHandle<JSTaggedValue> propName);

    ACCESSORS_ATTACHED_TYPEREF(KindGT, KIND_GT_OFFSET, ELEMENT_GT_OFFSET);
    ACCESSORS_ATTACHED_TYPEREF(ElementGT, ELEMENT_GT_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_DUMP()
};

class TSNamespaceType : public TSType {
public:
    CAST_CHECK(TSNamespaceType, IsTSNamespaceType);
    static constexpr size_t PROTOTYPE_TYPE_OFFSET = TSType::SIZE;

    ACCESSORS(PropertyType, PROTOTYPE_TYPE_OFFSET, SIZE);

    static void AddKeyAndValue(const JSThread *thread, const JSHandle<TSNamespaceType> &namespaceType,
                               const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static GlobalTSTypeRef GetPropTypeGT(JSThread *thread, const JSHandle<TSNamespaceType> &namespaceType,
                                         const JSHandle<JSTaggedValue> &propName);

    DECL_VISIT_OBJECT(PROTOTYPE_TYPE_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_H
