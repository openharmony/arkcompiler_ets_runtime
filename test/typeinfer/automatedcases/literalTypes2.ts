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

// === tests/cases/conformance/types/literal/literalTypes2.ts ===
declare function AssertType(value:any, type:string):void;
enum E {
    A, B, C
}

let cond: boolean;
AssertType(cond, "boolean");

function f1(p1 = 1, p2 = "abc", p3 = true, p4 = E.A) {
    let v1 = 1;
AssertType(v1, "number");
AssertType(1, "int");

    let v2 = -123;
AssertType(v2, "number");
AssertType(-123, "int");
AssertType(123, "int");

    let v3 = 3 + 4;
AssertType(v3, "number");
AssertType(3 + 4, "number");
AssertType(3, "int");
AssertType(4, "int");

    let v4 = "abc";
AssertType(v4, "string");
AssertType("abc", "string");

    let v5 = "";
AssertType(v5, "string");
AssertType("", "string");

    let v6 = "abc" + "def";
AssertType(v6, "string");
AssertType("abc" + "def", "string");
AssertType("abc", "string");
AssertType("def", "string");

    let v7 = true;
AssertType(v7, "boolean");
AssertType(true, "boolean");

    let v8 = E.A;
AssertType(v8, "E");
AssertType(E.A, "E.A");

    let x1 = 1;
AssertType(x1, "number");
AssertType(1, "int");

    let x2 = -123;
AssertType(x2, "number");
AssertType(-123, "int");
AssertType(123, "int");

    let x3 = 3 + 4;
AssertType(x3, "number");
AssertType(3 + 4, "number");
AssertType(3, "int");
AssertType(4, "int");

    let x4 = "abc";
AssertType(x4, "string");
AssertType("abc", "string");

    let x5 = "";
AssertType(x5, "string");
AssertType("", "string");

    let x6 = "abc" + "def";
AssertType(x6, "string");
AssertType("abc" + "def", "string");
AssertType("abc", "string");
AssertType("def", "string");

    let x7 = true;
AssertType(x7, "boolean");
AssertType(true, "boolean");

    let x8 = E.A;
AssertType(x8, "E");
AssertType(E.A, "E.A");

    const c1 = 1;
AssertType(c1, "int");
AssertType(1, "int");

    const c2 = -123;
AssertType(c2, "int");
AssertType(-123, "int");
AssertType(123, "int");

    const c3 = 3 + 4;
AssertType(c3, "number");
AssertType(3 + 4, "number");
AssertType(3, "int");
AssertType(4, "int");

    const c4 = "abc";
AssertType(c4, "string");
AssertType("abc", "string");

    const c5 = "";
AssertType(c5, "string");
AssertType("", "string");

    const c6 = "abc" + "def";
AssertType(c6, "string");
AssertType("abc" + "def", "string");
AssertType("abc", "string");
AssertType("def", "string");

    const c7 = true;
AssertType(c7, "boolean");
AssertType(true, "boolean");

    const c8 = E.A;
AssertType(c8, "E.A");
AssertType(E.A, "E.A");
}

function f2(p1: 1 = 1, p2: "abc" = "abc", p3: true = true, p4: E.A = E.A) {
    let v1: 1 = 1;
AssertType(v1, "int");
AssertType(1, "int");

    let v2: -123 = -123;
AssertType(v2, "int");
AssertType(-123, "int");
AssertType(123, "int");
AssertType(-123, "int");
AssertType(123, "int");

    let v3: "abc" = "abc";
AssertType(v3, "string");
AssertType("abc", "string");

    let v4: true = true;
AssertType(v4, "boolean");
AssertType(true, "boolean");
AssertType(true, "boolean");

    let v5: E.A = E.A;
AssertType(v5, "E.A");
AssertType(E, "any");
AssertType(E.A, "E.A");

    let x1: 1 = 1;
AssertType(x1, "int");
AssertType(1, "int");

    let x2: -123 = -123;
AssertType(x2, "int");
AssertType(-123, "int");
AssertType(123, "int");
AssertType(-123, "int");
AssertType(123, "int");

    let x3: "abc" = "abc";
AssertType(x3, "string");
AssertType("abc", "string");

    let x4: true = true;
AssertType(x4, "boolean");
AssertType(true, "boolean");
AssertType(true, "boolean");

    let x5: E.A = E.A;
AssertType(x5, "E.A");
AssertType(E, "any");
AssertType(E.A, "E.A");
}

