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

// === tests/cases/compiler/test.ts ===
declare function AssertType(value:any, type:string):void;
function getFoo() {
AssertType({ foo: { test: 42 } }, "{ foo: { test: number; }; }");
  return { foo: { test: 42 } 

AssertType(foo, "{ test: number; }");

AssertType({ test: 42 }, "{ test: number; }");

AssertType(test, "number");

AssertType(42, "int");
}
}

const { foo } = getFoo()
AssertType(foo, "{ test: number; }");
AssertType(getFoo(), "{ foo: { test: number; }; }");
AssertType(getFoo, "() => { foo: { test: number; }; }");

export type AliasType = typeof foo

const { foo: renamed } = getFoo()
AssertType(foo, "any");
AssertType(renamed, "{ test: number; }");
AssertType(getFoo(), "{ foo: { test: number; }; }");
AssertType(getFoo, "() => { foo: { test: number; }; }");

export type AliasType2 = typeof renamed

function getNested() {
AssertType({ a: { b: { c: 'd' } } }, "{ a: { b: { c: string; }; }; }");
  return { a: { b: { c: 'd' } } 

AssertType(a, "{ b: { c: string; }; }");

AssertType({ b: { c: 'd' } }, "{ b: { c: string; }; }");

AssertType(b, "{ c: string; }");

AssertType({ c: 'd' }, "{ c: string; }");

AssertType(c, "string");

AssertType('d', "string");
}
}

const { a: { b: { c } } } = getNested()
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "string");
AssertType(getNested(), "{ a: { b: { c: string; }; }; }");
AssertType(getNested, "() => { a: { b: { c: string; }; }; }");

export type AliasType3 = typeof c


