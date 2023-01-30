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

// === tests/cases/compiler/inferenceAndSelfReferentialConstraint.ts ===
declare function AssertType(value:any, type:string):void;
// @strict

// Repro from #29520

type Test<K extends keyof any> = {
  [P in K | "foo"]: P extends "foo" ? true : () => any
}

function test<T extends Test<keyof T>>(arg: T) {
AssertType(arg, "T");
  return arg;
}

const res1 = test({
AssertType(res1, "{ foo: true; bar(): void; }");
AssertType(test({  foo: true,  bar() {  }}), "{ foo: true; bar(): void; }");
AssertType(test, "<T extends Test<keyof T>>(T) => T");
AssertType({  foo: true,  bar() {  }}, "{ foo: true; bar(): void; }");

  foo: true,
AssertType(foo, "boolean");
AssertType(true, "boolean");

  bar() {
AssertType(bar, "() => void");
  }
});

const res2 = test({
AssertType(res2, "{ foo: true; bar: () => void; }");
AssertType(test({  foo: true,  bar: function () {  }}), "{ foo: true; bar: () => void; }");
AssertType(test, "<T extends Test<keyof T>>(T) => T");
AssertType({  foo: true,  bar: function () {  }}, "{ foo: true; bar: () => void; }");

  foo: true,
AssertType(foo, "boolean");
AssertType(true, "boolean");

  bar: function () {
AssertType(bar, "() => void");
AssertType(function () {  }, "() => void");
  }
});

const res3 = test({
AssertType(res3, "{ foo: true; bar: () => void; }");
AssertType(test({  foo: true,  bar: () => {  }}), "{ foo: true; bar: () => void; }");
AssertType(test, "<T extends Test<keyof T>>(T) => T");
AssertType({  foo: true,  bar: () => {  }}, "{ foo: true; bar: () => void; }");

  foo: true,
AssertType(foo, "boolean");
AssertType(true, "boolean");

  bar: () => {
AssertType(bar, "() => void");
AssertType(() => {  }, "() => void");
  }
});


