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

// === tests/cases/conformance/types/mapped/mappedTypesArraysTuples.ts ===
declare function AssertType(value:any, type:string):void;
type Box<T> = { value: T };
type Boxified<T> = { [P in keyof T]: Box<T[P]> };

type T00 = Boxified<[number, string?, ...boolean[]]>;
type T01 = Partial<[number, string?, ...boolean[]]>;
type T02 = Required<[number, string?, ...boolean[]]>;

type T10 = Boxified<string[]>;
type T11 = Partial<string[]>;
type T12 = Required<string[]>;
type T13 = Boxified<ReadonlyArray<string>>;
type T14 = Partial<ReadonlyArray<string>>;
type T15 = Required<ReadonlyArray<string>>;

type T20 = Boxified<(string | undefined)[]>;
type T21 = Partial<(string | undefined)[]>;
type T22 = Required<(string | undefined)[]>;
type T23 = Boxified<ReadonlyArray<string | undefined>>;
type T24 = Partial<ReadonlyArray<string | undefined>>;
type T25 = Required<ReadonlyArray<string | undefined>>;

type T30 = Boxified<Partial<string[]>>;
type T31 = Partial<Boxified<string[]>>;

type A = { a: string };
type B = { b: string };

type T40 = Boxified<A | A[] | ReadonlyArray<A> | [A, B] | string | string[]>;

type ReadWrite<T> = { -readonly [P in keyof T] : T[P] };

type T50 = Readonly<string[]>;
type T51 = Readonly<[number, number]>;
type T52 = Partial<Readonly<string[]>>;
type T53 = Readonly<Partial<string[]>>;
type T54 = ReadWrite<Required<T53>>;

declare function unboxify<T>(x: Boxified<T>): T;

declare let x10: [Box<number>, Box<string>, ...Box<boolean>[]];
AssertType(x10, "[Box<number>, Box<string>, ...Box<boolean>[]]");

let y10 = unboxify(x10);
AssertType(y10, "[number, string, ...boolean[]]");
AssertType(unboxify(x10), "[number, string, ...boolean[]]");
AssertType(unboxify, "<T>(Boxified<T>) => T");
AssertType(x10, "[Box<number>, Box<string>, ...Box<boolean>[]]");

declare let x11: Box<number>[];
AssertType(x11, "Box<number>[]");

let y11 = unboxify(x11);
AssertType(y11, "number[]");
AssertType(unboxify(x11), "number[]");
AssertType(unboxify, "<T>(Boxified<T>) => T");
AssertType(x11, "Box<number>[]");

declare let x12: { a: Box<number>, b: Box<string[]> };
AssertType(x12, "{ a: Box<number>; b: Box<string[]>; }");
AssertType(a, "Box<number>");
AssertType(b, "Box<string[]>");

let y12 = unboxify(x12);
AssertType(y12, "{ a: number; b: string[]; }");
AssertType(unboxify(x12), "{ a: number; b: string[]; }");
AssertType(unboxify, "<T>(Boxified<T>) => T");
AssertType(x12, "{ a: Box<number>; b: Box<string[]>; }");

declare function nonpartial<T>(x: Partial<T>): T;

declare let x20: [number | undefined, string?, ...boolean[]];
AssertType(x20, "[union, (union)?, ...boolean[]]");

let y20 = nonpartial(x20);
AssertType(y20, "[number, string, ...boolean[]]");
AssertType(nonpartial(x20), "[number, string, ...boolean[]]");
AssertType(nonpartial, "<T>(Partial<T>) => T");
AssertType(x20, "[union, (union)?, ...boolean[]]");

declare let x21: (number | undefined)[];
AssertType(x21, "(union)[]");

let y21 = nonpartial(x21);
AssertType(y21, "number[]");
AssertType(nonpartial(x21), "number[]");
AssertType(nonpartial, "<T>(Partial<T>) => T");
AssertType(x21, "(union)[]");

