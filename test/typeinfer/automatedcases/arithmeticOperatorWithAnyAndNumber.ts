/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/expressions/binaryOperators/arithmeticOperator/arithmeticOperatorWithAnyAndNumber.ts ===
declare function AssertType(value:any, type:string):void;
let a: any;
AssertType(a, "any");

let b: number;
AssertType(b, "number");

// operator *
let ra1 = a * a;
AssertType(ra1, "number");
AssertType(a * a, "number");
AssertType(a, "any");
AssertType(a, "any");

let ra2 = a * b;
AssertType(ra2, "number");
AssertType(a * b, "number");
AssertType(a, "any");
AssertType(b, "number");

let ra3 = a * 0;
AssertType(ra3, "number");
AssertType(a * 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let ra4 = 0 * a;
AssertType(ra4, "number");
AssertType(0 * a, "number");
AssertType(0, "int");
AssertType(a, "any");

let ra5 = 0 * 0;
AssertType(ra5, "number");
AssertType(0 * 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let ra6 = b * 0;
AssertType(ra6, "number");
AssertType(b * 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let ra7 = 0 * b;
AssertType(ra7, "number");
AssertType(0 * b, "number");
AssertType(0, "int");
AssertType(b, "number");

let ra8 = b * b;
AssertType(ra8, "number");
AssertType(b * b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator /
let rb1 = a / a;
AssertType(rb1, "number");
AssertType(a / a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rb2 = a / b;
AssertType(rb2, "number");
AssertType(a / b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rb3 = a / 0;
AssertType(rb3, "number");
AssertType(a / 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rb4 = 0 / a;
AssertType(rb4, "number");
AssertType(0 / a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rb5 = 0 / 0;
AssertType(rb5, "number");
AssertType(0 / 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rb6 = b / 0;
AssertType(rb6, "number");
AssertType(b / 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rb7 = 0 / b;
AssertType(rb7, "number");
AssertType(0 / b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rb8 = b / b;
AssertType(rb8, "number");
AssertType(b / b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator %
let rc1 = a % a;
AssertType(rc1, "number");
AssertType(a % a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rc2 = a % b;
AssertType(rc2, "number");
AssertType(a % b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rc3 = a % 0;
AssertType(rc3, "number");
AssertType(a % 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rc4 = 0 % a;
AssertType(rc4, "number");
AssertType(0 % a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rc5 = 0 % 0;
AssertType(rc5, "number");
AssertType(0 % 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rc6 = b % 0;
AssertType(rc6, "number");
AssertType(b % 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rc7 = 0 % b;
AssertType(rc7, "number");
AssertType(0 % b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rc8 = b % b;
AssertType(rc8, "number");
AssertType(b % b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator -
let rd1 = a - a;
AssertType(rd1, "number");
AssertType(a - a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rd2 = a - b;
AssertType(rd2, "number");
AssertType(a - b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rd3 = a - 0;
AssertType(rd3, "number");
AssertType(a - 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rd4 = 0 - a;
AssertType(rd4, "number");
AssertType(0 - a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rd5 = 0 - 0;
AssertType(rd5, "number");
AssertType(0 - 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rd6 = b - 0;
AssertType(rd6, "number");
AssertType(b - 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rd7 = 0 - b;
AssertType(rd7, "number");
AssertType(0 - b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rd8 = b - b;
AssertType(rd8, "number");
AssertType(b - b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator <<
let re1 = a << a;
AssertType(re1, "number");
AssertType(a << a, "number");
AssertType(a, "any");
AssertType(a, "any");

let re2 = a << b;
AssertType(re2, "number");
AssertType(a << b, "number");
AssertType(a, "any");
AssertType(b, "number");

let re3 = a << 0;
AssertType(re3, "number");
AssertType(a << 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let re4 = 0 << a;
AssertType(re4, "number");
AssertType(0 << a, "number");
AssertType(0, "int");
AssertType(a, "any");

let re5 = 0 << 0;
AssertType(re5, "number");
AssertType(0 << 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let re6 = b << 0;
AssertType(re6, "number");
AssertType(b << 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let re7 = 0 << b;
AssertType(re7, "number");
AssertType(0 << b, "number");
AssertType(0, "int");
AssertType(b, "number");

let re8 = b << b;
AssertType(re8, "number");
AssertType(b << b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator >>
let rf1 = a >> a;
AssertType(rf1, "number");
AssertType(a >> a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rf2 = a >> b;
AssertType(rf2, "number");
AssertType(a >> b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rf3 = a >> 0;
AssertType(rf3, "number");
AssertType(a >> 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rf4 = 0 >> a;
AssertType(rf4, "number");
AssertType(0 >> a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rf5 = 0 >> 0;
AssertType(rf5, "number");
AssertType(0 >> 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rf6 = b >> 0;
AssertType(rf6, "number");
AssertType(b >> 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rf7 = 0 >> b;
AssertType(rf7, "number");
AssertType(0 >> b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rf8 = b >> b;
AssertType(rf8, "number");
AssertType(b >> b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator >>>
let rg1 = a >>> a;
AssertType(rg1, "number");
AssertType(a >>> a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rg2 = a >>> b;
AssertType(rg2, "number");
AssertType(a >>> b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rg3 = a >>> 0;
AssertType(rg3, "number");
AssertType(a >>> 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rg4 = 0 >>> a;
AssertType(rg4, "number");
AssertType(0 >>> a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rg5 = 0 >>> 0;
AssertType(rg5, "number");
AssertType(0 >>> 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rg6 = b >>> 0;
AssertType(rg6, "number");
AssertType(b >>> 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rg7 = 0 >>> b;
AssertType(rg7, "number");
AssertType(0 >>> b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rg8 = b >>> b;
AssertType(rg8, "number");
AssertType(b >>> b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator &
let rh1 = a & a;
AssertType(rh1, "number");
AssertType(a & a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rh2 = a & b;
AssertType(rh2, "number");
AssertType(a & b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rh3 = a & 0;
AssertType(rh3, "number");
AssertType(a & 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rh4 = 0 & a;
AssertType(rh4, "number");
AssertType(0 & a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rh5 = 0 & 0;
AssertType(rh5, "number");
AssertType(0 & 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rh6 = b & 0;
AssertType(rh6, "number");
AssertType(b & 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rh7 = 0 & b;
AssertType(rh7, "number");
AssertType(0 & b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rh8 = b & b;
AssertType(rh8, "number");
AssertType(b & b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator ^
let ri1 = a ^ a;
AssertType(ri1, "number");
AssertType(a ^ a, "number");
AssertType(a, "any");
AssertType(a, "any");

let ri2 = a ^ b;
AssertType(ri2, "number");
AssertType(a ^ b, "number");
AssertType(a, "any");
AssertType(b, "number");

let ri3 = a ^ 0;
AssertType(ri3, "number");
AssertType(a ^ 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let ri4 = 0 ^ a;
AssertType(ri4, "number");
AssertType(0 ^ a, "number");
AssertType(0, "int");
AssertType(a, "any");

let ri5 = 0 ^ 0;
AssertType(ri5, "number");
AssertType(0 ^ 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let ri6 = b ^ 0;
AssertType(ri6, "number");
AssertType(b ^ 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let ri7 = 0 ^ b;
AssertType(ri7, "number");
AssertType(0 ^ b, "number");
AssertType(0, "int");
AssertType(b, "number");

let ri8 = b ^ b;
AssertType(ri8, "number");
AssertType(b ^ b, "number");
AssertType(b, "number");
AssertType(b, "number");

// operator |
let rj1 = a | a;
AssertType(rj1, "number");
AssertType(a | a, "number");
AssertType(a, "any");
AssertType(a, "any");

let rj2 = a | b;
AssertType(rj2, "number");
AssertType(a | b, "number");
AssertType(a, "any");
AssertType(b, "number");

let rj3 = a | 0;
AssertType(rj3, "number");
AssertType(a | 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let rj4 = 0 | a;
AssertType(rj4, "number");
AssertType(0 | a, "number");
AssertType(0, "int");
AssertType(a, "any");

let rj5 = 0 | 0;
AssertType(rj5, "number");
AssertType(0 | 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let rj6 = b | 0;
AssertType(rj6, "number");
AssertType(b | 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let rj7 = 0 | b;
AssertType(rj7, "number");
AssertType(0 | b, "number");
AssertType(0, "int");
AssertType(b, "number");

let rj8 = b | b;
AssertType(rj8, "number");
AssertType(b | b, "number");
AssertType(b, "number");
AssertType(b, "number");


