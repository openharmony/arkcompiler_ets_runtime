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

// === tests/cases/compiler/modularizeLibrary_UsingES5LibAndES6FeatureLibs.ts ===
declare function AssertType(value:any, type:string):void;
let s = Symbol();
AssertType(s, "symbol");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

let t = {};
AssertType(t, "{}");
AssertType({}, "{}");

let p = new Proxy(t, {});
AssertType(p, "{}");
AssertType(new Proxy(t, {}), "{}");
AssertType(Proxy, "ProxyConstructor");
AssertType(t, "{}");
AssertType({}, "{}");

Reflect.ownKeys({});
AssertType(Reflect.ownKeys({}), "(union)[]");
AssertType(Reflect.ownKeys, "(object) => (union)[]");
AssertType({}, "{}");

function* idGen() {
    let i = 10;
AssertType(i, "number");
AssertType(10, "int");

    while (i < 20) {
AssertType(i < 20, "boolean");
AssertType(i, "number");
AssertType(20, "int");

        yield i + 2;
AssertType(yield i + 2, "any");
AssertType(i + 2, "number");
AssertType(i, "number");
AssertType(2, "int");
    }
}


