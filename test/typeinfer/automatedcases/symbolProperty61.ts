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

// === tests/cases/conformance/es6/Symbols/symbolProperty61.ts ===
declare function AssertType(value:any, type:string):void;
declare global {
  interface SymbolConstructor {
    readonly obs: symbol
  }
}

const observable: typeof Symbol.obs = Symbol.obs
AssertType(observable, "unique symbol");
AssertType(Symbol.obs, "unique symbol");
AssertType(Symbol, "SymbolConstructor");
AssertType(obs, "unique symbol");
AssertType(Symbol.obs, "unique symbol");

export class MyObservable<T> {
    constructor(private _val: T) {}

    subscribe(next: (val: T) => void) {
        next(this._val)
AssertType(next(this._val), "void");
AssertType(next, "(T) => void");
AssertType(this._val, "T");
AssertType(this, "this");
    }

    [observable]() {
AssertType(this, "this");
        return this
    }
}

type InteropObservable<T> = {
    [Symbol.obs]: () => { subscribe(next: (val: T) => void): void }
}

function from<T>(obs: InteropObservable<T>) {
AssertType(obs[Symbol.obs](), "{ subscribe((T) => void): void; }");
AssertType(obs[Symbol.obs], "() => { subscribe((T) => void): void; }");
AssertType(obs, "InteropObservable<T>");
AssertType(Symbol.obs, "unique symbol");
    return obs[Symbol.obs]()
}

from(new MyObservable(42))
AssertType(from(new MyObservable(42)), "{ subscribe((number) => void): void; }");
AssertType(from, "<T>(InteropObservable<T>) => { subscribe((T) => void): void; }");
AssertType(new MyObservable(42), "MyObservable<number>");
AssertType(MyObservable, "typeof MyObservable");
AssertType(42, "int");


