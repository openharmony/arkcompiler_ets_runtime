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

// === tests/cases/conformance/controlFlow/typeGuardsAsAssertions.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #8513

let cond: boolean;
AssertType(cond, "boolean");

export type Optional<a> = Some<a> | None;

export interface None { readonly none: string; }
export interface Some<a> { readonly some: a; }

export const none : None = { none: '' };
AssertType(none, "None");
AssertType({ none: '' }, "{ none: string; }");
AssertType(none, "string");
AssertType('', "string");

export function isSome<a>(value: Optional<a>): value is Some<a> {
AssertType('some' in value, "boolean");
AssertType('some', "string");
AssertType(value, "Optional<a>");
    return 'some' in value;
}

function someFrom<a>(some: a) {
AssertType({ some }, "{ some: a; }");
AssertType(some, "a");
    return { some };
}

export function fn<r>(makeSome: () => r): void {
    let result: Optional<r> = none;
AssertType(result, "Optional<r>");
AssertType(none, "None");

    result;  // None
AssertType(result, "None");

    while (cond) {
AssertType(cond, "boolean");

        result;  // Some<r> | None
AssertType(result, "Optional<r>");

        result = someFrom(isSome(result) ? result.some : makeSome());
AssertType(result = someFrom(isSome(result) ? result.some : makeSome()), "{ some: r; }");
AssertType(result, "Optional<r>");
AssertType(someFrom(isSome(result) ? result.some : makeSome()), "{ some: r; }");
AssertType(someFrom, "<a>(a) => { some: a; }");
AssertType(isSome(result) ? result.some : makeSome(), "r");
AssertType(isSome(result), "boolean");
AssertType(isSome, "<a>(Optional<a>) => value is Some<a>");
AssertType(result, "Optional<r>");
AssertType(result.some, "r");
AssertType(makeSome(), "r");
AssertType(makeSome, "() => r");

        result;  // Some<r>
AssertType(result, "Some<r>");
    }
}

function foo1() {
    let x: string | number | boolean = 0;
AssertType(x, "union");
AssertType(0, "int");

    x;  // number
AssertType(x, "number");

    while (cond) {
AssertType(cond, "boolean");

        x;  // number, then string | number
AssertType(x, "union");

        x = typeof x === "string" ? x.slice() : "abc";
AssertType(x = typeof x === "string" ? x.slice() : "abc", "string");
AssertType(x, "union");
AssertType(typeof x === "string" ? x.slice() : "abc", "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(x.slice(), "string");
AssertType(x.slice, "(?union, ?union) => string");
AssertType("abc", "string");

        x;  // string
AssertType(x, "string");
    }
    x;
AssertType(x, "union");
}

function foo2() {
    let x: string | number | boolean = 0;
AssertType(x, "union");
AssertType(0, "int");

    x;  // number
AssertType(x, "number");

    while (cond) {
AssertType(cond, "boolean");

        x;  // number, then string | number
AssertType(x, "union");

        if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

            x = x.slice();
AssertType(x = x.slice(), "string");
AssertType(x, "union");
AssertType(x.slice(), "string");
AssertType(x.slice, "(?union, ?union) => string");
        }
        else {
            x = "abc";
AssertType(x = "abc", "string");
AssertType(x, "union");
AssertType("abc", "string");
        }
        x;  // string
AssertType(x, "string");
    }
    x;
AssertType(x, "union");
}

// Type guards as assertions

function f1() {
    let x: string | number | undefined = undefined;
AssertType(x, "union");
AssertType(undefined, "undefined");

    x;  // undefined
AssertType(x, "undefined");

    if (x) {
AssertType(x, "undefined");

        x;  // string | number (guard as assertion)
AssertType(x, "never");
    }
    x;  // string | number | undefined
AssertType(x, "undefined");
}

function f2() {
    let x: string | number | undefined = undefined;
AssertType(x, "union");
AssertType(undefined, "undefined");

    x;  // undefined
AssertType(x, "undefined");

    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "undefined");
AssertType("string", "string");

        x;  // string (guard as assertion)
AssertType(x, "never");
    }
    x;  // string | undefined
AssertType(x, "undefined");
}

function f3() {
    let x: string | number | undefined = undefined;
AssertType(x, "union");
AssertType(undefined, "undefined");

    x;  // undefined
AssertType(x, "undefined");

    if (!x) {
AssertType(!x, "boolean");
AssertType(x, "undefined");

        return;
    }
    x;  // string | number (guard as assertion)
AssertType(x, "never");
}

function f4() {
    let x: string | number | undefined = undefined;
AssertType(x, "union");
AssertType(undefined, "undefined");

    x;  // undefined
AssertType(x, "undefined");

    if (typeof x === "boolean") {
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "undefined");
AssertType("boolean", "string");

        x;  // nothing (boolean not in declared type)
AssertType(x, "never");
    }
    x;  // undefined
AssertType(x, "undefined");
}

function f5(x: string | number) {
    if (typeof x === "string" && typeof x === "number") {
AssertType(typeof x === "string" && typeof x === "number", "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "string");
AssertType("number", "string");

        x;  // number (guard as assertion)
AssertType(x, "never");
    }
    else {
        x;  // string | number
AssertType(x, "union");
    }
    x;  // string | number
AssertType(x, "union");
}

function f6() {
    let x: string | undefined | null;
AssertType(x, "union");
AssertType(null, "null");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "string");

    x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "union");

    x = null;
AssertType(x = null, "null");
AssertType(x, "union");
AssertType(null, "null");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "union");

    x = <undefined | null>undefined;
AssertType(x = <undefined | null>undefined, "union");
AssertType(x, "union");
AssertType(<undefined | null>undefined, "union");
AssertType(null, "null");
AssertType(undefined, "undefined");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "union");

    x = <string | undefined>"";
AssertType(x = <string | undefined>"", "union");
AssertType(x, "union");
AssertType(<string | undefined>"", "union");
AssertType("", "string");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "union");

    x = <string | null>"";
AssertType(x = <string | null>"", "union");
AssertType(x, "union");
AssertType(<string | null>"", "union");
AssertType(null, "null");
AssertType("", "string");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "union");
}

function f7() {
    let x: string;
AssertType(x, "string");

    x!.slice();
AssertType(x!.slice(), "string");
AssertType(x!.slice, "(?union, ?union) => string");
AssertType(x!, "string");
AssertType(x, "string");
}


