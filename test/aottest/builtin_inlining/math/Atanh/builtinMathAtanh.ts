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
len = Math.atanh();
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: NaN

len = Math.atanh(NaN);
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: NaN

// Check with single param
len = Math.atanh(0);
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: 0

// Check with single param
len = Math.atanh(0.5);
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: 0.5493061443340548

// Check with single param
len = Math.atanh(-1);
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: -Infinity

// Check with single param
len = Math.atanh(1);
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: Infinity

// Check with single param
len = Math.atanh(10);
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
print(len); //: NaN

// Replace standart builtin
let true_atanh = Math.atanh
Math.atanh = replace
len = Math.atanh(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.atanh = true_atanh
len = Math.atanh("abc"); // deopt
//aot: [trace] aot inline builtin: Math.atanh, caller function name:func_main_0@builtinMathAtanh
//aot: [trace] Check Type: NotNumber1
print(len); //: NaN
