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

print("test callthis1")
print(Math.sqrt(9))
print(Math.sin(9))
print(Math.cos(9))
print(Math.acos(9))
print(Math.atan(9))
print(Math.abs(-9))
print(Math.floor(9.1))
print(Math.ceil(9.1))

print("test call1")
let func = Math.sqrt
print(func(9))
func = Math.sin
print(func(9))
func = Math.cos
print(func(9))
func = Math.acos
print(func(9))
func = Math.atan
print(func(9))
func = Math.abs
print(func(-9))
func = Math.floor
print(func(9.1))
func = Math.ceil
print(func(9.1))

print("test localeCompare")
let str1 = "Straße"
let str2 = "Strasse"
print(str1.localeCompare(str2, "de", { sensitivity: "base" }))

let a = new Array(20)
for (let i = 0; i < 20; i++) {
    a[i] = Math.random()
}
a.sort()

let b = new Array(1, 2, 5.2, 5,1, 3, 4, 5, 6, 9, 3.1, 3.2, 4)
b.sort()
print(b)