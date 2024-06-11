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

function doAbs(x: any): number {
    return Math.abs(x);
}

function printAbs(x: any) {
    try {
        print(doAbs(x));
    } finally {
    }
}

function printAbs2() {
    try {
        const INT_MAX: number = 2147483647;
        const INT_MIN: number = -INT_MAX - 1;
        print(Math.abs(INT_MIN));
    } finally {
    }
}

// Check without params
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs()); //: NaN

// Check with single int param
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(0)); //: 0
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(3)); //: 3
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-5)); //: 5

// Check with single float param
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-1.5)); //: 1.5
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(Math.PI)); //: 3.141592653589793
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-Math.PI)); //: 3.141592653589793
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-1.9e80)); //: 1.9e+80
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(1.9e80)); //: 1.9e+80
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-1.9e-80)); //: 1.9e-80
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(1.9e-80)); //: 1.9e-80


// Check with special float params
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(NaN)); //: NaN

//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(1 / Math.abs(-0)); //: Infinity

// Check with 2 params
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(3, 0)); //: 3

// Check with 3 params
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-3, 0, 0)); //: 3

// Check with 4 params
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(4, 0, 0, 0)); //: 4

// Check with 5 params
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
print(Math.abs(-4, 0, 0, 0, 0)); //: 4

// Replace standard builtin
let true_abs = Math.abs
Math.abs = replace

// no deopt
print(Math.abs(-1.001)); //: -1.001
Math.abs = true_abs

// Check edge cases
const INT_MAX: number = 2147483647;
const INT_MIN: number = -INT_MAX - 1;
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
printAbs(INT_MAX); //: 2147483647
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
printAbs(2147483648); //: 2147483648
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
printAbs(-INT_MAX); //: 2147483647
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#printAbs2@builtinMathAbs
//aot: [trace] Check Type: NotInt3
printAbs2(); //: 2147483648
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
printAbs(INT_MIN - 1); //: 2147483649

//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
printAbs(-12); //: 12
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
//aot: [trace] Check Type: NotNumber2
printAbs("abc"); //: NaN
//aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
//aot: [trace] Check Type: NotNumber2
printAbs("-12"); //: 12

if (ArkTools.isAOTCompiled(printAbs)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.abs = replace
}
printAbs(-12); //pgo: 12
//aot: [trace] Check Type: NotCallTarget1
//aot: -12

printAbs("abc"); //pgo: NaN
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

Math.abs = true_abs

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
    printAbs(-12); //: 12
    //aot: [trace] aot inline builtin: Math.abs, caller function name:#*#doAbs@builtinMathAbs
    //aot: [trace] Check Type: NotNumber2
    printAbs("abc"); //: NaN
} catch (e) {
}

let obj = {
    valueOf: () => { return -23; }
};
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinMathAbs
//aot: [trace] Check Type: NotNumber2
print(Math.abs(obj)); //: 23

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
    print(Math.abs(throwingObj)); //: 14
    throwingObj.value = 10;
    print(Math.abs(throwingObj)); //: Error: already positive
} catch(e) {
    print(e);
} finally {
    print(Math.abs(obj)); //: 23
}