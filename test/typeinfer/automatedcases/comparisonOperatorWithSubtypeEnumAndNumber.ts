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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithSubtypeEnumAndNumber.ts ===
declare function AssertType(value:any, type:string):void;
enum E { a, b, c }

let a: E;
AssertType(a, "E");

let b: number;
AssertType(b, "number");

// operator <
let ra1 = a < b;
AssertType(ra1, "boolean");
AssertType(a < b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let ra2 = b < a;
AssertType(ra2, "boolean");
AssertType(b < a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let ra3 = E.a < b;
AssertType(ra3, "boolean");
AssertType(E.a < b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let ra4 = b < E.a;
AssertType(ra4, "boolean");
AssertType(b < E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let ra5 = E.a < 0;
AssertType(ra5, "boolean");
AssertType(E.a < 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let ra6 = 0 < E.a;
AssertType(ra6, "boolean");
AssertType(0 < E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator >
let rb1 = a > b;
AssertType(rb1, "boolean");
AssertType(a > b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let rb2 = b > a;
AssertType(rb2, "boolean");
AssertType(b > a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let rb3 = E.a > b;
AssertType(rb3, "boolean");
AssertType(E.a > b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rb4 = b > E.a;
AssertType(rb4, "boolean");
AssertType(b > E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let rb5 = E.a > 0;
AssertType(rb5, "boolean");
AssertType(E.a > 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let rb6 = 0 > E.a;
AssertType(rb6, "boolean");
AssertType(0 > E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator <=
let rc1 = a <= b;
AssertType(rc1, "boolean");
AssertType(a <= b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let rc2 = b <= a;
AssertType(rc2, "boolean");
AssertType(b <= a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let rc3 = E.a <= b;
AssertType(rc3, "boolean");
AssertType(E.a <= b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rc4 = b <= E.a;
AssertType(rc4, "boolean");
AssertType(b <= E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let rc5 = E.a <= 0;
AssertType(rc5, "boolean");
AssertType(E.a <= 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let rc6 = 0 <= E.a;
AssertType(rc6, "boolean");
AssertType(0 <= E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator >=
let rd1 = a >= b;
AssertType(rd1, "boolean");
AssertType(a >= b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let rd2 = b >= a;
AssertType(rd2, "boolean");
AssertType(b >= a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let rd3 = E.a >= b;
AssertType(rd3, "boolean");
AssertType(E.a >= b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rd4 = b >= E.a;
AssertType(rd4, "boolean");
AssertType(b >= E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let rd5 = E.a >= 0;
AssertType(rd5, "boolean");
AssertType(E.a >= 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let rd6 = 0 >= E.a;
AssertType(rd6, "boolean");
AssertType(0 >= E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator ==
let re1 = a == b;
AssertType(re1, "boolean");
AssertType(a == b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let re2 = b == a;
AssertType(re2, "boolean");
AssertType(b == a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let re3 = E.a == b;
AssertType(re3, "boolean");
AssertType(E.a == b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let re4 = b == E.a;
AssertType(re4, "boolean");
AssertType(b == E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let re5 = E.a == 0;
AssertType(re5, "boolean");
AssertType(E.a == 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let re6 = 0 == E.a;
AssertType(re6, "boolean");
AssertType(0 == E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator !=
let rf1 = a != b;
AssertType(rf1, "boolean");
AssertType(a != b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let rf2 = b != a;
AssertType(rf2, "boolean");
AssertType(b != a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let rf3 = E.a != b;
AssertType(rf3, "boolean");
AssertType(E.a != b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rf4 = b != E.a;
AssertType(rf4, "boolean");
AssertType(b != E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let rf5 = E.a != 0;
AssertType(rf5, "boolean");
AssertType(E.a != 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let rf6 = 0 != E.a;
AssertType(rf6, "boolean");
AssertType(0 != E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator ===
let rg1 = a === b;
AssertType(rg1, "boolean");
AssertType(a === b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let rg2 = b === a;
AssertType(rg2, "boolean");
AssertType(b === a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let rg3 = E.a === b;
AssertType(rg3, "boolean");
AssertType(E.a === b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rg4 = b === E.a;
AssertType(rg4, "boolean");
AssertType(b === E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let rg5 = E.a === 0;
AssertType(rg5, "boolean");
AssertType(E.a === 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let rg6 = 0 === E.a;
AssertType(rg6, "boolean");
AssertType(0 === E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");

// operator !==
let rh1 = a !== b;
AssertType(rh1, "boolean");
AssertType(a !== b, "boolean");
AssertType(a, "E");
AssertType(b, "number");

let rh2 = b !== a;
AssertType(rh2, "boolean");
AssertType(b !== a, "boolean");
AssertType(b, "number");
AssertType(a, "E");

let rh3 = E.a !== b;
AssertType(rh3, "boolean");
AssertType(E.a !== b, "boolean");
AssertType(E.a, "E.a");
AssertType(b, "number");

let rh4 = b !== E.a;
AssertType(rh4, "boolean");
AssertType(b !== E.a, "boolean");
AssertType(b, "number");
AssertType(E.a, "E.a");

let rh5 = E.a !== 0;
AssertType(rh5, "boolean");
AssertType(E.a !== 0, "boolean");
AssertType(E.a, "E.a");
AssertType(0, "int");

let rh6 = 0 !== E.a;
AssertType(rh6, "boolean");
AssertType(0 !== E.a, "boolean");
AssertType(0, "int");
AssertType(E.a, "E.a");


