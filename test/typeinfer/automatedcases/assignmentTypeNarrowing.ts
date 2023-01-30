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

// === tests/cases/conformance/expressions/assignmentOperator/assignmentTypeNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
let x: string | number | boolean | RegExp;
AssertType(x, "union");

x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

x; // string
AssertType(x, "string");

[x] = [true];
AssertType([x] = [true], "[true]");
AssertType([x], "[union]");
AssertType(x, "union");
AssertType([true], "[true]");
AssertType(true, "boolean");

x; // boolean
AssertType(x, "boolean");

[x = ""] = [1];
AssertType([x = ""] = [1], "[number]");
AssertType([x = ""], "[string]");
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");
AssertType([1], "[number]");
AssertType(1, "int");

x; // string | number
AssertType(x, "union");

({x} = {x: true});
AssertType(({x} = {x: true}), "{ x: true; }");
AssertType({x} = {x: true}, "{ x: true; }");
AssertType({x}, "{ x: union; }");
AssertType(x, "union");
AssertType({x: true}, "{ x: true; }");
AssertType(x, "boolean");
AssertType(true, "boolean");

x; // boolean
AssertType(x, "boolean");

({y: x} = {y: 1});
AssertType(({y: x} = {y: 1}), "{ y: number; }");
AssertType({y: x} = {y: 1}, "{ y: number; }");
AssertType({y: x}, "{ y: union; }");
AssertType(y, "union");
AssertType(x, "union");
AssertType({y: 1}, "{ y: number; }");
AssertType(y, "number");
AssertType(1, "int");

x; // number
AssertType(x, "number");

({x = ""} = {x: true});
AssertType(({x = ""} = {x: true}), "{ x?: true; }");
AssertType({x = ""} = {x: true}, "{ x?: true; }");
AssertType({x = ""}, "{ x?: union; }");
AssertType(x, "union");
AssertType("", "string");
AssertType({x: true}, "{ x?: true; }");
AssertType(x, "boolean");
AssertType(true, "boolean");

x; // string | boolean
AssertType(x, "union");

({y: x = /a/} = {y: 1});
AssertType(({y: x = /a/} = {y: 1}), "{ y?: number; }");
AssertType({y: x = /a/} = {y: 1}, "{ y?: number; }");
AssertType({y: x = /a/}, "{ y?: RegExp; }");
AssertType(y, "RegExp");
AssertType(x = /a/, "RegExp");
AssertType(x, "union");
AssertType(/a/, "RegExp");
AssertType({y: 1}, "{ y?: number; }");
AssertType(y, "number");
AssertType(1, "int");

x; // number | RegExp
AssertType(x, "union");

let a: string[];
AssertType(a, "string[]");

for (x of a) {
    x; // string
AssertType(x, "string");
}

// Repro from #26405

type AOrArrA<T> = T | T[];
const arr: AOrArrA<{x?: "ok"}> = [{ x: "ok" }]; // weak type
AssertType(arr, "AOrArrA<{ x?: "ok"; }>");
AssertType(x, "string");
AssertType([{ x: "ok" }], "{ x: "ok"; }[]");
AssertType({ x: "ok" }, "{ x: "ok"; }");
AssertType(x, "string");
AssertType("ok", "string");

arr.push({ x: "ok" });
AssertType(arr.push({ x: "ok" }), "number");
AssertType(arr.push, "(...{ x?: "ok"; }[]) => number");
AssertType({ x: "ok" }, "{ x: "ok"; }");
AssertType(x, "string");
AssertType("ok", "string");


