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

// === tests/cases/compiler/es5-asyncFunctionHoisting.ts ===
declare function AssertType(value:any, type:string):void;
declare let y;
AssertType(y, "any");

async function hoisting() {
    let a0, a1 = 1;
AssertType(a0, "any");
AssertType(a1, "number");
AssertType(1, "int");

    function z() {
AssertType(z, "() => void");

        let b0, b1 = 1;
AssertType(b0, "any");
AssertType(b1, "number");
AssertType(1, "int");
    }

    if (true) {
AssertType(true, "boolean");

        let c0, c1 = 1;
AssertType(c0, "any");
AssertType(c1, "number");
AssertType(1, "int");
    }

    for (let a = 0; y;) {
AssertType(a, "number");
AssertType(0, "int");
AssertType(y, "any");

    }

    for (let b in y) {
AssertType(b, "string");
AssertType(y, "any");

    }

    for (let c of y) {
AssertType(c, "any");
AssertType(y, "any");

    }
}

async function hoistingWithAwait() {
    let a0, a1 = 1;
AssertType(a0, "any");
AssertType(a1, "number");
AssertType(1, "int");

    function z() {
AssertType(z, "() => void");

        let b0, b1 = 1;
AssertType(b0, "any");
AssertType(b1, "number");
AssertType(1, "int");
    }

    await 0;
AssertType(await 0, "int");
AssertType(0, "int");

    if (true) {
AssertType(true, "boolean");

        let c0, c1 = 1;
AssertType(c0, "any");
AssertType(c1, "number");
AssertType(1, "int");
    }

    for (let a = 0; y;) {
AssertType(a, "number");
AssertType(0, "int");
AssertType(y, "any");

    }

    for (let b in y) {
AssertType(b, "string");
AssertType(y, "any");

    }

    for (let c of y) {
AssertType(c, "any");
AssertType(y, "any");

    }
}



