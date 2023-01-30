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

// === tests/cases/conformance/expressions/typeGuards/TypeGuardWithEnumUnion.ts ===
declare function AssertType(value:any, type:string):void;
enum Color { R, G, B }

function f1(x: Color | string) {
    if (typeof x === "number") {
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        let y = x;
AssertType(y, "Color");
AssertType(x, "Color");

        let y: Color;
AssertType(y, "Color");
    }
    else {
        let z = x;
AssertType(z, "string");
AssertType(x, "string");

        let z: string;
AssertType(z, "string");
    }
}

function f2(x: Color | string | string[]) {
    if (typeof x === "object") {
AssertType(typeof x === "object", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("object", "string");

        let y = x;
AssertType(y, "string[]");
AssertType(x, "string[]");

        let y: string[];
AssertType(y, "string[]");
    }
    if (typeof x === "number") {
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        let z = x;
AssertType(z, "Color");
AssertType(x, "Color");

        let z: Color;
AssertType(z, "Color");
    }
    else {
        let w = x;
AssertType(w, "union");
AssertType(x, "union");

        let w: string | string[];
AssertType(w, "union");
    }
    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        let a = x;
AssertType(a, "string");
AssertType(x, "string");

        let a: string;
AssertType(a, "string");
    }
    else {
        let b = x;
AssertType(b, "union");
AssertType(x, "union");

        let b: Color | string[];
AssertType(b, "union");
    }
}


