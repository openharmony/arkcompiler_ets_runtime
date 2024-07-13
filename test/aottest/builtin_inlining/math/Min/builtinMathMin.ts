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
function printMin(x: any, y: any) {
    try {
        print(Math.min(x, y));
    } finally {
    }
}

function printMin1(x: any) {
    try {
        print(Math.min(x));
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
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min()); //: Infinity

// Check with single param
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(0)); //: 0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(567)); //: 567
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1e80)); //: -1e+80

// Check with special float params
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(Infinity)); //: Infinity
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-Infinity)); //: -Infinity
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(NaN)); //: NaN

// Check with 2 int params
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(3, 300)); //: 3
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(300, 3)); //: 3
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(3, -2)); //: -2
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-22, -2)); //: -22
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1, -1)); //: -1

// // Check with 2 float params
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(3.2, 300.7)); //: 3.2
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(300.7, 3.2)); //: 3.2
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(3e11, -2e10)); //: -20000000000
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-22.1, -2e-10)); //: -22.1
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1.8, -1.8)); //: -1.8
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(Infinity, -1.8)); //: -1.8
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1.8, Infinity)); //: -1.8
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-Infinity, -1.8)); //: -Infinity
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1.8, -Infinity)); //: -Infinity
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1.8, NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(NaN, -1.8)); //: NaN
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-Infinity, NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(Infinity, NaN)); //: NaN


// Check 0 and -0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] aot inline function name: #*#printZero@builtinMathMin caller function name: func_main_0@builtinMathMin
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMin
printZero(Math.min(0, -0));  //: -0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] aot inline function name: #*#printZero@builtinMathMin caller function name: func_main_0@builtinMathMin
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMin
printZero(Math.min(-0, 0));  //: -0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] aot inline function name: #*#printZero@builtinMathMin caller function name: func_main_0@builtinMathMin
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMin
printZero(Math.min(0, 0));   //: 0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] aot inline function name: #*#printZero@builtinMathMin caller function name: func_main_0@builtinMathMin
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMin
printZero(Math.min(-0, -0)); //: -0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] aot inline function name: #*#printZero@builtinMathMin caller function name: func_main_0@builtinMathMin
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMin
printZero(Math.min(5, -0)); //: -0
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] aot inline function name: #*#printZero@builtinMathMin caller function name: func_main_0@builtinMathMin
//aot: [trace] aot inline builtin: Object.is, caller function name:#*#printZero@builtinMathMin
printZero(Math.min(5, 0)); //: 0

// Check with int and float param
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(1.5, 3)); //: 1.5
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(3, 1.5)); //: 1.5
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(2.5, 1)); //: 1
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(1, 2.5)); //: 1
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(1, Infinity)); //: 1
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(5, NaN)); //: NaN
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(NaN, 5)); //: NaN

// Check with 3 params
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(2, 4, 6)); //: 2
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(4, 6, 2)); //: 2
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(6, 2, 4)); //: 2
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1, 1, -2.5)); //: -2.5

// Check with 4 params
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(4, 0, -2, Infinity)); //: -2

// Check with 5 params
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
print(Math.min(-1, -2, -1.5, -2, -8)); //: -8

// Replace standard builtin
let trueMin = Math.min
Math.min = replace
print(Math.min(-1.001, -90)); //: -1.001
Math.min = trueMin

//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
printMin(-12, 1); //: -12
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
//aot: [trace] Check Type: NotNumber2
printMin("abc", 1); //: NaN
//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
//aot: [trace] Check Type: NotNumber2
printMin("-12", 1); //: -12

if (ArkTools.isAOTCompiled(printMin)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.min = replace
}
//aot: [trace] Check Type: NotCallTarget1
printMin(5, -12); //pgo: -12
                  //aot: 5
//aot: [trace] Check Type: NotCallTarget1
printMin("abc", 2); //pgo: NaN
                    //aot: abc
Math.min = trueMin

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
    printMin(19, 12); //: 12
    //aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
    //aot: [trace] Check Type: NotNumber2
    printMin("abc", 5); //: NaN
} catch (e) {
}

//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin1@builtinMathMin
//aot: [trace] Check Type: InconsistentType1
//: obj.valueOf
printMin1(obj); //: -23
//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin1@builtinMathMin
//aot: [trace] Check Type: InconsistentType1
printMin1(doubleObj); //: 2.7
//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
//aot: [trace] Check Type: NotNumber2
printMin(doubleObj, doubleObj); //: 2.7
//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
printMin(nanObj, obj); //: NaN
//aot: [trace] aot inline builtin: Math.min, caller function name:#*#printMin@builtinMathMin
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
printMin(24, obj); //: -23

// call obj.valueOf twice
//aot: [trace] aot inline builtin: Math.min, caller function name:func_main_0@builtinMathMin
//aot: [trace] Check Type: NotNumber2
//: obj.valueOf
//: obj.valueOf
print(Math.min(NaN, obj, obj)); //: NaN
