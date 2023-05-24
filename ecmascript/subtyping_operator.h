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

#ifndef ECMASCRIPT_SUBTYPING_OPERATOR_H
#define ECMASCRIPT_SUBTYPING_OPERATOR_H

#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript {
class SubtypingOperator {
public:
    static constexpr uint32_t DEFAULT_SUPERS_CAPACITY = 4;

    static bool CheckBaseClass(const JSThread *thread, const JSHandle<TSClassType> &classType);

    static bool CheckSubtyping(const JSThread *thread, const JSHandle<TSClassType> &classType);

    static void MergeClassField(const JSThread *thread, const JSHandle<TSClassType> &classType);

    static void FillTSInheritInfo(JSThread *thread, const JSHandle<TSClassType> &classType,
                                  const JSHandle<JSHClass> &ihcHandle);

    static void GenVTable(const JSThread *thread, const JSHandle<JSHClass> &ihcHandle,
                          const JSHandle<JSHClass> &phcHandle, const JSHandle<JSHClass> &eIhcHandle);

    static void TryMaintainTSSubtyping(const JSThread *thread, const JSHandle<JSHClass> &oldHClass,
                                       JSHandle<JSHClass> &newHClass, const JSHandle<JSTaggedValue> &key);

    static bool TryMaintainTSSubtypingOnPrototype(const JSThread *thread, const JSHandle<JSHClass> &hclass,
                                                  const JSHandle<JSTaggedValue> &key);

private:
    static constexpr uint8_t MAX_LEVEL = 1 << JSHClass::LEVEL_BTTFIELD_NUM;

    /* rule check operations will check whether the properties in the Local and Vtable
     * of TSHClass meet the requirements for establishing a TSHClass chain.The local
     * properties refer to the properties that exists on the object instance, and the
     * vtable properties refers to the properties of its ancestor prototype objects
     * (such as accessor, method, and ordinary property in special cases)
     *
     * if B extends A
     * specific requirements are as follows (N: name, T: type):
     * 1. local_N(B) ∩ vtable_N(B) = ∅
     * 2. local(A) ⊆ local(B)
     * 3. vtable_NT(A) ⊆ vtable_NT(B)
     * 4. local_N(A) ∩ vtable_N(B) = ∅
     * 5. vtable_N(A) ∩ local_N(B) = ∅
     */
    enum class ConditionType : uint8_t {
        NO_OVERLAP_SUB_LOCAL_SUB_VTABLE = 0,
        SUB_LOCAL_CONTAIN_SUP_LOCAL,
        SUB_VTABLE_CONTAIN_SUP_VTABLE,
        NO_OVERLAP_SUP_LOCAL_SUB_VTABLE,
        NO_OVERLAP_SUP_VTABLE_SUB_LOCAL,
    };

    template<class T>
    static int GetElementByKey(const JSThread *thread, T *obj, JSTaggedValue key);

    template<class T>
    static bool IsLegalElement(const JSThread *thread, T *obj, JSTaggedValue key, JSTaggedValue expectedType);

    template<class T>
    static uint32_t GetLength(T *obj);

    template<class T>
    static JSTaggedValue GetKey(T *obj, uint32_t index);

    template<class T>
    static JSTaggedValue GetType(const JSThread *thread, T *obj, uint32_t index, ConditionType ConditionType);

    template<class Suber, class Super>
    static bool SubtypingCondition(const JSThread *thread, Suber *suber, Super *super, ConditionType ConditionType);

    static void AddSuper(const JSThread *thread, const JSHandle<JSHClass> &iHClass,
                         const JSHandle<JSHClass> &extendHClass);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SUBTYPING_OPERATOR_H
