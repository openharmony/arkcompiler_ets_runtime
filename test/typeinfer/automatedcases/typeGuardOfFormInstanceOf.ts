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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormInstanceOf.ts ===
declare function AssertType(value:any, type:string):void;
// A type guard of the form x instanceof C, where C is of a subtype of the global type 'Function' 
// and C has a property named 'prototype'
//  - when true, narrows the type of x to the type of the 'prototype' property in C provided 
//    it is a subtype of the type of x, or
//  - when false, has no effect on the type of x.

class C1 {
    p1: string;
}
class C2 {
    p2: number;
}
class D1 extends C1 {
    p3: number;
}
class C3 {
    p4: number;
}
let str: string;
AssertType(str, "string");

let num: number;
AssertType(num, "number");

let strOrNum: string | number;
AssertType(strOrNum, "union");

let ctor1: C1 | C2;
AssertType(ctor1, "union");

str = ctor1 instanceof C1 && ctor1.p1; // C1
AssertType(str = ctor1 instanceof C1 && ctor1.p1, "string");
AssertType(str, "string");
AssertType(ctor1 instanceof C1 && ctor1.p1, "string");
AssertType(ctor1 instanceof C1, "boolean");
AssertType(ctor1, "union");
AssertType(C1, "typeof C1");
AssertType(ctor1.p1, "string");

num = ctor1 instanceof C2 && ctor1.p2; // C2
AssertType(num = ctor1 instanceof C2 && ctor1.p2, "number");
AssertType(num, "number");
AssertType(ctor1 instanceof C2 && ctor1.p2, "number");
AssertType(ctor1 instanceof C2, "boolean");
AssertType(ctor1, "union");
AssertType(C2, "typeof C2");
AssertType(ctor1.p2, "number");

str = ctor1 instanceof D1 && ctor1.p1; // D1
AssertType(str = ctor1 instanceof D1 && ctor1.p1, "string");
AssertType(str, "string");
AssertType(ctor1 instanceof D1 && ctor1.p1, "string");
AssertType(ctor1 instanceof D1, "boolean");
AssertType(ctor1, "union");
AssertType(D1, "typeof D1");
AssertType(ctor1.p1, "string");

num = ctor1 instanceof D1 && ctor1.p3; // D1
AssertType(num = ctor1 instanceof D1 && ctor1.p3, "number");
AssertType(num, "number");
AssertType(ctor1 instanceof D1 && ctor1.p3, "number");
AssertType(ctor1 instanceof D1, "boolean");
AssertType(ctor1, "union");
AssertType(D1, "typeof D1");
AssertType(ctor1.p3, "number");

let ctor2: C2 | D1;
AssertType(ctor2, "union");

num = ctor2 instanceof C2 && ctor2.p2; // C2
AssertType(num = ctor2 instanceof C2 && ctor2.p2, "number");
AssertType(num, "number");
AssertType(ctor2 instanceof C2 && ctor2.p2, "number");
AssertType(ctor2 instanceof C2, "boolean");
AssertType(ctor2, "union");
AssertType(C2, "typeof C2");
AssertType(ctor2.p2, "number");

num = ctor2 instanceof D1 && ctor2.p3; // D1
AssertType(num = ctor2 instanceof D1 && ctor2.p3, "number");
AssertType(num, "number");
AssertType(ctor2 instanceof D1 && ctor2.p3, "number");
AssertType(ctor2 instanceof D1, "boolean");
AssertType(ctor2, "union");
AssertType(D1, "typeof D1");
AssertType(ctor2.p3, "number");

str = ctor2 instanceof D1 && ctor2.p1; // D1
AssertType(str = ctor2 instanceof D1 && ctor2.p1, "string");
AssertType(str, "string");
AssertType(ctor2 instanceof D1 && ctor2.p1, "string");
AssertType(ctor2 instanceof D1, "boolean");
AssertType(ctor2, "union");
AssertType(D1, "typeof D1");
AssertType(ctor2.p1, "string");

let r2: D1 | C2 = ctor2 instanceof C1 && ctor2; // C2 | D1
AssertType(r2, "union");
AssertType(ctor2 instanceof C1 && ctor2, "D1");
AssertType(ctor2 instanceof C1, "boolean");
AssertType(ctor2, "union");
AssertType(C1, "typeof C1");
AssertType(ctor2, "D1");

let ctor3: C1 | C2;
AssertType(ctor3, "union");

if (ctor3 instanceof C1) {
    ctor3.p1; // C1
AssertType(ctor3.p1, "string");
}
else {
    ctor3.p2; // C2
AssertType(ctor3.p2, "number");
}

let ctor4: C1 | C2 | C3;
AssertType(ctor4, "union");

if (ctor4 instanceof C1) {
    ctor4.p1; // C1
AssertType(ctor4.p1, "string");
}
else if (ctor4 instanceof C2) {
    ctor4.p2; // C2
AssertType(ctor4.p2, "number");
}
else {
    ctor4.p4; // C3
AssertType(ctor4.p4, "number");
}

let ctor5: C1 | D1 | C2;
AssertType(ctor5, "union");

if (ctor5 instanceof C1) {
    ctor5.p1; // C1
AssertType(ctor5.p1, "string");
}
else {
    ctor5.p2; // C2
AssertType(ctor5.p2, "number");
}

let ctor6: C1 | C2 | C3;
AssertType(ctor6, "union");

if (ctor6 instanceof C1 || ctor6 instanceof C2) {
}
else {
    ctor6.p4; // C3
AssertType(ctor6.p4, "number");
}

