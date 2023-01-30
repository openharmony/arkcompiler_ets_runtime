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

// === tests/cases/conformance/types/literal/literalTypeWidening.ts ===
declare function AssertType(value:any, type:string):void;
// Widening vs. non-widening literal types

function f1() {
    const c1 = "hello";  // Widening type "hello"
AssertType(c1, "string");
AssertType("hello", "string");

    let v1 = c1;  // Type string
AssertType(v1, "string");
AssertType(c1, "string");

    const c2 = c1;  // Widening type "hello"
AssertType(c2, "string");
AssertType(c1, "string");

    let v2 = c2;  // Type string
AssertType(v2, "string");
AssertType(c2, "string");

    const c3: "hello" = "hello";  // Type "hello"
AssertType(c3, "string");
AssertType("hello", "string");

    let v3 = c3;  // Type "hello"
AssertType(v3, "string");
AssertType(c3, "string");

    const c4: "hello" = c1;  // Type "hello"
AssertType(c4, "string");
AssertType(c1, "string");

    let v4 = c4;  // Type "hello"
AssertType(v4, "string");
AssertType(c4, "string");
}

function f2(cond: boolean) {
    const c1 = cond ? "foo" : "bar";  // widening "foo" | widening "bar"
AssertType(c1, "union");
AssertType(cond ? "foo" : "bar", "union");
AssertType(cond, "boolean");
AssertType("foo", "string");
AssertType("bar", "string");

    const c2: "foo" | "bar" = c1;  // "foo" | "bar"
AssertType(c2, "union");
AssertType(c1, "union");

    const c3 = cond ? c1 : c2;  // "foo" | "bar"
AssertType(c3, "union");
AssertType(cond ? c1 : c2, "union");
AssertType(cond, "boolean");
AssertType(c1, "union");
AssertType(c2, "union");

    const c4 = cond ? c3 : "baz";  // "foo" | "bar" | widening "baz"
AssertType(c4, "union");
AssertType(cond ? c3 : "baz", "union");
AssertType(cond, "boolean");
AssertType(c3, "union");
AssertType("baz", "string");

    const c5: "foo" | "bar" | "baz" = c4; // "foo" | "bar" | "baz"
AssertType(c5, "union");
AssertType(c4, "union");

    let v1 = c1;  // string
AssertType(v1, "string");
AssertType(c1, "union");

    let v2 = c2;  // "foo" | "bar"
AssertType(v2, "union");
AssertType(c2, "union");

    let v3 = c3;  // "foo" | "bar"
AssertType(v3, "union");
AssertType(c3, "union");

    let v4 = c4;  // string
AssertType(v4, "string");
AssertType(c4, "union");

    let v5 = c5;  // "foo" | "bar" | "baz"
AssertType(v5, "union");
AssertType(c5, "union");
}

function f3() {
    const c1 = 123;  // Widening type 123
AssertType(c1, "int");
AssertType(123, "int");

    let v1 = c1;  // Type number
AssertType(v1, "number");
AssertType(c1, "int");

    const c2 = c1;  // Widening type 123
AssertType(c2, "int");
AssertType(c1, "int");

    let v2 = c2;  // Type number
AssertType(v2, "number");
AssertType(c2, "int");

    const c3: 123 = 123;  // Type 123
AssertType(c3, "int");
AssertType(123, "int");

    let v3 = c3;  // Type 123
AssertType(v3, "int");
AssertType(c3, "int");

    const c4: 123 = c1;  // Type 123
AssertType(c4, "int");
AssertType(c1, "int");

    let v4 = c4;  // Type 123
AssertType(v4, "int");
AssertType(c4, "int");
}

