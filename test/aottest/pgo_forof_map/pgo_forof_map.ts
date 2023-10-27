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

let r = 0
function foo(obj) {
    for (let value of obj) {
        if (typeof value[0] == "string"){
            r += value[1]
        }
    }
}
let p1 = new Array("apples", 1)
let p2 = new Array("bananas", 2)
let p3 = new Array("oranges", 3)
let a = new Array(p1, p2, p3)
let obj1 = new Map(a)
for (let i = 0; i < 100; i++) {
    foo(obj1)
}
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))
Object.prototype[Symbol.iterator] = { "next": function () { return {"value":1, "done":true} } }
print(ArkTools.isSymbolIteratorDetectorValid("Map"))
print(ArkTools.isSymbolIteratorDetectorValid("Set"))
print(ArkTools.isSymbolIteratorDetectorValid("Array"))
print(ArkTools.isSymbolIteratorDetectorValid("String"))
print(ArkTools.isSymbolIteratorDetectorValid("TypedArray"))
print(r)
