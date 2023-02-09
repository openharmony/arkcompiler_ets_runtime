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

// === tests/cases/conformance/interfaces/declarationMerging/mergeTwoInterfaces.ts ===
declare function AssertType(value:any, type:string):void;
// two interfaces with the same root module should merge

// basic case
interface A {
    foo: string;
}

interface A {
    bar: number;
}

let a: A;
AssertType(a, "A");

let r1 = a.foo
AssertType(r1, "string");
AssertType(a.foo, "string");

let r2 = a.bar;
AssertType(r2, "number");
AssertType(a.bar, "number");

// basic generic case
interface B<T> {
    baz: string;
    foo: T;
}

interface B<T> {
    bar: T;
}

let b: B<string>;
AssertType(b, "B<string>");

let r3 = b.foo
AssertType(r3, "string");
AssertType(b.foo, "string");

let r4 = b.bar;
AssertType(r4, "string");
AssertType(b.bar, "string");

// basic non-generic and generic case inside a module
module M {
    interface A {
        foo: string;
    }

    interface A {
        bar: number;
    }

    let a: A;
    let r1 = a.foo;
    // BUG 856491
    let r2 = a.bar; // any, should be number

    interface B<T> {
        foo: T;
    }

    interface B<T> {
        bar: T;
    }

    let b: B<string>;
    let r3 = b.foo
    // BUG 856491
    let r4 = b.bar; // any, should be string
}

