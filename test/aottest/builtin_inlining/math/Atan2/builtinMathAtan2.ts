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
function replace(a : number)
{
    return a;
}

let len:number = 1;

// Check without params
len = Math.atan2();
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: NaN

len = Math.atan2(NaN);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: NaN

len = Math.atan2(NaN, NaN);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: NaN

len = Math.atan2(0, NaN);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: NaN

len = Math.atan2(NaN, 0);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: NaN

len = Math.atan2(-1, 1.5);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: -0.5880026035475675

len = Math.atan2(1, -0); // PI/2
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: 1.5707963267948966

len = Math.atan2(0, 1);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: 0

len = Math.atan2(0, -0); // PI
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: 3.141592653589793

len = Math.atan2(-0, 0);
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print("1/x: " + 1/len); //: 1/x: -Infinity

len = Math.atan2(-0, -0); // -PI
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len); //: -3.141592653589793

len = Math.atan2(-1, Number.POSITIVE_INFINITY)
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print("1/x: " + 1/len) //: 1/x: -Infinity

len = Math.atan2(1, Number.POSITIVE_INFINITY)
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
print(len) //: 0

// Replace standart builtin
let true_atan2 = Math.atan2
Math.atan2 = replace
len = Math.atan2(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.atan2 = true_atan2
len = Math.atan2(0, "NaN"); // deopt
//aot: [trace] aot inline builtin: Math.atan2, caller function name:func_main_0@builtinMathAtan2
//aot: [trace] Check Type: NotNumber1
print(len); //: NaN
