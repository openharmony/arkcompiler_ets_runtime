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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericContextualTypes1.ts ===
declare function AssertType(value:any, type:string):void;
type Box<T> = { value: T };

declare function wrap<A, B>(f: (a: A) => B): (a: A) => B;

declare function compose<A, B, C>(f: (a: A) => B, g: (b: B) => C): (a: A) => C;

declare function list<T>(a: T): T[];

declare function unlist<T>(a: T[]): T;

declare function box<V>(x: V): Box<V>;

declare function unbox<W>(x: Box<W>): W;

declare function map<T, U>(a: T[], f: (x: T) => U): U[];

declare function identity<T>(x: T): T;

declare function zip<A, B>(a: A, b: B): [A, B];

declare function flip<X, Y, Z>(f: (x: X, y: Y) => Z): (y: Y, x: X) => Z;

const f00: <A>(x: A) => A[] = list;
AssertType(f00, "<A>(A) => A[]");
AssertType(x, "A");
AssertType(list, "<T>(T) => T[]");

const f01: <A>(x: A) => A[] = x => [x];
AssertType(f01, "<A>(A) => A[]");
AssertType(x, "A");
AssertType(x => [x], "<A>(A) => A[]");
AssertType(x, "A");
AssertType([x], "A[]");
AssertType(x, "A");

const f02: <A>(x: A) => A[] = wrap(list);
AssertType(f02, "<A>(A) => A[]");
AssertType(x, "A");
AssertType(wrap(list), "(A) => A[]");
AssertType(wrap, "<A, B>((A) => B) => (A) => B");
AssertType(list, "<T>(T) => T[]");

const f03: <A>(x: A) => A[] = wrap(x => [x]);
AssertType(f03, "<A>(A) => A[]");
AssertType(x, "A");
AssertType(wrap(x => [x]), "(A) => A[]");
AssertType(wrap, "<A, B>((A) => B) => (A) => B");
AssertType(x => [x], "(A) => A[]");
AssertType(x, "A");
AssertType([x], "A[]");
AssertType(x, "A");

