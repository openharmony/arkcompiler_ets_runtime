/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_H
#define ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_H

#include <string>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"

#include "ecmascript/builtins/builtins_shared_typedarray.h"
#include "ecmascript/builtins/builtins_typedarray.h"

namespace panda::ecmascript::base {
enum ElementSize : uint8_t { ONE = 1, TWO = 2, FOUR = 4, EIGHT = 8 };

enum class TypedArrayKind : uint8_t {
    NON_SHARED = 0,
    SHARED = 1
};

template <TypedArrayKind typedArrayKind>
struct BuiltinsArrayBufferType {};

class TypedArrayHelper {
public:
    static JSTaggedValue TypedArrayConstructor(EcmaRuntimeCallInfo *argv,
                                               const JSHandle<JSTaggedValue> &constructorName,
                                               const DataViewType arrayType);
    static JSTaggedValue SharedTypedArrayConstructor(EcmaRuntimeCallInfo *argv,
                                                     const JSHandle<JSTaggedValue> &constructorName,
                                                     const DataViewType arrayType);
    static JSHandle<JSObject> FastCreateTypedArray(JSThread *thread,
                                                   const JSHandle<JSTaggedValue> &constructorName,
                                                   uint32_t length,
                                                   const DataViewType arrayType);
    static JSHandle<JSObject> AllocateTypedArray(JSThread *thread,
                                                 const JSHandle<JSTaggedValue> &constructorName,
                                                 const JSHandle<JSTaggedValue> &newTarget,
                                                 const DataViewType arrayType);
    static JSHandle<JSObject> AllocateTypedArray(JSThread *thread,
                                                 const JSHandle<JSTaggedValue> &constructorName,
                                                 const JSHandle<JSTaggedValue> &newTarget, uint32_t length,
                                                 const DataViewType arrayType);
    static JSHandle<JSObject> AllocateSharedTypedArray(JSThread *thread,
                                                       const JSHandle<JSTaggedValue> &constructorName,
                                                       const JSHandle<JSTaggedValue> &newTarget,
                                                       const DataViewType arrayType);
    static JSHandle<JSObject> AllocateSharedTypedArray(JSThread *thread,
                                                       const JSHandle<JSTaggedValue> &constructorName,
                                                       const JSHandle<JSTaggedValue> &newTarget, uint32_t length,
                                                       const DataViewType arrayType);
    template <TypedArrayKind typedArrayKind = TypedArrayKind::NON_SHARED>
    static JSHandle<JSObject> TypedArraySpeciesCreate(JSThread *thread, const JSHandle<JSTypedArray> &obj,
                                                      uint32_t argc, JSTaggedType argv[]);
    static JSHandle<JSObject> TypedArrayCreate(JSThread *thread, const JSHandle<JSTaggedValue> &constructor,
                                               uint32_t argc, const JSTaggedType argv[]);
    static JSHandle<JSObject> TypedArrayCreateSameType(JSThread *thread, const JSHandle<JSTypedArray> &obj,
                                                       uint32_t argc, JSTaggedType argv[]);
    static JSTaggedValue ValidateTypedArray(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    inline static DataViewType GetType(const JSHandle<JSTypedArray> &obj);
    inline static DataViewType GetType(JSType type);
    inline static uint32_t GetElementSize(const JSHandle<JSTypedArray> &obj);
    inline static uint32_t GetElementSize(JSType type);
    inline static JSHandle<JSTaggedValue> GetConstructor(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    inline static JSHandle<JSFunction> GetConstructorFromType(JSThread *thread,
                                                              const DataViewType arrayType);
    inline static JSHandle<JSFunction> GetSharedConstructorFromType(JSThread *thread,
                                                                    const DataViewType arrayType);
    inline static JSHandle<JSTaggedValue> GetConstructorNameFromType(JSThread *thread,
                                                                     const DataViewType arrayType);
    inline static JSHandle<JSTaggedValue> GetSharedConstructorNameFromType(JSThread *thread,
                                                                           const DataViewType arrayType);
    inline static JSHandle<JSHClass> GetOnHeapHclassFromType(
        JSThread *thread, const JSHandle<JSTypedArray> &obj, const DataViewType arrayType);
    inline static JSHandle<JSHClass> GetNotOnHeapHclassFromType(
        JSThread *thread, const JSHandle<JSTypedArray> &obj, const DataViewType arrayType);
    inline static JSHandle<JSHClass> GetSharedOnHeapHclassFromType(
        JSThread *thread, const JSHandle<JSTypedArray> &obj, const DataViewType arrayType);
    inline static JSHandle<JSHClass> GetSharedNotOnHeapHclassFromType(
        JSThread *thread, const JSHandle<JSSharedTypedArray> &obj, const DataViewType arrayType);
    inline static uint32_t GetSizeFromType(const DataViewType arrayType);
    inline static bool IsAccessorHasChanged(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static int32_t SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                               const JSHandle<JSTaggedValue> &buffer, const JSHandle<JSTaggedValue> &firstValue,
                               const JSHandle<JSTaggedValue> &secondValue);
    static bool IsNativeArrayIterator(JSThread *thread,
        const JSHandle<JSTaggedValue> &obj, JSHandle<JSTaggedValue> &iterMethod);
    template <TypedArrayKind typedArrayKind = TypedArrayKind::NON_SHARED>
    static JSHandle<JSObject> AllocateTypedArrayBuffer(JSThread *thread, const JSHandle<JSObject> &obj,
                                                       uint64_t length, const DataViewType arrayType);

    #define DEFINE_GET_ONHEAP_HCLASS_FROM_TYPE(Type)                                                          \
        inline static JSHandle<JSHClass> GetOnHeapHclass##Type(JSThread *thread, JSHClass* objHclass);
    TYPED_ARRAY_TYPES(DEFINE_GET_ONHEAP_HCLASS_FROM_TYPE)
    SHARED_TYPED_ARRAY_TYPES(DEFINE_GET_ONHEAP_HCLASS_FROM_TYPE)
    #undef DEFINE_GET_ONHEAP_HCLASS_FROM_TYPE

    #define DEFINE_GET_NOT_ONHEAP_HCLASS_FROM_TYPE(Type)                                                          \
        inline static JSHandle<JSHClass> GetNotOnHeapHclass##Type(JSThread *thread, JSHClass* objHclass);
    TYPED_ARRAY_TYPES(DEFINE_GET_NOT_ONHEAP_HCLASS_FROM_TYPE)
    SHARED_TYPED_ARRAY_TYPES(DEFINE_GET_NOT_ONHEAP_HCLASS_FROM_TYPE)
    #undef DEFINE_GET_NOT_ONHEAP_HCLASS_FROM_TYPE

private:
    template <TypedArrayKind typedArrayKind = TypedArrayKind::NON_SHARED>
    static JSTaggedValue CreateFromOrdinaryObject(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                                  const DataViewType arrayType);
    static JSTaggedValue CreateFromTypedArray(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                              const DataViewType arrayType);
    static JSTaggedValue CreateSharedFromTypedArray(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                              const DataViewType arrayType);
    static JSTaggedValue CreateFromArrayBuffer(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                               const DataViewType arrayType);
    static JSTaggedValue CreateFromSendableArrayBuffer(EcmaRuntimeCallInfo *argv,
                                                       const JSHandle<JSObject> &obj,
                                                       const DataViewType arrayType);
    template <TypedArrayKind typedArrayKind = TypedArrayKind::NON_SHARED>
    static JSTaggedValue FastCopyElementFromArray(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                                         const DataViewType arrayType);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_H
