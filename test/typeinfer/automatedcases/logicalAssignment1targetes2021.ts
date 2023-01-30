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

// === tests/cases/conformance/es2021/logicalAssignment/logicalAssignment1.ts ===
declare function AssertType(value:any, type:string):void;
declare let a: string | undefined
AssertType(a, "union");

declare let b: string | undefined
AssertType(b, "union");

declare let c: string | undefined
AssertType(c, "union");

declare let d: number | undefined
AssertType(d, "union");

declare let e: number | undefined
AssertType(e, "union");

declare let f: number | undefined
AssertType(f, "union");

declare let g: 0 | 1 | 42
AssertType(g, "union");

declare let h: 0 | 1 | 42
AssertType(h, "union");

declare let i: 0 | 1 | 42
AssertType(i, "union");


a &&= "foo"
AssertType(a &&= "foo", "union");
AssertType(a, "union");
AssertType("foo", "string");

b ||= "foo"
AssertType(b ||= "foo", "string");
AssertType(b, "union");
AssertType("foo", "string");

c ??= "foo"
AssertType(c ??= "foo", "string");
AssertType(c, "union");
AssertType("foo", "string");


d &&= 42
AssertType(d &&= 42, "union");
AssertType(d, "union");
AssertType(42, "int");

e ||= 42
AssertType(e ||= 42, "number");
AssertType(e, "union");
AssertType(42, "int");

f ??= 42
AssertType(f ??= 42, "number");
AssertType(f, "union");
AssertType(42, "int");

g &&= 42
AssertType(g &&= 42, "union");
AssertType(g, "union");
AssertType(42, "int");

h ||= 42
AssertType(h ||= 42, "union");
AssertType(h, "union");
AssertType(42, "int");

i ??= 42
AssertType(i ??= 42, "union");
AssertType(i, "union");
AssertType(42, "int");


