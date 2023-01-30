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

// === tests/cases/conformance/controlFlow/controlFlowTruthiness.ts ===
declare function AssertType(value:any, type:string):void;
declare function foo(): string | undefined;

function f1() {
    let x = foo();
AssertType(x, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");

    if (x) {
AssertType(x, "union");

        x; // string
AssertType(x, "string");
    }
    else {
        x; // string | undefined
AssertType(x, "union");
    }
}

function f2() {
    let x: string | undefined;
AssertType(x, "union");

    x = foo();
AssertType(x = foo(), "union");
AssertType(x, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");

    if (x) {
AssertType(x, "union");

        x; // string
AssertType(x, "string");
    }
    else {
        x; // string | undefined
AssertType(x, "union");
    }
}

function f3() {
    let x: string | undefined;
AssertType(x, "union");

    if (x = foo()) {
AssertType(x = foo(), "union");
AssertType(x, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");

        x; // string
AssertType(x, "string");
    }
    else {
        x; // string | undefined
AssertType(x, "union");
    }
}

function f4() {
    let x: string | undefined;
AssertType(x, "union");

    if (!(x = foo())) {
AssertType(!(x = foo()), "boolean");
AssertType((x = foo()), "union");
AssertType(x = foo(), "union");
AssertType(x, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");

        x; // string | undefined
AssertType(x, "union");
    }
    else {
        x; // string
AssertType(x, "string");
    }
}

function f5() {
    let x: string | undefined;
AssertType(x, "union");

    let y: string | undefined;
AssertType(y, "union");

    if (x = y = foo()) {
AssertType(x = y = foo(), "union");
AssertType(x, "union");
AssertType(y = foo(), "union");
AssertType(y, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");

        x; // string
AssertType(x, "string");

        y; // string | undefined
AssertType(y, "string");
    }
    else {
        x; // string | undefined
AssertType(x, "union");

        y; // string | undefined
AssertType(y, "union");
    }
}

function f6() {
    let x: string | undefined;
AssertType(x, "union");

    let y: string | undefined;
AssertType(y, "union");

    if (x = foo(), y = foo()) {
AssertType(x = foo(), y = foo(), "union");
AssertType(x = foo(), "union");
AssertType(x, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");
AssertType(y = foo(), "union");
AssertType(y, "union");
AssertType(foo(), "union");
AssertType(foo, "() => union");

        x; // string | undefined
AssertType(x, "union");

        y; // string
AssertType(y, "string");
    }
    else {
        x; // string | undefined
AssertType(x, "union");

        y; // string | undefined
AssertType(y, "union");
    }
}

function f7(x: {}) {
    if (x) {
AssertType(x, "{}");

        x; // {
AssertType(x, "{}");
}
    }
    else {
        x; // {
AssertType(x, "{}");
}
    }
}

function f8<T>(x: T) {
    if (x) {
AssertType(x, "T");

        x; // {
AssertType(x, "NonNullable<T>");
}
    }
    else {
        x; // {
AssertType(x, "T");
}
    }
}

function f9<T extends object>(x: T) {
    if (x) {
AssertType(x, "T");

        x; // {
AssertType(x, "T");
}
    }
    else {
        x; // never
AssertType(x, "never");
    }
}

