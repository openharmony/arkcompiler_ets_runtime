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

// === tests/cases/compiler/es5-asyncFunctionCallExpressions.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function callExpression0() {
    await x(y, z);
AssertType(await x(y, z), "any");
AssertType(x(y, z), "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression1() {
    (await x)(y, z);
AssertType((await x)(y, z), "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression2() {
    x(await y, z);
AssertType(x(await y, z), "any");
AssertType(x, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression3() {
    x(y, await z);
AssertType(x(y, await z), "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function callExpression4() {
    await x(...y, z);
AssertType(await x(...y, z), "any");
AssertType(x(...y, z), "any");
AssertType(x, "any");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression5() {
    (await x)(...y, z);
AssertType((await x)(...y, z), "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression6() {
    x(...(await y), z);
AssertType(x(...(await y), z), "any");
AssertType(x, "any");
AssertType(...(await y), "any");
AssertType((await y), "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression7() {
    x(...y, await z);
AssertType(x(...y, await z), "any");
AssertType(x, "any");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function callExpression8() {
    x(await y, ...z);
AssertType(x(await y, ...z), "any");
AssertType(x, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(...z, "any");
AssertType(z, "any");
}

async function callExpression9() {
    x(y, ...(await z));
AssertType(x(y, ...(await z)), "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(...(await z), "any");
AssertType((await z), "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function callExpression10() {
    await x.a(y, z);
AssertType(await x.a(y, z), "any");
AssertType(x.a(y, z), "any");
AssertType(x.a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression11() {
    (await x.a)(y, z);
AssertType((await x.a)(y, z), "any");
AssertType((await x.a), "any");
AssertType(await x.a, "any");
AssertType(x.a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression12() {
    (await x).a(y, z);
AssertType((await x).a(y, z), "any");
AssertType((await x).a, "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression13() {
    x.a(await y, z);
AssertType(x.a(await y, z), "any");
AssertType(x.a, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression14() {
    x.a(y, await z);
AssertType(x.a(y, await z), "any");
AssertType(x.a, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function callExpression15() {
    await x[a](y, z);
AssertType(await x[a](y, z), "any");
AssertType(x[a](y, z), "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression16() {
    (await x[a])(y, z);
AssertType((await x[a])(y, z), "any");
AssertType((await x[a]), "any");
AssertType(await x[a], "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression17() {
    (await x)[a](y, z);
AssertType((await x)[a](y, z), "any");
AssertType((await x)[a], "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression18() {
    x[await a](y, z);
AssertType(x[await a](y, z), "any");
AssertType(x[await a], "any");
AssertType(x, "any");
AssertType(await a, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression19() {
    x[a](await y, z);
AssertType(x[a](await y, z), "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function callExpression20() {
    x[a](y, await z);
AssertType(x[a](y, await z), "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}


