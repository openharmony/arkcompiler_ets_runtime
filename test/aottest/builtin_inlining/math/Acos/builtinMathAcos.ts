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
len = Math.acos();
print(len); // NaN

len = Math.acos(NaN);
print(len); // NaN

// Check with single param, in |x| <= 1
len = Math.acos(0);
print(len); // PI / 2 = 1.5707963267948966

// Check with single param, in |x| <= 1
len = Math.acos(-1);
print(len); // PI = 3.141592653589793

// Check with single param, in |x| <= 1
len = Math.acos(1);
print(len); // 0

// Check with single param, in |x| > 1
len = Math.acos(10);
print(len); // Nan

// Replace standart builtin
let true_acos = Math.acos
Math.acos = replace
len = Math.acos(111);
print(len);

// Call standart builtin with non-number param
Math.acos = true_acos
len = Math.acos("NaN"); // deopt
print(len); // NaN