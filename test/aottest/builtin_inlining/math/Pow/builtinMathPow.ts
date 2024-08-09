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

let len:number = 1;

// Check without params
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow();
print(len); //: NaN

// Check with single param
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(0);
print(len); //: NaN

// Check with three param
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(2, 4, 6);
print(len); //: 16

// If exponent is NaN, return NaN.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(2, NaN)
print(len) //: NaN

// If exponent is either +0.0 or -0.0, return 1.0
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(3, +0.0);
print(len); //: 1

let temp = -0.0
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(3, -0.0);
print(len); //: 1

// If base is -inf, then:
// a. If exponent > +0.0, then
//  * If exponent is an odd integral Number, return -inf. Otherwise, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-Infinity, 5);
print(len); //: -Infinity
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-Infinity, 6);
print(len); //: Infinity
// b. Else:
//  * If exponent is an odd integral Number, return -0.0. Otherwise, return +0.0.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-Infinity, -3);
print("1/x: " + 1 / len); //: 1/x: -Infinity
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-Infinity, -4);
print(len); //: 0

// If base is +0.0 and if exponent > +0.0, return +0.0. Otherwise, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(+0.0, 2);
print(len); //: 0
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(+0.0, -2);
print(len); //: Infinity


// If base is -0.0, then
// a. If exponent > +0.0, then
//  * If exponent is an odd integral Number, return -0.0. Otherwise, return +0.0.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-0.0, 7);
print("1/x: " + 1 / len); //: 1/x: -Infinity
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-0.0, 8);
print(len); //: 0

// b. Else,
//  * If exponent is an odd integral Number, return -inf. Otherwise, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-0.0, -9);
print(len); //: -Infinity
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-0.0, -10);
print(len); //: Infinity

// If exponent is +inf, then
// a. If abs(base) > 1, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(1.5, +Infinity);
print(len); //: Infinity
// b. If abs(base) = 1, return NaN.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(1, +Infinity);
print(len); //: NaN
// c. If abs(base) < 1, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(0.5, +Infinity);
print(len); //: 0

// If exponent is -inf, then
// a. If abs(base) > 1, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(1.5, -Infinity);
print(len); //: 0
// b. If abs(base) = 1, return NaN.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(1, -Infinity);
print(len); //: NaN
// c. If abs(base) < 1, return +inf.
//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(0.2, -Infinity);
print(len); //: Infinity

//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(1, Infinity);
print(len); //: NaN

//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-1, Infinity);
print(len); //: NaN

//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(1, -Infinity);
print(len); //: NaN

//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
len = Math.pow(-1, -Infinity);
print(len); //: NaN

//aot: [trace] aot inline builtin: Math.pow, caller function name:func_main_0@builtinMathPow
//aot: [trace] Check Type: NotNumber1
len = Math.pow(2, "three");
print(len); //: NaN
