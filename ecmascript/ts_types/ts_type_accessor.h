/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_ACCESSOR_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_ACCESSOR_H

#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript {
#define GET_TSTYPE_LIST(V)                                 \
    V(Class, JSHandle<TSClassType>)                        \
    V(ClassInstance, JSHandle<TSClassInstanceType>)        \
    V(Function, JSHandle<TSFunctionType>)                  \
    V(Union, JSHandle<TSUnionType>)                        \
    V(Array, JSHandle<TSArrayType>)                        \
    V(Object, JSHandle<TSObjectType>)                      \
    V(Interface, JSHandle<TSInterfaceType>)                \
    V(IteratorInstance, JSHandle<TSIteratorInstanceType>)  \
    V(Namespace, JSHandle<TSNamespaceType>)

#define CLASS_TYPE_BITFIELD_ACCESSOR_LIST(V)  \
    V(HasLinked)                              \
    V(HasPassedSubtypingCheck)                \
    V(HasAnalysedInitialization)              \
    V(HasEscapedThisInConstructor)            \
    V(HasSuperCallInConstructor)

class TSFieldAttributes {
public:
    explicit TSFieldAttributes(uint32_t value) : value_(value) {}
    ~TSFieldAttributes() = default;
    NO_COPY_SEMANTIC(TSFieldAttributes);
    NO_MOVE_SEMANTIC(TSFieldAttributes);

    static constexpr size_t BOOL_FLAG_BIT_LENGTH = 1;
    FIRST_BIT_FIELD(Value, InitializedFlag, bool, BOOL_FLAG_BIT_LENGTH);

    inline uint32_t GetValue() const
    {
        return value_;
    }

    inline void SetValue(const uint32_t value)
    {
        value_ = value;
    }

private:
    uint32_t value_ {0};
};

class TSTypeAccessor {
public:
    explicit TSTypeAccessor(TSManager *tsManager, const GlobalTSTypeRef gt)
        : tsManager_(tsManager), thread_(tsManager_->GetThread()), gt_(gt) {}

    explicit TSTypeAccessor(TSManager *tsManager, const kungfu::GateType type)
        : tsManager_(tsManager), thread_(tsManager_->GetThread()), gt_(type.Value()) {}

    ~TSTypeAccessor() = default;
    NO_COPY_SEMANTIC(TSTypeAccessor);
    NO_MOVE_SEMANTIC(TSTypeAccessor);

    void PUBLIC_API MarkPropertyInitialized(JSTaggedValue key);

    void PUBLIC_API UpdateNonStaticProp(JSTaggedValue key, GlobalTSTypeRef newGT);

    void PUBLIC_API UpdateStaticProp(JSTaggedValue key, GlobalTSTypeRef newGT);

    void PUBLIC_API UpdateForEachCBPara(kungfu::GateType targetType);

    bool PUBLIC_API IsPropertyInitialized(JSTaggedValue key) const;

    std::string PUBLIC_API GetInitializedProperties() const;

    std::string PUBLIC_API GetClassTypeName() const;

    std::string PUBLIC_API GetFunctionName() const;

    GlobalTSTypeRef PUBLIC_API GetAccessorGT(JSTaggedValue key, bool isSetter) const;

#define CLASS_TYPE_BITFIELD_ACCESSOR(NAME)                 \
    inline void PUBLIC_API MarkClass##NAME()               \
    {                                                      \
        ASSERT(tsManager_->IsClassTypeKind(gt_));          \
        JSHandle<TSClassType> classType = GetClassType();  \
        classType->Set##NAME(true);                        \
    }                                                      \
                                                           \
    inline bool PUBLIC_API Class##NAME() const             \
    {                                                      \
        ASSERT(tsManager_->IsClassTypeKind(gt_));          \
        JSHandle<TSClassType> classType = GetClassType();  \
        return classType->Get##NAME();                     \
    }

    CLASS_TYPE_BITFIELD_ACCESSOR_LIST(CLASS_TYPE_BITFIELD_ACCESSOR)
#undef CLASS_TYPE_BITFIELD_ACCESSOR

private:
#define GET_TSTYPE(NAME, TYPE)                                        \
    inline TYPE Get##NAME##Type() const                               \
    {                                                                 \
        ASSERT(tsManager_->Is##NAME##TypeKind(gt_));                  \
        JSHandle<JSTaggedValue> tsType = tsManager_->GetTSType(gt_);  \
        return TYPE(tsType);                                          \
    }

    GET_TSTYPE_LIST(GET_TSTYPE)
#undef GET_TSTYPE

    JSTaggedValue MarkInitialized(JSTaggedValue flag);

    JSHandle<TSObjLayoutInfo> GetInstanceTypeLayout() const;
    JSHandle<TSObjLayoutInfo> GetPrototypeTypeLayout() const;

    bool IsGetterGT(size_t parameterLength, bool isSetter) const
    {
        return parameterLength == 0 && !isSetter;
    }

    bool IsSetterGT(size_t parameterLength, bool isSetter) const
    {
        return parameterLength == 1 && isSetter;
    }

    TSManager *tsManager_ {nullptr};
    JSThread *thread_ {nullptr};
    GlobalTSTypeRef gt_;
};
#undef CLASS_TYPE_BITFIELD_ACCESSOR_LIST
#undef GET_TSTYPE_LIST
}  // panda::ecmascript
#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_ACCESSOR_H
