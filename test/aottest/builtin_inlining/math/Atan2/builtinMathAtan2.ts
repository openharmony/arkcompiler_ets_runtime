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
print(len); // NaN

len = Math.atan2(NaN);
print(len); // NaN

len = Math.atan2(NaN, NaN);
print(len); // NaN

len = Math.atan2(0, NaN);
print(len); // NaN

len = Math.atan2(NaN, 0);
print(len); // NaN

len = Math.atan2(-1, 1.5);
print(len); // -0.5880026035475675

len = Math.atan2(1, -0);
print(len); // Math.PI / 2

len = Math.atan2(0, 1);
print(len); // 0

len = Math.atan2(0, -0);
print(len); // Math.PI

len = Math.atan2(-0, 0);
print(len); // -0.

len = Math.atan2(-0, -0);
print(len); // -Math.PI

len = Math.atan2(-1, Number.POSITIVE_INFINITY)
print(len) // -0. !!!! NOTE: now it's return 0, need investigate

len = Math.atan2(1, Number.POSITIVE_INFINITY)
print(len) // 0.

// Replace standart builtin
let true_atan2 = Math.atan2
Math.atan2 = replace
len = Math.atan2(111);
print(len);

// Call standart builtin with non-number param
Math.atan2 = true_atan2
len = Math.atan2(0, "NaN"); // deopt
print(len); // NaN
