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

// === tests/cases/compiler/es5-asyncFunctionConditionals.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function conditional0() {
    a = (await x) ? y : z;
AssertType(a = (await x) ? y : z, "any");
AssertType(a, "any");
AssertType((await x) ? y : z, "any");
AssertType((await x), "any");
AssertType(await x, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function conditional1() {
    a = x ? await y : z;
AssertType(a = x ? await y : z, "any");
AssertType(a, "any");
AssertType(x ? await y : z, "any");
AssertType(x, "any");
AssertType(await y, "any");
AssertType(y, "any");
AssertType(z, "any");
}

async function conditional2() {
    a = x ? y : await z;
AssertType(a = x ? y : await z, "any");
AssertType(a, "any");
AssertType(x ? y : await z, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(await z, "any");
AssertType(z, "any");
}

