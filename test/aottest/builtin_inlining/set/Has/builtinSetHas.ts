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

function doHas(x: any) {
    return mySet.has(x);
}

function printHas(x: any) {
    try {
        print(doHas(x));
    } finally {
    }
}

let mySet = new Set([0, 0.0, -5, 2.5, 1e-78, NaN, "xyz", "12345"]);

// Check without params
print(mySet.has()); //: false

// Check with adding element undefined
mySet.add(undefined);
print(mySet.has()); //: true

// Check with single param
print(mySet.has(0)); //: true
print(mySet.has(3)); //: false
print(mySet.has(2.5)); //: true
print(mySet.has(NaN)); //: true

// Check with 2 params
print(mySet.has(0, 0)); //: true

// Check with 3 params
print(mySet.has(-21, 10.2, 15)); //: false

// Check with 4 params
print(mySet.has(2.5, -800, 0.56, 0)); //: true

// Check after inserting elements
mySet.add(-5);
mySet.add(133.33);
print(mySet.has(-5)); //: true
print(mySet.has(133.33)); //: true

// Replace standard builtin
let true_has = mySet.has
mySet.has = replace

// no deopt
print(mySet.has(2.5)); //: 2.5
mySet.has = true_has

printHas(-5); //: true
// Call standard builtin with non-number param
printHas("abc"); //: false
printHas("-5"); //: false
printHas("xyz"); //: true

if (ArkTools.isAOTCompiled(printHas)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.has = replace
}
printHas(2.5); //pgo: true
//aot: 2.5

printHas("abc"); //pgo: false
//aot: abc

mySet.has = true_has

// Check IR correctness inside try-block
try {
    printHas(NaN); //: true
    printHas("abc"); //: false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })
print(mySet.has(obj)); //: false

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
    print(mySet.has(throwingObj)); //: false
    throwingObj.value = 2.5;
    print(mySet.has(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
    print(mySet.has(obj)); //: false
}

// Check after clearing
mySet.clear();
print(mySet.has(0)); //: false
