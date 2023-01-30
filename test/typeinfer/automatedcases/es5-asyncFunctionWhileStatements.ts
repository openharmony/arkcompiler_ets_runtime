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

// === tests/cases/compiler/es5-asyncFunctionWhileStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function whileStatement0() {
    while (x) { y; 
AssertType(x, "any");

AssertType(y, "any");
}
}

async function whileStatement1() {
    while (await x) { y; 
AssertType(await x, "any");

AssertType(x, "any");

AssertType(y, "any");
}
}

async function whileStatement2() {
    while (x) { await y; 
AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");
}
}

async function whileStatement3() {
    while (x) { continue; 
AssertType(x, "any");
}
}

async function whileStatement4() {
    while (await x) { continue; 
AssertType(await x, "any");

AssertType(x, "any");
}
}

async function whileStatement5() {
    while (x) { await y; continue; 
AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");
}
}

async function whileStatement6() {
    while (x) { if (1) continue; await y; 
AssertType(x, "any");

AssertType(1, "int");

AssertType(await y, "any");

AssertType(y, "any");
}
}

async function whileStatement7() {
    A: while (x) { continue A; 
AssertType(A, "any");

AssertType(x, "any");

AssertType(A, "any");
}
}

async function whileStatement8() {
    B: while (await x) { continue B; 
AssertType(B, "any");

AssertType(await x, "any");

AssertType(x, "any");

AssertType(B, "any");
}
}

async function whileStatement9() {
    C: while (x) { await y; continue C; 
AssertType(C, "any");

AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(C, "any");
}
}

async function whileStatement10() {
    D: while (x) { if (1) continue D; await y; 
AssertType(D, "any");

AssertType(x, "any");

AssertType(1, "int");

AssertType(D, "any");

AssertType(await y, "any");

AssertType(y, "any");
}
}

async function whileStatement11() {
    while (x) { break; 
AssertType(x, "any");
}
}

async function whileStatement12() {
    while (await x) { break; 
AssertType(await x, "any");

AssertType(x, "any");
}
}

async function whileStatement13() {
    while (x) { await y; break; 
AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");
}
}

async function whileStatement14() {
    while (x) { if (1) break; await y; 
AssertType(x, "any");

AssertType(1, "int");

AssertType(await y, "any");

AssertType(y, "any");
}
}

async function whileStatement15() {
    E: while (x) { break E; 
AssertType(E, "any");

AssertType(x, "any");

AssertType(E, "any");
}
}

async function whileStatement16() {
    F: while (await x) { break F; 
AssertType(F, "any");

AssertType(await x, "any");

AssertType(x, "any");

AssertType(F, "any");
}
}

async function whileStatement17() {
    G: while (x) { await y; break G; 
AssertType(G, "any");

AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(G, "any");
}
}

async function whileStatement18() {
    H: while (x) { if (1) break H; await y; 
AssertType(H, "any");

AssertType(x, "any");

AssertType(1, "int");

AssertType(H, "any");

AssertType(await y, "any");

AssertType(y, "any");
}
}

