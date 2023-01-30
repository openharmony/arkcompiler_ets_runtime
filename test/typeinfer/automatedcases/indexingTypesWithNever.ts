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

// === tests/cases/compiler/indexingTypesWithNever.ts ===
declare function AssertType(value:any, type:string):void;
type TestObj = {
  a: string;
  b: number;
};

// Should be never but without an error
type Result1 = TestObj[never];

type EmptyObj = {};

// Should be never but without an error
type Result2 = EmptyObj[keyof EmptyObj];

declare function genericFn1<T>(obj: T): T[never];

// Should be never
const result3 = genericFn1({ c: "ctest", d: "dtest" });
AssertType(result3, "never");
AssertType(genericFn1({ c: "ctest", d: "dtest" }), "never");
AssertType(genericFn1, "<T>(T) => T[never]");
AssertType({ c: "ctest", d: "dtest" }, "{ c: string; d: string; }");
AssertType(c, "string");
AssertType("ctest", "string");
AssertType(d, "string");
AssertType("dtest", "string");

declare function genericFn2<T extends { [ind: string]: string }>(
  obj: T
): T[never];

// Should be never
const result4 = genericFn2({ e: "etest", f: "ftest" });
AssertType(result4, "never");
AssertType(genericFn2({ e: "etest", f: "ftest" }), "never");
AssertType(genericFn2, "<T extends { [string]: string; }>(T) => T[never]");
AssertType({ e: "etest", f: "ftest" }, "{ e: string; f: string; }");
AssertType(e, "string");
AssertType("etest", "string");
AssertType(f, "string");
AssertType("ftest", "string");

declare function genericFn3<
  T extends { [K in keyof T]: T[K] },
  U extends keyof T,
  V extends keyof T
>(obj: T, u: U, v: V): T[U & V];

// Should be never
const result5 = genericFn3({ g: "gtest", h: "htest" }, "g", "h"); // 'g' & 'h' will reduce to never
AssertType(result5, "never");
AssertType(genericFn3({ g: "gtest", h: "htest" }, "g", "h"), "never");
AssertType(genericFn3, "<T extends { [K in keyof T]: T[K]; }, U extends keyof T, V extends keyof T>(T, U, V) => T[U & V]");
AssertType({ g: "gtest", h: "htest" }, "{ g: string; h: string; }");
AssertType(g, "string");
AssertType("gtest", "string");
AssertType(h, "string");
AssertType("htest", "string");
AssertType("g", "string");
AssertType("h", "string");


declare const obj: {a: string, b: number
AssertType(obj, "{ a: string; b: number; }");

AssertType(a, "string");

AssertType(b, "number");
}

declare const key: never
AssertType(key, "never");

const result6 = obj[key]
AssertType(result6, "never");
AssertType(obj[key], "never");
AssertType(obj, "{ a: string; b: number; }");
AssertType(key, "never");

// Expanded examples from https://github.com/Microsoft/TypeScript/issues/21988
type RequiredPropNames<T> = {
  [P in keyof T]-?: undefined extends T[P] ? never : P
}[keyof T];

type OptionalPropNames<T> = {
  [P in keyof T]-?: undefined extends T[P] ? P : never
}[keyof T];

type RequiredProps<T> = { [P in RequiredPropNames<T>]: T[P] };
type OptionalProps<T> = { [P in OptionalPropNames<T>]?: T[P] };

type Match<Exp, Act> = [Exp] extends [Act]
  ? ([Act] extends [Exp] ? "Match" : "Did not match 2")
  : "Did not match 1";

type ExpectType<Exp, Act> = Match<Exp, Act> extends "Match"
  ? ({} extends Exp ? Match<Required<Exp>, Required<Act>> : "Match")
  : "Did not match";

type P3 = { a: string; b: number; c?: boolean };
type P2 = { a: string; c?: boolean };
type P1 = { c?: boolean };
type P0 = {};

type P3Names = RequiredPropNames<P3>; // expect 'a' | 'b'
type P2Names = RequiredPropNames<P2>; // expect 'a'
type P1Names = RequiredPropNames<P1>; // expect never
type P0Names = RequiredPropNames<P0>; // expect never

declare const p3NameTest: ExpectType<"a" | "b", P3Names>;
AssertType(p3NameTest, "string");

declare const p2NameTest: ExpectType<"a", P2Names>;
AssertType(p2NameTest, "string");

declare const p1NameTest: ExpectType<never, P1Names>;
AssertType(p1NameTest, "string");

declare const p0NameTest: ExpectType<never, P0Names>;
AssertType(p0NameTest, "string");

type P3Props = RequiredProps<P3>; // expect { a: string; b: number }
type P2Props = RequiredProps<P2>; // expect { a: string; }
type P1Props = RequiredProps<P1>; // expect {}
type P0Props = RequiredProps<P0>; // expect {}

declare const p3Test: ExpectType<{ a: string; b: number }, P3Props>;
AssertType(p3Test, "string");
AssertType(a, "string");
AssertType(b, "number");

declare const p2Test: ExpectType<{ a: string }, P2Props>;
AssertType(p2Test, "string");
AssertType(a, "string");

declare const p1Test: ExpectType<{}, P1Props>;
AssertType(p1Test, "string");

declare const p0Test: ExpectType<{}, P0Props>;
AssertType(p0Test, "string");

type O3 = { a?: string; b?: number; c: boolean };
type O2 = { a?: string; c: boolean };
type O1 = { c: boolean };
type O0 = {};

type O3Names = OptionalPropNames<O3>; // expect 'a' | 'b'
type O2Names = OptionalPropNames<O2>; // expect 'a'
type O1Names = OptionalPropNames<O1>; // expect never
type O0Names = OptionalPropNames<O0>; // expect never

declare const o3NameTest: ExpectType<"a" | "b", O3Names>;
AssertType(o3NameTest, "string");

declare const o2NameTest: ExpectType<"a", O2Names>;
AssertType(o2NameTest, "string");

declare const o1NameTest: ExpectType<never, O1Names>;
AssertType(o1NameTest, "string");

declare const o0NameTest: ExpectType<never, O0Names>;
AssertType(o0NameTest, "string");

type O3Props = OptionalProps<O3>; // expect { a?: string | undefined; b?: number | undefined }
type O2Props = OptionalProps<O2>; // expect { a?: string | undefined; }
type O1Props = OptionalProps<O1>; // expect {}
type O0Props = OptionalProps<O0>; // expect {}

declare const o3Test: ExpectType<{ a?: string; b?: number }, O3Props>;
AssertType(o3Test, "string");
AssertType(a, "union");
AssertType(b, "union");

declare const o2Test: ExpectType<{ a?: string }, O2Props>;
AssertType(o2Test, "string");
AssertType(a, "union");

declare const o1Test: ExpectType<{}, O1Props>;
AssertType(o1Test, "string");

declare const o0Test: ExpectType<{}, O0Props>;
AssertType(o0Test, "string");

// Repro from #23005

type Example<T extends Record<'a', string>> = T['a'];

type Res1 = Example<{ a: "x" } | { a: "y" }>;  // "x" | "y"
type Res2 = Example<{ a: "x" }>;  // "x"
type Res3 = Example<never>;  // never 


