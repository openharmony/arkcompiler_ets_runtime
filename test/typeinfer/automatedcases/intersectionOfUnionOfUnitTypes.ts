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

// === tests/cases/conformance/types/intersection/intersectionOfUnionOfUnitTypes.ts ===
declare function AssertType(value:any, type:string):void;
// @strict

const enum E { A, B, C, D, E, F }

let x0: ('a' | 'b' | 'c') & ('a' | 'b' | 'c');  // 'a' | 'b' | 'c'
AssertType(x0, "union");

let x1: ('a' | 'b' | 'c') & ('b' | 'c' | 'd');  // 'b' | 'c'
AssertType(x1, "union");

let x2: ('a' | 'b' | 'c') & ('c' | 'd' | 'e');  // 'c'
AssertType(x2, "string");

let x3: ('a' | 'b' | 'c') & ('d' | 'e' | 'f');  // never
AssertType(x3, "never");

let x4: ('a' | 'b' | 'c') & ('b' | 'c' | 'd') & ('c' | 'd' | 'e');  // 'c'
AssertType(x4, "string");

let x5: ('a' | 'b' | 'c') & ('b' | 'c' | 'd') & ('c' | 'd' | 'e') & ('d' | 'e' | 'f');  // never
AssertType(x5, "never");

let y0: (0 | 1 | 2) & (0 | 1 | 2);  // 0 | 1 | 2
AssertType(y0, "union");

let y1: (0 | 1 | 2) & (1 | 2 | 3);  // 1 | 2
AssertType(y1, "union");

let y2: (0 | 1 | 2) & (2 | 3 | 4);  // 2
AssertType(y2, "int");

let y3: (0 | 1 | 2) & (3 | 4 | 5);  // never
AssertType(y3, "never");

let y4: (0 | 1 | 2) & (1 | 2 | 3) & (2 | 3 | 4);  // 2
AssertType(y4, "int");

let y5: (0 | 1 | 2) & (1 | 2 | 3) & (2 | 3 | 4) & (3 | 4 | 5);  // never
AssertType(y5, "never");

let z0: (E.A | E.B | E.C) & (E.A | E.B | E.C);  // E.A | E.B | E.C
AssertType(z0, "union");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");

let z1: (E.A | E.B | E.C) & (E.B | E.C | E.D);  // E.B | E.C
AssertType(z1, "union");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");

let z2: (E.A | E.B | E.C) & (E.C | E.D | E.E);  // E.C
AssertType(z2, "E.C");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");

let z3: (E.A | E.B | E.C) & (E.D | E.E | E.F);  // never
AssertType(z3, "never");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");

let z4: (E.A | E.B | E.C) & (E.B | E.C | E.D) & (E.C | E.D | E.E);  // E.C
AssertType(z4, "E.C");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");

let z5: (E.A | E.B | E.C) & (E.B | E.C | E.D) & (E.C | E.D | E.E) & (E.D | E.E | E.F);  // never
AssertType(z5, "never");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");
AssertType(E, "any");


