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

// === tests/cases/compiler/narrowingUnionToUnion.ts ===
declare function AssertType(value:any, type:string):void;
type Falsy = false | 0 | 0n | '' | null | undefined;

declare function isFalsy(value: unknown): value is Falsy;

function fx1(x: string | number | undefined) {
    if (isFalsy(x)) {
AssertType(isFalsy(x), "boolean");
AssertType(isFalsy, "(unknown) => value is Falsy");
AssertType(x, "union");

        x;  // "" | 0 | undefined
AssertType(x, "union");
    }
}

function fx2<T>(x: T | undefined) {
    if (isFalsy(x)) {
AssertType(isFalsy(x), "boolean");
AssertType(isFalsy, "(unknown) => value is Falsy");
AssertType(x, "union");

        x;  // T & Falsy | undefined
AssertType(x, "union");
    }
}

function fx3<T extends string | number>(x: T) {
    if (isFalsy(x)) {
AssertType(isFalsy(x), "boolean");
AssertType(isFalsy, "(unknown) => value is Falsy");
AssertType(x, "union");

        x;  // T & "" | T & 0
AssertType(x, "union");
    }
}

declare function isA(obj: unknown): obj is { a: false } | { b: 0 };

function fx4(obj: { b: number }) {
    if (isA(obj)) {
AssertType(isA(obj), "boolean");
AssertType(isA, "(unknown) => obj is union");
AssertType(obj, "{ b: number; }");

        obj;  // { b: 0 
AssertType(obj, "{ b: 0; }");
}
    }
}

declare class X { x: string }
declare class XS extends X { xs: string }

declare class Y { y: string }
declare class YS extends Y { ys: string }

declare function isXSorY(obj: unknown): obj is XS | Y;

function fx5<T extends X>(obj: X | YS, c: typeof XS | typeof Y) {
    if (obj instanceof c) {
AssertType(obj instanceof c, "boolean");
AssertType(obj, "union");
AssertType(c, "union");

        obj;  // XS | YS
AssertType(obj, "union");
    }
    if (isXSorY(obj)) {
AssertType(isXSorY(obj), "boolean");
AssertType(isXSorY, "(unknown) => obj is union");
AssertType(obj, "union");

        obj;  // XS | YS
AssertType(obj, "union");
    }
}

// Repro from #31156

declare function isEmptyStrOrUndefined(mixed: any): mixed is "" | undefined;

function fx10(s: string | undefined) {
    if (isEmptyStrOrUndefined(s)) {
AssertType(isEmptyStrOrUndefined(s), "boolean");
AssertType(isEmptyStrOrUndefined, "(any) => mixed is union");
AssertType(s, "union");

        s;  // "" | undefined
AssertType(s, "union");

        if (s == undefined) {
AssertType(s == undefined, "boolean");
AssertType(s, "union");
AssertType(undefined, "undefined");

            s;  // undefined
AssertType(s, "undefined");
        }
        else {
            s;  // ""
AssertType(s, "string");
        }
    }
}

// Repro from #37807

function f1(x: any): asserts x is number | undefined { }
let v1: number | string | undefined;
AssertType(v1, "union");

f1(v1);
AssertType(f1(v1), "void");
AssertType(f1, "(any) => asserts x is union");
AssertType(v1, "union");

v1;  // number | undefined
AssertType(v1, "union");

function f2(x: any): asserts x is 6 | undefined { }
let v2: number | string | undefined;
AssertType(v2, "union");

f2(v2);
AssertType(f2(v2), "void");
AssertType(f2, "(any) => asserts x is union");
AssertType(v2, "union");

v2;  // 6 | undefined
AssertType(v2, "union");

// #39105

declare function isEmptyString(value: string): value is '';
declare function isMaybeEmptyString(value: string | null | undefined): value is '' | null | undefined;

declare function isZero(value: number): value is 0;
declare function isMaybeZero(value: number | null | undefined): value is 0 | null | undefined;

declare function isEmptyArray<T>(value: T[]): value is [];
declare function isMaybeEmptyArray<T>(value: T[] | null | undefined): value is [] | null | undefined;

