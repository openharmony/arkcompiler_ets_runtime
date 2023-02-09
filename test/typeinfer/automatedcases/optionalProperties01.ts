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

// === tests/cases/conformance/types/typeRelationships/comparable/optionalProperties01.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo {
  required1: string;
  required2: string;
  optional?: string;
}

const foo1 = { required1: "hello" } as Foo;
AssertType(foo1, "Foo");
AssertType({ required1: "hello" } as Foo, "Foo");
AssertType({ required1: "hello" }, "{ required1: string; }");
AssertType(required1, "string");
AssertType("hello", "string");

const foo2 = { required1: "hello", optional: "bar" } as Foo;
AssertType(foo2, "Foo");
AssertType({ required1: "hello", optional: "bar" } as Foo, "Foo");
AssertType({ required1: "hello", optional: "bar" }, "{ required1: string; optional: string; }");
AssertType(required1, "string");
AssertType("hello", "string");
AssertType(optional, "string");
AssertType("bar", "string");


