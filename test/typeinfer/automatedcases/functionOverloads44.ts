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

// === tests/cases/compiler/functionOverloads44.ts ===
declare function AssertType(value:any, type:string):void;
interface Animal { animal }
interface Dog extends Animal { dog }
interface Cat extends Animal { cat }

function foo1(bar: { a:number }[]): Dog;
function foo1(bar: { a:string }[]): Animal;
function foo1([x]: { a:number | string }[]): Dog {
AssertType(undefined, "undefined");
    return undefined;
}

function foo2(bar: { a:number }[]): Cat;
function foo2(bar: { a:string }[]): Cat | Dog;
function foo2([x]: { a:number | string }[]): Cat {
AssertType(undefined, "undefined");
    return undefined;
}


let x1 = foo1([{a: "str"}]);
AssertType(x1, "Animal");
AssertType(foo1([{a: "str"}]), "Animal");
AssertType(foo1, "{ ({ a: number; }[]): Dog; ({ a: string; }[]): Animal; }");
AssertType([{a: "str"}], "{ a: string; }[]");
AssertType({a: "str"}, "{ a: string; }");
AssertType(a, "string");
AssertType("str", "string");

let y1 = foo1([{a: 100}]);
AssertType(y1, "Dog");
AssertType(foo1([{a: 100}]), "Dog");
AssertType(foo1, "{ ({ a: number; }[]): Dog; ({ a: string; }[]): Animal; }");
AssertType([{a: 100}], "{ a: number; }[]");
AssertType({a: 100}, "{ a: number; }");
AssertType(a, "number");
AssertType(100, "int");

let x2 = foo2([{a: "str"}]);
AssertType(x2, "union");
AssertType(foo2([{a: "str"}]), "union");
AssertType(foo2, "{ ({ a: number; }[]): Cat; ({ a: string; }[]): union; }");
AssertType([{a: "str"}], "{ a: string; }[]");
AssertType({a: "str"}, "{ a: string; }");
AssertType(a, "string");
AssertType("str", "string");

let y2 = foo2([{a: 100}]);
AssertType(y2, "Cat");
AssertType(foo2([{a: 100}]), "Cat");
AssertType(foo2, "{ ({ a: number; }[]): Cat; ({ a: string; }[]): union; }");
AssertType([{a: 100}], "{ a: number; }[]");
AssertType({a: 100}, "{ a: number; }");
AssertType(a, "number");
AssertType(100, "int");


