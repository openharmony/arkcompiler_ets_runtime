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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormTypeOfFunction.ts ===
declare function AssertType(value:any, type:string):void;
function f1(x: any) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "any");
AssertType("function", "string");

        x;  // any
AssertType(x, "any");
    }
}

function f2(x: unknown) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "unknown");
AssertType("function", "string");

        x;  // Function
AssertType(x, "Function");
    }
}

function f3(x: {}) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "{}");
AssertType("function", "string");

        x;  // Function
AssertType(x, "Function");
    }
}

function f4<T>(x: T) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "T");
AssertType("function", "string");

        x;  // T & Function
AssertType(x, "T & Function");
    }
}

function f5(x: { s: string }) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "{ s: string; }");
AssertType("function", "string");

        x;  // never
AssertType(x, "never");
    }
}

function f6(x: () => string) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "() => string");
AssertType("function", "string");

        x;  // () => string
AssertType(x, "() => string");
    }
}

function f10(x: string | (() => string)) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("function", "string");

        x;  // () => string
AssertType(x, "() => string");
    }
    else {
        x;  // string
AssertType(x, "string");
    }
}

function f11(x: { s: string } | (() => string)) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("function", "string");

        x;  // () => string
AssertType(x, "() => string");
    }
    else {
        x;  // { s: string 
AssertType(x, "{ s: string; }");
}
    }
}

function f12(x: { s: string } | { n: number }) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("function", "string");

        x;  // never
AssertType(x, "never");
    }
    else {
        x;  // { s: string } | { n: number 
AssertType(x, "union");
}
    }
}

// Repro from #18238

function f100<T, K extends keyof T>(obj: T, keys: K[]) : void {
    for (const k of keys) {
AssertType(k, "K");
AssertType(keys, "K[]");

        const item = obj[k];
AssertType(item, "T[K]");
AssertType(obj[k], "T[K]");
AssertType(obj, "T");
AssertType(k, "K");

        if (typeof item == 'function')
AssertType(typeof item == 'function', "boolean");
AssertType(typeof item, "union");
AssertType(item, "T[K]");
AssertType('function', "string");

            item.call(obj);
AssertType(item.call(obj), "any");
AssertType(item.call, "(Function, any, ...any[]) => any");
AssertType(obj, "T");
    }
}

// Repro from #49316

function configureStore<S extends object>(reducer: (() => void) | Record<keyof S, () => void>) {
    let rootReducer: () => void;
AssertType(rootReducer, "() => void");

    if (typeof reducer === 'function') {
AssertType(typeof reducer === 'function', "boolean");
AssertType(typeof reducer, "union");
AssertType(reducer, "union");
AssertType('function', "string");

        rootReducer = reducer;
AssertType(rootReducer = reducer, "() => void");
AssertType(rootReducer, "() => void");
AssertType(reducer, "() => void");
    }
}

function f101(x: string | Record<string, any>) {
AssertType(typeof x === "object" && x.anything, "any");
AssertType(typeof x === "object", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("object", "string");
AssertType(x.anything, "any");
    return typeof x === "object" && x.anything;
}


