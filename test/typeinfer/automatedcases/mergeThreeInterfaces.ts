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

// === tests/cases/conformance/interfaces/declarationMerging/mergeThreeInterfaces.ts ===
declare function AssertType(value:any, type:string):void;
// interfaces with the same root module should merge

// basic case
interface A {
    foo: string;
}

interface A {
    bar: number;
}

interface A {
    baz: boolean;
}

let a: A;
AssertType(a, "A");

let r1 = a.foo
AssertType(r1, "string");
AssertType(a.foo, "string");

let r2 = a.bar;
AssertType(r2, "number");
AssertType(a.bar, "number");

let r3 = a.baz;
AssertType(r3, "boolean");
AssertType(a.baz, "boolean");

// basic generic case
interface B<T> {
    foo: T;
}

interface B<T> {
    bar: T;
}

interface B<T> {
    baz: T;
}

let b: B<string>;
AssertType(b, "B<string>");

let r4 = b.foo
AssertType(r4, "string");
AssertType(b.foo, "string");

let r5 = b.bar;
AssertType(r5, "string");
AssertType(b.bar, "string");

let r6 = b.baz;
AssertType(r6, "string");
AssertType(b.baz, "string");

// basic non-generic and generic case inside a module
module M {
    interface A {
        foo: string;
    }

    interface A {
        bar: number;
    }

    interface A {
        baz: boolean;
    }

    let a: A;
    let r1 = a.foo;
    // BUG 856491
    let r2 = a.bar; // any, should be number
    // BUG 856491
    let r3 = a.baz; // any, should be boolean

    interface B<T> {
        foo: T;
    }

    interface B<T> {
        bar: T;
    }

    interface B<T> {
        baz: T;
    }

    let b: B<string>;
    let r4 = b.foo
    // BUG 856491
    let r5 = b.bar; // any, should be number
    // BUG 856491
    let r6 = b.baz; // any, should be boolean
}

