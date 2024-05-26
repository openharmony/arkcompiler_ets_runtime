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

function printZero(x: any) {
    if (Object.is(x, -0)) {
        print("-0");
    } else {
        print(x);
    }
}

function replace(a : number)
{
    return a;
}

// Use try to prevent inlining to main
function printExpm1(x: any) {
    try {
        print(Math.expm1(x));
    } finally {
    }
}

let doubleObj = {
    valueOf: () => { return 2.7; }
}

let nanObj = {
    valueOf: () => { return "something"; }
}

let obj = {
    valueOf: () => {
        print("obj.valueOf")
        return -23;
    }
};

// Check without params
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1()); //: NaN

// Check with single param
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
//aot: [trace] aot inline function name: #*#printZero@builtinMathExpm1 caller function name: func_main_0@builtinMathExpm1
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathExpm1
printZero(Math.expm1(0)); //: 0
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
//aot: [trace] aot inline function name: #*#printZero@builtinMathExpm1 caller function name: func_main_0@builtinMathExpm1
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathExpm1
printZero("1/x: " + 1 / Math.expm1(-0)); //: 1/x: -Infinity
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(1)); //: 1.718281828459045
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(-100)); //: -1
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(100)); //: 2.6881171418161356e+43
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(10e-10)); //: 1.0000000005000001e-9

// Check with special float params
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(-Infinity)); //: -1
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(NaN)); //: NaN

// Check with 2 params
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(1, 1)); //: 1.718281828459045

// Check with 3 params
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(1, 1, 1)); //: 1.718281828459045

// Check with 4 params
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(1, 1, 1, 1)); //: 1.718281828459045

// Check with 5 params
//aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
print(Math.expm1(1, 1, 1, 1, 1)); //: 1.718281828459045

try {
    //aot: [trace] aot inline builtin: Math.expm1, caller function name:func_main_0@builtinMathExpm1
    print(Math.expm1(1)); //: 1.718281828459045
} catch(e) {}

// Replace standart builtin
let trueExpm1 = Math.expm1
Math.expm1 = replace
print(Math.expm1(111)); //: 111
Math.expm1 = trueExpm1

//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
printExpm1(1); //: 1.718281828459045

// Call standart builtin with non-number param
//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
//aot: [trace] Check Type: NotNumber1
printExpm1("1"); //: 1.718281828459045
//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
//aot: [trace] Check Type: NotNumber1
printExpm1("NaN"); //: NaN
//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
//aot: [trace] Check Type: NotNumber1
printExpm1("abc"); //: NaN

if (ArkTools.isAOTCompiled(printExpm1)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.expm1 = replace
}

printExpm1(1); //pgo: 1.718281828459045
//aot: [trace] Check Type: NotCallTarget1
//aot: 1

printExpm1(2); //pgo: 6.38905609893065
//aot: [trace] Check Type: NotCallTarget1
//aot: 2

printExpm1("1"); //pgo: 1.718281828459045
//aot: [trace] Check Type: NotCallTarget1
//aot: 1

printExpm1("2"); //pgo: 6.38905609893065
//aot: [trace] Check Type: NotCallTarget1
//aot: 2

Math.expm1 = trueExpm1

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
    printExpm1(1); //: 1.718281828459045
    //aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
    printExpm1(1, 2); //: 1.718281828459045
    //aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
    printExpm1(1, 2); //: 1.718281828459045
    //aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
    //aot: [trace] Check Type: NotNumber1
    printExpm1("abc", 3e3); //: NaN
} catch (e) {
}

//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
//aot: [trace] Check Type: NotNumber1
//: obj.valueOf
printExpm1(obj); //: -0.9999999998973812
//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
//aot: [trace] Check Type: NotNumber1
printExpm1(doubleObj); //: 13.879731724872837
//aot: [trace] aot inline builtin: Math.expm1, caller function name:#*#printExpm1@builtinMathExpm1
//aot: [trace] Check Type: NotNumber1
printExpm1(nanObj); //: NaN
