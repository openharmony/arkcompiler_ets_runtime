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

// === tests/cases/conformance/es6/templates/templateStringBinaryOperationsES6.ts ===
declare function AssertType(value:any, type:string):void;
let a = 1 + `${ 3 }`;
AssertType(a, "string");
AssertType(1 + `${ 3 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 }`, "string");
AssertType(3, "int");

let b = 1 + `2${ 3 }`;
AssertType(b, "string");
AssertType(1 + `2${ 3 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 }`, "string");
AssertType(3, "int");

let c = 1 + `${ 3 }4`;
AssertType(c, "string");
AssertType(1 + `${ 3 }4`, "string");
AssertType(1, "int");
AssertType(`${ 3 }4`, "string");
AssertType(3, "int");

let d = 1 + `2${ 3 }4`;
AssertType(d, "string");
AssertType(1 + `2${ 3 }4`, "string");
AssertType(1, "int");
AssertType(`2${ 3 }4`, "string");
AssertType(3, "int");

let e = `${ 3 }` + 5;
AssertType(e, "string");
AssertType(`${ 3 }` + 5, "string");
AssertType(`${ 3 }`, "string");
AssertType(3, "int");
AssertType(5, "int");

let f = `2${ 3 }` + 5;
AssertType(f, "string");
AssertType(`2${ 3 }` + 5, "string");
AssertType(`2${ 3 }`, "string");
AssertType(3, "int");
AssertType(5, "int");

let g = `${ 3 }4` + 5;
AssertType(g, "string");
AssertType(`${ 3 }4` + 5, "string");
AssertType(`${ 3 }4`, "string");
AssertType(3, "int");
AssertType(5, "int");

let h = `2${ 3 }4` + 5;
AssertType(h, "string");
AssertType(`2${ 3 }4` + 5, "string");
AssertType(`2${ 3 }4`, "string");
AssertType(3, "int");
AssertType(5, "int");

let i = 1 + `${ 3 }` + 5;
AssertType(i, "string");
AssertType(1 + `${ 3 }` + 5, "string");
AssertType(1 + `${ 3 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 }`, "string");
AssertType(3, "int");
AssertType(5, "int");

let j = 1 + `2${ 3 }` + 5;
AssertType(j, "string");
AssertType(1 + `2${ 3 }` + 5, "string");
AssertType(1 + `2${ 3 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 }`, "string");
AssertType(3, "int");
AssertType(5, "int");

let k = 1 + `${ 3 }4` + 5;
AssertType(k, "string");
AssertType(1 + `${ 3 }4` + 5, "string");
AssertType(1 + `${ 3 }4`, "string");
AssertType(1, "int");
AssertType(`${ 3 }4`, "string");
AssertType(3, "int");
AssertType(5, "int");

let l = 1 + `2${ 3 }4` + 5;
AssertType(l, "string");
AssertType(1 + `2${ 3 }4` + 5, "string");
AssertType(1 + `2${ 3 }4`, "string");
AssertType(1, "int");
AssertType(`2${ 3 }4`, "string");
AssertType(3, "int");
AssertType(5, "int");