declare let x22: { a: number | undefined, b?: string[] };
AssertType(x22, "{ a: union; b?: union; }");
AssertType(a, "union");
AssertType(b, "union");

let y22 = nonpartial(x22);
AssertType(y22, "{ a: number; b: string[]; }");
AssertType(nonpartial(x22), "{ a: number; b: string[]; }");
AssertType(nonpartial, "<T>(Partial<T>) => T");
AssertType(x22, "{ a: union; b?: union; }");

type __Awaited<T> = T extends PromiseLike<infer U> ? U : T;
type Awaitified<T> = { [P in keyof T]: __Awaited<T[P]> };

declare function all<T extends any[]>(...values: T): Promise<Awaitified<T>>;

function f1(a: number, b: Promise<number>, c: string[], d: Promise<string[]>) {
    let x1 = all(a);
AssertType(x1, "Promise<[number]>");
AssertType(all(a), "Promise<[number]>");
AssertType(all, "<T extends any[]>(...T) => Promise<Awaitified<T>>");
AssertType(a, "number");

    let x2 = all(a, b);
AssertType(x2, "Promise<[number, number]>");
AssertType(all(a, b), "Promise<[number, number]>");
AssertType(all, "<T extends any[]>(...T) => Promise<Awaitified<T>>");
AssertType(a, "number");
AssertType(b, "Promise<number>");

    let x3 = all(a, b, c);
AssertType(x3, "Promise<[number, number, string[]]>");
AssertType(all(a, b, c), "Promise<[number, number, string[]]>");
AssertType(all, "<T extends any[]>(...T) => Promise<Awaitified<T>>");
AssertType(a, "number");
AssertType(b, "Promise<number>");
AssertType(c, "string[]");

    let x4 = all(a, b, c, d);
AssertType(x4, "Promise<[number, number, string[], string[]]>");
AssertType(all(a, b, c, d), "Promise<[number, number, string[], string[]]>");
AssertType(all, "<T extends any[]>(...T) => Promise<Awaitified<T>>");
AssertType(a, "number");
AssertType(b, "Promise<number>");
AssertType(c, "string[]");
AssertType(d, "Promise<string[]>");
}

function f2<T extends any[]>(a: Boxified<T>) {
    let x: Box<any> | undefined = a.pop();
AssertType(x, "union");
AssertType(a.pop(), "union");
AssertType(a.pop, "() => union");

    let y: Box<any>[] = a.concat(a);
AssertType(y, "Box<any>[]");
AssertType(a.concat(a), "Box<any>[]");
AssertType(a.concat, "{ (...ConcatArray<Box<any>>[]): Box<any>[]; (...(union)[]): Box<any>[]; }");
AssertType(a, "Boxified<T>");
}

// Repro from #26163

type ElementType<T> = T extends Array<infer U> ? U : never;
type Mapped<T> = { [K in keyof T]: T[K] };

type F<T> = ElementType<Mapped<T>>;
type R1 = F<[string, number, boolean]>;  // string | number | boolean
type R2 = ElementType<Mapped<[string, number, boolean]>>;  // string | number | boolean

// Repro from #26163

declare function acceptArray(arr: any[]): void;
declare function mapArray<T extends any[]>(arr: T): Mapped<T>;
function acceptMappedArray<T extends any[]>(arr: T) {
    acceptArray(mapArray(arr));
AssertType(acceptArray(mapArray(arr)), "void");
AssertType(acceptArray, "(any[]) => void");
AssertType(mapArray(arr), "Mapped<T>");
AssertType(mapArray, "<T extends any[]>(T) => Mapped<T>");
AssertType(arr, "T");
}

// Repro from #26163

type Unconstrained<T> = ElementType<Mapped<T>>;
type T1 = Unconstrained<[string, number, boolean]>;  // string | number | boolean

type Constrained<T extends any[]> = ElementType<Mapped<T>>;
type T2 = Constrained<[string, number, boolean]>;  // string | number | boolean


