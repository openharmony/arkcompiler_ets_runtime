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
declare function print(arg:any) : string;
function foo1() {
    let s = 0;
    for (let i = 1; i <= 10; i++) {
        s += i;
    }
    return s;
}
print(foo1())

let arr = new Int32Array([1, 2, 3, 4]);
function foo2(u : Int32Array) {
    let s = 10;
    for (let i = 0; i < u.length; i++) {
        for (let j = 0; j < u.length; ++j) {
            s += u[i] * u[j];
        }
    }
    return s;
}
print(foo2(arr))

let arr2 = new Int32Array([0, 0, 0, 0]);
function foo3(u : Int32Array, v : Int32Array) {
    let s = 10;
    for (let i = 0; i < u.length; ++i) {
        for (let j = i; j < v.length; ++j) {
            for (let k = i; k < v.length; ++k) {
                u[i] += v[j] * v[k];
            }
        }
    }
    for (let i = 0; i < arr.length; ++i) {
        s += u[i];
    }
    return s;
}
print(foo3(arr2, arr));