function f4(cond: boolean) {
    const c1 = cond ? 123 : 456;  // widening 123 | widening 456
AssertType(c1, "union");
AssertType(cond ? 123 : 456, "union");
AssertType(cond, "boolean");
AssertType(123, "int");
AssertType(456, "int");

    const c2: 123 | 456 = c1;  // 123 | 456
AssertType(c2, "union");
AssertType(c1, "union");

    const c3 = cond ? c1 : c2;  // 123 | 456
AssertType(c3, "union");
AssertType(cond ? c1 : c2, "union");
AssertType(cond, "boolean");
AssertType(c1, "union");
AssertType(c2, "union");

    const c4 = cond ? c3 : 789;  // 123 | 456 | widening 789
AssertType(c4, "union");
AssertType(cond ? c3 : 789, "union");
AssertType(cond, "boolean");
AssertType(c3, "union");
AssertType(789, "int");

    const c5: 123 | 456 | 789 = c4; // 123 | 456 | 789
AssertType(c5, "union");
AssertType(c4, "union");

    let v1 = c1;  // number
AssertType(v1, "number");
AssertType(c1, "union");

    let v2 = c2;  // 123 | 456
AssertType(v2, "union");
AssertType(c2, "union");

    let v3 = c3;  // 123 | 456
AssertType(v3, "union");
AssertType(c3, "union");

    let v4 = c4;  // number
AssertType(v4, "number");
AssertType(c4, "union");

    let v5 = c5;  // 123 | 456 | 789
AssertType(v5, "union");
AssertType(c5, "union");
}

function f5() {
    const c1 = "foo";
AssertType(c1, "string");
AssertType("foo", "string");

    let v1 = c1;
AssertType(v1, "string");
AssertType(c1, "string");

    const c2: "foo" = "foo";
AssertType(c2, "string");
AssertType("foo", "string");

    let v2 = c2;
AssertType(v2, "string");
AssertType(c2, "string");

    const c3 = "foo" as "foo";
AssertType(c3, "string");
AssertType("foo" as "foo", "string");
AssertType("foo", "string");

    let v3 = c3;
AssertType(v3, "string");
AssertType(c3, "string");

    const c4 = <"foo">"foo";
AssertType(c4, "string");
AssertType(<"foo">"foo", "string");
AssertType("foo", "string");

    let v4 = c4;
AssertType(v4, "string");
AssertType(c4, "string");
}

declare function widening<T>(x: T): T;
declare function nonWidening<T extends string | number | symbol>(x: T): T;

function f6(cond: boolean) {
    let x1 = widening('a');
AssertType(x1, "string");
AssertType(widening('a'), "string");
AssertType(widening, "<T>(T) => T");
AssertType('a', "string");

    let x2 = widening(10);
AssertType(x2, "number");
AssertType(widening(10), "int");
AssertType(widening, "<T>(T) => T");
AssertType(10, "int");

    let x3 = widening(cond ? 'a' : 10);
AssertType(x3, "union");
AssertType(widening(cond ? 'a' : 10), "union");
AssertType(widening, "<T>(T) => T");
AssertType(cond ? 'a' : 10, "union");
AssertType(cond, "boolean");
AssertType('a', "string");
AssertType(10, "int");

    let y1 = nonWidening('a');
AssertType(y1, "string");
AssertType(nonWidening('a'), "string");
AssertType(nonWidening, "<T extends union>(T) => T");
AssertType('a', "string");

    let y2 = nonWidening(10);
AssertType(y2, "int");
AssertType(nonWidening(10), "int");
AssertType(nonWidening, "<T extends union>(T) => T");
AssertType(10, "int");

    let y3 = nonWidening(cond ? 'a' : 10);
AssertType(y3, "union");
AssertType(nonWidening(cond ? 'a' : 10), "union");
AssertType(nonWidening, "<T extends union>(T) => T");
AssertType(cond ? 'a' : 10, "union");
AssertType(cond, "boolean");
AssertType('a', "string");
AssertType(10, "int");
}

// Repro from #10898

type FAILURE = "FAILURE";
const FAILURE = "FAILURE";
AssertType(FAILURE, "string");
AssertType("FAILURE", "string");

type Result<T> = T | FAILURE;

function doWork<T>(): Result<T> {
AssertType(FAILURE, "string");
  return FAILURE;
}

function isSuccess<T>(result: Result<T>): result is T {
AssertType(!isFailure(result), "boolean");
AssertType(isFailure(result), "boolean");
AssertType(isFailure, "<T>(Result<T>) => result is "FAILURE"");
AssertType(result, "Result<T>");
  return !isFailure(result);
}

function isFailure<T>(result: Result<T>): result is FAILURE {
AssertType(result === FAILURE, "boolean");
AssertType(result, "Result<T>");
AssertType(FAILURE, "string");
  return result === FAILURE;
}

