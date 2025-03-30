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
    return myArray.values(x);
}

function printItems(x: any) {
    try {
        print(getItems(x));
    } finally {
    }
}

let myArray = new Uint8Array([10, 20, 30]);

//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
let items0 = myArray.values();
print(items0); //: [object Array Iterator]

//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
let items1 = myArray.values();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
print(items1.next().value); //: 10

//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
let items2 = myArray.values();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
items2.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
print(items2.next().value); //: 20

//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
let items3 = myArray.values();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
items3.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
items3.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
print(items3.next().value); //: 30

//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
let items4 = myArray.values();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
items4.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
items4.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
items4.next();
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
print(items4.next().value); //: undefined

// Check own methods
//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
print(myArray.values().throw); //: function throw() { [native code] }
//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
print(myArray.values().return); //: function return() { [native code] }

// Check using in loop
//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
for (let item of myArray.values()) {
    print(item);
}
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
//: 10
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
//: 20
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
//: 30
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues

// Replace standard builtin
let restore = myArray.values
myArray.values = replace

// no deopt
print(myArray.values(2.5)); //: 2.5
myArray.values = restore

if (ArkTools.isAOTCompiled(printItems)) {
    // Replace standard builtin after call to standard builtin was profiled
    myArray.values = replace
}
//aot: [trace] Check Type: NotCallTarget1
printItems(2.5); //aot: 2.5
                 //pgo: [object Array Iterator]

//aot: [trace] Check Type: NotCallTarget1
printItems("abc"); //aot: abc
                   //pgo: [object Array Iterator]

myArray.values = restore

// Check IR correctness inside try-block
try {
    //aot: [trace] Check Type: NotCallTarget1
    printItems(2.5); //: [object Array Iterator]
    //aot: [trace] Check Type: NotCallTarget1
    printItems("abc"); //: [object Array Iterator]
} catch (e) {
}

// Check using out of boundaries
//aot: [trace] aot inline builtin: TypedArray.values, caller function name:func_main_0@builtinTypedArrayValues
let items5 = myArray.values();
for (let item of items5) {
    print(item);
}
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
//: 10
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
//: 20
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
//: 30
//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues

// Check reusing possibility
for (let item of items5) {
    //aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
    print(item);
} // <nothing>

//aot: [trace] aot call builtin: ArrayIterator.next, caller function name:func_main_0@builtinTypedArrayValues
print(items5.next().value); //: undefined
