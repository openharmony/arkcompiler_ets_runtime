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

// === tests/cases/conformance/types/rest/genericObjectRest.ts ===
declare function AssertType(value:any, type:string):void;
const a = 'a';
AssertType(a, "string");
AssertType('a', "string");

function f1<T extends { a: string, b: number }>(obj: T) {
    let { ...r0 } = obj;
AssertType(r0, "T");
AssertType(obj, "T");

    let { a: a1, ...r1 } = obj;
AssertType(a, "any");
AssertType(a1, "string");
AssertType(r1, "Omit<T, "a">");
AssertType(obj, "T");

    let { a: a2, b: b2, ...r2 } = obj;
AssertType(a, "any");
AssertType(a2, "string");
AssertType(b, "any");
AssertType(b2, "number");
AssertType(r2, "Omit<T, union>");
AssertType(obj, "T");

    let { 'a': a3, ...r3 } = obj;
AssertType(a3, "string");
AssertType(r3, "Omit<T, "a">");
AssertType(obj, "T");

    let { ['a']: a4, ...r4 } = obj;
AssertType('a', "string");
AssertType(a4, "string");
AssertType(r4, "Omit<T, "a">");
AssertType(obj, "T");

    let { [a]: a5, ...r5 } = obj;
AssertType(a, "string");
AssertType(a5, "string");
AssertType(r5, "Omit<T, "a">");
AssertType(obj, "T");
}

const sa = Symbol();
AssertType(sa, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

const sb = Symbol();
AssertType(sb, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

function f2<T extends { [sa]: string, [sb]: number }>(obj: T) {
    let { [sa]: a1, [sb]: b1, ...r1 } = obj;
AssertType(sa, "unique symbol");
AssertType(a1, "string");
AssertType(sb, "unique symbol");
AssertType(b1, "number");
AssertType(r1, "Omit<T, union>");
AssertType(obj, "T");
}

function f3<T, K1 extends keyof T, K2 extends keyof T>(obj: T, k1: K1, k2: K2) {
    let { [k1]: a1, [k2]: a2, ...r1 } = obj;
AssertType(k1, "K1");
AssertType(a1, "T[K1]");
AssertType(k2, "K2");
AssertType(a2, "T[K2]");
AssertType(r1, "Omit<T, union>");
AssertType(obj, "T");
}

type Item = { a: string, b: number, c: boolean };

function f4<K1 extends keyof Item, K2 extends keyof Item>(obj: Item, k1: K1, k2: K2) {
    let { [k1]: a1, [k2]: a2, ...r1 } = obj;
AssertType(k1, "K1");
AssertType(a1, "Item[K1]");
AssertType(k2, "K2");
AssertType(a2, "Item[K2]");
AssertType(r1, "Omit<Item, union>");
AssertType(obj, "Item");
}


