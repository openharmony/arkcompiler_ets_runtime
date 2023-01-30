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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithSubtypeObjectOnOptionalProperty.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    a: string;
    b?: number;
}

interface J {
    a: string;
}

let a: I;
AssertType(a, "I");

let b: J;
AssertType(b, "J");

// operator <
let ra1 = a < b;
AssertType(ra1, "boolean");
AssertType(a < b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let ra2 = b < a;
AssertType(ra2, "boolean");
AssertType(b < a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator >
let rb1 = a > b;
AssertType(rb1, "boolean");
AssertType(a > b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let rb2 = b > a;
AssertType(rb2, "boolean");
AssertType(b > a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator <=
let rc1 = a <= b;
AssertType(rc1, "boolean");
AssertType(a <= b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let rc2 = b <= a;
AssertType(rc2, "boolean");
AssertType(b <= a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator >=
let rd1 = a >= b;
AssertType(rd1, "boolean");
AssertType(a >= b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let rd2 = b >= a;
AssertType(rd2, "boolean");
AssertType(b >= a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator ==
let re1 = a == b;
AssertType(re1, "boolean");
AssertType(a == b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let re2 = b == a;
AssertType(re2, "boolean");
AssertType(b == a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator !=
let rf1 = a != b;
AssertType(rf1, "boolean");
AssertType(a != b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let rf2 = b != a;
AssertType(rf2, "boolean");
AssertType(b != a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator ===
let rg1 = a === b;
AssertType(rg1, "boolean");
AssertType(a === b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let rg2 = b === a;
AssertType(rg2, "boolean");
AssertType(b === a, "boolean");
AssertType(b, "J");
AssertType(a, "I");

// operator !==
let rh1 = a !== b;
AssertType(rh1, "boolean");
AssertType(a !== b, "boolean");
AssertType(a, "I");
AssertType(b, "J");

let rh2 = b !== a;
AssertType(rh2, "boolean");
AssertType(b !== a, "boolean");
AssertType(b, "J");
AssertType(a, "I");


