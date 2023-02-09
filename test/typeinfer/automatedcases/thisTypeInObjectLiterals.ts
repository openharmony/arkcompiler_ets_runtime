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

// === tests/cases/conformance/types/thisType/thisTypeInObjectLiterals.ts ===
declare function AssertType(value:any, type:string):void;
let o = {
AssertType(o, "{ d: string; m(): number; f: () => number; }");
AssertType({    d: "bar",    m() {        return this.d.length;    },    f: function() {        return this.d.length;    }}, "{ d: string; m(): number; f: () => number; }");

    d: "bar",
AssertType(d, "string");
AssertType("bar", "string");

    m() {
AssertType(m, "() => number");

AssertType(this.d.length, "number");
AssertType(this.d, "string");
AssertType(this, "{ d: string; m(): number; f: () => number; }");
        return this.d.length;

    },
    f: function() {
AssertType(f, "() => number");
AssertType(function() {        return this.d.length;    }, "() => number");

AssertType(this.d.length, "number");
AssertType(this.d, "string");
AssertType(this, "{ d: string; m(): number; f: () => number; }");
        return this.d.length;
    }
}

let mutuallyRecursive = {
AssertType(mutuallyRecursive, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");
AssertType({    a: 100,    start() {        return this.passthrough(this.a);    },    passthrough(n: number) {        return this.sub1(n);    },    sub1(n: number): number {        if (n > 0) {            return this.passthrough(n - 1);        }        return n;    }}, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");

    a: 100,
AssertType(a, "number");
AssertType(100, "int");

    start() {
AssertType(start, "() => number");

AssertType(this.passthrough(this.a), "number");
AssertType(this.passthrough, "(number) => number");
AssertType(this, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");
AssertType(this.a, "number");
AssertType(this, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");
        return this.passthrough(this.a);

    },
    passthrough(n: number) {
AssertType(passthrough, "(number) => number");
AssertType(n, "number");

AssertType(this.sub1(n), "number");
AssertType(this.sub1, "(number) => number");
AssertType(this, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");
AssertType(n, "number");
        return this.sub1(n);

    },
    sub1(n: number): number {
AssertType(sub1, "(number) => number");
AssertType(n, "number");

        if (n > 0) {
AssertType(n > 0, "boolean");
AssertType(n, "number");
AssertType(0, "int");

AssertType(this.passthrough(n - 1), "number");
AssertType(this.passthrough, "(number) => number");
AssertType(this, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");
AssertType(n - 1, "number");
AssertType(n, "number");
AssertType(1, "int");
            return this.passthrough(n - 1);
        }
AssertType(n, "number");
        return n;
    }
}
let i: number = mutuallyRecursive.start();
AssertType(i, "number");
AssertType(mutuallyRecursive.start(), "number");
AssertType(mutuallyRecursive.start, "() => number");

interface I {
    a: number;
    start(): number;
    passthrough(n: number): number;
    sub1(n: number): number;
}
let impl: I = mutuallyRecursive;
AssertType(impl, "I");
AssertType(mutuallyRecursive, "{ a: number; start(): number; passthrough(number): number; sub1(number): number; }");


