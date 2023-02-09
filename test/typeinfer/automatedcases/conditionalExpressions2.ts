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

// === tests/cases/compiler/conditionalExpressions2.ts ===
declare function AssertType(value:any, type:string):void;
let a = false ? 1 : null;
AssertType(a, "number");
AssertType(false ? 1 : null, "int");
AssertType(false, "boolean");
AssertType(1, "int");
AssertType(null, "null");

let b = false ? undefined : 0;
AssertType(b, "number");
AssertType(false ? undefined : 0, "int");
AssertType(false, "boolean");
AssertType(undefined, "undefined");
AssertType(0, "int");

let c = false ? 1 : 0;
AssertType(c, "number");
AssertType(false ? 1 : 0, "union");
AssertType(false, "boolean");
AssertType(1, "int");
AssertType(0, "int");

let d = false ? false : true;
AssertType(d, "boolean");
AssertType(false ? false : true, "boolean");
AssertType(false, "boolean");
AssertType(false, "boolean");
AssertType(true, "boolean");

let e = false ? "foo" : "bar";
AssertType(e, "string");
AssertType(false ? "foo" : "bar", "union");
AssertType(false, "boolean");
AssertType("foo", "string");
AssertType("bar", "string");

let f = false ? null : undefined;
AssertType(f, "any");
AssertType(false ? null : undefined, "null");
AssertType(false, "boolean");
AssertType(null, "null");
AssertType(undefined, "undefined");

let g = true ? {g:5} : null;
AssertType(g, "{ g: number; }");
AssertType(true ? {g:5} : null, "{ g: number; }");
AssertType(true, "boolean");
AssertType({g:5}, "{ g: number; }");
AssertType(g, "number");
AssertType(5, "int");
AssertType(null, "null");

let h = [{h:5}, null];
AssertType(h, "{ h: number; }[]");
AssertType([{h:5}, null], "{ h: number; }[]");
AssertType({h:5}, "{ h: number; }");
AssertType(h, "number");
AssertType(5, "int");
AssertType(null, "null");

function i() { if (true) { 
AssertType(true, "boolean");
return { x: 5 }; } else { return null; } 

AssertType({ x: 5 }, "{ x: number; }");

AssertType(x, "number");

AssertType(5, "int");

AssertType(null, "null");
}


