/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_IC_IC_RUNTIME_STUB_H
#define ECMASCRIPT_IC_IC_RUNTIME_STUB_H

#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
class ICRuntimeStub {
public:
    static inline JSTaggedValue LoadGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                   JSTaggedValue globalValue, JSTaggedValue key, uint32_t slotId,
                                                   bool tryLoad);
    static inline JSTaggedValue StoreGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                    JSTaggedValue globalValue, JSTaggedValue key,
                                                    JSTaggedValue value, uint32_t slotId, bool tryStore);
    static inline JSTaggedValue LoadICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                             JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId);
    static inline JSTaggedValue TryLoadICByName(JSThread *thread, JSTaggedValue receiver,
                                                JSTaggedValue firstValue, JSTaggedValue secondValue);
    static inline JSTaggedValue TryStoreICByName(JSThread *thread, JSTaggedValue receiver,
                                                 JSTaggedValue firstValue, JSTaggedValue secondValue,
                                                 JSTaggedValue value);
    static inline JSTaggedValue StoreICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                              JSTaggedValue receiver, JSTaggedValue key,
                                              JSTaggedValue value, uint32_t slotId);
    static inline JSTaggedValue CheckPolyHClass(const JSThread *thread, JSTaggedValue cachedValue, JSHClass* hclass);
    static inline JSTaggedValue LoadICWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                  JSTaggedValue handler);
    static inline JSTaggedValue LoadICWithElementHandler(JSThread *thread, JSTaggedValue receiver,
                                                         JSTaggedValue handler, JSTaggedValue key);
    static inline JSTaggedValue StoreICWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                   JSTaggedValue value, JSTaggedValue handler);
    static inline void StoreWithTransition(JSThread *thread, JSObject *receiver, JSTaggedValue value,
                                           JSTaggedValue handler, bool withPrototype = false);
    static inline JSTaggedValue StoreTransWithProto(JSThread *thread, JSObject *receiver,
                                                    JSTaggedValue value, JSTaggedValue handler);
    static inline JSTaggedValue StoreWithTS(JSThread *thread, JSTaggedValue receiver,
                                            JSTaggedValue value, JSTaggedValue handler);
    static inline JSTaggedValue StorePrototype(JSThread *thread, JSTaggedValue receiver,
                                               JSTaggedValue value, JSTaggedValue handler);
    static inline JSTaggedValue LoadFromField(const JSThread *thread, JSObject *receiver, uint64_t handlerInfo);
    static inline void StoreField(JSThread *thread, JSObject *receiver, JSTaggedValue value, uint64_t handler);
    static inline JSTaggedValue LoadGlobal(const JSThread *thread, JSTaggedValue handler);
    static inline JSTaggedValue StoreGlobal(JSThread *thread, JSTaggedValue value, JSTaggedValue handler);
    static inline JSTaggedValue LoadPrototype(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler);

    static inline JSTaggedValue TryLoadICByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                 JSTaggedValue firstValue, JSTaggedValue secondValue);
    static inline JSTaggedValue LoadICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                              JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId);
    static inline JSTaggedValue TryStoreICByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue firstValue, JSTaggedValue secondValue,
                                                  JSTaggedValue value);
    static inline JSTaggedValue StoreICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                               JSTaggedValue receiver, JSTaggedValue key, JSTaggedValue value,
                                               uint32_t slotId);
    static inline JSTaggedValue StoreOwnICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                  JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue value, uint32_t slotId);
    static inline JSTaggedValue LoadElement(JSThread *thread, JSObject *receiver, JSTaggedValue key);
    static inline JSTaggedValue LoadStringElement(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    static inline JSTaggedValue LoadTypedArrayElement(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template <typename T>
    static inline JSTaggedValue StoreElementWithProtoHandler(JSThread *thread, JSObject *receiver, JSTaggedValue key,
                                                             JSTaggedValue value, JSTaggedValue handlerInfo);
    static inline JSTaggedValue StoreElement(JSThread *thread, JSObject *receiver, JSTaggedValue key,
                                             JSTaggedValue value, JSTaggedValue handlerInfo);
    static inline JSTaggedValue StoreTypedArrayElement(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                       JSTaggedValue value);
    static inline int64_t TryToElementsIndex(JSThread *thread, JSTaggedValue key);
    static inline JSTaggedValue LoadMiss(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                         JSTaggedValue key, uint32_t slotId, ICKind kind);
    static inline JSTaggedValue LoadValueMiss(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
        JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId, ICKind kind);
    static inline JSTaggedValue StoreMiss(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                          JSTaggedValue key, JSTaggedValue value, uint32_t slotId,
                                          ICKind kind, bool isOwn = false);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_RUNTIME_STUB_H
