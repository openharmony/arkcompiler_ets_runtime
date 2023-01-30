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

// === tests/cases/compiler/controlFlowArrays.ts ===
declare function AssertType(value:any, type:string):void;
declare function cond(): boolean;

function f1() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x[0] = 5;
AssertType(x[0] = 5, "int");
AssertType(x[0], "any");
AssertType(x, "any[]");
AssertType(0, "int");
AssertType(5, "int");

    x[1] = "hello";
AssertType(x[1] = "hello", "string");
AssertType(x[1], "any");
AssertType(x, "any[]");
AssertType(1, "int");
AssertType("hello", "string");

    x[2] = true;
AssertType(x[2] = true, "boolean");
AssertType(x[2], "any");
AssertType(x, "any[]");
AssertType(2, "int");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f2() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

    x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");

    x.push(true);
AssertType(x.push(true), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f3() {
    let x;
AssertType(x, "any");

    x = [];
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");

    x.push(5, "hello");
AssertType(x.push(5, "hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");
AssertType("hello", "string");

AssertType(x, "(union)[]");
    return x;  // (string | number)[]
}

function f4() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    if (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");
    }
    else {
        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
    }
AssertType(x, "(union)[]");
    return x;  // (string | number)[]
}

function f5() {
    let x;
AssertType(x, "any");

    if (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        x = [];
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");

        x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");
    }
    else {
        x = [];
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");

        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
    }
AssertType(x, "(union)[]");
    return x;  // (string | number)[]
}

function f6() {
    let x;
AssertType(x, "any");

    if (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        x = 5;
AssertType(x = 5, "int");
AssertType(x, "any");
AssertType(5, "int");
    }
    else {
        x = [];
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");

        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
    }
AssertType(x, "union");
    return x;  // number | string[]
}

function f7() {
    let x = null;
AssertType(x, "any");
AssertType(null, "null");

    if (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        x = [];
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");

        while (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

            x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
        }
    }
AssertType(x, "union");
    return x;  // string[] | null
}

function f8() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

    if (cond()) 
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");
AssertType(x, "number[]");
return x;  // number[]

    x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");

    if (cond()) 
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");
AssertType(x, "(union)[]");
return x;  // (string | number)[]

    x.push(true);
AssertType(x.push(true), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f9() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    if (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

AssertType(x, "number[]");
        return x;  // number[]
    }
    else {
        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");

AssertType(x, "string[]");
        return x;  // string[]
    }
}

function f10() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    if (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        x.push(true);
AssertType(x.push(true), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(true, "boolean");

        x;  // boolean[]
AssertType(x, "boolean[]");
    }
    else {
        x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

        x;  // number[]
AssertType(x, "number[]");

        while (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

            x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
        }
        x;  // (string | number)[]
AssertType(x, "(union)[]");
    }
    x.push(99);
AssertType(x.push(99), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(99, "int");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f11() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    if (x.length === 0) {  // x.length ok on implicit any[]
AssertType(x.length === 0, "boolean");
AssertType(x.length, "number");
AssertType(0, "int");

        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
    }
AssertType(x, "string[]");
    return x;
}

function f12() {
    let x;
AssertType(x, "any");

    x = [];
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");

    if (x.length === 0) {  // x.length ok on implicit any[]
AssertType(x.length === 0, "boolean");
AssertType(x.length, "number");
AssertType(0, "int");

        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
    }
AssertType(x, "string[]");
    return x;
}

function f13() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

    x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");

    x.push(true);
AssertType(x.push(true), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f14() {
    const x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

    x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");

    x.push(true);
AssertType(x.push(true), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f15() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    while (cond()) {
AssertType(cond(), "boolean");
AssertType(cond, "() => boolean");

        while (cond()) {
AssertType(cond(), "boolean");

AssertType(cond, "() => boolean");
}

        x.push("hello");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
    }
AssertType(x, "string[]");
    return x;  // string[]
}

function f16() {
    let x;
AssertType(x, "any");

    let y;
AssertType(y, "any");

    (x = [], x).push(5);
AssertType((x = [], x).push(5), "number");
AssertType((x = [], x).push, "(...any[]) => number");
AssertType((x = [], x), "any[]");
AssertType(x = [], x, "any[]");
AssertType(x = [], "never[]");
AssertType(x, "any");
AssertType([], "never[]");
AssertType(x, "any[]");
AssertType(5, "int");

    (x.push("hello"), x).push(true);
AssertType((x.push("hello"), x).push(true), "number");
AssertType((x.push("hello"), x).push, "(...any[]) => number");
AssertType((x.push("hello"), x), "any[]");
AssertType(x.push("hello"), x, "any[]");
AssertType(x.push("hello"), "number");
AssertType(x.push, "(...any[]) => number");
AssertType("hello", "string");
AssertType(x, "any[]");
AssertType(true, "boolean");

    ((x))[3] = { a: 1 };
AssertType(((x))[3] = { a: 1 }, "{ a: number; }");
AssertType(((x))[3], "any");
AssertType(((x)), "any[]");
AssertType((x), "any[]");
AssertType(x, "any[]");
AssertType(3, "int");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean | { a: number })[]
}

function f17() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x.unshift(5);
AssertType(x.unshift(5), "number");
AssertType(x.unshift, "(...any[]) => number");
AssertType(5, "int");

    x.unshift("hello");
AssertType(x.unshift("hello"), "number");
AssertType(x.unshift, "(...any[]) => number");
AssertType("hello", "string");

    x.unshift(true);
AssertType(x.unshift(true), "number");
AssertType(x.unshift, "(...any[]) => number");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

function f18() {
    let x = [];
AssertType(x, "any[]");
AssertType([], "never[]");

    x.push(5);
AssertType(x.push(5), "number");
AssertType(x.push, "(...any[]) => number");
AssertType(5, "int");

    x.unshift("hello");
AssertType(x.unshift("hello"), "number");
AssertType(x.unshift, "(...any[]) => number");
AssertType("hello", "string");

    x[2] = true;
AssertType(x[2] = true, "boolean");
AssertType(x[2], "any");
AssertType(x, "any[]");
AssertType(2, "int");
AssertType(true, "boolean");

AssertType(x, "(union)[]");
    return x;  // (string | number | boolean)[]
}

// Repro from #39470

declare function foo(arg: { val: number }[]): void;

let arr = []
AssertType(arr, "any[]");
AssertType([], "never[]");

arr.push({ val: 1, bar: 2 });
AssertType(arr.push({ val: 1, bar: 2 }), "number");
AssertType(arr.push, "(...any[]) => number");
AssertType({ val: 1, bar: 2 }, "{ val: number; bar: number; }");
AssertType(val, "number");
AssertType(1, "int");
AssertType(bar, "number");
AssertType(2, "int");

foo(arr);
AssertType(foo(arr), "void");
AssertType(foo, "({ val: number; }[]) => void");
AssertType(arr, "{ val: number; bar: number; }[]");