const f10: <T>(x: T) => Box<T[]> = compose(a => list(a), b => box(b));
AssertType(f10, "<T>(T) => Box<T[]>");
AssertType(x, "T");
AssertType(compose(a => list(a), b => box(b)), "(T) => Box<T[]>");
AssertType(compose, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(a => list(a), "(T) => T[]");
AssertType(a, "T");
AssertType(list(a), "T[]");
AssertType(list, "<T>(T) => T[]");
AssertType(a, "T");
AssertType(b => box(b), "(T[]) => Box<T[]>");
AssertType(b, "T[]");
AssertType(box(b), "Box<T[]>");
AssertType(box, "<V>(V) => Box<V>");
AssertType(b, "T[]");

const f11: <T>(x: T) => Box<T[]> = compose(list, box);
AssertType(f11, "<T>(T) => Box<T[]>");
AssertType(x, "T");
AssertType(compose(list, box), "(T) => Box<T[]>");
AssertType(compose, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(list, "<T>(T) => T[]");
AssertType(box, "<V>(V) => Box<V>");

const f12: <T>(x: Box<T[]>) => T = compose(a => unbox(a), b => unlist(b));
AssertType(f12, "<T>(Box<T[]>) => T");
AssertType(x, "Box<T[]>");
AssertType(compose(a => unbox(a), b => unlist(b)), "(Box<T[]>) => T");
AssertType(compose, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(a => unbox(a), "(Box<T[]>) => T[]");
AssertType(a, "Box<T[]>");
AssertType(unbox(a), "T[]");
AssertType(unbox, "<W>(Box<W>) => W");
AssertType(a, "Box<T[]>");
AssertType(b => unlist(b), "(T[]) => T");
AssertType(b, "T[]");
AssertType(unlist(b), "T");
AssertType(unlist, "<T>(T[]) => T");
AssertType(b, "T[]");

const f13: <T>(x: Box<T[]>) => T = compose(unbox, unlist);
AssertType(f13, "<T>(Box<T[]>) => T");
AssertType(x, "Box<T[]>");
AssertType(compose(unbox, unlist), "(Box<T[]>) => T");
AssertType(compose, "<A, B, C>((A) => B, (B) => C) => (A) => C");
AssertType(unbox, "<W>(Box<W>) => W");
AssertType(unlist, "<T>(T[]) => T");

const arrayMap = <T, U>(f: (x: T) => U) => (a: T[]) => a.map(f);
AssertType(arrayMap, "<T, U>((T) => U) => (T[]) => U[]");
AssertType(<T, U>(f: (x: T) => U) => (a: T[]) => a.map(f), "<T, U>((T) => U) => (T[]) => U[]");
AssertType(f, "(T) => U");
AssertType(x, "T");
AssertType((a: T[]) => a.map(f), "(T[]) => U[]");
AssertType(a, "T[]");
AssertType(a.map(f), "U[]");
AssertType(a.map, "<U>((T, number, T[]) => U, ?any) => U[]");
AssertType(f, "(T) => U");

const arrayFilter = <T>(f: (x: T) => boolean) => (a: T[]) => a.filter(f);
AssertType(arrayFilter, "<T>((T) => boolean) => (T[]) => T[]");
AssertType(<T>(f: (x: T) => boolean) => (a: T[]) => a.filter(f), "<T>((T) => boolean) => (T[]) => T[]");
AssertType(f, "(T) => boolean");
AssertType(x, "T");
AssertType((a: T[]) => a.filter(f), "(T[]) => T[]");
AssertType(a, "T[]");
AssertType(a.filter(f), "T[]");
AssertType(a.filter, "{ <S extends T>((T, number, T[]) => value is S, ?any): S[]; ((T, number, T[]) => unknown, ?any): T[]; }");
AssertType(f, "(T) => boolean");

const f20: (a: string[]) => number[] = arrayMap(x => x.length);
AssertType(f20, "(string[]) => number[]");
AssertType(a, "string[]");
AssertType(arrayMap(x => x.length), "(string[]) => number[]");
AssertType(arrayMap, "<T, U>((T) => U) => (T[]) => U[]");
AssertType(x => x.length, "(string) => number");
AssertType(x, "string");
AssertType(x.length, "number");

const f21: <A>(a: A[]) => A[][] = arrayMap(x => [x]);
AssertType(f21, "<A>(A[]) => A[][]");
AssertType(a, "A[]");
AssertType(arrayMap(x => [x]), "(A[]) => A[][]");
AssertType(arrayMap, "<T, U>((T) => U) => (T[]) => U[]");
AssertType(x => [x], "(A) => A[]");
AssertType(x, "A");
AssertType([x], "A[]");
AssertType(x, "A");

const f22: <A>(a: A[]) => A[] = arrayMap(identity);
AssertType(f22, "<A>(A[]) => A[]");
AssertType(a, "A[]");
AssertType(arrayMap(identity), "(A[]) => A[]");
AssertType(arrayMap, "<T, U>((T) => U) => (T[]) => U[]");
AssertType(identity, "<T>(T) => T");

const f23: <A>(a: A[]) => Box<A>[] = arrayMap(value => ({ value }));
AssertType(f23, "<A>(A[]) => Box<A>[]");
AssertType(a, "A[]");
AssertType(arrayMap(value => ({ value })), "(A[]) => { value: A; }[]");
AssertType(arrayMap, "<T, U>((T) => U) => (T[]) => U[]");
AssertType(value => ({ value }), "(A) => { value: A; }");
AssertType(value, "A");
AssertType(({ value }), "{ value: A; }");
AssertType({ value }, "{ value: A; }");
AssertType(value, "A");

const f30: (a: string[]) => string[] = arrayFilter(x => x.length > 10);
AssertType(f30, "(string[]) => string[]");
AssertType(a, "string[]");
AssertType(arrayFilter(x => x.length > 10), "(string[]) => string[]");
AssertType(arrayFilter, "<T>((T) => boolean) => (T[]) => T[]");
AssertType(x => x.length > 10, "(string) => boolean");
AssertType(x, "string");
AssertType(x.length > 10, "boolean");
AssertType(x.length, "number");
AssertType(10, "int");

const f31: <T extends Box<number>>(a: T[]) => T[] = arrayFilter(x => x.value > 10);
AssertType(f31, "<T extends Box<number>>(T[]) => T[]");
AssertType(a, "T[]");
AssertType(arrayFilter(x => x.value > 10), "(T[]) => T[]");
AssertType(arrayFilter, "<T>((T) => boolean) => (T[]) => T[]");
AssertType(x => x.value > 10, "(T) => boolean");
AssertType(x, "T");
AssertType(x.value > 10, "boolean");
AssertType(x.value, "number");
AssertType(10, "int");

const f40: <A, B>(b: B, a: A) => [A, B] = flip(zip);
AssertType(f40, "<A, B>(B, A) => [A, B]");
AssertType(b, "B");
AssertType(a, "A");
AssertType(flip(zip), "(B, A) => [A, B]");
AssertType(flip, "<X, Y, Z>((X, Y) => Z) => (Y, X) => Z");
AssertType(zip, "<A, B>(A, B) => [A, B]");

// Repro from #16293

type fn = <A>(a: A) => A;
const fn: fn = a => a;
AssertType(fn, "fn");
AssertType(a => a, "<A>(A) => A");
AssertType(a, "A");
AssertType(a, "A");


