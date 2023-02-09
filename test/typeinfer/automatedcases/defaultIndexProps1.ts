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

// === tests/cases/compiler/defaultIndexProps1.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
	public v = "Yo";
}

let f = new Foo();
AssertType(f, "Foo");
AssertType(new Foo(), "Foo");
AssertType(Foo, "typeof Foo");

let q = f["v"];
AssertType(q, "string");
AssertType(f["v"], "string");
AssertType(f, "Foo");
AssertType("v", "string");

let o = {v:"Yo2"};
AssertType(o, "{ v: string; }");
AssertType({v:"Yo2"}, "{ v: string; }");
AssertType(v, "string");
AssertType("Yo2", "string");

let q2 = o["v"];
AssertType(q2, "string");
AssertType(o["v"], "string");
AssertType(o, "{ v: string; }");
AssertType("v", "string");


