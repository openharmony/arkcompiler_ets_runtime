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

// === tests/cases/conformance/jsdoc/0.js ===
declare function AssertType(value:any, type:string):void;
// @ts-check
let lol = "hello Lol"
AssertType(lol, "string");
AssertType("hello Lol", "string");

const obj = {
AssertType(obj, "{ [string]: union; foo: union; bar: union; method1(number): number; lol: string; arrowFunc: (number) => number; }");
AssertType({  /** @type {string|undefined} */  foo: undefined,  /** @type {string|undefined} */  bar: "42",  /** @type {function(number): number} */  method1(n1) {      return n1 + 42;  },  /** @type {string} */  lol,  /** @type {number} */  ['b' + 'ar1']: 42,  /** @type {function(number): number} */  arrowFunc: (num) => num + 42}, "{ [string]: union; foo: union; bar: union; method1(number): number; lol: string; arrowFunc: (number) => number; }");

  /** @type {string|undefined} */
  foo: undefined,
AssertType(foo, "union");
AssertType(undefined, "undefined");

  /** @type {string|undefined} */
  bar: "42",
AssertType(bar, "union");
AssertType("42", "string");

  /** @type {function(number): number} */
  method1(n1) {
AssertType(method1, "(number) => number");
AssertType(n1, "number");

AssertType(n1 + 42, "number");
AssertType(n1, "number");
AssertType(42, "int");
      return n1 + 42;

  },
  /** @type {string} */
  lol,
AssertType(lol, "string");

  /** @type {number} */
  ['b' + 'ar1']: 42,
AssertType(['b' + 'ar1'], "number");
AssertType('b' + 'ar1', "string");
AssertType('b', "string");
AssertType('ar1', "string");
AssertType(42, "int");

  /** @type {function(number): number} */
  arrowFunc: (num) => num + 42
AssertType(arrowFunc, "(number) => number");
AssertType((num) => num + 42, "(number) => number");
AssertType(num, "number");
AssertType(num + 42, "number");
AssertType(num, "number");
AssertType(42, "int");
}
obj.foo = 'string'
AssertType(obj.foo = 'string', "string");
AssertType(obj.foo, "union");
AssertType('string', "string");

obj.lol
AssertType(obj.lol, "string");

obj.bar = undefined;
AssertType(obj.bar = undefined, "undefined");
AssertType(obj.bar, "union");
AssertType(undefined, "undefined");

let k = obj.method1(0);
AssertType(k, "number");
AssertType(obj.method1(0), "number");
AssertType(obj.method1, "(number) => number");
AssertType(0, "int");

obj.bar1 = "42";
AssertType(obj.bar1 = "42", "string");
AssertType(obj.bar1, "union");
AssertType("42", "string");

obj.arrowFunc(0);
AssertType(obj.arrowFunc(0), "number");
AssertType(obj.arrowFunc, "(number) => number");
AssertType(0, "int");


