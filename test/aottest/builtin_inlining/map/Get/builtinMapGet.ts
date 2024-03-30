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

function doGet(x: any): any {
    return myMap.get(x);
}

function printGet(x: any) {
    try {
        print(doGet(x));
    } finally {
    }
}

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, -0.0], [56, "oops"], ["xyz", "12345"]]);

// Check without params
print(myMap.get()); //: undefined

// Check with adding element undefined
myMap.set(undefined, 42);
print(myMap.get()); //: 42

// Check with single param
print(myMap.get(0)); //: 5
print(myMap.get(3)); //: undefined
print(myMap.get(2.5)); //: -2.5
print(myMap.get(NaN)); //: Infinity
print("1/x: " + 1 / myMap.get(2000)); //: 1/x: -Infinity

// Check with 2 params
print(myMap.get(0, 0)); //: 5

// Check with 3 params
print(myMap.get(-1, 10.2, 15)); //: 1

// Check with 4 params
print(myMap.get(2.5, -800, 0.56, 0)); //: -2.5

// Check after inserting elements
myMap.set(2000, 1e-98);
myMap.set(133.33, -1);
print(myMap.get(2000)); //: 1e-98
print(myMap.get(133.33)); //: -1

// Replace standard builtin
let true_get = myMap.get
myMap.get = replace

// no deopt
print(myMap.get(2.5)); //: 2.5
myMap.get = true_get

printGet(-1); //: 1
// Call standard builtin with non-number param
printGet("abc"); //: undefined
printGet("-1"); //: undefined
printGet(56); //: oops
printGet("xyz"); //: 12345

if (ArkTools.isAOTCompiled(printGet)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.get = replace
}
printGet(2.5); //pgo: -2.5
//aot: 2.5

printGet("abc"); //pgo: undefined
//aot: abc

myMap.get = true_get

// Check IR correctness inside try-block
try {
    printGet(2.5); //: -2.5
    printGet("abc"); //: undefined
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })
print(myMap.get(obj)); //: undefined

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
    print(myMap.get(throwingObj)); //: undefined
    throwingObj.value = 2.5;
    print(myMap.get(throwingObj)); //: undefined
} catch(e) {
    print(e);
} finally {
    print(myMap.get(obj)); //: undefined
}

// Check after clearing
myMap.clear();
print(myMap.get(2000)); //: undefined