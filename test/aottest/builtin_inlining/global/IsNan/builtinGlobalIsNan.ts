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

function doIsNaN(x: any): any {
    return isNaN(x);
}

function printIsNaN(x: any) {
    try {
        print(doIsNaN(x));
    } finally {
    }
}

var x = "x"
x -= 1
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(x)) //: true

// Check with single int param
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(0)); //: false
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(3)); //: false
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(-5)); //: false

// Check with single float param
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(-1.5)); //: false
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(1.5)); //: false

// Check with special float params
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(Infinity)); //: false
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(-Infinity)); //: false
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(NaN)); //: true
//aot: [trace] aot inline builtin: Math.exp, caller function name:func_main_0@builtinGlobalIsNan
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(Math.exp(800))); //: false

// Check with 2 params
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(NaN, Infinity)); //: true

// Check with 3 params
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
print(isNaN(NaN, 3, Infinity)); //: true


// Replace standard builtin
let true_is_nan = isNaN
isNaN = replace
print(isNaN(NaN)); //: NaN
isNaN = true_is_nan

//aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
printIsNaN(-3);    //: false
//aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
//aot: [trace] Check Type: NotNumber1
printIsNaN("abc"); //: true
//aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
//aot: [trace] Check Type: NotNumber1
printIsNaN("abc"); //: true
//aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
printIsNaN(-12); //: false

// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
//aot: [trace] Check Type: NotNumber1
printIsNaN("abc"); //: true
//aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
//aot: [trace] Check Type: NotNumber1
printIsNaN("-12"); //: false

if (ArkTools.isAOTCompiled(doIsNaN)) {
    // Replace standard builtin after call to standard builtin was profiled
    isNaN = replace
}
printIsNaN(-12); //pgo: false
                 //aot: [trace] Check Type: NotCallTarget1
                 //aot: -12
printIsNaN("abc"); //pgo: true
                   //aot: [trace] Check Type: NotCallTarget1
                   //aot: abc
isNaN = true_is_nan

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
    printIsNaN(-12); //: false
    //aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
    printIsNaN(NaN); //: true
    //aot: [trace] aot inline builtin: isNan, caller function name:#*#doIsNaN@builtinGlobalIsNan
    //aot: [trace] Check Type: NotNumber1
    printIsNaN("abc"); //: true
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -23; })
//aot: [trace] aot inline builtin: isNan, caller function name:func_main_0@builtinGlobalIsNan
//aot: [trace] Check Type: NotNumber1
print(isNaN(obj)); //: false

function Throwing() {
    this.value = -14;
}
Throwing.prototype.valueOf = function() {
    if (this.value > 0) {
        throw new Error("already positive");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(isNaN(throwingObj)); //: false
    throwingObj.value = 10;
    print(isNaN(throwingObj)); //: Error: already positive
} catch(e) {
    print(e);
} finally {
    print(isNaN(obj)); //: false
}