function f3() {
    const c1 = cond ? 1 : 2;
AssertType(c1, "union");
AssertType(cond ? 1 : 2, "union");
AssertType(cond, "boolean");
AssertType(1, "int");
AssertType(2, "int");

    const c2 = cond ? 1 : "two";
AssertType(c2, "union");
AssertType(cond ? 1 : "two", "union");
AssertType(cond, "boolean");
AssertType(1, "int");
AssertType("two", "string");

    const c3 = cond ? E.A : cond ? true : 123;
AssertType(c3, "union");
AssertType(cond ? E.A : cond ? true : 123, "union");
AssertType(cond, "boolean");
AssertType(E.A, "E.A");
AssertType(cond ? true : 123, "union");
AssertType(cond, "boolean");
AssertType(true, "boolean");
AssertType(123, "int");

    const c4 = cond ? "abc" : null;
AssertType(c4, "string");
AssertType(cond ? "abc" : null, "string");
AssertType(cond, "boolean");
AssertType("abc", "string");
AssertType(null, "null");

    const c5 = cond ? 456 : undefined;
AssertType(c5, "int");
AssertType(cond ? 456 : undefined, "int");
AssertType(cond, "boolean");
AssertType(456, "int");
AssertType(undefined, "undefined");

    const c6: { kind: 123 } = { kind: 123 };
AssertType(c6, "{ kind: 123; }");
AssertType(kind, "int");
AssertType({ kind: 123 }, "{ kind: 123; }");
AssertType(kind, "int");
AssertType(123, "int");

    const c7: [1 | 2, "foo" | "bar"] = [1, "bar"];
AssertType(c7, "[union, union]");
AssertType([1, "bar"], "[1, "bar"]");
AssertType(1, "int");
AssertType("bar", "string");

    const c8 = cond ? c6 : cond ? c7 : "hello";
AssertType(c8, "union");
AssertType(cond ? c6 : cond ? c7 : "hello", "union");
AssertType(cond, "boolean");
AssertType(c6, "{ kind: 123; }");
AssertType(cond ? c7 : "hello", "union");
AssertType(cond, "boolean");
AssertType(c7, "[union, union]");
AssertType("hello", "string");

    let x1 = c1;
AssertType(x1, "number");
AssertType(c1, "union");

    let x2 = c2;
AssertType(x2, "union");
AssertType(c2, "union");

    let x3 = c3;
AssertType(x3, "union");
AssertType(c3, "union");

    let x4 = c4;
AssertType(x4, "string");
AssertType(c4, "string");

    let x5 = c5;
AssertType(x5, "number");
AssertType(c5, "int");

    let x6 = c6;
AssertType(x6, "{ kind: 123; }");
AssertType(c6, "{ kind: 123; }");

    let x7 = c7;
AssertType(x7, "[union, union]");
AssertType(c7, "[union, union]");

    let x8 = c8;
AssertType(x8, "union");
AssertType(c8, "union");
}

class C1 {
    x1 = 1;
    x2 = -123;
    x3 = 3 + 4;
    x4 = "abc";
    x5 = "";
    x6 = "abc" + "def";
    x7 = true;
    x8 = E.A;
    readonly c1 = 1;
    readonly c2 = -123;
    readonly c3 = 3 + 4;
    readonly c4 = "abc";
    readonly c5 = "";
    readonly c6 = "abc" + "def";
    readonly c7 = true;
    readonly c8 = E.A;
}

function f4() {
    const c1 = { a: 1, b: "foo" };
AssertType(c1, "{ a: number; b: string; }");
AssertType({ a: 1, b: "foo" }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "string");
AssertType("foo", "string");

    const c2: { a : 0 | 1, b: "foo" | "bar" } = { a: 1, b: "foo" };
AssertType(c2, "{ a: union; b: union; }");
AssertType(a, "union");
AssertType(b, "union");
AssertType({ a: 1, b: "foo" }, "{ a: 1; b: "foo"; }");
AssertType(a, "int");
AssertType(1, "int");
AssertType(b, "string");
AssertType("foo", "string");

    let x1 = { a: 1, b: "foo" };
AssertType(x1, "{ a: number; b: string; }");
AssertType({ a: 1, b: "foo" }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "string");
AssertType("foo", "string");

    let x2: { a : 0 | 1, b: "foo" | "bar" } = { a: 1, b: "foo" };
AssertType(x2, "{ a: union; b: union; }");
AssertType(a, "union");
AssertType(b, "union");
AssertType({ a: 1, b: "foo" }, "{ a: 1; b: "foo"; }");
AssertType(a, "int");
AssertType(1, "int");
AssertType(b, "string");
AssertType("foo", "string");
}

