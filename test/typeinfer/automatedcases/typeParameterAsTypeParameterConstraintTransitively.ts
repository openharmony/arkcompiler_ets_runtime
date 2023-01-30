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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/typeParameterAsTypeParameterConstraintTransitively.ts ===
declare function AssertType(value:any, type:string):void;
// using a type parameter as a constraint for a type parameter is valid
// no errors expected

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

foo(1, 2, 3);
AssertType(foo(1, 2, 3), "int");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

foo({ x: 1 }, { x: 1, y: '' }, { x: 2, y: '', z: true });
AssertType(foo({ x: 1 }, { x: 1, y: '' }, { x: 2, y: '', z: true }), "{ x: number; y: string; z: boolean; }");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType({ x: 1, y: '' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "string");
AssertType('', "string");
AssertType({ x: 2, y: '', z: true }, "{ x: number; y: string; z: true; }");
AssertType(x, "number");
AssertType(2, "int");
AssertType(y, "string");
AssertType('', "string");
AssertType(z, "boolean");
AssertType(true, "boolean");

foo(a, b, c);
AssertType(foo(a, b, c), "C");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(a, "A");
AssertType(b, "B");
AssertType(c, "C");

foo(a, b, { foo: 1, bar: '', hm: true });
AssertType(foo(a, b, { foo: 1, bar: '', hm: true }), "{ foo: number; bar: string; hm: boolean; }");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(a, "A");
AssertType(b, "B");
AssertType({ foo: 1, bar: '', hm: true }, "{ foo: number; bar: string; hm: true; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(bar, "string");
AssertType('', "string");
AssertType(hm, "boolean");
AssertType(true, "boolean");

foo((x: number, y) => { }, (x) => { }, () => { });
AssertType(foo((x: number, y) => { }, (x) => { }, () => { }), "() => void");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType((x: number, y) => { }, "(number, any) => void");
AssertType(x, "number");
AssertType(y, "any");
AssertType((x) => { }, "(any) => void");
AssertType(x, "any");
AssertType(() => { }, "() => void");

function foo2<T extends A, U, V>(x: T, y: U, z: V): V { 
AssertType(z, "V");
return z; 
}

//function foo2<T extends A, U extends T, V extends U>(x: T, y: U, z: V): V { return z; }
foo(a, a, a);
AssertType(foo(a, a, a), "A");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(a, "A");
AssertType(a, "A");
AssertType(a, "A");

foo(a, b, c);
AssertType(foo(a, b, c), "C");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(a, "A");
AssertType(b, "B");
AssertType(c, "C");

foo(b, b, { foo: 1, bar: '', hm: '' });
AssertType(foo(b, b, { foo: 1, bar: '', hm: '' }), "{ foo: number; bar: string; hm: string; }");
AssertType(foo, "<T, U, V>(T, U, V) => V");
AssertType(b, "B");
AssertType(b, "B");
AssertType({ foo: 1, bar: '', hm: '' }, "{ foo: number; bar: string; hm: string; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(bar, "string");
AssertType('', "string");
AssertType(hm, "string");
AssertType('', "string");


