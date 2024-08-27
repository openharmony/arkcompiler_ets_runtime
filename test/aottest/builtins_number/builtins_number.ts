/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
{
    // test new builtin array
    let numObj1 = "-100";
    print(Number.parseInt(numObj1));

    let numObj2 = "1000";
    print(Number.parseInt(numObj2));

    let numObj3 = "0001030";
    print(Number.parseInt(numObj3));

    let numObj4 = "   -1123";
    print(Number.parseInt(numObj4));
}
print()

//isNaN
print(Number.isNaN(NaN)); // true
print(Number.isNaN(123)); // false
print(Number.isNaN("Hello")); // false
print(Number.isNaN(undefined)); // false
print(Number.isNaN(null)); // false
print(Number.isNaN({})); // false
print()

//isFinite
print(Number.isFinite(0)); // true
print(Number.isFinite(3.14)); // true
print(Number.isFinite(Infinity)); // false
print(Number.isFinite(-Infinity)); // false
print(Number.isFinite(NaN)); // false
print()

//isInteger
print(Number.isInteger(0)); // true
print(Number.isInteger(3.14)); // false
print(Number.isInteger(123456789012345)); // true
print(Number.isInteger(Infinity)); // false
print()

//isSafeIntger
print(Number.isSafeInteger(0)); // true
print(Number.isSafeInteger(123456789012345)); // false
print(Number.isSafeInteger(9007199254740991)); // true
print(Number.isSafeInteger(-9007199254740991)); // true