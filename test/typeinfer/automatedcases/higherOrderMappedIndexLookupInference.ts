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

// === tests/cases/compiler/higherOrderMappedIndexLookupInference.ts ===
declare function AssertType(value:any, type:string):void;
// @strict

function f1(a: <T>() => keyof T, b: <U>() => keyof U) {
    a = b;
AssertType(a = b, "<U>() => keyof U");
AssertType(a, "<T>() => keyof T");
AssertType(b, "<U>() => keyof U");

    b = a;
AssertType(b = a, "<T>() => keyof T");
AssertType(b, "<U>() => keyof U");
AssertType(a, "<T>() => keyof T");
}

function f2(a: <T, K extends keyof T>() => T[K], b: <U, L extends keyof U>() => U[L]) {
    a = b;
AssertType(a = b, "<U, L extends keyof U>() => U[L]");
AssertType(a, "<T, K extends keyof T>() => T[K]");
AssertType(b, "<U, L extends keyof U>() => U[L]");

    b = a;
AssertType(b = a, "<T, K extends keyof T>() => T[K]");
AssertType(b, "<U, L extends keyof U>() => U[L]");
AssertType(a, "<T, K extends keyof T>() => T[K]");
}

function f3(a: <T>() => { [K in keyof T]: T[K] }, b: <U>() => { [K in keyof U]: U[K] }) {
    a = b;
AssertType(a = b, "<U>() => { [K in keyof U]: U[K]; }");
AssertType(a, "<T>() => { [K in keyof T]: T[K]; }");
AssertType(b, "<U>() => { [K in keyof U]: U[K]; }");

    b = a;
AssertType(b = a, "<T>() => { [K in keyof T]: T[K]; }");
AssertType(b, "<U>() => { [K in keyof U]: U[K]; }");
AssertType(a, "<T>() => { [K in keyof T]: T[K]; }");
}

// Repro from #18338

type IdMapped<T> = { [K in keyof T]: T[K] }

declare const f: <T>() => IdMapped<T>;
AssertType(f, "<T>() => IdMapped<T>");

declare const g: <U>() => { [K in keyof U]: U[K] };
AssertType(g, "<U>() => { [K in keyof U]: U[K]; }");

const h: typeof g = f;
AssertType(h, "<U>() => { [K in keyof U]: U[K]; }");
AssertType(g, "<U>() => { [K in keyof U]: U[K]; }");
AssertType(f, "<T>() => IdMapped<T>");


