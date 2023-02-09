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

// === tests/cases/conformance/types/literal/stringLiteralsWithTypeAssertions01.ts ===
declare function AssertType(value:any, type:string):void;
let fooOrBar: "foo" | "bar";
AssertType(fooOrBar, "union");

let a = "foo" as "bar";
AssertType(a, "string");
AssertType("foo" as "bar", "string");
AssertType("foo", "string");

let b = "bar" as "foo";
AssertType(b, "string");
AssertType("bar" as "foo", "string");
AssertType("bar", "string");

let c = fooOrBar as "foo";
AssertType(c, "string");
AssertType(fooOrBar as "foo", "string");
AssertType(fooOrBar, "union");

let d = fooOrBar as "bar";
AssertType(d, "string");
AssertType(fooOrBar as "bar", "string");
AssertType(fooOrBar, "union");

let e = fooOrBar as "baz";
AssertType(e, "string");
AssertType(fooOrBar as "baz", "string");
AssertType(fooOrBar, "union");

let f = "baz" as typeof fooOrBar;
AssertType(f, "union");
AssertType("baz" as typeof fooOrBar, "union");
AssertType("baz", "string");
AssertType(fooOrBar, "union");


