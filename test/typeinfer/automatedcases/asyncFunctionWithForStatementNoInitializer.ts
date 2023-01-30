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

// === tests/cases/compiler/asyncFunctionWithForStatementNoInitializer.ts ===
declare function AssertType(value:any, type:string):void;
async function test1() {
    let i = 0
AssertType(i, "number");
AssertType(0, "int");

    let limit = 10
AssertType(limit, "number");
AssertType(10, "int");

    for (; i < limit; ++i) {
AssertType(i < limit, "boolean");
AssertType(i, "number");
AssertType(limit, "number");
AssertType(++i, "number");
AssertType(i, "number");
    }
}

async function test2() {
    let i = 0
AssertType(i, "number");
AssertType(0, "int");

    let limit = 10
AssertType(limit, "number");
AssertType(10, "int");

    for (i = 1; i < limit; ++i) {
AssertType(i = 1, "int");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i < limit, "boolean");
AssertType(i, "number");
AssertType(limit, "number");
AssertType(++i, "number");
AssertType(i, "number");
    }
}

async function test3() {
    let i = 0
AssertType(i, "number");
AssertType(0, "int");

    for (;; ++i) {
AssertType(++i, "number");
AssertType(i, "number");
    }
}

async function test4() {
    for (;;) {
    }
}

