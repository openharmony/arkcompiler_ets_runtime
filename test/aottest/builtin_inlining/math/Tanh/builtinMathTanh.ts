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
//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
len = Math.tanh();
print(len); //: NaN

//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
len = Math.tanh(NaN);
print(len); //: NaN

// Check with single param
//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
len = Math.tanh(0);
print(len); //: 0

// Check with single param
//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
len = Math.tanh(-1);
print(len); //: -0.7615941559557649

// Check with single param
//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
len = Math.tanh(1);
print(len); //: 0.7615941559557649

// Check with single param
//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
len = Math.tanh(10);
print(len); //: 0.9999999958776927

// Replace stanhdart builtin
let true_tanh = Math.tanh
Math.tanh = replace
len = Math.tanh(111);
print(len); //: 111

// Call stanhdart builtin with non-number param
Math.tanh = true_tanh
//aot: [trace] aot inline builtin: Math.tanh, caller function name:func_main_0@builtinMathTanh
//aot: [trace] Check Type: NotNumber1
len = Math.tanh("NaN"); // deopt
print(len); //: NaN
