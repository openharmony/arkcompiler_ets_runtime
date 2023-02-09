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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithSubtypeObjectOnIndexSignature.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    public a: string;
}

class Derived extends Base {
    public b: string;
}

let a1: { [a: string]: string };
AssertType(a1, "{ [string]: string; }");
AssertType(a, "string");

let b1: { [b: string]: string };
AssertType(b1, "{ [string]: string; }");
AssertType(b, "string");

let a2: { [index: string]: Base };
AssertType(a2, "{ [string]: Base; }");
AssertType(index, "string");

let b2: { [index: string]: Derived };
AssertType(b2, "{ [string]: Derived; }");
AssertType(index, "string");

let a3: { [index: number]: string };
AssertType(a3, "{ [number]: string; }");
AssertType(index, "number");

let b3: { [index: number]: string };
AssertType(b3, "{ [number]: string; }");
AssertType(index, "number");

let a4: { [index: number]: Base };
AssertType(a4, "{ [number]: Base; }");
AssertType(index, "number");

let b4: { [index: string]: Derived };
AssertType(b4, "{ [string]: Derived; }");
AssertType(index, "string");

// operator <
let r1a1 = a1 < b1;
AssertType(r1a1, "boolean");
AssertType(a1 < b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r1a1 = a2 < b2;
AssertType(r1a1, "boolean");
AssertType(a2 < b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r1a1 = a3 < b3;
AssertType(r1a1, "boolean");
AssertType(a3 < b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r1a1 = a4 < b4;
AssertType(r1a1, "boolean");
AssertType(a4 < b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r1b1 = b1 < a1;
AssertType(r1b1, "boolean");
AssertType(b1 < a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r1b1 = b2 < a2;
AssertType(r1b1, "boolean");
AssertType(b2 < a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r1b1 = b3 < a3;
AssertType(r1b1, "boolean");
AssertType(b3 < a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r1b1 = b4 < a4;
AssertType(r1b1, "boolean");
AssertType(b4 < a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator >
let r2a1 = a1 > b1;
AssertType(r2a1, "boolean");
AssertType(a1 > b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r2a1 = a2 > b2;
AssertType(r2a1, "boolean");
AssertType(a2 > b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r2a1 = a3 > b3;
AssertType(r2a1, "boolean");
AssertType(a3 > b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r2a1 = a4 > b4;
AssertType(r2a1, "boolean");
AssertType(a4 > b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r2b1 = b1 > a1;
AssertType(r2b1, "boolean");
AssertType(b1 > a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r2b1 = b2 > a2;
AssertType(r2b1, "boolean");
AssertType(b2 > a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r2b1 = b3 > a3;
AssertType(r2b1, "boolean");
AssertType(b3 > a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r2b1 = b4 > a4;
AssertType(r2b1, "boolean");
AssertType(b4 > a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator <=
let r3a1 = a1 <= b1;
AssertType(r3a1, "boolean");
AssertType(a1 <= b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r3a1 = a2 <= b2;
AssertType(r3a1, "boolean");
AssertType(a2 <= b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r3a1 = a3 <= b3;
AssertType(r3a1, "boolean");
AssertType(a3 <= b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r3a1 = a4 <= b4;
AssertType(r3a1, "boolean");
AssertType(a4 <= b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r3b1 = b1 <= a1;
AssertType(r3b1, "boolean");
AssertType(b1 <= a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r3b1 = b2 <= a2;
AssertType(r3b1, "boolean");
AssertType(b2 <= a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r3b1 = b3 <= a3;
AssertType(r3b1, "boolean");
AssertType(b3 <= a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r3b1 = b4 <= a4;
AssertType(r3b1, "boolean");
AssertType(b4 <= a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator >=
let r4a1 = a1 >= b1;
AssertType(r4a1, "boolean");
AssertType(a1 >= b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r4a1 = a2 >= b2;
AssertType(r4a1, "boolean");
AssertType(a2 >= b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r4a1 = a3 >= b3;
AssertType(r4a1, "boolean");
AssertType(a3 >= b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r4a1 = a4 >= b4;
AssertType(r4a1, "boolean");
AssertType(a4 >= b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r4b1 = b1 >= a1;
AssertType(r4b1, "boolean");
AssertType(b1 >= a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r4b1 = b2 >= a2;
AssertType(r4b1, "boolean");
AssertType(b2 >= a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r4b1 = b3 >= a3;
AssertType(r4b1, "boolean");
AssertType(b3 >= a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r4b1 = b4 >= a4;
AssertType(r4b1, "boolean");
AssertType(b4 >= a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator ==
let r5a1 = a1 == b1;
AssertType(r5a1, "boolean");
AssertType(a1 == b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r5a1 = a2 == b2;
AssertType(r5a1, "boolean");
AssertType(a2 == b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r5a1 = a3 == b3;
AssertType(r5a1, "boolean");
AssertType(a3 == b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r5a1 = a4 == b4;
AssertType(r5a1, "boolean");
AssertType(a4 == b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r5b1 = b1 == a1;
AssertType(r5b1, "boolean");
AssertType(b1 == a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r5b1 = b2 == a2;
AssertType(r5b1, "boolean");
AssertType(b2 == a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r5b1 = b3 == a3;
AssertType(r5b1, "boolean");
AssertType(b3 == a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r5b1 = b4 == a4;
AssertType(r5b1, "boolean");
AssertType(b4 == a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator !=
let r6a1 = a1 != b1;
AssertType(r6a1, "boolean");
AssertType(a1 != b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r6a1 = a2 != b2;
AssertType(r6a1, "boolean");
AssertType(a2 != b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r6a1 = a3 != b3;
AssertType(r6a1, "boolean");
AssertType(a3 != b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r6a1 = a4 != b4;
AssertType(r6a1, "boolean");
AssertType(a4 != b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r6b1 = b1 != a1;
AssertType(r6b1, "boolean");
AssertType(b1 != a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r6b1 = b2 != a2;
AssertType(r6b1, "boolean");
AssertType(b2 != a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r6b1 = b3 != a3;
AssertType(r6b1, "boolean");
AssertType(b3 != a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r6b1 = b4 != a4;
AssertType(r6b1, "boolean");
AssertType(b4 != a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator ===
let r7a1 = a1 === b1;
AssertType(r7a1, "boolean");
AssertType(a1 === b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r7a1 = a2 === b2;
AssertType(r7a1, "boolean");
AssertType(a2 === b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r7a1 = a3 === b3;
AssertType(r7a1, "boolean");
AssertType(a3 === b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r7a1 = a4 === b4;
AssertType(r7a1, "boolean");
AssertType(a4 === b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r7b1 = b1 === a1;
AssertType(r7b1, "boolean");
AssertType(b1 === a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r7b1 = b2 === a2;
AssertType(r7b1, "boolean");
AssertType(b2 === a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r7b1 = b3 === a3;
AssertType(r7b1, "boolean");
AssertType(b3 === a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r7b1 = b4 === a4;
AssertType(r7b1, "boolean");
AssertType(b4 === a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");

// operator !==
let r8a1 = a1 !== b1;
AssertType(r8a1, "boolean");
AssertType(a1 !== b1, "boolean");
AssertType(a1, "{ [string]: string; }");
AssertType(b1, "{ [string]: string; }");

let r8a1 = a2 !== b2;
AssertType(r8a1, "boolean");
AssertType(a2 !== b2, "boolean");
AssertType(a2, "{ [string]: Base; }");
AssertType(b2, "{ [string]: Derived; }");

let r8a1 = a3 !== b3;
AssertType(r8a1, "boolean");
AssertType(a3 !== b3, "boolean");
AssertType(a3, "{ [number]: string; }");
AssertType(b3, "{ [number]: string; }");

let r8a1 = a4 !== b4;
AssertType(r8a1, "boolean");
AssertType(a4 !== b4, "boolean");
AssertType(a4, "{ [number]: Base; }");
AssertType(b4, "{ [string]: Derived; }");

let r8b1 = b1 !== a1;
AssertType(r8b1, "boolean");
AssertType(b1 !== a1, "boolean");
AssertType(b1, "{ [string]: string; }");
AssertType(a1, "{ [string]: string; }");

let r8b1 = b2 !== a2;
AssertType(r8b1, "boolean");
AssertType(b2 !== a2, "boolean");
AssertType(b2, "{ [string]: Derived; }");
AssertType(a2, "{ [string]: Base; }");

let r8b1 = b3 !== a3;
AssertType(r8b1, "boolean");
AssertType(b3 !== a3, "boolean");
AssertType(b3, "{ [number]: string; }");
AssertType(a3, "{ [number]: string; }");

let r8b1 = b4 !== a4;
AssertType(r8b1, "boolean");
AssertType(b4 !== a4, "boolean");
AssertType(b4, "{ [string]: Derived; }");
AssertType(a4, "{ [number]: Base; }");


