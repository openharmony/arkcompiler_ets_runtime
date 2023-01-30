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

// === tests/cases/compiler/spreadBooleanRespectsFreshness.ts ===
declare function AssertType(value:any, type:string):void;
type Foo = FooBase | FooArray;
type FooBase = string | false;
type FooArray = FooBase[];

declare let foo1: Foo;
AssertType(foo1, "Foo");

declare let foo2: Foo;
AssertType(foo2, "Foo");

foo1 = [...Array.isArray(foo2) ? foo2 : [foo2]];
AssertType(foo1 = [...Array.isArray(foo2) ? foo2 : [foo2]], "FooBase[]");
AssertType(foo1, "Foo");
AssertType([...Array.isArray(foo2) ? foo2 : [foo2]], "FooBase[]");
AssertType(...Array.isArray(foo2) ? foo2 : [foo2], "FooBase");
AssertType(Array.isArray(foo2) ? foo2 : [foo2], "FooArray");
AssertType(Array.isArray(foo2), "boolean");
AssertType(Array.isArray, "(any) => arg is any[]");
AssertType(foo2, "Foo");
AssertType(foo2, "FooArray");
AssertType([foo2], "FooBase[]");
AssertType(foo2, "FooBase");


