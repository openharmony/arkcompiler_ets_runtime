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

declare function print(str:any):string;
declare function assert_true(condition: boolean):void;
declare function assert_equal(a: Object, b: Object):void;

print("test callthis1")
assert_equal(Math.sqrt(9), 3)
assert_equal(Math.sin(9), 0.4121184852417566)
assert_equal(Math.cos(9), -0.9111302618846769)
assert_true(isNaN(Math.acos(9)))
assert_equal(Math.atan(9), 1.460139105621001)
assert_equal(Math.abs(-9), 9)
assert_equal(Math.floor(9.1), 9)
assert_equal(Math.ceil(9.1), 10)

print("test call1")
let func = Math.sqrt
assert_equal(func(9), 3)
func = Math.sin
assert_equal(func(9), 0.4121184852417566)
func = Math.cos
assert_equal(func(9), -0.9111302618846769)
func = Math.acos
assert_true(isNaN(func(9)))
func = Math.atan
assert_equal(func(9), 1.460139105621001)
func = Math.abs
assert_equal(func(-9), 9)
func = Math.floor
assert_equal(func(9.1), 9)
func = Math.ceil
assert_equal(func(9.1), 10)

print("test localeCompare")
let str1 = "Straße"
let str2 = "Strasse"
assert_equal(str1.localeCompare(str2, "de", { sensitivity: "base" }), 0)

let a = new Array(20)
for (let i = 0; i < 20; i++) {
    a[i] = Math.random()
}
a.sort()

let b = new Array(1, 2, 5.2, 5,1, 3, 4, 5, 6, 9, 3.1, 3.2, 4)
b.sort()
assert_equal(b, [1,1,2,3,3.1,3.2,4,4,5,5,5.2,6,9])