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

// === tests/cases/compiler/es5-asyncFunctionDoStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function doStatement0() {
    do { x; } while (y);
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement1() {
    do { await x; } while (y);
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement2() {
    do { x; } while (await y);
AssertType(x, "any");
AssertType(await y, "any");
AssertType(y, "any");
}

async function doStatement3() {
    do { continue; } while (y);
AssertType(y, "any");
}

async function doStatement4() {
    do { await x; continue; } while (y);
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement5() {
    do { if (1) continue; await x; } while (y);
AssertType(1, "int");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement6() {
    do { continue; } while (await y);
AssertType(await y, "any");
AssertType(y, "any");
}

async function doStatement7() {
    A: do { continue A; } while (y);
AssertType(A, "any");
AssertType(A, "any");
AssertType(y, "any");
}

async function doStatement8() {
    B: do { await x; continue B; } while (y);
AssertType(B, "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(B, "any");
AssertType(y, "any");
}

async function doStatement9() {
    C: do { if (1) continue C; await x; } while (y);
AssertType(C, "any");
AssertType(1, "int");
AssertType(C, "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement10() {
    D: do { continue D; } while (await y);
AssertType(D, "any");
AssertType(D, "any");
AssertType(await y, "any");
AssertType(y, "any");
}

async function doStatement11() {
    do { break; } while (y);
AssertType(y, "any");
}

async function doStatement12() {
    do { await x; break; } while (y);
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement13() {
    do { if (1) break; await x; } while (y);
AssertType(1, "int");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement14() {
    do { break; } while (await y);
AssertType(await y, "any");
AssertType(y, "any");
}

async function doStatement15() {
    E: do { break E; } while (y);
AssertType(E, "any");
AssertType(E, "any");
AssertType(y, "any");
}

async function doStatement16() {
    F: do { await x; break F; } while (y);
AssertType(F, "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(F, "any");
AssertType(y, "any");
}

async function doStatement17() {
    G: do { if (1) break G; await x; } while (y);
AssertType(G, "any");
AssertType(1, "int");
AssertType(G, "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
}

async function doStatement18() {
    H: do { break H; } while (await y);
AssertType(H, "any");
AssertType(H, "any");
AssertType(await y, "any");
AssertType(y, "any");
}

