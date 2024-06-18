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
len = Math.acosh();
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
print(len); //: NaN

len = Math.acosh(NaN);
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
print(len); //: NaN

// Check with single param, in x < 1
len = Math.acosh(0);
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
print(len); //: NaN

// Check with single param, in x < 1
len = Math.acosh(-1);
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
print(len); //: NaN

// Check with single param, in x >= 1
len = Math.acosh(1);
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
print(len); //: 0

// Check with single param, in x >= 1
len = Math.acosh(10);
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
print(len); //: 2.993222846126381

// Replace standart builtin
let true_acosh = Math.acosh
Math.acosh = replace
len = Math.acosh(111);
print(len); //: 111

// Call standart builtin with non-number 
Math.acosh = true_acosh
len = Math.acosh("NaN"); // deopt
//aot: [trace] aot inline builtin: Math.acosh, caller function name:func_main_0@builtinMathAcosh
//aot: [trace] Check Type: NotNumber1
print(len); //: NaN
