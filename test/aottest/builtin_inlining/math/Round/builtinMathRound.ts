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

function printZero(x: any) {
    if (Object.is(x, -0)) {
        print("-0");
    } else {
        print(x);
    }
}

function doRound(x: any): number {
    return Math.round(x);
}

function printRound(x: any) {
    try {
        print(doRound(x));
    } finally {
    }
}

// Check without params
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round()); //: NaN

// Check with single int param
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(0)); //: 0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(3)); //: 3
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-5)); //: -5

// Test large ints
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(1073741824)) //: 1073741824
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(1073741804)) //: 1073741804
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(1073741784)) //: 1073741784
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-1073741804)) //: -1073741804

// Check with single float param
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(1.9e80)); //: 1.9e+80
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline function name: #*#printZero@builtinMathRound caller function name: func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(2.5)); //: 3
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline function name: #*#printZero@builtinMathRound caller function name: func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(1.5)); //: 2
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline function name: #*#printZero@builtinMathRound caller function name: func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(0.5)); //: 1
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline function name: #*#printZero@builtinMathRound caller function name: func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(0.2)); //: 0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline function name: #*#printZero@builtinMathRound caller function name: func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(-0)); //: -0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline function name: #*#printZero@builtinMathRound caller function name: func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(-1.9e-80)); //: -0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(-0.1)); //: -0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathRound
printZero(Math.round(-0.5)); //: -0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-0.7)); //: -1
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-1.2)); //: -1
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-1.5)); //: -1
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-2.1)); //: -2
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-2.49)); //: -2
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-2.5)); //: -2
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-2.7)); //: -3
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-1.9e80)); //: -1.9e+80
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(1.9e-80)); //: 0
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(0.5 - Number.EPSILON / 4)); //: 0


// Check with special float params
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-Infinity)); //: -Infinity
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(NaN)); //: NaN

// Check with 2 params
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(3, 0)); //: 3

// Check with 3 params
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-3.5, 0, 0)); //: -3

// Check with 4 params
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(4.1, 0, 0, 0)); //: 4

// Check with 5 params
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
print(Math.round(-4.1, 0, 0, 0, 0)); //: -4

// Replace standard builtin
let true_round = Math.round
Math.round = replace

// no deopt
print(Math.round(-1.001)); //: -1.001
Math.round = true_round

//aot: [trace] aot inline builtin: Math.round, caller function name:#*#doRound@builtinMathRound
printRound(12.3); //: 12
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Math.round, caller function name:#*#doRound@builtinMathRound
//aot: [trace] Check Type: NotNumber2
printRound("abc"); //: NaN
//aot: [trace] aot inline builtin: Math.round, caller function name:#*#doRound@builtinMathRound
//aot: [trace] Check Type: NotNumber2
printRound("-12.9"); //: -13

if (ArkTools.isAOTCompiled(printRound)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.round = replace
}
printRound(-12.2); //pgo: -12
//aot: [trace] Check Type: NotCallTarget1
//aot: -12.2

printRound("abc"); //pgo: NaN
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

Math.round = true_round

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.round, caller function name:#*#doRound@builtinMathRound
    printRound(0.3) //: 0
    //aot: [trace] aot inline builtin: Math.round, caller function name:#*#doRound@builtinMathRound
    printRound(-12); //: -12
    //aot: [trace] aot inline builtin: Math.round, caller function name:#*#doRound@builtinMathRound
    //aot: [trace] Check Type: NotNumber2
    printRound("abc"); //: NaN
} catch (e) {
}

let obj = {
    valueOf: () => { return -22.5; }
};
//aot: [trace] aot inline builtin: Math.round, caller function name:func_main_0@builtinMathRound
//aot: [trace] Check Type: NotNumber2
print(Math.round(obj)); //: -22
