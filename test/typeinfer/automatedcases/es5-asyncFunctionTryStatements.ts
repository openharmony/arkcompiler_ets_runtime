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

// === tests/cases/compiler/es5-asyncFunctionTryStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x: any, y: any, z: any, a: any, b: any, c: any;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function tryCatch0() {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        x;
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        y;
AssertType(y, "any");
    }
}

async function tryCatch1() {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        await x;
AssertType(await x, "any");
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        y;
AssertType(y, "any");
    }
}

async function tryCatch2() {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        x;
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        await y;
AssertType(await y, "any");
AssertType(y, "any");
    }
}

async function tryCatch3(): Promise<Function> {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        await x;
AssertType(await x, "any");
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

AssertType(() => e, "() => any");
AssertType(e, "any");
        return () => e;
    }
}
async function tryFinally0() {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        x;
AssertType(x, "any");
    }
    finally {
        y;
AssertType(y, "any");
    }
}

async function tryFinally1() {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        await x;
AssertType(await x, "any");
AssertType(x, "any");
    }
    finally {
        y;
AssertType(y, "any");
    }
}

async function tryFinally2() {
    let x: any, y: any;
AssertType(x, "any");
AssertType(y, "any");

    try {
        x;
AssertType(x, "any");
    }
    finally {
        await y;
AssertType(await y, "any");
AssertType(y, "any");
    }
}

async function tryCatchFinally0() {
    let x: any, y: any, z: any;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");

    try {
        x;
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        y;
AssertType(y, "any");
    }
    finally {
        z;
AssertType(z, "any");
    }
}

async function tryCatchFinally1() {
    let x: any, y: any, z: any;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");

    try {
        await x;
AssertType(await x, "any");
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        y;
AssertType(y, "any");
    }
    finally {
        z;
AssertType(z, "any");
    }
}

async function tryCatchFinally2() {
    let x: any, y: any, z: any;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");

    try {
        x;
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        await y;
AssertType(await y, "any");
AssertType(y, "any");
    }
    finally {
        z;
AssertType(z, "any");
    }
}

async function tryCatchFinally3() {
    let x: any, y: any, z: any;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");

    try {
        x;
AssertType(x, "any");
    }
    catch (e) {
AssertType(e, "any");

        y;
AssertType(y, "any");
    }
    finally {
        await z;
AssertType(await z, "any");
AssertType(z, "any");
    }
}

