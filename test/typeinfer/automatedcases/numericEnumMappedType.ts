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

// === tests/cases/compiler/numericEnumMappedType.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #31771

enum E1 { ONE, TWO, THREE }
declare enum E2 { ONE, TWO, THREE }

type Bins1 = { [k in E1]?: string; }
type Bins2 = { [k in E2]?: string; }

const b1: Bins1 = {};
AssertType(b1, "Bins1");
AssertType({}, "{}");

const b2: Bins2 = {};
AssertType(b2, "Bins2");
AssertType({}, "{}");

const e1: E1 = E1.ONE;
AssertType(e1, "E1");
AssertType(E1.ONE, "E1.ONE");

const e2: E2 = E2.ONE;
AssertType(e2, "E2");
AssertType(E2.ONE, "E2.ONE");

b1[1] = "a";
AssertType(b1[1] = "a", "string");
AssertType(b1[1], "union");
AssertType(b1, "Bins1");
AssertType(1, "int");
AssertType("a", "string");

b1[e1] = "b";
AssertType(b1[e1] = "b", "string");
AssertType(b1[e1], "union");
AssertType(b1, "Bins1");
AssertType(e1, "E1.ONE");
AssertType("b", "string");

b2[1] = "a";
AssertType(b2[1] = "a", "string");
AssertType(b2[1], "union");
AssertType(b2, "Bins2");
AssertType(1, "int");
AssertType("a", "string");

b2[e2] = "b";
AssertType(b2[e2] = "b", "string");
AssertType(b2[e2], "union");
AssertType(b2, "Bins2");
AssertType(e2, "E2.ONE");
AssertType("b", "string");

// Multiple numeric enum types accrue to the same numeric index signature in a mapped type

declare function val(): number;

enum N1 { A = val(), B = val() }
enum N2 { C = val(), D = val() }

type T1 = { [K in N1 | N2]: K };

// Enum types with string valued members are always literal enum types and therefore
// ONE and TWO below are not computed members but rather just numerically valued members
// with auto-incremented values.

declare enum E { ONE, TWO, THREE = 'x' }
const e: E = E.ONE;
AssertType(e, "E");
AssertType(E.ONE, "E.ONE");

const x: E.ONE = e;
AssertType(x, "E.ONE");
AssertType(E, "any");
AssertType(e, "E.ONE");