let a2 = 1 + `${ 3 - 4 }`;
AssertType(a2, "string");
AssertType(1 + `${ 3 - 4 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 - 4 }`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let b2 = 1 + `2${ 3 - 4 }`;
AssertType(b2, "string");
AssertType(1 + `2${ 3 - 4 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 - 4 }`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let c2 = 1 + `${ 3 - 4 }5`;
AssertType(c2, "string");
AssertType(1 + `${ 3 - 4 }5`, "string");
AssertType(1, "int");
AssertType(`${ 3 - 4 }5`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let d2 = 1 + `2${ 3 - 4 }5`;
AssertType(d2, "string");
AssertType(1 + `2${ 3 - 4 }5`, "string");
AssertType(1, "int");
AssertType(`2${ 3 - 4 }5`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let e2 = `${ 3 - 4 }` + 6;
AssertType(e2, "string");
AssertType(`${ 3 - 4 }` + 6, "string");
AssertType(`${ 3 - 4 }`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let f2 = `2${ 3 - 4 }` + 6;
AssertType(f2, "string");
AssertType(`2${ 3 - 4 }` + 6, "string");
AssertType(`2${ 3 - 4 }`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let g2 = `${ 3 - 4 }5` + 6;
AssertType(g2, "string");
AssertType(`${ 3 - 4 }5` + 6, "string");
AssertType(`${ 3 - 4 }5`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let h2 = `2${ 3 - 4 }5` + 6;
AssertType(h2, "string");
AssertType(`2${ 3 - 4 }5` + 6, "string");
AssertType(`2${ 3 - 4 }5`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let i2 = 1 + `${ 3 - 4 }` + 6;
AssertType(i2, "string");
AssertType(1 + `${ 3 - 4 }` + 6, "string");
AssertType(1 + `${ 3 - 4 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 - 4 }`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let j2 = 1 + `2${ 3 - 4 }` + 6;
AssertType(j2, "string");
AssertType(1 + `2${ 3 - 4 }` + 6, "string");
AssertType(1 + `2${ 3 - 4 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 - 4 }`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let k2 = 1 + `${ 3 - 4 }5` + 6;
AssertType(k2, "string");
AssertType(1 + `${ 3 - 4 }5` + 6, "string");
AssertType(1 + `${ 3 - 4 }5`, "string");
AssertType(1, "int");
AssertType(`${ 3 - 4 }5`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let l2 = 1 + `2${ 3 - 4 }5` + 6;
AssertType(l2, "string");
AssertType(1 + `2${ 3 - 4 }5` + 6, "string");
AssertType(1 + `2${ 3 - 4 }5`, "string");
AssertType(1, "int");
AssertType(`2${ 3 - 4 }5`, "string");
AssertType(3 - 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let a3 = 1 + `${ 3 * 4 }`;
AssertType(a3, "string");
AssertType(1 + `${ 3 * 4 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 * 4 }`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let b3 = 1 + `2${ 3 * 4 }`;
AssertType(b3, "string");
AssertType(1 + `2${ 3 * 4 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 * 4 }`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let c3 = 1 + `${ 3 * 4 }5`;
AssertType(c3, "string");
AssertType(1 + `${ 3 * 4 }5`, "string");
AssertType(1, "int");
AssertType(`${ 3 * 4 }5`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let d3 = 1 + `2${ 3 * 4 }5`;
AssertType(d3, "string");
AssertType(1 + `2${ 3 * 4 }5`, "string");
AssertType(1, "int");
AssertType(`2${ 3 * 4 }5`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let e3 = `${ 3 * 4 }` + 6;
AssertType(e3, "string");
AssertType(`${ 3 * 4 }` + 6, "string");
AssertType(`${ 3 * 4 }`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let f3 = `2${ 3 * 4 }` + 6;
AssertType(f3, "string");
AssertType(`2${ 3 * 4 }` + 6, "string");
AssertType(`2${ 3 * 4 }`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let g3 = `${ 3 * 4 }5` + 6;
AssertType(g3, "string");
AssertType(`${ 3 * 4 }5` + 6, "string");
AssertType(`${ 3 * 4 }5`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let h3 = `2${ 3 * 4 }5` + 6;
AssertType(h3, "string");
AssertType(`2${ 3 * 4 }5` + 6, "string");
AssertType(`2${ 3 * 4 }5`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let i3 = 1 + `${ 3 * 4 }` + 6;
AssertType(i3, "string");
AssertType(1 + `${ 3 * 4 }` + 6, "string");
AssertType(1 + `${ 3 * 4 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 * 4 }`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let j3 = 1 + `2${ 3 * 4 }` + 6;
AssertType(j3, "string");
AssertType(1 + `2${ 3 * 4 }` + 6, "string");
AssertType(1 + `2${ 3 * 4 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 * 4 }`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let k3 = 1 + `${ 3 * 4 }5` + 6;
AssertType(k3, "string");
AssertType(1 + `${ 3 * 4 }5` + 6, "string");
AssertType(1 + `${ 3 * 4 }5`, "string");
AssertType(1, "int");
AssertType(`${ 3 * 4 }5`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let l3 = 1 + `2${ 3 * 4 }5` + 6;
AssertType(l3, "string");
AssertType(1 + `2${ 3 * 4 }5` + 6, "string");
AssertType(1 + `2${ 3 * 4 }5`, "string");
AssertType(1, "int");
AssertType(`2${ 3 * 4 }5`, "string");
AssertType(3 * 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let a4 = 1 + `${ 3 & 4 }`;
AssertType(a4, "string");
AssertType(1 + `${ 3 & 4 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 & 4 }`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let b4 = 1 + `2${ 3 & 4 }`;
AssertType(b4, "string");
AssertType(1 + `2${ 3 & 4 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 & 4 }`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let c4 = 1 + `${ 3 & 4 }5`;
AssertType(c4, "string");
AssertType(1 + `${ 3 & 4 }5`, "string");
AssertType(1, "int");
AssertType(`${ 3 & 4 }5`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let d4 = 1 + `2${ 3 & 4 }5`;
AssertType(d4, "string");
AssertType(1 + `2${ 3 & 4 }5`, "string");
AssertType(1, "int");
AssertType(`2${ 3 & 4 }5`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");

let e4 = `${ 3 & 4 }` + 6;
AssertType(e4, "string");
AssertType(`${ 3 & 4 }` + 6, "string");
AssertType(`${ 3 & 4 }`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let f4 = `2${ 3 & 4 }` + 6;
AssertType(f4, "string");
AssertType(`2${ 3 & 4 }` + 6, "string");
AssertType(`2${ 3 & 4 }`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let g4 = `${ 3 & 4 }5` + 6;
AssertType(g4, "string");
AssertType(`${ 3 & 4 }5` + 6, "string");
AssertType(`${ 3 & 4 }5`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let h4 = `2${ 3 & 4 }5` + 6;
AssertType(h4, "string");
AssertType(`2${ 3 & 4 }5` + 6, "string");
AssertType(`2${ 3 & 4 }5`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let i4 = 1 + `${ 3 & 4 }` + 6;
AssertType(i4, "string");
AssertType(1 + `${ 3 & 4 }` + 6, "string");
AssertType(1 + `${ 3 & 4 }`, "string");
AssertType(1, "int");
AssertType(`${ 3 & 4 }`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let j4 = 1 + `2${ 3 & 4 }` + 6;
AssertType(j4, "string");
AssertType(1 + `2${ 3 & 4 }` + 6, "string");
AssertType(1 + `2${ 3 & 4 }`, "string");
AssertType(1, "int");
AssertType(`2${ 3 & 4 }`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let k4 = 1 + `${ 3 & 4 }5` + 6;
AssertType(k4, "string");
AssertType(1 + `${ 3 & 4 }5` + 6, "string");
AssertType(1 + `${ 3 & 4 }5`, "string");
AssertType(1, "int");
AssertType(`${ 3 & 4 }5`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");

let l4 = 1 + `2${ 3 & 4 }5` + 6;
AssertType(l4, "string");
AssertType(1 + `2${ 3 & 4 }5` + 6, "string");
AssertType(1 + `2${ 3 & 4 }5`, "string");
AssertType(1, "int");
AssertType(`2${ 3 & 4 }5`, "string");
AssertType(3 & 4, "number");
AssertType(3, "int");
AssertType(4, "int");
AssertType(6, "int");


