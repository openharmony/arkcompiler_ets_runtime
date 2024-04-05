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

#ifndef ECMASCRIPT_BASE_ARRAY_HELPER_H
#define ECMASCRIPT_BASE_ARRAY_HELPER_H

#include <string>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::base {
struct FlattenArgs {
    int64_t sourceLen = 0;
    int64_t start = 0;
    double depth = 0;
};

enum class HolesType {
    SKIP_HOLES,
    READ_THROUGH_HOLES,
};
class ArrayHelper {
public:
    // Common subprocedure for Array.prototype.at, Array.prototype.indexOf, Array.prototype.slice, etc.
    // Gets start index that falls in range [0, length].
    // length is returned on pending exception.
    static int64_t GetStartIndex(JSThread *thread, const JSHandle<JSTaggedValue> &startIndexHandle,
                                 int64_t length);
    // If argIndex is out of range [0, argc), then start index = 0 by default.
    // Otherwise, let startIndexHandle = GetCallArg(argv, argIndex) and call GetStartIndex.
    static int64_t GetStartIndexFromArgs(JSThread *thread, EcmaRuntimeCallInfo *argv,
                                         uint32_t argIndex, int64_t length);
    // Common subprocedure for Array.prototype.lastIndexOf, etc.
    // Gets last start index that falls in range [-1, length - 1].
    // -1 is returned on pending exception.
    static int64_t GetLastStartIndex(JSThread *thread, const JSHandle<JSTaggedValue> &startIndexHandle,
                                     int64_t length);
    // If argIndex is out of range [0, argc), then start index = length - 1 by default.
    // Otherwise, let startIndexHandle = GetCallArg(argv, argIndex) and call GetLastStartIndex.
    static int64_t GetLastStartIndexFromArgs(JSThread *thread, EcmaRuntimeCallInfo *argv,
                                             uint32_t argIndex, int64_t length);
    // Let thisHandle be the array object. Checks whether array[key] (if exists) is strictly equal to target.
    // Returns false on pending exception.
    static bool ElementIsStrictEqualTo(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                       const JSHandle<JSTaggedValue> &keyHandle,
                                       const JSHandle<JSTaggedValue> &target);

    static bool IsConcatSpreadable(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static double SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                              const JSHandle<JSTaggedValue> &valueX, const JSHandle<JSTaggedValue> &valueY);
    static double StringSortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &valueX,
                                    const JSHandle<JSTaggedValue> &valueY);
    static PUBLIC_API int64_t GetLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static int64_t GetArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue FlattenIntoArray(JSThread *thread, const JSHandle<JSObject> &newArrayHandle,
                                          const JSHandle<JSTaggedValue> &thisObjVal, const FlattenArgs &args,
                                          const JSHandle<JSTaggedValue> &mapperFunctionHandle,
                                          const JSHandle<JSTaggedValue> &thisArg);
    static JSHandle<TaggedArray> SortIndexedProperties(JSThread *thread, const JSHandle<JSTaggedValue> &thisObj,
                                                       int64_t len, const JSHandle<JSTaggedValue> &callbackFnHandle,
                                                       HolesType holes);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_ARRAY_HELPER_H
