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

#ifndef ECMASCRIPT_OBJECT_FAST_OPERATOR_H
#define ECMASCRIPT_OBJECT_FAST_OPERATOR_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
using SCheckMode = JSShared::SCheckMode;

class ObjectFastOperator final {
public:
    enum class Status: uint8_t {
        None = 0x00UL,
        UseOwn = 0x01UL,
        GetInternal = 0x1UL << 1,
        DefineSemantics = 0x1UL << 2,
    };

    static inline bool UseOwn(Status status)
    {
        return (static_cast<int32_t>(status) & static_cast<int32_t>(Status::UseOwn)) > 0;
    }
    
    static inline bool GetInternal(Status status)
    {
        return (static_cast<int32_t>(status) & static_cast<int32_t>(Status::GetInternal)) > 0;
    }

    static inline bool DefineSemantics(Status status)
    {
        return (static_cast<uint32_t>(status) & static_cast<int32_t>(Status::DefineSemantics)) > 0;
    }

    static inline std::pair<JSTaggedValue, bool> HasOwnProperty(JSThread *thread,
                                                                JSTaggedValue receiver, JSTaggedValue key);

    template<Status status = Status::None>
    static inline JSTaggedValue TryFastHasProperty(JSThread *thread, JSTaggedValue receiver,
                                                   JSMutableHandle<JSTaggedValue> keyHandle);

    template<Status status = Status::None>
    static inline JSTaggedValue TryFastGetPropertyByValue(JSThread *thread, JSTaggedValue receiver,
                                                          JSMutableHandle<JSTaggedValue> keyHandle);

    template<Status status = Status::None>
    static inline JSTaggedValue TryFastGetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index);

    template<Status status = Status::None>
    static inline JSTaggedValue TryGetPropertyByNameThroughCacheAtLocal(JSThread *thread, JSTaggedValue receiver,
                                                                        JSTaggedValue key);

    template<Status status = Status::None>
    static inline JSTaggedValue GetPropertyByName(JSThread *thread, JSTaggedValue receiver,
                                                  JSTaggedValue key, bool noAllocate = false,
                                                  bool *isCallGetter = nullptr);

    template <Status status = Status::None>
    static inline JSTaggedValue TrySetPropertyByNameThroughCacheAtLocal(JSThread *thread,
        JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value);

    template <Status status = Status::None>
    static inline JSTaggedValue SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue value, SCheckMode sCheckMode = SCheckMode::CHECK);

    static inline JSTaggedValue SetJsonPropertyByName(JSThread *thread, JSTaggedValue receiver,
                                                      JSTaggedValue key, JSTaggedValue value);
    
    template <Status status = Status::None>
    static inline JSTaggedValue GetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index);

    template <Status status = Status::None>
    static inline JSTaggedValue SetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                   JSTaggedValue value);

    template <Status status = Status::None>
    static inline JSTaggedValue GetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);

    template <Status status = Status::None>
    static inline JSTaggedValue SetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                   JSTaggedValue value, SCheckMode sCheckMode = SCheckMode::CHECK);

    static inline bool FastSetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                              JSTaggedValue value, SCheckMode sCheckMode = SCheckMode::CHECK);

    static inline bool FastSetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                              JSTaggedValue value);

    static inline JSTaggedValue FastGetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);

    static inline JSTaggedValue FastGetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                       SCheckMode sCheckMode = SCheckMode::CHECK);

    static inline JSTaggedValue FastGetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index);

    static inline JSTaggedValue FastParseDate(const EcmaString *str);

    static inline PropertyAttributes AddPropertyByName(JSThread *thread, JSHandle<JSObject> objHandle,
                                                       JSHandle<JSTaggedValue> keyHandle,
                                                       JSHandle<JSTaggedValue> valueHandle,
                                                       PropertyAttributes attr);

    static inline JSTaggedValue CallGetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                           JSTaggedValue value);

    static inline int64_t TryToElementsIndex(JSTaggedValue key);
private:
    static inline JSTaggedValue CallSetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue value,
                                           JSTaggedValue accessorValue);

    static inline bool ShouldCallSetter(JSTaggedValue receiver, JSTaggedValue holder, JSTaggedValue accessorValue,
                                        PropertyAttributes attr);

    static inline bool IsSpecialIndexedObj(JSType jsType);

    static inline bool IsJSSharedArray(JSType jsType);

    static inline bool IsFastTypeArray(JSType jsType);

    static inline bool IsString(JSType jsType);

    static inline bool IsJSPrimitiveRef(JSType jsType);

    static inline bool IsJSProxy(JSType jsType);

    static inline bool TryStringOrSymbolToIndex(JSTaggedValue key, uint32_t *output);

    static inline JSTaggedValue FastGetTypeArrayProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                         JSTaggedValue key, JSType jsType);

    static inline JSTaggedValue FastSetTypeArrayProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                         JSTaggedValue key, JSTaggedValue value, JSType jsType);

    // non ECMA standard jsapi container
    static inline bool IsSpecialContainer(JSType jsType);

    static inline JSTaggedValue GetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                     JSType jsType);

    static inline JSTaggedValue SetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                     JSTaggedValue value, JSType jsType);

    static inline JSTaggedValue AddPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                   JSTaggedValue value);

    static inline bool GetNumFromString(const char *str, int len, int *index, int *num);

    friend class FastRuntimeStub;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_FAST_OPERATOR_H
