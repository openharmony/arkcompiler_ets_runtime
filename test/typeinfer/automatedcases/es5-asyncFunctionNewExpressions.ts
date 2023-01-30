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

// === tests/cases/compiler/es5-asyncFunctionNewExpressions.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function newExpression0() {
    await new x(y, z);
AssertType(await new x(y, z), "any");
AssertType(new x(y, z), "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression1() {
    new (await x)(y, z);
AssertType(new (await x)(y, z), "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression2() {
    new x(await y, z);
AssertType(new x(await y, z), "any");
AssertType(x, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression3() {
    new x(y, await z);
AssertType(new x(y, await z), "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function newExpression4() {
    await new x(...y, z);
AssertType(await new x(...y, z), "any");
AssertType(new x(...y, z), "any");
AssertType(x, "any");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression5() {
    new (await x)(...y, z);
AssertType(new (await x)(...y, z), "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression6() {
    new x(...(await y), z);
AssertType(new x(...(await y), z), "any");
AssertType(x, "any");
AssertType(...(await y), "any");
AssertType((await y), "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression7() {
    new x(...y, await z);
AssertType(new x(...y, await z), "any");
AssertType(x, "any");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function newExpression8() {
    new x(await y, ...z);
AssertType(new x(await y, ...z), "any");
AssertType(x, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(...z, "any");
AssertType(z, "any");
}

async function newExpression9() {
    new x(y, ...(await z));
AssertType(new x(y, ...(await z)), "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(...(await z), "any");
AssertType((await z), "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function newExpression10() {
    await new x.a(y, z);
AssertType(await new x.a(y, z), "any");
AssertType(new x.a(y, z), "any");
AssertType(x.a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression11() {
    new (await x.a)(y, z);
AssertType(new (await x.a)(y, z), "any");
AssertType((await x.a), "any");
AssertType(await x.a, "any");
AssertType(x.a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression12() {
    new (await x).a(y, z);
AssertType(new (await x).a(y, z), "any");
AssertType((await x).a, "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression13() {
    new x.a(await y, z);
AssertType(new x.a(await y, z), "any");
AssertType(x.a, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression14() {
    new x.a(y, await z);
AssertType(new x.a(y, await z), "any");
AssertType(x.a, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function newExpression15() {
    await new x[a](y, z);
AssertType(await new x[a](y, z), "any");
AssertType(new x[a](y, z), "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression16() {
    new (await x[a])(y, z);
AssertType(new (await x[a])(y, z), "any");
AssertType((await x[a]), "any");
AssertType(await x[a], "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression17() {
    new (await x)[a](y, z);
AssertType(new (await x)[a](y, z), "any");
AssertType((await x)[a], "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression18() {
    new x[await a](y, z);
AssertType(new x[await a](y, z), "any");
AssertType(x[await a], "any");
AssertType(x, "any");
AssertType(await a, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression19() {
    new x[a](await y, z);
AssertType(new x[a](await y, z), "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function newExpression20() {
    new x[a](y, await z);
AssertType(new x[a](y, await z), "any");
AssertType(x[a], "any");
AssertType(x, "any");
AssertType(a, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

