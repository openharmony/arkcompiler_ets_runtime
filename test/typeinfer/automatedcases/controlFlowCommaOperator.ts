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

// === tests/cases/conformance/controlFlow/controlFlowCommaOperator.ts ===
declare function AssertType(value:any, type:string):void;
function f(x: string | number | boolean) {
    let y: string | number | boolean = false;
AssertType(y, "union");
AssertType(false, "boolean");

    let z: string | number | boolean = false;
AssertType(z, "union");
AssertType(false, "boolean");

    if (y = "", typeof x === "string") {
AssertType(y = "", typeof x === "string", "boolean");
AssertType(y = "", "string");
AssertType(y, "union");
AssertType("", "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        x; // string
AssertType(x, "string");

        y; // string
AssertType(y, "string");

        z; // boolean
AssertType(z, "boolean");
    }
    else if (z = 1, typeof x === "number") {
AssertType(z = 1, typeof x === "number", "boolean");
AssertType(z = 1, "int");
AssertType(z, "union");
AssertType(1, "int");
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        x; // number
AssertType(x, "number");

        y; // string
AssertType(y, "string");

        z; // number
AssertType(z, "number");
    }
    else {
        x; // boolean
AssertType(x, "boolean");

        y; // string
AssertType(y, "string");

        z; // number
AssertType(z, "number");
    }
    x; // string | number | boolean
AssertType(x, "union");

    y; // string
AssertType(y, "string");

    z; // number | boolean
AssertType(z, "union");
}


