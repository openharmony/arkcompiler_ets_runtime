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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInWhileStatement.ts ===
declare function AssertType(value:any, type:string):void;
let cond: boolean;
AssertType(cond, "boolean");

function a(x: string | number) {
    while (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        x; // string
AssertType(x, "string");

        x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");
    }
    x; // number
AssertType(x, "number");
}
function b(x: string | number) {
    while (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        if (cond) continue;
AssertType(cond, "boolean");

        x; // string
AssertType(x, "string");

        x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");
    }
    x; // number
AssertType(x, "number");
}
function c(x: string | number) {
    while (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        if (cond) break;
AssertType(cond, "boolean");

        x; // string
AssertType(x, "string");

        x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");
    }
    x; // string | number
AssertType(x, "union");
}


