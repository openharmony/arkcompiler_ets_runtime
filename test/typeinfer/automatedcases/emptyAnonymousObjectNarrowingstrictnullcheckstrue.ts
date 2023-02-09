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

// === tests/cases/compiler/emptyAnonymousObjectNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
declare let nonNull: {};
AssertType(nonNull, "{}");

if (nonNull === "foo") {
  nonNull;
AssertType(nonNull, "string");
}
else {
  nonNull;
AssertType(nonNull, "{}");
}

declare let obj: { a: string };
AssertType(obj, "{ a: string; }");
AssertType(a, "string");

if (nonNull === obj) {
  nonNull;
AssertType(nonNull, "object");
}
else {
  nonNull;
AssertType(nonNull, "{}");
}

function f1<T>(x: T) {
  if (nonNull === x) {
AssertType(nonNull === x, "boolean");
AssertType(nonNull, "{}");
AssertType(x, "T");

    nonNull;
AssertType(nonNull, "{}");
  }
  else {
    nonNull;
AssertType(nonNull, "{}");
  }
}

function f2<T extends object>(x: T) {
  if (nonNull === x) {
AssertType(nonNull === x, "boolean");
AssertType(nonNull, "{}");
AssertType(x, "T");

    nonNull;
AssertType(nonNull, "{}");
  }
  else {
    nonNull;
AssertType(nonNull, "{}");
  }
}

declare let union: "xyz" | { a: string } | undefined;
AssertType(union, "union");
AssertType(a, "string");

if (nonNull === union) {
  nonNull;
AssertType(nonNull, "{}");
}
else {
  nonNull;
AssertType(nonNull, "{}");
}

if (nonNull === undefined) {
  nonNull;
AssertType(nonNull, "never");
}
else {
  nonNull;
AssertType(nonNull, "{}");
}

if (nonNull === null) {
  nonNull;
AssertType(nonNull, "never");
}
else {
  nonNull;
AssertType(nonNull, "{}");
}

if (nonNull == undefined) {
  nonNull;
AssertType(nonNull, "never");
}
else {
  nonNull;
AssertType(nonNull, "{}");
}

// Repro from #50567
const foo = (value: unknown): string => {
AssertType(foo, "(unknown) => string");
AssertType(value, "unknown");
AssertType((value: unknown): string => {  if (!value) {      return 'foo';  }  if (value === 'xyz') {      return value; // Type '{}' is not assignable to type 'string'.  }  return '';}, "(unknown) => string");

  if (!value) {
AssertType(!value, "boolean");
AssertType(value, "unknown");

AssertType('foo', "string");
      return 'foo';
  }
  if (value === 'xyz') {
AssertType(value === 'xyz', "boolean");
AssertType(value, "{}");
AssertType('xyz', "string");

AssertType(value, "string");
      return value; // Type '{}' is not assignable to type 'string'.
  }
AssertType('', "string");
  return '';

};


