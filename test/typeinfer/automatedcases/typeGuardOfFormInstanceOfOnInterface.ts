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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormInstanceOfOnInterface.ts ===
declare function AssertType(value:any, type:string):void;
// A type guard of the form x instanceof C, where C is of a subtype of the global type 'Function' 
// and C has a property named 'prototype'
//  - when true, narrows the type of x to the type of the 'prototype' property in C provided 
//    it is a subtype of the type of x, or
//  - when false, has no effect on the type of x.

interface C1 {
    (): C1;
    prototype: C1;
    p1: string;
}
interface C2 {
    (): C2;
    prototype: C2;
    p2: number;
}
interface D1 extends C1 {
    prototype: D1;
    p3: number;
}
let str: string;
AssertType(str, "string");

let num: number;
AssertType(num, "number");

let strOrNum: string | number;
AssertType(strOrNum, "union");

let c1: C1;
AssertType(c1, "C1");

let c2: C2;
AssertType(c2, "C2");

let d1: D1;
AssertType(d1, "D1");

let c1Orc2: C1 | C2;
AssertType(c1Orc2, "union");

str = c1Orc2 instanceof c1 && c1Orc2.p1; // C1
AssertType(str = c1Orc2 instanceof c1 && c1Orc2.p1, "string");
AssertType(str, "string");
AssertType(c1Orc2 instanceof c1 && c1Orc2.p1, "string");
AssertType(c1Orc2 instanceof c1, "boolean");
AssertType(c1Orc2, "union");
AssertType(c1, "C1");
AssertType(c1Orc2.p1, "string");

num = c1Orc2 instanceof c2 && c1Orc2.p2; // C2
AssertType(num = c1Orc2 instanceof c2 && c1Orc2.p2, "number");
AssertType(num, "number");
AssertType(c1Orc2 instanceof c2 && c1Orc2.p2, "number");
AssertType(c1Orc2 instanceof c2, "boolean");
AssertType(c1Orc2, "union");
AssertType(c2, "C2");
AssertType(c1Orc2.p2, "number");

str = c1Orc2 instanceof d1 && c1Orc2.p1; // C1
AssertType(str = c1Orc2 instanceof d1 && c1Orc2.p1, "string");
AssertType(str, "string");
AssertType(c1Orc2 instanceof d1 && c1Orc2.p1, "string");
AssertType(c1Orc2 instanceof d1, "boolean");
AssertType(c1Orc2, "union");
AssertType(d1, "D1");
AssertType(c1Orc2.p1, "string");

num = c1Orc2 instanceof d1 && c1Orc2.p3; // D1
AssertType(num = c1Orc2 instanceof d1 && c1Orc2.p3, "number");
AssertType(num, "number");
AssertType(c1Orc2 instanceof d1 && c1Orc2.p3, "number");
AssertType(c1Orc2 instanceof d1, "boolean");
AssertType(c1Orc2, "union");
AssertType(d1, "D1");
AssertType(c1Orc2.p3, "number");

let c2Ord1: C2 | D1;
AssertType(c2Ord1, "union");

num = c2Ord1 instanceof c2 && c2Ord1.p2; // C2
AssertType(num = c2Ord1 instanceof c2 && c2Ord1.p2, "number");
AssertType(num, "number");
AssertType(c2Ord1 instanceof c2 && c2Ord1.p2, "number");
AssertType(c2Ord1 instanceof c2, "boolean");
AssertType(c2Ord1, "union");
AssertType(c2, "C2");
AssertType(c2Ord1.p2, "number");

num = c2Ord1 instanceof d1 && c2Ord1.p3; // D1
AssertType(num = c2Ord1 instanceof d1 && c2Ord1.p3, "number");
AssertType(num, "number");
AssertType(c2Ord1 instanceof d1 && c2Ord1.p3, "number");
AssertType(c2Ord1 instanceof d1, "boolean");
AssertType(c2Ord1, "union");
AssertType(d1, "D1");
AssertType(c2Ord1.p3, "number");

str = c2Ord1 instanceof d1 && c2Ord1.p1; // D1
AssertType(str = c2Ord1 instanceof d1 && c2Ord1.p1, "string");
AssertType(str, "string");
AssertType(c2Ord1 instanceof d1 && c2Ord1.p1, "string");
AssertType(c2Ord1 instanceof d1, "boolean");
AssertType(c2Ord1, "union");
AssertType(d1, "D1");
AssertType(c2Ord1.p1, "string");

let r2: D1 | C2 = c2Ord1 instanceof c1 && c2Ord1; // C2 | D1
AssertType(r2, "union");
AssertType(c2Ord1 instanceof c1 && c2Ord1, "D1");
AssertType(c2Ord1 instanceof c1, "boolean");
AssertType(c2Ord1, "union");
AssertType(c1, "C1");
AssertType(c2Ord1, "D1");


