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

// === tests/cases/conformance/types/literal/literalTypesAndDestructuring.ts ===
declare function AssertType(value:any, type:string):void;
declare let x: { a: 0 | 1 | undefined };
AssertType(x, "{ a: union; }");
AssertType(a, "union");

let { a: a1 } = x;
AssertType(a, "any");
AssertType(a1, "union");
AssertType(x, "{ a: union; }");

let { a: a2 = 0 } = x;
AssertType(a, "any");
AssertType(a2, "union");
AssertType(0, "int");
AssertType(x, "{ a: union; }");

let { a: a3 = 2 } = x;
AssertType(a, "any");
AssertType(a3, "number");
AssertType(2, "int");
AssertType(x, "{ a: union; }");

let { a: a4 = 2 as const } = x;
AssertType(a, "any");
AssertType(a4, "union");
AssertType(2 as const, "int");
AssertType(2, "int");
AssertType(x, "{ a: union; }");

let b1 = x.a;
AssertType(b1, "union");
AssertType(x.a, "union");

let b2 = x.a ?? 0;
AssertType(b2, "union");
AssertType(x.a ?? 0, "union");
AssertType(x.a, "union");
AssertType(0, "int");

let b3 = x.a ?? 2;
AssertType(b3, "number");
AssertType(x.a ?? 2, "union");
AssertType(x.a, "union");
AssertType(2, "int");

let b4 = x.a ?? 2 as const;
AssertType(b4, "union");
AssertType(x.a ?? 2 as const, "union");
AssertType(x.a, "union");
AssertType(2 as const, "int");
AssertType(2, "int");

// Repro from #35693

interface Foo {
  bar: 'yo' | 'ha' | undefined;
}

let { bar = 'yo' } = {} as Foo;
AssertType(bar, "union");
AssertType('yo', "string");
AssertType({} as Foo, "Foo");
AssertType({}, "{}");

bar;  // "yo" | "ha"
AssertType(bar, "union");


