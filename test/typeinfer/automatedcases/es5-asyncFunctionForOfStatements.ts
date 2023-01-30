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

// === tests/cases/compiler/es5-asyncFunctionForOfStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function forOfStatement0() {
    for (x of y) { z; 
AssertType(x, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement1() {
    for (x of await y) { z; 
AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement2() {
    for (x of y) { await z; 
AssertType(x, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forOfStatement3() {
    for ((await x).a of y) { z; 
AssertType((await x).a, "any");

AssertType((await x), "any");

AssertType(await x, "any");

AssertType(x, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement4() {
    for (x.a of await y) { z; 
AssertType(x.a, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement5() {
    for (x.a of y) { await z; 
AssertType(x.a, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forOfStatement6() {
    for (let b of y) { z; 
AssertType(b, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement7() {
    for (let c of await y) { z; 
AssertType(c, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement8() {
    for (let d of y) { await z; 
AssertType(d, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forOfStatement9() {
    for ([x] of await y) { z; 
AssertType([x], "[any]");

AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement10() {
    for ([x] of y) { await z; 
AssertType([x], "[any]");

AssertType(x, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forOfStatement11() {
    for ([x = await a] of y) { z; 
AssertType([x = await a], "[any]");

AssertType(x = await a, "any");

AssertType(x, "any");

AssertType(await a, "any");

AssertType(a, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement12() {
    for ([x = a] of await y) { z; 
AssertType([x = a], "[any]");

AssertType(x = a, "any");

AssertType(x, "any");

AssertType(a, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement13() {
    for ([x = a] of y) { await z; 
AssertType([x = a], "[any]");

AssertType(x = a, "any");

AssertType(x, "any");

AssertType(a, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forOfStatement14() {
    for ({ x } of await y) { z; 
AssertType({ x }, "{ x: any; }");

AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement15() {
    for ({ x } of y) { await z; 
AssertType({ x }, "{ x: any; }");

AssertType(x, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forOfStatement16() {
    for ({ x = await a } of y) { z; 
AssertType({ x = await a }, "{ x?: any; }");

AssertType(x, "any");

AssertType(await a, "any");

AssertType(a, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement17() {
    for ({ x = a } of await y) { z; 
AssertType({ x = a }, "{ x?: any; }");

AssertType(x, "any");

AssertType(a, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forOfStatement18() {
    for ({ x = a } of y) { await z; 
AssertType({ x = a }, "{ x?: any; }");

AssertType(x, "any");

AssertType(a, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

