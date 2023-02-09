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

// === tests/cases/conformance/types/mapped/mappedTypes4.ts ===
declare function AssertType(value:any, type:string):void;
type Box<T> = {
};

type Boxified<T> = {
    [P in keyof T]: Box<T[P]>;
};

function boxify<T>(obj: T): Boxified<T> {
    if (typeof obj === "object") {
AssertType(typeof obj === "object", "boolean");
AssertType(typeof obj, "union");
AssertType(obj, "T");
AssertType("object", "string");

        let result = {} as Boxified<T>;
AssertType(result, "Boxified<T>");
AssertType({} as Boxified<T>, "Boxified<T>");
AssertType({}, "{}");

        for (let k in obj) {
AssertType(k, "Extract<keyof T, string>");
AssertType(obj, "union");

            result[k] = { value: obj[k] };
AssertType(result[k] = { value: obj[k] }, "{ value: (T & object)[Extract<keyof T, string>]; }");
AssertType(result[k], "Boxified<T>[Extract<keyof T, string>]");
AssertType(result, "Boxified<T>");
AssertType(k, "Extract<keyof T, string>");
AssertType({ value: obj[k] }, "{ value: (T & object)[Extract<keyof T, string>]; }");
AssertType(value, "(T & object)[Extract<keyof T, string>]");
AssertType(obj[k], "(T & object)[Extract<keyof T, string>]");
AssertType(obj, "T & object");
AssertType(k, "Extract<keyof T, string>");
        }
AssertType(result, "Boxified<T>");
        return result;
    }
AssertType(<any>obj, "any");
AssertType(obj, "T");
    return <any>obj;
}

type A = { a: string };
type B = { b: string };
type C = { c: string };

function f1(x: A | B | C | undefined) {
AssertType(boxify(x), "Boxified<union>");
AssertType(boxify, "<T>(T) => Boxified<T>");
AssertType(x, "union");
    return boxify(x);
}

type T00 = Partial<A | B | C>;
type T01 = Readonly<A | B | C | null | undefined>;
type T02 = Boxified<A | B[] | C | string>
type T03 = Readonly<string | number | boolean | null | undefined | void>;
type T04 = Boxified<string | number | boolean | null | undefined | void>;
type T05 = Partial<"hello" | "world" | 42>;

type BoxifiedWithSentinel<T, U> = {
    [P in keyof T]: Box<T[P]> | U;
}

type T10 = BoxifiedWithSentinel<A | B | C, null>;
type T11 = BoxifiedWithSentinel<A | B | C, undefined>;
type T12 = BoxifiedWithSentinel<string, undefined>;

type DeepReadonly<T> = {
    readonly [P in keyof T]: DeepReadonly<T[P]>;
};

type Foo = {
    x: number;
    y: { a: string, b: number };
    z: boolean;
};

type DeepReadonlyFoo = {
    readonly x: number;
    readonly y: { readonly a: string, readonly b: number };
    readonly z: boolean;
};

let x1: DeepReadonly<Foo>;
AssertType(x1, "DeepReadonly<Foo>");

let x1: DeepReadonlyFoo;
AssertType(x1, "DeepReadonly<Foo>");

// Repro from #13232

type Z = { a: number };
type Clone<T> = {
  [P in keyof (T & {})]: (T & {})[P];
};
type M = Clone<Z>; // M should be { a: number }

let z1: Z;
AssertType(z1, "Z");

let z1: Clone<Z>;
AssertType(z1, "Z");


