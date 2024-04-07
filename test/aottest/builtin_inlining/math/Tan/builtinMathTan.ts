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
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan();
print(len); //: NaN

//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan(NaN);
print(len); //: NaN

// Check with single param
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan(0);
print(len); //: 0

// Check with single param
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan(-1);
print(len); //: -1.5574077246549023

// Check with single param
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan(1);
print(len); //: 1.5574077246549023

// Check with single param
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan(Math.PI / 10);
print(len); //: 0.3249196962329063

// Check with single param
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
len = Math.tan(10);
print(len); //: 0.6483608274590866

// Replace standart builtin
let true_tan = Math.tan
Math.tan = replace
len = Math.tan(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.tan = true_tan
//aot: [trace] aot inline builtin: Math.tan, caller function name:func_main_0@builtinMathTan
//aot: [trace] Check Type: NotNumber1
len = Math.tan("NaN"); // deopt
print(len); //: NaN
