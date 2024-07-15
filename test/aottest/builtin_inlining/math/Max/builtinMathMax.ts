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
function printMax(x: any, y: any) {
    try {
        print(Math.max(x, y));
    } finally {
    }
}

function printMax1(x: any) {
    try {
        print(Math.max(x));
    } finally {
    }
}

function printMax3(x: any, y: any, z: any) {
    try {
        print(Math.max(x, y, z));
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
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max()); //: -Infinity

// Check with single param
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(0)); //: 0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(567)); //: 567
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1e80)); //: -1e+80

// Check with special float params
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-Infinity)); //: -Infinity
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(NaN)); //: NaN

// Check with 2 int params
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(3, 300)); //: 300
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(300, 3)); //: 300
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(3, -2)); //: 3
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-22, -2)); //: -2
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1, -1)); //: -1

// Check with 2 float params
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(3.2, 300.7)); //: 300.7
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(300.7, 3.2)); //: 300.7
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(3e9, -2e10)); //: 3000000000
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-22.1, -2e-10)); //: -2e-10
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1.8, -1.8)); //: -1.8
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(Infinity, -1.8)); //: Infinity
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1.8, Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-Infinity, -1.8)); //: -1.8
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1.8, -Infinity)); //: -1.8
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1.8, NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(NaN, -1.8)); //: NaN
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-Infinity, NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(Infinity, NaN)); //: NaN

// Check 0 and -0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
//aot: [trace] aot inline function name: #*#printZero@builtinMathMax caller function name: func_main_0@builtinMathMax
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMax
printZero(Math.max(0, -0));  //: 0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
//aot: [trace] aot inline function name: #*#printZero@builtinMathMax caller function name: func_main_0@builtinMathMax
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMax
printZero(Math.max(-0, 0));  //: 0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
//aot: [trace] aot inline function name: #*#printZero@builtinMathMax caller function name: func_main_0@builtinMathMax
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMax
printZero(Math.max(0, 0));   //: 0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
//aot: [trace] aot inline function name: #*#printZero@builtinMathMax caller function name: func_main_0@builtinMathMax
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMax
printZero(Math.max(-0, -0)); //: -0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
//aot: [trace] aot inline function name: #*#printZero@builtinMathMax caller function name: func_main_0@builtinMathMax
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMax
printZero(Math.max(-5, -0)); //: -0
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
//aot: [trace] aot inline function name: #*#printZero@builtinMathMax caller function name: func_main_0@builtinMathMax
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMax
printZero(Math.max(-5, 0)); //: 0

// Check with int and float param
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(1.5, 3)); //: 3
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(3, 1.5)); //: 3
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(2.5, 1)); //: 2.5
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(1, 2.5)); //: 2.5
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(1, -Infinity)); //: 1
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(5, NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(NaN, 5)); //: NaN


// Check with 3 params
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(2, 4, 6)); //: 6
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(4, 6, 2)); //: 6
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(6, 2, 4)); //: 6
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-1, 1, 2.5)); //: 2.5

// Check with 4 params
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(-4, 0, 2, -Infinity)); //: 2

// Check with 5 params
//aot: [trace] aot inline builtin: Math.max, caller function name:func_main_0@builtinMathMax
print(Math.max(1, 2, 1.5, 2, 8)); //: 8

// Replace standard builtin
let trueMax = Math.max
Math.max = replace
print(Math.max(-1.001, -90)); //: -1.001
Math.max = trueMax

//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
printMax(-12, -100); //: -12
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
//aot: [trace] Check Type: NotNumber2
printMax("abc", -100); //: NaN
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
//aot: [trace] Check Type: NotNumber2
printMax("-12", -100); //: -12

if (ArkTools.isAOTCompiled(printMax)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.max = replace
}
//aot: [trace] Check Type: NotCallTarget1
printMax(5, 12); //pgo: 12
                  //aot: 5
//aot: [trace] Check Type: NotCallTarget1
printMax("abc", 2); //pgo: NaN
                    //aot: abc
Math.max = trueMax

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
    printMax(19, 20); //: 20
    //aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
    printMax(19, 12); //: 19
    //aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
    //aot: [trace] Check Type: NotNumber2
    printMax("abc", 5); //: NaN
} catch (e) {
}

//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax1@builtinMathMax
//aot: [trace] Check Type: InconsistentType1
//: obj.valueOf
printMax1(obj); //: -23
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax1@builtinMathMax
//aot: [trace] Check Type: InconsistentType1
printMax1(doubleObj); //: 2.7
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
//aot: [trace] Check Type: NotNumber2
printMax(doubleObj, doubleObj); //: 2.7
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
printMax(nanObj, obj); //: NaN
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
printMax(2, obj); //: 2
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax@builtinMathMax
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
printMax(-100, obj); //: -23

// call obj.valueOf twice
//aot: [trace] aot inline builtin: Math.max, caller function name:#*#printMax3@builtinMathMax
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
//: obj.valueOf
printMax3(NaN, obj, obj); //: NaN
