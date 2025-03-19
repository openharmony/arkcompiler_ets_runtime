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

function doEntries(x : any) {
    return mySet.entries(x);
}

function printEntries(x : any) {
    try {
        print(doEntries(x));
    } finally {
    }
}

let mySet = new Set([0, 0.0, 5, -200.5, 1e-78, NaN, "12345", ", ."]);

// Check without params
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
print(mySet.entries()); //: [object Set Iterator]

// Check with single param
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
print(mySet.entries(0).next().value); //: 0,0

// Check with 2 params
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
print(mySet.entries(0, 0).next().value); //: 0,0

// Check with 3 params
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
print(mySet.entries(-1, 10.2, 15).next().value); //: 0,0

// Check own methods
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
print(mySet.entries().throw); //: function throw() { [native code] }
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
print(mySet.entries().return); //: function return() { [native code] }

// Check using in loop
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
for (let key of mySet.entries()) {
    print(key);
}
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 0,0
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 5,5
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: -200.5,-200.5
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 1e-78,1e-78
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: NaN,NaN
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 12345,12345
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: , .,, .
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries

// Replace standard builtin
let true_entries = mySet.entries
mySet.entries = replace

// no deopt
print(mySet.entries(2.5)); //: 2.5
mySet.entries = true_entries

if (ArkTools.isAOTCompiled(printEntries)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.entries = replace
}
printEntries(2.5); //pgo: [object Set Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printEntries("abc"); //pgo: [object Set Iterator]
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

mySet.entries = true_entries

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Set.entries, caller function name:#*#doEntries@builtinSetEntries
    printEntries(2.5); //: [object Set Iterator]
    //aot: [trace] aot inline builtin: Set.entries, caller function name:#*#doEntries@builtinSetEntries
    printEntries("abc"); //: [object Set Iterator]
} catch (e) {
}

// Check using out of boundaries
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
let iter1 = mySet.entries();

for (let key of iter1) {
    print(key);
}
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 0,0
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 5,5
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: -200.5,-200.5
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 1e-78,1e-78
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: NaN,NaN
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 12345,12345
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: , .,, .
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries

// Check reusing possibility
for (let key of iter1) {
    //aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
    print(key);
} // <nothing>

//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
print(iter1.next().value);
//: undefined

// Check using after inserting / deleting
//aot: [trace] aot inline builtin: Set.entries, caller function name:func_main_0@builtinSetEntries
let iter2 = mySet.entries();
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetEntries
mySet.delete(5);
mySet.add(30);  //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetEntries
mySet.add(NaN); //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetEntries
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetEntries
mySet.delete("12345");
for (let key of iter2) {
    print(key);
}
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 0,0
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: -200.5,-200.5
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 1e-78,1e-78
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: NaN,NaN
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: , .,, .
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
//: 30,30
//aot: [trace] aot call builtin: SetIterator.next, caller function name:func_main_0@builtinSetEntries
