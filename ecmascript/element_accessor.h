/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ELEMENT_ACCESSOR_H
#define ECMASCRIPT_ELEMENT_ACCESSOR_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array.h"

namespace panda {
namespace ecmascript {
// ElementAccessor intends to replace all .GetElements and share the common following methods.
class ElementAccessor {
public:
    static JSTaggedValue PUBLIC_API Get(const JSThread *thread, JSHandle<JSObject> receiver, uint32_t idx);
    static JSTaggedValue Get(const JSThread *thread, JSObject *receiver, uint32_t idx);
    static JSTaggedValue PUBLIC_API FastGet(JSHandle<TaggedArray> elements, uint32_t idx, ElementsKind kind);

    template<typename T>
    static void Set(const JSThread *thread, JSHandle<JSObject> receiver, uint32_t idx, const JSHandle<T> &value,
                    bool needTransition, ElementsKind extraKind = ElementsKind::NONE);

    template<typename T>
    static void FastSet(const JSThread *thread, JSHandle<TaggedArray> elements, uint32_t idx,
                        const JSHandle<T> &value, ElementsKind kind);
    static bool IsDictionaryMode(JSHandle<JSObject> receiver);
    static bool IsDictionaryMode(JSObject *receiver);
    static uint32_t GetElementsLength(JSHandle<JSObject> receiver);
    static uint32_t GetElementsLength(JSObject *receiver);

    static JSTaggedValue GetTaggedValueWithElementsKind(JSTaggedType rawValue, ElementsKind kind);

    static void CopyJSArrayObject(const JSThread *thread, JSHandle<JSObject>srcObj, JSHandle<JSObject>dstObj,
                                  uint32_t effectiveLength);
    static void CopyJSArrayToTaggedArray(const JSThread *thread, JSHandle<JSObject>srcObj,
                                         JSHandle<TaggedArray>dstElements, uint32_t effectiveLength);
    static JSTaggedType PUBLIC_API ConvertTaggedValueWithElementsKind(JSTaggedValue rawValue, ElementsKind kind);
private:
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_ELEMENT_ACCESSOR_H