function f5() {
    const c1 = [1, "foo"];
AssertType(c1, "(union)[]");
AssertType([1, "foo"], "(union)[]");
AssertType(1, "int");
AssertType("foo", "string");

    const c2: (1 | "foo")[] = [1, "foo"];
AssertType(c2, "(union)[]");
AssertType([1, "foo"], "(union)[]");
AssertType(1, "int");
AssertType("foo", "string");

    const c3: [1, "foo"] = [1, "foo"];
AssertType(c3, "[1, "foo"]");
AssertType([1, "foo"], "[1, "foo"]");
AssertType(1, "int");
AssertType("foo", "string");

    let x1 = [1, "foo"];
AssertType(x1, "(union)[]");
AssertType([1, "foo"], "(union)[]");
AssertType(1, "int");
AssertType("foo", "string");

    let x2: (1 | "foo")[] = [1, "foo"];
AssertType(x2, "(union)[]");
AssertType([1, "foo"], "(union)[]");
AssertType(1, "int");
AssertType("foo", "string");

    let x3: [1, "foo"] = [1, "foo"];
AssertType(x3, "[1, "foo"]");
AssertType([1, "foo"], "[1, "foo"]");
AssertType(1, "int");
AssertType("foo", "string");
}

function f6() {
    const { c1 = true, c2 = 0, c3 = "foo" } = { c1: false, c2: 1, c3: "bar" };
AssertType(c1, "boolean");
AssertType(true, "boolean");
AssertType(c2, "union");
AssertType(0, "int");
AssertType(c3, "union");
AssertType("foo", "string");
AssertType({ c1: false, c2: 1, c3: "bar" }, "{ c1?: false; c2?: 1; c3?: "bar"; }");
AssertType(c1, "boolean");
AssertType(false, "boolean");
AssertType(c2, "int");
AssertType(1, "int");
AssertType(c3, "string");
AssertType("bar", "string");

    let { x1 = true, x2 = 0, x3 = "foo" } = { x1: false, x2: 1, x3: "bar" };
AssertType(x1, "boolean");
AssertType(true, "boolean");
AssertType(x2, "number");
AssertType(0, "int");
AssertType(x3, "string");
AssertType("foo", "string");
AssertType({ x1: false, x2: 1, x3: "bar" }, "{ x1?: false; x2?: number; x3?: string; }");
AssertType(x1, "boolean");
AssertType(false, "boolean");
AssertType(x2, "number");
AssertType(1, "int");
AssertType(x3, "string");
AssertType("bar", "string");
}

function f10() {
AssertType("hello", "string");
    return "hello";
}

function f11() {
AssertType(cond ? 1 : "two", "union");
AssertType(cond, "boolean");
AssertType(1, "int");
AssertType("two", "string");
    return cond ? 1 : "two";
}

function f12() {
    if (cond) {
AssertType(cond, "boolean");

AssertType(1, "int");
        return 1;
    }
    else {
AssertType("two", "string");
        return "two";
    }
}

class C2 {
    foo() {
AssertType(0, "int");
        return 0;
    }
    bar() {
AssertType(cond ? 0 : 1, "union");
AssertType(cond, "boolean");
AssertType(0, "int");
AssertType(1, "int");
        return cond ? 0 : 1;
    }
}

function f20() {
    const f1 = () => 0;
AssertType(f1, "() => number");
AssertType(() => 0, "() => number");
AssertType(0, "int");

    const f2 = () => "hello";
AssertType(f2, "() => string");
AssertType(() => "hello", "() => string");
AssertType("hello", "string");

    const f3 = () => true;
AssertType(f3, "() => boolean");
AssertType(() => true, "() => boolean");
AssertType(true, "boolean");

    const f4 = () => E.C;
AssertType(f4, "() => E");
AssertType(() => E.C, "() => E");
AssertType(E.C, "E.C");

    const f5 = (): "foo" => "foo";
AssertType(f5, "() => "foo"");
AssertType((): "foo" => "foo", "() => "foo"");
AssertType("foo", "string");

    const f6: () => "foo" | "bar" = () => "bar";
AssertType(f6, "() => union");
AssertType(() => "bar", "() => "bar"");
AssertType("bar", "string");

    const f7: (() => "foo") | (() => "bar") = () => "bar";
AssertType(f7, "union");
AssertType(() => "bar", "() => "bar"");
AssertType("bar", "string");
}

