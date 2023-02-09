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

// === tests/cases/compiler/arrayEvery.ts ===
declare function AssertType(value:any, type:string):void;
const foo: (number | string)[] = ['aaa'];
AssertType(foo, "(union)[]");
AssertType(['aaa'], "string[]");
AssertType('aaa', "string");

const isString = (x: unknown): x is string => typeof x === 'string';
AssertType(isString, "(unknown) => x is string");
AssertType((x: unknown): x is string => typeof x === 'string', "(unknown) => x is string");
AssertType(x, "unknown");
AssertType(typeof x === 'string', "boolean");
AssertType(typeof x, "union");
AssertType(x, "unknown");
AssertType('string', "string");

if (foo.every(isString)) {
  foo[0].slice(0);
AssertType(foo[0].slice(0), "string");
AssertType(foo[0].slice, "(?number, ?number) => string");
AssertType(foo[0], "string");
AssertType(foo, "string[]");
AssertType(0, "int");
AssertType(0, "int");
}


