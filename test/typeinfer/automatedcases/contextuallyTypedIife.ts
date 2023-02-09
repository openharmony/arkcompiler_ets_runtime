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

// === tests/cases/conformance/expressions/functions/contextuallyTypedIife.ts ===
declare function AssertType(value:any, type:string):void;
// arrow
(jake => { })("build");
AssertType((jake => { })("build"), "void");
AssertType((jake => { }), "(string) => void");
AssertType(jake => { }, "(string) => void");
AssertType(jake, "string");
AssertType("build", "string");

// function expression
(function (cats) { })("lol");
AssertType((function (cats) { })("lol"), "void");
AssertType((function (cats) { }), "(string) => void");
AssertType(function (cats) { }, "(string) => void");
AssertType(cats, "string");
AssertType("lol", "string");

// Lots of Irritating Superfluous Parentheses
(function (x) { } ("!"));
AssertType((function (x) { } ("!")), "void");
AssertType(function (x) { } ("!"), "void");
AssertType(function (x) { }, "(string) => void");
AssertType(x, "string");
AssertType("!", "string");

((((function (y) { }))))("-");
AssertType(((((function (y) { }))))("-"), "void");
AssertType(((((function (y) { })))), "(string) => void");
AssertType((((function (y) { }))), "(string) => void");
AssertType(((function (y) { })), "(string) => void");
AssertType((function (y) { }), "(string) => void");
AssertType(function (y) { }, "(string) => void");
AssertType(y, "string");
AssertType("-", "string");

// multiple arguments
((a, b, c) => { })("foo", 101, false);
AssertType(((a, b, c) => { })("foo", 101, false), "void");
AssertType(((a, b, c) => { }), "(string, number, boolean) => void");
AssertType((a, b, c) => { }, "(string, number, boolean) => void");
AssertType(a, "string");
AssertType(b, "number");
AssertType(c, "boolean");
AssertType("foo", "string");
AssertType(101, "int");
AssertType(false, "boolean");

// default parameters
((m = 10) => m + 1)(12);
AssertType(((m = 10) => m + 1)(12), "number");
AssertType(((m = 10) => m + 1), "(?number) => number");
AssertType((m = 10) => m + 1, "(?number) => number");
AssertType(m, "number");
AssertType(10, "int");
AssertType(m + 1, "number");
AssertType(m, "number");
AssertType(1, "int");
AssertType(12, "int");

((n = 10) => n + 1)();
AssertType(((n = 10) => n + 1)(), "number");
AssertType(((n = 10) => n + 1), "(?number) => number");
AssertType((n = 10) => n + 1, "(?number) => number");
AssertType(n, "number");
AssertType(10, "int");
AssertType(n + 1, "number");
AssertType(n, "number");
AssertType(1, "int");

// optional parameters
((j?) => j + 1)(12);
AssertType(((j?) => j + 1)(12), "number");
AssertType(((j?) => j + 1), "(?number) => number");
AssertType((j?) => j + 1, "(?number) => number");
AssertType(j, "number");
AssertType(j + 1, "number");
AssertType(j, "number");
AssertType(1, "int");
AssertType(12, "int");

((k?) => k + 1)();
AssertType(((k?) => k + 1)(), "any");
AssertType(((k?) => k + 1), "(?any) => any");
AssertType((k?) => k + 1, "(?any) => any");
AssertType(k, "any");
AssertType(k + 1, "any");
AssertType(k, "any");
AssertType(1, "int");

((l, o?) => l + o)(12); // o should be any
AssertType(((l, o?) => l + o)(12), "any");
AssertType(((l, o?) => l + o), "(number, ?any) => any");
AssertType((l, o?) => l + o, "(number, ?any) => any");
AssertType(l, "number");
AssertType(o, "any");
AssertType(l + o, "any");
AssertType(l, "number");
AssertType(o, "any");
AssertType(12, "int");

// rest parameters
((...numbers) => numbers.every(n => n > 0))(5,6,7);
AssertType(((...numbers) => numbers.every(n => n > 0))(5,6,7), "boolean");
AssertType(((...numbers) => numbers.every(n => n > 0)), "(number, number, number) => boolean");
AssertType((...numbers) => numbers.every(n => n > 0), "(number, number, number) => boolean");
AssertType(numbers, "[number, number, number]");
AssertType(numbers.every(n => n > 0), "boolean");
AssertType(numbers.every, "{ <S extends number>((number, number, number[]) => value is S, ?any): this is S[]; ((number, number, number[]) => unknown, ?any): boolean; }");
AssertType(n => n > 0, "(number) => boolean");
AssertType(n, "number");
AssertType(n > 0, "boolean");
AssertType(n, "number");
AssertType(0, "int");
AssertType(5, "int");
AssertType(6, "int");
AssertType(7, "int");

((...mixed) => mixed.every(n => !!n))(5,'oops','oh no');
AssertType(((...mixed) => mixed.every(n => !!n))(5,'oops','oh no'), "boolean");
AssertType(((...mixed) => mixed.every(n => !!n)), "(number, string, string) => boolean");
AssertType((...mixed) => mixed.every(n => !!n), "(number, string, string) => boolean");
AssertType(mixed, "[number, string, string]");
AssertType(mixed.every(n => !!n), "boolean");
AssertType(mixed.every, "{ <S extends union>((union, number, (union)[]) => value is S, ?any): this is S[]; ((union, number, (union)[]) => unknown, ?any): boolean; }");
AssertType(n => !!n, "(union) => boolean");
AssertType(n, "union");
AssertType(!!n, "boolean");
AssertType(!n, "boolean");
AssertType(n, "union");
AssertType(5, "int");
AssertType('oops', "string");
AssertType('oh no', "string");