declare function g1<T>(x: T): T;
declare function g2<T>(x: T, y: T): T;
declare function g3<T, U>(x: T, y: U): T | U;
declare function g4<T>(x: T): T[];
declare function g5<T extends number>(x: T, y: T): T[];
declare function g6<T>(x: T[]): T;
declare function g7<T>(x: T[]): T[];
declare function g8<T>(x: T, f: (p: T) => T): T;

const a: (1 | 2)[] = [1, 2];
AssertType(a, "(union)[]");
AssertType([1, 2], "(union)[]");
AssertType(1, "int");
AssertType(2, "int");

const x1 = g1(1);  // Type 1
AssertType(x1, "int");
AssertType(g1(1), "int");
AssertType(g1, "<T>(T) => T");
AssertType(1, "int");

const x2 = g2(1, 1);  // Type 1
AssertType(x2, "int");
AssertType(g2(1, 1), "int");
AssertType(g2, "<T>(T, T) => T");
AssertType(1, "int");
AssertType(1, "int");

const x3 = g2(1, 2);  // Type 1 | 2
AssertType(x3, "union");
AssertType(g2(1, 2), "union");
AssertType(g2, "<T>(T, T) => T");
AssertType(1, "int");
AssertType(2, "int");

const x4 = g3(1, "two");  // Type 1 | "two"
AssertType(x4, "union");
AssertType(g3(1, "two"), "union");
AssertType(g3, "<T, U>(T, U) => union");
AssertType(1, "int");
AssertType("two", "string");

const x5 = g4(1);  // Type number[]
AssertType(x5, "number[]");
AssertType(g4(1), "number[]");
AssertType(g4, "<T>(T) => T[]");
AssertType(1, "int");

const x6 = g5(1, 2);  // Type (1 | 2)[]
AssertType(x6, "(union)[]");
AssertType(g5(1, 2), "(union)[]");
AssertType(g5, "<T extends number>(T, T) => T[]");
AssertType(1, "int");
AssertType(2, "int");

const x7 = g6([1, 2]);  // Type number
AssertType(x7, "number");
AssertType(g6([1, 2]), "number");
AssertType(g6, "<T>(T[]) => T");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

const x8 = g6(a);  // Type 1 | 2
AssertType(x8, "union");
AssertType(g6(a), "union");
AssertType(g6, "<T>(T[]) => T");
AssertType(a, "(union)[]");

const x9 = g7(a);  // Type (1 | 2)[]
AssertType(x9, "(union)[]");
AssertType(g7(a), "(union)[]");
AssertType(g7, "<T>(T[]) => T[]");
AssertType(a, "(union)[]");

const x10 = g8(1, x => x);  // Type number
AssertType(x10, "number");
AssertType(g8(1, x => x), "number");
AssertType(g8, "<T>(T, (T) => T) => T");
AssertType(1, "int");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");

const x11 = g8(1, x => x + 1);  // Type number
AssertType(x11, "number");
AssertType(g8(1, x => x + 1), "number");
AssertType(g8, "<T>(T, (T) => T) => T");
AssertType(1, "int");
AssertType(x => x + 1, "(number) => number");
AssertType(x, "number");
AssertType(x + 1, "number");
AssertType(x, "number");
AssertType(1, "int");

function makeArray<T>(x: T): T[] {
AssertType([x], "T[]");
AssertType(x, "T");
    return [x];
}

function append<T>(a: T[], x: T): T[] {
    let result = a.slice();
AssertType(result, "T[]");
AssertType(a.slice(), "T[]");
AssertType(a.slice, "(?number, ?number) => T[]");

    result.push(x);
AssertType(result.push(x), "number");
AssertType(result.push, "(...T[]) => number");
AssertType(x, "T");

AssertType(result, "T[]");
    return result;
}

type Bit = 0 | 1;

let aa = makeArray<Bit>(0);
AssertType(aa, "Bit[]");
AssertType(makeArray<Bit>(0), "Bit[]");
AssertType(makeArray, "<T>(T) => T[]");
AssertType(0, "int");

aa = append(aa, 1);
AssertType(aa = append(aa, 1), "Bit[]");
AssertType(aa, "Bit[]");
AssertType(append(aa, 1), "Bit[]");
AssertType(append, "<T>(T[], T) => T[]");
AssertType(aa, "Bit[]");
AssertType(1, "int");


