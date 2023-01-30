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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithOneOperandIsAny.ts ===
declare function AssertType(value:any, type:string):void;
let x: any;
AssertType(x, "any");

enum E { a, b, c }

function foo<T>(t: T) {
    let foo_r1 = t < x;
AssertType(foo_r1, "boolean");
AssertType(t < x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r2 = t > x;
AssertType(foo_r2, "boolean");
AssertType(t > x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r3 = t <= x;
AssertType(foo_r3, "boolean");
AssertType(t <= x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r4 = t >= x;
AssertType(foo_r4, "boolean");
AssertType(t >= x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r5 = t == x;
AssertType(foo_r5, "boolean");
AssertType(t == x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r6 = t != x;
AssertType(foo_r6, "boolean");
AssertType(t != x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r7 = t === x;
AssertType(foo_r7, "boolean");
AssertType(t === x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r8 = t !== x;
AssertType(foo_r8, "boolean");
AssertType(t !== x, "boolean");
AssertType(t, "T");
AssertType(x, "any");

    let foo_r1 = x < t;
AssertType(foo_r1, "boolean");
AssertType(x < t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r2 = x > t;
AssertType(foo_r2, "boolean");
AssertType(x > t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r3 = x <= t;
AssertType(foo_r3, "boolean");
AssertType(x <= t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r4 = x >= t;
AssertType(foo_r4, "boolean");
AssertType(x >= t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r5 = x == t;
AssertType(foo_r5, "boolean");
AssertType(x == t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r6 = x != t;
AssertType(foo_r6, "boolean");
AssertType(x != t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r7 = x === t;
AssertType(foo_r7, "boolean");
AssertType(x === t, "boolean");
AssertType(x, "any");
AssertType(t, "T");

    let foo_r8 = x !== t;
AssertType(foo_r8, "boolean");
AssertType(x !== t, "boolean");
AssertType(x, "any");
AssertType(t, "T");
}

let a: boolean;
AssertType(a, "boolean");

let b: number;
AssertType(b, "number");

let c: string;
AssertType(c, "string");

let d: void;
AssertType(d, "void");

let e: E;
AssertType(e, "E");

let f: {};
AssertType(f, "{}");

let g: string[];
AssertType(g, "string[]");

// operator <
let r1a1 = x < a;
AssertType(r1a1, "boolean");
AssertType(x < a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r1a2 = x < b;
AssertType(r1a2, "boolean");
AssertType(x < b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r1a3 = x < c;
AssertType(r1a3, "boolean");
AssertType(x < c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r1a4 = x < d;
AssertType(r1a4, "boolean");
AssertType(x < d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r1a5 = x < e;
AssertType(r1a5, "boolean");
AssertType(x < e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r1a6 = x < f;
AssertType(r1a6, "boolean");
AssertType(x < f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r1a7 = x < g;
AssertType(r1a7, "boolean");
AssertType(x < g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r1b1 = a < x;
AssertType(r1b1, "boolean");
AssertType(a < x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r1b2 = b < x;
AssertType(r1b2, "boolean");
AssertType(b < x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r1b3 = c < x;
AssertType(r1b3, "boolean");
AssertType(c < x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r1b4 = d < x;
AssertType(r1b4, "boolean");
AssertType(d < x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r1b5 = e < x;
AssertType(r1b5, "boolean");
AssertType(e < x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r1b6 = f < x;
AssertType(r1b6, "boolean");
AssertType(f < x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r1b7 = g < x;
AssertType(r1b7, "boolean");
AssertType(g < x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator >
let r2a1 = x > a;
AssertType(r2a1, "boolean");
AssertType(x > a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r2a2 = x > b;
AssertType(r2a2, "boolean");
AssertType(x > b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r2a3 = x > c;
AssertType(r2a3, "boolean");
AssertType(x > c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r2a4 = x > d;
AssertType(r2a4, "boolean");
AssertType(x > d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r2a5 = x > e;
AssertType(r2a5, "boolean");
AssertType(x > e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r2a6 = x > f;
AssertType(r2a6, "boolean");
AssertType(x > f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r2a7 = x > g;
AssertType(r2a7, "boolean");
AssertType(x > g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r2b1 = a > x;
AssertType(r2b1, "boolean");
AssertType(a > x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r2b2 = b > x;
AssertType(r2b2, "boolean");
AssertType(b > x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r2b3 = c > x;
AssertType(r2b3, "boolean");
AssertType(c > x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r2b4 = d > x;
AssertType(r2b4, "boolean");
AssertType(d > x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r2b5 = e > x;
AssertType(r2b5, "boolean");
AssertType(e > x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r2b6 = f > x;
AssertType(r2b6, "boolean");
AssertType(f > x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r2b7 = g > x;
AssertType(r2b7, "boolean");
AssertType(g > x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator <=
let r3a1 = x <= a;
AssertType(r3a1, "boolean");
AssertType(x <= a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r3a2 = x <= b;
AssertType(r3a2, "boolean");
AssertType(x <= b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r3a3 = x <= c;
AssertType(r3a3, "boolean");
AssertType(x <= c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r3a4 = x <= d;
AssertType(r3a4, "boolean");
AssertType(x <= d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r3a5 = x <= e;
AssertType(r3a5, "boolean");
AssertType(x <= e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r3a6 = x <= f;
AssertType(r3a6, "boolean");
AssertType(x <= f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r3a7 = x <= g;
AssertType(r3a7, "boolean");
AssertType(x <= g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r3b1 = a <= x;
AssertType(r3b1, "boolean");
AssertType(a <= x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r3b2 = b <= x;
AssertType(r3b2, "boolean");
AssertType(b <= x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r3b3 = c <= x;
AssertType(r3b3, "boolean");
AssertType(c <= x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r3b4 = d <= x;
AssertType(r3b4, "boolean");
AssertType(d <= x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r3b5 = e <= x;
AssertType(r3b5, "boolean");
AssertType(e <= x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r3b6 = f <= x;
AssertType(r3b6, "boolean");
AssertType(f <= x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r3b7 = g <= x;
AssertType(r3b7, "boolean");
AssertType(g <= x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator >=
let r4a1 = x >= a;
AssertType(r4a1, "boolean");
AssertType(x >= a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r4a2 = x >= b;
AssertType(r4a2, "boolean");
AssertType(x >= b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r4a3 = x >= c;
AssertType(r4a3, "boolean");
AssertType(x >= c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r4a4 = x >= d;
AssertType(r4a4, "boolean");
AssertType(x >= d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r4a5 = x >= e;
AssertType(r4a5, "boolean");
AssertType(x >= e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r4a6 = x >= f;
AssertType(r4a6, "boolean");
AssertType(x >= f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r4a7 = x >= g;
AssertType(r4a7, "boolean");
AssertType(x >= g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r4b1 = a >= x;
AssertType(r4b1, "boolean");
AssertType(a >= x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r4b2 = b >= x;
AssertType(r4b2, "boolean");
AssertType(b >= x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r4b3 = c >= x;
AssertType(r4b3, "boolean");
AssertType(c >= x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r4b4 = d >= x;
AssertType(r4b4, "boolean");
AssertType(d >= x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r4b5 = e >= x;
AssertType(r4b5, "boolean");
AssertType(e >= x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r4b6 = f >= x;
AssertType(r4b6, "boolean");
AssertType(f >= x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r4b7 = g >= x;
AssertType(r4b7, "boolean");
AssertType(g >= x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator ==
let r5a1 = x == a;
AssertType(r5a1, "boolean");
AssertType(x == a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r5a2 = x == b;
AssertType(r5a2, "boolean");
AssertType(x == b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r5a3 = x == c;
AssertType(r5a3, "boolean");
AssertType(x == c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r5a4 = x == d;
AssertType(r5a4, "boolean");
AssertType(x == d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r5a5 = x == e;
AssertType(r5a5, "boolean");
AssertType(x == e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r5a6 = x == f;
AssertType(r5a6, "boolean");
AssertType(x == f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r5a7 = x == g;
AssertType(r5a7, "boolean");
AssertType(x == g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r5b1 = a == x;
AssertType(r5b1, "boolean");
AssertType(a == x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r5b2 = b == x;
AssertType(r5b2, "boolean");
AssertType(b == x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r5b3 = c == x;
AssertType(r5b3, "boolean");
AssertType(c == x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r5b4 = d == x;
AssertType(r5b4, "boolean");
AssertType(d == x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r5b5 = e == x;
AssertType(r5b5, "boolean");
AssertType(e == x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r5b6 = f == x;
AssertType(r5b6, "boolean");
AssertType(f == x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r5b7 = g == x;
AssertType(r5b7, "boolean");
AssertType(g == x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator !=
let r6a1 = x != a;
AssertType(r6a1, "boolean");
AssertType(x != a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r6a2 = x != b;
AssertType(r6a2, "boolean");
AssertType(x != b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r6a3 = x != c;
AssertType(r6a3, "boolean");
AssertType(x != c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r6a4 = x != d;
AssertType(r6a4, "boolean");
AssertType(x != d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r6a5 = x != e;
AssertType(r6a5, "boolean");
AssertType(x != e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r6a6 = x != f;
AssertType(r6a6, "boolean");
AssertType(x != f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r6a7 = x != g;
AssertType(r6a7, "boolean");
AssertType(x != g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r6b1 = a != x;
AssertType(r6b1, "boolean");
AssertType(a != x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r6b2 = b != x;
AssertType(r6b2, "boolean");
AssertType(b != x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r6b3 = c != x;
AssertType(r6b3, "boolean");
AssertType(c != x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r6b4 = d != x;
AssertType(r6b4, "boolean");
AssertType(d != x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r6b5 = e != x;
AssertType(r6b5, "boolean");
AssertType(e != x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r6b6 = f != x;
AssertType(r6b6, "boolean");
AssertType(f != x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r6b7 = g != x;
AssertType(r6b7, "boolean");
AssertType(g != x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator ===
let r7a1 = x === a;
AssertType(r7a1, "boolean");
AssertType(x === a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r7a2 = x === b;
AssertType(r7a2, "boolean");
AssertType(x === b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r7a3 = x === c;
AssertType(r7a3, "boolean");
AssertType(x === c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r7a4 = x === d;
AssertType(r7a4, "boolean");
AssertType(x === d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r7a5 = x === e;
AssertType(r7a5, "boolean");
AssertType(x === e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r7a6 = x === f;
AssertType(r7a6, "boolean");
AssertType(x === f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r7a7 = x === g;
AssertType(r7a7, "boolean");
AssertType(x === g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r7b1 = a === x;
AssertType(r7b1, "boolean");
AssertType(a === x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r7b2 = b === x;
AssertType(r7b2, "boolean");
AssertType(b === x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r7b3 = c === x;
AssertType(r7b3, "boolean");
AssertType(c === x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r7b4 = d === x;
AssertType(r7b4, "boolean");
AssertType(d === x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r7b5 = e === x;
AssertType(r7b5, "boolean");
AssertType(e === x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r7b6 = f === x;
AssertType(r7b6, "boolean");
AssertType(f === x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r7b7 = g === x;
AssertType(r7b7, "boolean");
AssertType(g === x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");

// operator !==
let r8a1 = x !== a;
AssertType(r8a1, "boolean");
AssertType(x !== a, "boolean");
AssertType(x, "any");
AssertType(a, "boolean");

let r8a2 = x !== b;
AssertType(r8a2, "boolean");
AssertType(x !== b, "boolean");
AssertType(x, "any");
AssertType(b, "number");

let r8a3 = x !== c;
AssertType(r8a3, "boolean");
AssertType(x !== c, "boolean");
AssertType(x, "any");
AssertType(c, "string");

let r8a4 = x !== d;
AssertType(r8a4, "boolean");
AssertType(x !== d, "boolean");
AssertType(x, "any");
AssertType(d, "void");

let r8a5 = x !== e;
AssertType(r8a5, "boolean");
AssertType(x !== e, "boolean");
AssertType(x, "any");
AssertType(e, "E");

let r8a6 = x !== f;
AssertType(r8a6, "boolean");
AssertType(x !== f, "boolean");
AssertType(x, "any");
AssertType(f, "{}");

let r8a7 = x !== g;
AssertType(r8a7, "boolean");
AssertType(x !== g, "boolean");
AssertType(x, "any");
AssertType(g, "string[]");

let r8b1 = a !== x;
AssertType(r8b1, "boolean");
AssertType(a !== x, "boolean");
AssertType(a, "boolean");
AssertType(x, "any");

let r8b2 = b !== x;
AssertType(r8b2, "boolean");
AssertType(b !== x, "boolean");
AssertType(b, "number");
AssertType(x, "any");

let r8b3 = c !== x;
AssertType(r8b3, "boolean");
AssertType(c !== x, "boolean");
AssertType(c, "string");
AssertType(x, "any");

let r8b4 = d !== x;
AssertType(r8b4, "boolean");
AssertType(d !== x, "boolean");
AssertType(d, "void");
AssertType(x, "any");

let r8b5 = e !== x;
AssertType(r8b5, "boolean");
AssertType(e !== x, "boolean");
AssertType(e, "E");
AssertType(x, "any");

let r8b6 = f !== x;
AssertType(r8b6, "boolean");
AssertType(f !== x, "boolean");
AssertType(f, "{}");
AssertType(x, "any");

let r8b7 = g !== x;
AssertType(r8b7, "boolean");
AssertType(g !== x, "boolean");
AssertType(g, "string[]");
AssertType(x, "any");


