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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithSubtypeObjectOnProperty.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    public a: string;
}

class Derived extends Base {
    public b: string;
}

class A1 {
    public a: Base;
    public b: Base;
}

class B1 {
    public a: Base;
    public b: Derived;
}

class A2 {
    private a;
}

class B2 extends A2 {
    private b;
}

let a1: A1;
AssertType(a1, "A1");

let a2: A2;
AssertType(a2, "A2");

let b1: B1;
AssertType(b1, "B1");

let b2: B2;
AssertType(b2, "B2");

// operator <
let ra1 = a1 < b1;
AssertType(ra1, "boolean");
AssertType(a1 < b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let ra2 = a2 < b2;
AssertType(ra2, "boolean");
AssertType(a2 < b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let ra3 = b1 < a1;
AssertType(ra3, "boolean");
AssertType(b1 < a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let ra4 = b2 < a2;
AssertType(ra4, "boolean");
AssertType(b2 < a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator >
let rb1 = a1 > b1;
AssertType(rb1, "boolean");
AssertType(a1 > b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let rb2 = a2 > b2;
AssertType(rb2, "boolean");
AssertType(a2 > b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let rb3 = b1 > a1;
AssertType(rb3, "boolean");
AssertType(b1 > a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let rb4 = b2 > a2;
AssertType(rb4, "boolean");
AssertType(b2 > a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator <=
let rc1 = a1 <= b1;
AssertType(rc1, "boolean");
AssertType(a1 <= b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let rc2 = a2 <= b2;
AssertType(rc2, "boolean");
AssertType(a2 <= b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let rc3 = b1 <= a1;
AssertType(rc3, "boolean");
AssertType(b1 <= a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let rc4 = b2 <= a2;
AssertType(rc4, "boolean");
AssertType(b2 <= a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator >=
let rd1 = a1 >= b1;
AssertType(rd1, "boolean");
AssertType(a1 >= b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let rd2 = a2 >= b2;
AssertType(rd2, "boolean");
AssertType(a2 >= b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let rd3 = b1 >= a1;
AssertType(rd3, "boolean");
AssertType(b1 >= a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let rd4 = b2 >= a2;
AssertType(rd4, "boolean");
AssertType(b2 >= a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator ==
let re1 = a1 == b1;
AssertType(re1, "boolean");
AssertType(a1 == b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let re2 = a2 == b2;
AssertType(re2, "boolean");
AssertType(a2 == b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let re3 = b1 == a1;
AssertType(re3, "boolean");
AssertType(b1 == a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let re4 = b2 == a2;
AssertType(re4, "boolean");
AssertType(b2 == a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator !=
let rf1 = a1 != b1;
AssertType(rf1, "boolean");
AssertType(a1 != b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let rf2 = a2 != b2;
AssertType(rf2, "boolean");
AssertType(a2 != b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let rf3 = b1 != a1;
AssertType(rf3, "boolean");
AssertType(b1 != a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let rf4 = b2 != a2;
AssertType(rf4, "boolean");
AssertType(b2 != a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator ===
let rg1 = a1 === b1;
AssertType(rg1, "boolean");
AssertType(a1 === b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let rg2 = a2 === b2;
AssertType(rg2, "boolean");
AssertType(a2 === b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let rg3 = b1 === a1;
AssertType(rg3, "boolean");
AssertType(b1 === a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let rg4 = b2 === a2;
AssertType(rg4, "boolean");
AssertType(b2 === a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");

// operator !==
let rh1 = a1 !== b1;
AssertType(rh1, "boolean");
AssertType(a1 !== b1, "boolean");
AssertType(a1, "A1");
AssertType(b1, "B1");

let rh2 = a2 !== b2;
AssertType(rh2, "boolean");
AssertType(a2 !== b2, "boolean");
AssertType(a2, "A2");
AssertType(b2, "B2");

let rh3 = b1 !== a1;
AssertType(rh3, "boolean");
AssertType(b1 !== a1, "boolean");
AssertType(b1, "B1");
AssertType(a1, "A1");

let rh4 = b2 !== a2;
AssertType(rh4, "boolean");
AssertType(b2 !== a2, "boolean");
AssertType(b2, "B2");
AssertType(a2, "A2");


