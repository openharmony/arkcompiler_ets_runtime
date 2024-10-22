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

declare function print(arg : any) : string;
declare interface ArkTools {
    isAOTCompiled(args : any) : boolean;
}

function replace(a : number)
{
    return a;
}

function doTrunc(x : any) : number {
    return Math.trunc(x);
}

function printTrunc(x : any) {
    try {
        print(doTrunc(x));
    } finally {
    }
}

// Check without params
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc()); //: NaN

// Check with special float params
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(-Infinity)); //: -Infinity
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(+0)); //: 0
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print("1/x: " + 1 / Math.trunc(-0)); //: 1/x: -Infinity

// Check with single integer param
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(1)); //: 1
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(-12)); //: -12

// Check with single float param
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(1.15613251)); //: 1
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(2.5)); //: 2
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(3.84556546)); //: 3
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(-1.124212)); //: -1
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print("1/x: " + 1 / Math.trunc(-8.5e-80)); //: 1/x: -Infinity
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(-4.5)); //: -4

// Check with 2 params
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(2.4, 10.5)); //: 2

// Check with 3 params
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(3.123, 10, 1e-39)); //: 3

// Check with 4 params
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
print(Math.trunc(4.89, 10.5, 0, 11)); //: 4

// Replace standart builtin
let true_trunc = Math.trunc
Math.trunc = replace
print(Math.trunc(111.09)); //: 111.09
Math.trunc = true_trunc

// Call standart builtin with non-number param
//aot: [trace] aot inline builtin: Math.trunc, caller function name:#*#doTrunc@builtinMathTrunc
//aot: [trace] Check Type: NotNumber2
printTrunc("abc"); //: NaN
//aot: [trace] aot inline builtin: Math.trunc, caller function name:#*#doTrunc@builtinMathTrunc
//aot: [trace] Check Type: NotNumber2
printTrunc("2.45"); //: 2

if (ArkTools.isAOTCompiled(printTrunc)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.trunc = replace
}
printTrunc(-12.1); //pgo: -12
//aot: [trace] Check Type: NotCallTarget1
//aot: -12.1

printTrunc("abc"); //pgo: NaN
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

Math.trunc = true_trunc

// Checl IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.trunc, caller function name:#*#doTrunc@builtinMathTrunc
    printTrunc(-48.12); //: -48
    //aot: [trace] aot inline builtin: Math.trunc, caller function name:#*#doTrunc@builtinMathTrunc
    //aot: [trace] Check Type: NotNumber2
    printTrunc("abc"); //: NaN
} catch (e) {
}

let obj = {
    valueOf: () => { return -35.121; }
};
//aot: [trace] aot inline builtin: Math.trunc, caller function name:func_main_0@builtinMathTrunc
//aot: [trace] Check Type: NotNumber2
print(Math.trunc(obj)); //: -35

function Throwing() {
    this.value = -14.12;
}
Throwing.prototype.valueOf = function() {
    if (this.value > 0) {
        throw new Error("already positive");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(Math.trunc(throwingObj)); //: -14
    throwingObj.value = 10;
    print(Math.trunc(throwingObj)); //: Error: already positive
} catch(e) {
    print(e);
} finally {
    print(Math.trunc(obj)); //: -35
}
