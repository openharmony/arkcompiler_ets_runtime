/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;
function replace(a : number)
{
    return a;
}

function doHas(x: any): any {
    return myMap.has(x);
}

function printHas(x: any) {
    try {
        print(doHas(x));
    } finally {
    }
}

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, -0.0], [56, "oops"], ["xyz", "12345"]]);

// Check without params
print(myMap.has()); //: false

// Check with adding element undefined
myMap.set(undefined, 42);
print(myMap.has()); //: true

// Check with single param
print(myMap.has(0)); //: true
print(myMap.has(3)); //: false
print(myMap.has(2.5)); //: true
print(myMap.has(NaN)); //: true

// Check with 2 params
print(myMap.has(0, 0)); //: true

// Check with 3 params
print(myMap.has(-21, 10.2, 15)); //: false

// Check with 4 params
print(myMap.has(2.5, -800, 0.56, 0)); //: true

// Check after inserting elements
myMap.set(2000, 1e-98);
myMap.set(133.33, -1);
print(myMap.has(2000)); //: true
print(myMap.has(133.33)); //: true

// Replace standard builtin
let true_has = myMap.has
myMap.has = replace

// no deopt
print(myMap.has(2.5)); //: 2.5
myMap.has = true_has

printHas(-1); //: true
// Call standard builtin with non-number param
printHas("abc"); //: false
printHas("-1"); //: false
printHas(56); //: true
printHas("xyz"); //: true

if (ArkTools.isAOTCompiled(printHas)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.has = replace
}
printHas(2.5); //pgo: true
//aot: 2.5

printHas("abc"); //pgo: false
//aot: abc

myMap.has = true_has

// Check IR correctness inside try-block
try {
    printHas(2000); //: true
    printHas("abc"); //: false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })
print(myMap.has(obj)); //: false

function Throwing() {
    this.value = -1;
}
Throwing.prototype.valueOf = function() {
    if (this.value > 0) {
        throw new Error("already positive");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(myMap.has(throwingObj)); //: false
    throwingObj.value = 2.5;
    print(myMap.has(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
    print(myMap.has(obj)); //: false
}

// Check after clearing
myMap.clear();
print(myMap.has(2000)); //: false
