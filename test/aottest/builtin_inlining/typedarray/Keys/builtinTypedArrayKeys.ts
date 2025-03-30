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

declare function print(arg: any): string;

function replace(a: any)
{
    return a;
}

function getItems(x: any) {
    return myArray.keys(x);
}

function printItems(x: any) {
    try {
        print(getItems(x));
    } finally {
    }
}

let myArray = new Uint8Array([10, 20, 30]);

//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
let items0 = myArray.keys();
print(items0); //: [object Array Iterator]

//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
let items1 = myArray.keys();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
print(items1.next().value); //: 0

//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
let items2 = myArray.keys();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
items2.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
print(items2.next().value); //: 1

//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
let items3 = myArray.keys();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
items3.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
items3.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
print(items3.next().value); //: 2

//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
let items4 = myArray.keys();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
items4.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
items4.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
items4.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
print(items4.next().value); //: undefined

// Check own methods
//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
print(myArray.keys().throw); //: function throw() { [native code] }
//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
print(myArray.keys().return); //: function return() { [native code] }

// Check using in loop
//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
for (let item of myArray.keys()) {
    print(item);
}
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
//: 0
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
//: 1
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
//: 2
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys

// Replace standard builtin
let restore = myArray.keys
myArray.keys = replace

// no deopt
print(myArray.keys(2.5)); //: 2.5
myArray.keys = restore

if (ArkTools.isAOTCompiled(printItems)) {
    // Replace standard builtin after call to standard builtin was profiled
    myArray.keys = replace
}
//aot: [trace] Check Type: NotCallTarget1
printItems(2.5); //aot: 2.5
                 //pgo: [object Array Iterator]

//aot: [trace] Check Type: NotCallTarget1
printItems("abc"); //aot: abc
                   //pgo: [object Array Iterator]

myArray.keys = restore

// Check IR correctness inside try-block
try {
    //aot: [trace] Check Type: NotCallTarget1
    printItems(2.5); //: [object Array Iterator]
    //aot: [trace] Check Type: NotCallTarget1
    printItems("abc"); //: [object Array Iterator]
} catch (e) {
}

// Check using out of boundaries
//aot: [trace] aot inline builtin: TypedArray.keys, caller function name:func_main_0@builtinTypedArrayKeys
let items5 = myArray.keys();
for (let item of items5) {
    print(item);
}
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
//: 0
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
//: 1
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
//: 2
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys

// Check reusing possibility
for (let item of items5) {
    //aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
    print(item);
} // <nothing>

//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayKeys
print(items5.next().value); //: undefined
