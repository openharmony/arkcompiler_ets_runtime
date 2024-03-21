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

let len:number = 1;

// Check without params
len = Math.pow();
print(len); // Nan

// Check with single param
len = Math.pow(0);
print(len); // Nan

// Check with three param
len = Math.pow(2, 4, 6);
print(len); // 16

// If exponent is NaN, return NaN.
len = Math.pow(2, NaN)
print(len) // NaN

// If exponent is either +0.0 or -0.0, return 1.0
len = Math.pow(3, +0.0);
print(len); // 1

let temp = -0.0
len = Math.pow(3, -0.0);
print(len); // 1

// If base is -inf, then:
// a. If exponent > +0.0, then
//  * If exponent is an odd integral Number, return -inf. Otherwise, return +inf.
len = Math.pow(-Infinity, 5);
print(len); // -Infinity
len = Math.pow(-Infinity, 6);
print(len); // Infinity
// b. Else:
//  * If exponent is an odd integral Number, return -0.0. Otherwise, return +0.0.
len = Math.pow(-Infinity, -3);
print(len); // -0.0
len = Math.pow(-Infinity, -4);
print(len); // +0.0

// If base is +0.0 and if exponent > +0.0, return +0.0. Otherwise, return +inf.
len = Math.pow(+0.0, 2);
print(len); // +0.0
len = Math.pow(+0.0, -2);
print(len); // +inf


// If base is -0.0, then
// a. If exponent > +0.0, then
//  * If exponent is an odd integral Number, return -0.0. Otherwise, return +0.0.
len = Math.pow(-0.0, 7);
print(len); // -0.0
len = Math.pow(-0.0, 8);
print(len); // +0.0

// b. Else,
//  * If exponent is an odd integral Number, return -inf. Otherwise, return +inf.
len = Math.pow(-0.0, -9);
print(len); // -inf
len = Math.pow(-0.0, -10);
print(len); // +inf

// If exponent is +inf, then
// a. If abs(base) > 1, return +inf.
len = Math.pow(1.5, +Infinity);
print(len); // +inf
// b. If abs(base) = 1, return NaN.
len = Math.pow(1, +Infinity);
print(len); // NaN
// c. If abs(base) < 1, return +inf.
len = Math.pow(0.5, +Infinity);
print(len); // +0.0

// If exponent is -inf, then
// a. If abs(base) > 1, return +inf.
len = Math.pow(1.5, -Infinity);
print(len); // +0.0
// b. If abs(base) = 1, return NaN.
len = Math.pow(1, -Infinity);
print(len); // NaN
// c. If abs(base) < 1, return +inf.
len = Math.pow(0.2, -Infinity);
print(len); // +inf


len = Math.pow(2, "three");
print(len); // Nan