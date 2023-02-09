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

// === tests/cases/conformance/expressions/binaryOperators/arithmeticOperator/arithmeticOperatorWithEnumUnion.ts ===
declare function AssertType(value:any, type:string):void;
// operands of an enum type are treated as having the primitive type Number.

enum E {
    a,
    b
}
enum F {
    c,
    d
}

let a: any;
AssertType(a, "any");

let b: number;
AssertType(b, "number");

let c: E | F;
AssertType(c, "union");

// operator *
let ra1 = c * a;
AssertType(ra1, "number");
AssertType(c * a, "number");
AssertType(c, "union");
AssertType(a, "any");

let ra2 = c * b;
AssertType(ra2, "number");
AssertType(c * b, "number");
AssertType(c, "union");
AssertType(b, "number");

let ra3 = c * c;
AssertType(ra3, "number");
AssertType(c * c, "number");
AssertType(c, "union");
AssertType(c, "union");

let ra4 = a * c;
AssertType(ra4, "number");
AssertType(a * c, "number");
AssertType(a, "any");
AssertType(c, "union");

let ra5 = b * c;
AssertType(ra5, "number");
AssertType(b * c, "number");
AssertType(b, "number");
AssertType(c, "union");

let ra6 = E.a * a;
AssertType(ra6, "number");
AssertType(E.a * a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let ra7 = E.a * b;
AssertType(ra7, "number");
AssertType(E.a * b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let ra8 = E.a * E.b;
AssertType(ra8, "number");
AssertType(E.a * E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let ra9 = E.a * 1;
AssertType(ra9, "number");
AssertType(E.a * 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let ra10 = a * E.b;
AssertType(ra10, "number");
AssertType(a * E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let ra11 = b * E.b;
AssertType(ra11, "number");
AssertType(b * E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let ra12 = 1 * E.b;
AssertType(ra12, "number");
AssertType(1 * E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator /
let rb1 = c / a;
AssertType(rb1, "number");
AssertType(c / a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rb2 = c / b;
AssertType(rb2, "number");
AssertType(c / b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rb3 = c / c;
AssertType(rb3, "number");
AssertType(c / c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rb4 = a / c;
AssertType(rb4, "number");
AssertType(a / c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rb5 = b / c;
AssertType(rb5, "number");
AssertType(b / c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rb6 = E.a / a;
AssertType(rb6, "number");
AssertType(E.a / a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rb7 = E.a / b;
AssertType(rb7, "number");
AssertType(E.a / b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rb8 = E.a / E.b;
AssertType(rb8, "number");
AssertType(E.a / E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rb9 = E.a / 1;
AssertType(rb9, "number");
AssertType(E.a / 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rb10 = a / E.b;
AssertType(rb10, "number");
AssertType(a / E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rb11 = b / E.b;
AssertType(rb11, "number");
AssertType(b / E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rb12 = 1 / E.b;
AssertType(rb12, "number");
AssertType(1 / E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator %
let rc1 = c % a;
AssertType(rc1, "number");
AssertType(c % a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rc2 = c % b;
AssertType(rc2, "number");
AssertType(c % b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rc3 = c % c;
AssertType(rc3, "number");
AssertType(c % c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rc4 = a % c;
AssertType(rc4, "number");
AssertType(a % c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rc5 = b % c;
AssertType(rc5, "number");
AssertType(b % c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rc6 = E.a % a;
AssertType(rc6, "number");
AssertType(E.a % a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rc7 = E.a % b;
AssertType(rc7, "number");
AssertType(E.a % b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rc8 = E.a % E.b;
AssertType(rc8, "number");
AssertType(E.a % E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rc9 = E.a % 1;
AssertType(rc9, "number");
AssertType(E.a % 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rc10 = a % E.b;
AssertType(rc10, "number");
AssertType(a % E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rc11 = b % E.b;
AssertType(rc11, "number");
AssertType(b % E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rc12 = 1 % E.b;
AssertType(rc12, "number");
AssertType(1 % E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator -
let rd1 = c - a;
AssertType(rd1, "number");
AssertType(c - a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rd2 = c - b;
AssertType(rd2, "number");
AssertType(c - b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rd3 = c - c;
AssertType(rd3, "number");
AssertType(c - c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rd4 = a - c;
AssertType(rd4, "number");
AssertType(a - c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rd5 = b - c;
AssertType(rd5, "number");
AssertType(b - c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rd6 = E.a - a;
AssertType(rd6, "number");
AssertType(E.a - a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rd7 = E.a - b;
AssertType(rd7, "number");
AssertType(E.a - b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rd8 = E.a - E.b;
AssertType(rd8, "number");
AssertType(E.a - E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rd9 = E.a - 1;
AssertType(rd9, "number");
AssertType(E.a - 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rd10 = a - E.b;
AssertType(rd10, "number");
AssertType(a - E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rd11 = b - E.b;
AssertType(rd11, "number");
AssertType(b - E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rd12 = 1 - E.b;
AssertType(rd12, "number");
AssertType(1 - E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator <<
let re1 = c << a;
AssertType(re1, "number");
AssertType(c << a, "number");
AssertType(c, "union");
AssertType(a, "any");

let re2 = c << b;
AssertType(re2, "number");
AssertType(c << b, "number");
AssertType(c, "union");
AssertType(b, "number");

let re3 = c << c;
AssertType(re3, "number");
AssertType(c << c, "number");
AssertType(c, "union");
AssertType(c, "union");

let re4 = a << c;
AssertType(re4, "number");
AssertType(a << c, "number");
AssertType(a, "any");
AssertType(c, "union");

let re5 = b << c;
AssertType(re5, "number");
AssertType(b << c, "number");
AssertType(b, "number");
AssertType(c, "union");

let re6 = E.a << a;
AssertType(re6, "number");
AssertType(E.a << a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let re7 = E.a << b;
AssertType(re7, "number");
AssertType(E.a << b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let re8 = E.a << E.b;
AssertType(re8, "number");
AssertType(E.a << E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let re9 = E.a << 1;
AssertType(re9, "number");
AssertType(E.a << 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let re10 = a << E.b;
AssertType(re10, "number");
AssertType(a << E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let re11 = b << E.b;
AssertType(re11, "number");
AssertType(b << E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let re12 = 1 << E.b;
AssertType(re12, "number");
AssertType(1 << E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator >>
let rf1 = c >> a;
AssertType(rf1, "number");
AssertType(c >> a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rf2 = c >> b;
AssertType(rf2, "number");
AssertType(c >> b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rf3 = c >> c;
AssertType(rf3, "number");
AssertType(c >> c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rf4 = a >> c;
AssertType(rf4, "number");
AssertType(a >> c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rf5 = b >> c;
AssertType(rf5, "number");
AssertType(b >> c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rf6 = E.a >> a;
AssertType(rf6, "number");
AssertType(E.a >> a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rf7 = E.a >> b;
AssertType(rf7, "number");
AssertType(E.a >> b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rf8 = E.a >> E.b;
AssertType(rf8, "number");
AssertType(E.a >> E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rf9 = E.a >> 1;
AssertType(rf9, "number");
AssertType(E.a >> 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rf10 = a >> E.b;
AssertType(rf10, "number");
AssertType(a >> E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rf11 = b >> E.b;
AssertType(rf11, "number");
AssertType(b >> E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rf12 = 1 >> E.b;
AssertType(rf12, "number");
AssertType(1 >> E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator >>>
let rg1 = c >>> a;
AssertType(rg1, "number");
AssertType(c >>> a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rg2 = c >>> b;
AssertType(rg2, "number");
AssertType(c >>> b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rg3 = c >>> c;
AssertType(rg3, "number");
AssertType(c >>> c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rg4 = a >>> c;
AssertType(rg4, "number");
AssertType(a >>> c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rg5 = b >>> c;
AssertType(rg5, "number");
AssertType(b >>> c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rg6 = E.a >>> a;
AssertType(rg6, "number");
AssertType(E.a >>> a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rg7 = E.a >>> b;
AssertType(rg7, "number");
AssertType(E.a >>> b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rg8 = E.a >>> E.b;
AssertType(rg8, "number");
AssertType(E.a >>> E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rg9 = E.a >>> 1;
AssertType(rg9, "number");
AssertType(E.a >>> 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rg10 = a >>> E.b;
AssertType(rg10, "number");
AssertType(a >>> E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rg11 = b >>> E.b;
AssertType(rg11, "number");
AssertType(b >>> E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rg12 = 1 >>> E.b;
AssertType(rg12, "number");
AssertType(1 >>> E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator &
let rh1 = c & a;
AssertType(rh1, "number");
AssertType(c & a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rh2 = c & b;
AssertType(rh2, "number");
AssertType(c & b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rh3 = c & c;
AssertType(rh3, "number");
AssertType(c & c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rh4 = a & c;
AssertType(rh4, "number");
AssertType(a & c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rh5 = b & c;
AssertType(rh5, "number");
AssertType(b & c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rh6 = E.a & a;
AssertType(rh6, "number");
AssertType(E.a & a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rh7 = E.a & b;
AssertType(rh7, "number");
AssertType(E.a & b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rh8 = E.a & E.b;
AssertType(rh8, "number");
AssertType(E.a & E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rh9 = E.a & 1;
AssertType(rh9, "number");
AssertType(E.a & 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rh10 = a & E.b;
AssertType(rh10, "number");
AssertType(a & E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rh11 = b & E.b;
AssertType(rh11, "number");
AssertType(b & E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rh12 = 1 & E.b;
AssertType(rh12, "number");
AssertType(1 & E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator ^
let ri1 = c ^ a;
AssertType(ri1, "number");
AssertType(c ^ a, "number");
AssertType(c, "union");
AssertType(a, "any");

let ri2 = c ^ b;
AssertType(ri2, "number");
AssertType(c ^ b, "number");
AssertType(c, "union");
AssertType(b, "number");

let ri3 = c ^ c;
AssertType(ri3, "number");
AssertType(c ^ c, "number");
AssertType(c, "union");
AssertType(c, "union");

let ri4 = a ^ c;
AssertType(ri4, "number");
AssertType(a ^ c, "number");
AssertType(a, "any");
AssertType(c, "union");

let ri5 = b ^ c;
AssertType(ri5, "number");
AssertType(b ^ c, "number");
AssertType(b, "number");
AssertType(c, "union");

let ri6 = E.a ^ a;
AssertType(ri6, "number");
AssertType(E.a ^ a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let ri7 = E.a ^ b;
AssertType(ri7, "number");
AssertType(E.a ^ b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let ri8 = E.a ^ E.b;
AssertType(ri8, "number");
AssertType(E.a ^ E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let ri9 = E.a ^ 1;
AssertType(ri9, "number");
AssertType(E.a ^ 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let ri10 = a ^ E.b;
AssertType(ri10, "number");
AssertType(a ^ E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let ri11 = b ^ E.b;
AssertType(ri11, "number");
AssertType(b ^ E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let ri12 = 1 ^ E.b;
AssertType(ri12, "number");
AssertType(1 ^ E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");

// operator |
let rj1 = c | a;
AssertType(rj1, "number");
AssertType(c | a, "number");
AssertType(c, "union");
AssertType(a, "any");

let rj2 = c | b;
AssertType(rj2, "number");
AssertType(c | b, "number");
AssertType(c, "union");
AssertType(b, "number");

let rj3 = c | c;
AssertType(rj3, "number");
AssertType(c | c, "number");
AssertType(c, "union");
AssertType(c, "union");

let rj4 = a | c;
AssertType(rj4, "number");
AssertType(a | c, "number");
AssertType(a, "any");
AssertType(c, "union");

let rj5 = b | c;
AssertType(rj5, "number");
AssertType(b | c, "number");
AssertType(b, "number");
AssertType(c, "union");

let rj6 = E.a | a;
AssertType(rj6, "number");
AssertType(E.a | a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let rj7 = E.a | b;
AssertType(rj7, "number");
AssertType(E.a | b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rj8 = E.a | E.b;
AssertType(rj8, "number");
AssertType(E.a | E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let rj9 = E.a | 1;
AssertType(rj9, "number");
AssertType(E.a | 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let rj10 = a | E.b;
AssertType(rj10, "number");
AssertType(a | E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let rj11 = b | E.b;
AssertType(rj11, "number");
AssertType(b | E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let rj12 = 1 | E.b;
AssertType(rj12, "number");
AssertType(1 | E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");


