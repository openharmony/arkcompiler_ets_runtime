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

// === tests/cases/compiler/typeArgInference2.ts ===
declare function AssertType(value:any, type:string):void;
interface Item {
    name: string;
}

declare function foo<T extends Item>(x?: T, y?: T): T;

let z1 = foo(null);                   // any
AssertType(z1, "any");
AssertType(foo(null), "any");
AssertType(foo, "<T extends Item>(?T, ?T) => T");
AssertType(null, "null");

let z2 = foo();                       // Item
AssertType(z2, "Item");
AssertType(foo(), "Item");
AssertType(foo, "<T extends Item>(?T, ?T) => T");

let z3 = foo({ name: null });         // { name: any 
AssertType(z3, "{ name: any; }");

AssertType(foo({ name: null }), "{ name: any; }");

AssertType(foo, "<T extends Item>(?T, ?T) => T");

AssertType({ name: null }, "{ name: null; }");

AssertType(name, "null");

AssertType(null, "null");
}

let z4 = foo({ name: "abc" });        // { name: string 
AssertType(z4, "{ name: string; }");

AssertType(foo({ name: "abc" }), "{ name: string; }");

AssertType(foo, "<T extends Item>(?T, ?T) => T");

AssertType({ name: "abc" }, "{ name: string; }");

AssertType(name, "string");

AssertType("abc", "string");
}

let z5 = foo({ name: "abc", a: 5 });  // { name: string; a: number 
AssertType(z5, "{ name: string; a: number; }");

AssertType(foo({ name: "abc", a: 5 }), "{ name: string; a: number; }");

AssertType(foo, "<T extends Item>(?T, ?T) => T");

AssertType({ name: "abc", a: 5 }, "{ name: string; a: number; }");

AssertType(name, "string");

AssertType("abc", "string");

AssertType(a, "number");

AssertType(5, "int");
}

let z6 = foo({ name: "abc", a: 5 }, { name: "def", b: 5 });  // error
AssertType(z6, "union");
AssertType(foo({ name: "abc", a: 5 }, { name: "def", b: 5 }), "union");
AssertType(foo, "<T extends Item>(?T, ?T) => T");
AssertType({ name: "abc", a: 5 }, "{ name: string; a: number; }");
AssertType(name, "string");
AssertType("abc", "string");
AssertType(a, "number");
AssertType(5, "int");
AssertType({ name: "def", b: 5 }, "{ name: string; b: number; }");
AssertType(name, "string");
AssertType("def", "string");
AssertType(b, "number");
AssertType(5, "int");


