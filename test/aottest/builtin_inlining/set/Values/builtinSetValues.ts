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
    return mySet.values(x);
}

function printValues(x : any) {
    try {
        print(doValues(x));
    } finally {
    }
}

let mySet = new Set([0, 0.0, 5, -200.5, 1e-78, NaN, "xyz", "12345"]);

// Check without params
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
print(mySet.values()); //: [object Set Iterator]

// Check with single param
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
print(mySet.values(0).next().value); //: 0

// Check with 2 params
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
print(mySet.values(0, 0).next().value); //: 0

// Check with 3 params
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
print(mySet.values(-1, 10.2, 15).next().value); //: 0

// Check own methods
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
print(mySet.values().throw); //: function throw() { [native code] }
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
print(mySet.values().return); //: function return() { [native code] }

// Check using in loop
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
for (let key of mySet.values()) {
    print(key);
}
//: 0
//: 5
//: -200.5
//: 1e-78
//: NaN
//: xyz
//: 12345

// Replace standard builtin
let true_values = mySet.values
mySet.values = replace

// no deopt
print(mySet.values(2.5)); //: 2.5
mySet.values = true_values

if (ArkTools.isAOTCompiled(printValues)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.values = replace
}
printValues(2.5); //pgo: [object Set Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printValues("abc"); //pgo: [object Set Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

mySet.values = true_values

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Set.values, caller function name:#*#doValues@builtinSetValues
    printValues(2.5); //: [object Set Iterator]
    //aot: [trace] aot inline builtin: Set.values, caller function name:#*#doValues@builtinSetValues
    printValues("abc"); //: [object Set Iterator]
} catch (e) {
}

// Check using out of boundaries
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
let iter1 = mySet.values();

for (let key of iter1) {
    print(key);
}
//: 0
//: 5
//: -200.5
//: 1e-78
//: NaN
//: xyz
//: 12345

// Check reusing possibility
for (let key of iter1) {
    print(key);
} // <nothing>

print(iter1.next().value); //: undefined

// Check using after inserting / deleting
//aot: [trace] aot inline builtin: Set.values, caller function name:func_main_0@builtinSetValues
let iter2 = mySet.values();
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetValues
mySet.delete(5);
mySet.add(30);  //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetValues
mySet.add(NaN); //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetValues
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetValues
mySet.delete("12345");
for (let key of iter2) {
    print(key);
}
//: 0
//: -200.5
//: 1e-78
//: NaN
//: xyz
//: 30
