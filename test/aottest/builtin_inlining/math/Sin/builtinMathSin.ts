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
len = Math.sin();
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: NaN

// Check with single param
len = Math.sin(0);
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: 0

// Check with single not zero param
len = Math.sin(Math.PI / 2);
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: 1

// Check with 2 params
len = Math.sin(0,0);
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: 0

// Check with 3 params
len = Math.sin(0,0,0);
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: 0

// Check with 4 params
len = Math.sin(0,0,0,0);
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: 0

// Check with 5 params
len = Math.sin(0,0,0,0,0);
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
print(len); //: 0

// Replace standart builtin
let true_sin = Math.sin
Math.sin = replace
len = Math.sin(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.sin = true_sin
//aot: [trace] aot inline builtin: Math.sin, caller function name:func_main_0@builtinMathSin
//aot: [trace] Check Type: NotNumber1
len = Math.sin("0"); // deopt
print(len); //: 0
