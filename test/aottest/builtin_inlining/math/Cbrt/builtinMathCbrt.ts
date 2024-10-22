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

declare function print(arg:any):string;
declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
function replace(a : number)
{
    return a;
}

function doCbrt(x: any): number {
    return Math.cbrt(x);
}

function printCbrt(x: any) {
    try {
        print(doCbrt(x));
    } finally {
    }
}

let res:number = 1;

// Check without params
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt()); //: NaN

// Check with single param
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(-0.027)); //: -0.29999999999999993
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(0.125)); //: 0.49999999999999994
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(1)); //: 1
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(8)); //: 2
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(2146689000)); //: 1290.0000000000002
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(1_0000_0000_0000)); //: 10000
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(-1_0000_0000_0000)); //: -10000
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(10e80)); //: 1.0000000000000002e+27

// Check with three param
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(64, 4, 6)); //: 4

// If n is NaN, +0.0f or -0.0f, return n
res = Math.cbrt(+0.0);
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(res); //: 0
print("1/x: " + 1.0/res); //: 1/x: Infinity

res = Math.cbrt(-0.0);
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(res); //: 0
print("1/x: " + 1.0/res); //: 1/x: -Infinity

//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
print(Math.cbrt(NaN)); //: NaN

// Replace standard builtin
let true_cbrt = Math.cbrt;
Math.cbrt = replace;

// no deopt
print(Math.cbrt(0.001)); //: 0.001
Math.cbrt = true_cbrt;

//aot: [trace] aot inline builtin: Math.cbrt, caller function name:#*#doCbrt@builtinMathCbrt
//aot: [trace] Check Type: NotNumber1
printCbrt("abcd"); //: NaN

//aot: [trace] aot inline builtin: Math.cbrt, caller function name:#*#doCbrt@builtinMathCbrt
//aot: [trace] Check Type: NotNumber1
printCbrt("-125"); //: -5

//aot: [trace] aot inline builtin: Math.cbrt, caller function name:#*#doCbrt@builtinMathCbrt
//aot: [trace] Check Type: NotNumber1
printCbrt("abcdef"); //: NaN

if (ArkTools.isAOTCompiled(printCbrt)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.cbrt = replace
}

printCbrt(-216); //pgo: -6.000000000000001
//aot: [trace] Check Type: NotCallTarget1
//aot: -216

printCbrt("abcd"); //pgo: NaN
//aot: [trace] Check Type: NotCallTarget1
//aot: abcd

Math.cbrt = true_cbrt;

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.cbrt, caller function name:#*#doCbrt@builtinMathCbrt
    printCbrt(10); //: 2.1544346900318834
    //aot: [trace] aot inline builtin: Math.cbrt, caller function name:#*#doCbrt@builtinMathCbrt
    //aot: [trace] Check Type: NotNumber1
    printCbrt("abc"); //: NaN
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -64; })
//aot: [trace] aot inline builtin: Math.cbrt, caller function name:func_main_0@builtinMathCbrt
//aot: [trace] Check Type: NotNumber1
print(Math.cbrt(obj)); //: -4

function Throwing() {
    this.value = 100;
}

Throwing.prototype.valueOf = function() {
    if (this.value > 999) {
        throw new Error("big value")
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(Math.cbrt(throwingObj)); //: 4.641588833612778
    throwingObj.value = 1000;
    print(Math.cbrt(throwingObj)); //: Error: big value
} catch(e) {
    print(e);
} finally {
    print(Math.cbrt(obj)); //: -4
}
