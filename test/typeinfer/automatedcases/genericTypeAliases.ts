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

// === tests/cases/conformance/types/typeAliases/genericTypeAliases.ts ===
declare function AssertType(value:any, type:string):void;
type Tree<T> = T | { left: Tree<T>, right: Tree<T> };

let tree: Tree<number> = {
AssertType(tree, "Tree<number>");
AssertType({    left: {        left: 0,        right: {            left: 1,            right: 2        },    },    right: 3}, "{ left: { left: number; right: { left: number; right: number; }; }; right: number; }");

    left: {
AssertType(left, "{ left: number; right: { left: number; right: number; }; }");
AssertType({        left: 0,        right: {            left: 1,            right: 2        },    }, "{ left: number; right: { left: number; right: number; }; }");

        left: 0,
AssertType(left, "number");
AssertType(0, "int");

        right: {
AssertType(right, "{ left: number; right: number; }");
AssertType({            left: 1,            right: 2        }, "{ left: number; right: number; }");

            left: 1,
AssertType(left, "number");
AssertType(1, "int");

            right: 2
AssertType(right, "number");
AssertType(2, "int");

        },
    },
    right: 3
AssertType(right, "number");
AssertType(3, "int");

};

type Lazy<T> = T | (() => T);

let ls: Lazy<string>;
AssertType(ls, "Lazy<string>");

ls = "eager";
AssertType(ls = "eager", "string");
AssertType(ls, "Lazy<string>");
AssertType("eager", "string");

ls = () => "lazy";
AssertType(ls = () => "lazy", "() => string");
AssertType(ls, "Lazy<string>");
AssertType(() => "lazy", "() => string");
AssertType("lazy", "string");

type Foo<T> = T | { x: Foo<T> };
type Bar<U> = U | { x: Bar<U> };

// Deeply instantiated generics
let x: Foo<string>;
AssertType(x, "Foo<string>");

let y: Bar<string>;
AssertType(y, "Bar<string>");

x = y;
AssertType(x = y, "Bar<string>");
AssertType(x, "Foo<string>");
AssertType(y, "Bar<string>");

y = x;
AssertType(y = x, "Foo<string>");
AssertType(y, "Bar<string>");
AssertType(x, "Foo<string>");

x = "string";
AssertType(x = "string", "string");
AssertType(x, "Foo<string>");
AssertType("string", "string");

x = { x: "hello" };
AssertType(x = { x: "hello" }, "{ x: string; }");
AssertType(x, "Foo<string>");
AssertType({ x: "hello" }, "{ x: string; }");
AssertType(x, "string");
AssertType("hello", "string");

x = { x: { x: "world" } };
AssertType(x = { x: { x: "world" } }, "{ x: { x: string; }; }");
AssertType(x, "Foo<string>");
AssertType({ x: { x: "world" } }, "{ x: { x: string; }; }");
AssertType(x, "{ x: string; }");
AssertType({ x: "world" }, "{ x: string; }");
AssertType(x, "string");
AssertType("world", "string");

let z: Foo<number>;
AssertType(z, "Foo<number>");

z = 42;
AssertType(z = 42, "int");
AssertType(z, "Foo<number>");
AssertType(42, "int");

z = { x: 42 };
AssertType(z = { x: 42 }, "{ x: number; }");
AssertType(z, "Foo<number>");
AssertType({ x: 42 }, "{ x: number; }");
AssertType(x, "number");
AssertType(42, "int");

z = { x: { x: 42 } };
AssertType(z = { x: { x: 42 } }, "{ x: { x: number; }; }");
AssertType(z, "Foo<number>");
AssertType({ x: { x: 42 } }, "{ x: { x: number; }; }");
AssertType(x, "{ x: number; }");
AssertType({ x: 42 }, "{ x: number; }");
AssertType(x, "number");
AssertType(42, "int");

type Strange<T> = string;  // Type parameter not used
let s: Strange<number>;
AssertType(s, "string");

s = "hello";
AssertType(s = "hello", "string");
AssertType(s, "string");
AssertType("hello", "string");

interface AB<A, B> {
    a: A;
    b: B;
}

type Pair<T> = AB<T, T>;

interface TaggedPair<T> extends Pair<T> {
    tag: string;
}

let p: TaggedPair<number>;
AssertType(p, "TaggedPair<number>");

p.a = 1;
AssertType(p.a = 1, "int");
AssertType(p.a, "number");
AssertType(1, "int");

p.b = 2;
AssertType(p.b = 2, "int");
AssertType(p.b, "number");
AssertType(2, "int");

p.tag = "test";
AssertType(p.tag = "test", "string");
AssertType(p.tag, "string");
AssertType("test", "string");

function f<A>() {
    type Foo<T> = T | { x: Foo<T> };
AssertType(Foo, "union");
AssertType(x, "union");

    let x: Foo<A[]>;
AssertType(x, "union");

AssertType(x, "union");
    return x;
}

function g<B>() {
    type Bar<U> = U | { x: Bar<U> };
AssertType(Bar, "union");
AssertType(x, "union");

    let x: Bar<B[]>;
AssertType(x, "union");

AssertType(x, "union");
    return x;
}

// Deeply instantiated generics
let a = f<string>();
AssertType(a, "union");
AssertType(f<string>(), "union");
AssertType(f, "<A>() => union");

let b = g<string>();
AssertType(b, "union");
AssertType(g<string>(), "union");
AssertType(g, "<B>() => union");

a = b;
AssertType(a = b, "union");
AssertType(a, "union");
AssertType(b, "union");


