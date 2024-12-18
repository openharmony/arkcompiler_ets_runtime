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

#ifndef ECMASCRIPT_JS_STABLE_ARRAY_H
#define ECMASCRIPT_JS_STABLE_ARRAY_H

#include "ecmascript/base/array_helper.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSStableArray {
public:
#if !ENABLE_NEXT_OPTIMIZATION
    enum SeparatorFlag : int { MINUS_ONE = -1, MINUS_TWO = -2 };
#endif
    static JSTaggedValue Push(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Push(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Splice(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv, uint32_t start,
                                uint32_t insertCount, uint32_t actualDeleteCount,
                                JSHandle<JSObject> newArrayHandle, uint32_t len);
    static JSTaggedValue Splice(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv, uint32_t start,
                                uint32_t insertCount, uint32_t actualDeleteCount,
                                JSHandle<JSObject> newArrayHandle, uint32_t len);
    static JSTaggedValue Shift(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Shift(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Join(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HandleFindIndexOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                 JSHandle<JSTaggedValue> callbackFnHandle,
                                                 JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k);
    static JSTaggedValue HandleFindLastIndexOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                     JSHandle<JSTaggedValue> callbackFnHandle,
                                                     JSHandle<JSTaggedValue> thisArgHandle, int64_t &k);
    static JSTaggedValue HandleEveryOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                             JSHandle<JSTaggedValue> callbackFnHandle,
                                             JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k);
    static JSTaggedValue HandleSomeOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                             JSHandle<JSTaggedValue> callbackFnHandle,
                                             JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k);
    static JSTaggedValue HandleforEachOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                               JSHandle<JSTaggedValue> callbackFnHandle,
                                               JSHandle<JSTaggedValue> thisArgHandle, uint32_t len, uint32_t &k);
    static JSTaggedValue IndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                 JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len);
    static JSTaggedValue LastIndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                 JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len);
    static JSTaggedValue Filter(JSHandle<JSObject> newArrayHandle, JSHandle<JSObject> thisObjHandle,
                                 EcmaRuntimeCallInfo *argv, uint32_t &k, uint32_t &toIndex);
    static JSTaggedValue Map(JSHandle<JSObject> newArrayHandle, JSHandle<JSObject> thisObjHandle,
                                 EcmaRuntimeCallInfo *argv, uint32_t &k, uint32_t len);
    static JSTaggedValue Reverse(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                 int64_t &lower, uint32_t len);
    static JSTaggedValue FastReverse(JSThread *thread, JSHandle<TaggedArray> elements,
                                     int64_t &lower, uint32_t len, ElementsKind kind);
    static JSTaggedValue Concat(JSThread *thread, JSHandle<JSObject> newArrayHandle,
                                JSHandle<JSObject> thisObjHandle, int64_t &k, int64_t &n);
    template<base::TypedArrayKind typedArrayKind = base::TypedArrayKind::NON_SHARED>
    static JSTaggedValue FastCopyFromArrayToTypedArray(JSThread *thread, JSHandle<JSTypedArray> &target,
                                                       DataViewType targetType, uint64_t targetOffset,
                                                       uint32_t srcLength, JSHandle<JSObject> &obj);
    static JSTaggedValue At(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue At(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue With(JSThread *thread, JSHandle<JSArray> receiver,
                              int64_t insertCount, int64_t index, JSHandle<JSTaggedValue> value);
    static JSTaggedValue ToSpliced(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                   int64_t argc, int64_t actualStart, int64_t actualSkipCount, int64_t insertCount);
    static JSTaggedValue ToReversed(JSThread *thread, JSHandle<JSArray> receiver, int64_t insertCount);
    static JSTaggedValue Reduce(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                JSHandle<JSTaggedValue> callbackFnHandle,
                                JSMutableHandle<JSTaggedValue> accumulator, int64_t &k, int64_t &len);
    static JSTaggedValue Slice(JSThread *thread, JSHandle<JSObject> thisObjHandle, int64_t &k, int64_t &count);
    static JSHandle<TaggedArray> SortIndexedProperties(JSThread *thread, const JSHandle<JSTaggedValue> &thisObj,
                                                       int64_t len, const JSHandle<JSTaggedValue> &callbackFnHandle,
                                                       base::HolesType holes);
    static JSTaggedValue Sort(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                              const JSHandle<JSTaggedValue> &callbackFnHandle);
    static JSTaggedValue CopySortedListToReceiver(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                                                  JSHandle<TaggedArray> sortedList, uint32_t len);
    static JSTaggedValue Fill(JSThread *thread, const JSHandle<JSObject> &thisObj,
                              const JSHandle<JSTaggedValue> &value,
                              int64_t start, int64_t end, int64_t len);
    static JSTaggedValue HandleFindLastOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                JSHandle<JSTaggedValue> callbackFnHandle,
                                                JSHandle<JSTaggedValue> thisArgHandle,
                                                JSMutableHandle<JSTaggedValue> &kValue, int64_t &k);
    static JSTaggedValue HandleReduceRightOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                   JSHandle<JSTaggedValue> callbackFnHandle,
                                                   JSMutableHandle<JSTaggedValue> &accumulator,
                                                   JSHandle<JSTaggedValue> thisArgHandle, int64_t &k);

