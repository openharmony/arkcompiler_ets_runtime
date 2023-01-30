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

// === tests/cases/conformance/controlFlow/typeGuardsTypeParameters.ts ===
declare function AssertType(value:any, type:string):void;
// Type guards involving type parameters produce intersection types

class C {
    prop: string;
}

function f1<T>(x: T) {
    if (x instanceof C) {
AssertType(x instanceof C, "boolean");
AssertType(x, "T");
AssertType(C, "typeof C");

        let v1: T = x;
AssertType(v1, "T");
AssertType(x, "T & C");

        let v2: C = x;
AssertType(v2, "C");
AssertType(x, "T & C");

        x.prop;
AssertType(x.prop, "string");
    }
}

function f2<T>(x: T) {
    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "T");
AssertType("string", "string");

        let v1: T = x;
AssertType(v1, "T");
AssertType(x, "T & string");

        let v2: string = x;
AssertType(v2, "string");
AssertType(x, "T & string");

        x.length;
AssertType(x.length, "number");
    }
}

// Repro from #13872

function fun<T>(item: { [P in keyof T]: T[P] }) {
    const strings: string[] = [];
AssertType(strings, "string[]");
AssertType([], "never[]");

    for (const key in item) {
AssertType(key, "Extract<keyof T, string>");
AssertType(item, "{ [P in keyof T]: T[P]; }");

        const value = item[key];
AssertType(value, "{ [P in keyof T]: T[P]; }[Extract<keyof T, string>]");
AssertType(item[key], "{ [P in keyof T]: T[P]; }[Extract<keyof T, string>]");
AssertType(item, "{ [P in keyof T]: T[P]; }");
AssertType(key, "Extract<keyof T, string>");

        if (typeof value === "string") {
AssertType(typeof value === "string", "boolean");
AssertType(typeof value, "union");
AssertType(value, "{ [P in keyof T]: T[P]; }[Extract<keyof T, string>]");
AssertType("string", "string");

            strings.push(value);
AssertType(strings.push(value), "number");
AssertType(strings.push, "(...string[]) => number");
AssertType(value, "{ [P in keyof T]: T[P]; }[Extract<keyof T, string>] & string");
        }
    }
}


