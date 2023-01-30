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

// === tests/cases/conformance/controlFlow/controlFlowForInStatement.ts ===
declare function AssertType(value:any, type:string):void;
let x: string | number | boolean | RegExp | Function;
AssertType(x, "union");

let obj: any;
AssertType(obj, "any");

let cond: boolean;
AssertType(cond, "boolean");

x = /a/;
AssertType(x = /a/, "RegExp");
AssertType(x, "union");
AssertType(/a/, "RegExp");

for (let y in obj) {
    x = y;
AssertType(x = y, "string");
AssertType(x, "union");
AssertType(y, "string");

    if (cond) {
AssertType(cond, "boolean");

        x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

        continue;
    }
    if (cond) {
AssertType(cond, "boolean");

        x = true;
AssertType(x = true, "boolean");
AssertType(x, "union");
AssertType(true, "boolean");

        break;
    }
}
x; // RegExp | string | number | boolean
AssertType(x, "union");


