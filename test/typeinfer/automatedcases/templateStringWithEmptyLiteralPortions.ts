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

// === tests/cases/conformance/es6/templates/templateStringWithEmptyLiteralPortions.ts ===
declare function AssertType(value:any, type:string):void;
let a = ``;
AssertType(a, "string");
AssertType(``, "string");

let b = `${ 0 }`;
AssertType(b, "string");
AssertType(`${ 0 }`, "string");
AssertType(0, "int");

let c = `1${ 0 }`;
AssertType(c, "string");
AssertType(`1${ 0 }`, "string");
AssertType(0, "int");

let d = `${ 0 }2`;
AssertType(d, "string");
AssertType(`${ 0 }2`, "string");
AssertType(0, "int");

let e = `1${ 0 }2`;
AssertType(e, "string");
AssertType(`1${ 0 }2`, "string");
AssertType(0, "int");

let f = `${ 0 }${ 0 }`;
AssertType(f, "string");
AssertType(`${ 0 }${ 0 }`, "string");
AssertType(0, "int");
AssertType(0, "int");

let g = `1${ 0 }${ 0 }`;
AssertType(g, "string");
AssertType(`1${ 0 }${ 0 }`, "string");
AssertType(0, "int");
AssertType(0, "int");

let h = `${ 0 }2${ 0 }`;
AssertType(h, "string");
AssertType(`${ 0 }2${ 0 }`, "string");
AssertType(0, "int");
AssertType(0, "int");

let i = `1${ 0 }2${ 0 }`;
AssertType(i, "string");
AssertType(`1${ 0 }2${ 0 }`, "string");
AssertType(0, "int");
AssertType(0, "int");

let j = `${ 0 }${ 0 }3`;
AssertType(j, "string");
AssertType(`${ 0 }${ 0 }3`, "string");
AssertType(0, "int");
AssertType(0, "int");

let k = `1${ 0 }${ 0 }3`;
AssertType(k, "string");
AssertType(`1${ 0 }${ 0 }3`, "string");
AssertType(0, "int");
AssertType(0, "int");

let l = `${ 0 }2${ 0 }3`;
AssertType(l, "string");
AssertType(`${ 0 }2${ 0 }3`, "string");
AssertType(0, "int");
AssertType(0, "int");

let m = `1${ 0 }2${ 0 }3`;
AssertType(m, "string");
AssertType(`1${ 0 }2${ 0 }3`, "string");
AssertType(0, "int");
AssertType(0, "int");