function increment(x: number): number {
AssertType(x + 1, "number");
AssertType(x, "number");
AssertType(1, "int");
  return x + 1;
}

let result = doWork<number>();
AssertType(result, "Result<number>");
AssertType(doWork<number>(), "Result<number>");
AssertType(doWork, "<T>() => Result<T>");

if (isSuccess(result)) {
  increment(result);
AssertType(increment(result), "number");
AssertType(increment, "(number) => number");
AssertType(result, "number");
}

// Repro from #10898

type TestEvent = "onmouseover" | "onmouseout";

function onMouseOver(): TestEvent { 
AssertType("onmouseover", "string");
return "onmouseover"; 
}

let x = onMouseOver();
AssertType(x, "TestEvent");
AssertType(onMouseOver(), "TestEvent");
AssertType(onMouseOver, "() => TestEvent");

// Repro from #23649

export function Set<K extends string>(...keys: K[]): Record<K, true | undefined> {
  const result = {} as Record<K, true | undefined>
AssertType(result, "Record<K, true>");
AssertType({} as Record<K, true | undefined>, "Record<K, true>");
AssertType({}, "{}");
AssertType(true, "boolean");

  keys.forEach(key => result[key] = true)
AssertType(keys.forEach(key => result[key] = true), "void");
AssertType(keys.forEach, "((K, number, K[]) => void, ?any) => void");
AssertType(key => result[key] = true, "(K) => boolean");
AssertType(key, "K");
AssertType(result[key] = true, "boolean");
AssertType(result[key], "Record<K, true>[K]");
AssertType(result, "Record<K, true>");
AssertType(key, "K");
AssertType(true, "boolean");

AssertType(result, "Record<K, true>");
  return result
}

export function keys<K extends string, V>(obj: Record<K, V>): K[] {
AssertType(Object.keys(obj) as K[], "K[]");
AssertType(Object.keys(obj), "string[]");
AssertType(Object.keys, "(object) => string[]");
AssertType(obj, "Record<K, V>");
  return Object.keys(obj) as K[]
}

type Obj = { code: LangCode }

const langCodeSet = Set('fr', 'en', 'es', 'it', 'nl')
AssertType(langCodeSet, "Record<union, true>");
AssertType(Set('fr', 'en', 'es', 'it', 'nl'), "Record<union, true>");
AssertType(Set, "<K extends string>(...K[]) => Record<K, true>");
AssertType('fr', "string");
AssertType('en', "string");
AssertType('es', "string");
AssertType('it', "string");
AssertType('nl', "string");

export type LangCode = keyof typeof langCodeSet
export const langCodes = keys(langCodeSet)
AssertType(langCodes, "(union)[]");
AssertType(keys(langCodeSet), "(union)[]");
AssertType(keys, "<K extends string, V>(Record<K, V>) => K[]");
AssertType(langCodeSet, "Record<union, true>");

const arr: Obj[] = langCodes.map(code => ({ code }))
AssertType(arr, "Obj[]");
AssertType(langCodes.map(code => ({ code })), "{ code: union; }[]");
AssertType(langCodes.map, "<U>((union, number, (union)[]) => U, ?any) => U[]");
AssertType(code => ({ code }), "(union) => { code: union; }");
AssertType(code, "union");
AssertType(({ code }), "{ code: union; }");
AssertType({ code }, "{ code: union; }");
AssertType(code, "union");

// Repro from #29081

function test<T extends { a: string, b: string }>(obj: T): T {
    let { a, ...rest } = obj;
AssertType(a, "string");
AssertType(rest, "Omit<T, "a">");
AssertType(obj, "T");

AssertType({ a: 'hello', ...rest } as T, "T");
AssertType({ a: 'hello', ...rest }, "{ a: string; } & Omit<T, "a">");
AssertType(a, "string");
AssertType('hello', "string");
AssertType(rest, "Omit<T, "a">");
    return { a: 'hello', ...rest } as T;
}

// Repro from #32169

declare function f<T>(x: T): NonNullable<T>;
enum E { A, B }
const a = f(E.A);
AssertType(a, "E.A");
AssertType(f(E.A), "E.A");
AssertType(f, "<T>(T) => NonNullable<T>");
AssertType(E.A, "E.A");

const b: E.A = a;
AssertType(b, "E.A");
AssertType(E, "any");
AssertType(a, "E.A");


