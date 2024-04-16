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

let arr = [1, 2, 3, 4, 5, 6, 7, 8];
arr.splice(0, 1, 9)
print(arr)
arr.splice(10, 1)
print(arr)
arr = [1, 2, 3, 4, 5, 6, 7, 8];
arr.splice(8, 0, 1, 1)
print(arr)
arr.splice(0, 8, 1, 1)
print(arr)
arr = [1, 2, 3, 4, 5, 6, 7, 8];
arr.splice(1, 8)
print(arr)
arr = new Array(100)
arr.splice(0, 20)
print(arr.length)

var array = new Array(10);
var spliced = array.splice(1, 1);
for (let i = 0; i < array.length; ++i) {
    print(i in array)
}
var array1 = new Array(10);
var spliced1 = array1.splice(1, 1, "a", "b");
for (let i = 0; i < array1.length; ++i) {
    print(i in array1)
}

array = [];
spliced = undefined;
for (var i = 0; i < 1; i++) {
    let bad_start = { valueOf: function () { array.push(2 * i); return -1; } };
    let bad_count = { valueOf: function () { array.push(2 * i + 1); return 1; } };
    spliced = array.splice(bad_start, bad_count);
    print("array.length", array.length);
}
for (let i = 0; i < 2; i++) {
    let array = [1, 2, 3, 4];
    print("array:", array);
    if (i == 0) {
        let spliced = array.splice(1, 3, 'one', 'two', 'three');
    }
}

let a = 0;
class C3 extends Array {
    constructor(a5, a6, a7, a8) {
        a++;
        super(-9223372036854775808, Set);
        if (a == 1) {
            this.splice(Array);
        }
        return "n"
    }
}
try { new C3(C3, Array, Array, C3); } catch (err) {print(err)};