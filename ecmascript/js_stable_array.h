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

#include "ecmascript/js_array.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSStableArray {
public:
    enum SeparatorFlag : int { MINUS_ONE = -1, MINUS_TWO = -2 };
    static JSTaggedValue Push(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Splice(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                uint32_t start, uint32_t insertCount, uint32_t actualDeleteCount);
    static JSTaggedValue Shift(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
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
    static JSTaggedValue HandleforEachOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                               JSHandle<JSTaggedValue> callbackFnHandle,
                                               JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k);
    static JSTaggedValue IndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                 JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len);
    static JSTaggedValue LastIndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                 JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len);
    static JSTaggedValue Filter(JSHandle<JSObject> newArrayHandle, JSHandle<JSObject> thisObjHandle,
                                 EcmaRuntimeCallInfo *argv, uint32_t &k, uint32_t &toIndex);
    static JSTaggedValue Map(JSHandle<JSObject> newArrayHandle, JSHandle<JSObject> thisObjHandle,
                                 EcmaRuntimeCallInfo *argv, uint32_t &k, uint32_t len);
    static JSTaggedValue Reverse(JSThread *thread, JSHandle<JSObject> thisObjHandle, uint32_t len);
    static JSTaggedValue Concat(JSThread *thread, JSHandle<JSObject> newArrayHandle,
                                JSHandle<JSObject> thisObjHandle, int64_t &k, int64_t &n);
    static JSTaggedValue FastCopyFromArrayToTypedArray(JSThread *thread, JSHandle<JSTypedArray> &target,
                                                       DataViewType targetType, uint64_t targetOffset,
                                                       uint32_t srcLength, JSHandle<TaggedArray> &elements);
    static JSTaggedValue At(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue With(JSThread *thread, JSHandle<JSArray> receiver,
                              int64_t insertCount, int64_t index, JSHandle<JSTaggedValue> value);
    static JSTaggedValue ToSpliced(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                   int64_t argc, int64_t actualStart, int64_t actualSkipCount, int64_t insertCount);
    static JSTaggedValue ToReversed(JSThread *thread, JSHandle<JSArray> receiver,   int64_t insertCount);
    static JSTaggedValue Reduce(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                JSHandle<JSTaggedValue> callbackFnHandle,
                                JSMutableHandle<JSTaggedValue> accumulator, int64_t &k, int64_t &len);
    static JSTaggedValue Slice(JSThread *thread, JSHandle<JSObject> thisObjHandle, int64_t &k, int64_t &count);

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
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_STABLE_ARRAY_H
