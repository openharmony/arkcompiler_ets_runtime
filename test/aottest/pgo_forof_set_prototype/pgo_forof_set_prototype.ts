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

// @ts-nocheck
declare function print(arg:any):string;

let r = 0;
function foo(obj) {
    for (let value of obj) {
        r += value
    }
}
let a = new Array(1, 2, 3)
let obj1 = new Uint8Array(a);
for (let i = 0; i < 100; i++) {
    foo(obj1)
}
print("before change")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

let obj2 = "123";
obj2.__proto__ = {}
print("after change raw string.__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

let obj3 = new String("123")
obj3.__proto__ = {}
print("after change string.__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

let obj4 = new Array(1, 2, 3)
obj4.__proto__ = {}
print("after change array.__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

let temp = new Array()
let obj5 = new Map(temp)
obj5.__proto__ = {}
print("after change map.__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

let obj6 = new Set(temp)
obj6.__proto__ = {}
print("after change set.__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

let index = 0
Uint8Array.prototype.__proto__ = {[Symbol.iterator] : { "next": function () {
    if (index == 1) {
        return {"value":1, "done":true} 
    } else {
        index++
        return {"value":1, "done":false}
    }
}}}
for (let i = 0; i < 100; i++) {
    foo(obj1)
}
print("after change Uint8Array.prototype.__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))

obj1.__proto__ = {[Symbol.iterator] : { "next": function () { return {"value":1, "done":true} } }}
for (let i = 0; i < 100; i++) {
    foo(obj1)
}
print("after change obj(typed array).__proto__")
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))
print(r)