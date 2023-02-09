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

// === tests/cases/compiler/index.ts ===
declare function AssertType(value:any, type:string):void;
import {Foo} from "./a";

const c = new Foo();
AssertType(c, "Foo");
AssertType(new Foo(), "Foo");
AssertType(Foo, "typeof Foo");

c.doThing({a: 42});
AssertType(c.doThing({a: 42}), "{ b: number; }");
AssertType(c.doThing, "({ a: number; }) => { b: number; }");
AssertType({a: 42}, "{ a: number; }");
AssertType(a, "number");
AssertType(42, "int");

export let x = c.doThing({a: 12});
AssertType(x, "{ b: number; }");
AssertType(c.doThing({a: 12}), "{ b: number; }");
AssertType(c.doThing, "({ a: number; }) => { b: number; }");
AssertType({a: 12}, "{ a: number; }");
AssertType(a, "number");
AssertType(12, "int");

export { c, Foo };


