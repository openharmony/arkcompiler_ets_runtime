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

// Check without params
print(Math.sqrt()); //: NaN

// Check multiple params
print(Math.sqrt(2, -3, 4)); //: 1.4142135623730951

print(Math.sqrt(2, -3, -4)); //: 1.4142135623730951

print(Math.sqrt(-2, -3, -4)); //: NaN

print(Math.sqrt(-2, 3, 4, 5)); //: NaN

// Check with single param
print(Math.sqrt(NaN)); //: NaN
print(Math.sqrt(0)); //: 0
print(Math.sqrt(-156)); //: NaN
print(Math.sqrt(1)); //: 1
print(Math.sqrt(10)); //: 3.1622776601683795
print(1 / Math.sqrt(-0)); //: -Infinity

// Check Infinity
print(Math.sqrt(Infinity)); //: Infinity
print(Math.sqrt(-Infinity)); //: NaN

// Replace standart builtin
let true_sqrt = Math.sqrt
Math.sqrt = replace
print(Math.sqrt(111)); //: 111

Math.sqrt = true_sqrt
print(Math.sqrt(Math.PI)); //: 1.7724538509055159
//aot: [trace] Check Type: NotNumber1
print(Math.sqrt("9")); //: 3

