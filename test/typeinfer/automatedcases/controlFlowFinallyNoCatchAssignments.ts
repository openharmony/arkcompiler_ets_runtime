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

// === tests/cases/compiler/controlFlowFinallyNoCatchAssignments.ts ===
declare function AssertType(value:any, type:string):void;
let x: number;
AssertType(x, "number");

x = Math.random();
AssertType(x = Math.random(), "number");
AssertType(x, "number");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");

let a: number;
AssertType(a, "number");

try {
    if (x) {
AssertType(x, "number");

        a = 1;
AssertType(a = 1, "int");
AssertType(a, "number");
AssertType(1, "int");

    } else {
        a = 2;
AssertType(a = 2, "int");
AssertType(a, "number");
AssertType(2, "int");
    }
} finally {
    console.log(x);
AssertType(console.log(x), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(x, "number");
}

console.log(a); // <- error here
AssertType(console.log(a), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(a, "number");


