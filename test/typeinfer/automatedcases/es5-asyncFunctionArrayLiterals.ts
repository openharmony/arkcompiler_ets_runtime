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

// === tests/cases/compiler/es5-asyncFunctionArrayLiterals.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");

async function arrayLiteral0() {
    x = [await y, z];
AssertType(x = [await y, z], "any[]");
AssertType(x, "any");
AssertType([await y, z], "any[]");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function arrayLiteral1() {
    x = [y, await z];
AssertType(x = [y, await z], "any[]");
AssertType(x, "any");
AssertType([y, await z], "any[]");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function arrayLiteral2() {
    x = [...(await y), z];
AssertType(x = [...(await y), z], "any[]");
AssertType(x, "any");
AssertType([...(await y), z], "any[]");
AssertType(...(await y), "any");
AssertType((await y), "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function arrayLiteral3() {
    x = [...y, await z];
AssertType(x = [...y, await z], "any[]");
AssertType(x, "any");
AssertType([...y, await z], "any[]");
AssertType(...y, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function arrayLiteral4() {
    x = [await y, ...z];
AssertType(x = [await y, ...z], "any[]");
AssertType(x, "any");
AssertType([await y, ...z], "any[]");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(...z, "any");
AssertType(z, "any");
}

async function arrayLiteral5() {
    x = [y, ...(await z)];
AssertType(x = [y, ...(await z)], "any[]");
AssertType(x, "any");
AssertType([y, ...(await z)], "any[]");
AssertType(y, "any");
AssertType(...(await z), "any");
AssertType((await z), "any");
AssertType(await z, "any");
AssertType(z, "any");
}

async function arrayLiteral6() {
    x = [y, await z, a];
AssertType(x = [y, await z, a], "any[]");
AssertType(x, "any");
AssertType([y, await z, a], "any[]");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
AssertType(a, "any");
}

async function arrayLiteral7() {
    x = [await y, z, await a];
AssertType(x = [await y, z, await a], "any[]");
AssertType(x, "any");
AssertType([await y, z, await a], "any[]");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(await a, "any");
AssertType(a, "any");
}

