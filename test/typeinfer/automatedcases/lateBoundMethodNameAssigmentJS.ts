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

// === tests/cases/compiler/lateBoundMethodNameAssigmentJS.js ===
declare function AssertType(value:any, type:string):void;
const _symbol = Symbol("_sym");
AssertType(_symbol, "unique symbol");
AssertType(Symbol("_sym"), "unique symbol");
AssertType(Symbol, "SymbolConstructor");
AssertType("_sym", "string");

export class MyClass {
    constructor() {
        this[_symbol] = this[_symbol].bind(this);
AssertType(this[_symbol] = this[_symbol].bind(this), "error");
AssertType(this[_symbol], "error");
AssertType(this, "this");
AssertType(_symbol, "unique symbol");
AssertType(this[_symbol].bind(this), "error");
AssertType(this[_symbol].bind, "error");
AssertType(this[_symbol], "any");
AssertType(this, "this");
AssertType(_symbol, "unique symbol");
AssertType(this, "this");
    }

    async [_symbol]() { }
}

