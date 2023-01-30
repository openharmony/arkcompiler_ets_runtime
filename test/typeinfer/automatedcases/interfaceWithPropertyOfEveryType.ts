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

// === tests/cases/conformance/interfaces/interfaceDeclarations/interfaceWithPropertyOfEveryType.ts ===
declare function AssertType(value:any, type:string):void;
class C { foo: string; }
function f1() { }
module M {
    export let y = 1;
}
enum E { A }

interface Foo {
    a: number;
    b: string;
    c: boolean;
    d: any;
    e: void;
    f: number[];
    g: Object;
    h: (x: number) => number;
    i: <T>(x: T) => T;
    j: Foo;
    k: C;
    l: typeof f1;
    m: typeof M;
    n: {};
    o: E;
}

let a: Foo = {
AssertType(a, "Foo");
AssertType({    a: 1,    b: '',    c: true,    d: {},    e: null ,    f: [1],    g: {},    h: (x: number) => 1,    i: <T>(x: T) => x,    j: <Foo>null,    k: new C(),    l: f1,    m: M,    n: {},    o: E.A}, "{ a: number; b: string; c: true; d: {}; e: null; f: number[]; g: {}; h: (number) => number; i: <T>(T) => T; j: Foo; k: C; l: () => void; m: typeof M; n: {}; o: E; }");

    a: 1,
AssertType(a, "number");
AssertType(1, "int");

    b: '',
AssertType(b, "string");
AssertType('', "string");

    c: true,
AssertType(c, "boolean");
AssertType(true, "boolean");

    d: {},
AssertType(d, "{}");
AssertType({}, "{}");

    e: null ,
AssertType(e, "null");
AssertType(null, "null");

    f: [1],
AssertType(f, "number[]");
AssertType([1], "number[]");
AssertType(1, "int");

    g: {},
AssertType(g, "{}");
AssertType({}, "{}");

    h: (x: number) => 1,
AssertType(h, "(number) => number");
AssertType((x: number) => 1, "(number) => number");
AssertType(x, "number");
AssertType(1, "int");

    i: <T>(x: T) => x,
AssertType(i, "<T>(T) => T");
AssertType(<T>(x: T) => x, "<T>(T) => T");
AssertType(x, "T");
AssertType(x, "T");

    j: <Foo>null,
AssertType(j, "Foo");
AssertType(<Foo>null, "Foo");
AssertType(null, "null");

    k: new C(),
AssertType(k, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

    l: f1,
AssertType(l, "() => void");
AssertType(f1, "() => void");

    m: M,
AssertType(m, "typeof M");
AssertType(M, "typeof M");

    n: {},
AssertType(n, "{}");
AssertType({}, "{}");

    o: E.A
AssertType(o, "E");
AssertType(E.A, "E");
}