private:
    enum class IndexOfType {
        IndexOf,
        LastIndexOf
    };

    struct IndexOfContext {
        JSThread *thread;
        JSHandle<JSTaggedValue> receiver;
        JSHandle<JSTaggedValue> searchElement;
        uint32_t fromIndex;
        uint32_t length;
    };

    template <class Predicate>
    static JSTaggedValue FindRawData(IndexOfContext &ctx, Predicate &&predicate);
    template <class Predicate>
    static JSTaggedValue FindLastRawData(IndexOfContext &ctx, Predicate &&predicate);
    template <class Predicate>
    static JSTaggedValue FindRawDataDispatch(IndexOfType type, IndexOfContext &ctx, Predicate &&predicate);

    static JSTaggedValue IndexOfZero(IndexOfType type, IndexOfContext &ctx);
    static JSTaggedValue IndexOfInt32(IndexOfType type, IndexOfContext &ctx, JSTaggedValue searchElement);
    static JSTaggedValue IndexOfDouble(IndexOfType type, IndexOfContext &ctx, JSTaggedValue searchElement);
    static JSTaggedValue IndexOfObjectAddress(IndexOfType type, IndexOfContext &ctx, JSTaggedValue searchElement);
    static JSTaggedValue IndexOfString(IndexOfType type, IndexOfContext &ctx, JSTaggedValue searchElement);
    static JSTaggedValue IndexOfBigInt(IndexOfType type, IndexOfContext &ctx, JSTaggedValue searchElement);
    static JSTaggedValue IndexOfDispatch(IndexOfType type, IndexOfContext &ctx, JSTaggedValue searchElement);
    static JSTaggedValue UpdateArrayCapacity(JSHandle<JSObject> &thisObjHandle, uint32_t &len,
                                             uint32_t &insertCount, uint32_t &actualDeleteCount,
                                             JSHandle<JSArray> &receiver, uint32_t &start,
                                             JSThread *thread, bool &needTransition,
                                             JSHandle<JSTaggedValue> &holeHandle,
                                             EcmaRuntimeCallInfo *argv, JSHandle<JSTaggedValue> &thisObjVal,
                                             JSHandle<JSTaggedValue> &lengthKey);
    static void HandleArray(JSHandle<JSObject> &newArrayHandle, uint32_t &actualDeleteCount,
                            JSThread *thread, uint32_t &start, JSHandle<JSObject> &thisObjHandle,
                            JSHandle<JSTaggedValue> &holeHandle);

#if !ENABLE_NEXT_OPTIMIZATION
    static void SetSepValue(JSHandle<EcmaString> sepStringHandle, int &sep, uint32_t &sepLength);
    static JSTaggedValue JoinUseTreeString(const JSThread *thread,
                                           const JSHandle<JSTaggedValue> receiverValue,
                                           const JSHandle<EcmaString> sepStringHandle, const int sep,
                                           CVector<JSHandle<EcmaString>> &vec);
    inline static bool WorthUseTreeString(int sep, size_t allocateLength, uint32_t len);
#endif

    // Allocate object larger than 256 need liner search in the free object list,
    // so try to use tree string when the join result is larger than 256.
    static constexpr size_t TREE_STRING_THRESHOLD = 256;
    static constexpr size_t NUM_2 = 2;

#if ENABLE_NEXT_OPTIMIZATION
    // When Array length is no more than 64, use array (stack memory) instead of vector to store the elements.
    static constexpr size_t USE_STACK_MEMORY_THRESHOLD = 64;
    inline static bool WorthUseTreeString(uint32_t sepLength, size_t allocateLength, uint32_t len);
    template <typename Container>
    static void ProcessElements(JSThread *thread, JSHandle<JSTaggedValue> receiverValue, uint32_t len,
                                Container &arrElements, bool &isOneByte, uint64_t &allocateLength);
    template <typename Container>
    static JSTaggedValue DoStableArrayJoin(JSThread *thread, JSHandle<JSTaggedValue> receiverValue, uint32_t len,
                                           Container &arrElements, bool &isOneByte, uint32_t sep,
                                           uint32_t sepLength, JSHandle<EcmaString> sepStringHandle);
    template <typename Container>
    static JSTaggedValue JoinUseTreeString(const JSThread *thread, JSHandle<JSTaggedValue> receiverValue,
                                           JSHandle<EcmaString> sepStringHandle, uint32_t sepLength,
                                           Container &arrElements, uint32_t elemNum);
#endif
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_STABLE_ARRAY_H
