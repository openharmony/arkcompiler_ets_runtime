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
len = Math.asinh();
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
print(len); //: NaN

len = Math.asinh(NaN);
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
print(len); //: NaN

// Check with single param
len = Math.asinh(0);
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
print(len); //: 0

// Check with single param
len = Math.asinh(-1);
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
print(len); //: -0.881373587019543

// Check with single param
len = Math.asinh(1);
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
print(len); //: 0.881373587019543

// Check with single param
len = Math.asinh(10);
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
print(len); //: 2.99822295029797

// Replace standart builtin
let true_asinh = Math.asinh
Math.asinh = replace
len = Math.asinh(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.asinh = true_asinh
len = Math.asinh("NaN"); // deopt
//aot: [trace] aot inline builtin: Math.asinh, caller function name:func_main_0@builtinMathAsinh
//aot: [trace] Check Type: NotNumber1
print(len); //: NaN