const TEST_CASES = [
AssertType(TEST_CASES, "(union)[]");
AssertType([    (value: string) => {        if (isEmptyString(value)) {            value;  // ""        }        else {            value;  // string        }        if (isMaybeEmptyString(value)) {            value;  // ""        }        else {            value;  // string        }    },    (value?: string) => {        if (isMaybeEmptyString(value)) {            value;  // "" | undefined        }        else {            value;  // string        }    },    (value: number) => {        if (isZero(value)) {            value;  // 0        }        else {            value;  // number        }        if (isMaybeZero(value)) {            value; // 0        }        else {            value;  // number        }    },    (value?: number) => {        if (isMaybeZero(value)) {            value;  // 0 | undefined        }        else {            value;  // number        }    },    (value: string[]) => {        if (isEmptyArray(value)) {            value;  // []        }        else {            value;  // string[]        }        if (isMaybeEmptyArray(value)) {            value;  // []        }        else {            value;  // string[]        }    },    (value?: string[]) => {        if (isMaybeEmptyArray(value)) {            value;  // [] | undefined        }        else {            value;  // string[]        }    },], "(union)[]");

    (value: string) => {
AssertType((value: string) => {        if (isEmptyString(value)) {            value;  // ""        }        else {            value;  // string        }        if (isMaybeEmptyString(value)) {            value;  // ""        }        else {            value;  // string        }    }, "(string) => void");
AssertType(value, "string");

        if (isEmptyString(value)) {
AssertType(isEmptyString(value), "boolean");
AssertType(isEmptyString, "(string) => value is """);
AssertType(value, "string");

            value;  // ""
AssertType(value, "string");
        }
        else {
            value;  // string
AssertType(value, "string");
        }
        if (isMaybeEmptyString(value)) {
AssertType(isMaybeEmptyString(value), "boolean");
AssertType(isMaybeEmptyString, "(union) => value is union");
AssertType(value, "string");

            value;  // ""
AssertType(value, "string");
        }
        else {
            value;  // string
AssertType(value, "string");
        }
    },
    (value?: string) => {
AssertType((value?: string) => {        if (isMaybeEmptyString(value)) {            value;  // "" | undefined        }        else {            value;  // string        }    }, "(?string) => void");
AssertType(value, "union");

        if (isMaybeEmptyString(value)) {
AssertType(isMaybeEmptyString(value), "boolean");
AssertType(isMaybeEmptyString, "(union) => value is union");
AssertType(value, "union");

            value;  // "" | undefined
AssertType(value, "union");
        }
        else {
            value;  // string
AssertType(value, "string");
        }
    },
    (value: number) => {
AssertType((value: number) => {        if (isZero(value)) {            value;  // 0        }        else {            value;  // number        }        if (isMaybeZero(value)) {            value; // 0        }        else {            value;  // number        }    }, "(number) => void");
AssertType(value, "number");

        if (isZero(value)) {
AssertType(isZero(value), "boolean");
AssertType(isZero, "(number) => value is 0");
AssertType(value, "number");

            value;  // 0
AssertType(value, "int");
        }
        else {
            value;  // number
AssertType(value, "number");
        }
        if (isMaybeZero(value)) {
AssertType(isMaybeZero(value), "boolean");
AssertType(isMaybeZero, "(union) => value is union");
AssertType(value, "number");

            value; // 0
AssertType(value, "int");
        }
        else {
            value;  // number
AssertType(value, "number");
        }
    },
    (value?: number) => {
AssertType((value?: number) => {        if (isMaybeZero(value)) {            value;  // 0 | undefined        }        else {            value;  // number        }    }, "(?number) => void");
AssertType(value, "union");

        if (isMaybeZero(value)) {
AssertType(isMaybeZero(value), "boolean");
AssertType(isMaybeZero, "(union) => value is union");
AssertType(value, "union");

            value;  // 0 | undefined
AssertType(value, "union");
        }
        else {
            value;  // number
AssertType(value, "number");
        }
    },
    (value: string[]) => {
AssertType((value: string[]) => {        if (isEmptyArray(value)) {            value;  // []        }        else {            value;  // string[]        }        if (isMaybeEmptyArray(value)) {            value;  // []        }        else {            value;  // string[]        }    }, "(string[]) => void");
AssertType(value, "string[]");

        if (isEmptyArray(value)) {
AssertType(isEmptyArray(value), "boolean");
AssertType(isEmptyArray, "<T>(T[]) => value is []");
AssertType(value, "string[]");

            value;  // []
AssertType(value, "[]");
        }
        else {
            value;  // string[]
AssertType(value, "string[]");
        }
        if (isMaybeEmptyArray(value)) {
AssertType(isMaybeEmptyArray(value), "boolean");
AssertType(isMaybeEmptyArray, "<T>(union) => value is union");
AssertType(value, "string[]");

            value;  // []
AssertType(value, "[]");
        }
        else {
            value;  // string[]
AssertType(value, "string[]");
        }
    },
    (value?: string[]) => {
AssertType((value?: string[]) => {        if (isMaybeEmptyArray(value)) {            value;  // [] | undefined        }        else {            value;  // string[]        }    }, "(?string[]) => void");
AssertType(value, "union");

        if (isMaybeEmptyArray(value)) {
AssertType(isMaybeEmptyArray(value), "boolean");
AssertType(isMaybeEmptyArray, "<T>(union) => value is union");
AssertType(value, "union");

            value;  // [] | undefined
AssertType(value, "union");
        }
        else {
            value;  // string[]
AssertType(value, "string[]");
        }
    },
];

// Repro from #42101

type EmptyString = '' | null | undefined;

function isEmpty(value: string | EmptyString): value is EmptyString {
AssertType(value === '' || value === null || value === undefined, "boolean");
AssertType(value === '' || value === null, "boolean");
AssertType(value === '', "boolean");
AssertType(value, "union");
AssertType('', "string");
AssertType(value === null, "boolean");
AssertType(value, "union");
AssertType(null, "null");
AssertType(value === undefined, "boolean");
AssertType(value, "union");
AssertType(undefined, "undefined");
    return value === '' || value === null || value === undefined;
}

let test: string | null | undefined;
AssertType(test, "union");
AssertType(null, "null");

if (isEmpty(test)) {
    test;  // EmptyString
AssertType(test, "EmptyString");
}

// Repro from #43825

declare function assert<T>(value: any): asserts value is T

function test1(foo: number | string | boolean) {
    assert<1 | string>(foo);
AssertType(assert<1 | string>(foo), "void");
AssertType(assert, "<T>(any) => asserts value is T");
AssertType(foo, "union");

    foo;  // string | 1
AssertType(foo, "union");
}

// Repro from #46909

function check1(x: unknown): x is (string | 0) {
AssertType(typeof x === "string" || x === 0, "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "unknown");
AssertType("string", "string");
AssertType(x === 0, "boolean");
AssertType(x, "unknown");
AssertType(0, "int");
	return typeof x === "string" || x === 0;
}

function check2(x: unknown): x is ("hello" | 0) {
AssertType(x === "hello" || x === 0, "boolean");
AssertType(x === "hello", "boolean");
AssertType(x, "unknown");
AssertType("hello", "string");
AssertType(x === 0, "boolean");
AssertType(x, "unknown");
AssertType(0, "int");
	return x === "hello" || x === 0;
}

function test3(x: unknown) {
	if (typeof x === "string" || x === 0) {
AssertType(typeof x === "string" || x === 0, "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "unknown");
AssertType("string", "string");
AssertType(x === 0, "boolean");
AssertType(x, "unknown");
AssertType(0, "int");

		x;  // string | 0
AssertType(x, "union");

		if (x === "hello" || x === 0) {
AssertType(x === "hello" || x === 0, "boolean");
AssertType(x === "hello", "boolean");
AssertType(x, "union");
AssertType("hello", "string");
AssertType(x === 0, "boolean");
AssertType(x, "union");
AssertType(0, "int");

			x;  // 0 | "hello"
AssertType(x, "union");
		}
	}
	if (check1(x)) {
AssertType(check1(x), "boolean");
AssertType(check1, "(unknown) => x is union");
AssertType(x, "unknown");

		x;  // string | 0
AssertType(x, "union");

		if (check2(x)) {
AssertType(check2(x), "boolean");
AssertType(check2, "(unknown) => x is union");
AssertType(x, "union");

			x;  // 0 | "hello"
AssertType(x, "union");
		}
	}
}

// Repro from #49588

function assertRelationIsNullOrStringArray(v: (string | number)[] | null): asserts v is string[] | null {}

function f1x(obj: (string | number)[] | null) {
    assertRelationIsNullOrStringArray(obj);
AssertType(assertRelationIsNullOrStringArray(obj), "void");
AssertType(assertRelationIsNullOrStringArray, "(union) => asserts v is union");
AssertType(obj, "union");

    obj;  // string[] | null
AssertType(obj, "union");
}


