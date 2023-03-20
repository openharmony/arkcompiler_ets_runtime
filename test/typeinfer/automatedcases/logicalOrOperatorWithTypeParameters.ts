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

// === tests/cases/conformance/expressions/binaryOperators/logicalOrOperator/logicalOrOperatorWithTypeParameters.ts ===
declare function AssertType(value:any, type:string):void;
function fn1<T, U>(t: T, u: U) {
    let r1 = t || t;
AssertType(r1, "T");
AssertType(t || t, "T");
AssertType(t, "T");
AssertType(t, "T");

    let r2: T = t || t;
AssertType(r2, "T");
AssertType(t || t, "T");
AssertType(t, "T");
AssertType(t, "T");

    let r3 = t || u;
AssertType(r3, "union");
AssertType(t || u, "union");
AssertType(t, "T");
AssertType(u, "U");

    let r4: {} = t || u;
AssertType(r4, "{}");
AssertType(t || u, "union");
AssertType(t, "T");
AssertType(u, "U");
}

function fn2<T, U/* extends T*/, V/* extends T*/>(t: T, u: U, v: V) {
    let r1 = t || u;
AssertType(r1, "union");
AssertType(t || u, "union");
AssertType(t, "T");
AssertType(u, "U");

    let r3 = u || u;
AssertType(r3, "U");
AssertType(u || u, "U");
AssertType(u, "U");
AssertType(u, "U");

    let r4: U = u || u;
AssertType(r4, "U");
AssertType(u || u, "U");
AssertType(u, "U");
AssertType(u, "U");

    let r5 = u || v;
AssertType(r5, "union");
AssertType(u || v, "union");
AssertType(u, "U");
AssertType(v, "V");

    let r6: {} = u || v;
AssertType(r6, "{}");
AssertType(u || v, "union");
AssertType(u, "U");
AssertType(v, "V");
}

function fn3<T extends { a: string; b: string }, U extends { a: string; b: number }>(t: T, u: U) {
    let r1 = t || u;
AssertType(r1, "union");
AssertType(t || u, "union");
AssertType(t, "T");
AssertType(u, "U");

    let r2: {} = t || u;
AssertType(r2, "{}");
AssertType(t || u, "union");
AssertType(t, "T");
AssertType(u, "U");

    let r3 = t || { a: '' };
AssertType(r3, "union");
AssertType(t || { a: '' }, "union");
AssertType(t, "T");
AssertType({ a: '' }, "{ a: string; }");
AssertType(a, "string");
AssertType('', "string");

    let r4: { a: string } = t || u;
AssertType(r4, "{ a: string; }");
AssertType(a, "string");
AssertType(t || u, "union");
AssertType(t, "T");
AssertType(u, "U");
}

