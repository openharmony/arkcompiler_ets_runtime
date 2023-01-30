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

// === tests/cases/conformance/expressions/functionCalls/newWithSpreadES5.ts ===
declare function AssertType(value:any, type:string):void;
function f(x: number, y: number, ...z: string[]) {
}

function f2(...x: string[]) {}

interface A {
    f: {
        new (x: number, y: number, ...z: string[]);
    }
}

class B {
    constructor(x: number, y: number, ...z: string[]) {}
}

interface C {
    "a-b": typeof B;
}

interface D {
    1: typeof B;
}

let a: string[];
AssertType(a, "string[]");

let b: A;
AssertType(b, "A");

let c: C;
AssertType(c, "C");

let d: A[];
AssertType(d, "A[]");

let e: { [key: string]: A };
AssertType(e, "{ [string]: A; }");
AssertType(key, "string");

let g: C[];
AssertType(g, "C[]");

let h: { [key: string]: C };
AssertType(h, "{ [string]: C; }");
AssertType(key, "string");

let i: C[][];
AssertType(i, "C[][]");

// Basic expression
new f(1, 2, "string");
AssertType(new f(1, 2, "string"), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new f(1, 2, ...a);
AssertType(new f(1, 2, ...a), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new f(1, 2, ...a, "string");
AssertType(new f(1, 2, ...a, "string"), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Multiple spreads arguments
new f2(...a, ...a);
AssertType(new f2(...a, ...a), "any");
AssertType(f2, "(...string[]) => void");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType(...a, "string");
AssertType(a, "string[]");

new f(1 ,2, ...a, ...a);
AssertType(new f(1 ,2, ...a, ...a), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType(...a, "string");
AssertType(a, "string[]");

// Call expression
new f(1, 2, "string")();
AssertType(new f(1, 2, "string")(), "any");
AssertType(new f(1, 2, "string"), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new f(1, 2, ...a)();
AssertType(new f(1, 2, ...a)(), "any");
AssertType(new f(1, 2, ...a), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new f(1, 2, ...a, "string")();
AssertType(new f(1, 2, ...a, "string")(), "any");
AssertType(new f(1, 2, ...a, "string"), "any");
AssertType(f, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Property access expression
new b.f(1, 2, "string");
AssertType(new b.f(1, 2, "string"), "any");
AssertType(b.f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new b.f(1, 2, ...a);
AssertType(new b.f(1, 2, ...a), "any");
AssertType(b.f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new b.f(1, 2, ...a, "string"); 
AssertType(new b.f(1, 2, ...a, "string"), "any");
AssertType(b.f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Parenthesised expression
new (b.f)(1, 2, "string");
AssertType(new (b.f)(1, 2, "string"), "any");
AssertType((b.f), "new (number, number, ...string[]) => any");
AssertType(b.f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new (b.f)(1, 2, ...a);
AssertType(new (b.f)(1, 2, ...a), "any");
AssertType((b.f), "new (number, number, ...string[]) => any");
AssertType(b.f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new (b.f)(1, 2, ...a, "string"); 
AssertType(new (b.f)(1, 2, ...a, "string"), "any");
AssertType((b.f), "new (number, number, ...string[]) => any");
AssertType(b.f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Element access expression
new d[1].f(1, 2, "string");
AssertType(new d[1].f(1, 2, "string"), "any");
AssertType(d[1].f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new d[1].f(1, 2, ...a);
AssertType(new d[1].f(1, 2, ...a), "any");
AssertType(d[1].f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new d[1].f(1, 2, ...a, "string");
AssertType(new d[1].f(1, 2, ...a, "string"), "any");
AssertType(d[1].f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Element access expression with a punctuated key
new e["a-b"].f(1, 2, "string");
AssertType(new e["a-b"].f(1, 2, "string"), "any");
AssertType(e["a-b"].f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new e["a-b"].f(1, 2, ...a);
AssertType(new e["a-b"].f(1, 2, ...a), "any");
AssertType(e["a-b"].f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new e["a-b"].f(1, 2, ...a, "string");
AssertType(new e["a-b"].f(1, 2, ...a, "string"), "any");
AssertType(e["a-b"].f, "new (number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Basic expression
new B(1, 2, "string");
AssertType(new B(1, 2, "string"), "B");
AssertType(B, "typeof B");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new B(1, 2, ...a);
AssertType(new B(1, 2, ...a), "B");
AssertType(B, "typeof B");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new B(1, 2, ...a, "string");
AssertType(new B(1, 2, ...a, "string"), "B");
AssertType(B, "typeof B");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Property access expression
new c["a-b"](1, 2, "string");
AssertType(new c["a-b"](1, 2, "string"), "B");
AssertType(c["a-b"], "typeof B");
AssertType(c, "C");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new c["a-b"](1, 2, ...a);
AssertType(new c["a-b"](1, 2, ...a), "B");
AssertType(c["a-b"], "typeof B");
AssertType(c, "C");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new c["a-b"](1, 2, ...a, "string");
AssertType(new c["a-b"](1, 2, ...a, "string"), "B");
AssertType(c["a-b"], "typeof B");
AssertType(c, "C");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Parenthesised expression
new (c["a-b"])(1, 2, "string");
AssertType(new (c["a-b"])(1, 2, "string"), "B");
AssertType((c["a-b"]), "typeof B");
AssertType(c["a-b"], "typeof B");
AssertType(c, "C");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new (c["a-b"])(1, 2, ...a);
AssertType(new (c["a-b"])(1, 2, ...a), "B");
AssertType((c["a-b"]), "typeof B");
AssertType(c["a-b"], "typeof B");
AssertType(c, "C");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new (c["a-b"])(1, 2, ...a, "string");
AssertType(new (c["a-b"])(1, 2, ...a, "string"), "B");
AssertType((c["a-b"]), "typeof B");
AssertType(c["a-b"], "typeof B");
AssertType(c, "C");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Element access expression
new g[1]["a-b"](1, 2, "string");
AssertType(new g[1]["a-b"](1, 2, "string"), "B");
AssertType(g[1]["a-b"], "typeof B");
AssertType(g[1], "C");
AssertType(g, "C[]");
AssertType(1, "int");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new g[1]["a-b"](1, 2, ...a);
AssertType(new g[1]["a-b"](1, 2, ...a), "B");
AssertType(g[1]["a-b"], "typeof B");
AssertType(g[1], "C");
AssertType(g, "C[]");
AssertType(1, "int");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new g[1]["a-b"](1, 2, ...a, "string");
AssertType(new g[1]["a-b"](1, 2, ...a, "string"), "B");
AssertType(g[1]["a-b"], "typeof B");
AssertType(g[1], "C");
AssertType(g, "C[]");
AssertType(1, "int");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Element access expression with a punctuated key
new h["a-b"]["a-b"](1, 2, "string");
AssertType(new h["a-b"]["a-b"](1, 2, "string"), "B");
AssertType(h["a-b"]["a-b"], "typeof B");
AssertType(h["a-b"], "C");
AssertType(h, "{ [string]: C; }");
AssertType("a-b", "string");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new h["a-b"]["a-b"](1, 2, ...a);
AssertType(new h["a-b"]["a-b"](1, 2, ...a), "B");
AssertType(h["a-b"]["a-b"], "typeof B");
AssertType(h["a-b"], "C");
AssertType(h, "{ [string]: C; }");
AssertType("a-b", "string");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new h["a-b"]["a-b"](1, 2, ...a, "string");
AssertType(new h["a-b"]["a-b"](1, 2, ...a, "string"), "B");
AssertType(h["a-b"]["a-b"], "typeof B");
AssertType(h["a-b"], "C");
AssertType(h, "{ [string]: C; }");
AssertType("a-b", "string");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");

// Element access expression with a number
new i["a-b"][1](1, 2, "string");
AssertType(new i["a-b"][1](1, 2, "string"), "error");
AssertType(i["a-b"][1], "error");
AssertType(i["a-b"], "error");
AssertType(i, "C[][]");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(1, "int");
AssertType(2, "int");
AssertType("string", "string");

new i["a-b"][1](1, 2, ...a);
AssertType(new i["a-b"][1](1, 2, ...a), "error");
AssertType(i["a-b"][1], "error");
AssertType(i["a-b"], "error");
AssertType(i, "C[][]");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

new i["a-b"][1](1, 2, ...a, "string");
AssertType(new i["a-b"][1](1, 2, ...a, "string"), "error");
AssertType(i["a-b"][1], "error");
AssertType(i["a-b"], "error");
AssertType(i, "C[][]");
AssertType("a-b", "string");
AssertType(1, "int");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("string", "string");


