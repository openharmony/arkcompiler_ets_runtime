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

// === tests/cases/conformance/types/rest/objectRestReadonly.ts ===
declare function AssertType(value:any, type:string):void;
// #23734
type ObjType = {
  foo: string
  baz: string
  quux: string
}

const obj: Readonly<ObjType> = {
AssertType(obj, "Readonly<ObjType>");
AssertType({  foo: 'bar',  baz: 'qux',  quux: 'quuz',}, "{ foo: string; baz: string; quux: string; }");

  foo: 'bar',
AssertType(foo, "string");
AssertType('bar', "string");

  baz: 'qux',
AssertType(baz, "string");
AssertType('qux', "string");

  quux: 'quuz',
AssertType(quux, "string");
AssertType('quuz', "string");
}

const { foo, ...rest } = obj
AssertType(foo, "string");
AssertType(rest, "{ baz: string; quux: string; }");
AssertType(obj, "Readonly<ObjType>");

delete rest.baz
AssertType(delete rest.baz, "boolean");
AssertType(rest.baz, "string");


