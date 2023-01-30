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

// === tests/cases/conformance/salsa/lateBoundClassMemberAssignmentJS.js ===
declare function AssertType(value:any, type:string):void;
const _sym = Symbol("_sym");
AssertType(_sym, "unique symbol");
AssertType(Symbol("_sym"), "unique symbol");
AssertType(Symbol, "SymbolConstructor");
AssertType("_sym", "string");

export class MyClass {
    constructor() {
        this[_sym] = "ok";
AssertType(this[_sym] = "ok", "string");
AssertType(this[_sym], "string");
AssertType(this, "this");
AssertType(_sym, "unique symbol");
AssertType("ok", "string");
    }

    method() {
        this[_sym] = "yep";
AssertType(this[_sym] = "yep", "string");
AssertType(this[_sym], "string");
AssertType(this, "this");
AssertType(_sym, "unique symbol");
AssertType("yep", "string");

        const x = this[_sym];
AssertType(x, "string");
AssertType(this[_sym], "string");
AssertType(this, "this");
AssertType(_sym, "unique symbol");
    }
}

