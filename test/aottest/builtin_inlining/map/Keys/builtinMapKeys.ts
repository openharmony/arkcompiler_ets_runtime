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

function doKeys(x : any) {
    return myMap.keys(x);
}

function printKeys(x : any) {
    try {
        print(doKeys(x));
    } finally {
    }
}

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, -0.0], [56, "oops"], ["xyz", "12345"]]);

// Check without params
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
print(myMap.keys()); //: [object Map Iterator]

// Check with single param
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
print(myMap.keys(0).next().value); //: 0

// Check with 2 params
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
print(myMap.keys(0, 0).next().value); //: 0

// Check with 3 params
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
print(myMap.keys(-1, 10.2, 15).next().value); //: 0

// Check own methods
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
print(myMap.keys().throw); //: function throw() { [native code] }
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
print(myMap.keys().return); //: function return() { [native code] }

// Check using in loop
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
for (let key of myMap.keys()) {
    print(key);
}
//: 0
//: -1
//: 2.5
//: NaN
//: 2000
//: 56
//: xyz

// Replace standard builtin
let true_keys = myMap.keys
myMap.keys = replace

// no deopt
print(myMap.keys(2.5)); //: 2.5
myMap.keys = true_keys

if (ArkTools.isAOTCompiled(printKeys)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.keys = replace
}
printKeys(2.5); //pgo: [object Map Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printKeys("abc"); //pgo: [object Map Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

myMap.keys = true_keys

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Map.keys, caller function name:#*#doKeys@builtinMapKeys
    printKeys(2.5); //: [object Map Iterator]
    //aot: [trace] aot inline builtin: Map.keys, caller function name:#*#doKeys@builtinMapKeys
    printKeys("abc"); //: [object Map Iterator]
} catch (e) {
}

// Check using in a loop
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
let iter1 = myMap.keys();
for (let key of iter1) {
    print(key);
}
//: 0
//: -1
//: 2.5
//: NaN
//: 2000
//: 56
//: xyz

// Check reusing possibility
for (let key of iter1) {
    print(key);
} // <nothing>

// Check using out of boundaries
print(iter1.next().value); //: undefined

// Check using after deleting
//aot: [trace] aot inline builtin: Map.keys, caller function name:func_main_0@builtinMapKeys
let iter2 = myMap.keys();
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapKeys
myMap.delete(NaN);
myMap.set("xyz", -100);
for (let key of iter2) {
    print(key);
}
//: 0
//: -1
//: 2.5
//: 2000
//: 56
//: xyz