((...noNumbers) => noNumbers.some(n => n > 0))();
AssertType(((...noNumbers) => noNumbers.some(n => n > 0))(), "boolean");
AssertType(((...noNumbers) => noNumbers.some(n => n > 0)), "() => boolean");
AssertType((...noNumbers) => noNumbers.some(n => n > 0), "() => boolean");
AssertType(noNumbers, "[]");
AssertType(noNumbers.some(n => n > 0), "boolean");
AssertType(noNumbers.some, "((never, number, never[]) => unknown, ?any) => boolean");
AssertType(n => n > 0, "(never) => boolean");
AssertType(n, "never");
AssertType(n > 0, "boolean");
AssertType(n, "never");
AssertType(0, "int");

((first, ...rest) => first ? [] : rest.map(n => n > 0))(8,9,10);
AssertType(((first, ...rest) => first ? [] : rest.map(n => n > 0))(8,9,10), "boolean[]");
AssertType(((first, ...rest) => first ? [] : rest.map(n => n > 0)), "(number, number, number) => boolean[]");
AssertType((first, ...rest) => first ? [] : rest.map(n => n > 0), "(number, number, number) => boolean[]");
AssertType(first, "number");
AssertType(rest, "[number, number]");
AssertType(first ? [] : rest.map(n => n > 0), "boolean[]");
AssertType(first, "number");
AssertType([], "undefined[]");
AssertType(rest.map(n => n > 0), "boolean[]");
AssertType(rest.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n > 0, "(number) => boolean");
AssertType(n, "number");
AssertType(n > 0, "boolean");
AssertType(n, "number");
AssertType(0, "int");
AssertType(8, "int");
AssertType(9, "int");
AssertType(10, "int");

// destructuring parameters (with defaults too!)
(({ q }) => q)({ q : 13 });
AssertType((({ q }) => q)({ q : 13 }), "number");
AssertType((({ q }) => q), "({ q: number; }) => number");
AssertType(({ q }) => q, "({ q: number; }) => number");
AssertType(q, "number");
AssertType(q, "number");
AssertType({ q : 13 }, "{ q: number; }");
AssertType(q, "number");
AssertType(13, "int");

(({ p = 14 }) => p)({ p : 15 });
AssertType((({ p = 14 }) => p)({ p : 15 }), "number");
AssertType((({ p = 14 }) => p), "({ p: number; }) => number");
AssertType(({ p = 14 }) => p, "({ p: number; }) => number");
AssertType(p, "number");
AssertType(14, "int");
AssertType(p, "number");
AssertType({ p : 15 }, "{ p: number; }");
AssertType(p, "number");
AssertType(15, "int");

(({ r = 17 } = { r: 18 }) => r)({r : 19});
AssertType((({ r = 17 } = { r: 18 }) => r)({r : 19}), "number");
AssertType((({ r = 17 } = { r: 18 }) => r), "(?{ r: number; }) => number");
AssertType(({ r = 17 } = { r: 18 }) => r, "(?{ r: number; }) => number");
AssertType(r, "number");
AssertType(17, "int");
AssertType({ r: 18 }, "{ r: number; }");
AssertType(r, "number");
AssertType(18, "int");
AssertType(r, "number");
AssertType({r : 19}, "{ r: number; }");
AssertType(r, "number");
AssertType(19, "int");

(({ u = 22 } = { u: 23 }) => u)();
AssertType((({ u = 22 } = { u: 23 }) => u)(), "number");
AssertType((({ u = 22 } = { u: 23 }) => u), "(?{ u?: number; }) => number");
AssertType(({ u = 22 } = { u: 23 }) => u, "(?{ u?: number; }) => number");
AssertType(u, "number");
AssertType(22, "int");
AssertType({ u: 23 }, "{ u?: number; }");
AssertType(u, "number");
AssertType(23, "int");
AssertType(u, "number");

// contextually typed parameters.
let twelve = (f => f(12))(i => i);
AssertType(twelve, "any");
AssertType((f => f(12))(i => i), "any");
AssertType((f => f(12)), "((any) => any) => any");
AssertType(f => f(12), "((any) => any) => any");
AssertType(f, "(any) => any");
AssertType(f(12), "any");
AssertType(f, "(any) => any");
AssertType(12, "int");
AssertType(i => i, "(any) => any");
AssertType(i, "any");
AssertType(i, "any");

let eleven = (o => o.a(11))({ a: function(n) { 
AssertType(eleven, "any");
AssertType((o => o.a(11))({ a: function(n) { return n; } }), "any");
AssertType((o => o.a(11)), "({ a: (any) => any; }) => any");
AssertType(o => o.a(11), "({ a: (any) => any; }) => any");
AssertType(o, "{ a: (any) => any; }");
AssertType(o.a(11), "any");
AssertType(o.a, "(any) => any");
AssertType(11, "int");
AssertType({ a: function(n) { return n; } }, "{ a: (any) => any; }");
AssertType(a, "(any) => any");
AssertType(function(n) { return n; }, "(any) => any");
AssertType(n, "any");
AssertType(n, "any");
return n; } });

// missing arguments
(function(x, undefined) { 
AssertType((function(x, undefined) { return x; })(42), "number");
AssertType((function(x, undefined) { return x; }), "(number, ?any) => number");
AssertType(function(x, undefined) { return x; }, "(number, ?any) => number");
AssertType(x, "number");
AssertType(undefined, "any");
AssertType(x, "number");
AssertType(42, "int");
return x; })(42);

((x, y, z) => 42)();
AssertType(((x, y, z) => 42)(), "number");
AssertType(((x, y, z) => 42), "(?any, ?any, ?any) => number");
AssertType((x, y, z) => 42, "(?any, ?any, ?any) => number");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(42, "int");


