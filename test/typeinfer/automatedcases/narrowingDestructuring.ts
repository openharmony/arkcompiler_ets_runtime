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

// === tests/cases/compiler/narrowingDestructuring.ts ===
declare function AssertType(value:any, type:string):void;
type X = { kind: "a", a: string } | { kind: "b", b: string }

function func<T extends X>(value: T) {
    if (value.kind === "a") {
AssertType(value.kind === "a", "boolean");
AssertType(value.kind, "union");
AssertType("a", "string");

        value.a;
AssertType(value.a, "string");

        const { a } = value;
AssertType(a, "string");
AssertType(value, "{ kind: "a"; a: string; }");

    } else {
        value.b;
AssertType(value.b, "string");

        const { b } = value;
AssertType(b, "string");
AssertType(value, "{ kind: "b"; b: string; }");
    }
}

type Z = { kind: "f", f: { a: number, b: string, c: number } }
    | { kind: "g", g: { a: string, b: number, c: string }};

function func2<T extends Z>(value: T) {
    if (value.kind === "f") {
AssertType(value.kind === "f", "boolean");
AssertType(value.kind, "union");
AssertType("f", "string");

        const { f: f1 } = value;
AssertType(f, "any");
AssertType(f1, "{ a: number; b: string; c: number; }");
AssertType(value, "{ kind: "f"; f: { a: number; b: string; c: number; }; }");

        const { f: { a, ...spread } } = value;
AssertType(f, "any");
AssertType(a, "number");
AssertType(spread, "{ b: string; c: number; }");
AssertType(value, "{ kind: "f"; f: { a: number; b: string; c: number; }; }");

        value.f;
AssertType(value.f, "{ a: number; b: string; c: number; }");

    } else {
        const { g: { c, ...spread } } = value;
AssertType(g, "any");
AssertType(c, "string");
AssertType(spread, "{ a: string; b: number; }");
AssertType(value, "{ kind: "g"; g: { a: string; b: number; c: string; }; }");

        value.g;
AssertType(value.g, "{ a: string; b: number; c: string; }");
    }
}

function func3<T extends { kind: "a", a: string } | { kind: "b", b: number }>(t: T) {
    if (t.kind === "a") {
AssertType(t.kind === "a", "boolean");
AssertType(t.kind, "union");
AssertType("a", "string");

        const { kind, ...r1 } = t;
AssertType(kind, "string");
AssertType(r1, "Omit<T, "kind">");
AssertType(t, "{ kind: "a"; a: string; }");

        const r2 = (({ kind, ...rest }) => rest)(t);
AssertType(r2, "{ a: string; }");
AssertType((({ kind, ...rest }) => rest)(t), "{ a: string; }");
AssertType((({ kind, ...rest }) => rest), "({ kind: "a"; a: string; }) => { a: string; }");
AssertType(({ kind, ...rest }) => rest, "({ kind: "a"; a: string; }) => { a: string; }");
AssertType(kind, "string");
AssertType(rest, "{ a: string; }");
AssertType(rest, "{ a: string; }");
AssertType(t, "{ kind: "a"; a: string; }");
    }
}

function farr<T extends [number, string, string] | [string, number, number]>(x: T) {
    const [head, ...tail] = x;
AssertType(head, "union");
AssertType(tail, "(union)[]");
AssertType(x, "union");

    if (x[0] === 'number') {
AssertType(x[0] === 'number', "boolean");
AssertType(x[0], "union");
AssertType(x, "union");
AssertType(0, "int");
AssertType('number', "string");

        const [head, ...tail] = x;
AssertType(head, "string");
AssertType(tail, "(union)[]");
AssertType(x, "union");
    }
}

