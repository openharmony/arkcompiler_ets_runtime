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

// === tests/cases/conformance/es7/exponentiationOperator/emitExponentiationOperatorInTempalteString4ES6.ts ===
declare function AssertType(value:any, type:string):void;
let t1 = 10;
AssertType(t1, "number");
AssertType(10, "int");

let t2 = 10;
AssertType(t2, "number");
AssertType(10, "int");

let s;
AssertType(s, "any");

// With TemplateTail
`${t1 ** -t2} world`;
AssertType(`${t1 ** -t2} world`, "string");
AssertType(t1 ** -t2, "number");
AssertType(t1, "number");
AssertType(-t2, "number");
AssertType(t2, "number");

`${(-t1) ** t2 - t1} world`;
AssertType(`${(-t1) ** t2 - t1} world`, "string");
AssertType((-t1) ** t2 - t1, "number");
AssertType((-t1) ** t2, "number");
AssertType((-t1), "number");
AssertType(-t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`${(-++t1) ** t2 - t1} world`;
AssertType(`${(-++t1) ** t2 - t1} world`, "string");
AssertType((-++t1) ** t2 - t1, "number");
AssertType((-++t1) ** t2, "number");
AssertType((-++t1), "number");
AssertType(-++t1, "number");
AssertType(++t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`${(-t1++) ** t2 - t1} world`;
AssertType(`${(-t1++) ** t2 - t1} world`, "string");
AssertType((-t1++) ** t2 - t1, "number");
AssertType((-t1++) ** t2, "number");
AssertType((-t1++), "number");
AssertType(-t1++, "number");
AssertType(t1++, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`${(~t1) ** t2 ** --t1 } world`;
AssertType(`${(~t1) ** t2 ** --t1 } world`, "string");
AssertType((~t1) ** t2 ** --t1, "number");
AssertType((~t1), "number");
AssertType(~t1, "number");
AssertType(t1, "number");
AssertType(t2 ** --t1, "number");
AssertType(t2, "number");
AssertType(--t1, "number");
AssertType(t1, "number");

`${typeof (t1 ** t2 ** t1) } world`;
AssertType(`${typeof (t1 ** t2 ** t1) } world`, "string");
AssertType(typeof (t1 ** t2 ** t1), "union");
AssertType((t1 ** t2 ** t1), "number");
AssertType(t1 ** t2 ** t1, "number");
AssertType(t1, "number");
AssertType(t2 ** t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

// TempateHead & TemplateTail are empt
`${t1 ** -t2} hello world ${t1 ** -t2}`;
AssertType(`${t1 ** -t2} hello world ${t1 ** -t2}`, "string");
AssertType(t1 ** -t2, "number");
AssertType(t1, "number");
AssertType(-t2, "number");
AssertType(t2, "number");
AssertType(t1 ** -t2, "number");
AssertType(t1, "number");
AssertType(-t2, "number");
AssertType(t2, "number");

`${(-t1) ** t2 - t1} hello world ${(-t1) ** t2 - t1}`;
AssertType(`${(-t1) ** t2 - t1} hello world ${(-t1) ** t2 - t1}`, "string");
AssertType((-t1) ** t2 - t1, "number");
AssertType((-t1) ** t2, "number");
AssertType((-t1), "number");
AssertType(-t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");
AssertType((-t1) ** t2 - t1, "number");
AssertType((-t1) ** t2, "number");
AssertType((-t1), "number");
AssertType(-t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`${(-++t1) ** t2 - t1} hello world ${t1 ** (-++t1) **- t1}`;
AssertType(`${(-++t1) ** t2 - t1} hello world ${t1 ** (-++t1) **- t1}`, "string");
AssertType((-++t1) ** t2 - t1, "number");
AssertType((-++t1) ** t2, "number");
AssertType((-++t1), "number");
AssertType(-++t1, "number");
AssertType(++t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");
AssertType(t1 ** (-++t1) **- t1, "number");
AssertType(t1, "number");
AssertType((-++t1) **- t1, "number");
AssertType((-++t1), "number");
AssertType(-++t1, "number");
AssertType(++t1, "number");
AssertType(t1, "number");
AssertType(- t1, "number");
AssertType(t1, "number");

`${(-t1++) ** t2 - t1} hello world ${t2 ** (-t1++) **  - t1}`;
AssertType(`${(-t1++) ** t2 - t1} hello world ${t2 ** (-t1++) **  - t1}`, "string");
AssertType((-t1++) ** t2 - t1, "number");
AssertType((-t1++) ** t2, "number");
AssertType((-t1++), "number");
AssertType(-t1++, "number");
AssertType(t1++, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");
AssertType(t2 ** (-t1++) **  - t1, "number");
AssertType(t2, "number");
AssertType((-t1++) **  - t1, "number");
AssertType((-t1++), "number");
AssertType(-t1++, "number");
AssertType(t1++, "number");
AssertType(t1, "number");
AssertType(- t1, "number");
AssertType(t1, "number");

`${(~t1) ** t2 ** --t1 } hello world ${(~t1) ** t2 ** --t1 }`;
AssertType(`${(~t1) ** t2 ** --t1 } hello world ${(~t1) ** t2 ** --t1 }`, "string");
AssertType((~t1) ** t2 ** --t1, "number");
AssertType((~t1), "number");
AssertType(~t1, "number");
AssertType(t1, "number");
AssertType(t2 ** --t1, "number");
AssertType(t2, "number");
AssertType(--t1, "number");
AssertType(t1, "number");
AssertType((~t1) ** t2 ** --t1, "number");
AssertType((~t1), "number");
AssertType(~t1, "number");
AssertType(t1, "number");
AssertType(t2 ** --t1, "number");
AssertType(t2, "number");
AssertType(--t1, "number");
AssertType(t1, "number");

`${typeof (t1 ** t2 ** t1)} hello world ${typeof (t1 ** t2 ** t1)}`;
AssertType(`${typeof (t1 ** t2 ** t1)} hello world ${typeof (t1 ** t2 ** t1)}`, "string");
AssertType(typeof (t1 ** t2 ** t1), "union");
AssertType((t1 ** t2 ** t1), "number");
AssertType(t1 ** t2 ** t1, "number");
AssertType(t1, "number");
AssertType(t2 ** t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");
AssertType(typeof (t1 ** t2 ** t1), "union");
AssertType((t1 ** t2 ** t1), "number");
AssertType(t1 ** t2 ** t1, "number");
AssertType(t1, "number");
AssertType(t2 ** t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

// With templateHead
`hello ${(-t1) ** t2 - t1}`;
AssertType(`hello ${(-t1) ** t2 - t1}`, "string");
AssertType((-t1) ** t2 - t1, "number");
AssertType((-t1) ** t2, "number");
AssertType((-t1), "number");
AssertType(-t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`hello ${(-++t1) ** t2 - t1}`;
AssertType(`hello ${(-++t1) ** t2 - t1}`, "string");
AssertType((-++t1) ** t2 - t1, "number");
AssertType((-++t1) ** t2, "number");
AssertType((-++t1), "number");
AssertType(-++t1, "number");
AssertType(++t1, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`hello ${(-t1++) ** t2 - t1}`;
AssertType(`hello ${(-t1++) ** t2 - t1}`, "string");
AssertType((-t1++) ** t2 - t1, "number");
AssertType((-t1++) ** t2, "number");
AssertType((-t1++), "number");
AssertType(-t1++, "number");
AssertType(t1++, "number");
AssertType(t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");

`hello ${(~t1) ** t2 ** --t1 }`;
AssertType(`hello ${(~t1) ** t2 ** --t1 }`, "string");
AssertType((~t1) ** t2 ** --t1, "number");
AssertType((~t1), "number");
AssertType(~t1, "number");
AssertType(t1, "number");
AssertType(t2 ** --t1, "number");
AssertType(t2, "number");
AssertType(--t1, "number");
AssertType(t1, "number");

`hello ${typeof (t1 ** t2 ** t1)}`;
AssertType(`hello ${typeof (t1 ** t2 ** t1)}`, "string");
AssertType(typeof (t1 ** t2 ** t1), "union");
AssertType((t1 ** t2 ** t1), "number");
AssertType(t1 ** t2 ** t1, "number");
AssertType(t1, "number");
AssertType(t2 ** t1, "number");
AssertType(t2, "number");
AssertType(t1, "number");


