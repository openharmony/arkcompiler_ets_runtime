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

// === tests/cases/compiler/destructuringTypeGuardFlow.ts ===
declare function AssertType(value:any, type:string):void;
type foo = {
  bar: number | null;
  baz: string;
  nested: {
    a: number;
    b: string | null;
  }
};

const aFoo: foo = { bar: 3, baz: "b", nested: { a: 1, b: "y" } };
AssertType(aFoo, "foo");
AssertType({ bar: 3, baz: "b", nested: { a: 1, b: "y" } }, "{ bar: number; baz: string; nested: { a: number; b: string; }; }");
AssertType(bar, "number");
AssertType(3, "int");
AssertType(baz, "string");
AssertType("b", "string");
AssertType(nested, "{ a: number; b: string; }");
AssertType({ a: 1, b: "y" }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "string");
AssertType("y", "string");

if (aFoo.bar && aFoo.nested.b) {
  const { bar, baz, nested: {a, b: text} } = aFoo;
AssertType(bar, "number");
AssertType(baz, "string");
AssertType(nested, "any");
AssertType(a, "number");
AssertType(b, "any");
AssertType(text, "string");
AssertType(aFoo, "foo");

  const right: number = aFoo.bar;
AssertType(right, "number");
AssertType(aFoo.bar, "number");

  const wrong: number = bar;
AssertType(wrong, "number");
AssertType(bar, "number");

  const another: string = baz;
AssertType(another, "string");
AssertType(baz, "string");

  const aAgain: number = a;
AssertType(aAgain, "number");
AssertType(a, "number");

  const bAgain: string = text;
AssertType(bAgain, "string");
AssertType(text, "string");
}

type bar = {
  elem1: number | null;
  elem2: foo | null;
};

const bBar = { elem1: 7, elem2: aFoo };
AssertType(bBar, "{ elem1: number; elem2: foo; }");
AssertType({ elem1: 7, elem2: aFoo }, "{ elem1: number; elem2: foo; }");
AssertType(elem1, "number");
AssertType(7, "int");
AssertType(elem2, "foo");
AssertType(aFoo, "foo");

if (bBar.elem2 && bBar.elem2.bar && bBar.elem2.nested.b) {
  const { bar, baz, nested: {a, b: text} } = bBar.elem2;
AssertType(bar, "number");
AssertType(baz, "string");
AssertType(nested, "any");
AssertType(a, "number");
AssertType(b, "any");
AssertType(text, "string");
AssertType(bBar.elem2, "foo");

  const right: number = bBar.elem2.bar;
AssertType(right, "number");
AssertType(bBar.elem2.bar, "number");
AssertType(bBar.elem2, "foo");

  const wrong: number = bar;
AssertType(wrong, "number");
AssertType(bar, "number");

  const another: string = baz;
AssertType(another, "string");
AssertType(baz, "string");

  const aAgain: number = a;
AssertType(aAgain, "number");
AssertType(a, "number");

  const bAgain: string = text;
AssertType(bAgain, "string");
AssertType(text, "string");
}


