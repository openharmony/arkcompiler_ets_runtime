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

// === tests/cases/conformance/types/literal/literalTypesAndTypeAssertions.ts ===
declare function AssertType(value:any, type:string):void;
const obj = {
AssertType(obj, "{ a: "foo"; b: "foo"; c: string; }");
AssertType({    a: "foo" as "foo",    b: <"foo">"foo",    c: "foo"}, "{ a: "foo"; b: "foo"; c: string; }");

    a: "foo" as "foo",
AssertType(a, "string");
AssertType("foo" as "foo", "string");
AssertType("foo", "string");

    b: <"foo">"foo",
AssertType(b, "string");
AssertType(<"foo">"foo", "string");
AssertType("foo", "string");

    c: "foo"
AssertType(c, "string");
AssertType("foo", "string");

};

let x1 = 1 as (0 | 1);
AssertType(x1, "union");
AssertType(1 as (0 | 1), "union");
AssertType(1, "int");

let x2 = 1;
AssertType(x2, "number");
AssertType(1, "int");

let { a = "foo" } = { a: "foo" };
AssertType(a, "string");
AssertType("foo", "string");
AssertType({ a: "foo" }, "{ a?: string; }");
AssertType(a, "string");
AssertType("foo", "string");

let { b = "foo" as "foo" } = { b: "bar" };
AssertType(b, "union");
AssertType("foo" as "foo", "string");
AssertType("foo", "string");
AssertType({ b: "bar" }, "{ b?: "bar"; }");
AssertType(b, "string");
AssertType("bar", "string");

let { c = "foo" } = { c: "bar" as "bar" };
AssertType(c, "string");
AssertType("foo", "string");
AssertType({ c: "bar" as "bar" }, "{ c?: "bar"; }");
AssertType(c, "string");
AssertType("bar" as "bar", "string");
AssertType("bar", "string");

let { d = "foo" as "foo" } = { d: "bar" as "bar" };
AssertType(d, "union");
AssertType("foo" as "foo", "string");
AssertType("foo", "string");
AssertType({ d: "bar" as "bar" }, "{ d?: "bar"; }");
AssertType(d, "string");
AssertType("bar" as "bar", "string");
AssertType("bar", "string");


