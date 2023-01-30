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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesAsTypeParameterConstraint01.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T extends "foo">(f: (x: T) => T) {
AssertType(f, "(T) => T");
    return f;
}

function bar<T extends "foo" | "bar">(f: (x: T) => T) {
AssertType(f, "(T) => T");
    return f;
}

let f = foo(x => x);
AssertType(f, "("foo") => "foo"");
AssertType(foo(x => x), "("foo") => "foo"");
AssertType(foo, "<T extends "foo">((T) => T) => (T) => T");
AssertType(x => x, "("foo") => "foo"");
AssertType(x, "string");
AssertType(x, "string");

let fResult = f("foo");
AssertType(fResult, "string");
AssertType(f("foo"), "string");
AssertType(f, "("foo") => "foo"");
AssertType("foo", "string");

let g = foo((x => x));
AssertType(g, "("foo") => "foo"");
AssertType(foo((x => x)), "("foo") => "foo"");
AssertType(foo, "<T extends "foo">((T) => T) => (T) => T");
AssertType((x => x), "("foo") => "foo"");
AssertType(x => x, "("foo") => "foo"");
AssertType(x, "string");
AssertType(x, "string");

let gResult = g("foo");
AssertType(gResult, "string");
AssertType(g("foo"), "string");
AssertType(g, "("foo") => "foo"");
AssertType("foo", "string");

let h = bar(x => x);
AssertType(h, "(union) => union");
AssertType(bar(x => x), "(union) => union");
AssertType(bar, "<T extends union>((T) => T) => (T) => T");
AssertType(x => x, "(union) => union");
AssertType(x, "union");
AssertType(x, "union");

let hResult = h("foo");
AssertType(hResult, "union");
AssertType(h("foo"), "union");
AssertType(h, "(union) => union");
AssertType("foo", "string");

hResult = h("bar");
AssertType(hResult = h("bar"), "union");
AssertType(hResult, "union");
AssertType(h("bar"), "union");
AssertType(h, "(union) => union");
AssertType("bar", "string");


