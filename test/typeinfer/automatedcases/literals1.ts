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

// === tests/cases/compiler/literals1.ts ===
declare function AssertType(value:any, type:string):void;
let a = 42;
AssertType(a, "number");
AssertType(42, "int");

let b = 0xFA34;
AssertType(b, "number");
AssertType(0xFA34, "int");

let c = 0.1715;
AssertType(c, "number");
AssertType(0.1715, "double");

let d = 3.14E5;
AssertType(d, "number");
AssertType(3.14E5, "int");

let e = 8.14e-5;
AssertType(e, "number");
AssertType(8.14e-5, "double");

let f = true;
AssertType(f, "boolean");
AssertType(true, "boolean");

let g = false;
AssertType(g, "boolean");
AssertType(false, "boolean");

let h = "";
AssertType(h, "string");
AssertType("", "string");

let i = "hi";
AssertType(i, "string");
AssertType("hi", "string");

let j = '';
AssertType(j, "string");
AssertType('', "string");

let k = 'q\tq';
AssertType(k, "string");
AssertType('q\tq', "string");

let m = /q/;
AssertType(m, "RegExp");
AssertType(/q/, "RegExp");

let n = /\d+/g;
AssertType(n, "RegExp");
AssertType(/\d+/g, "RegExp");

let o = /[3-5]+/i;
AssertType(o, "RegExp");
AssertType(/[3-5]+/i, "RegExp");


