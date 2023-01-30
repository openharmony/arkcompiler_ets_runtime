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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormIsTypeOnInterfaces.ts ===
declare function AssertType(value:any, type:string):void;
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


function isC1(x: any): x is C1 {
AssertType(true, "boolean");
    return true;
}

function isC2(x: any): x is C2 {
AssertType(true, "boolean");
    return true;
}

function isD1(x: any): x is D1 {
AssertType(true, "boolean");
    return true;
}

let c1: C1;
AssertType(c1, "C1");

let c2: C2;
AssertType(c2, "C2");

let d1: D1;
AssertType(d1, "D1");

let c1Orc2: C1 | C2;
AssertType(c1Orc2, "union");

str = isC1(c1Orc2) && c1Orc2.p1; // C1
AssertType(str = isC1(c1Orc2) && c1Orc2.p1, "string");
AssertType(str, "string");
AssertType(isC1(c1Orc2) && c1Orc2.p1, "string");
AssertType(isC1(c1Orc2), "boolean");
AssertType(isC1, "(any) => x is C1");
AssertType(c1Orc2, "union");
AssertType(c1Orc2.p1, "string");

num = isC2(c1Orc2) && c1Orc2.p2; // C2
AssertType(num = isC2(c1Orc2) && c1Orc2.p2, "number");
AssertType(num, "number");
AssertType(isC2(c1Orc2) && c1Orc2.p2, "number");
AssertType(isC2(c1Orc2), "boolean");
AssertType(isC2, "(any) => x is C2");
AssertType(c1Orc2, "union");
AssertType(c1Orc2.p2, "number");

str = isD1(c1Orc2) && c1Orc2.p1; // D1
AssertType(str = isD1(c1Orc2) && c1Orc2.p1, "string");
AssertType(str, "string");
AssertType(isD1(c1Orc2) && c1Orc2.p1, "string");
AssertType(isD1(c1Orc2), "boolean");
AssertType(isD1, "(any) => x is D1");
AssertType(c1Orc2, "union");
AssertType(c1Orc2.p1, "string");

num = isD1(c1Orc2) && c1Orc2.p3; // D1
AssertType(num = isD1(c1Orc2) && c1Orc2.p3, "number");
AssertType(num, "number");
AssertType(isD1(c1Orc2) && c1Orc2.p3, "number");
AssertType(isD1(c1Orc2), "boolean");
AssertType(isD1, "(any) => x is D1");
AssertType(c1Orc2, "union");
AssertType(c1Orc2.p3, "number");

let c2Ord1: C2 | D1;
AssertType(c2Ord1, "union");

num = isC2(c2Ord1) && c2Ord1.p2; // C2
AssertType(num = isC2(c2Ord1) && c2Ord1.p2, "number");
AssertType(num, "number");
AssertType(isC2(c2Ord1) && c2Ord1.p2, "number");
AssertType(isC2(c2Ord1), "boolean");
AssertType(isC2, "(any) => x is C2");
AssertType(c2Ord1, "union");
AssertType(c2Ord1.p2, "number");

num = isD1(c2Ord1) && c2Ord1.p3; // D1
AssertType(num = isD1(c2Ord1) && c2Ord1.p3, "number");
AssertType(num, "number");
AssertType(isD1(c2Ord1) && c2Ord1.p3, "number");
AssertType(isD1(c2Ord1), "boolean");
AssertType(isD1, "(any) => x is D1");
AssertType(c2Ord1, "union");
AssertType(c2Ord1.p3, "number");

str = isD1(c2Ord1) && c2Ord1.p1; // D1
AssertType(str = isD1(c2Ord1) && c2Ord1.p1, "string");
AssertType(str, "string");
AssertType(isD1(c2Ord1) && c2Ord1.p1, "string");
AssertType(isD1(c2Ord1), "boolean");
AssertType(isD1, "(any) => x is D1");
AssertType(c2Ord1, "union");
AssertType(c2Ord1.p1, "string");

let r2: C2 | D1 = isC1(c2Ord1) && c2Ord1; // C2 | D1
AssertType(r2, "union");
AssertType(isC1(c2Ord1) && c2Ord1, "D1");
AssertType(isC1(c2Ord1), "boolean");
AssertType(isC1, "(any) => x is C1");
AssertType(c2Ord1, "union");
AssertType(c2Ord1, "D1");


