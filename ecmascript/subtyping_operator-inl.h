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

#ifndef ECMASCRIPT_SUBTYPING_OPERATOR_INL_H
#define ECMASCRIPT_SUBTYPING_OPERATOR_INL_H

#include "ecmascript/subtyping_operator.h"

namespace panda::ecmascript {
template<class T>
int SubtypingOperator::GetElementByKey(const JSThread *thread, T *obj, JSTaggedValue key)
{
    if constexpr(std::is_same_v<T, TSObjLayoutInfo>) {
        return obj->GetElementIndexByKey(key);
    } else if constexpr(std::is_same_v<T, VTable>) {
        return obj->GetTupleIndexByName(key);
    } else if constexpr(std::is_same_v<T, JSHClass>) {
        LayoutInfo *objLayout = LayoutInfo::Cast(obj->GetLayout().GetTaggedObject());
        return objLayout->FindElementWithCache(thread, obj, key, obj->NumberOfProps());
    }
    return -1;
}

template<class T>
bool SubtypingOperator::IsLegalElement(const JSThread *thread, T *obj, JSTaggedValue key, JSTaggedValue expectedType)
{
    // when the property of the subtyping does not exist on the parent class,
    // or exists and has the same type, the property is considered legal
    if constexpr(std::is_same_v<T, TSObjLayoutInfo>) {
        int index = obj->GetElementIndexByKey(key);
        if (index != -1 && obj->GetTypeId(index) != expectedType) {
            return false;
        }
    } else if constexpr(std::is_same_v<T, VTable>) {
        int index = obj->GetTupleIndexByName(key);
        if (index != -1) {
            // type: ture --> isAccessor, false --> normal function
            ASSERT(expectedType.IsBoolean());
            bool isAccessor = expectedType.IsTrue();
            if (obj->IsAccessor(index) != isAccessor) {
                return false;
            }
        }
    } else if constexpr(std::is_same_v<T, JSHClass>) {
        LayoutInfo *objLayout = LayoutInfo::Cast(obj->GetLayout().GetTaggedObject());
        int index = objLayout->FindElementWithCache(thread, obj, key, obj->NumberOfProps());
        if (index != -1) {
            // type: ture --> isAccessor, false --> normal function
            ASSERT(expectedType.IsBoolean());
            PropertyAttributes attr(objLayout->GetAttr(index));
            bool isAccessor = expectedType.IsTrue();
            if (attr.IsAccessor() != isAccessor) {
                return false;
            }
        }
    }
    return true;
}

template<class T>
uint32_t SubtypingOperator::GetLength(T *obj)
{
    if constexpr(std::is_same_v<T, TSObjLayoutInfo>) {
        return obj->GetNumOfProperties();
    } else {
        return 0;
    }
}

template<class T>
JSTaggedValue SubtypingOperator::GetKey(T *obj, uint32_t index)
{
    if constexpr(std::is_same_v<T, TSObjLayoutInfo>) {
        return obj->GetKey(index);
    } else {
        return JSTaggedValue::Undefined();
    }
}

template<class T>
JSTaggedValue SubtypingOperator::GetType(const JSThread *thread, T *obj, uint32_t index, ConditionType ConditionType)
{
    if constexpr(std::is_same_v<T, TSObjLayoutInfo>) {
        if (ConditionType == ConditionType::SUB_LOCAL_CONTAIN_SUP_LOCAL) {
            return obj->GetTypeId(index);
        } else if (ConditionType == ConditionType::SUB_VTABLE_CONTAIN_SUP_VTABLE) {
            TSManager *tsManager = const_cast<JSThread*>(thread)->GetCurrentEcmaContext()->GetTSManager();
            GlobalTSTypeRef typeGT = GlobalTSTypeRef(obj->GetTypeId(index).GetInt());
                return JSTaggedValue(tsManager->IsGetterSetterFunc(typeGT));
        }
    }
    return JSTaggedValue::Undefined();
}

template<class Suber, class Super>
bool SubtypingOperator::SubtypingCondition(const JSThread *thread, Suber *suber, Super *super,
                                           ConditionType ConditionType)
{
    uint32_t len = GetLength(suber);
    for (uint32_t index = 0; index < len; ++index) {
        JSTaggedValue key = GetKey(suber, index);
        ASSERT(!key.IsUndefined());
        if (ConditionType == ConditionType::SUB_LOCAL_CONTAIN_SUP_LOCAL ||
            ConditionType == ConditionType::SUB_VTABLE_CONTAIN_SUP_VTABLE) {
            JSTaggedValue type = GetType(thread, suber, index, ConditionType);
            ASSERT(!type.IsUndefined());
            if (!IsLegalElement(thread, super, key, type)) {
                return false;
            }
        } else {
            if (GetElementByKey(thread, super, key) != -1) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_SUBTYPING_OPERATOR_INL_H