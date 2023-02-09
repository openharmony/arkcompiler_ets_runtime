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

// === tests/cases/compiler/asyncAwaitWithCapturedBlockScopeVar.ts ===
declare function AssertType(value:any, type:string):void;
async function fn1() {
    let ar = [];
AssertType(ar, "any[]");
AssertType([], "undefined[]");

    for (let i = 0; i < 1; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 1, "boolean");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i++, "number");
AssertType(i, "number");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

        ar.push(() => i);
AssertType(ar.push(() => i), "number");
AssertType(ar.push, "(...any[]) => number");
AssertType(() => i, "() => number");
AssertType(i, "number");
    }
}

async function fn2() {
    let ar = [];
AssertType(ar, "any[]");
AssertType([], "undefined[]");

    for (let i = 0; i < 1; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 1, "boolean");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i++, "number");
AssertType(i, "number");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

        ar.push(() => i);
AssertType(ar.push(() => i), "number");
AssertType(ar.push, "(...any[]) => number");
AssertType(() => i, "() => number");
AssertType(i, "number");

        break;
    }
}

async function fn3() {
    let ar = [];
AssertType(ar, "any[]");
AssertType([], "undefined[]");

    for (let i = 0; i < 1; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 1, "boolean");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i++, "number");
AssertType(i, "number");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

        ar.push(() => i);
AssertType(ar.push(() => i), "number");
AssertType(ar.push, "(...any[]) => number");
AssertType(() => i, "() => number");
AssertType(i, "number");

        continue;
    }
}

async function fn4(): Promise<number> {
    let ar = [];
AssertType(ar, "any[]");
AssertType([], "undefined[]");

    for (let i = 0; i < 1; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 1, "boolean");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i++, "number");
AssertType(i, "number");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

        ar.push(() => i);
AssertType(ar.push(() => i), "number");
AssertType(ar.push, "(...any[]) => number");
AssertType(() => i, "() => number");
AssertType(i, "number");

AssertType(1, "int");
        return 1;
    }
}


