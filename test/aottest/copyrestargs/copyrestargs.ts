/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function assert_equal(a: Object, b: Object):void;
var testArrary = [2, 3, "success", "fail"];
function f(a:any,...A:any) {
    for (let p in A) {
        A[p] = testArrary[p];
    }
}

f(1, [2, 3, "success", "fail"]);

// The following test cases have exposed a bug: the variable actualRestNum in RuntimeOptCopyRestArgs
// computed mistakely and may out of uint32_t range.
function foo(x: number, y?: number, ...restArgs: number[]):void {
    let arr = [...restArgs];
    assert_equal(arr.length, 0);
}

foo(1);
