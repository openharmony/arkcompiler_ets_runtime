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

// === tests/cases/compiler/es5-asyncFunctionSwitchStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function switchStatement0() {
    switch (x) {
AssertType(x, "any");

        case y: a; break;
AssertType(y, "any");
AssertType(a, "any");

        default: b; break;
AssertType(b, "any");
    }
}

async function switchStatement1() {
    switch (await x) {
AssertType(await x, "any");
AssertType(x, "any");

        case y: a; break;
AssertType(y, "any");
AssertType(a, "any");

        default: b; break;
AssertType(b, "any");
    }
}

async function switchStatement2() {
    switch (x) {
AssertType(x, "any");

        case await y: a; break;
AssertType(await y, "any");
AssertType(y, "any");
AssertType(a, "any");

        default: b; break;
AssertType(b, "any");
    }
}

async function switchStatement3() {
    switch (x) {
AssertType(x, "any");

        case y: await a; break;
AssertType(y, "any");
AssertType(await a, "any");
AssertType(a, "any");

        default: b; break;
AssertType(b, "any");
    }
}

async function switchStatement4() {
    switch (x) {
AssertType(x, "any");

        case y: a; break;
AssertType(y, "any");
AssertType(a, "any");

        default: await b; break;
AssertType(await b, "any");
AssertType(b, "any");
    }
}

async function switchStatement5() {
    switch (x) {
AssertType(x, "any");

        case y: a; break;
AssertType(y, "any");
AssertType(a, "any");

        case await z: b; break;
AssertType(await z, "any");
AssertType(z, "any");
AssertType(b, "any");
    }
}

async function switchStatement6() {
    switch (x) {
AssertType(x, "any");

        default: c; break;
AssertType(c, "any");

        case await y: a; break;
AssertType(await y, "any");
AssertType(y, "any");
AssertType(a, "any");

        case z: b; break;
AssertType(z, "any");
AssertType(b, "any");
    }
}

async function switchStatement7() {
    switch (x) {
AssertType(x, "any");

        default: c; break;
AssertType(c, "any");

        case y: a; break;
AssertType(y, "any");
AssertType(a, "any");

        case await z: b; break;
AssertType(await z, "any");
AssertType(z, "any");
AssertType(b, "any");
    }
}

async function switchStatement8() {
    switch (x) {
AssertType(x, "any");

        default: c;
AssertType(c, "any");

        case y: a; break;
AssertType(y, "any");
AssertType(a, "any");

        case await z: b; break;
AssertType(await z, "any");
AssertType(z, "any");
AssertType(b, "any");
    }
}

