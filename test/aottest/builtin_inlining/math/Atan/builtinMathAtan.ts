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
len = Math.atan();
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
print(len); //: NaN

len = Math.atan(NaN);
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
print(len); //: NaN

// Check with single param
len = Math.atan(0);
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
print(len); //: 0

// Check with single param
len = Math.atan(-1); // -PI/4
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
print(len); //: -0.7853981633974483

// Check with single param
len = Math.atan(1); // PI/4
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
print(len); //: 0.7853981633974483

// Check with single param, in |x| > 1
len = Math.atan(10);
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
print(len); //: 1.4711276743037347

// Replace standart builtin
let true_atan = Math.atan
Math.atan = replace
len = Math.atan(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.atan = true_atan
len = Math.atan("NaN"); // deopt
//aot: [trace] aot inline builtin: Math.atan, caller function name:func_main_0@builtinMathAtan
//aot: [trace] Check Type: NotNumber1
print(len); //: NaN
