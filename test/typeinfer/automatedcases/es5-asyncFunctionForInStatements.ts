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

// === tests/cases/compiler/es5-asyncFunctionForInStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function forInStatement0() {
    for (x in y) { z; 
AssertType(x, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forInStatement1() {
    for (x in await y) { z; 
AssertType(x, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forInStatement2() {
    for (x in y) { await z; 
AssertType(x, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forInStatement3() {
    for ((await x).a in y) { z; 
AssertType((await x).a, "any");

AssertType((await x), "any");

AssertType(await x, "any");

AssertType(x, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forInStatement4() {
    for (x.a in await y) { z; 
AssertType(x.a, "any");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forInStatement5() {
    for (x.a in y) { await z; 
AssertType(x.a, "any");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

async function forInStatement6() {
    for (let a in y) { z; 
AssertType(a, "string");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forInStatement7() {
    for (let b in await y) { z; 
AssertType(b, "string");

AssertType(await y, "any");

AssertType(y, "any");

AssertType(z, "any");
}
}

async function forInStatement8() {
    for (let c in y) { await z; 
AssertType(c, "string");

AssertType(y, "any");

AssertType(await z, "any");

AssertType(z, "any");
}
}

