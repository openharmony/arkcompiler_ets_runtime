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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/typeParameterAsTypeParameterConstraintTransitively2.ts ===
declare function AssertType(value:any, type:string):void;
// using a type parameter as a constraint for a type parameter is invalid
// these should be errors at the type parameter constraint declarations, and have no downstream errors

interface A { foo: number }
interface B extends A { bar: string; }
interface C extends B { baz: boolean; }
let a: A;
AssertType(a, "A");

let b: B;
AssertType(b, "B");

let c: C;
AssertType(c, "C");

function foo<T, U, V>(x: T, y: U, z: V): V { 
AssertType(z, "V");
return z; 
}

//function foo<T, U extends T, V extends U>(x: T, y: U, z: V): V { return z; }

foo(1, 2, '');
AssertType(foo(1, 2, ''), "string");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(1, "int");
AssertType(2, "int");
AssertType('', "string");

foo({ x: 1 }, { x: 1, y: '' }, { x: 2, y: 2, z: true });
AssertType(foo({ x: 1 }, { x: 1, y: '' }, { x: 2, y: 2, z: true }), "{ x: number; y: number; z: boolean; }");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType({ x: 1, y: '' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "string");
AssertType('', "string");
AssertType({ x: 2, y: 2, z: true }, "{ x: number; y: number; z: true; }");
AssertType(x, "number");
AssertType(2, "int");
AssertType(y, "number");
AssertType(2, "int");
AssertType(z, "boolean");
AssertType(true, "boolean");

foo(a, b, a);
AssertType(foo(a, b, a), "A");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(a, "A");
AssertType(b, "B");
AssertType(a, "A");

foo(a, { foo: 1, bar: '', hm: true }, b);
AssertType(foo(a, { foo: 1, bar: '', hm: true }, b), "B");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(a, "A");
AssertType({ foo: 1, bar: '', hm: true }, "{ foo: number; bar: string; hm: true; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(bar, "string");
AssertType('', "string");
AssertType(hm, "boolean");
AssertType(true, "boolean");
AssertType(b, "B");

foo((x: number, y: string) => { }, (x, y: boolean) => { }, () => { });
AssertType(foo((x: number, y: string) => { }, (x, y: boolean) => { }, () => { }), "() => void");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType((x: number, y: string) => { }, "(number, string) => void");
AssertType(x, "number");
AssertType(y, "string");
AssertType((x, y: boolean) => { }, "(any, boolean) => void");
AssertType(x, "any");
AssertType(y, "boolean");
AssertType(() => { }, "() => void");

function foo2<T extends A, U extends A, V extends A>(x: T, y: U, z: V): V { 
AssertType(z, "V");
return z; 
}

//function foo2<T extends A, U extends T, V extends U>(x: T, y: U, z: V): V { return z; }
foo(b, a, c);
AssertType(foo(b, a, c), "C");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(b, "B");
AssertType(a, "A");
AssertType(c, "C");

foo(c, c, a);
AssertType(foo(c, c, a), "A");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(c, "C");
AssertType(c, "C");
AssertType(a, "A");


