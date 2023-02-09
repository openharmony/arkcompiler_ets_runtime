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

// === tests/cases/conformance/expressions/assignmentOperator/assignmentGenericLookupTypeNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #26130

let mappedObject: {[K in "foo"]: null | {x: string}} = {foo: {x: "hello"}};
AssertType(mappedObject, "{ foo: { x: string; }; }");
AssertType(null, "null");
AssertType(x, "string");
AssertType({foo: {x: "hello"}}, "{ foo: { x: string; }; }");
AssertType(foo, "{ x: string; }");
AssertType({x: "hello"}, "{ x: string; }");
AssertType(x, "string");
AssertType("hello", "string");

declare function foo<T>(x: T): null | T;

function bar<K extends "foo">(key: K) {
  const element = foo(mappedObject[key]);
AssertType(element, "{ foo: { x: string; }; }[K]");
AssertType(foo(mappedObject[key]), "{ foo: { x: string; }; }[K]");
AssertType(foo, "<T>(T) => T");
AssertType(mappedObject[key], "{ foo: { x: string; }; }[K]");
AssertType(mappedObject, "{ foo: { x: string; }; }");
AssertType(key, "K");

  if (element == null)
AssertType(element == null, "boolean");
AssertType(element, "{ foo: { x: string; }; }[K]");
AssertType(null, "null");

    return;
  const x = element.x;
AssertType(x, "string");
AssertType(element.x, "string");
}


