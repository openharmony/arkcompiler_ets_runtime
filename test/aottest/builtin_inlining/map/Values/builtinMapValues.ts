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

function doValues(x : any) {
    return myMap.values(x);
}

function printValues(x : any) {
    try {
        print(doValues(x));
    } finally {
    }
}

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, 0.0], [56, "oops"], ["xyz", "12345"]]);

// Check without params
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
print(myMap.values()); //: [object Map Iterator]

// Check with single param
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
print(myMap.values(0).next().value); //: 5

// Check with 2 params
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
print(myMap.values(0, 0).next().value); //: 5

// Check with 3 params
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
print(myMap.values(-1, 10.2, 15).next().value); //: 5

// Check own methods
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
print(myMap.values().throw); //: function throw() { [native code] }
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
print(myMap.values().return); //: function return() { [native code] }

// Check using in loop
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
for (let key of myMap.values()) {
    print(key);
}
//: 5
//: 1
//: -2.5
//: Infinity
//: 0
//: oops
//: 12345

// Replace standard builtin
let true_values = myMap.values
myMap.values = replace

// no deopt
print(myMap.values(2.5)); //: 2.5
myMap.values = true_values

if (ArkTools.isAOTCompiled(printValues)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.values = replace
}
printValues(2.5); //pgo: [object Map Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printValues("abc"); //pgo: [object Map Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

myMap.values = true_values

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Map.values, caller function name:#*#doValues@builtinMapValues
    printValues(2.5); //: [object Map Iterator]
    //aot: [trace] aot inline builtin: Map.values, caller function name:#*#doValues@builtinMapValues
    printValues("abc"); //: [object Map Iterator]
} catch (e) {
}

// Check using in a loop
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
let iter1 = myMap.values();
for (let key of iter1) {
    print(key);
}
//: 5
//: 1
//: -2.5
//: Infinity
//: 0
//: oops
//: 12345

// Check reusing possibility
for (let key of iter1) {
    print(key);
} // <nothing>

// Check using out of boundaries
print(iter1.next().value); //: undefined

// Check using after inserting / deleting
//aot: [trace] aot inline builtin: Map.values, caller function name:func_main_0@builtinMapValues
let iter2 = myMap.values();
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapValues
myMap.delete(NaN);
myMap.set(2000, 1e-98);
myMap.set("xyz", -100);
for (let key of iter2) {
    print(key);
}
//: 5
//: 1
//: -2.5
//: 1e-98
//: oops
//: -100
